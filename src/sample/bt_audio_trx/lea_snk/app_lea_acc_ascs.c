/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "ascs_def.h"
#include "ascs_mgr.h"
#include "ble_audio.h"
#include "ble_conn.h"
#include "bt_direct_msg.h"
#include "metadata_def.h"
#include "app_lea_acc_ascs.h"
#include "app_lea_acc_pacs.h"
#include "app_main.h"

static uint16_t app_lea_ascs_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    if ((msg & 0xFF00) == LE_AUDIO_MSG_GROUP_ASCS)
    {
        APP_PRINT_INFO1("app_lea_ascs_ble_audio_cback: msg %x", msg);
    }

    switch (msg)
    {
    case LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH:
        {
            T_ASCS_SETUP_DATA_PATH *p_data = (T_ASCS_SETUP_DATA_PATH *)buf;

            p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_data->conn_handle,
                                                      (void *)(&p_data->ase_id));
            if (p_ase_entry != NULL)
            {
                p_ase_entry->path_direction = p_data->path_direction;

                if (p_ase_entry->path_direction == DATA_PATH_OUTPUT_FLAG)
                {
                    T_APP_LE_LINK *p_link;

                    p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);
                    if (p_ase_entry->codec_cfg.audio_channel_allocation == AUDIO_CHANNEL_LOCATION_FL)
                    {
                        p_link->cis_left_ch_iso_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
                        p_link->cis_left_ch_conn_handle = p_data->cis_conn_handle;

                    }
                    else
                    {
                        p_link->cis_right_ch_iso_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
                        p_link->cis_right_ch_conn_handle = p_data->cis_conn_handle;
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_REMOVE_DATA_PATH:
        {
            T_ASCS_REMOVE_DATA_PATH *p_data = (T_ASCS_REMOVE_DATA_PATH *)buf;
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_CONFIG_CODEC_IND:
        {
            uint8_t i;
            T_ASCS_CP_CONFIG_CODEC_IND *p_data = (T_ASCS_CP_CONFIG_CODEC_IND *)buf;

            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_CONFIG_CODEC_IND: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle,
                            p_data->number_of_ase);

            for (uint8_t i = 0; i < p_data->number_of_ase; i++)
            {
                p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_data->conn_handle,
                                                          (void *)(&p_data->param[i].data.ase_id));
                if (p_ase_entry != NULL)
                {
                    memcpy(&p_ase_entry->codec_cfg, &p_data->param[i].codec_parsed_data, sizeof(T_CODEC_CFG));
                }
                else
                {
                    app_lea_ascs_alloc_ase_entry(p_data->conn_handle, p_data->param[i].data.ase_id,
                                                 p_data->param[i].data.codec_id, p_data->param[i].codec_parsed_data);
                }
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
                p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_data->conn_handle,
                                                          (void *)(&p_data->param[i].ase_id));
                if (p_ase_entry != NULL)
                {
                    p_ase_entry->cig_id = p_data->param[i].cig_id;
                    p_ase_entry->cis_id = p_data->param[i].cis_id;
                    LE_ARRAY_TO_UINT24(p_ase_entry->presentation_delay, p_data->param[i].presentation_delay);
                }
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
                uint16_t audio_context = 0;

                audio_context = app_lea_ascs_get_context(p_data->param[i].p_metadata,
                                                         p_data->param[i].metadata_length);
                p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_data->conn_handle,
                                                          (void *)(&p_data->param[i].ase_id));
                if (p_ase_entry != NULL)
                {
                    p_ase_entry->control_point_enable = true;
                    p_ase_entry->audio_context = audio_context;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_DISABLE_IND:
        {
            T_ASCS_CP_DISABLE_IND *p_data = (T_ASCS_CP_DISABLE_IND *)buf;

            APP_PRINT_INFO2("LE_AUDIO_MSG_ASCS_CP_DISABLE_IND: conn_handle 0x%x, number_of_ase %d",
                            p_data->conn_handle, p_data->number_of_ase);

            p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CONN, p_data->conn_handle, NULL);
            if (p_ase_entry != NULL)
            {
                p_ase_entry->control_point_enable = false;
            }
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
                uint16_t audio_context = 0;

                audio_context = app_lea_ascs_get_context(p_data->param[i].p_metadata,
                                                         p_data->param[i].metadata_length);
                p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_data->conn_handle,
                                                          (void *)(&p_data->param[i].ase_id));
                if (p_ase_entry != NULL && audio_context)
                {
                    p_ase_entry->audio_context = audio_context;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CP_RELEASE_IND:
        {
            T_ASCS_CP_RELEASE_IND *p_data = (T_ASCS_CP_RELEASE_IND *)buf;

            for (uint8_t i = 0; i < p_data->number_of_ase; i++)
            {
                p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_ASE_ID, p_data->conn_handle,  &p_data->ase_id[i]);

                if (p_ase_entry != NULL)
                {
                    p_ase_entry->control_point_enable = false;
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
                T_LEA_ASE_ENTRY *p_ase_entry = NULL;

                for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
                {
                    p_ase_entry = &p_link->lea_ase_entry[i];
                    if (p_ase_entry->used == true &&
                        p_ase_entry->cig_id == p_data->cig_id &&
                        p_ase_entry->cis_id == p_data->cis_id)
                    {
                        p_ase_entry->cis_conn_handle = 0;
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_ASCS_CIS_CONN_INFO:
        {
            T_ASCS_CIS_CONN_INFO *p_data = (T_ASCS_CIS_CONN_INFO *)buf;
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);

            if (p_link)
            {
                T_LEA_ASE_ENTRY *p_ase_entry = NULL;

                for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
                {
                    p_ase_entry = &p_link->lea_ase_entry[i];
                    if (p_ase_entry->used == true &&
                        p_ase_entry->cig_id == p_data->cig_id &&
                        p_ase_entry->cis_id == p_data->cis_id)
                    {
                        p_ase_entry->cis_conn_handle = p_data->cis_conn_handle;
                    }
                }
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

static bool app_lea_ascs_check_ase_condition(T_LEA_ASE_ENTRY *p_ase_entry, T_LEA_ASE_TYPE type,
                                             void *para)
{
    bool ret = false;

    switch (type)
    {
    case LEA_ASE_DEVICE_INOUT:
        {
            if (p_ase_entry->track_handle != NULL)
            {
                uint32_t device = 0;

                audio_track_device_get(p_ase_entry->track_handle, &device);

                if (device == (app_db.playback_device | AUDIO_DEVICE_IN_MIC))
                {
                    ret = true;
                }
            }
        }
        break;

    case LEA_ASE_UP_DIRECT:
        {
            if (p_ase_entry->path_direction == DATA_PATH_INPUT_FLAG)
            {
                ret = true;
            }
        }
        break;

    case LEA_ASE_DOWN_DIRECT:
        {
            if (p_ase_entry->cis_conn_handle == *(uint16_t *)para &&
                p_ase_entry->path_direction == DATA_PATH_OUTPUT_FLAG)
            {
                ret = true;
            }
        }
        break;

    case LEA_ASE_ASE_ID:
        {
            if (p_ase_entry->ase_id == *(uint8_t *)para)
            {
                ret = true;
            }
        }
        break;

    case LEA_ASE_CONN:
        {
            if (p_ase_entry->conn_handle == *(uint16_t *)para)
            {
                ret = true;
            }
        }
        break;

    case LEA_ASE_CIS_CONN:
        {
            if (p_ase_entry->cis_conn_handle == *(uint16_t *)para)
            {
                ret = true;
            }
        }
        break;

    case LEA_ASE_TRACK:
        {
            if (p_ase_entry->track_handle == para)
            {
                ret = true;
            }
        }
        break;

    case LEA_ASE_CONTEXT:
        {
            if (p_ase_entry->audio_context == *(uint16_t *)para)
            {
                ret = true;
            }
        }
        break;

    default:
        break;
    }

    return ret;
}

bool app_lea_ascs_alloc_ase_entry(uint16_t conn_handle, uint8_t ase_id,
                                  uint8_t codec_id[CODEC_ID_LEN], T_CODEC_CFG codec_cfg)
{
    bool ret = false;
    T_APP_LE_LINK *p_link;

    p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        T_LEA_ASE_ENTRY *p_ase_entry = NULL;

        for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
        {
            if (p_link->lea_ase_entry[i].used == false)
            {
                p_ase_entry = &p_link->lea_ase_entry[i];

                p_ase_entry->used = true;
                p_ase_entry->ase_id = ase_id;
                p_ase_entry->conn_handle = conn_handle;
                p_ase_entry->codec_type = AUDIO_FORMAT_TYPE_LC3;
                memcpy(&p_ase_entry->codec_cfg, &codec_cfg, sizeof(T_CODEC_CFG));
#if F_APP_LC3_PLUS_SUPPORT
                p_ase_entry->codec_type = app_lea_pacs_check_codec_type(codec_id);
#endif
                ret = true;
                break;
            }
        }
    }

    return ret;
}

bool app_lea_ascs_free_ase_entry(T_LEA_ASE_ENTRY *p_ase_entry)
{
    if (p_ase_entry != NULL)
    {
        if (p_ase_entry->used == true)
        {
            if (p_ase_entry->track_handle != NULL)
            {
                audio_track_release(p_ase_entry->track_handle);
            }

            memset(p_ase_entry, 0, sizeof(T_LEA_ASE_ENTRY));
            return true;
        }
    }

    return false;
}

uint16_t app_lea_ascs_get_context(uint8_t *p_data, uint16_t len)
{
    uint16_t audio_context = 0;
    uint8_t length = 0;
    uint8_t type = 0;
    uint8_t *p = p_data;

    if (p_data == NULL || len == 0)
    {
        return false;
    }

    while (len > 0)
    {
        LE_STREAM_TO_UINT8(length, p);
        LE_STREAM_TO_UINT8(type, p);

        if (type == METADATA_TYPE_STREAMING_AUDIO_CONTEXTS)
        {
            if (length == 3)
            {
                LE_STREAM_TO_UINT16(audio_context, p);
                break;
            }
        }

        len -= (length + 1);
    }

    return audio_context;
}

T_LEA_ASE_ENTRY *app_lea_ascs_find_ase_entry_by_direction(T_APP_LE_LINK *p_link,
                                                          uint8_t path_direction)
{
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    if (p_link)
    {
        for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
        {
            if (p_link->lea_ase_entry[i].used == true &&
                p_link->lea_ase_entry[i].path_direction == path_direction)
            {
                T_AUDIO_TRACK_STATE state;

                audio_track_state_get(p_link->lea_ase_entry[i].track_handle, &state);
                if (state == AUDIO_TRACK_STATE_STARTED)
                {
                    p_ase_entry = &p_link->lea_ase_entry[i];
                    break;
                }
                else
                {
                    APP_PRINT_INFO0("app_lea_mgr_find_ase_by_direction track not running!");
                }
            }
        }
    }

    return p_ase_entry;
}

T_LEA_ASE_ENTRY *app_lea_ascs_find_ase_entry(T_LEA_ASE_TYPE type, uint16_t conn_handle,
                                             void *data)
{
    T_APP_LE_LINK *p_link;
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;
    void *p_data = data;

    p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        if (p_data == NULL)
        {
            p_data = (void *)(&conn_handle);
        }

        for (uint8_t i = 0; i < ASCS_ASE_ENTRY_NUM; i++)
        {
            if (p_link->lea_ase_entry[i].used == true &&
                app_lea_ascs_check_ase_condition(&p_link->lea_ase_entry[i], type, p_data))
            {
                p_ase_entry = &p_link->lea_ase_entry[i];
                break;
            }
        }
    }

    return p_ase_entry;
}

T_LEA_ASE_ENTRY *app_lea_ascs_find_ase_entry_non_conn(T_LEA_ASE_TYPE type, void *data, uint8_t *idx)
{
    uint8_t i, j;
    T_APP_LE_LINK *p_link;
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true)
        {
            p_link = &app_db.le_link[i];
            for (j = 0; j < ASCS_ASE_ENTRY_NUM; j++)
            {
                if (p_link->lea_ase_entry[j].used == true &&
                    app_lea_ascs_check_ase_condition(&p_link->lea_ase_entry[j], type, data))
                {
                    if (idx != NULL)
                    {
                        *idx = i;
                    }
                    p_ase_entry = &p_link->lea_ase_entry[j];
                    return p_ase_entry;
                }
            }
        }
    }

    return p_ase_entry;
}

void app_lea_ascs_init(void)
{
    ble_audio_cback_register(app_lea_ascs_ble_audio_cback);
}
#endif
