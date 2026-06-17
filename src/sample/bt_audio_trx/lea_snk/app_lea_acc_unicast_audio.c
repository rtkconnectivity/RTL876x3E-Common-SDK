/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "audio.h"
#include "audio_probe.h"
#include "ascs_mgr.h"
#include "bap.h"
#include "bt_gatt_client.h"
#include "gap_conn_le.h"
#include "gap_iso_data.h"
#include "gap_vendor.h"
#include "mcp_client.h"
#include "metadata_def.h"
#include "pacs_def.h"
#include "sidetone.h"
#include "tmas_mgr.h"
#include "vcs_mgr.h"
#include "app_a2dp.h"
#include "app_audio_policy.h"
#include "app_ble_gap.h"
#include "app_bond.h"
#include "app_cfg.h"
#include "app_lea_acc_adv.h"
#include "app_lea_acc_ascs.h"
#include "app_lea_acc_ccp.h"
#include "app_lea_acc_csis.h"
#include "app_lea_acc_def.h"
#include "app_lea_acc_mcp.h"
#include "app_lea_acc_pacs.h"
#include "app_lea_acc_profile.h"
#include "app_lea_acc_vcs.h"
#include "app_lea_acc_vol_def.h"
#include "app_lea_acc_unicast_audio.h"
#include "app_lea_acc_mgr.h"
#include "app_main.h"
#if F_APP_SIDETONE_SUPPORT
#include "app_sidetone.h"
#endif
#include "app_sniff_mode_cs.h"

#if F_APP_LC3_PLUS_SUPPORT
#include "app_lea_acc_pacs.h"
#endif

#define CIS_PERMIT_MIN_PD_LOW_LATENCY_DELAY 4500
#define CIS_PERMIT_MIN_PD_DELAY 20000

#define UPSTREAM_ALGO_AUDIO     0

#define LEA_RECORD_PREQUEUE_PKT_NUM     4

typedef enum
{
    LEA_CHECK_NONE              = 0x00,
    LEA_CHECK_1                 = 0x01,
    LEA_CHECK_2                 = 0x02,
    LEA_CHECK_3                 = 0x03,

} T_LEA_CHECK_MIC_TYPE;

typedef struct uca_iso_elem
{
    struct uca_iso_elem *p_next;
    uint8_t *iso_data;
    uint16_t iso_seq;
    uint16_t iso_sdu_len;
    uint32_t timestamp;
} T_UCA_ISO_ELEM;

static bool app_lea_uca_diff_path = true;
static T_AUDIO_EFFECT_INSTANCE app_lea_uca_spk_eq = NULL;
static bool app_lea_uca_spk_eq_enabled = false;
static T_AUDIO_EFFECT_INSTANCE app_lea_uca_mic_eq = NULL;
static T_APP_LE_LINK *app_lea_uca_active_stream_link = NULL;
static T_OS_QUEUE app_lea_uca_left_ch_queue;
static T_OS_QUEUE app_lea_uca_right_ch_queue;
static T_OS_QUEUE app_lea_uca_upstream_queue;

static const uint32_t app_lea_sample_rate_freq_map[SAMPLING_FREQUENCY_CFG_384K] =
{
    8000, 11000, 16000, 22000, 24000, 32000, 44100, 48000, 88000, 96000, 176000, 192000, 384000
};

static void app_lea_uca_track_reconfig_write_length(T_APP_LE_LINK *p_link);
static void app_lea_uca_track_release(T_APP_LE_LINK *p_link, T_LEA_ASE_ENTRY *p_ase_entry);

T_APP_LE_LINK *app_lea_uca_get_stream_link(void)
{
    return app_lea_uca_active_stream_link;
}

static T_GAP_CAUSE app_lea_uca_send_iso_data(uint8_t *p_data, T_LEA_ASE_ENTRY *p_ase_entry,
                                             uint16_t iso_sdu_len,
                                             bool ts_flag, uint32_t timestamp)
{
    T_SYNCCLK_LATCH_INFO_TypeDef *p_latch_info = synclk_drv_time_get(SYNCCLK_ID4);
    uint32_t next_ap = 0;
    uint16_t iso_interval = 0;
    uint16_t seq_num;
    uint16_t handle = p_ase_entry->cis_conn_handle;
    uint8_t i = 0;
    T_UCA_ISO_ELEM *iso_elem = NULL;
    T_GAP_CAUSE ret = GAP_CAUSE_ERROR_UNKNOWN;

    if (p_ase_entry->prequeue_pkt_cnt < LEA_RECORD_PREQUEUE_PKT_NUM)
    {
        iso_elem = audio_probe_media_buffer_malloc(sizeof(T_UCA_ISO_ELEM));

        bool iso_enqueue = false;

        if (iso_elem)
        {
            iso_elem->iso_data = audio_probe_media_buffer_malloc(iso_sdu_len);

            if (iso_elem->iso_data)
            {
                memcpy(iso_elem->iso_data, p_data, iso_sdu_len);
                iso_elem->iso_sdu_len = iso_sdu_len;
                iso_elem->iso_seq = p_ase_entry->pkt_seq;
                iso_elem->timestamp = p_latch_info->exp_sync_clock;

                iso_enqueue = true;
            }
        }

        if (iso_enqueue)
        {
            os_queue_in(&app_lea_uca_upstream_queue, iso_elem);
            p_ase_entry->prequeue_pkt_cnt++;

            ret = GAP_CAUSE_SUCCESS;
        }

        APP_PRINT_TRACE1("app_lea_uca_send_iso_data: prequeue seq 0x%x", iso_elem->iso_seq);
    }
    else
    {
        /* dequeue iso and send all to prevent lowerstack starvation issue
        *  (due to pkt not generate uniformly)
        */

        iso_elem = os_queue_out(&app_lea_uca_upstream_queue);

        while (iso_elem != NULL)
        {
            gap_iso_send_data(iso_elem->iso_data, handle, iso_elem->iso_sdu_len,
                              ts_flag, iso_elem->timestamp, iso_elem->iso_seq);

            audio_probe_media_buffer_free(iso_elem->iso_data);
            audio_probe_media_buffer_free(iso_elem);

            iso_elem = os_queue_out(&app_lea_uca_upstream_queue);
        }

        ret = gap_iso_send_data(p_data, handle, iso_sdu_len,
                                ts_flag, p_latch_info->exp_sync_clock, p_ase_entry->pkt_seq);

        APP_PRINT_TRACE2("app_lea_uca_send_iso_data: send seq 0x%x len %d", p_ase_entry->pkt_seq,
                         iso_sdu_len);
    }

    p_ase_entry->pkt_seq++;

    return ret;
}

static void app_lea_uca_le_disconnect_cb(uint8_t conn_id, uint8_t local_disc_cause,
                                         uint16_t disc_cause)
{
    APP_PRINT_TRACE2("app_lea_uca_le_disconnect_cb: disc_cause 0x%x, cause 0x%x", local_disc_cause,
                     disc_cause);
    T_APP_LE_LINK *p_link;
    uint8_t lea_link_state = LEA_LINK_IDLE;

    p_link = app_link_find_le_link_by_conn_id(conn_id);
    if (p_link)
    {
        lea_link_state = p_link->lea_link_state;
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
        app_lea_uca_link_sm(p_link->conn_handle, LEA_DISCONNECT, &disc_cause);
#endif
    }

    if ((disc_cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE) &&
         (lea_link_state != LEA_LINK_IDLE)) ||
        (disc_cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT))  ||
        (disc_cause == (HCI_ERR | HCI_ERR_FAIL_TO_ESTABLISH_CONN))  ||
        (disc_cause == (HCI_ERR | HCI_ERR_INSTANT_PASSED))  ||
        (disc_cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT))  ||
        ((disc_cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)) &&
         (local_disc_cause == LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED)))
    {
        app_lea_mgr_adv_sm(LEA_CIG_START, &disc_cause);
    }
}

static uint16_t app_lea_uca_handle_cccd_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    uint8_t i = 0;

    APP_PRINT_INFO1("app_lea_uca_handle_cccd_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO:
        {
            T_SERVER_ATTR_CCCD_INFO *p_cccd = (T_SERVER_ATTR_CCCD_INFO *)buf;

            if (p_cccd->char_uuid >= PACS_UUID_CHAR_SINK_PAC &&
                p_cccd->char_uuid <= PACS_UUID_CHAR_SUPPORTED_AUDIO_CONTEXTS)
            {
                uint8_t conn_id;
                T_APP_LE_LINK *p_link;

                le_get_conn_id_by_handle(p_cccd->conn_handle, &conn_id);
                p_link = app_link_find_le_link_by_conn_id(conn_id);
                if (p_link && ((p_link->lea_device & APP_LEA_PACS_CCCD_ENABLED) == 0))
                {
                    p_link->lea_device |= APP_LEA_PACS_CCCD_ENABLED;
                    if (p_link->lea_link_state == LEA_LINK_IDLE)
                    {
                        app_lea_uca_link_sm(p_link->conn_handle, LEA_CONNECT, NULL);
                    }
                    app_lea_mgr_adv_sm(LEA_ADV_STOP, NULL);
                    app_link_reg_le_link_disc_cb(conn_id, app_lea_uca_le_disconnect_cb);
                }
            }
            APP_PRINT_INFO5("LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO: conn_handle 0x%x, service_id %d, char_uuid 0x%x, ccc_bits 0x%x, param %d",
                            p_cccd->conn_handle,
                            p_cccd->service_id,
                            p_cccd->char_uuid,
                            p_cccd->ccc_bits,
                            p_cccd->param);
        }
        break;

    default:
        break;
    }
    return cb_result;
}

static uint16_t app_lea_uca_handle_ascs_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    uint8_t i = 0;

    APP_PRINT_INFO1("app_lea_uca_handle_ascs_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_ASCS_ASE_STATE:
        {
            T_ASCS_ASE_STATE *p_data = (T_ASCS_ASE_STATE *)buf;

            APP_PRINT_INFO3("LE_AUDIO_MSG_ASCS_ASE_STATE: conn_handle 0x%x, ase_id %d, ase_state %d",
                            p_data->conn_handle,
                            p_data->ase_data.ase_id,
                            p_data->ase_data.ase_state);
            switch (p_data->ase_data.ase_state)
            {
            case ASE_STATE_CODEC_CONFIGURED:
                {
                    T_LEA_ASE_ENTRY *p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_data->conn_handle,
                                                                               (void *)(&p_data->ase_data.ase_id));
                    if (p_ase_entry != NULL)
                    {
                        if (p_data->ase_data.direction == SERVER_AUDIO_SINK)
                        {
                            p_ase_entry->path_direction = DATA_PATH_OUTPUT_FLAG;
                        }
                        else
                        {
                            p_ase_entry->path_direction = DATA_PATH_INPUT_FLAG;
                        }
                        app_lea_uca_link_sm(p_data->conn_handle, LEA_CODEC_CONFIGURED, p_ase_entry);
                    }
                }
                break;

            case ASE_STATE_ENABLING:
                {
                    app_lea_uca_link_sm(p_data->conn_handle, LEA_ENABLEING, NULL);
                }
                break;

            case ASE_STATE_STREAMING:
                {
                    APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_STREAMING: cig_id %d, cis_id %d, metadata[%d] %b",
                                    p_data->ase_data.param.streaming.cig_id,
                                    p_data->ase_data.param.streaming.cis_id,
                                    p_data->ase_data.param.streaming.metadata_length,
                                    TRACE_BINARY(p_data->ase_data.param.streaming.metadata_length,
                                                 p_data->ase_data.param.streaming.p_metadata));
                    app_lea_uca_link_sm(p_data->conn_handle, LEA_STREAMING, NULL);
                }
                break;

            case ASE_STATE_DISABLING:
                {
                    APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_DISABLING: cig_id %d, cis_id %d, metadata[%d] %b",
                                    p_data->ase_data.param.disabling.cig_id,
                                    p_data->ase_data.param.disabling.cis_id,
                                    p_data->ase_data.param.disabling.metadata_length,
                                    TRACE_BINARY(p_data->ase_data.param.disabling.metadata_length,
                                                 p_data->ase_data.param.disabling.p_metadata));
                    // ascs_action_rec_stop_ready(p_data->conn_handle, ASCS_TEST_ASE_ID);
                }
                break;

            case ASE_STATE_RELEASING:
                {
                    app_lea_uca_link_sm(p_data->conn_handle, LEA_RELEASING, NULL);
                }
                break;

            default:
                break;
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_ENABLE_IND:
        {
            T_ASCS_CP_ENABLE_IND *p_data = (T_ASCS_CP_ENABLE_IND *)buf;
            bool need_return = false;

            if (app_hfp_get_call_status() != APP_CALL_IDLE)
            {
                cb_result = ASE_CP_RESP_INSUFFICIENT_RESOURCE;
                break;
            }
            app_lea_uca_link_sm(p_data->conn_handle, LEA_ENABLE, p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_DISABLE_IND:
        {
            T_ASCS_CP_DISABLE_IND *p_data = (T_ASCS_CP_DISABLE_IND *)buf;

            app_lea_uca_link_sm(p_data->conn_handle, LEA_PAUSE, NULL);
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_UPDATE_METADATA_IND:
        {
            T_ASCS_CP_UPDATE_METADATA_IND *p_data = (T_ASCS_CP_UPDATE_METADATA_IND *)buf;

            app_lea_uca_link_sm(p_data->conn_handle, LEA_METADATA_UPDATE, p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH:
        {
            T_ASCS_SETUP_DATA_PATH *p_data = (T_ASCS_SETUP_DATA_PATH *)buf;

            app_lea_uca_link_sm(p_data->conn_handle, LEA_SETUP_DATA_PATH, p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_REMOVE_DATA_PATH:
        {
            T_ASCS_REMOVE_DATA_PATH *p_data = (T_ASCS_REMOVE_DATA_PATH *)buf;

            app_lea_uca_link_sm(p_data->conn_handle, LEA_REMOVE_DATA_PATH, p_data);
        }
        break;

#if F_APP_FRAUNHOFER_SUPPORT
    case LE_AUDIO_MSG_ASCS_CP_CONFIG_CODEC_IND:
        {
            T_LEA_ASE_ENTRY *p_ase_entry = NULL;
            T_ASCS_CP_CONFIG_CODEC_IND *p_data = (T_ASCS_CP_CONFIG_CODEC_IND *)buf;

            for (uint8_t i = 0; i < p_data->number_of_ase; i++)
            {
                p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_data->conn_handle,
                                                          (void *)(&p_data->param[i].data.ase_id));
                if (p_ase_entry != NULL)
                {
                    uint16_t idx = 0;
                    uint16_t length;
                    uint8_t type;

                    for (; idx < p_data->param[i].data.codec_spec_cfg_len;)
                    {
                        length = p_data->param[i].p_codec_spec_cfg[idx];
                        idx++;
                        type = p_data->param[i].p_codec_spec_cfg[idx];

                        if (type == FRAUNHOFER_CFG_FD && length == 2)
                        {
                            uint8_t frame_duration = p_data->param[i].p_codec_spec_cfg[idx + 1];
                            p_ase_entry->codec_cfg.frame_duration = frame_duration;
                            break;
                        }
                        idx += length;
                    }
                }
            }
        }
        break;
#endif

    case LE_AUDIO_MSG_ASCS_GET_PREFER_QOS:
        {
            T_ASCS_GET_PREFER_QOS *p_data = (T_ASCS_GET_PREFER_QOS *)buf;

            app_lea_uca_link_sm(p_data->conn_handle, LEA_PREFER_QOS, p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_CIS_REQUEST_IND:
        {
            T_ASCS_CIS_REQUEST_IND *p_data = (T_ASCS_CIS_REQUEST_IND *)buf;

            APP_PRINT_INFO6("LE_AUDIO_MSG_ASCS_CIS_REQUEST_IND: conn_handle 0x%x, cis_conn_handle 0x%x, snk_ase_id %d, snk_ase_state %d, src_ase_id %d, src_ase_state %d",
                            p_data->conn_handle, p_data->cis_conn_handle,
                            p_data->snk_ase_id, p_data->snk_ase_state,
                            p_data->src_ase_id, p_data->src_ase_state);

            if (app_hfp_get_call_status() != APP_CALL_IDLE)
            {
                cb_result = ASE_CP_RESP_INSUFFICIENT_RESOURCE;
                break;
            }

            if (app_lea_mcp_get_active_conn_handle() == p_data->conn_handle ||
                app_lea_ccp_get_active_conn_handle() == p_data->conn_handle)
            {
                app_lea_uca_active_stream_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_AUDIO_CONTEXTS_CHECK_IND:
        {
            T_ASCS_AUDIO_CONTEXTS_CHECK_IND *p_data = (T_ASCS_AUDIO_CONTEXTS_CHECK_IND *)buf;

            APP_PRINT_INFO5("LE_AUDIO_MSG_ASCS_AUDIO_CONTEXTS_CHECK_IND: conn_handle 0x%x, update 0x%x, ase_id %d, direction %d, context 0x%x",
                            p_data->conn_handle, p_data->is_update_metadata,
                            p_data->ase_id, p_data->direction,
                            p_data->available_contexts);
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);

            if (p_link)
            {
                uint16_t available_contexts;
                if (p_data->direction == SERVER_AUDIO_SINK)
                {
                    available_contexts = p_link->sink_available_contexts;
                }
                else
                {
                    available_contexts = p_link->source_available_contexts;
                }

                if ((available_contexts & p_data->available_contexts) == 0)
                {
                    cb_result = BLE_AUDIO_CB_RESULT_REJECT;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CIS_DISCONN_INFO:
        {
            T_ASCS_CIS_DISCONN_INFO *p_data = (T_ASCS_CIS_DISCONN_INFO *)buf;
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);

            if (p_link)
            {
                T_LEA_ASE_ENTRY *p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CIS_CONN, p_data->conn_handle,
                                                                           (void *)(&p_data->cis_conn_handle));

                if (p_ase_entry)
                {
                    if (p_ase_entry->path_direction == DATA_PATH_OUTPUT_FLAG)
                    {
                        p_link->stream_channel_allocation &= ~p_ase_entry->codec_cfg.audio_channel_allocation;
                    }
                }
                app_lea_uca_track_reconfig_write_length(p_link);
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CIS_CONN_INFO:
        {
            T_ASCS_CIS_CONN_INFO *p_data = (T_ASCS_CIS_CONN_INFO *)buf;
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);

            if (p_link)
            {
                app_lea_uca_track_reconfig_write_length(p_link);
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

static uint16_t app_lea_uca_handle_mcp_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    APP_PRINT_INFO1("app_lea_uca_handle_mcp_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_MCP_CLIENT_DIS_DONE:
        {
            T_APP_LE_LINK *p_link;
            T_MCP_CLIENT_DIS_DONE *p_dis_done = (T_MCP_CLIENT_DIS_DONE *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_dis_done->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_dis_done->is_found)
            {
                if (p_link->gmcs)
                {
                    app_lea_mcp_set_active_conn_handle(p_link->conn_handle);
                }
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_READ_RESULT:
        {
            T_APP_LE_LINK *p_link;
            T_MCP_CLIENT_READ_RESULT *p_read_result = (T_MCP_CLIENT_READ_RESULT *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_read_result->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_read_result->cause == GAP_SUCCESS)
            {
                if (p_read_result->char_uuid == MCS_UUID_CHAR_MEDIA_STATE)
                {
                    if (p_link->media_state == MCS_MEDIA_STATE_PLAYING)
                    {
                        app_lea_mcp_set_active_conn_handle(p_read_result->conn_handle);
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_NOTIFY:
        {
            T_APP_LE_LINK *p_link;
            T_MCP_CLIENT_NOTIFY *p_notify_result = (T_MCP_CLIENT_NOTIFY *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_notify_result->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            switch (p_notify_result->char_uuid)
            {
            case MCS_UUID_CHAR_MEDIA_STATE:
                {
                    app_lea_uca_link_sm(p_link->conn_handle, LEA_MCP_STATE, NULL);
                }
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

static uint16_t app_lea_uca_handle_ccp_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    APP_PRINT_INFO1("app_lea_uca_handle_ccp_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_CCP_CLIENT_DIS_DONE:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_CLIENT_DIS_DONE *p_dis_done = (T_CCP_CLIENT_DIS_DONE *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_dis_done->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_dis_done->is_found)
            {
                if (p_link->gtbs)
                {
                    if (app_link_get_le_link_num() == 1)
                    {
                        app_lea_ccp_set_active_conn_handle(p_link->conn_handle);
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_READ_RESULT:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_CLIENT_READ_RESULT *p_read_result = (T_CCP_CLIENT_READ_RESULT *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_read_result->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_read_result->cause == GAP_SUCCESS)
            {
                if (p_read_result->char_uuid == TBS_UUID_CHAR_CALL_STATE)
                {
                    if (p_read_result->data.call_state.call_state_len >= CCP_CALL_STATE_CHARA_LEN)
                    {
                        app_lea_uca_link_sm(p_link->conn_handle, LEA_CCP_READ_RESULT, p_read_result);
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_NOTIFY:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_CLIENT_NOTIFY *p_notify_data = (T_CCP_CLIENT_NOTIFY *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_notify_data->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if ((p_notify_data->char_uuid == TBS_UUID_CHAR_CALL_STATE) ||
                (p_notify_data->char_uuid == TBS_UUID_CHAR_BEARER_LIST_CURRENT_CALLS) ||
                (p_notify_data->char_uuid == TBS_UUID_CHAR_INCOMING_CALL))
            {
                if (app_hfp_get_call_status() != APP_CALL_IDLE)
                {
                    cb_result = BLE_AUDIO_CB_RESULT_REJECT;
                    break;
                }
            }

            if (p_notify_data->char_uuid == TBS_UUID_CHAR_CALL_STATE)
            {
                if (p_notify_data->data.call_state.call_state_len >= CCP_CALL_STATE_CHARA_LEN)
                {
                    app_lea_uca_link_sm(p_link->conn_handle, LEA_CCP_CALL_STATE, p_notify_data);
                }
            }

            if (p_notify_data->char_uuid == TBS_UUID_CHAR_BEARER_LIST_CURRENT_CALLS)
            {
                if (p_notify_data->data.bearer_list_current_calls.bearer_list_current_calls_len)
                {
                    app_lea_uca_link_sm(p_link->conn_handle, LEA_CCP_CALL_STATE, p_notify_data);
                }
            }

            if (p_notify_data->char_uuid == TBS_UUID_CHAR_INCOMING_CALL)
            {
                if (p_notify_data->data.incoming_call.uri_len)
                {
                    app_lea_uca_link_sm(p_link->conn_handle, LEA_CCP_CALL_STATE, p_notify_data);
                }
            }

            if (p_notify_data->char_uuid == TBS_UUID_CHAR_TERMINATION_REASON)
            {
                app_lea_uca_link_sm(p_link->conn_handle, LEA_CCP_TERM_NOTIFY, p_notify_data);
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

static uint16_t app_lea_uca_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    uint16_t msg_group;

    msg_group = msg & 0xFF00;
    switch (msg_group)
    {
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    case LE_AUDIO_MSG_GROUP_SERVER:
        cb_result = app_lea_uca_handle_cccd_msg(msg, buf);
        break;

    case LE_AUDIO_MSG_GROUP_ASCS:
        cb_result = app_lea_uca_handle_ascs_msg(msg, buf);
        break;
#endif
#if F_APP_MCP_SUPPORT
    case LE_AUDIO_MSG_GROUP_MCP_CLIENT:
        cb_result = app_lea_uca_handle_mcp_msg(msg, buf);
        break;
#endif
#if F_APP_CCP_SUPPORT
    case LE_AUDIO_MSG_GROUP_CCP_CLIENT:
        cb_result = app_lea_uca_handle_ccp_msg(msg, buf);
        break;
#endif
    default:
        break;
    }

    return cb_result;
}

T_AUDIO_EFFECT_INSTANCE app_lea_uca_get_eq_instance(void)
{
    return app_lea_uca_spk_eq;
}

T_AUDIO_EFFECT_INSTANCE *app_lea_uca_p_eq_instance(void)
{
    return &app_lea_uca_spk_eq;
}

bool *app_lea_uca_get_eq_abled(void)
{
    return &app_lea_uca_spk_eq_enabled;
}

bool app_lea_uca_get_diff_path(void)
{
    return app_lea_uca_diff_path;
}

static void app_lea_uca_vcs_volume_change(T_APP_LE_LINK *p_link)
{
    T_LEA_ASE_ENTRY *p_ase_entry;
    uint16_t audio_context = 0;
    uint8_t result = 0;

    if (app_lea_ccp_get_call_status() != APP_CALL_IDLE)
    {
        if (p_link->conn_handle != app_lea_ccp_get_active_conn_handle())
        {
            result = 1;
            goto RESULT_FAIL;
        }
    }
    else
    {
        if (p_link->conn_handle != app_lea_mcp_get_active_conn_handle())
        {
            result = 2;
            goto RESULT_FAIL;
        }
    }

    p_ase_entry = app_lea_ascs_find_ase_entry_by_direction(p_link, DATA_PATH_OUTPUT_FLAG);

    if (p_ase_entry != NULL)
    {
        uint8_t level;

        level = VOLUME_LEVEL(app_cfg_nv.lea_vol_setting);
        if (level == 0 && app_cfg_nv.lea_vol_setting)
        {
            level = 1;
        }

        audio_track_volume_out_set(p_ase_entry->track_handle, level);

        if (app_cfg_nv.lea_vol_out_mute == VCS_MUTED)
        {
            audio_track_volume_out_mute(p_ase_entry->track_handle);
        }
        else if (app_cfg_nv.lea_vol_out_mute == VCS_NOT_MUTED)
        {
            audio_track_volume_out_unmute(p_ase_entry->track_handle);
        }
    }
    else
    {
        result = 3;
        goto RESULT_FAIL;
    }

    return;

RESULT_FAIL:
    APP_PRINT_INFO1("app_lea_uca_vcs_volume_change: fail cause %d", -result);
}

static void app_lea_uca_dump_ase_info(T_APP_LE_LINK *p_link)
{
    for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
    {
        T_LEA_ASE_ENTRY *p_ase_entry = &p_link->lea_ase_entry[i];

        APP_PRINT_TRACE8("app_lea_uca_dump_ase_info: used %d, conn_handle 0x%02X, ase_id %d, \
context 0x%02X, cis_conn_handle 0x%02X, presentation_delay 0x%02X, track_handle 0x%02X, frame_num %d",
                         p_ase_entry->used,
                         p_ase_entry->conn_handle,
                         p_ase_entry->ase_id,
                         p_ase_entry->audio_context,
                         p_ase_entry->cis_conn_handle,
                         p_ase_entry->presentation_delay,
                         p_ase_entry->track_handle,
                         p_ase_entry->frame_num);
    }
}

void app_lea_uca_eq_release(T_AUDIO_EFFECT_INSTANCE *eq_instance)
{
    if (*eq_instance != NULL)
    {
        eq_release(*eq_instance);
        *eq_instance = NULL;
    }
}

void app_lea_uca_eq_detach(T_AUDIO_STREAM_TYPE stream_type)
{
    //Detach EQ effect
    if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        app_lea_uca_eq_release(&app_lea_uca_spk_eq);
    }
    else
    {
        if (stream_type == AUDIO_STREAM_TYPE_RECORD)
        {
            app_lea_uca_eq_release(&app_lea_uca_mic_eq);
        }
        else if (stream_type == AUDIO_STREAM_TYPE_VOICE)
        {
            app_lea_uca_eq_release(&app_lea_uca_spk_eq);
            app_lea_uca_eq_release(&app_lea_uca_mic_eq);
            app_eq_change_audio_eq_mode(true);
        }
    }

    APP_PRINT_TRACE1("app_lea_uca_eq_detach: stream_type %d", stream_type);
}

void app_lea_uca_voice_eq_setting(T_AUDIO_EFFECT_INSTANCE *eq_spk_instance,
                                  T_AUDIO_EFFECT_INSTANCE *eq_mic_instance,
                                  T_AUDIO_TRACK_HANDLE audio_track_handle)
{
    if (*eq_spk_instance)
    {
        eq_release(*eq_spk_instance);
    }

    if (*eq_mic_instance)
    {
        eq_release(*eq_mic_instance);
    }

    app_eq_change_audio_eq_mode(true);

    app_eq_idx_check_accord_mode();

    *eq_spk_instance = app_eq_create(EQ_CONTENT_TYPE_VOICE, EQ_STREAM_TYPE_VOICE, SPK_SW_EQ,
                                     app_db.spk_eq_mode, app_cfg_nv.eq_idx);

    if (*eq_spk_instance != NULL)
    {
        eq_enable(*eq_spk_instance);
        audio_track_effect_attach(audio_track_handle, *eq_spk_instance);
    }

    *eq_mic_instance = app_eq_create(EQ_CONTENT_TYPE_VOICE, EQ_STREAM_TYPE_VOICE, MIC_SW_EQ,
                                     VOICE_MIC_MODE, app_cfg_nv.eq_idx);

    if (*eq_mic_instance != NULL)
    {
        eq_enable(*eq_mic_instance);
        audio_track_effect_attach(audio_track_handle, *eq_mic_instance);
    }
}

void app_lea_uca_record_eq_setting(T_AUDIO_EFFECT_INSTANCE *eq_instance,
                                   T_AUDIO_TRACK_HANDLE audio_track_handle)
{
    if (*eq_instance)
    {
        eq_release(*eq_instance);
    }

    *eq_instance = app_eq_create(EQ_CONTENT_TYPE_RECORD, EQ_STREAM_TYPE_RECORD, MIC_SW_EQ,
                                 VOICE_MIC_MODE, 0);

    if (*eq_instance != NULL)
    {
        eq_enable(*eq_instance);
        audio_track_effect_attach(audio_track_handle, *eq_instance);
    }
}


void app_lea_uca_eq_setting(T_AUDIO_EFFECT_INSTANCE *eq_instance,  bool *audio_eq_enabled,
                            T_AUDIO_TRACK_HANDLE audio_track_handle)
{
    if (*eq_instance)
    {
        eq_release(*eq_instance);
    }

    app_eq_idx_check_accord_mode();
    *eq_instance = app_eq_create(EQ_CONTENT_TYPE_AUDIO, EQ_STREAM_TYPE_AUDIO, SPK_SW_EQ,
                                 app_db.spk_eq_mode,
                                 app_cfg_nv.eq_idx);
    *audio_eq_enabled = false;

    if (*eq_instance != NULL)
    {
        //not set default eq when audio connect,enable when set eq para from SRC
        if (!app_db.eq_ctrl_by_src)
        {
            app_eq_audio_eq_enable(eq_instance, audio_eq_enabled);
        }
#if (F_APP_USER_EQ_SUPPORT == 1)
        else if (app_eq_is_valid_user_eq_index(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx))
        {
            app_eq_audio_eq_enable(eq_instance, audio_eq_enabled);
        }
#endif
        audio_track_effect_attach(audio_track_handle, *eq_instance);
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
        app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
#if (F_APP_USER_EQ_SUPPORT == 1)
        if (app_eq_is_valid_user_eq_index(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx))
        {
            app_report_eq_idx(EQ_INDEX_REPORT_BY_GET_UNSAVED_EQ);
        }
        else
#endif
        {
            app_report_eq_idx(EQ_INDEX_REPORT_BY_PLAYING);
        }
    }
}

void app_lea_uca_eq_attach(T_AUDIO_STREAM_TYPE stream_type, T_AUDIO_TRACK_HANDLE track_handle)
{
    // Attach EQ effect
    if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        app_lea_uca_eq_setting(&app_lea_uca_spk_eq, &app_lea_uca_spk_eq_enabled,
                               track_handle);
    }
    else
    {
        if (stream_type == AUDIO_STREAM_TYPE_RECORD)
        {
            app_lea_uca_record_eq_setting(&app_lea_uca_mic_eq, track_handle);
        }
        else if (stream_type == AUDIO_STREAM_TYPE_VOICE)
        {
            app_lea_uca_voice_eq_setting(&app_lea_uca_spk_eq, &app_lea_uca_mic_eq, track_handle);
        }
    }

    APP_PRINT_TRACE2("app_lea_uca_eq_attach: stream_type %d, track_handle 0x%x",
                     stream_type, track_handle);
}

void app_lea_uca_sidetone_attach(T_LEA_ASE_ENTRY *p_ase_entry)
{
    if (p_ase_entry == NULL || p_ase_entry->track_handle == NULL)
    {
        return;
    }

    if (p_ase_entry->stream_type == AUDIO_STREAM_TYPE_VOICE ||
        p_ase_entry->stream_type == AUDIO_STREAM_TYPE_RECORD)
    {
        if (app_dsp_cfg_sidetone.hw_enable)
        {
            audio_volume_in_unmute(p_ase_entry->stream_type);
        }

        p_ase_entry->sidetone_instance = app_sidetone_attach(p_ase_entry->track_handle,
                                                             app_dsp_cfg_sidetone);
    }
}

void app_lea_uca_sidetone_detach(T_LEA_ASE_ENTRY *p_ase_entry)
{
    if (p_ase_entry == NULL || p_ase_entry->track_handle == NULL)
    {
        return;
    }

    if (p_ase_entry->stream_type == AUDIO_STREAM_TYPE_VOICE ||
        p_ase_entry->stream_type == AUDIO_STREAM_TYPE_RECORD)
    {
        app_sidetone_detach(p_ase_entry->track_handle, p_ase_entry->sidetone_instance);
    }
}

static bool app_lea_uca_free_link(T_APP_LE_LINK *p_link)
{
    uint8_t i;

    for (i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
    {
        T_LEA_ASE_ENTRY *p_ase_entry = &p_link->lea_ase_entry[i];

        if (p_ase_entry->used == true)
        {
            if (p_ase_entry->track_handle != NULL)
            {
                ascs_action_release(p_ase_entry->conn_handle, p_ase_entry->ase_id);
                app_lea_uca_sidetone_detach(p_ase_entry);
                audio_track_release(p_ase_entry->track_handle);
            }

            app_lea_uca_eq_detach(p_ase_entry->stream_type);

            memset(p_ase_entry, 0, sizeof(T_LEA_ASE_ENTRY));
        }
    }

    memset(p_link->lea_call_entry, 0, sizeof(T_LEA_CALL_ENTRY) * CCP_CALL_ENTRY_NUM);
    p_link->call_status = APP_CALL_IDLE;
    if (p_link->active_call_uri != NULL)
    {
        free(p_link->active_call_uri);
        p_link->active_call_uri = NULL;
    }

    return true;
}

static uint8_t app_lea_uca_vcs(void)
{
    T_VCS_PARAM vcs_param;

    vcs_get_param(&vcs_param);
    if (vcs_param.volume_setting != app_cfg_nv.lea_vol_setting ||
        vcs_param.mute != app_cfg_nv.lea_vol_out_mute)
    {
#if F_APP_LE_AUDIO_DISABLE_ABSOLUTE_VOLUME
        vcs_param.volume_setting = app_cfg_nv.lea_vol_setting;
        vcs_param.mute = app_cfg_nv.lea_vol_out_mute;
#else
        app_cfg_nv.lea_vol_setting = vcs_param.volume_setting;
        app_cfg_nv.lea_vol_out_mute = vcs_param.mute;
#endif
    }

    return VOLUME_LEVEL(vcs_param.volume_setting);
}

static bool app_lea_uca_create_check(T_APP_LE_LINK *p_link, T_LEA_ASE_ENTRY *p_ase_entry,
                                     bool clean)
{
    if (!app_lea_uca_diff_path && p_ase_entry->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
    {
        for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
        {
            if (p_link->lea_ase_entry[i].used == true && p_ase_entry != &p_link->lea_ase_entry[i])
            {
                if (p_link->lea_ase_entry[i].audio_context == AUDIO_CONTEXT_CONVERSATIONAL &&
                    p_link->lea_ase_entry[i].path_direction != p_ase_entry->path_direction)
                {
                    if (clean && p_link->lea_ase_entry[i].track_handle == p_ase_entry->track_handle)
                    {
                        p_link->lea_ase_entry[i].track_handle = NULL;
                        return true;
                    }
                    else
                    {
                        if (p_link->lea_ase_entry[i].track_handle)
                        {
                            p_ase_entry->track_handle = p_link->lea_ase_entry[i].track_handle;
                            return true;
                        }
                    }
                }
            }
        }

    }
    return false;
}

static bool app_lea_umr_is_voice_track_existed(T_APP_LE_LINK *p_link,
                                               T_LEA_ASE_ENTRY *p_ase_entry)
{
    for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
    {
        if ((p_link->lea_ase_entry[i].used == true) && (&p_link->lea_ase_entry[i] != p_ase_entry))
        {
            if (p_link->lea_ase_entry[i].path_direction != p_ase_entry->path_direction)
            {
                if (p_link->lea_ase_entry[i].track_handle)
                {
                    p_ase_entry->track_handle = p_link->lea_ase_entry[i].track_handle;
                    p_ase_entry->stream_type = AUDIO_STREAM_TYPE_VOICE;
                    p_ase_entry->track_write_len = p_link->lea_ase_entry[i].track_write_len;
                    p_ase_entry->frame_num = p_link->lea_ase_entry[i].frame_num;
                    p_link->cis_right_ch_conn_handle = 0x0;
                    return true;
                }
            }
        }
    }
    return false;
}

static bool app_lea_umr_is_input_path_enabled(T_APP_LE_LINK *p_link)
{
    for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
    {
        T_LEA_ASE_ENTRY p_ase_entry = p_link->lea_ase_entry[i];

        if (p_ase_entry.used == true &&
            p_ase_entry.control_point_enable == true &&
            p_ase_entry.path_direction == DATA_PATH_INPUT_FLAG)
        {
            return true;
        }
    }
    return false;
}

static void app_lea_umr_clear_playback_track_handle(T_APP_LE_LINK *p_link)
{
    for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
    {
        T_LEA_ASE_ENTRY p_ase_entry = p_link->lea_ase_entry[i];

        if (p_ase_entry.used == true &&
            p_ase_entry.control_point_enable == true &&
            p_ase_entry.path_direction == DATA_PATH_OUTPUT_FLAG)
        {
            if (p_ase_entry.track_handle && p_ase_entry.stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                app_lea_uca_sidetone_detach(&p_ase_entry);
                audio_track_release(p_ase_entry.track_handle);
                p_ase_entry.track_handle = NULL;
                p_ase_entry.track_write_len = 0;
                app_lea_uca_eq_detach(p_ase_entry.stream_type);
            }
        }
    }
}

static bool app_lea_umr_is_output_track_existed(T_APP_LE_LINK *p_link,
                                                T_LEA_ASE_ENTRY *p_ase_entry)
{
    for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
    {
        if ((p_link->lea_ase_entry[i].used == true) && (&p_link->lea_ase_entry[i] != p_ase_entry))
        {
            if (p_link->lea_ase_entry[i].path_direction == p_ase_entry->path_direction &&
                (p_ase_entry->path_direction == DATA_PATH_OUTPUT_FLAG))
            {
                if (p_link->lea_ase_entry[i].track_handle)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

static void app_lea_uca_dump_call_info(T_APP_LE_LINK *p_link)
{
    for (uint8_t i = 0; i < CCP_CALL_ENTRY_NUM; i++)
    {
        T_LEA_CALL_ENTRY *p_call_entry = &p_link->lea_call_entry[i];

        APP_PRINT_INFO3("dump call info: used %d, call_index %d, call_state %d",
                        p_call_entry->used, p_call_entry->call_index, p_call_entry->call_state);
    }
    APP_PRINT_INFO3("dump call info: active_call_index %d, call_status %d, ccp_call_status %d",
                    p_link->active_call_index, p_link->call_status, app_lea_ccp_get_call_status());
}

static void app_lea_uca_stop_inactive_stream(T_APP_LE_LINK *p_link)
{
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        T_APP_LE_LINK *p_temp_link = &app_db.le_link[i];
        if (p_temp_link != p_link)
        {
            if (p_temp_link->used == true)
            {
                if (p_temp_link->lea_link_state == LEA_LINK_STREAMING)
                {
                    T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM param;

                    param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_STOP;
                    mcp_client_write_media_cp(p_temp_link->conn_handle, 0, p_temp_link->gmcs, &param, true);
                }
            }
        }
    }

    APP_PRINT_TRACE1("app_lea_uca_stop_inactive_stream: active conn_handle 0x%x", p_link->conn_handle);
}

static void app_lea_uca_set_s2m_zero_packet(uint16_t cis_conn_handle, uint8_t val)
{
    static uint8_t used_val = 0xFF;
    uint8_t buffer[4] = {0};

    if (val == 0xFF)
    {
        //reset only occur at cis reconnection.
        used_val = val;
        return;
    }

    if (used_val == val)
    {
        return;
    }

    used_val = val;

    /* fixed para */
    buffer[0] = 0x1B;

    memcpy(&buffer[1], &cis_conn_handle, 2);

    /*
        1 : enable
        0 : disable
    */
    buffer[3] = val;

    gap_vendor_cmd_req(0xFD80, sizeof(buffer), buffer);

    APP_PRINT_TRACE1("app_lea_uca_set_s2m_zero_packet: %b", TRACE_BINARY(sizeof(buffer), buffer));
}

static bool app_lea_uca_record_read_cb(T_AUDIO_TRACK_HANDLE   handle,
                                       uint32_t              *timestamp,
                                       uint16_t              *seq_num,
                                       T_AUDIO_STREAM_STATUS *status,
                                       uint8_t               *frame_num,
                                       void                  *buf,
                                       uint16_t               required_len,
                                       uint16_t              *actual_len)
{
    uint16_t conn_handle = 0;
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

#if F_APP_CCP_SUPPORT
    conn_handle = app_lea_ccp_get_active_conn_handle();
#endif
    p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_TRACK, conn_handle,  handle);
    if (p_ase_entry != NULL)
    {
#if F_APP_LEA_ALWAYS_CONVERSATION
        if (p_ase_entry->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
        {
            //real conversation
            app_lea_uca_set_s2m_zero_packet(p_ase_entry->cis_conn_handle, 0);
        }
#endif
        //T_BT_LE_ISO_SYNC_REF_AP_INFO p_info;
        APP_PRINT_INFO4("app_lea_uca_record_read_cb: buf 0x%08x, required_len %d, timestamp=0x%x, seq_num =0x%x",
                        buf,
                        required_len,
                        *(uint32_t *)timestamp,
                        *(uint16_t *)seq_num);
#if 0
        p_info.dir = 1;
        p_info.cis = 1;
        p_info.conn_handle = p_ase_entry->cis_conn_handle;


        bt_get_le_iso_sync_ref_ap_info(&p_info);

        APP_PRINT_INFO8("app_lea_mgr_record_read_cb__: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
                        p_info.conn_handle,
                        p_info.sdu_seq_num,
                        p_info.sdu_interval,
                        p_info.cur_sync_ref_point,
                        p_info.pre_sync_ref_point,
                        p_info.accumulate_sw_timer,
                        p_info.iso_interval_us,
                        p_info.group_anchor_point);
#endif
        app_lea_uca_send_iso_data(buf, p_ase_entry, required_len,
                                  true, *timestamp);
    }
PROCESS_END:
    *actual_len = required_len;
    return true;
}

static void app_lea_uca_track_reconfig_write_length(T_APP_LE_LINK *p_link)
{
    for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
    {
        T_LEA_ASE_ENTRY *p_ase_entry = &p_link->lea_ase_entry[i];

        if ((p_ase_entry->used == true) &&
            (p_ase_entry->track_handle != NULL) &&
            (p_ase_entry->stream_type ==
             AUDIO_STREAM_TYPE_PLAYBACK)) //TODO: modify condition when support voice mix 2 cis
        {
            uint8_t channel_count = 0;

            //Only handle 2 cis mix situation, no need to check AUDIO_CHANNEL_LOCATION_MONO
            channel_count = __builtin_popcount(p_link->stream_channel_allocation);
            p_ase_entry->frame_num = p_ase_entry->codec_cfg.codec_frame_blocks_per_sdu * channel_count;
            p_ase_entry->track_write_len = p_ase_entry->codec_cfg.octets_per_codec_frame *
                                           p_ase_entry->frame_num;
        }
    }
}

static void app_lea_uca_track_set_format_info(T_APP_LE_LINK *p_link, T_LEA_ASE_ENTRY *p_ase_entry,
                                              T_AUDIO_FORMAT_INFO *format_info)
{
    uint32_t sample_rate;
    uint32_t chann_location;
    T_CODEC_CFG *p_codec = &p_ase_entry->codec_cfg;

    sample_rate = app_lea_sample_rate_freq_map[p_codec->sample_frequency - 1];

    if (p_ase_entry->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        chann_location = p_link->stream_channel_allocation;
    }
    else
    {
        chann_location = p_codec->audio_channel_allocation;
    }

    format_info->frame_num = FIXED_FRAME_NUM;
#if F_APP_LC3_PLUS_SUPPORT
    if (p_ase_entry->codec_type == AUDIO_FORMAT_TYPE_LC3PLUS)
    {
        format_info->type = AUDIO_FORMAT_TYPE_LC3PLUS;
        format_info->attr.lc3plus.sample_rate = sample_rate;
        format_info->attr.lc3plus.bit_width = p_ase_entry->bit_depth;
        format_info->attr.lc3plus.chann_location = chann_location;
        format_info->attr.lc3plus.frame_length = p_codec->octets_per_codec_frame;
        format_info->attr.lc3plus.presentation_delay = p_ase_entry->presentation_delay;
#if F_APP_FRAUNHOFER_SUPPORT
        if (p_codec->frame_duration == FRAUNHOFER_CFG_FD_10_MS)
        {
            format_info->attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_10_MS;
        }
        else if (p_codec->frame_duration == FRAUNHOFER_CFG_FD_7_5_MS)
        {
            format_info->attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_7_5_MS;
        }
        else if (p_codec->frame_duration == FRAUNHOFER_CFG_FD_5_MS)
        {
            format_info->attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_5_MS;
        }
        else
        {
            format_info->attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_2_5_MS;
        }
        format_info->attr.lc3plus.mode = app_lea_pacs_get_mode(sample_rate,
                                                               format_info->attr.lc3plus.frame_duration,
                                                               format_info->attr.lc3plus.frame_length);
#else
        if (p_codec->frame_duration == FRAME_DURATION_CFG_10_MS)
        {
            format_info->attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_10_MS;
        }
        else if (p_codec->frame_duration == FRAME_DURATION_CFG_7_5_MS)
        {
            format_info->attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_7_5_MS;
        }
        else if (p_codec->frame_duration == FRAME_DURATION_CFG_5_MS)
        {
            format_info->attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_5_MS;
        }
        else
        {
            format_info->attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_2_5_MS;
        }
#endif
    }
    else
#endif
    {
        format_info->type = AUDIO_FORMAT_TYPE_LC3;
        format_info->attr.lc3.sample_rate = sample_rate;
        format_info->attr.lc3.chann_location = chann_location;
        format_info->attr.lc3.frame_length = p_codec->octets_per_codec_frame;
        format_info->attr.lc3.frame_duration = (T_AUDIO_LC3_FRAME_DURATION)p_codec->frame_duration;
        format_info->attr.lc3.presentation_delay = p_ase_entry->presentation_delay;
    }

    APP_PRINT_TRACE8("app_lea_uca_track_set_format_info: ase_id 0x%02X, frame_duration 0x%02X, sample_frequency 0x%02X, \
octets_per_codec_frame %d, codec_frame_blocks_per_sdu %d, audio_channel_allocation %d, stream_type %d, bit_depth %d",
                     p_ase_entry->ase_id,
                     p_codec->frame_duration,
                     p_codec->sample_frequency,
                     p_codec->octets_per_codec_frame,
                     p_codec->codec_frame_blocks_per_sdu,
                     p_codec->audio_channel_allocation,
                     p_ase_entry->stream_type,
                     p_ase_entry->bit_depth);
}

static void app_lea_uca_track_create(T_APP_LE_LINK *p_link, T_LEA_ASE_ENTRY *p_ase_entry)
{
    T_AUDIO_STREAM_TYPE type = AUDIO_STREAM_TYPE_PLAYBACK;
    uint8_t volume_out = 0;
    uint8_t volume_in = 0;
    uint8_t channel_count = 0;
    uint32_t device = app_db.playback_device;
    P_AUDIO_TRACK_ASYNC_IO async_read = NULL;
    T_AUDIO_FORMAT_INFO format_info = {};
    uint32_t audio_channel_allocation = 0;
    uint8_t result = 0;

    app_lea_mgr_set_media_state(ISOCH_DATA_PKT_STATUS_LOST_DATA);

    if (p_ase_entry == NULL || p_link == NULL)
    {
        result = 1;
        goto fail_create_track;
    }
    if (app_lea_umr_is_output_track_existed(p_link, p_ase_entry))
    {
        result = 2;
        goto fail_create_track;
    }

    audio_channel_allocation = p_ase_entry->codec_cfg.audio_channel_allocation;
    if (app_lea_uca_diff_path)
    {
        if (p_ase_entry->path_direction == DATA_PATH_OUTPUT_FLAG)
        {
            p_ase_entry->stream_type = AUDIO_STREAM_TYPE_PLAYBACK;
            volume_out = app_lea_uca_vcs();
            volume_in = 0;
            device = app_db.playback_device;
            audio_channel_allocation = p_link->stream_channel_allocation;
        }
        else if (p_ase_entry->path_direction == DATA_PATH_INPUT_FLAG)
        {
            p_ase_entry->stream_type = AUDIO_STREAM_TYPE_RECORD;
            volume_out = 0;
            volume_in = app_dsp_cfg_vol.record_volume_default;
            device = AUDIO_DEVICE_IN_MIC;
            async_read = app_lea_uca_record_read_cb;
            //MICS
        }
    }
    else
    {
        bool create_voice_track = false;

        if (app_lea_umr_is_input_path_enabled(p_link))
        {
            app_lea_umr_clear_playback_track_handle(p_link);

            if (app_lea_umr_is_voice_track_existed(p_link, p_ase_entry))
            {
                result = 3;
                goto fail_create_track;
            }

            create_voice_track = true;
        }

        if (create_voice_track)
        {
            p_ase_entry->stream_type = AUDIO_STREAM_TYPE_VOICE;
            volume_out = app_lea_uca_vcs();
            volume_in = app_dsp_cfg_vol.voice_volume_in_default;
            device = app_db.playback_device | AUDIO_DEVICE_IN_MIC;
            p_link->cis_right_ch_conn_handle = 0x0;
        }
        else
        {
            p_ase_entry->stream_type = AUDIO_STREAM_TYPE_PLAYBACK;
            volume_out = app_lea_uca_vcs();
            volume_in = 0;
            device = app_db.playback_device;
            audio_channel_allocation = p_link->stream_channel_allocation;
        }
    }

    channel_count = (audio_channel_allocation == AUDIO_CHANNEL_LOCATION_MONO) ?
                    1 : __builtin_popcount(audio_channel_allocation);
    p_ase_entry->frame_num = p_ase_entry->codec_cfg.codec_frame_blocks_per_sdu * channel_count;

    app_lea_uca_track_set_format_info(p_link, p_ase_entry, &format_info);
    p_ase_entry->track_handle = audio_track_create(p_ase_entry->stream_type,
                                                   AUDIO_STREAM_MODE_DIRECT,
                                                   AUDIO_STREAM_USAGE_LOCAL,
                                                   format_info,
                                                   volume_out,
                                                   volume_in,
                                                   device,
                                                   NULL,
                                                   async_read);
    if (p_ase_entry->track_handle != NULL)
    {
        p_ase_entry->track_write_len = p_ase_entry->codec_cfg.octets_per_codec_frame *
                                       p_ase_entry->frame_num;

        if (app_cfg_nv.lea_vol_out_mute)
        {
            audio_track_volume_out_mute(p_ase_entry->track_handle);
        }
        else
        {
            audio_track_volume_out_unmute(p_ase_entry->track_handle);
        }

        app_lea_uca_eq_attach(p_ase_entry->stream_type, p_ase_entry->track_handle);
        app_lea_uca_sidetone_attach(p_ase_entry);
        audio_track_start(p_ase_entry->track_handle);
    }
    else
    {
        result = 4;
        goto fail_create_track;
    }

    return;

fail_create_track:
    APP_PRINT_ERROR1("app_lea_uca_track_create: fail cause %d",  -result);
}

static void app_lea_uca_track_release(T_APP_LE_LINK *p_link, T_LEA_ASE_ENTRY *p_ase_entry)
{
    APP_PRINT_INFO0("app_lea_uca_track_release");

    if (p_ase_entry != NULL)
    {
        app_lea_mgr_set_media_state(ISOCH_DATA_PKT_STATUS_LOST_DATA);
        app_lea_uca_sidetone_detach(p_ase_entry);
        audio_track_release(p_ase_entry->track_handle);
        p_ase_entry->track_handle = NULL;
        p_ase_entry->track_write_len = 0;
        app_lea_uca_eq_detach(p_ase_entry->stream_type);
    }
}

static void app_lea_uca_prefer_qos(void *p_data)
{
    bool ret = false;
    T_ASCS_GET_PREFER_QOS *p_info = (T_ASCS_GET_PREFER_QOS *)p_data;
    T_QOS_CFG_PREFERRED qos_data;
    T_ASCS_PREFER_QOS_DATA prefer_qos;
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_info->conn_handle,
                                              (void *)(&p_info->ase_id));
    if (p_ase_entry)
    {
        if (p_ase_entry->codec_type == AUDIO_FORMAT_TYPE_LC3)
        {
            if ((qos_preferred_cfg_get_by_codec(&p_info->codec_cfg, p_info->target_latency, &qos_data) == true)
                && (ascs_get_ase_prefer_qos(p_info->conn_handle, p_info->ase_id, &prefer_qos) == true))
            {
                ret = true;
            }
        }
#if F_APP_LC3_PLUS_SUPPORT
        else
        {
            if (app_lea_pacs_qos_preferred_cfg_get_by_codec(&p_info->codec_cfg, p_info->target_latency,
                                                            &qos_data, p_info->conn_handle, p_info->ase_id) == true)
            {
                prefer_qos.max_transport_latency = qos_data.max_transport_latency;
                prefer_qos.preferred_retrans_number = qos_data.retransmission_number;
                prefer_qos.supported_framing = qos_data.framing;
                prefer_qos.preferred_phy = p_info->target_phy;
                ret = true;
            }
        }
#endif
    }

    if (ret == true)
    {
        prefer_qos.presentation_delay_max = qos_data.presentation_delay;

        if (app_lea_pacs_is_low_latency())
        {
            prefer_qos.preferred_presentation_delay_max = CIS_PERMIT_MIN_PD_LOW_LATENCY_DELAY;
            prefer_qos.presentation_delay_min = CIS_PERMIT_MIN_PD_LOW_LATENCY_DELAY;
            prefer_qos.preferred_presentation_delay_min = CIS_PERMIT_MIN_PD_LOW_LATENCY_DELAY;
        }
        else
        {
            prefer_qos.preferred_presentation_delay_max = qos_data.presentation_delay;
            //LEA source may select min pd, RTK DSP just support 20ms pd in normal mode
            prefer_qos.presentation_delay_min = CIS_PERMIT_MIN_PD_DELAY;
            prefer_qos.preferred_presentation_delay_min = CIS_PERMIT_MIN_PD_DELAY;
        }

        if (prefer_qos.presentation_delay_max < prefer_qos.presentation_delay_min)
        {
            prefer_qos.presentation_delay_min = prefer_qos.presentation_delay_max;
        }

        if (ascs_cfg_ase_prefer_qos(p_info->conn_handle, p_info->ase_id, &prefer_qos) == true)
        {
            T_LEA_ASE_ENTRY *p_ase_entry = NULL;

            p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_info->conn_handle,
                                                      (void *)(&p_info->ase_id));
            if (p_ase_entry != NULL)
            {
                p_ase_entry->path_direction = p_info->direction;
            }
        }
    }
}

static void app_lea_uca_setup_data_path(T_APP_LE_LINK *p_link, void *p_data)
{
    T_LEA_ASE_ENTRY *p_ase_entry;
    T_ASCS_SETUP_DATA_PATH *p_info = (T_ASCS_SETUP_DATA_PATH *)p_data;

    p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_link->conn_handle,
                                              (void *)(&p_info->ase_id));
    if (p_ase_entry != NULL)
    {
        app_lea_uca_track_create(p_link, p_ase_entry);
        if (p_info->path_direction == DATA_PATH_INPUT_FLAG)
        {
            p_ase_entry->prequeue_pkt_cnt = 0;
            p_ase_entry->pkt_seq = 0;
        }
    }
}

static void app_lea_uca_remove_data_path(T_APP_LE_LINK *p_link, void *p_data)
{
    T_ASCS_REMOVE_DATA_PATH *p_info = (T_ASCS_REMOVE_DATA_PATH *)p_data;
    T_LEA_ASE_ENTRY *p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_link->conn_handle,
                                                               (void *)(&p_info->ase_id));

    app_lea_uca_track_release(p_link, p_ase_entry);
    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_LEA);

    if (p_link->cis_left_ch_conn_handle == p_info->cis_conn_handle)
    {
        app_lea_mgr_clear_iso_queue(&app_lea_uca_left_ch_queue);
        p_link->cis_left_ch_conn_handle = 0;
        p_link->cis_left_ch_iso_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
    }
    else if (p_link->cis_right_ch_conn_handle == p_info->cis_conn_handle)
    {
        app_lea_mgr_clear_iso_queue(&app_lea_uca_right_ch_queue);
        p_link->cis_right_ch_conn_handle  = 0;
        p_link->cis_right_ch_iso_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
    }

    if (app_lea_uca_active_stream_link != NULL &&
        p_link->conn_handle == app_lea_uca_active_stream_link->conn_handle)
    {
        if (p_link->cis_right_ch_conn_handle == 0 && p_link->cis_left_ch_conn_handle == 0)
        {
            app_lea_uca_active_stream_link = NULL;
        }
    }
}

static void app_lea_uca_handle_mcp_state(T_APP_LE_LINK *p_link)
{
    if (p_link->media_state == MCS_MEDIA_STATE_PLAYING)
    {
        if ((app_lea_ccp_get_call_status() != APP_CALL_IDLE) ||
            (app_hfp_get_call_status() != APP_CALL_IDLE))
        {
            T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM param;

            param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PAUSE;
            mcp_client_write_media_cp(p_link->conn_handle, 0, p_link->gmcs, &param, true);
        }
        else
        {
            app_lea_mcp_set_active_conn_handle(p_link->conn_handle);
        }
    }
}

static void app_lea_uca_handle_ccp_state(T_APP_LE_LINK *p_link, void *p_data)
{
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    app_lea_uca_stop_inactive_stream(p_link);

    //In multi-party call scenario, if active call is ended and accept the inactive call
    //Audio effect will be disabled when call terminated, app_lea_uca_handle_ccp_term,
    //Re-enable audio effect when call state change
    p_ase_entry = app_lea_ascs_find_ase_entry_non_conn(LEA_ASE_UP_DIRECT, NULL, NULL);
    if (p_ase_entry != NULL)
    {
        sidetone_enable(p_ase_entry->sidetone_instance);
    }
}

static void app_lea_uca_handle_ccp_term(T_APP_LE_LINK *p_link, void *p_data)
{
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    //IOP: Unicast client, ex: Google Pixel 8, may delay ASE release when call terminated
    //     Cause headset stay in conversation mode and not remove data path.
    //     1. Audio track is not released, and audio effect are all enabled
    //     2. MIC is not disabled and not enter DLPS
    //Solution1: Control audio effect along with call status
    //Solution2: TBD
    p_ase_entry = app_lea_ascs_find_ase_entry_non_conn(LEA_ASE_UP_DIRECT, NULL, NULL);
    if (p_ase_entry != NULL)
    {
        sidetone_disable(p_ase_entry->sidetone_instance);
    }
}

static void app_lea_uca_link_state_change(T_APP_LE_LINK *p_link, T_LEA_LINK_STATE state)
{
#if F_APP_AUTO_POWER_TEST_LOG
    TEST_PRINT_INFO0("app_lea_uca_link_state_change: new state %d", state);
#else
    APP_PRINT_INFO2("app_lea_uca_link_state_change: change from %d to %d", p_link->lea_link_state,
                    state);
#endif

    p_link->lea_link_state = state;
}

static void app_lea_uca_ase_ctrl(T_LEA_LINK_EVENT event, T_APP_LE_LINK *p_link)
{
    T_LEA_ASE_ENTRY *p_ase_entry;
    bool lea_streaming = false;

    p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CONN, p_link->conn_handle, NULL);

    switch (event)
    {
    case LEA_PAUSE:
        {
            if (p_ase_entry != NULL && p_ase_entry->track_handle != NULL)
            {
                app_lea_uca_sidetone_detach(p_ase_entry);
                audio_track_release(p_ase_entry->track_handle);
                app_lea_uca_eq_detach(p_ase_entry->stream_type);
            }
        }
        break;

    case LEA_RELEASING:
        {
            if (p_ase_entry != NULL)
            {
                if (p_ase_entry->path_direction == DATA_PATH_OUTPUT_FLAG)
                {
                    p_link->stream_channel_allocation &= ~p_ase_entry->codec_cfg.audio_channel_allocation;
                }

                app_lea_ascs_free_ase_entry(p_ase_entry);
            }
        }
        break;

    default:
        return;
    }

    app_lea_uca_link_state_change(p_link, LEA_LINK_CONNECTED);

    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].lea_link_state == LEA_LINK_STREAMING)
        {
            lea_streaming = true;
            break;
        }
    }

    if (lea_streaming == false)
    {
        T_AUDIO_TRACK_STATE state;
        T_APP_BR_LINK *p_edr_link;

        p_edr_link = &app_db.br_link[app_a2dp_get_active_idx()];
        audio_track_state_get(p_edr_link->a2dp.track_handle, &state);
        if (state == AUDIO_TRACK_STATE_PAUSED ||
            state == AUDIO_TRACK_STATE_STOPPED)
        {
            if (p_edr_link->a2dp.is_streaming == true)
            {
                audio_track_start(p_edr_link->a2dp.track_handle);
            }
        }
    }
    // TODO: pause state, change to 60ms
    //ble_set_prefer_conn_param(p_link->conn_id, 0x30, 0x30, 0, 500);
    APP_PRINT_INFO1("LEA_PAUSE: le_audio_fg %x", lea_streaming);
}

static void app_lea_uca_handle_le_disconnect(T_APP_LE_LINK *p_link, uint8_t event, void *p_data)
{
    if (event == LEA_DISCON_LOCAL)
    {
        app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_POWER_OFF);
    }
    else
    {
        uint16_t cause = *(uint16_t *)p_data;

        if ((cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
            (cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
        {
            app_audio_tone_type_play(TONE_CIS_LOST, false, false);
        }
    }

    app_lea_uca_free_link(p_link);
    app_lea_uca_link_state_change(p_link, LEA_LINK_IDLE);
#if F_APP_CCP_SUPPORT
    app_lea_ccp_update_call_status();
#endif
}

static void app_lea_uca_link_idle(T_APP_LE_LINK *p_link, uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_lea_uca_link_idle: event %x, state %x", event, p_link->lea_link_state);
    switch (event)
    {
    case LEA_CONNECT:
        {
            // TODO: to avoid service discovery taking long time, change to 7.5ms
            //ble_set_prefer_conn_param(p_link->conn_id, 0x06, 0x06, 0, 500);
            app_lea_uca_link_state_change(p_link, LEA_LINK_CONNECTED);
            app_bond_le_set_bond_flag((void *)p_link, BOND_FLAG_LEA);

            app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_LEA);
        }
        break;

    default:
        break;
    }
}

static void app_lea_uca_link_connected(T_APP_LE_LINK *p_link, uint8_t event, void *p_data)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    APP_PRINT_INFO2("app_lea_uca_link_connected: event %x, state %x", event, p_link->lea_link_state);

    switch (event)
    {
    case LEA_DISCONNECT:
    case LEA_DISCON_LOCAL:
        {
            app_lea_uca_handle_le_disconnect(p_link, event, p_data);
        }
        break;

    case LEA_PREFER_QOS:
        {
            app_lea_uca_prefer_qos(p_data);
        }
        break;

    case LEA_ENABLE:
        {
            uint8_t i;
            T_LEA_ASE_ENTRY *p_ase_entry;
            T_ASCS_CP_ENABLE_IND *p_info = (T_ASCS_CP_ENABLE_IND *)p_data;
            uint8_t sample_frequency = 0;
            uint32_t presentation_delay = 0;
            bool freq_diff = false;
            bool pd_diff = false;
            bool lc3_plus = false;

            for (i = 0; i < p_info->number_of_ase; i++)
            {
                p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_info->conn_handle,
                                                          (void *)(&p_info->param[i].ase_id));
                if (p_ase_entry != NULL)
                {
#if F_APP_LC3_PLUS_SUPPORT
                    p_ase_entry->bit_depth = app_lea_pacs_check_bit_depth(p_ase_entry->codec_type,
                                                                          p_info->param[i].metadata_length, p_info->param[i].p_metadata);
                    if (p_ase_entry->codec_type == AUDIO_FORMAT_TYPE_LC3PLUS)
                    {
                        lc3_plus = true;
                    }
#endif
                    //reset
                    app_lea_uca_set_s2m_zero_packet(p_ase_entry->cis_conn_handle, 0xFF);

                    if (!sample_frequency)
                    {
                        sample_frequency = p_ase_entry->codec_cfg.sample_frequency;
                    }
                    else if (sample_frequency != p_ase_entry->codec_cfg.sample_frequency)
                    {
                        freq_diff = true;
                    }

                    if (!presentation_delay)
                    {
                        presentation_delay =  p_ase_entry->presentation_delay;
                    }
                    else if (presentation_delay != p_ase_entry->presentation_delay)
                    {
                        pd_diff = true;
                    }
                }
            }

            if (freq_diff || pd_diff || lc3_plus || UPSTREAM_ALGO_AUDIO)
            {
                app_lea_uca_diff_path = true;
            }
            else
            {
                app_lea_uca_diff_path = false;
            }
        }
        break;

    case LEA_ENABLEING:
        {
            T_APP_BR_LINK *p_link = NULL;

            p_link = &app_db.br_link[app_a2dp_get_active_idx()];
            if (p_link)
            {
                if (p_link->avrcp.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    bt_avrcp_pause(p_link->bd_addr);
                    p_link->avrcp.play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                }
            }
        }
        break;

    case LEA_CODEC_CONFIGURED:
        {
            T_LEA_ASE_ENTRY *p_ase_entry = (T_LEA_ASE_ENTRY *)p_data;

            if (p_ase_entry->path_direction == DATA_PATH_OUTPUT_FLAG)
            {
                p_link->stream_channel_allocation |= p_ase_entry->codec_cfg.audio_channel_allocation;
            }
        }
        break;

    case LEA_SETUP_DATA_PATH:
        {
            app_lea_uca_setup_data_path(p_link, p_data);
        }
        break;

    case LEA_REMOVE_DATA_PATH:
        {
            app_lea_uca_remove_data_path(p_link, p_data);
        }
        break;

    case LEA_STREAMING:
        {
            T_LEA_ASE_ENTRY *p_ase_entry;

            p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CONN, p_link->conn_handle, NULL);
            if (p_ase_entry != NULL)
            {
                if (p_ase_entry->audio_context == AUDIO_CONTEXT_MEDIA ||
                    p_ase_entry->audio_context == AUDIO_CONTEXT_UNSPECIFIED)
                {
#if F_APP_MCP_SUPPORT
                    app_lea_mcp_set_active_conn_handle(p_ase_entry->conn_handle);
#endif
                }
                else if (p_ase_entry->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
                {
#if F_APP_CCP_SUPPORT
                    app_lea_ccp_set_active_conn_handle(p_ase_entry->conn_handle);
#endif
                }
                app_lea_uca_stop_inactive_stream(p_link);
            }

            app_lea_uca_link_state_change(p_link, LEA_LINK_STREAMING);
            // TODO: streaming state, change to 40ms
            //ble_set_prefer_conn_param(p_link->conn_id, 0x20, 0x20, 0, 500);
        }
        break;

    case LEA_PAUSE:
        {
            app_lea_uca_ase_ctrl(LEA_PAUSE, p_link);
        }
        break;

    case LEA_RELEASING:
        {
            app_lea_uca_ase_ctrl(LEA_RELEASING, p_link);
        }
        break;

    case LEA_A2DP_START:
    case LEA_AVRCP_PLAYING:
        if (p_link->call_status != APP_CALL_IDLE)
        {
            T_APP_BR_LINK *p_edr_link;
            p_edr_link = &app_db.br_link[active_a2dp_idx];

            if (p_edr_link != NULL)
            {
                if (p_edr_link->avrcp.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    bt_avrcp_pause(p_edr_link->bd_addr);
                    p_edr_link->avrcp.play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                }
            }
        }
        break;

#if F_APP_MCP_SUPPORT
    case LEA_MCP_STATE:
        {
            app_lea_uca_handle_mcp_state(p_link);
        }
        break;
#endif

#if F_APP_CCP_SUPPORT
    case LEA_CCP_CALL_STATE:
        {
            app_lea_uca_handle_ccp_state(p_link, p_data);
        }
        break;

    case LEA_CCP_TERM_NOTIFY:
        {
            app_lea_uca_handle_ccp_term(p_link, p_data);
        }
        break;
#endif

    default:
        break;
    }
}

static void app_lea_uca_link_streaming(T_APP_LE_LINK *p_link, uint8_t event, void *p_data)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    APP_PRINT_INFO2("app_lea_uca_link_streaming: event %x, state %x", event, p_link->lea_link_state);

    switch (event)
    {
    case LEA_DISCONNECT:
    case LEA_DISCON_LOCAL:
        {
            app_lea_uca_handle_le_disconnect(p_link, event, p_data);
        }
        break;

    case LEA_PREFER_QOS:
        {
            app_lea_uca_prefer_qos(p_data);
        }
        break;

    case LEA_SETUP_DATA_PATH:
        {
            app_lea_uca_setup_data_path(p_link, p_data);
        }
        break;

    case LEA_REMOVE_DATA_PATH:
        {
            app_lea_uca_remove_data_path(p_link, p_data);
        }
        break;

    case LEA_PAUSE:
        {
            app_lea_uca_ase_ctrl(LEA_PAUSE, p_link);
        }
        break;

    case LEA_RELEASING:
        {
            app_lea_uca_ase_ctrl(LEA_RELEASING, p_link);
        }
        break;

    case LEA_PAUSE_LOCAL:
        {
            if (app_lea_ccp_get_call_status() != APP_CALL_IDLE)
            {
                break;
            }

            T_LEA_ASE_ENTRY *p_ase_entry;
            bool is_ascs_release = false;
            T_ASE_CHAR_DATA ase_data;

            p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CONN, p_link->conn_handle, NULL);

            if (p_ase_entry == NULL)
            {
                break;
            }

            if (ascs_get_ase_data(p_ase_entry->conn_handle, p_ase_entry->ase_id, &ase_data))
            {
                is_ascs_release = ascs_action_release_by_cig(p_ase_entry->conn_handle,
                                                             ase_data.param.enabling.cig_id);
            }

            if (is_ascs_release)
            {
                app_lea_ascs_free_ase_entry(p_ase_entry);

                app_lea_uca_link_state_change(p_link, LEA_LINK_CONNECTED);
            }
        }
        break;

    case LEA_A2DP_START:
    case LEA_AVRCP_PLAYING:
        {
            if (p_link->media_state == MCS_MEDIA_STATE_PLAYING)
            {
                T_LEA_ASE_ENTRY *p_ase_entry;

                p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CONN, p_link->conn_handle, NULL);
                if (p_ase_entry != NULL)
                {
                    T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM param;

                    param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PAUSE;
                    mcp_client_write_media_cp(p_ase_entry->conn_handle, 0, p_link->gmcs, &param, true);
                }
            }

            if (p_link->call_status != APP_CALL_IDLE)
            {
                T_APP_BR_LINK *p_edr_link;
                p_edr_link = &app_db.br_link[active_a2dp_idx];

                if (p_edr_link != NULL)
                {
                    if (p_edr_link->avrcp.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                    {
                        bt_avrcp_pause(p_edr_link->bd_addr);
                        p_edr_link->avrcp.play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                    }
                }
            }
        }
        break;

#if F_APP_MCP_SUPPORT
    case LEA_MCP_STATE:
        {
            app_lea_uca_handle_mcp_state(p_link);
        }
        break;
#endif
    case LEA_HFP_CALL_STATE:
        {
            if (app_hfp_get_call_status() != APP_CALL_IDLE)
            {
                if (app_lea_ccp_get_call_status() == APP_CALL_IDLE &&
                    p_link->media_state == MCS_MEDIA_STATE_PLAYING)
                {
                    T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM param;

                    param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PAUSE;
                    mcp_client_write_media_cp(p_link->conn_handle, 0, p_link->gmcs, &param, true);
                }
            }
        }
        break;

#if F_APP_CCP_SUPPORT
    case LEA_CCP_CALL_STATE:
        {
            app_lea_uca_handle_ccp_state(p_link, p_data);
        }
        break;

    case LEA_CCP_TERM_NOTIFY:
        {
            app_lea_uca_handle_ccp_term(p_link, p_data);
        }
        break;
#endif

    default:
        break;
    }
}

void app_lea_uca_link_sm(uint16_t conn_handle, uint8_t event, void *p_data)
{
    T_APP_LE_LINK *p_link;
    p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link == NULL)
    {
        return;
    }

    APP_PRINT_INFO3("app_lea_uca_link_sm: conn_handle 0x%x, event %x, %d", conn_handle, event,
                    p_link->lea_link_state);
    switch (p_link->lea_link_state)
    {
    case LEA_LINK_IDLE:
        app_lea_uca_link_idle(p_link, event, p_data);
        break;

    case LEA_LINK_CONNECTED:
        app_lea_uca_link_connected(p_link, event, p_data);
        break;

    case LEA_LINK_STREAMING:
        app_lea_uca_link_streaming(p_link, event, p_data);
        break;

    default:
        break;
    }
    app_lea_uca_dump_ase_info(p_link);
    app_lea_uca_dump_call_info(p_link);
}

void app_lea_uca_set_sidetone_instance(T_APP_DSP_CFG_SIDETONE info)
{
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;
    int32_t ret = 0;

    if (app_lea_ccp_get_call_status() != APP_CALL_ACTIVE)
    {
        ret = 1;
        goto fail_status;
    }

    p_ase_entry = app_lea_ascs_find_ase_entry_non_conn(LEA_ASE_UP_DIRECT, NULL, NULL);

    if (p_ase_entry == NULL)
    {
        ret = 2;
        goto fail_status;
    }

    app_sidetone_detach(p_ase_entry->track_handle, p_ase_entry->sidetone_instance);
    p_ase_entry->sidetone_instance = app_sidetone_attach(p_ase_entry->track_handle, info);

    if (p_ase_entry->sidetone_instance == NULL)
    {
        ret = 3;
        goto fail_status;
    }

fail_status:
    APP_PRINT_ERROR1("app_lea_uca_set_sidetone_instance: failed %d", -ret);
}

static T_ISO_DATA_IND *app_lea_uca_get_iso_data(uint8_t *output_channel,
                                                T_BT_DIRECT_CB_DATA *p_data)
{
    T_OS_QUEUE *p_mirror_queue = NULL;
    T_OS_QUEUE *p_insert_queue = NULL;
    T_ISO_DATA_IND *p_iso_elem = NULL;
    uint16_t output_handle = 0;
    uint8_t result = 0;

    *output_channel = 0;

    if (app_lea_uca_active_stream_link->cis_left_ch_conn_handle == p_data->p_bt_direct_iso->conn_handle)
    {
        if (p_data->p_bt_direct_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
        {
            app_lea_uca_active_stream_link->cis_left_ch_iso_state = p_data->p_bt_direct_iso->pkt_status_flag;
        }
        p_mirror_queue = &(app_lea_uca_right_ch_queue);
        *output_channel = AUDIO_LOCATION_FL;
        p_insert_queue = &(app_lea_uca_left_ch_queue);
        output_handle = app_lea_uca_active_stream_link->cis_left_ch_conn_handle;
    }
    else if (app_lea_uca_active_stream_link->cis_right_ch_conn_handle ==
             p_data->p_bt_direct_iso->conn_handle)
    {
        if (p_data->p_bt_direct_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
        {
            app_lea_uca_active_stream_link->cis_right_ch_iso_state = p_data->p_bt_direct_iso->pkt_status_flag;
        }
        p_mirror_queue = &(app_lea_uca_left_ch_queue);
        *output_channel = AUDIO_LOCATION_FR;
        p_insert_queue = &(app_lea_uca_right_ch_queue);
        output_handle = app_lea_uca_active_stream_link->cis_right_ch_conn_handle;
    }

    if (p_insert_queue == NULL)
    {
        result = 1;
        goto fail_get_iso;
    }

    if ((app_lea_uca_active_stream_link->cis_left_ch_iso_state != ISOCH_DATA_PKT_STATUS_VALID_DATA) ||
        (app_lea_uca_active_stream_link->cis_right_ch_iso_state != ISOCH_DATA_PKT_STATUS_VALID_DATA))
    {
        result = 2;
        goto fail_get_iso;
    }

    if (p_mirror_queue == NULL || p_mirror_queue->count == 0)
    {
        app_lea_mgr_enqueue_iso_data(p_insert_queue, p_data->p_bt_direct_iso, output_handle);

        result = 3;
        goto fail_get_iso;
    }

    p_iso_elem = app_lea_mgr_find_iso_elem(p_mirror_queue, p_data->p_bt_direct_iso);
    if (p_iso_elem == NULL)
    {
        app_lea_mgr_enqueue_iso_data(p_insert_queue, p_data->p_bt_direct_iso, output_handle);
        result = 4;
    }

fail_get_iso:
    if (result != 0)
    {
        APP_PRINT_INFO7("app_lea_uca_get_iso_data: result %d,left_ch %d, right_ch %d,left handle 0x%x, right_handle 0x%x, data handle 0x%x, output_channel %d",
                        result, app_lea_uca_active_stream_link->cis_left_ch_iso_state,
                        app_lea_uca_active_stream_link->cis_right_ch_iso_state,
                        app_lea_uca_active_stream_link->cis_left_ch_conn_handle,
                        app_lea_uca_active_stream_link->cis_right_ch_conn_handle,
                        p_data->p_bt_direct_iso->conn_handle, *output_channel);
    }
    return p_iso_elem;
}

void app_lea_uca_handle_iso_data(T_BT_DIRECT_CB_DATA *p_data)
{
    uint8_t ret = 0;
    T_LEA_ASE_ENTRY *p_ase_entry;

    if (app_lea_uca_active_stream_link == NULL)
    {
        ret = 1;
        goto fail_track_write;
    }

    if (app_lea_uca_active_stream_link->cis_left_ch_conn_handle != 0 &&
        app_lea_uca_active_stream_link->cis_right_ch_conn_handle != 0)
    {
        p_ase_entry = app_lea_ascs_find_ase_entry_by_direction(app_lea_uca_active_stream_link,
                                                               DATA_PATH_OUTPUT_FLAG);
        if (p_ase_entry != NULL && p_ase_entry->track_handle != NULL)
        {
            if (p_ase_entry->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                T_OS_QUEUE *p_mirror_queue = NULL;
                T_ISO_DATA_IND *p_iso_elem = NULL;
                uint16_t sendout_pkt_seq_num;
                uint32_t sendout_pkt_timestamp;
                uint16_t written_len;
                T_AUDIO_STREAM_STATUS status;
                uint8_t *p_send_buf = NULL;
                uint16_t sdu_len = 0;
                uint8_t output_channel = AUDIO_LOCATION_MONO;

                T_AUDIO_TRACK_STATE track_state;

                audio_track_state_get(p_ase_entry->track_handle, &track_state);
                if (track_state != AUDIO_TRACK_STATE_STARTED)
                {
                    ret = 2;
                    goto fail_track_write;
                }

                p_iso_elem = app_lea_uca_get_iso_data(&output_channel, p_data);
                if (p_iso_elem == NULL)
                {
                    ret = 3;
                    goto fail_track_write;
                }

                sdu_len = p_ase_entry->codec_cfg.octets_per_codec_frame * p_ase_entry->frame_num;

                p_send_buf = audio_probe_media_buffer_malloc(sdu_len);

                if (p_send_buf == NULL)
                {
                    ret = 4;
                    goto fail_track_write;
                }

                if (p_data->p_bt_direct_iso->iso_sdu_len != 0 && p_iso_elem->iso_sdu_len != 0)
                {
                    status = AUDIO_STREAM_STATUS_CORRECT;
                }
                else
                {
                    status = AUDIO_STREAM_STATUS_LOST;
                }

                if (output_channel == AUDIO_LOCATION_FL)
                {
                    p_mirror_queue = &(app_lea_uca_right_ch_queue);
                    if (status == AUDIO_STREAM_STATUS_CORRECT)
                    {
                        memcpy(p_send_buf, p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset,
                               p_data->p_bt_direct_iso->iso_sdu_len);

                        memcpy(p_send_buf + p_data->p_bt_direct_iso->iso_sdu_len, p_iso_elem->p_buf,
                               p_iso_elem->iso_sdu_len);
                    }

                    sendout_pkt_seq_num = p_data->p_bt_direct_iso->pkt_seq_num;
                    sendout_pkt_timestamp = p_data->p_bt_direct_iso->time_stamp;
                }
                else
                {
                    p_mirror_queue = &(app_lea_uca_left_ch_queue);
                    if (status == AUDIO_STREAM_STATUS_CORRECT)
                    {
                        memcpy(p_send_buf, p_iso_elem->p_buf,  p_iso_elem->iso_sdu_len);
                        memcpy(p_send_buf + p_iso_elem->iso_sdu_len,
                               p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset,
                               p_data->p_bt_direct_iso->iso_sdu_len);
                    }

                    sendout_pkt_seq_num = p_iso_elem->pkt_seq_num;
                    sendout_pkt_timestamp = p_iso_elem->time_stamp;
                }

                APP_PRINT_INFO5("app_lea_umr_handle_iso_data: handle two cis, handle 0x%x, seq_num 0x%x, time_stamp 0x%x, p_data %p, data %b",
                                p_iso_elem->conn_handle,
                                p_iso_elem->pkt_seq_num,
                                p_iso_elem->time_stamp,
                                p_send_buf,
                                TRACE_BINARY(sdu_len, p_send_buf));

                audio_track_write(p_ase_entry->track_handle, sendout_pkt_timestamp,
                                  sendout_pkt_seq_num,
                                  status,
                                  FIXED_FRAME_NUM,
                                  p_send_buf,
                                  sdu_len,
                                  &written_len);

                audio_probe_media_buffer_free(p_send_buf);
                os_queue_delete(p_mirror_queue, p_iso_elem);
                audio_probe_media_buffer_free(p_iso_elem);
            }
            else if (p_ase_entry->stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                app_lea_uca_active_stream_link->cis_left_ch_conn_handle = 0;
                app_lea_uca_active_stream_link->cis_right_ch_conn_handle = 0;
            }
        }
    }
    else
    {
        p_ase_entry = app_lea_ascs_find_ase_entry_non_conn(LEA_ASE_DOWN_DIRECT,
                                                           (void *)&p_data->p_bt_direct_iso->conn_handle,
                                                           NULL);
        if (p_ase_entry != NULL && p_ase_entry->track_handle != NULL)
        {
            T_AUDIO_TRACK_STATE track_state;
            uint16_t written_len;
            T_AUDIO_STREAM_STATUS status;
            uint8_t *p_iso_data = p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset;

            audio_track_state_get(p_ase_entry->track_handle, &track_state);
            if (track_state != AUDIO_TRACK_STATE_STARTED)
            {
                ret = 5;
                goto fail_track_write;
            }

            if (p_data->p_bt_direct_iso->iso_sdu_len != 0)
            {
                status = AUDIO_STREAM_STATUS_CORRECT;

                if (p_ase_entry->track_write_len != p_data->p_bt_direct_iso->iso_sdu_len)
                {
                    ret = 6;
                    goto fail_track_write;
                }
            }
            else
            {
                status = AUDIO_STREAM_STATUS_LOST;
            }

            APP_PRINT_INFO5("app_lea_umr_handle_iso_data: one cis, iso_sdu_len 0x%x, p_buf %p, offset %d, p_data %p, data %b",
                            p_data->p_bt_direct_iso->iso_sdu_len, p_data->p_bt_direct_iso->p_buf,
                            p_data->p_bt_direct_iso->offset, p_iso_data, TRACE_BINARY(p_data->p_bt_direct_iso->iso_sdu_len,
                                                                                      p_iso_data));
            audio_track_write(p_ase_entry->track_handle, p_data->p_bt_direct_iso->time_stamp,
                              p_data->p_bt_direct_iso->pkt_seq_num,
                              status,
                              FIXED_FRAME_NUM,
                              p_iso_data,
                              p_data->p_bt_direct_iso->iso_sdu_len,
                              &written_len);
        }
    }

fail_track_write:
    if (ret != 0)
    {
        APP_PRINT_ERROR1("app_lea_uca_handle_iso_data: failed %d", -ret);
    }
}

static void app_lea_uca_audio_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            T_AUDIO_STREAM_TYPE stream_type;
            uint16_t conn_handle = 0;
            T_APP_LE_LINK *p_link = NULL;
            T_LEA_ASE_ENTRY *p_ase_entry = NULL;

#if F_APP_CCP_SUPPORT
            if (app_lea_ccp_get_call_status() != APP_CALL_IDLE)
            {
                conn_handle = app_lea_ccp_get_active_conn_handle();
            }
            else
#endif
#if F_APP_MCP_SUPPORT
            {
                conn_handle = app_lea_mcp_get_active_conn_handle();
            }
#endif

            if ((conn_handle == 0) && (app_lea_uca_active_stream_link != NULL))
            {
                conn_handle = app_lea_uca_active_stream_link->conn_handle;
            }

            if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == false)
            {
                break;
            }

            p_link = app_link_find_le_link_by_conn_handle(conn_handle);

            if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_TRACK, conn_handle,
                                                          param->track_state_changed.handle);
            }
            else
            {
                p_ase_entry = app_lea_ascs_find_ase_entry_non_conn(LEA_ASE_UP_DIRECT, NULL, NULL);
            }

            if ((p_link == NULL) || (p_ase_entry == NULL))
            {
                break;
            }

            if (stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                if (param->track_state_changed.handle == p_ase_entry->track_handle)
                {
                    p_ase_entry->track_state = param->track_state_changed.state;
                    if (p_ase_entry->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
                    {
                        for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
                        {
                            if (p_link->lea_ase_entry[i].used == true && p_ase_entry != &p_link->lea_ase_entry[i])
                            {
                                if (p_link->lea_ase_entry[i].audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
                                {
                                    p_link->lea_ase_entry[i].track_state = p_ase_entry->track_state;
                                    break;
                                }
                            }
                        }
                    }

                    APP_PRINT_INFO1("app_lea_uca_audio_cback: state %d", p_ase_entry->track_state);
                }
            }
        }
        break;

    case AUDIO_EVENT_TRACK_DATA_IND:
        {
            uint32_t timestamp;
            uint16_t seq_num;
            uint8_t frame_num;
            uint16_t read_len;
            uint8_t *buf;
            T_AUDIO_STREAM_STATUS status;

            if (param->track_data_ind.len == 0)
            {
                return;
            }

            buf = audio_probe_media_buffer_malloc(param->track_data_ind.len);

            if (buf == NULL)
            {
                return;
            }

            if (audio_track_read(param->track_data_ind.handle,
                                 &timestamp,
                                 &seq_num,
                                 &status,
                                 &frame_num,
                                 buf,
                                 param->track_data_ind.len,
                                 &read_len) == true)
            {
                uint16_t conn_handle = 0;

#if F_APP_CCP_SUPPORT
                conn_handle = app_lea_ccp_get_active_conn_handle();
#endif
                T_LEA_ASE_ENTRY *p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_TRACK, conn_handle,
                                                                           param->track_data_ind.handle);
#if 0
                T_BT_LE_ISO_SYNC_REF_AP_INFO p_info;
                p_info.dir = 1;
                p_info.cis = 1;
                p_info.conn_handle = p_ase_entry->cis_conn_handle;


                bt_get_le_iso_sync_ref_ap_info(&p_info);

                APP_PRINT_INFO8("AUDIO_EVENT_TRACK_DATA_IND: timestamp 0x%x, seq_num 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
                                timestamp,
                                seq_num,
                                p_info.sdu_seq_num,
                                p_info.cur_sync_ref_point,
                                p_info.pre_sync_ref_point,
                                p_info.accumulate_sw_timer,
                                p_info.iso_interval_us,
                                p_info.group_anchor_point);
#endif
                if (p_ase_entry != NULL)
                {
                    app_lea_uca_send_iso_data(buf, p_ase_entry, param->track_data_ind.len,
                                              true, timestamp);
                }

            }
            audio_probe_media_buffer_free(buf);
        }
        break;
    }
}

void app_lea_uca_init(void)
{
    os_queue_init(&app_lea_uca_left_ch_queue);
    os_queue_init(&app_lea_uca_right_ch_queue);
    os_queue_init(&app_lea_uca_upstream_queue);

    audio_mgr_cback_register(app_lea_uca_audio_cback);
    ble_audio_cback_register(app_lea_uca_ble_audio_cback);
}
#endif
