/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_lea_cap_ini_main.h"
#include "app_lea_cap_ini_bap_bsrc.h"
#include "app_lea_cap_ini_bap.h"
#include "app_lea_cap_ini_mcp.h"
#include "app_lea_cap_ini_audio_data.h"
#include "base_data_generate.h"
#include "metadata_def.h"
#include "trace.h"
#include "pbp_def.h"
#include "stdlib.h"
#include "gatt.h"
#include "data_uart.h"

#if BAP_BROADCAST_SOURCE
#define PUBLIC_BS_ANNOUNCEMENTS_BROADCAST_ID_OFFSET 8
#define PUBLIC_BS_ANNOUNCEMENTS_FEATURES_OFFSET     15
#define BROADCAST_SOURCE_ADV_SID                    0

uint8_t brs_broadcast_code[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5};

void app_lea_bsrc_target_state(void);

static uint8_t media_codec_level2[] =
{
    0x02,
    CODEC_CFG_TYPE_SAMPLING_FREQUENCY,
    SAMPLING_FREQUENCY_CFG_48K,
    0x02,
    CODEC_CFG_TYPE_FRAME_DURATION,
    FRAME_DURATION_CFG_10_MS,
    0x03,
    CODEC_CFG_TYPE_OCTET_PER_CODEC_FRAME,
    0x64, 0x00,
    0x02,
    CODEC_CFG_TYPE_BLOCKS_PER_SDU,
    1
};

static const uint8_t media_codec_level3_bisl[] =
{
    0x05,
    CODEC_CFG_TYPE_AUDIO_CHANNEL_ALLOCATION,
    (uint8_t)AUDIO_LOCATION_FL,
    (uint8_t)((AUDIO_LOCATION_FL >> 8) & 0xFF),
    (uint8_t)((AUDIO_LOCATION_FL >> 16) & 0xFF),
    (uint8_t)((AUDIO_LOCATION_FL >> 24) & 0xFF),
};

static const uint8_t media_codec_level3_bisr[] =
{
    0x05,
    CODEC_CFG_TYPE_AUDIO_CHANNEL_ALLOCATION,
    (uint8_t)AUDIO_LOCATION_FR,
    (uint8_t)((AUDIO_LOCATION_FR >> 8) & 0xFF),
    (uint8_t)((AUDIO_LOCATION_FR >> 16) & 0xFF),
    (uint8_t)((AUDIO_LOCATION_FR >> 24) & 0xFF),

};

uint8_t app_lea_bsrc_gen_pbp_adv_data(uint8_t **p_audio_adv_data)
{
    uint8_t idx = 0;
    uint8_t name_len = strlen(device_name_le);
    uint8_t feature = 0;
    uint8_t *audio_adv_data;
    uint8_t audio_data_len;
    T_BROADCAST_SOURCE_INFO src_info;

    if (broadcast_source_get_info(app_db.bsrc_db.source_handle, &src_info) == false)
    {
        return 0;
    }

    audio_data_len = 23 + name_len;
    audio_adv_data = calloc(1, audio_data_len);
    if (audio_adv_data == NULL)
    {
        return 0;
    }
    audio_adv_data[idx++] = 0x03;
    audio_adv_data[idx++] = GAP_ADTYPE_APPEARANCE;
    audio_adv_data[idx++] = LO_WORD(GAP_GATT_APPEARANCE_GENERIC_MEDIA_PLAYER);
    audio_adv_data[idx++] = HI_WORD(GAP_GATT_APPEARANCE_GENERIC_MEDIA_PLAYER);
    audio_adv_data[idx++] = 0x06;
    audio_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
    audio_adv_data[idx++] = (uint8_t)(BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID);
    audio_adv_data[idx++] = (uint8_t)(BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID >> 8);
    audio_adv_data[idx++] = 0x00; // broadcast id
    audio_adv_data[idx++] = 0x00;
    audio_adv_data[idx++] = 0x00;
    audio_adv_data[idx++] = 0x09;
    audio_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
    audio_adv_data[idx++] = (uint8_t)(PUBIC_BROADCAST_ANNOUNCEMENT_SRV_UUID);
    audio_adv_data[idx++] = (uint8_t)(PUBIC_BROADCAST_ANNOUNCEMENT_SRV_UUID >> 8);
    audio_adv_data[idx++] = 0x00; // Public Broadcast Announcement features
    audio_adv_data[idx++] = 0x04; // Metadata length
    audio_adv_data[idx++] = 0x03; // Metadata
    audio_adv_data[idx++] = METADATA_TYPE_PROGRAM_INFO;
    audio_adv_data[idx++] = 'T';
    audio_adv_data[idx++] = 'V';
    audio_adv_data[idx++] = name_len + 1;
    audio_adv_data[idx++] = GAP_ADTYPE_BROADCAST_NAME;
    memcpy(&audio_adv_data[idx], device_name_le, name_len);
    idx += name_len;

    memcpy(&audio_adv_data[PUBLIC_BS_ANNOUNCEMENTS_BROADCAST_ID_OFFSET],
           src_info.broadcast_id, BROADCAST_ID_LEN);
    if (app_db.bsrc_db.cfg_encryption)
    {
        feature |= PUBIC_BROADCAST_BIT_ENCRYPTED;
    }
    if (app_db.bsrc_db.cfg_codec_index == CODEC_CFG_ITEM_16_2 ||
        app_db.bsrc_db.cfg_codec_index == CODEC_CFG_ITEM_24_2)
    {
        feature |= PUBIC_BROADCAST_BIT_STANDARD_QUALITY_PRESENT;
    }
    if (app_db.bsrc_db.cfg_codec_index >= CODEC_CFG_ITEM_48_1)
    {
        feature |= PUBIC_BROADCAST_BIT_HIGH_QUALITY_PRESENT;
    }
    audio_adv_data[PUBLIC_BS_ANNOUNCEMENTS_FEATURES_OFFSET] = feature;
    *p_audio_adv_data = audio_adv_data;
    return idx;
}

bool app_lea_bsrc_config(void)
{
    uint16_t pa_data_len;
    uint8_t *pa_data = NULL;
    uint32_t primary_adv_interval_min = PRIMARY_ADV_INTERVAL_MIN;
    uint32_t primary_adv_interval_max = PRIMARY_ADV_INTERVAL_MAX;
    uint8_t primary_adv_channel_map = GAP_ADVCHAN_ALL;
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    uint8_t tx_power = 127;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    uint8_t secondary_adv_max_skip = 0;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_1M;
    uint16_t periodic_adv_interval_min = PERIODIC_ADV_INTERVAL_MIN;
    uint16_t periodic_adv_interval_max = PERIODIC_ADV_INTERVAL_MAX;
    uint16_t periodic_adv_prop = 0;
    uint8_t *p_broadcast_audio_announcements = NULL;

    if (app_db.bsrc_db.source_handle != NULL &&
        app_db.bsrc_db.state == BROADCAST_SOURCE_STATE_IDLE)
    {
        uint8_t adv_len;

        adv_len = app_lea_bsrc_gen_pbp_adv_data(&p_broadcast_audio_announcements);
        if (adv_len == 0)
        {
            goto error;
        }

        if (broadcast_source_set_eadv_param(app_db.bsrc_db.source_handle, BROADCAST_SOURCE_ADV_SID,
                                            primary_adv_interval_min, primary_adv_interval_max,
                                            primary_adv_channel_map, app_db.bsrc_db.cfg_local_addr_type, NULL,
                                            filter_policy, tx_power,
                                            primary_adv_phy, secondary_adv_max_skip,
                                            secondary_adv_phy,
                                            adv_len, p_broadcast_audio_announcements) == false)
        {
            goto error;
        }
        if (base_data_gen_pa_data(app_db.bsrc_db.group1_idx, &pa_data_len, &pa_data) == false)
        {
            goto error;
        }
        if (broadcast_source_set_pa_param(app_db.bsrc_db.source_handle, periodic_adv_interval_min,
                                          periodic_adv_interval_max, periodic_adv_prop,
                                          pa_data_len, pa_data) == false)
        {
            goto error;
        }
    }
    else
    {
        goto error;
    }

    if (broadcast_source_config(app_db.bsrc_db.source_handle) == false)
    {
        goto error;
    }
    if (pa_data)
    {
        free(pa_data);
    }
    if (p_broadcast_audio_announcements)
    {
        free(p_broadcast_audio_announcements);
    }
    return true;
error:
    if (pa_data)
    {
        free(pa_data);
    }
    if (p_broadcast_audio_announcements)
    {
        free(p_broadcast_audio_announcements);
    }
    return false;
}

bool app_lea_bsrc_establish(void)
{
    T_BIG_MGR_ISOC_BROADCASTER_CREATE_BIG_PARAM big_param = {0};

    big_param.num_bis = app_db.bsrc_db.group1_bis_num;
    big_param.sdu_interval = app_db.bsrc_db.sdu_interval;
    big_param.max_sdu = app_db.bsrc_db.max_sdu * 2; //Fix me later, check the parameter
    big_param.max_transport_latency = app_db.bsrc_db.prefer_qos.max_transport_latency;
    big_param.rtn = app_db.bsrc_db.prefer_qos.retransmission_number;
    big_param.phy = 2;
    big_param.packing = 0;
    big_param.framing = app_db.bsrc_db.prefer_qos.framing;
    big_param.encryption = app_db.bsrc_db.cfg_encryption;

    if (app_db.bsrc_db.cfg_encryption)
    {
        memcpy(big_param.broadcast_code, brs_broadcast_code, 16);
    }

    if (broadcast_source_establish(app_db.bsrc_db.source_handle, big_param) == false)
    {
        goto error;
    }
    return true;
error:
    return false;
}

bool app_lea_bsrc_start(void)
{
    if (app_db.bsrc_db.source_handle == NULL)
    {
        return false;
    }
    app_db.bsrc_db.prefer_state = BROADCAST_SOURCE_STATE_STREAMING;
    app_lea_bsrc_target_state();
    return true;
}

bool app_lea_bsrc_stop(bool release)
{
    if (app_db.bsrc_db.source_handle == NULL)
    {
        return false;
    }
    if (release)
    {
        app_db.bsrc_db.prefer_state = BROADCAST_SOURCE_STATE_IDLE;
    }
    else
    {
        if (app_db.bsrc_db.prefer_state != BROADCAST_SOURCE_STATE_IDLE)
        {
            app_db.bsrc_db.prefer_state = BROADCAST_SOURCE_STATE_CONFIGURED;
        }
    }
    app_lea_bsrc_target_state();
    return true;
}

static void app_lea_free_basic_data(void)
{
    if (app_db.bsrc_db.group1_idx != 0xFF)
    {
        base_data_del_group(app_db.bsrc_db.group1_idx);
        app_db.bsrc_db.group1_idx = 0xFF;
    }
}

static void app_lea_gen_basic_data(void)
{
    uint8_t error_idx = 0;
    uint8_t codec_id2[CODEC_ID_LEN] = {LC3_CODEC_ID, 0, 0, 0, 0};
    uint8_t media_metadata[] =
    {
        0x03,
        METADATA_TYPE_STREAMING_AUDIO_CONTEXTS,
        (uint8_t)(AUDIO_CONTEXT_MEDIA),
        (uint8_t)(AUDIO_CONTEXT_MEDIA >> 8),
#if MCP_MEDIA_CONTROL_SERVER
        0x02,
        METADATA_TYPE_CCID_LIST,
        APP_LEA_GMCS_CCID
#endif
    };
    app_lea_free_basic_data();

    media_codec_level2[2] = app_db.bsrc_db.codec_cfg.sample_frequency;
    media_codec_level2[5] = app_db.bsrc_db.codec_cfg.frame_duration;
    memcpy(&media_codec_level2[8], &app_db.bsrc_db.codec_cfg.octets_per_codec_frame, 2);
    media_codec_level2[12] = app_db.bsrc_db.codec_cfg.codec_frame_blocks_per_sdu;

    if (base_data_add_group(&app_db.bsrc_db.group1_idx,
                            app_db.bsrc_db.prefer_qos.presentation_delay) == false)
    {
        error_idx = 1;
        app_db.bsrc_db.group1_idx = 0xFF;
        goto error;
    }
    if (base_data_add_subgroup(&app_db.bsrc_db.group1_subgroup1_idx, app_db.bsrc_db.group1_idx,
                               codec_id2,
                               sizeof(media_codec_level2), media_codec_level2, sizeof(media_metadata),
                               (uint8_t *)media_metadata) == false)
    {
        error_idx = 2;
        app_db.bsrc_db.group1_subgroup1_idx = 0xFF;
        goto error;
    }

    if (app_db.bsrc_db.cfg_bis_num == 1)
    {
        if (base_data_add_bis(&app_db.bsrc_db.group1_bis1_idx, app_db.bsrc_db.group1_idx,
                              app_db.bsrc_db.group1_subgroup1_idx,
                              sizeof(media_codec_level3_bisl), (uint8_t *)media_codec_level3_bisl) == false)
        {
            error_idx = 3;
            goto error;
        }
    }
    else if (app_db.bsrc_db.cfg_bis_num == 2)
    {
        if (base_data_add_bis(&app_db.bsrc_db.group1_bis1_idx, app_db.bsrc_db.group1_idx,
                              app_db.bsrc_db.group1_subgroup1_idx,
                              sizeof(media_codec_level3_bisl), (uint8_t *)media_codec_level3_bisl) == false)
        {
            error_idx = 4;
            goto error;
        }

        if (base_data_add_bis(&app_db.bsrc_db.group1_bis2_idx, app_db.bsrc_db.group1_idx,
                              app_db.bsrc_db.group1_subgroup1_idx,
                              sizeof(media_codec_level3_bisr), (uint8_t *)media_codec_level3_bisr) == false)
        {
            error_idx = 5;
            goto error;
        }
    }
    base_data_get_bis_num(app_db.bsrc_db.group1_idx, &app_db.bsrc_db.group1_bis_num);
    return;
error:
    APP_PRINT_ERROR1("app_lea_gen_basic_data: failed, error_idx %d", error_idx);
}

void app_lea_bsrc_target_state(void)
{
    if (app_db.bsrc_db.state ==  app_db.bsrc_db.prefer_state)
    {
        return;
    }

    switch (app_db.bsrc_db.state)
    {
    case BROADCAST_SOURCE_STATE_IDLE:
        {
            if (app_db.bsrc_db.prefer_state == BROADCAST_SOURCE_STATE_CONFIGURED ||
                app_db.bsrc_db.prefer_state == BROADCAST_SOURCE_STATE_STREAMING)
            {
                app_lea_bsrc_config();
            }
        }
        break;
    case BROADCAST_SOURCE_STATE_CONFIGURED:
        {
            if (app_db.bsrc_db.prefer_state == BROADCAST_SOURCE_STATE_STREAMING)
            {
                app_lea_bsrc_establish();
            }
            else if (app_db.bsrc_db.prefer_state == BROADCAST_SOURCE_STATE_IDLE)
            {
                broadcast_source_release(app_db.bsrc_db.source_handle);
            }
        }
        break;

    case BROADCAST_SOURCE_STATE_STREAMING:
        {
            if (app_db.bsrc_db.prefer_state == BROADCAST_SOURCE_STATE_CONFIGURED ||
                app_db.bsrc_db.prefer_state == BROADCAST_SOURCE_STATE_IDLE)
            {
                broadcast_source_disable(app_db.bsrc_db.source_handle, HCI_ERR_LOCAL_HOST_TERMINATE);
            }
        }
        break;

    default:
        break;
    }
}

void app_lea_bsrc_sm_cb(T_BROADCAST_SOURCE_HANDLE handle, uint8_t cb_type, void *p_cb_data)
{
    T_BROADCAST_SOURCE_SM_CB_DATA *p_sm_data = (T_BROADCAST_SOURCE_SM_CB_DATA *)p_cb_data;

    switch (cb_type)
    {
    case MSG_BROADCAST_SOURCE_STATE_CHANGE:
        {
            APP_PRINT_INFO2("MSG_BROADCAST_SOURCE_STATE_CHANGE: state %d, cause 0x%x",
                            p_sm_data->p_state_change->state,
                            p_sm_data->p_state_change->cause);
            data_uart_print("MSG_BROADCAST_SOURCE_STATE_CHANGE: state %d, cause 0x%x\r\n",
                            p_sm_data->p_state_change->state,
                            p_sm_data->p_state_change->cause);
            app_db.bsrc_db.state = p_sm_data->p_state_change->state;
            if (p_sm_data->p_state_change->cause == GAP_SUCCESS)
            {
                app_lea_bsrc_target_state();
            }
            if (p_sm_data->p_state_change->state == BROADCAST_SOURCE_STATE_STREAMING &&
                p_sm_data->p_state_change->cause == GAP_SUCCESS)
            {
                uint8_t codec_id[5] = {LC3_CODEC_ID, 0, 0, 0, 0};
                for (uint8_t i = 0; i < app_db.bsrc_db.group1_bis_num; i++)
                {
                    broadcast_source_setup_data_path(app_db.bsrc_db.source_handle, i + 1,
                                                     codec_id, 0, 0, NULL);
                }
            }
        }
        break;

    case MSG_BROADCAST_SOURCE_METADATA_UPDATE:
        {
            APP_PRINT_INFO1("MSG_BROADCAST_SOURCE_METADATA_UPDATE: cause 0x%x", p_sm_data->cause);
        }
        break;

    case MSG_BROADCAST_SOURCE_RECONFIG:
        {
            APP_PRINT_INFO1("MSG_BROADCAST_SOURCE_RECONFIG: cause 0x%x", p_sm_data->cause);
        }
        break;

    case MSG_BROADCAST_SOURCE_BIG_SYNC_MODE:
        {
            APP_PRINT_INFO2("MSG_BROADCAST_SOURCE_BIG_SYNC_MODE: big_sync_mode %d, cause 0x%x",
                            p_sm_data->p_big_sync_mode->big_sync_mode,
                            p_sm_data->p_big_sync_mode->cause);
        }
        break;

    case MSG_BROADCAST_SOURCE_SETUP_DATA_PATH:
        {
            APP_PRINT_INFO2("MSG_BROADCAST_SOURCE_SETUP_DATA_PATH: bis_idx %d, cause 0x%x",
                            p_sm_data->p_setup_data_path->bis_idx,
                            p_sm_data->p_setup_data_path->cause);
            if (p_sm_data->p_setup_data_path->cause == GAP_SUCCESS)
            {
                T_LEA_SETUP_DATA_PATH data = {0};

                data.iso_mode = BIG_ISO_MODE;
                data.iso_conn_handle = p_sm_data->p_setup_data_path->bis_conn_handle;
                data.path_direction = DATA_PATH_INPUT_FLAG;
                data.presentation_delay = app_db.bsrc_db.prefer_qos.presentation_delay;
                memcpy(&data.codec_parsed_data, &app_db.bsrc_db.codec_cfg, sizeof(T_CODEC_CFG));
                app_lea_handle_data_path_setup(&data);

            }
        }
        break;

    case MSG_BROADCAST_SOURCE_REMOVE_DATA_PATH:
        {
            APP_PRINT_INFO2("MSG_BROADCAST_SOURCE_REMOVE_DATA_PATH: bis_conn_handle %d, cause 0x%x",
                            p_sm_data->p_remove_data_path->bis_conn_handle,
                            p_sm_data->p_remove_data_path->cause);
            T_LEA_REMOVE_DATA_PATH data;
            data.iso_mode = BIG_ISO_MODE;
            data.iso_conn_handle = p_sm_data->p_remove_data_path->bis_conn_handle;
            data.path_direction = DATA_PATH_INPUT_FLAG;
            app_lea_handle_data_path_remove(&data);
        }
        break;
    default:
        break;
    }
}

T_BROADCAST_SOURCE_HANDLE app_lea_bsrc_get_handle(uint8_t *bd_addr, uint8_t bd_type,
                                                  uint8_t adv_sid, uint8_t broadcast_id[3])
{
    if (app_db.bsrc_db.source_handle != NULL &&
        app_db.bsrc_db.source_adv_sid == adv_sid &&
        memcmp(app_db.bsrc_db.broadcast_id, broadcast_id, 3) == 0 &&
        app_db.bsrc_db.cfg_local_addr_type == bd_type &&
        memcmp(app_db.bsrc_db.source_address, bd_addr, 6) == 0
       )
    {
        return app_db.bsrc_db.source_handle;
    }
    return NULL;
}

void app_lea_bsrc_init(T_CODEC_CFG_ITEM index, T_QOS_CFG_TYPE qos_type, uint8_t bis_num,
                       T_GAP_LOCAL_ADDR_TYPE local_addr_type, bool encryption)
{
    T_BROADCAST_SOURCE_INFO src_info;

    if (app_db.bsrc_db.source_handle != NULL)
    {
        return;
    }
    else
    {
        app_db.bsrc_db.source_handle = broadcast_source_add(app_lea_bsrc_sm_cb);
        if (app_db.bsrc_db.source_handle == NULL)
        {
            return;
        }
    }

    app_db.bsrc_db.cfg_codec_index = index;
    app_db.bsrc_db.group1_idx = 0xFF;
    app_db.bsrc_db.group1_subgroup1_idx = 0xFF;
    app_db.bsrc_db.cfg_qos_type = qos_type;
    app_db.bsrc_db.cfg_bis_num = bis_num;
    app_db.bsrc_db.cfg_local_addr_type = local_addr_type;
    app_db.bsrc_db.cfg_encryption = encryption;
    codec_preferred_cfg_get((T_CODEC_CFG_ITEM)index, &app_db.bsrc_db.codec_cfg);
    codec_sdu_interval_get(&app_db.bsrc_db.codec_cfg, &app_db.bsrc_db.sdu_interval);
    codec_max_sdu_len_get(&app_db.bsrc_db.codec_cfg, &app_db.bsrc_db.max_sdu);
    qos_preferred_cfg_get((T_CODEC_CFG_ITEM)index, qos_type, &app_db.bsrc_db.prefer_qos);
    app_lea_gen_basic_data();

    if (broadcast_source_get_info(app_db.bsrc_db.source_handle, &src_info))
    {
        app_db.bsrc_db.source_adv_sid = src_info.adv_sid;
        memcpy(app_db.bsrc_db.broadcast_id, src_info.broadcast_id, BROADCAST_ID_LEN);
        if (app_db.bsrc_db.cfg_local_addr_type == GAP_LOCAL_ADDR_LE_PUBLIC)
        {
            gap_get_param(GAP_PARAM_BD_ADDR, app_db.bsrc_db.source_address);
        }
        else
        {
            //fixme later
        }
#if BAP_BROADCAST_ASSISTANT
        app_lea_link_brs_char_add_source_handle();
#endif
    }
}
#endif
