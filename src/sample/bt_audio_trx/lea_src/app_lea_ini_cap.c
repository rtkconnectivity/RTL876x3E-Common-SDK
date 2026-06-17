/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include <stdlib.h>
#include "os_queue.h"
#include "ble_audio.h"
#include "cap.h"
#include "bap.h"
#include "ble_isoch_def.h"
#include "trace.h"
#if CSIP_SET_COORDINATOR
#include "app_lea_ini_csis.h"
#include "string.h"
#endif
#include "app_lea_ini_cap.h"
#include "app_lea_ini_audio_data.h"
#include "ble_audio.h"
#include "metadata_def.h"
#include "app_main.h"
#include "app_lea_ini_ccp.h"
#include "app_lea_ini_mcp.h"
#include "app_lea_ini_cis_cfg.h"

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_lea.h"
#include "app_tri_dongle_mgr.h"
#include "app_tri_dongle_cmd.h"
#include "csis_client.h"
#include "app_bond.h"
#include "app_tri_dongle_bond_manager.h"
#endif

#if F_SOURCE_PLAY_SUPPORT
#include "app_src_play.h"
#endif

#if VCP_VOLUME_CONTROLLER
#include "vcs_client.h"
#endif

#define CAP_METADATA_DATA_MAX_LEN 20
uint8_t lea_ini_metadata[CAP_METADATA_DATA_MAX_LEN];
uint8_t lea_ini_metadata_len = 0;

#define GAMING_QOS_RTN                          (2)
#define GAMING_DURATION_7_5_MS_QOS_LATENCY      (0x08)
#define GAMING_DURATION_10_MS_QOS_LATENCY       (0x0A)

static void unicast_src_update_qos_cfg(T_AUDIO_STREAM_SESSION_HANDLE handle, uint8_t prefer_idx,
                                       void *buf)
{
    uint8_t ret = 0;
    T_AUDIO_SESSION_QOS_CFG default_session_cfg = {0x0};
    T_AUDIO_SESSION_QOS_CFG session_cfg = {0x0};
    T_BAP_UNICAST_SESSION_INFO session_info;
    T_BAP_UNICAST_DEV_INFO *p_dev = NULL;
    T_AUDIO_ASE_QOS_CFG default_ase_qos = {0x0};
    T_AUDIO_ASE_QOS_CFG ase_qos = {0x0};
    T_AUDIO_GROUP_BAP_START_QOS_CFG *p_data = (T_AUDIO_GROUP_BAP_START_QOS_CFG *)buf;
    T_CODEC_CFG codec_cfg;

    if ((!handle) || (!buf))
    {
        ret = 1;
        goto fail;
    }

    if (!bap_unicast_audio_get_session_info(handle, &session_info))
    {
        ret = 2;
        goto fail;
    }

    if (!bap_unicast_audio_get_session_qos(handle, &default_session_cfg))
    {
        ret = 2;
        goto fail;
    }

    memset(&session_cfg, 0, sizeof(T_AUDIO_SESSION_QOS_CFG));
    memcpy(&session_cfg, &default_session_cfg, sizeof(T_AUDIO_SESSION_QOS_CFG));


    session_cfg.latency_s_m = default_session_cfg.latency_s_m;
    session_cfg.latency_m_s = default_session_cfg.latency_m_s;
    session_cfg.sink_presentation_delay = default_session_cfg.sink_presentation_delay;
    session_cfg.source_presentation_delay = default_session_cfg.source_presentation_delay;

#if (LE_AUDIO_CIG_INTERLEAVED == 1)
    session_cfg.packing = 1;    //interleaved
#else
    session_cfg.packing = 0;
#endif

    if (app_db.cis_ull_mode)
    {
        codec_preferred_cfg_get((T_CODEC_CFG_ITEM)prefer_idx, &codec_cfg);
        if (codec_cfg.frame_duration == FRAME_DURATION_CFG_7_5_MS)
        {
            session_cfg.latency_m_s = GAMING_DURATION_7_5_MS_QOS_LATENCY;
            session_cfg.latency_s_m = GAMING_DURATION_7_5_MS_QOS_LATENCY;
        }
        else
        {
            session_cfg.latency_m_s = GAMING_DURATION_10_MS_QOS_LATENCY;
            session_cfg.latency_s_m = GAMING_DURATION_10_MS_QOS_LATENCY;
        }
    }

    if (session_cfg.latency_m_s > p_data->sink_transport_latency_max)
    {
        session_cfg.latency_m_s = p_data->sink_transport_latency_max;
    }
    if (session_cfg.latency_s_m > p_data->source_transport_latency_max)
    {
        session_cfg.latency_s_m = p_data->source_transport_latency_max;
    }

    if ((session_cfg.sink_presentation_delay < p_data->sink_presentation_delay_min) ||
        (session_cfg.sink_presentation_delay > p_data->sink_presentation_delay_max))
    {
        session_cfg.sink_presentation_delay = p_data->sink_presentation_delay_max;
    }

    if ((session_cfg.source_presentation_delay < p_data->source_presentation_delay_min) ||
        (session_cfg.source_presentation_delay > p_data->source_presentation_delay_max))
    {
        session_cfg.source_presentation_delay = p_data->source_presentation_delay_max;
    }

    APP_PRINT_INFO4("unicast_src_update_qos_cfg: m_s 0x%x s_m 0x%x snk_pd 0x%x src_pd 0x%x",
                    session_cfg.latency_m_s, session_cfg.latency_s_m,
                    session_cfg.sink_presentation_delay,
                    session_cfg.source_presentation_delay);

    if (!bap_unicast_audio_cfg_session_qos(handle, &session_cfg))
    {
        ret = 3;
        goto fail;
    }

    for (uint8_t i = 0; i < session_info.dev_num; i++)
    {
        p_dev = &session_info.dev_info[i];
        if (p_dev->conn_state != GAP_CONN_STATE_CONNECTED)
        {
            continue;
        }
        for (uint8_t j = 0; j < p_dev->ase_num; j++)
        {
            if (!bap_unicast_audio_get_ase_qos(handle, p_dev->dev_handle, p_dev->ase_info[j].ase_id,
                                               &default_ase_qos))
            {
                ret = 4;
                goto fail;
            }

            memset(&ase_qos, 0, sizeof(T_AUDIO_ASE_QOS_CFG));
            memcpy(&ase_qos, &default_ase_qos, sizeof(T_AUDIO_ASE_QOS_CFG));
            ase_qos.retransmission_number = default_ase_qos.retransmission_number;

            if (app_db.cis_ull_mode)
            {
                ase_qos.retransmission_number = GAMING_QOS_RTN;
            }

            if (!bap_unicast_audio_cfg_ase_qos(handle, p_dev->dev_handle, p_dev->ase_info[j].ase_id, &ase_qos))
            {
                ret = 5;
                goto fail;
            }
        }
    }
    return;

fail:
    APP_PRINT_INFO2("unicast_src_update_qos_cfg: fail! ret %d, handle %x", -ret, handle);
}

static bool app_lea_get_ase_param(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                  T_BLE_AUDIO_DEV_HANDLE dev_handle,
                                  uint8_t ase_id, T_BAP_UNICAST_ASE_INFO *ase_info)
{
    T_BAP_UNICAST_SESSION_INFO session_info;

    if (bap_unicast_audio_get_session_info(handle, &session_info))
    {
        for (uint8_t i = 0; i < session_info.dev_num; i++)
        {
            if (session_info.dev_info[i].dev_handle == dev_handle)
            {
                for (uint8_t j = 0; j < session_info.dev_info[i].ase_num; j++)
                {
                    if (session_info.dev_info[i].ase_info[j].ase_id == ase_id)
                    {
                        memcpy(ase_info, &session_info.dev_info[i].ase_info, sizeof(T_BAP_UNICAST_ASE_INFO));
                        return true;
                    }
                }
            }
        }
    }

    APP_PRINT_ERROR0("app_lea_get_ase_param fail!");
    return false;
}

T_BLE_AUDIO_GROUP_HANDLE app_lea_ini_find_group_handle_by_idx(uint8_t group_idx)
{
    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);

        if (p_group)
        {
            return p_group->group_handle;
        }
    }
    return NULL;
}

T_APP_LEA_GROUP_INFO *app_lea_find_group_by_conn_handle(uint16_t conn_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;
    for (uint8_t i = 0; i < app_db.group_handle_queue.count; i++)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue, i);
        if (p_group != NULL)
        {
            T_BLE_AUDIO_DEV_HANDLE dev_handle = ble_audio_group_find_dev_by_conn_handle(p_group->group_handle,
                                                                                        conn_handle);
            if (dev_handle)
            {
                return p_group;
            }
        }
    }
    return NULL;
}

T_APP_LEA_GROUP_INFO *app_lea_find_group(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;
    for (uint8_t i = 0; i < app_db.group_handle_queue.count; i++)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue, i);
        if (p_group != NULL && p_group->group_handle == group_handle)
        {
            return p_group;
        }
    }
    return NULL;
}

T_APP_LEA_GROUP_INFO *app_lea_add_group(T_BLE_AUDIO_GROUP_HANDLE group_handle, uint8_t set_mem_size)
{
    T_APP_LEA_GROUP_INFO *p_group;

    p_group = app_lea_find_group(group_handle);
    if (p_group)
    {
        return p_group;
    }
    p_group = calloc(1, sizeof(T_APP_LEA_GROUP_INFO));
    if (p_group)
    {
        p_group->group_handle = group_handle;
        p_group->set_mem_size = set_mem_size;
        APP_PRINT_INFO3("app_lea_add_group: queue_count %d, group_handle 0x%x, set_mem_size %d\r\n",
                        app_db.group_handle_queue.count, p_group->group_handle, p_group->set_mem_size);
        app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_ADD_GROUP, 0,
                         (uint8_t *)&app_db.group_handle_queue.count,
                         sizeof(app_db.group_handle_queue.count));
        os_queue_in(&app_db.group_handle_queue, p_group);
    }
    return p_group;
}

void app_lea_remove_group(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;
    APP_PRINT_TRACE0("app_lea_ini_unicast_audio_stop");
    p_group = app_lea_find_group(group_handle);
    if (p_group)
    {
        if (ble_audio_group_release(&group_handle))
        {
            os_queue_delete(&app_db.group_handle_queue, p_group);
            free(p_group);
        }
    }
}

#if BAP_UNICAST_CLIENT
bool app_lea_ini_unicast_audio_stop(T_BLE_AUDIO_GROUP_HANDLE group_handle, uint32_t cis_timeout_ms)
{
    T_APP_LEA_GROUP_INFO *p_group;
    APP_PRINT_TRACE0("app_lea_ini_unicast_audio_release");
    p_group = app_lea_find_group(group_handle);
    if (p_group != NULL && p_group->lea_unicast.session_handle != NULL)
    {
        if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_PARTIAL_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STARTING)
        {
            if (bap_unicast_audio_stop(p_group->lea_unicast.session_handle, cis_timeout_ms))
            {
                return true;
            }
        }
        else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STOPPING)
        {
            return true;
        }
    }
    APP_PRINT_ERROR1("app_lea_ini_unicast_audio_stop: failed, group_handle 0x%x",
                     group_handle);
    return false;
}

bool app_lea_ini_unicast_audio_release(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;

    p_group = app_lea_find_group(group_handle);
    if (p_group != NULL && p_group->lea_unicast.session_handle != NULL)
    {
        p_group->lea_unicast.release_req = true;
        if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_CONFIGURED ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STARTING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_PARTIAL_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STOPPING)
        {
            if (bap_unicast_audio_release(p_group->lea_unicast.session_handle))
            {
                return true;
            }
        }
        else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE ||
                 p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED)
        {
            if (bap_unicast_audio_remove_session(p_group->lea_unicast.session_handle))
            {
                return true;
            }
        }
        else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_RELEASING)
        {
            return true;
        }
    }
    APP_PRINT_ERROR1("app_lea_ini_unicast_audio_release: failed, group_handle 0x%x",
                     group_handle);
    return false;
}

bool app_lea_ini_ready_to_start(T_APP_LEA_GROUP_INFO *p_group, uint16_t sink_context,
                                uint16_t src_context)
{
    uint8_t dev_num;
    uint8_t ready_dev_num = 0;
    T_AUDIO_DEV_INFO *p_dev_tbl;

    if (p_group->lea_unicast.session_handle)
    {
        if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_RELEASING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STARTING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_PARTIAL_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STOPPING)
        {
            APP_PRINT_ERROR2("app_lea_ini_ready_to_start: failed, group_handle 0x%x, invalid bap_state %d",
                             p_group->group_handle, p_group->lea_unicast.bap_state);
            return false;
        }
    }

    dev_num = ble_audio_group_get_dev_num(p_group->group_handle);
    APP_PRINT_INFO2("app_lea_ini_ready_to_start: dev_num %d, mem_size %d",
                    dev_num, p_group->set_mem_size);

    if (dev_num)
    {
        p_dev_tbl = calloc(1, dev_num * sizeof(T_AUDIO_DEV_INFO));
        if (p_dev_tbl)
        {
            if (ble_audio_group_get_info(p_group->group_handle, &dev_num, p_dev_tbl))
            {
                for (uint8_t j = 0; j < dev_num; j++)
                {
                    if (j == 0)
                    {
                        p_group->lea_unicast.dev_num = 1;
                        p_group->lea_unicast.dev_tbl[0] = p_dev_tbl[j].dev_handle;
                    }
                    else if (j == 1)
                    {
                        p_group->lea_unicast.dev_num = 2;
                        p_group->lea_unicast.dev_tbl[1] = p_dev_tbl[j].dev_handle;
                    }

                    if (p_dev_tbl[j].conn_state == GAP_CONN_STATE_CONNECTED)
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_dev_tbl[j].conn_handle);
                        if (p_link)
                        {
                            if (p_link->lea_srv_found_flags & LEA_LINK_DISC_CSIS_DONE)
                            {
                                if ((p_link->lea_disc_flags & LEA_LINK_DISC_CSIS_DONE) == 0)
                                {
                                    //wait csis read procedure done
                                    continue;
                                }
                            }
                            if (p_link->lea_srv_found_flags & LEA_LINK_DISC_PACS_DONE)
                            {
                                T_BAP_PACS_INFO pacs_info;
                                if (bap_pacs_get_info(p_link->conn_handle, &pacs_info))
                                {
                                    //Check available context
                                    if (sink_context != 0 &&
                                        (pacs_info.snk_avail_context & sink_context) == 0)
                                    {
                                        continue;
                                    }
                                    if (src_context != 0 &&
                                        (pacs_info.src_avail_context & src_context) == 0)
                                    {
                                        continue;
                                    }
                                    p_group->lea_unicast.ready_conn_handle = p_dev_tbl[j].conn_handle;
                                    ready_dev_num++;
                                }
                            }
                        }
                    }
                }
            }
            free(p_dev_tbl);
            if (ready_dev_num)
            {
                return true;
            }
        }
    }
    APP_PRINT_ERROR1("app_lea_ini_ready_to_start: failed, group_handle 0x%x",
                     p_group->group_handle);
    return false;
}

T_UNICAST_AUDIO_CFG_TYPE app_lea_ini_get_topo_cfg(uint32_t sup_mask,
                                                  T_UNICAST_AUDIO_CFG_TYPE *topo_table, uint8_t table_size)
{
    for (uint8_t i = 0; i < table_size; i++)
    {
        if (sup_mask & (1 << topo_table[i]))
        {
            return topo_table[i];
        }
    }

    uint8_t idx = 0;
    while (sup_mask)
    {
        if (sup_mask & 0x01)
        {
            break;
        }
        sup_mask >>= 1;
        idx++;
    }
    return (T_UNICAST_AUDIO_CFG_TYPE)idx;
}

T_CODEC_CFG_ITEM app_lea_ini_get_codec_cfg(uint32_t sup_mask, T_CODEC_CFG_ITEM *codec_table,
                                           uint8_t table_size)
{
    T_CODEC_CFG cfg;
    uint8_t idx = 0;
    for (uint8_t i = 0; i < table_size; i++)
    {
        if (sup_mask & (1 << codec_table[i]))
        {
            if (codec_preferred_cfg_get((T_CODEC_CFG_ITEM)idx, &cfg))
            {
                return codec_table[i];
            }
        }
    }

    idx = 0;
    while (sup_mask)
    {
        if (sup_mask & 0x01)
        {
            if (codec_preferred_cfg_get((T_CODEC_CFG_ITEM)idx, &cfg))
            {
                break;
            }
        }
        sup_mask >>= 1;
        idx++;
    }
    return (T_CODEC_CFG_ITEM)idx;
}

bool app_lea_ini_select_media_prefer_codec(T_APP_LEA_GROUP_INFO *p_group)
{
    uint32_t sup_topo_mask;
    T_UNICAST_AUDIO_CFG_TYPE *p_topo_table = NULL;
    uint8_t topo_table_size = 0;

    uint32_t sup_snk_mask = 0;
    T_CODEC_CFG_ITEM *p_codec_table = NULL;
    uint8_t codec_table_size = 0;

    sup_topo_mask = bap_unicast_get_audio_cfg_mask(p_group->lea_unicast.session_handle,
                                                   LEA_MEDIA_TOPO_CFG_SUPPORT,
                                                   p_group->lea_unicast.dev_num, p_group->lea_unicast.dev_tbl);
    APP_PRINT_INFO1("app_lea_ini_select_media_prefer_codec p1 %d", sup_topo_mask);
    if (sup_topo_mask)
    {
        if (p_group->set_mem_size <= 1)
        {
            p_topo_table = (T_UNICAST_AUDIO_CFG_TYPE *)lea_headset_media_topo;
            topo_table_size = sizeof(lea_headset_media_topo) / sizeof(T_UNICAST_AUDIO_CFG_TYPE);
        }
        else
        {
            p_topo_table = (T_UNICAST_AUDIO_CFG_TYPE *)lea_rws_media_topo;
            topo_table_size = sizeof(lea_rws_media_topo) / sizeof(T_UNICAST_AUDIO_CFG_TYPE);
        }

        p_group->lea_unicast.cfg_type = app_lea_ini_get_topo_cfg(sup_topo_mask, p_topo_table,
                                                                 topo_table_size);

        uint8_t channel_count = p_group->lea_unicast.cfg_type == UNICAST_AUDIO_CFG_4 ? 2 : 1;

        APP_PRINT_INFO3("app_lea_ini_select_media_prefer_codec: member_size %d, sup_topo_mask 0x%4x, topo_item %d",
                        p_group->set_mem_size, sup_topo_mask, p_group->lea_unicast.cfg_type);

        sup_snk_mask = bap_pacs_get_lc3_snk_table_msk(p_group->lea_unicast.ready_conn_handle,
                                                      AUDIO_CONTEXT_MEDIA,
                                                      channel_count, 1);

        if (!sup_snk_mask)
        {
            sup_snk_mask = bap_pacs_get_lc3_snk_table_msk(p_group->lea_unicast.ready_conn_handle,
                                                          AUDIO_CONTEXT_UNSPECIFIED,
                                                          channel_count, 1);
        }

        if (sup_snk_mask != 0)
        {
#if F_APP_ATTACH_NREC_SUPPORT
            if (app_src_play_is_nrec_mode())
            {
                p_codec_table = (T_CODEC_CFG_ITEM *)lea_nrec_codec;
                codec_table_size = sizeof(lea_nrec_codec) / sizeof(T_CODEC_CFG_ITEM);
            }
            else
#endif
            {
                if (app_db.cis_ull_mode)
                {
                    p_codec_table = (T_CODEC_CFG_ITEM *)lea_media_gaming_codec;
                    codec_table_size = sizeof(lea_media_gaming_codec) / sizeof(T_CODEC_CFG_ITEM);
                }
                else
                {
                    p_codec_table = (T_CODEC_CFG_ITEM *)lea_media_codec;
                    codec_table_size = sizeof(lea_media_codec) / sizeof(T_CODEC_CFG_ITEM);
                }
            }
            p_group->lea_unicast.codec_cfg_item = app_lea_ini_get_codec_cfg(sup_snk_mask, p_codec_table,
                                                                            codec_table_size);
            APP_PRINT_INFO2("app_lea_ini_select_media_prefer_codec: sup_codec_mask 0x%4x, codec_item %d",
                            sup_snk_mask, p_group->lea_unicast.codec_cfg_item);
            return true;
        }
    }

    return false;
}

bool app_lea_ini_select_conversation_prefer_codec(T_APP_LEA_GROUP_INFO *p_group)
{
    uint32_t sup_topo_mask;
    T_UNICAST_AUDIO_CFG_TYPE *p_topo_table = NULL;
    uint8_t topo_table_size = 0;

    uint32_t sup_snk_mask = 0;
    T_CODEC_CFG_ITEM *p_codec_table = NULL;
    uint8_t codec_table_size = 0;

    sup_topo_mask = bap_unicast_get_audio_cfg_mask(p_group->lea_unicast.session_handle,
                                                   LEA_CONVS_TOPO_CFG_SUPPORT,
                                                   p_group->lea_unicast.dev_num, p_group->lea_unicast.dev_tbl);
    APP_PRINT_INFO1("app_lea_ini_select_media_prefer_codec p2 %d", sup_topo_mask);
    if (sup_topo_mask)
    {
        if (p_group->set_mem_size <= 1)
        {
            p_topo_table = (T_UNICAST_AUDIO_CFG_TYPE *)lea_headset_conv_topo;
            topo_table_size = sizeof(lea_headset_conv_topo) / sizeof(T_UNICAST_AUDIO_CFG_TYPE);
        }
        else
        {
            p_topo_table = (T_UNICAST_AUDIO_CFG_TYPE *)lea_rws_conv_topo;
            topo_table_size = sizeof(lea_rws_conv_topo) / sizeof(T_UNICAST_AUDIO_CFG_TYPE);
        }

        p_group->lea_unicast.cfg_type = app_lea_ini_get_topo_cfg(sup_topo_mask, p_topo_table,
                                                                 topo_table_size);
        APP_PRINT_INFO3("app_lea_ini_select_conversation_prefer_codec: member_size %d, sup_topo_mask 0x%4x, topo_item %d",
                        p_group->set_mem_size, sup_topo_mask, p_group->lea_unicast.cfg_type);

        uint8_t channel_count = p_group->lea_unicast.cfg_type == UNICAST_AUDIO_CFG_5 ? 2 : 1;

        sup_snk_mask = bap_pacs_get_lc3_snk_table_msk(p_group->lea_unicast.ready_conn_handle,
                                                      AUDIO_CONTEXT_CONVERSATIONAL,
                                                      channel_count, 1);
        if (!sup_snk_mask)
        {
            sup_snk_mask = bap_pacs_get_lc3_snk_table_msk(p_group->lea_unicast.ready_conn_handle,
                                                          AUDIO_CONTEXT_UNSPECIFIED,
                                                          channel_count, 1);
        }
        if (sup_snk_mask != 0)
        {
            p_codec_table = (T_CODEC_CFG_ITEM *)lea_conv_codec;
            codec_table_size = sizeof(lea_conv_codec) / sizeof(T_CODEC_CFG_ITEM);
            p_group->lea_unicast.codec_cfg_item = app_lea_ini_get_codec_cfg(sup_snk_mask, p_codec_table,
                                                                            codec_table_size);
            APP_PRINT_INFO2("app_lea_ini_select_conversation_prefer_codec: sup_codec_mask 0x%4x, codec_item %d",
                            sup_snk_mask, p_group->lea_unicast.codec_cfg_item);
            return true;
        }
    }

    return false;
}

void app_lea_ini_gen_metadata(uint16_t contexts_type, uint8_t ccid_num, uint8_t *p_ccid)
{
    uint8_t idx = 0;
    //contexts_type
    lea_ini_metadata[idx++] = 0x03;
    lea_ini_metadata[idx++] = METADATA_TYPE_STREAMING_AUDIO_CONTEXTS;
    LE_UINT16_TO_ARRAY(lea_ini_metadata + idx,
                       contexts_type);
    idx += 2;
    //ccid
    if (ccid_num && p_ccid != NULL)
    {
        lea_ini_metadata[idx++] = 1 + ccid_num;
        lea_ini_metadata[idx++] = METADATA_TYPE_CCID_LIST;
        memcpy(lea_ini_metadata + idx, p_ccid, ccid_num);
        idx += ccid_num;
    }
    lea_ini_metadata_len = idx;
}

uint32_t app_lea_ini_get_channel_location(T_BAP_PACS_INFO *p_info, uint8_t direction,
                                          uint8_t channel_num, uint32_t used_loc)
{
    uint32_t temp_audio_loc;
    uint32_t audio_loc = 0;
    uint16_t index = 0;
    uint8_t num = 0;

    if (direction == SERVER_AUDIO_SINK)
    {
        temp_audio_loc = p_info->snk_audio_loc;
    }
    else
    {
        temp_audio_loc = p_info->src_audio_loc;
    }
    temp_audio_loc &= ~used_loc;
    while (temp_audio_loc)
    {
        if (temp_audio_loc & 0x01)
        {
            audio_loc |= (1 << index);
            num++;
        }
        temp_audio_loc >>= 1;
        index++;
        if (num >= channel_num)
        {
            break;
        }
    }
    if (num != channel_num)
    {
        APP_PRINT_TRACE3("app_lea_ini_get_channel_location: get failed, direction 0x%x, channel_num %d, used_loc 0x%x",
                         direction, channel_num, used_loc);
        return 0;
    }
    return audio_loc;
}

static void app_lea_ini_copy_pacs(T_BAP_PACS_INFO *pacs_info_cpy, T_BAP_PACS_INFO *pacs_info_raw)
{
    memcpy(pacs_info_cpy, pacs_info_raw, sizeof(T_BAP_PACS_INFO));
    if (pacs_info_raw->snk_audio_loc == AUDIO_LOCATION_FL)
    {
        pacs_info_cpy->snk_audio_loc = AUDIO_LOCATION_FR;
    }
    else if (pacs_info_raw->snk_audio_loc == AUDIO_LOCATION_FR)
    {
        pacs_info_cpy->snk_audio_loc = AUDIO_LOCATION_FL;
    }
    APP_PRINT_INFO2("app_lea_ini_copy_pacs: loc %d -> %d",
                    pacs_info_raw->snk_audio_loc,
                    pacs_info_cpy->snk_audio_loc);
}

static bool app_lea_ini_copy_available_pacs(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                            T_BAP_UNICAST_SESSION_INFO *p_info,
                                            T_BAP_PACS_INFO *cpy_pacs_info)
{
    T_AUDIO_DEV_INFO dev_info;
    T_BAP_PACS_INFO pacs_info;

    for (uint8_t i = 0; i < p_info->dev_num; i++)
    {
        if (ble_audio_group_get_dev_info(group_handle,
                                         p_info->dev_info[i].dev_handle, &dev_info) == false)
        {
            continue;
        }
        if (dev_info.conn_state == GAP_CONN_STATE_CONNECTED)
        {
            if (bap_pacs_get_info(dev_info.conn_handle, &pacs_info))
            {
                app_lea_ini_copy_pacs(cpy_pacs_info, &pacs_info);
                return true;
            }
        }
    }
    APP_PRINT_ERROR0("app_lea_ini_copy_available_pacs: failed");
    return false;
}

bool app_lea_ini_config_codec(T_APP_LEA_GROUP_INFO *p_group)
{
    T_BAP_UNICAST_SESSION_INFO  session_info;
    T_AUDIO_ASE_CODEC_CFG       ase_codec_cfg = {0};
    bool                        reserved_flag = false;

    app_lea_ini_gen_metadata(p_group->lea_unicast.contexts_type, 0, NULL);
    if (codec_preferred_cfg_get(p_group->lea_unicast.codec_cfg_item, &ase_codec_cfg.codec_cfg) == false)
    {
        goto failed;
    }
    ase_codec_cfg.codec_id[0] = LC3_CODEC_ID;
    ase_codec_cfg.target_latency = p_group->lea_unicast.target_latency;
    ase_codec_cfg.target_phy = ASE_TARGET_PHY_2M;

    if (bap_unicast_audio_get_session_info(p_group->lea_unicast.session_handle, &session_info))
    {
#if 1
        APP_PRINT_INFO4("app_lea_ini_config_codec: session info, state %d, cfg_type %d, conn_dev_num %d, dev_num %d",
                        session_info.state, session_info.cfg_type,
                        session_info.conn_dev_num, session_info.dev_num);
        for (uint8_t i = 0; i < session_info.dev_num; i++)
        {
            APP_PRINT_INFO4("dev_info[%d]: dev_handle %p, conn_state %d, ase_num %d",
                            i, session_info.dev_info[i].dev_handle,
                            session_info.dev_info[i].conn_state,
                            session_info.dev_info[i].ase_num);
            for (uint8_t j = 0; j < session_info.dev_info[i].ase_num; j++)
            {
                APP_PRINT_INFO6("ase_info[%d]: ase_id %d, cfg_idx %d, ase_state 0x%x, direction %d, channel_num %d",
                                j, session_info.dev_info[i].ase_info[j].ase_id,
                                session_info.dev_info[i].ase_info[j].cfg_idx,
                                session_info.dev_info[i].ase_info[j].ase_state,
                                session_info.dev_info[i].ase_info[j].direction,
                                session_info.dev_info[i].ase_info[j].channel_num);
            }
        }
#endif
        uint32_t sink_channel_allocation;
        uint32_t source_channel_allocation;

        p_group->lea_unicast.sink_used_location = 0;
        p_group->lea_unicast.source_used_location = 0;
        ase_codec_cfg.codec_cfg.type_exist |= CODEC_CFG_AUDIO_CHANNEL_ALLOCATION_EXIST;

        for (uint8_t i = 0; i < session_info.dev_num; i++)
        {
            T_AUDIO_DEV_INFO dev_info;
            T_BAP_PACS_INFO pacs_info;

            if (ble_audio_group_get_dev_info(p_group->group_handle,
                                             session_info.dev_info[i].dev_handle, &dev_info) == false)
            {
                continue;
            }

            if (dev_info.conn_state == GAP_CONN_STATE_CONNECTED)
            {
                if (bap_pacs_get_info(dev_info.conn_handle, &pacs_info) == false)
                {
                    continue;
                }
            }
            else
            {
                APP_PRINT_INFO1("app_lea_ini_config_codec: dev %d not connected, try copy available pacs", i);
                if (app_lea_ini_copy_available_pacs(p_group->group_handle, &session_info, &pacs_info) == false)
                {
                    continue;
                }
            }

            for (uint8_t j = 0; j < session_info.dev_info[i].ase_num; j++)
            {
                if (session_info.dev_info[i].ase_info[j].direction == SERVER_AUDIO_SINK)
                {
                    sink_channel_allocation = app_lea_ini_get_channel_location(&pacs_info,
                                                                               session_info.dev_info[i].ase_info[j].direction,
                                                                               session_info.dev_info[i].ase_info[j].channel_num,
                                                                               p_group->lea_unicast.sink_used_location);
                    ase_codec_cfg.codec_cfg.audio_channel_allocation = sink_channel_allocation;
                    p_group->lea_unicast.sink_used_location |= sink_channel_allocation;
                }
                else
                {
                    source_channel_allocation = app_lea_ini_get_channel_location(&pacs_info,
                                                                                 session_info.dev_info[i].ase_info[j].direction,
                                                                                 session_info.dev_info[i].ase_info[j].channel_num,
                                                                                 p_group->lea_unicast.source_used_location);
                    ase_codec_cfg.codec_cfg.audio_channel_allocation = source_channel_allocation;
                    p_group->lea_unicast.source_used_location |= source_channel_allocation;
                }
                if (bap_unicast_audio_cfg_ase_codec(p_group->lea_unicast.session_handle,
                                                    session_info.dev_info[i].dev_handle,
                                                    session_info.dev_info[i].ase_info[j].cfg_idx, &ase_codec_cfg) == false)
                {
                    goto failed;
                }
            }
        }
    }
    else
    {
        goto failed;
    }
    return true;
failed:
    return false;
}

static bool app_lea_ini_start_media(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;
    uint8_t err_idx = 0;

    p_group = app_lea_find_group(group_handle);
    if (p_group)
    {
        if (app_lea_ini_ready_to_start(p_group, AUDIO_CONTEXT_MEDIA, 0))
        {
            if (p_group->lea_unicast.session_handle == NULL)
            {
                p_group->lea_unicast.session_handle = audio_stream_session_allocate(p_group->group_handle);
                if (p_group->lea_unicast.session_handle == NULL)
                {
                    err_idx = 1;
                    goto failed;
                }
#if MCP_MEDIA_CONTROL_SERVER
                if (app_db.mcp_db.mcp_enabled_cccd)
                {
                    p_group->lea_unicast.ccid_num = 1;
                    p_group->lea_unicast.ccid[0] = APP_LEA_GMCS_CCID;
                }
#endif
                p_group->lea_unicast.bap_state = AUDIO_STREAM_STATE_IDLE;
                p_group->lea_unicast.contexts_type = AUDIO_CONTEXT_MEDIA;
                p_group->lea_unicast.target_latency = ASE_TARGET_HIGHER_RELIABILITY;
            }

            if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE)
            {
                if (app_lea_ini_select_media_prefer_codec(p_group) == false)
                {
                    err_idx = 2;
                    goto failed;
                }
                if (bap_unicast_audio_cfg(p_group->lea_unicast.session_handle, p_group->lea_unicast.cfg_type,
                                          p_group->lea_unicast.dev_num, p_group->lea_unicast.dev_tbl) == false)
                {
                    err_idx = 3;
                    goto failed;
                }
                if (app_lea_ini_config_codec(p_group) == false)
                {
                    err_idx = 4;
                    goto failed;
                }
                if (bap_unicast_audio_start(p_group->lea_unicast.session_handle) == false)
                {
                    err_idx = 5;
                    goto failed;
                }
            }
            else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED ||
                     p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_CONFIGURED)
            {
                if (p_group->lea_unicast.contexts_type != AUDIO_CONTEXT_MEDIA)
                {
                    err_idx = 6;
                    goto failed;
                }
                if (bap_unicast_audio_start(p_group->lea_unicast.session_handle) == false)
                {
                    err_idx = 7;
                    goto failed;
                }
            }
            else
            {
                err_idx = 8;
                goto failed;
            }
        }
        else
        {
            err_idx = 9;
            goto failed;
        }
    }
    else
    {
        err_idx = 10;
        goto failed;
    }
    return true;
failed:
    APP_PRINT_ERROR2("app_lea_ini_start_media: failed, group_handle 0x%x, err_idx %d",
                     group_handle, err_idx);
    return false;
}

bool app_lea_ini_cis_media_start(uint8_t group_idx)
{
    bool ret = false;

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            ret = app_lea_ini_start_media(p_group->group_handle);
        }
    }

    return ret;
}

bool app_lea_ini_cis_stream_stop(uint8_t group_idx, bool release)
{
    bool ret = false;
    APP_PRINT_TRACE0("app_lea_ini_cis_stream_stop");
    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            if (release)
            {
                ret = app_lea_ini_unicast_audio_release(p_group->group_handle);
            }
            else
            {
                ret = app_lea_ini_unicast_audio_stop(p_group->group_handle, 0);
            }

        }
    }
    return ret;
}

bool app_lea_ini_cis_conversation_start(uint8_t group_idx)
{
    bool ret = false;

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            ret = app_lea_ini_start_conversation(p_group->group_handle);
        }
    }

    return ret;
}

bool app_lea_ini_start_conversation(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;
    uint8_t err_idx = 0;

    p_group = app_lea_find_group(group_handle);
    if (p_group)
    {
        if (app_lea_ini_ready_to_start(p_group, AUDIO_CONTEXT_CONVERSATIONAL, AUDIO_CONTEXT_CONVERSATIONAL))
        {
            if (p_group->lea_unicast.session_handle == NULL)
            {
                p_group->lea_unicast.session_handle = audio_stream_session_allocate(p_group->group_handle);
                if (p_group->lea_unicast.session_handle == NULL)
                {
                    err_idx = 1;
                    goto failed;
                }
#if CCP_CALL_CONTROL_SERVER
                if (app_db.ccp_db.ccp_enabled_cccd)
                {
                    p_group->lea_unicast.ccid_num = 1;
                    p_group->lea_unicast.ccid[0] = APP_LEA_GTBS_CCID;
                }
#endif
                p_group->lea_unicast.bap_state = AUDIO_STREAM_STATE_IDLE;
                p_group->lea_unicast.contexts_type = AUDIO_CONTEXT_CONVERSATIONAL;
                p_group->lea_unicast.target_latency = ASE_TARGET_LOWER_LATENCY;
            }

            if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE)
            {
                if (app_lea_ini_select_conversation_prefer_codec(p_group) == false)
                {
                    err_idx = 2;
                    goto failed;
                }
                if (bap_unicast_audio_cfg(p_group->lea_unicast.session_handle, p_group->lea_unicast.cfg_type,
                                          p_group->lea_unicast.dev_num, p_group->lea_unicast.dev_tbl) == false)
                {
                    err_idx = 3;
                    goto failed;
                }
                if (app_lea_ini_config_codec(p_group) == false)
                {
                    err_idx = 4;
                    goto failed;
                }
                if (bap_unicast_audio_start(p_group->lea_unicast.session_handle) == false)
                {
                    err_idx = 5;
                    goto failed;
                }
            }
            else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED ||
                     p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_CONFIGURED)
            {
                if (p_group->lea_unicast.contexts_type != AUDIO_CONTEXT_CONVERSATIONAL)
                {
                    err_idx = 6;
                    goto failed;
                }
                if (bap_unicast_audio_start(p_group->lea_unicast.session_handle) == false)
                {
                    err_idx = 7;
                    goto failed;
                }
            }
            else
            {
                err_idx = 8;
                goto failed;
            }
        }
        else
        {
            err_idx = 9;
            goto failed;
        }
    }
    else
    {
        err_idx = 10;
        goto failed;
    }
    return true;
failed:
    APP_PRINT_ERROR2("app_lea_ini_start_conversation: failed, group_handle 0x%x, err_idx %d",
                     group_handle, err_idx);
    return false;
}
#endif

// TODO: CSIP_SET_COORDINATOR
void app_lea_ini_group_cb(T_AUDIO_GROUP_MSG msg, T_BLE_AUDIO_GROUP_HANDLE handle,
                          void *buf)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_APP_LEA_GROUP_INFO *p_group = app_lea_find_group(handle);
    switch (msg)
    {
    case AUDIO_GROUP_MSG_DEV_CONN:
        {
            T_AUDIO_GROUP_MSG_DEV_CONN *p_data = (T_AUDIO_GROUP_MSG_DEV_CONN *)buf;
            APP_PRINT_INFO2("AUDIO_GROUP_MSG_DEV_CONN: group handle 0x%x, dev handle 0x%x",
                            handle, p_data->dev_handle);
        }
        break;

    case AUDIO_GROUP_MSG_DEV_DISCONN:
        {
            T_AUDIO_GROUP_MSG_DEV_DISCONN *p_data = (T_AUDIO_GROUP_MSG_DEV_DISCONN *)buf;
            APP_PRINT_INFO3("AUDIO_GROUP_MSG_DEV_DISCONN: group handle 0x%x, dev handle 0x%x, cause 0x%x",
                            handle, p_data->dev_handle, p_data->cause);
        }
        break;

    case AUDIO_GROUP_MSG_DEV_BOND_REMOVE:
        {
            T_AUDIO_GROUP_MSG_DEV_BOND_REMOVE *p_data = (T_AUDIO_GROUP_MSG_DEV_BOND_REMOVE *)buf;
            APP_PRINT_INFO2("AUDIO_GROUP_MSG_DEV_BOND_REMOVE: group handle 0x%x, dev handle 0x%x",
                            handle, p_data->dev_handle);
            bool ret = ble_audio_group_del_dev(handle, &p_data->dev_handle);
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
            if (ret)
            {
                set_coordinator_cfg_discover(NULL, false, 0);
            }
#endif
        }
        break;

#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE == 0)
    case AUDIO_GROUP_MSG_DEV_BOND_CLEAR:
        {
            APP_PRINT_INFO1("AUDIO_GROUP_MSG_DEV_BOND_CLEAR: group handle 0x%x", handle);
            app_lea_remove_group(handle);
        }
        break;

    case AUDIO_GROUP_MSG_DEV_EMPTY:
        {
            APP_PRINT_INFO1("AUDIO_GROUP_MSG_DEV_EMPTY: group handle 0x%x", handle);
            app_lea_remove_group(handle);
        }
        break;
#endif

#if BAP_UNICAST_CLIENT
    case AUDIO_GROUP_MSG_BAP_STATE:
        {
            T_AUDIO_GROUP_BAP_STATE *p_data = (T_AUDIO_GROUP_BAP_STATE *)buf;
#if F_APP_AUTO_POWER_TEST_LOG
            TEST_PRINT_INFO1("AUDIO_GROUP_MSG_BAP_STATE: state %d", p_data->state);
#else
            APP_PRINT_INFO6("AUDIO_GROUP_MSG_BAP_STATE: group handle 0x%x, session handle 0x%x, curr_action %d, state %d, result %d, cause 0x%x",
                            handle, p_data->handle, p_data->curr_action,
                            p_data->state, p_data->result, p_data->cause);
#endif
            struct
            {
                void *group_handle;
                uint8_t state;
            } __attribute__((packed)) rpt = {};
            rpt.group_handle = handle;
            rpt.state = p_data->state;
            app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_BAP_STATE, 0, (uint8_t *)&rpt,
                             sizeof(rpt));

            if (p_group != NULL && p_group->lea_unicast.session_handle == p_data->handle)
            {
                p_group->lea_unicast.bap_state = p_data->state;
                if ((p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE ||
                     p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED) &&
                    p_group->lea_unicast.release_req == true)
                {
                    bap_unicast_audio_remove_session(p_group->lea_unicast.session_handle);
                }

                if (p_data->state == AUDIO_STREAM_STATE_CONFIGURED &&
                    p_data->result)
                {
                    app_lea_ini_unicast_audio_release(handle);
                }
            }
        }
        break;

    case AUDIO_GROUP_MSG_BAP_START_QOS_CFG:
        {
            T_AUDIO_GROUP_BAP_START_QOS_CFG *p_data = (T_AUDIO_GROUP_BAP_START_QOS_CFG *)buf;
            APP_PRINT_INFO4("AUDIO_GROUP_MSG_BAP_START_QOS_CFG: group handle 0x%x, session handle 0x%x, sink latency 0x%x, source latency 0x%x",
                            handle, p_data->handle,
                            p_data->sink_transport_latency_max, p_data->source_transport_latency_max);
            APP_PRINT_INFO8("AUDIO_GROUP_MSG_BAP_START_QOS_CFG: sink preferred presentation delay(0x%x-0x%x), sink presentation delay(0x%x-0x%x), source preferred presentation delay(0x%x-0x%x), source presentation delay(0x%x-0x%x)",
                            p_data->sink_preferred_presentation_delay_min, p_data->sink_preferred_presentation_delay_max,
                            p_data->sink_presentation_delay_min, p_data->sink_presentation_delay_max,
                            p_data->source_preferred_presentation_delay_min, p_data->source_preferred_presentation_delay_max,
                            p_data->source_presentation_delay_min, p_data->source_presentation_delay_max);
            if (p_group != NULL)
            {
                unicast_src_update_qos_cfg(p_data->handle, p_group->lea_unicast.codec_cfg_item, buf);
            }
        }
        break;

    case AUDIO_GROUP_MSG_BAP_CREATE_CIS:
        {
            T_AUDIO_GROUP_BAP_CREATE_CIS *p_data = (T_AUDIO_GROUP_BAP_CREATE_CIS *)buf;
            APP_PRINT_INFO3("AUDIO_GROUP_MSG_BAP_CREATE_CIS: group handle 0x%x, session handle 0x%x, dev_num 0x%x",
                            handle, p_data->handle, p_data->dev_num);
            if (p_group != NULL && p_group->lea_unicast.session_handle == p_data->handle)
            {
                for (uint8_t i = 0; i < p_data->dev_num; i++)
                {
                    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle_tbl[i]);
                    if (p_link)
                    {
                        app_ini_disable_conn_update(p_link->bd_addr);
                    }
                }
            }
        }
        break;
    case AUDIO_GROUP_MSG_BAP_CIS_DISCONN:
        {
            T_AUDIO_GROUP_BAP_CIS_DISCONN *p_data = (T_AUDIO_GROUP_BAP_CIS_DISCONN *)buf;
            APP_PRINT_INFO4("AUDIO_GROUP_MSG_BAP_CIS_DISCONN: group handle 0x%x, session handle 0x%x, conn_handle 0x%x, reason 0x%x",
                            handle, p_data->handle, p_data->conn_handle, p_data->cause);
            if (p_group != NULL && p_group->lea_unicast.session_handle == p_data->handle)
            {
                T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);
                if (p_link)
                {
                    app_lea_ini_handle_dev_state_change_event(p_link->bd_addr, CP_EVENT_CIS_DISCONNECT);
                    app_ini_enable_conn_update(p_link->bd_addr);
                }
            }
        }
        break;

    case AUDIO_GROUP_MSG_BAP_START_METADATA_CFG:
        {
            T_AUDIO_GROUP_BAP_START_METADATA_CFG *p_data = (T_AUDIO_GROUP_BAP_START_METADATA_CFG *)buf;
            T_AUDIO_DEV_INFO dev_info;
            T_AUDIO_SESSION_QOS_CFG qos_cfg;
            T_BAP_UNICAST_ASE_INFO ase_info;

            APP_PRINT_INFO4("AUDIO_GROUP_MSG_BAP_START_METADATA_CFG: group handle 0x%x, session handle 0x%x, dev_handle 0x%x, ase_id %d",
                            handle, p_data->handle, p_data->dev_handle, p_data->ase_id);
            bap_unicast_audio_cfg_ase_metadata(p_data->handle, p_data->dev_handle, p_data->ase_id,
                                               lea_ini_metadata_len, (uint8_t *)lea_ini_metadata);

            if (app_lea_get_ase_param(p_data->handle, p_data->dev_handle, p_data->ase_id, &ase_info))
            {
                if (ase_info.direction == SERVER_AUDIO_SINK)
                {
                    if (bap_unicast_audio_get_session_qos(p_data->handle, &qos_cfg) &&
                        ble_audio_group_get_dev_info(handle, p_data->dev_handle, &dev_info))
                    {
                        app_lea_ini_handle_dev_state_change_event(dev_info.bd_addr, CP_EVENT_CIS_ESTABLISH);
                    }
                }
            }
        }
        break;

    case AUDIO_GROUP_MSG_BAP_SESSION_REMOVE:
        {
            T_AUDIO_GROUP_BAP_SESSION_REMOVE *p_data = (T_AUDIO_GROUP_BAP_SESSION_REMOVE *)buf;

            APP_PRINT_INFO2("AUDIO_GROUP_MSG_BAP_SESSION_REMOVE: group handle 0x%x, session handle 0x%x",
                            handle, p_data->handle);
            if (p_group != NULL && p_group->lea_unicast.session_handle == p_data->handle)
            {
                memset(&p_group->lea_unicast, 0, sizeof(T_APP_LEA_UNICAST));
            }
        }
        break;

    case AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH:
        {
            T_AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH *p_data = (T_AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH *)buf;
            APP_PRINT_INFO6("AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH: group handle 0x%x, session handle 0x%x, dev_handle 0x%x, ase_id 0x%x, direction %d, cis_conn_handle 0x%x",
                            handle, p_data->handle,
                            p_data->dev_handle,
                            p_data->ase_id,
                            p_data->path_direction,
                            p_data->cis_conn_handle);
            T_LEA_SETUP_DATA_PATH data = {0};
            data.iso_mode = CIG_ISO_MODE;
            data.iso_conn_handle = p_data->cis_conn_handle;
            data.codec_parsed_data = p_data->codec_parsed_data;
            data.path_direction = p_data->path_direction;
            // data.presentation_delay = 40000; //TODO: Need?
            if (p_group != NULL && p_group->lea_unicast.session_handle == p_data->handle)
            {
                T_AUDIO_SESSION_QOS_CFG qos_cfg;
                if (bap_unicast_audio_get_session_qos(p_group->lea_unicast.session_handle, &qos_cfg))
                {
                    if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
                    {
                        data.presentation_delay = qos_cfg.sink_presentation_delay;
                    }
                    else
                    {
                        data.presentation_delay = qos_cfg.source_presentation_delay;
                    }
                }
            }
            app_lea_handle_cis_data_path_setup(&data);
        }
        break;

    case AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH:
        {
            T_AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH *p_data = (T_AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH *)buf;
            APP_PRINT_INFO7("AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH: group handle 0x%x, session handle 0x%x, dev_handle 0x%x, ase_id 0x%x, direction %d, cis_conn_handle 0x%x, cause 0x%x",
                            handle, p_data->handle,
                            p_data->dev_handle,
                            p_data->ase_id,
                            p_data->path_direction,
                            p_data->cis_conn_handle,
                            p_data->cause);
            T_LEA_REMOVE_DATA_PATH data;
            data.iso_mode = CIG_ISO_MODE;
            data.iso_conn_handle = p_data->cis_conn_handle;
            data.path_direction = p_data->path_direction;
            app_lea_handle_cis_data_path_remove(&data);
        }
        break;

    case AUDIO_GROUP_MSG_BAP_METADATA_UPDATE:
        {
            T_AUDIO_GROUP_MSG_BAP_METADATA_UPDATE *p_data = (T_AUDIO_GROUP_MSG_BAP_METADATA_UPDATE *)buf;
            APP_PRINT_INFO6("AUDIO_GROUP_MSG_BAP_METADATA_UPDATE: group handle 0x%x, session handle 0x%x, dev_handle 0x%x, ase_id 0x%x, metadata_length %d, p_metadata %b",
                            handle, p_data->handle,
                            p_data->dev_handle,
                            p_data->ase_id,
                            p_data->metadata_length,
                            TRACE_BINARY(p_data->metadata_length, p_data->p_metadata));
        }
        break;
#endif
    default:
        break;
    }
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_tri_dongle_lea_ua_group_cb(msg, handle, buf);
    if ((msg == AUDIO_GROUP_MSG_DEV_BOND_CLEAR) ||
        (msg == AUDIO_GROUP_MSG_DEV_EMPTY))
    {
        app_lea_remove_group(handle);
    }
#endif
    return;
}

uint16_t app_lea_ini_cap_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_APP_LE_LINK *p_link = NULL;

    switch (msg)
    {
    case LE_AUDIO_MSG_CAP_DIS_DONE:
        {
            T_CAP_DIS_DONE *p_data = (T_CAP_DIS_DONE *)buf;
            APP_PRINT_INFO6("LE_AUDIO_MSG_CAP_DIS_DONE: conn_handle 0x%x, load_from_ftl %d, cas_is_found %d, cas_inc_csis %d, vcs_is_found %d, mics_is_found %d",
                            p_data->conn_handle,
                            p_data->load_from_ftl,
                            p_data->cas_is_found,
                            p_data->cas_inc_csis,
                            p_data->vcs_is_found,
                            p_data->mics_is_found);

            p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);
            if (p_link)
            {
                if (p_data->cas_is_found)
                {
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
                    app_tri_dongle_set_lea_group_mem_dev(p_link->bd_addr, BOND_FLAG_LEA);
                    if (app_tri_dongle_get_lea_group_mem_type(p_link->bd_addr) == BOND_FLAG_LEA)
                    {
                        app_bond_le_set_bond_flag((void *)p_link, BOND_FLAG_LEA);
                        p_link->ble_device_type = BOND_FLAG_LEA;
                        APP_PRINT_INFO0("CAP_DIS_DONE Wearable Audio Device");
                        app_bond_mgr_store_refresh_lea_le_bond(p_link->conn_handle, p_link->ble_device_type);
                        app_set_lea_connecting_status(false);
                    }
#endif
                    p_link->lea_disc_flags |= LEA_LINK_DISC_CAS_DONE;
                    p_link->lea_srv_found_flags |= LEA_LINK_DISC_CAS_DONE;
                    if (p_data->cas_inc_csis)
                    {
                        p_link->lea_srv_found_flags |= LEA_LINK_DISC_CSIS_DONE;
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CLIENT_ATTR_CCCD_INFO:
        {
            T_CLIENT_ATTR_CCCD_INFO *p_data = (T_CLIENT_ATTR_CCCD_INFO *)buf;
            APP_PRINT_INFO8("LE_AUDIO_MSG_CLIENT_ATTR_CCCD_INFO: conn_handle 0x%x, srv_cfg %d, srv_instance_id %d, srv_uuid 0x%x, char_instance_id %d, char_uuid 0x%x, cccd_cfg 0x%x, cause 0x%x",
                            p_data->conn_handle,
                            p_data->srv_cfg,
                            p_data->srv_instance_id,
                            p_data->srv_uuid,
                            p_data->char_instance_id,
                            p_data->char_uuid,
                            p_data->cccd_cfg,
                            p_data->cause);
        }
        break;

#if VCP_VOLUME_CONTROLLER
    case LE_AUDIO_MSG_VCS_CLIENT_DIS_DONE:
        {
            T_VCS_CLIENT_DIS_DONE *p_dis_done = (T_VCS_CLIENT_DIS_DONE *)buf;
            APP_PRINT_INFO4("LE_AUDIO_MSG_VCS_CLIENT_DIS_DONE: conn_handle 0x%x, is_found %d, load_from_ftl %d, type_exist 0x%x",
                            p_dis_done->conn_handle, p_dis_done->is_found, p_dis_done->load_from_ftl, p_dis_done->type_exist);
            if (p_dis_done->type_exist & VCS_VOLUME_STATE_FLAG)
            {
                T_VOLUME_STATE volume_state;
                if (vcs_get_volume_state(p_dis_done->conn_handle, &volume_state))
                {
                    APP_PRINT_INFO3("LE_AUDIO_MSG_VCS_CLIENT_DIS_DONE: volume_setting %d, mute %d, change_counter 0x%x",
                                    volume_state.volume_setting,  volume_state.mute,  volume_state.change_counter);
                }
            }

            if (p_dis_done->type_exist & VCS_VOLUME_FLAGS_FLAG)
            {
                uint8_t volume_flags;
                if (vcs_get_volume_flags(p_dis_done->conn_handle, &volume_flags))
                {
                    APP_PRINT_INFO1("LE_AUDIO_MSG_VCS_CLIENT_DIS_DONE: volume_flags 0x%x", volume_flags);
                }
            }
        }
        break;

    case LE_AUDIO_MSG_VCS_CLIENT_CP_RESULT:
        {
            T_VCS_CLIENT_CP_RESULT *p_cp_result = (T_VCS_CLIENT_CP_RESULT *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_VCS_CLIENT_CP_RESULT: conn_handle 0x%x, cause 0x%x, cp_op 0x%x",
                            p_cp_result->conn_handle, p_cp_result->cause, p_cp_result->cp_op);
        }
        break;

    case LE_AUDIO_MSG_VCS_CLIENT_VOLUME_STATE_DATA:
        {
            T_VCS_CLIENT_VOLUME_STATE_DATA *p_data = (T_VCS_CLIENT_VOLUME_STATE_DATA *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_VCS_CLIENT_VOLUME_STATE_DATA: conn_handle 0x%x, is_notify %d, type %d",
                            p_data->conn_handle, p_data->is_notify, p_data->type);
            if (p_data->type == VCS_CHAR_VOLUME_STATE)
            {
                APP_PRINT_INFO3("VCS_CHAR_VOLUME_STATE: volume_setting %d, mute %d, change_counter 0x%x",
                                p_data->data.volume_state.volume_setting,  p_data->data.volume_state.mute,
                                p_data->data.volume_state.change_counter);
            }
            else if (p_data->type == VCS_CHAR_VOLUME_FLAGS)
            {
                APP_PRINT_INFO1("VCS_CHAR_VOLUME_FLAGS: volume_flags 0x%x",
                                p_data->data.volume_flags);
            }
        }
        break;
#endif

    default:
        break;
    }
    return cb_result;
}

void app_lea_ini_cap_init(void)
{
    T_CAP_INIT_PARAMS cap_init_param = {0};

    ble_audio_cback_register(app_lea_ini_cap_handle_msg);
    app_db.cap_role = CAP_INITIATOR_ROLE;
    cap_init_param.cap_role = app_db.cap_role;
    cap_init_param.cas_client = true;
#if CSIP_SET_COORDINATOR
    app_lea_ini_csis_init(&cap_init_param);
#endif
#if VCP_VOLUME_CONTROLLER
    cap_init_param.vcp_micp.vcp_vcs_client = true;
#endif
#if MICP_MIC_CONTROLLER
    cap_init_param.vcp_micp.micp_mic_controller = true;
#endif
#if MCP_MEDIA_CONTROL_SERVER
    app_lea_ini_mcp_init_cap(&cap_init_param);
#endif
#if CCP_CALL_CONTROL_SERVER
    app_lea_ini_ccp_init_cap(&cap_init_param);
#endif
    cap_init(&cap_init_param);
}
#endif
