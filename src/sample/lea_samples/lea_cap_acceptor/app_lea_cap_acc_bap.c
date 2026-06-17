/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "stdlib.h"
#include "ble_audio.h"
#include "bap.h"
#include "trace.h"
#include "data_uart.h"
#include "app_lea_cap_acc_main.h"
#include "app_lea_cap_acc_bap.h"
#include "pacs_mgr.h"
#include "ble_audio.h"
#include "ble_audio_def.h"
#include "metadata_def.h"
#include "ascs_mgr.h"
#include "app_lea_cap_acc_adv.h"
#include "app_lea_cap_acc_scan.h"
#include "app_lea_cap_acc_audio_data.h"
#include "bass_mgr.h"

#define APP_ISOC_CIG_MAX_NUM                      2
#define APP_ISOC_CIS_MAX_NUM                      4
#define APP_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM      2
#define APP_SYNC_RECEIVER_MAX_BIS_NUM             4
#define APP_MAX_SYNC_HANDLE_NUM                   4

const uint8_t app_lea_sink_codec[] =
{
    //Number_of_PAC_records
    1,
    //PAC Record
    LC3_CODEC_ID, 0, 0, 0, 0,//Codec_ID
    //Codec_Specific_Capabilities_Length
    16,
    //Codec_Specific_Capabilities
    0x03,
    CODEC_CAP_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
    (uint8_t)(SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_24K | SAMPLING_FREQUENCY_32K | SAMPLING_FREQUENCY_48K),
    (uint8_t)((SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_24K | SAMPLING_FREQUENCY_32K | SAMPLING_FREQUENCY_48K) >> 8),
    0x02,
    CODEC_CAP_TYPE_SUPPORTED_FRAME_DURATIONS,
    FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT,
    0x02,
    CODEC_CAP_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
    0x05,
    CODEC_CAP_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    0x1E, 0x00, 0x9B, 0x00,
    //Metadata_Length
    0
    //Metadata
};
const uint8_t app_lea_source_codec[] =
{
    //Number_of_PAC_records
    1,
    //PAC Record
    LC3_CODEC_ID, 0, 0, 0, 0,//Codec_ID
    //Codec_Specific_Capabilities_Length
    16,
    //Codec_Specific_Capabilities
    0x03,
    CODEC_CAP_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
    (uint8_t)(SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_24K | SAMPLING_FREQUENCY_32K | SAMPLING_FREQUENCY_48K),
    (uint8_t)((SAMPLING_FREQUENCY_16K | SAMPLING_FREQUENCY_24K | SAMPLING_FREQUENCY_32K | SAMPLING_FREQUENCY_48K) >> 8),
    0x02,
    CODEC_CAP_TYPE_SUPPORTED_FRAME_DURATIONS,
    FRAME_DURATION_10_MS_BIT | FRAME_DURATION_7_5_MS_BIT,
    0x02,
    CODEC_CAP_TYPE_AUDIO_CHANNEL_COUNTS,
    AUDIO_CHANNEL_COUNTS_1 | AUDIO_CHANNEL_COUNTS_2,
    0x05,
    CODEC_CAP_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
    0x1E, 0x00, 0x9B, 0x00,
    //Metadata_Length
    0
    //Metadata
};

#if (BAP_BROADCAST_SINK || BAP_SCAN_DELEGATOR)
void  app_lea_acc_sync_cb(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t cb_type, void *p_cb_data);

T_APP_LEA_SYNC_INFO *app_lea_add_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    T_APP_LEA_SYNC_INFO *p_sync_info;

    p_sync_info = app_lea_find_sync_handle(sync_handle);
    if (p_sync_info)
    {
        return p_sync_info;
    }
    p_sync_info = calloc(1, sizeof(T_APP_LEA_SYNC_INFO));
    if (p_sync_info)
    {
        p_sync_info->sync_handle = sync_handle;
        data_uart_print("Add Sync handle[%d]: sync_handle 0x%x\r\n",
                        app_db.sync_handle_queue.count, p_sync_info->sync_handle);
        os_queue_in(&app_db.sync_handle_queue, p_sync_info);
    }
    return p_sync_info;
}


T_APP_LEA_SYNC_INFO *app_lea_find_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    T_APP_LEA_SYNC_INFO *p_sync_info;
    for (uint8_t i = 0; i < app_db.sync_handle_queue.count; i++)
    {
        p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue, i);
        if (p_sync_info != NULL && p_sync_info->sync_handle == sync_handle)
        {
            return p_sync_info;
        }
    }
    return NULL;
}

T_APP_LEA_SYNC_INFO *app_lea_find_by_source_id(uint8_t source_id)
{
    T_APP_LEA_SYNC_INFO *p_sync_info;
    for (uint8_t i = 0; i < app_db.sync_handle_queue.count; i++)
    {
        p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue, i);
        if (p_sync_info != NULL && p_sync_info->source_id != 0 &&
            p_sync_info->source_id == source_id)
        {
            return p_sync_info;
        }
    }
    return NULL;
}

void app_lea_remove_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    T_APP_LEA_SYNC_INFO *p_sync_info;

    p_sync_info = app_lea_find_sync_handle(sync_handle);
    if (p_sync_info)
    {
        os_queue_delete(&app_db.sync_handle_queue, p_sync_info);
        free(p_sync_info);
    }
}

void app_lea_remove_sync(T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    ble_audio_sync_release(&sync_handle);
}
#endif

#if BAP_BROADCAST_SINK
bool app_lea_big_terminate(T_APP_LEA_SYNC_INFO *p_sync_info)
{
    p_sync_info->big_proc_flags &= ~APP_LEA_BIG_PROC_BIG_SYNC_REQ;
    return ble_audio_big_terminate(p_sync_info->sync_handle);
}

bool app_lea_pa_terminate(T_APP_LEA_SYNC_INFO *p_sync_info)
{
    return ble_audio_pa_terminate(p_sync_info->sync_handle);
}

bool app_lea_pa_sync(T_APP_LEA_SYNC_INFO *p_sync_info)
{
    if (p_sync_info->pa_state == GAP_PA_SYNC_STATE_TERMINATED)
    {
        uint8_t options = 0;
        uint8_t sync_cte_type = 0;
        uint16_t sync_timeout = 1000;
        uint16_t skip = 0;
        if (ble_audio_pa_sync_establish(p_sync_info->sync_handle, options, sync_cte_type, skip,
                                        sync_timeout))
        {
            return true;
        }
    }
    return false;
}

bool app_lea_pa_sync_by_dev_info(T_APP_LEA_BAAS_DEV_INFO *p_dev_info)
{
    T_BLE_AUDIO_SYNC_HANDLE sync_handle;
    T_APP_LEA_SYNC_INFO *p_sync_info = NULL;
    uint8_t options = 0;
    uint8_t sync_cte_type = 0;
    uint16_t sync_timeout = 1000;
    uint16_t skip = 0;

    sync_handle = ble_audio_sync_find(p_dev_info->adv_sid,
                                      p_dev_info->broadcast_id);
    if (sync_handle == NULL)
    {
        sync_handle = ble_audio_sync_allocate(app_lea_acc_sync_cb, p_dev_info->bd_type,
                                              p_dev_info->bd_addr, p_dev_info->adv_sid, p_dev_info->broadcast_id);
        if (sync_handle)
        {
            p_sync_info = app_lea_add_sync_handle(sync_handle);
        }
    }
    else
    {
        p_sync_info = app_lea_find_sync_handle(sync_handle);
    }
    if (p_sync_info && p_sync_info->pa_state == GAP_PA_SYNC_STATE_TERMINATED)
    {
        if (ble_audio_pa_sync_establish(sync_handle, options, sync_cte_type, skip, sync_timeout))
        {
            return true;
        }
    }
    return false;
}

bool app_lea_big_select_bis(T_APP_LEA_SYNC_INFO *p_sync_info)
{
    p_sync_info->bis_num = 0;
    if (p_sync_info->supported_bis_array != 0)
    {
        uint32_t supported_bis_array = p_sync_info->supported_bis_array;
        uint8_t idx = 0;

        while (supported_bis_array)
        {
            if (supported_bis_array & 0x01)
            {
                p_sync_info->bis_idx[p_sync_info->bis_num] = (idx + 1);
                p_sync_info->bis_num++;
                if (p_sync_info->bis_num == APP_BIG_SYNC_MAX_BIS_NUM)
                {
                    break;
                }
            }
            supported_bis_array >>= 1;
            idx++;
        }
        if (p_sync_info->bis_num)
        {
            return true;
        }
    }
    return false;
}

void app_lea_big_check_sync(T_APP_LEA_SYNC_INFO *p_sync_info)
{
    APP_PRINT_INFO3("app_lea_big_check_sync: big_proc_flags 0x%x, pa_state %d, big_state %d",
                    p_sync_info->big_proc_flags, p_sync_info->pa_state,
                    p_sync_info->big_state);
    if (p_sync_info->big_proc_flags & APP_LEA_BIG_PROC_BIG_SYNC_REQ)
    {
        if (p_sync_info->big_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED)
        {
            if (p_sync_info->pa_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
            {
                if (p_sync_info->big_proc_flags & APP_LEA_BIG_PROC_BIG_INFO_RECEIVED)
                {
                    T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM sync_param = {0};

                    sync_param.encryption = p_sync_info->encryption;
                    if (p_sync_info->encryption)
                    {
                        if (p_sync_info->big_proc_flags & APP_LEA_BIG_PROC_BROADCAST_CODE_EXIST)
                        {
                            memcpy(sync_param.broadcast_code, p_sync_info->broadcast_code, BROADCAST_CODE_LEN);
                        }
                        else
                        {
#if BAP_SCAN_DELEGATOR
                            bass_send_broadcast_code_required(p_sync_info->source_id);
#endif
                            return;
                        }
                    }
                    sync_param.mse = 0;
                    sync_param.big_sync_timeout = 100;

                    if (app_lea_big_select_bis(p_sync_info))
                    {
                        sync_param.num_bis = p_sync_info->bis_num;
                        memcpy(sync_param.bis, p_sync_info->bis_idx, sync_param.num_bis);
                        if (ble_audio_big_sync_establish(p_sync_info->sync_handle, &sync_param) == false)
                        {
                            APP_PRINT_ERROR1("app_lea_big_check_sync:ble_audio_big_sync_establish failed, sync_handle %p",
                                             p_sync_info->sync_handle);
                        }
                    }
                    return;
                }
            }
            else if (p_sync_info->pa_state == GAP_PA_SYNC_STATE_TERMINATED)
            {
                app_lea_pa_sync(p_sync_info);
                return;
            }
        }
        else if (p_sync_info->big_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
        {
            p_sync_info->big_proc_flags &= ~APP_LEA_BIG_PROC_BIG_SYNC_REQ;
        }
    }
}

void app_lea_big_sync(T_APP_LEA_SYNC_INFO *p_sync_info)
{
    p_sync_info->big_proc_flags |= APP_LEA_BIG_PROC_BIG_SYNC_REQ;

    app_lea_big_check_sync(p_sync_info);
}

void app_lea_acc_bass_get_supported_bis(T_APP_LEA_SYNC_INFO *p_sync_info)
{
    uint32_t supported_bis_array = 0;
    T_BLE_AUDIO_SYNC_INFO sync_info;

    if (ble_audio_sync_get_info(p_sync_info->sync_handle, &sync_info))
    {
        if (sync_info.p_base_mapping && sync_info.p_base_mapping->p_subgroup != NULL)
        {
            T_BASE_DATA_SUBGROUP_PARAM *p_subgroup;
            T_BASE_DATA_BIS_PARAM *p_bis;
            uint8_t i = 0;
            uint8_t j = 0;

            for (i = 0; i < sync_info.p_base_mapping->num_subgroups; i++)
            {
                p_subgroup = &sync_info.p_base_mapping->p_subgroup[i];
#if 0
                if (p_subgroup->metadata_len != 0)
                {
                    //Application can add metadata checks here.
                }
#endif
                for (j = 0; j < p_subgroup->num_bis; j++)
                {
                    p_bis = &p_subgroup->p_bis_param[j];
                    if (pacs_check_codec_cfg(SERVER_AUDIO_SINK, p_bis->codec_id, &p_bis->bis_codec_cfg))
                    {
                        if (p_bis->bis_codec_cfg.audio_channel_allocation == 0 ||
                            (app_db.lea_sink_audio_location & p_bis->bis_codec_cfg.audio_channel_allocation))
                        {
                            supported_bis_array |= (1 << (sync_info.p_base_mapping->p_subgroup[i].p_bis_param[j].bis_index -
                                                          1));
                        }
                    }
                }
            }

        }
    }
    APP_PRINT_INFO1("app_lea_acc_bass_get_supported_bis: supported_bis_array 0x%x",
                    supported_bis_array);
    p_sync_info->supported_bis_array = supported_bis_array;
}
#endif

#if BAP_SCAN_DELEGATOR
void app_lea_acc_bass_set_prefer_bis(T_BASS_SET_PREFER_BIS_SYNC *p_data)
{
    T_APP_LEA_SYNC_INFO *p_sync_info;
    T_BLE_AUDIO_SYNC_INFO sync_info;

    p_sync_info = app_lea_find_sync_handle(p_data->handle);
    if (p_sync_info == NULL)
    {
        return;
    }

    if (ble_audio_sync_get_info(p_data->handle, &sync_info))
    {
        if (sync_info.p_base_mapping)
        {
            for (uint8_t i = 0; i < p_data->num_subgroups; i++)
            {
                APP_PRINT_INFO4("bis_data[%d]: bis_sync 0x%x, metadata[%d] %b", i,
                                p_data->p_cp_bis_info[i].bis_sync,
                                p_data->p_cp_bis_info[i].metadata_len,
                                TRACE_BINARY(p_data->p_cp_bis_info[i].metadata_len, p_data->p_cp_bis_info[i].p_metadata));
                if (p_data->p_cp_bis_info[i].bis_sync == BASS_CP_BIS_SYNC_NO_PREFER)
                {
                    uint32_t bis_array = 0;
                    if (i < sync_info.p_base_mapping->num_subgroups)
                    {
                        bis_array = (p_sync_info->supported_bis_array & sync_info.p_base_mapping->p_subgroup[i].bis_array);

                        if (bis_array != 0)
                        {
                            APP_PRINT_INFO2("BASE Mapping: bis_array 0x%x, select bis_array 0x%x",
                                            sync_info.p_base_mapping->p_subgroup[i].bis_array, bis_array);
                            bass_cfg_prefer_bis_sync(p_data->source_id, i, bis_array);
                        }
                    }
                }
            }
        }
    }
}
#endif

#if (BAP_BROADCAST_SINK || BAP_SCAN_DELEGATOR)
static void  app_lea_acc_sync_cb(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t cb_type, void *p_cb_data)
{
    T_BLE_AUDIO_SYNC_CB_DATA *p_sync_cb = (T_BLE_AUDIO_SYNC_CB_DATA *)p_cb_data;
    T_APP_LEA_SYNC_INFO *p_sync_info = app_lea_find_sync_handle(handle);
    switch (cb_type)
    {
    case MSG_BLE_AUDIO_SYNC_HANDLE_RELEASED:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_SYNC_HANDLE_RELEASED: action_role %d",
                             p_sync_cb->p_sync_handle_released->action_role);
            if (p_sync_info)
            {
                app_lea_remove_sync_handle(handle);
            }
        }
        break;

    case MSG_BLE_AUDIO_ADDR_UPDATE:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_ADDR_UPDATE: advertiser_address %s",
                             TRACE_BDADDR(p_sync_cb->p_addr_update->advertiser_address));
        }
        break;

    case MSG_BLE_AUDIO_PA_SYNC_STATE:
        {
            APP_PRINT_TRACE3("MSG_BLE_AUDIO_PA_SYNC_STATE: sync_state %d, action %d, cause 0x%x",
                             p_sync_cb->p_pa_sync_state->sync_state,
                             p_sync_cb->p_pa_sync_state->action,
                             p_sync_cb->p_pa_sync_state->cause);
            if (p_sync_info)
            {
                p_sync_info->pa_state = p_sync_cb->p_pa_sync_state->sync_state;
                if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)
                {
                    p_sync_info->big_proc_flags &= ~APP_LEA_BIG_PROC_BIG_INFO_RECEIVED;
                    p_sync_info->supported_bis_array = 0;
                }
                app_lea_big_check_sync(p_sync_info);

            }

            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
            {
                data_uart_print("PA SYNCHRONIZED\r\n");
            }
            else if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)
            {
                data_uart_print("PA TERMINATED\r\n");
            }
            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_WAIT_SCANNING)
            {
                app_lea_baas_scan_start();
            }
            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED ||
                p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)
            {
                app_lea_baas_scan_stop();
            }
        }
        break;

    case MSG_BLE_AUDIO_PA_REPORT_INFO:
        {
#if 0
            if (p_sync_info)
            {
                if (p_sync_info->big_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED &&
                    p_sync_info->pa_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
                {
                    //APP can terminate PA to save power when BIG state is BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED.
                    ble_audio_pa_terminate(p_sync_info->sync_handle);
                }
            }
#endif
        }
        break;

    case MSG_BLE_AUDIO_BASE_DATA_MODIFY_INFO:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_BASE_DATA_MODIFY_INFO: p_base_mapping %p\r\n",
                             p_sync_cb->p_base_data_modify_info->p_base_mapping);
            if (p_sync_cb->p_base_data_modify_info->p_base_mapping)
            {
                T_BASE_DATA_MAPPING *p_mapping = p_sync_cb->p_base_data_modify_info->p_base_mapping;
                T_BASE_DATA_SUBGROUP_PARAM *p_subgroup;
                T_BASE_DATA_BIS_PARAM *p_bis;
                uint8_t i = 0;
                uint8_t j = 0;

                if (p_mapping == NULL || p_mapping->p_subgroup == NULL)
                {
                    break;
                }
                APP_PRINT_INFO3("Level 1 Group: num_subgroups %d, num_bis %d, presentation_delay 0x%x",
                                p_mapping->num_subgroups, p_mapping->num_bis, p_mapping->presentation_delay);

                for (i = 0; i < p_mapping->num_subgroups; i++)
                {
                    p_subgroup = &p_mapping->p_subgroup[i];
                    APP_PRINT_INFO3("Level 2 Subgroup[%d]: num_bis %d, bis_array 0x%x", p_subgroup->subgroup_idx,
                                    p_subgroup->num_bis, p_subgroup->bis_array);
                    if (p_subgroup->metadata_len != 0)
                    {
                        APP_PRINT_INFO3("Level 2 Subgroup[%d]: metadata[%d] %b",
                                        p_subgroup->subgroup_idx,
                                        p_subgroup->metadata_len, TRACE_BINARY(p_subgroup->metadata_len, p_subgroup->p_metadata));
                    }
                    if (p_subgroup->p_bis_param == NULL)
                    {
                        continue;
                    }
                    for (j = 0; j < p_subgroup->num_bis; j++)
                    {
                        p_bis = &p_subgroup->p_bis_param[j];
                        APP_PRINT_INFO3("Level 3 BIS[%d]: bis_index %d, code id: %b",
                                        p_bis->subgroup_idx, p_bis->bis_index,
                                        TRACE_BINARY(CODEC_ID_LEN, p_bis->codec_id));
                        APP_PRINT_INFO3("Level 3 bis codec, frame_duration: 0x%x, sample_frequency: 0x%x , codec_frame_blocks_per_sdu: 0x%x",
                                        p_bis->bis_codec_cfg.frame_duration, p_bis->bis_codec_cfg.sample_frequency,
                                        p_bis->bis_codec_cfg.codec_frame_blocks_per_sdu);
                        APP_PRINT_INFO2("Level 3 bis codec, octets_per_codec_frame: 0x%x, audio_channel_allocation: 0x%x",
                                        p_bis->bis_codec_cfg.octets_per_codec_frame, p_bis->bis_codec_cfg.audio_channel_allocation);
                    }
                }
                if (p_sync_info)
                {
                    app_lea_acc_bass_get_supported_bis(p_sync_info);
                }
            }
        }
        break;

    case MSG_BLE_AUDIO_PA_BIGINFO:
        {
            if (p_sync_info)
            {
                if ((p_sync_info->big_proc_flags & APP_LEA_BIG_PROC_BIG_INFO_RECEIVED) == 0)
                {
                    p_sync_info->big_proc_flags |= APP_LEA_BIG_PROC_BIG_INFO_RECEIVED;
                    p_sync_info->encryption = p_sync_cb->p_le_biginfo_adv_report_info->encryption;
                    app_lea_big_check_sync(p_sync_info);
                }
            }
        }
        break;

    case MSG_BLE_AUDIO_BIG_SYNC_STATE:
        {
            APP_PRINT_INFO4("MSG_BLE_AUDIO_BIG_SYNC_STATE: sync_state %d, action %d, action role %d, cause 0x%x\r\n",
                            p_sync_cb->p_big_sync_state->sync_state,
                            p_sync_cb->p_big_sync_state->action,
                            p_sync_cb->p_big_sync_state->action_role,
                            p_sync_cb->p_big_sync_state->cause);
            if (p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
            {
                if (p_sync_cb->p_big_sync_state->action == BLE_AUDIO_BIG_SYNC)
                {
                    T_BLE_AUDIO_BIS_INFO bis_sync_info;
                    bool result = ble_audio_get_bis_sync_info(handle, &bis_sync_info);
                    uint8_t codec_id[5] = {1, 0, 0, 0, 0};
                    uint32_t controller_delay = 0x1122;
                    codec_id[0] = LC3_CODEC_ID;

                    data_uart_print("BIG SYNCHRONIZED\r\n");

                    if (result == false)
                    {
                        // goto failed;
                    }
                    for (uint8_t i = 0; i < bis_sync_info.bis_num; i++)
                    {
                        ble_audio_bis_setup_data_path(handle, bis_sync_info.bis_info[i].bis_idx, codec_id,
                                                      controller_delay, 0, NULL);
                    }
                }
            }
            else if (p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED)
            {
                data_uart_print("BIG TERMINATED\r\n");
            }
            if (p_sync_info)
            {
                p_sync_info->big_state = p_sync_cb->p_big_sync_state->sync_state;
                app_lea_big_check_sync(p_sync_info);
            }
        }
        break;

    case MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH:
        {
            APP_PRINT_INFO2("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH: bis_idx %d, cause 0x%x\r\n",
                            p_sync_cb->p_setup_data_path->bis_idx,
                            p_sync_cb->p_setup_data_path->cause);
            if (p_sync_cb->p_setup_data_path->cause == GAP_SUCCESS)
            {
                T_LEA_SETUP_DATA_PATH data = {0};
                T_BLE_AUDIO_SYNC_INFO sync_info;

                if (ble_audio_sync_get_info(handle, &sync_info) == false)
                {
                    break;
                }
                data.iso_mode = BIG_ISO_MODE;
                data.iso_conn_handle = p_sync_cb->p_setup_data_path->bis_conn_handle;
                data.path_direction = DATA_PATH_OUTPUT_FLAG;
                data.presentation_delay = 40000;
                if (!base_data_get_bis_codec_cfg(sync_info.p_base_mapping, p_sync_cb->p_setup_data_path->bis_idx,
                                                 &data.codec_parsed_data))
                {
                    APP_PRINT_ERROR1("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH: can't find bis_idx %d",
                                     p_sync_cb->p_setup_data_path->bis_idx);
                    break;
                }
                if (sync_info.p_base_mapping)
                {
                    data.presentation_delay = sync_info.p_base_mapping->presentation_delay;
                }
                app_lea_handle_data_path_setup(&data);
            }
        }
        break;

    case MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH:
        {
            APP_PRINT_INFO2("MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH: bis_idx %d, cause 0x%x",
                            p_sync_cb->p_remove_data_path->bis_idx,
                            p_sync_cb->p_remove_data_path->cause);
            if (p_sync_cb->p_remove_data_path->cause == GAP_SUCCESS)
            {
                T_LEA_REMOVE_DATA_PATH data;
                data.iso_mode = BIG_ISO_MODE;
                data.iso_conn_handle = p_sync_cb->p_remove_data_path->bis_conn_handle;
                data.path_direction = DATA_PATH_OUTPUT_FLAG;
                app_lea_handle_data_path_remove(&data);
            }
        }
        break;

    default:
        break;
    }
    return;
}
#endif

#if (BAP_UNICAST_SERVER || BAP_BROADCAST_SINK)
void app_lea_acc_bap_update_available_contexts(uint16_t lea_sink_available_contexts,
                                               uint16_t lea_source_available_contexts)
{
    if (lea_sink_available_contexts != app_db.lea_sink_available_contexts ||
        lea_source_available_contexts != app_db.lea_source_available_contexts)
    {
        if ((lea_sink_available_contexts | app_db.lea_sink_supported_contexts) !=
            app_db.lea_sink_supported_contexts)
        {
            APP_PRINT_ERROR2("app_lea_acc_bap_update_available_contexts: not support, lea_sink_available_contexts 0x%x, lea_sink_supported_contexts 0x%x",
                             lea_sink_available_contexts, app_db.lea_sink_supported_contexts);
            return;
        }

        if ((lea_source_available_contexts | app_db.lea_source_supported_contexts) !=
            app_db.lea_source_supported_contexts)
        {
            APP_PRINT_ERROR2("app_lea_acc_bap_update_available_contexts: not support, lea_source_available_contexts 0x%x, lea_source_supported_contexts 0x%x",
                             lea_source_available_contexts, app_db.lea_source_supported_contexts);
            return;
        }

        app_db.lea_sink_available_contexts = lea_sink_available_contexts;
        app_db.lea_source_available_contexts = lea_source_available_contexts;
        APP_PRINT_INFO2("app_lea_acc_bap_update_available_contexts: lea_sink_available_contexts 0x%x, lea_source_available_contexts 0x%x",
                        app_db.lea_sink_available_contexts,
                        app_db.lea_source_available_contexts);
        for (uint8_t i = 0; i < APP_MAX_BLE_LINK_NUM; i++)
        {
            if (app_db.le_link[i].used == true)
            {
                pacs_update_available_contexts(app_db.le_link[i].conn_handle, app_db.lea_sink_available_contexts,
                                               app_db.lea_source_available_contexts);
            }
        }
        app_ext_adv_update();
    }
}
#endif

#if BAP_UNICAST_SERVER
T_APP_LEA_ASE *app_lea_acc_bap_find_ase(uint16_t conn_handle, uint8_t ase_id)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link)
    {
        for (uint8_t i = 0; i < APP_LEA_ASE_NUM; i++)
        {
            if (p_link->lea_ase_tbl[i].used == true &&
                p_link->lea_ase_tbl[i].ase_id == ase_id)
            {
                return &p_link->lea_ase_tbl[i];
            }
        }
    }

    return NULL;
}

T_APP_LEA_ASE *app_lea_acc_bap_add_ase(uint16_t conn_handle, uint8_t ase_id)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link)
    {
        for (uint8_t i = 0; i < APP_LEA_ASE_NUM; i++)
        {
            if (p_link->lea_ase_tbl[i].used == false)
            {
                p_link->lea_ase_tbl[i].used = true;
                p_link->lea_ase_tbl[i].ase_id = ase_id;
                return &p_link->lea_ase_tbl[i];
            }
        }
    }
    return NULL;
}

void app_lea_acc_bap_update_metadata(T_APP_LEA_ASE *p_lea_ase, uint8_t metadata_length,
                                     uint8_t *p_metadata, bool *p_ccid_update, bool *p_contexts_update)
{
    uint16_t audio_contexts = 0;
    uint8_t  ccid_num = 0;
    uint8_t  ccid[APP_LEA_CCID_MAX_NUM];
    bool     ccid_update = false;

    *p_ccid_update  = false;
    *p_contexts_update = false;

    if (metadata_length != 0 && p_metadata != NULL)
    {
        uint16_t idx = 0;
        uint16_t length;
        uint8_t type;

        for (; idx < metadata_length;)
        {
            length = p_metadata[idx];
            idx++;
            type = p_metadata[idx];

            switch (type)
            {
            case METADATA_TYPE_STREAMING_AUDIO_CONTEXTS:
                {
                    if (length == 3)
                    {
                        LE_ARRAY_TO_UINT16(audio_contexts, &p_metadata[idx + 1]);
                    }
                }
                break;

            case METADATA_TYPE_CCID_LIST:
                {
                    if (length > 1)
                    {
                        ccid_num = (length - 1);
                        if (ccid_num > APP_LEA_CCID_MAX_NUM)
                        {
                            ccid_num = APP_LEA_CCID_MAX_NUM;
                        }
                        memcpy(ccid, &p_metadata[idx + 1], ccid_num);
                    }
                }
                break;

            default:
                break;
            }
            idx += length;
        }
    }

    if (p_lea_ase->audio_contexts != audio_contexts)
    {
        APP_PRINT_INFO1("app_lea_acc_bap_update_metadata: update audio_contexts 0x%x",
                        audio_contexts);
        *p_contexts_update = true;
        p_lea_ase->audio_contexts = audio_contexts;

    }

    if (p_lea_ase->ccid_num != ccid_num)
    {
        ccid_update = true;
    }
    else if (p_lea_ase->ccid_num == ccid_num)
    {
        if (ccid_num != 0 &&
            memcmp(p_lea_ase->ccid, ccid, ccid_num) != 0)
        {
            ccid_update = true;
        }
    }

    if (ccid_update)
    {
        APP_PRINT_INFO1("app_lea_acc_bap_update_metadata: ccid_update, ccid_num %d",
                        ccid_num);
        p_lea_ase->ccid_num = ccid_num;
        if (p_lea_ase->ccid_num)
        {
            memcpy(p_lea_ase->ccid, ccid, p_lea_ase->ccid_num);
            APP_PRINT_INFO1("app_lea_acc_bap_update_metadata: ccid_update, ccid %b",
                            TRACE_BINARY(p_lea_ase->ccid_num, p_lea_ase->ccid));
        }
    }
    *p_ccid_update = ccid_update;
}

void app_lea_acc_bap_update_ase_state(T_ASCS_ASE_STATE *p_data)
{
    bool     ccid_update = false;
    bool     contexts_update = false;
    T_APP_LEA_ASE *p_lea_ase = app_lea_acc_bap_find_ase(p_data->conn_handle, p_data->ase_data.ase_id);
    if (p_lea_ase == NULL)
    {
        p_lea_ase = app_lea_acc_bap_add_ase(p_data->conn_handle, p_data->ase_data.ase_id);
        if (p_lea_ase == NULL)
        {
            return;
        }
        p_lea_ase->direction = p_data->ase_data.direction;
    }
    p_lea_ase->ase_state = p_data->ase_data.ase_state;

    if (p_data->ase_data.ase_state == ASE_STATE_QOS_CONFIGURED)
    {
        p_lea_ase->cig_id = p_data->ase_data.param.qos_configured.cig_id;
    }

    if (p_data->ase_data.ase_state == ASE_STATE_IDLE ||
        p_data->ase_data.ase_state == ASE_STATE_CODEC_CONFIGURED ||
        p_data->ase_data.ase_state == ASE_STATE_QOS_CONFIGURED)
    {
        p_lea_ase->ccid_num = 0;
        p_lea_ase->audio_contexts = 0;
        app_lea_acc_bap_update_metadata(p_lea_ase, 0, NULL, &ccid_update, &contexts_update);
    }
    else if (p_data->ase_data.ase_state == ASE_STATE_STREAMING)
    {
        app_lea_acc_bap_update_metadata(p_lea_ase, p_data->ase_data.param.streaming.metadata_length,
                                        p_data->ase_data.param.streaming.p_metadata,
                                        &ccid_update, &contexts_update);
    }
}

void app_lea_acc_handle_data_path_setup(T_ASCS_SETUP_DATA_PATH *p_data)
{
    T_APP_LEA_ASE *p_lea_ase = NULL;
    T_LEA_SETUP_DATA_PATH data = {0};
    T_ASE_QOS_CFG_STATE_DATA cfg_data;

    data.iso_mode = CIG_ISO_MODE;
    data.iso_conn_handle = p_data->cis_conn_handle;
    data.codec_parsed_data = p_data->codec_parsed_data;
    data.path_direction = p_data->path_direction;
    data.presentation_delay = 40000;
    if (ascs_get_qos_cfg(p_data->conn_handle, p_data->ase_id, &cfg_data))
    {
        data.presentation_delay = cfg_data.presentation_delay[0] | cfg_data.presentation_delay[1] << 8 |
                                  cfg_data.presentation_delay[2] << 16;
    }
    app_lea_handle_data_path_setup(&data);
    p_lea_ase = app_lea_acc_bap_find_ase(p_data->conn_handle, p_data->ase_id);
    if (p_lea_ase)
    {
        p_lea_ase->iso_conn_handle = p_data->cis_conn_handle;
    }
}

void app_lea_acc_handle_data_path_remove(T_ASCS_REMOVE_DATA_PATH *p_data)
{
    T_APP_LEA_ASE *p_lea_ase = NULL;
    T_LEA_REMOVE_DATA_PATH data;

    data.iso_mode = CIG_ISO_MODE;
    data.iso_conn_handle = p_data->cis_conn_handle;
    data.path_direction = p_data->path_direction;
    app_lea_handle_data_path_remove(&data);
    p_lea_ase = app_lea_acc_bap_find_ase(p_data->conn_handle, p_data->ase_id);
    if (p_lea_ase)
    {
        p_lea_ase->iso_conn_handle = NULL;
    }
}
#endif

uint16_t app_lea_acc_bap_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO:
        {
            T_SERVER_ATTR_CCCD_INFO *p_cccd = (T_SERVER_ATTR_CCCD_INFO *)buf;
            APP_PRINT_INFO6("LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO: conn_handle 0x%x, service_id %d, char_uuid 0x%x, char_attrib_index %d, ccc_bits 0x%x, param %d",
                            p_cccd->conn_handle,
                            p_cccd->service_id,
                            p_cccd->char_uuid,
                            p_cccd->char_attrib_index,
                            p_cccd->ccc_bits,
                            p_cccd->param);
        }
        break;
#if (BAP_UNICAST_SERVER || BAP_BROADCAST_SINK)
    case LE_AUDIO_MSG_PACS_READ_AVAILABLE_CONTEXTS_IND:
        {
            T_PACS_READ_AVAILABLE_CONTEXTS_IND *p_data = (T_PACS_READ_AVAILABLE_CONTEXTS_IND *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_PACS_READ_AVAILABLE_CONTEXTS_IND: conn_handle 0x%x, cid %d",
                            p_data->conn_handle, p_data->cid);
            pacs_available_contexts_read_cfm(p_data->conn_handle, p_data->cid,
                                             app_db.lea_sink_available_contexts,
                                             app_db.lea_source_available_contexts);
        }
        break;
#endif
#if BAP_UNICAST_SERVER
    case LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH:
        {
            T_ASCS_SETUP_DATA_PATH *p_data = (T_ASCS_SETUP_DATA_PATH *)buf;

            APP_PRINT_INFO4("LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH: conn_handle 0x%x, ase_id %d, path_direction 0x%x, cis_conn_handle 0x%x",
                            p_data->conn_handle,
                            p_data->ase_id,
                            p_data->path_direction,
                            p_data->cis_conn_handle);
            app_lea_acc_handle_data_path_setup(p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_REMOVE_DATA_PATH:
        {
            T_ASCS_REMOVE_DATA_PATH *p_data = (T_ASCS_REMOVE_DATA_PATH *)buf;

            APP_PRINT_INFO4("LE_AUDIO_MSG_ASCS_REMOVE_DATA_PATH: conn_handle 0x%x, ase_id %d, path_direction 0x%x, cis_conn_handle 0x%x",
                            p_data->conn_handle,
                            p_data->ase_id,
                            p_data->path_direction,
                            p_data->cis_conn_handle);
            app_lea_acc_handle_data_path_remove(p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_ASE_STATE:
        {
            T_ASCS_ASE_STATE *p_data = (T_ASCS_ASE_STATE *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_ASCS_ASE_STATE: conn_handle 0x%x, ase_id %d, ase_state %d",
                            p_data->conn_handle,
                            p_data->ase_data.ase_id,
                            p_data->ase_data.ase_state);

            switch (p_data->ase_data.ase_state)
            {
            case ASE_STATE_IDLE:
                APP_PRINT_INFO0("ASE_STATE_IDLE");
                break;

            case ASE_STATE_RELEASING:
                APP_PRINT_INFO0("ASE_STATE_RELEASING");
                break;

            case ASE_STATE_CODEC_CONFIGURED:
                {
                    uint32_t presentation_delay_min;
                    uint32_t presentation_delay_max;
                    uint16_t max_transport_latency;
                    LE_ARRAY_TO_UINT24(presentation_delay_min,
                                       p_data->ase_data.param.codec_configured.data.presentation_delay_min);
                    LE_ARRAY_TO_UINT24(presentation_delay_max,
                                       p_data->ase_data.param.codec_configured.data.presentation_delay_max);
                    LE_ARRAY_TO_UINT16(max_transport_latency,
                                       p_data->ase_data.param.codec_configured.data.max_transport_latency);

                    APP_PRINT_INFO5("[BAP][ASCS] ASE_STATE_CODEC_CONFIGURED: direction 0x%01x, supported_framing 0x%01x, preferred_phy 0x%01x, preferred_retrans_number 0x%01x",
                                    p_data->ase_data.direction,
                                    p_data->ase_data.param.codec_configured.data.supported_framing,
                                    p_data->ase_data.param.codec_configured.data.preferred_phy,
                                    p_data->ase_data.param.codec_configured.data.preferred_retrans_number,
                                    p_data->ase_data.param.codec_configured.data.preferred_retrans_number);
                    APP_PRINT_INFO3("[BAP][ASCS] ASE_STATE_CODEC_CONFIGURED: max_transport_latency 0x%02x, presentation_delay_min 0x%03x, presentation_delay_max 0x%03x",
                                    max_transport_latency,
                                    presentation_delay_min, presentation_delay_max);
                    APP_PRINT_INFO2("[BAP][ASCS] ASE_STATE_CODEC_CONFIGURED: codec_id %b, codec_spec_cfg_len %d",
                                    TRACE_BINARY(5, p_data->ase_data.param.codec_configured.data.codec_id),
                                    p_data->ase_data.param.codec_configured.data.codec_spec_cfg_len);
                    if (p_data->ase_data.param.codec_configured.data.codec_spec_cfg_len)
                    {
                        APP_PRINT_INFO1("[BAP][ASCS] codec_spec_cfg %b",
                                        TRACE_BINARY(p_data->ase_data.param.codec_configured.data.codec_spec_cfg_len,
                                                     p_data->ase_data.param.codec_configured.p_codec_spec_cfg));
                    }
                }
                break;

            case ASE_STATE_QOS_CONFIGURED:
                {
                    uint16_t max_sdu_size;
                    uint16_t max_transport_latency;
                    uint32_t sdu_interval;
                    uint32_t presentation_delay;
                    LE_ARRAY_TO_UINT24(sdu_interval, p_data->ase_data.param.qos_configured.sdu_interval);
                    LE_ARRAY_TO_UINT24(presentation_delay, p_data->ase_data.param.qos_configured.presentation_delay);
                    LE_ARRAY_TO_UINT16(max_sdu_size, p_data->ase_data.param.qos_configured.max_sdu);
                    LE_ARRAY_TO_UINT16(max_transport_latency,
                                       p_data->ase_data.param.qos_configured.max_transport_latency);
                    APP_PRINT_INFO5("[BAP][ASCS] ASE_STATE_QOS_CONFIGURED: cig_id %d, cis_id %d, sdu_interval 0x%03x, framing 0x%01x, phy 0x%x",
                                    p_data->ase_data.param.qos_configured.cig_id,
                                    p_data->ase_data.param.qos_configured.cis_id, sdu_interval,
                                    p_data->ase_data.param.qos_configured.framing,
                                    p_data->ase_data.param.qos_configured.phy);
                    APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_QOS_CONFIGURED: max_sdu 0x%02x, retransmission_number %d, max_transport_latency 0x%02x, presentation_delay 0x%03x",
                                    max_sdu_size, p_data->ase_data.param.qos_configured.retransmission_number,
                                    max_transport_latency, presentation_delay);
                }
                break;

            case ASE_STATE_ENABLING:
                APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_ENABLING: cig_id %d, cis_id %d, metadata[%d] %b",
                                p_data->ase_data.param.enabling.cig_id,
                                p_data->ase_data.param.enabling.cis_id,
                                p_data->ase_data.param.enabling.metadata_length,
                                TRACE_BINARY(p_data->ase_data.param.enabling.metadata_length,
                                             p_data->ase_data.param.enabling.p_metadata));
                break;

            case ASE_STATE_STREAMING:
                APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_STREAMING: cig_id %d, cis_id %d, metadata[%d] %b",
                                p_data->ase_data.param.streaming.cig_id,
                                p_data->ase_data.param.streaming.cis_id,
                                p_data->ase_data.param.streaming.metadata_length,
                                TRACE_BINARY(p_data->ase_data.param.streaming.metadata_length,
                                             p_data->ase_data.param.streaming.p_metadata));
                break;

            case ASE_STATE_DISABLING:
                APP_PRINT_INFO4("[BAP][ASCS] ASE_STATE_DISABLING: cig_id %d, cis_id %d, metadata[%d] %b",
                                p_data->ase_data.param.disabling.cig_id,
                                p_data->ase_data.param.disabling.cis_id,
                                p_data->ase_data.param.disabling.metadata_length,
                                TRACE_BINARY(p_data->ase_data.param.disabling.metadata_length,
                                             p_data->ase_data.param.disabling.p_metadata));
                break;

            default:
                break;
            }
            app_lea_acc_bap_update_ase_state(p_data);
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_CONFIG_CODEC_IND:
        {
            T_ASCS_CP_CONFIG_CODEC_IND *p_data = (T_ASCS_CP_CONFIG_CODEC_IND *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_CONFIG_CODEC_IND: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);
            for (uint8_t i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO5("ase param[%d]: ase_id %d, target_latency %d, target_phy %d, codec_id %b",
                                i, p_data->param[i].data.ase_id,
                                p_data->param[i].data.target_latency,
                                p_data->param[i].data.target_phy,
                                TRACE_BINARY(5, p_data->param[i].data.codec_id));

                APP_PRINT_INFO7("ase param[%d]: type_exist 0x%x, frame_duration 0x%x, sample_frequency 0x%x, codec_frame_blocks_per_sdu %d, octets_per_codec_frame %d, audio_channel_allocation 0x%x",
                                i, p_data->param[i].codec_parsed_data.type_exist,
                                p_data->param[i].codec_parsed_data.frame_duration,
                                p_data->param[i].codec_parsed_data.sample_frequency,
                                p_data->param[i].codec_parsed_data.codec_frame_blocks_per_sdu,
                                p_data->param[i].codec_parsed_data.octets_per_codec_frame,
                                p_data->param[i].codec_parsed_data.audio_channel_allocation
                               );
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_CONFIG_QOS_IND:
        {
            T_ASCS_CP_CONFIG_QOS_IND *p_data = (T_ASCS_CP_CONFIG_QOS_IND *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_CONFIG_QOS_IND: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);
            for (uint8_t i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO4("ase param[%d]: ase_id %d,  cig_id %d, cis_id %d",
                                i, p_data->param[i].ase_id,
                                p_data->param[i].cig_id, p_data->param[i].cis_id);
                APP_PRINT_INFO8("ase param[%d]: sdu_interval %x %x %x, framing %d, phy 0x%x, max_sdu  %x %x",
                                i, p_data->param[i].sdu_interval[0], p_data->param[i].sdu_interval[1],
                                p_data->param[i].sdu_interval[2],
                                p_data->param[i].framing, p_data->param[i].phy,
                                p_data->param[i].max_sdu[0], p_data->param[i].max_sdu[1]);

                APP_PRINT_INFO7("ase param[%d]: retransmission_number %d, max_transport_latency %x %x, presentation_delay  %x %x %x",
                                i,
                                p_data->param[i].retransmission_number,
                                p_data->param[i].max_transport_latency[0],
                                p_data->param[i].max_transport_latency[1],
                                p_data->param[i].presentation_delay[0],
                                p_data->param[i].presentation_delay[1],
                                p_data->param[i].presentation_delay[2]);
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_ENABLE_IND:
        {
            T_ASCS_CP_ENABLE_IND *p_data = (T_ASCS_CP_ENABLE_IND *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_ENABLE_IND: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);
            for (uint8_t i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO4("ase param[%d]: ase_id %d, metadata_length %d, metadata %b",
                                i, p_data->param[i].ase_id,
                                p_data->param[i].metadata_length,
                                TRACE_BINARY(p_data->param[i].metadata_length, p_data->param[i].p_metadata));
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_DISABLE_IND:
        {
            T_ASCS_CP_DISABLE_IND *p_data = (T_ASCS_CP_DISABLE_IND *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_ASCS_CP_DISABLE_IND: conn_handle 0x%x, number_of_ase %d, ase_ids %b",
                            p_data->conn_handle,
                            p_data->number_of_ase, TRACE_BINARY(p_data->number_of_ase, p_data->ase_id));
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_UPDATE_METADATA_IND:
        {
            T_ASCS_CP_UPDATE_METADATA_IND *p_data = (T_ASCS_CP_UPDATE_METADATA_IND *)buf;

            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_UPDATE_METADATA_IND: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);

            for (uint8_t i = 0; i < p_data->number_of_ase; i++)
            {
                APP_PRINT_INFO4("ase param[%d]: ase_id %d, metadata_length %d, metadata %b",
                                i, p_data->param[i].ase_id,
                                p_data->param[i].metadata_length,
                                TRACE_BINARY(p_data->param[i].metadata_length, p_data->param[i].p_metadata));
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_GET_PREFER_QOS:
        {
            T_ASCS_GET_PREFER_QOS *p_data = (T_ASCS_GET_PREFER_QOS *)buf;

            APP_PRINT_INFO5("LE_AUDIO_MSG_ASCS_GET_PREFER_QOS: conn_handle 0x%x, ase_id %d, direction %d, target_latency %d, target_phy %d",
                            p_data->conn_handle, p_data->ase_id, p_data->direction, p_data->target_latency, p_data->target_phy);
            APP_PRINT_INFO3("LE_AUDIO_MSG_ASCS_GET_PREFER_QOS:frame_duration %d, sample_frequency 0x%x, octets_per_codec_frame %d",
                            p_data->codec_cfg.frame_duration,
                            p_data->codec_cfg.sample_frequency,
                            p_data->codec_cfg.octets_per_codec_frame);
#if 0 //Sample for Configure ASE preferred QoS data
            T_ASCS_PREFER_QOS_DATA prefer_qos;
            if (ascs_get_ase_prefer_qos(p_data->conn_handle, p_data->ase_id, &prefer_qos))
            {
                prefer_qos.preferred_presentation_delay_min = 0;
                prefer_qos.preferred_presentation_delay_max = 30000;
                ascs_cfg_ase_prefer_qos(p_data->conn_handle, p_data->ase_id, &prefer_qos);
            }
#endif
        }
        break;

    case LE_AUDIO_MSG_ASCS_CIS_REQUEST_IND:
        {
            T_ASCS_CIS_REQUEST_IND *p_data = (T_ASCS_CIS_REQUEST_IND *)buf;
            APP_PRINT_INFO6("LE_AUDIO_MSG_ASCS_CIS_REQUEST_IND: conn_handle 0x%x, cis_conn_handle 0x%x, snk_ase_id %d, snk_ase_state %d, src_ase_id %d, src_ase_state %d",
                            p_data->conn_handle, p_data->cis_conn_handle,
                            p_data->snk_ase_id, p_data->snk_ase_state,
                            p_data->src_ase_id, p_data->src_ase_state);
            cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
        }
        break;


    case LE_AUDIO_MSG_ASCS_AUDIO_CONTEXTS_CHECK_IND:
        {
            T_ASCS_AUDIO_CONTEXTS_CHECK_IND *p_data = (T_ASCS_AUDIO_CONTEXTS_CHECK_IND *)buf;
            uint16_t available_contexts;

            APP_PRINT_INFO5("LE_AUDIO_MSG_ASCS_AUDIO_CONTEXTS_CHECK: conn_handle 0x%x, update 0x%x, ase_id %d, direction %d, context 0x%x",
                            p_data->conn_handle, p_data->is_update_metadata,
                            p_data->ase_id, p_data->direction,
                            p_data->available_contexts);
            if (p_data->direction == SERVER_AUDIO_SINK)
            {
                available_contexts = app_db.lea_sink_available_contexts;
            }
            else
            {
                available_contexts = app_db.lea_source_available_contexts;
            }
            if ((available_contexts & p_data->available_contexts) == 0)
            {
                cb_result = BLE_AUDIO_CB_RESULT_REJECT;
            }
            if ((available_contexts | p_data->available_contexts) != available_contexts)
            {
                cb_result = BLE_AUDIO_CB_RESULT_REJECT;
            }
        }
        break;
#endif

#if BAP_SCAN_DELEGATOR
    case LE_AUDIO_MSG_BASS_CP_IND:
        {
            T_BASS_CP_IND *p_data = (T_BASS_CP_IND *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_CP_IND: conn_handle 0x%x, cp opcode 0x%x",
                            p_data->conn_handle, p_data->p_cp_data->cp_op);
            if (p_data->p_cp_data->cp_op == BASS_CP_OP_SET_BROADCAST_CODE)
            {
                T_APP_LEA_SYNC_INFO *p_sync_info = app_lea_find_by_source_id(
                                                       p_data->p_cp_data->param.set_broadcast_code.source_id);
                if (p_sync_info)
                {
                    memcpy(p_sync_info->broadcast_code, p_data->p_cp_data->param.set_broadcast_code.broadcast_code,
                           BROADCAST_CODE_LEN);
                    p_sync_info->big_proc_flags |= APP_LEA_BIG_PROC_BROADCAST_CODE_EXIST;
                    app_lea_big_check_sync(p_sync_info);
                }
            }
        }
        break;

    case LE_AUDIO_MSG_BASS_BRS_MODIFY:
        {
            T_BASS_BRS_MODIFY *p_data = (T_BASS_BRS_MODIFY *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_BRS_MODIFY: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
            if (p_data->p_brs_data)
            {
                if (p_data->p_brs_data->brs_is_used)
                {
                    APP_PRINT_INFO2("source_address_type 0x%02x, source_adv_sid 0x%02x",
                                    p_data->p_brs_data->source_address_type, p_data->p_brs_data->source_adv_sid);
                    APP_PRINT_INFO4("pa_sync_state %d, bis_sync_state 0x%x, big_encryption %d, num_subgroups %d",
                                    p_data->p_brs_data->pa_sync_state, p_data->p_brs_data->bis_sync_state,
                                    p_data->p_brs_data->big_encryption, p_data->p_brs_data->num_subgroups);
                    for (uint8_t i = 0; i < p_data->p_brs_data->num_subgroups; i++)
                    {
                        if (p_data->p_brs_data->p_cp_bis_info[i].metadata_len == 0)
                        {
                            APP_PRINT_INFO2("Subgroup[%d], BIS Sync: [0x%x], Metadata Length: [0]", i,
                                            p_data->p_brs_data->p_cp_bis_info[i].bis_sync);
                        }
                        else
                        {
                            APP_PRINT_INFO4("Subgroup[%d], BIS Sync: [0x%x] Metadata data[%d]: %b", i,
                                            p_data->p_brs_data->p_cp_bis_info[i].bis_sync,
                                            p_data->p_brs_data->p_cp_bis_info[i].metadata_len,
                                            TRACE_BINARY(p_data->p_brs_data->p_cp_bis_info[i].metadata_len,
                                                         p_data->p_brs_data->p_cp_bis_info[i].p_metadata));
                        }
                    }
                }
                else
                {
                    APP_PRINT_INFO0("LE_AUDIO_MSG_BASS_BRS_MODIFY: brs char not used");
                }
            }
        }
        break;

    case LE_AUDIO_MSG_BASS_BA_ADD_SOURCE:
        {
            T_APP_LEA_SYNC_INFO *p_sync_info;
            T_BASS_BA_ADD_SOURCE *p_data = (T_BASS_BA_ADD_SOURCE *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_BA_ADD_SOURCE: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);

            p_sync_info = app_lea_add_sync_handle(p_data->handle);
            if (p_sync_info)
            {
                p_sync_info->source_id = p_data->source_id;
            }
            ble_audio_sync_update_cb(p_data->handle, app_lea_acc_sync_cb);
        }
        break;

    case LE_AUDIO_MSG_BASS_LOCAL_ADD_SOURCE:
        {
            T_APP_LEA_SYNC_INFO *p_sync_info;
            T_BASS_LOCAL_ADD_SOURCE *p_data = (T_BASS_LOCAL_ADD_SOURCE *)buf;

            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_LOCAL_ADD_SOURCE: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
            p_sync_info = app_lea_add_sync_handle(p_data->handle);
            if (p_sync_info)
            {
                p_sync_info->source_id = p_data->source_id;
            }
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_PA_SYNC_PARAM:
        {
            T_BASS_GET_PA_SYNC_PARAM *p_data = (T_BASS_GET_PA_SYNC_PARAM *)buf;

            APP_PRINT_INFO4("LE_AUDIO_MSG_BASS_GET_PA_SYNC_PARAM: sync handle %p, source_id %d, is_past %d, pa_interval 0x%x",
                            p_data->handle, p_data->source_id,
                            p_data->is_past, p_data->pa_interval);
#if 0
            uint16_t pa_sync_timeout_10ms = 200;
            bass_cfg_pa_sync_param(p_data->source_id, 0, 0, pa_sync_timeout_10ms, 300);
#endif
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_BIG_SYNC_PARAM:
        {
            T_BASS_GET_BIG_SYNC_PARAM *p_data = (T_BASS_GET_BIG_SYNC_PARAM *)buf;

            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_GET_BIG_SYNC_PARAM: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
#if 0
            T_BLE_AUDIO_SYNC_INFO sync_info;
            uint16_t big_sync_timeout_10ms = 200;
            if (ble_audio_sync_get_info(p_data->handle, &sync_info))
            {
                if (sync_info.big_info_received)
                {
                    big_sync_timeout_10ms = ((sync_info.big_info.iso_interval * 1.25) / 10) * 20;
                }

            }
            bass_cfg_big_sync_param(p_data->source_id, 0, big_sync_timeout_10ms);
#endif
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_BROADCAST_CODE:
        {
            T_BASS_GET_BROADCAST_CODE *p_data = (T_BASS_GET_BROADCAST_CODE *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_GET_BROADCAST_CODE: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC:
        {
            T_BASS_SET_PREFER_BIS_SYNC *p_data = (T_BASS_SET_PREFER_BIS_SYNC *)buf;

            APP_PRINT_INFO3("LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC: sync handle %p, source_id %d, num_subgroups %d",
                            p_data->handle, p_data->source_id,
                            p_data->num_subgroups);
            app_lea_acc_bass_set_prefer_bis(p_data);
        }
        break;

    case LE_AUDIO_MSG_BASS_BRS_CHAR_NO_EMPTY:
        {
            T_BASS_BRS_CHAR_NO_EMPTY *p_data = (T_BASS_BRS_CHAR_NO_EMPTY *)buf;
            APP_PRINT_INFO1("LE_AUDIO_MSG_BASS_BRS_CHAR_NO_EMPTY:conn_handle 0x%x",
                            p_data->conn_handle);
        }
        break;
#endif
    default:
        break;
    }
    return cb_result;
}

void app_lea_acc_bap_init(void)
{
#if LE_AUDIO_BAP_SUPPORT
    T_BAP_ROLE_INFO role_info = {0};

    role_info.init_gap = true;
#endif
    ble_audio_cback_register(app_lea_acc_bap_handle_msg);

#if BAP_UNICAST_SERVER
    role_info.role_mask |= BAP_UNICAST_SRV_SRC_ROLE;
    role_info.role_mask |= BAP_UNICAST_SRV_SNK_ROLE;
#endif
#if BAP_SCAN_DELEGATOR
    role_info.role_mask |= BAP_SCAN_DELEGATOR_ROLE;
#endif
#if BAP_BROADCAST_SINK
    role_info.role_mask |= BAP_BROADCAST_SINK_ROLE;
#endif
#if BAP_UNICAST_SERVER
    role_info.isoc_cig_max_num = APP_ISOC_CIG_MAX_NUM;
    role_info.isoc_cis_max_num = APP_ISOC_CIS_MAX_NUM;
    role_info.snk_ase_num = APP_LEA_SINK_ASE_NUM;
    role_info.src_ase_num = APP_LEA_SRC_ASE_NUM;
#endif
#if BAP_SCAN_DELEGATOR
    role_info.pa_sync_num = APP_MAX_SYNC_HANDLE_NUM;
    role_info.brs_num = APP_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM;
#endif
#if BAP_BROADCAST_SINK
    role_info.isoc_big_receiver_num = APP_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM;
    role_info.isoc_bis_receiver_num = APP_SYNC_RECEIVER_MAX_BIS_NUM;
#endif

#if LE_AUDIO_BAP_SUPPORT
    bap_role_init(&role_info);
#endif

#if (BAP_UNICAST_SERVER || BAP_BROADCAST_SINK)
    T_PACS_PARAMS pacs_params = {0};

    app_db.lea_sink_available_contexts = AUDIO_CONTEXT_UNSPECIFIED | AUDIO_CONTEXT_MEDIA |
                                         AUDIO_CONTEXT_CONVERSATIONAL;
    app_db.lea_source_available_contexts = AUDIO_CONTEXT_UNSPECIFIED | AUDIO_CONTEXT_CONVERSATIONAL;
    app_db.lea_sink_supported_contexts = AUDIO_CONTEXT_UNSPECIFIED | AUDIO_CONTEXT_MEDIA |
                                         AUDIO_CONTEXT_CONVERSATIONAL;
    app_db.lea_source_supported_contexts = AUDIO_CONTEXT_UNSPECIFIED | AUDIO_CONTEXT_CONVERSATIONAL;
    if (app_db.csis_cfg == APP_LEA_CSIS_CFG_RANK_1)
    {
        app_db.lea_sink_audio_location = AUDIO_LOCATION_FL;
        app_db.lea_source_audio_location = AUDIO_LOCATION_FL;
    }
    else if (app_db.csis_cfg == APP_LEA_CSIS_CFG_RANK_1)
    {
        app_db.lea_sink_audio_location = AUDIO_LOCATION_FR;
        app_db.lea_source_audio_location = AUDIO_LOCATION_FR;
    }
    else
    {
        app_db.lea_sink_audio_location = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;
        app_db.lea_source_audio_location = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;
    }

    pacs_params.sink_locations.is_exist = true;
    pacs_params.sink_locations.is_notify = true;
    pacs_params.sink_locations.is_write = false;
    pacs_params.sink_locations.sink_audio_location = app_db.lea_sink_audio_location;
    pacs_params.source_locations.is_exist = true;
    pacs_params.source_locations.is_notify = true;
    pacs_params.source_locations.is_write = false;
    pacs_params.source_locations.source_audio_location = app_db.lea_source_audio_location;
    pacs_params.supported_contexts.is_notify = true;
    pacs_params.supported_contexts.sink_supported_contexts = app_db.lea_sink_supported_contexts;
    pacs_params.supported_contexts.source_supported_contexts = app_db.lea_source_supported_contexts;

    pacs_pac_add(SERVER_AUDIO_SINK, app_lea_sink_codec,
                 sizeof(app_lea_sink_codec),
                 true);
    pacs_pac_add(SERVER_AUDIO_SOURCE, app_lea_source_codec,
                 sizeof(app_lea_source_codec),
                 true);

    pacs_init(&pacs_params);
#endif
#if BAP_UNICAST_SERVER
    app_ext_adv_adv_cfg(LE_EXT_ADV_ASCS);
#endif
#if BAP_SCAN_DELEGATOR
    app_ext_adv_adv_cfg(LE_EXT_ADV_BASS);
#endif
}

