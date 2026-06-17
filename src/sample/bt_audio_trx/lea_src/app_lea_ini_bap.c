/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include <stdint.h>
#include <stdlib.h>
#include "ble_audio.h"
#include "bap.h"
#include "trace.h"
// #include "data_uart.h"
// #include "app_lea_ini_main.h"
#include "app_lea_ini_bap.h"
#include "ble_audio.h"
#include "ble_audio_def.h"
#include "bass_client.h"
#include "metadata_def.h"
#include "app_lea_ini_scan.h"
#include "base_data_parse.h"
#include "base_data_generate.h"

#include "app_main.h"
#include "string.h"

#if BAP_BROADCAST_ASSISTANT
#define APP_MAX_SYNC_HANDLE_NUM                   4
#endif

#if (BAP_BROADCAST_ASSISTANT || BAP_BROADCAST_SOURCE)
#if BAP_BROADCAST_ASSISTANT
void app_lea_link_init_brs_char(T_APP_LE_LINK *p_link, uint8_t brs_char_num)
{
    if (p_link->brs_char_tbl)
    {
        free(p_link->brs_char_tbl);
        p_link->brs_char_num = 0;
        p_link->brs_char_tbl = NULL;
    }
    if (brs_char_num)
    {
        p_link->brs_char_tbl = calloc(1, brs_char_num * sizeof(T_BASS_BRS_CHAR_DB));
        if (p_link->brs_char_tbl)
        {
            T_BASS_BRS_DATA *p_brs_data;

            p_link->brs_char_num = brs_char_num;
            for (uint8_t i = 0; i < brs_char_num; i++)
            {
                p_link->brs_char_tbl[i].instance_id = i;
                p_brs_data = bass_get_brs_data(p_link->conn_handle, i);
                if (p_brs_data)
                {
                    if (p_brs_data->brs_is_used)
                    {
                        p_link->brs_char_tbl[i].brs_is_used = true;
                        p_link->brs_char_tbl[i].source_id = p_brs_data->source_id;
#if BAP_BROADCAST_SOURCE
                        p_link->brs_char_tbl[i].handle = app_lea_bsrc_get_handle(p_brs_data->source_address,
                                                                                 p_brs_data->source_address_type,
                                                                                 p_brs_data->source_adv_sid, p_brs_data->broadcast_id);
#endif
                        T_APP_LEA_SYNC_INFO *p_sync_info;
                        p_sync_info = app_lea_find_sync_handle_by_addr(p_brs_data->source_address,
                                                                       p_brs_data->source_address_type,
                                                                       p_brs_data->source_adv_sid, p_brs_data->broadcast_id);
                        if (p_sync_info)
                        {
                            APP_PRINT_TRACE1("app_lea_link_init_brs_char: idx %d add sync", i);
                            p_link->brs_char_tbl[i].sync_handle = p_sync_info->sync_handle;
                        }

                    }
                }
            }
        }
    }
}
#endif

T_BASS_BRS_CHAR_DB *app_lea_link_find_brs_char(T_APP_LE_LINK *p_link, uint8_t instance_id)
{
    if (instance_id < p_link->brs_char_num)
    {
        return &p_link->brs_char_tbl[instance_id];
    }
    return NULL;
}

T_BASS_BRS_CHAR_DB *app_lea_link_find_brs_char_by_source_handle(T_APP_LE_LINK *p_link,
                                                                T_BROADCAST_SOURCE_HANDLE source_handle)
{
    for (uint8_t i = 0; i < p_link->brs_char_num; i++)
    {
        if (p_link->brs_char_tbl[i].brs_is_used &&
            p_link->brs_char_tbl[i].handle == source_handle)
        {
            return &p_link->brs_char_tbl[i];
        }
    }
    APP_PRINT_TRACE1("app_lea_link_find_brs_char_by_source_handle: not find source_handle %p",
                     source_handle);
    return NULL;
}

void app_lea_link_update_brs_char(T_APP_LE_LINK *p_link, T_BASS_CLIENT_BRS_DATA *p_data)
{
    T_BASS_BRS_CHAR_DB *p_brs_char = app_lea_link_find_brs_char(p_link, p_data->char_instance_id);

    if (p_brs_char)
    {
        if (p_brs_char->brs_is_used != p_data->p_brs_data->brs_is_used)
        {
            if (p_data->p_brs_data->brs_is_used)
            {
                p_brs_char->brs_is_used = true;
                p_brs_char->source_id = p_data->p_brs_data->source_id;
#if BAP_BROADCAST_SOURCE
                p_brs_char->handle = app_lea_bsrc_get_handle(p_data->p_brs_data->source_address,
                                                             p_data->p_brs_data->source_address_type,
                                                             p_data->p_brs_data->source_adv_sid, p_data->p_brs_data->broadcast_id);
#endif
#if BAP_BROADCAST_ASSISTANT
                T_APP_LEA_SYNC_INFO *p_sync_info;
                p_sync_info = app_lea_find_sync_handle_by_addr(p_data->p_brs_data->source_address,
                                                               p_data->p_brs_data->source_address_type,
                                                               p_data->p_brs_data->source_adv_sid, p_data->p_brs_data->broadcast_id);
                if (p_sync_info)
                {
                    APP_PRINT_TRACE1("app_lea_link_update_brs_char: idx %d add sync", p_brs_char->instance_id);
                    p_brs_char->sync_handle = p_sync_info->sync_handle;
                }
#endif
            }
            else
            {
                p_brs_char->brs_is_used = false;
                p_brs_char->source_id = 0;
                p_brs_char->handle = NULL;
            }
        }
    }
}

#if BAP_BROADCAST_ASSISTANT
void app_lea_link_brs_char_add_source_handle(void)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].brs_char_tbl != NULL)
        {
            p_link = &app_db.le_link[i];
            for (uint8_t j = 0; j < p_link->brs_char_num; j++)
            {
                if (p_link->brs_char_tbl[j].brs_is_used &&
                    p_link->brs_char_tbl[j].handle == NULL)
                {
                    T_BASS_BRS_DATA *p_brs_data = bass_get_brs_data(p_link->conn_handle, j);

                    if (p_brs_data)
                    {
#if BAP_BROADCAST_SOURCE
                        p_link->brs_char_tbl[j].handle = app_lea_bsrc_get_handle(p_brs_data->source_address,
                                                                                 p_brs_data->source_address_type,
                                                                                 p_brs_data->source_adv_sid, p_brs_data->broadcast_id);
#endif
                    }
                }
            }
        }
    }
}
#endif

void app_lea_ini_bst_reception_stop(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                    T_BROADCAST_SOURCE_HANDLE source_handle)
{
    uint8_t dev_num;
    T_AUDIO_DEV_INFO *p_dev_tbl;

    dev_num = ble_audio_group_get_dev_num(group_handle);
    if (dev_num)
    {
        p_dev_tbl = calloc(1, dev_num * sizeof(T_AUDIO_DEV_INFO));
        if (p_dev_tbl)
        {
            if (ble_audio_group_get_info(group_handle, &dev_num, p_dev_tbl))
            {
                for (uint8_t j = 0; j < dev_num; j++)
                {
                    if (p_dev_tbl[j].is_used && p_dev_tbl[j].conn_state == GAP_CONN_STATE_CONNECTED)
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_dev_tbl[j].conn_handle);
                        if (p_link)
                        {
                            if (p_link->lea_disc_flags & LEA_LINK_DISC_BASS_DONE)
                            {
                                T_BASS_BRS_CHAR_DB *p_brs_char = app_lea_link_find_brs_char_by_source_handle(p_link, source_handle);
                                if (p_brs_char)
                                {
                                    T_BASS_CP_MODIFY_SOURCE modify_src = {0};
                                    T_BASS_CP_BIS_INFO bis_info = {0};

                                    modify_src.source_id = p_brs_char->source_id;
                                    modify_src.pa_sync = BASS_PA_NOT_SYNC;
                                    modify_src.pa_interval = BASS_PA_INTERVAL_UNKNOWN;
                                    modify_src.num_subgroups = 1;
                                    bis_info.bis_sync = 0;
                                    bis_info.metadata_len = 0;
                                    bis_info.p_metadata = NULL;
                                    modify_src.bis_info_size = modify_src.num_subgroups * sizeof(bis_info) + bis_info.metadata_len;
                                    modify_src.p_cp_bis_info = &bis_info;
                                    bass_cp_modify_source(p_link->conn_handle, &modify_src, false);
                                }
                            }
                        }
                    }
                }
            }
            free(p_dev_tbl);
        }
    }
}

void app_lea_ini_bst_reception_remove(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                      T_BROADCAST_SOURCE_HANDLE source_handle)
{
    uint8_t dev_num;
    T_AUDIO_DEV_INFO *p_dev_tbl;

    dev_num = ble_audio_group_get_dev_num(group_handle);
    if (dev_num)
    {
        p_dev_tbl = calloc(1, dev_num * sizeof(T_AUDIO_DEV_INFO));
        if (p_dev_tbl)
        {
            if (ble_audio_group_get_info(group_handle, &dev_num, p_dev_tbl))
            {
                for (uint8_t j = 0; j < dev_num; j++)
                {
                    if (p_dev_tbl[j].is_used && p_dev_tbl[j].conn_state == GAP_CONN_STATE_CONNECTED)
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_dev_tbl[j].conn_handle);
                        if (p_link)
                        {
                            APP_PRINT_TRACE2("app_lea_ini_bst_reception_remove: conn_handle 0x%x, lea_disc_flags 0x%x",
                                             p_dev_tbl[j].conn_handle, p_link->lea_disc_flags);
                            if (p_link->lea_disc_flags & LEA_LINK_DISC_BASS_DONE)
                            {
                                T_BASS_BRS_CHAR_DB *p_brs_char = app_lea_link_find_brs_char_by_source_handle(p_link, source_handle);
                                if (p_brs_char)
                                {
                                    T_BASS_CP_REMOVE_SOURCE remove_source;
                                    remove_source.source_id = p_brs_char->source_id;
                                    bass_cp_remove_source(p_link->conn_handle, &remove_source, false);
                                }
                            }
                        }
                    }
                }
            }
            free(p_dev_tbl);
        }
    }
}

void app_lea_ini_bst_start(T_BASS_BRS_CHAR_DB *p_brs_char, uint16_t conn_handle)
{
    uint8_t err_idx = 0;
    uint16_t pa_data_len;
    uint8_t *pa_data = NULL;
    T_BASE_DATA_MAPPING *p_base_mapping = NULL;
    uint32_t bis_array;
    T_BASS_CP_BIS_INFO *p_cp_bis_info = NULL;
    uint16_t meta_data_len = 0;
    uint32_t bis_array_temp = 0;
    uint16_t bis_info_size = 0;
    uint8_t *p_meta_data;

    if (base_data_gen_pa_data(app_db.bsrc_db.group1_idx, &pa_data_len, &pa_data) == false)
    {
        err_idx = 1;
        goto ret;
    }
    p_base_mapping = base_data_parse_data(pa_data_len, pa_data);


    bis_array = bap_pacs_check_cfg_by_base_data(p_base_mapping, conn_handle,
                                                (BA_PACS_CHECK_FILTER_CODEC));
    if (bis_array == 0)
    {
        err_idx = 2;
        goto ret;
    }

    if (p_brs_char)
    {
        T_BASS_BRS_DATA *p_brs_data = bass_get_brs_data(conn_handle, p_brs_char->instance_id);
        if (p_brs_data && p_brs_data->p_cp_bis_info != NULL)
        {
            if (p_brs_data->p_cp_bis_info->bis_sync == bis_array)
            {
                err_idx = 0;
                goto ret;
            }
        }
    }
    for (uint8_t i = 0; i < p_base_mapping->num_subgroups; i++)
    {
        if (p_base_mapping->p_subgroup[i].bis_array & bis_array)
        {
            bis_array_temp |= (p_base_mapping->p_subgroup[i].bis_array & bis_array);
        }
        meta_data_len += p_base_mapping->p_subgroup[i].metadata_len;
    }
    if (bis_array != BASS_CP_BIS_SYNC_NO_PREFER && bis_array != bis_array_temp)
    {
        err_idx = 3;
        goto ret;
    }
    bis_info_size = p_base_mapping->num_subgroups * sizeof(
                        T_BASS_CP_BIS_INFO) + meta_data_len;
    p_cp_bis_info = calloc(1, bis_info_size);
    if (p_cp_bis_info == NULL)
    {
        err_idx = 4;
        goto ret;
    }

    p_meta_data = (uint8_t *)(p_cp_bis_info + p_base_mapping->num_subgroups);
    for (uint8_t i = 0; i < p_base_mapping->num_subgroups; i++)
    {
        if (bis_array == BASS_CP_BIS_SYNC_NO_PREFER)
        {
            p_cp_bis_info[i].bis_sync = BASS_CP_BIS_SYNC_NO_PREFER;
        }
        else
        {
            p_cp_bis_info[i].bis_sync = (p_base_mapping->p_subgroup[i].bis_array & bis_array);
        }
        p_cp_bis_info[i].metadata_len = p_base_mapping->p_subgroup[i].metadata_len;
        if (p_cp_bis_info[i].metadata_len > 0)
        {
            memcpy(p_meta_data, p_base_mapping->p_subgroup[i].p_metadata,
                   p_cp_bis_info[i].metadata_len);
            p_cp_bis_info[i].p_metadata = p_meta_data;
            p_meta_data += p_cp_bis_info->metadata_len;
        }
    }

    if (p_brs_char)
    {
        T_BASS_CP_MODIFY_SOURCE modify_src = {0};

        modify_src.bis_info_size = bis_info_size;
        modify_src.p_cp_bis_info = p_cp_bis_info;
        modify_src.num_subgroups = p_base_mapping->num_subgroups;

        modify_src.source_id = p_brs_char->source_id;
        modify_src.pa_sync = BASS_PA_SYNC_PAST;
        modify_src.pa_interval = BASS_PA_INTERVAL_UNKNOWN;

        if (bass_cp_modify_source(conn_handle, &modify_src, false) == false)
        {
            err_idx = 5;
            goto ret;
        }
    }
    else
    {
        T_BASS_CP_ADD_SOURCE add_src = {0};

        add_src.bis_info_size = bis_info_size;
        add_src.p_cp_bis_info = p_cp_bis_info;
        add_src.num_subgroups = p_base_mapping->num_subgroups;

        add_src.advertiser_address_type = app_db.bsrc_db.cfg_local_addr_type;
        memcpy(add_src.advertiser_address, app_db.bsrc_db.source_address, GAP_BD_ADDR_LEN);
        add_src.advertiser_sid = app_db.bsrc_db.source_adv_sid;
        memcpy(add_src.broadcast_id, app_db.bsrc_db.broadcast_id, BROADCAST_ID_LEN);
        add_src.pa_sync = BASS_PA_SYNC_PAST;
        add_src.pa_interval = BASS_PA_INTERVAL_UNKNOWN;

        if (bass_cp_add_source(conn_handle, &add_src, false) == false)
        {
            err_idx = 6;
            goto ret;
        }
    }

    err_idx = 0;
ret:
    if (pa_data)
    {
        free(pa_data);
    }
    if (p_base_mapping)
    {
        base_data_free(p_base_mapping);
    }
    if (p_cp_bis_info)
    {
        free(p_cp_bis_info);
    }
    if (err_idx != 0)
    {
        APP_PRINT_INFO2("app_lea_ini_bst_start: failed, conn_handle 0x%x, err_idx %d", conn_handle,
                        err_idx);
    }
    return;
}

void app_lea_ini_bst_reception_start(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                     T_BROADCAST_SOURCE_HANDLE source_handle)
{
    uint8_t dev_num;
    T_AUDIO_DEV_INFO *p_dev_tbl;

    if (app_db.bsrc_db.source_handle == NULL ||
        app_db.bsrc_db.state != BROADCAST_SOURCE_STATE_STREAMING)
    {
        return;
    }

    dev_num = ble_audio_group_get_dev_num(group_handle);
    if (dev_num)
    {
        p_dev_tbl = calloc(1, dev_num * sizeof(T_AUDIO_DEV_INFO));
        if (p_dev_tbl)
        {
            if (ble_audio_group_get_info(group_handle, &dev_num, p_dev_tbl))
            {
                for (uint8_t j = 0; j < dev_num; j++)
                {
                    if (p_dev_tbl[j].is_used && p_dev_tbl[j].conn_state == GAP_CONN_STATE_CONNECTED)
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_dev_tbl[j].conn_handle);
                        if (p_link)
                        {
                            APP_PRINT_TRACE2("app_lea_ini_bst_reception_start: conn_handle 0x%x, lea_disc_flags 0x%x",
                                             p_dev_tbl[j].conn_handle, p_link->lea_disc_flags);
                            if (p_link->lea_disc_flags & LEA_LINK_DISC_BASS_DONE)
                            {
                                T_BASS_BRS_CHAR_DB *p_brs_char;
                                p_brs_char = app_lea_link_find_brs_char_by_source_handle(p_link, source_handle);
                                app_lea_ini_bst_start(p_brs_char, p_link->conn_handle);
                            }
                        }
                    }
                }
            }
            free(p_dev_tbl);
        }
    }
}
#endif

#if BAP_BROADCAST_ASSISTANT
static void  app_lea_com_sync_cb(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t cb_type, void *p_cb_data);

T_BASS_BRS_CHAR_DB *app_lea_link_find_brs_char_by_sync_handle(T_APP_LE_LINK *p_link,
                                                              T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    for (uint8_t i = 0; i < p_link->brs_char_num; i++)
    {
        if (p_link->brs_char_tbl[i].brs_is_used &&
            p_link->brs_char_tbl[i].sync_handle == sync_handle)
        {
            return &p_link->brs_char_tbl[i];
        }
    }
    APP_PRINT_TRACE1("app_lea_link_find_brs_char_by_sync_handle: not find sync_handle %p", sync_handle);
    return NULL;
}

void app_lea_link_brs_char_add_sync_handle(T_APP_LEA_SYNC_INFO *p_sync_info)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].brs_char_tbl != NULL)
        {
            p_link = &app_db.le_link[i];
            for (uint8_t j = 0; j < p_link->brs_char_num; j++)
            {
                if (p_link->brs_char_tbl[j].brs_is_used &&
                    p_link->brs_char_tbl[j].sync_handle == NULL)
                {
                    T_BASS_BRS_DATA *p_brs_data = bass_get_brs_data(p_link->conn_handle, j);

                    if (p_brs_data)
                    {
                        if (p_brs_data->source_adv_sid == p_sync_info->dev_info.adv_sid &&
                            p_brs_data->source_address_type == p_sync_info->dev_info.bd_type &&
                            memcmp(p_brs_data->broadcast_id, p_sync_info->dev_info.broadcast_id, 3) == 0 &&
                            memcmp(p_brs_data->source_address, p_sync_info->dev_info.bd_addr, 6) == 0)
                        {
                            APP_PRINT_TRACE1("app_lea_link_brs_char_add_sync_handle: idx %d add sync", j);
                            p_link->brs_char_tbl[j].sync_handle = p_sync_info->sync_handle;
                        }
                    }
                }
            }
        }
    }
}

void app_lea_link_brs_char_remove_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].brs_char_tbl != NULL)
        {
            p_link = &app_db.le_link[i];
            for (uint8_t j = 0; j < p_link->brs_char_num; j++)
            {
                if (p_link->brs_char_tbl[j].brs_is_used &&
                    p_link->brs_char_tbl[j].sync_handle == sync_handle)
                {
                    APP_PRINT_TRACE1("app_lea_link_brs_char_remove_sync_handle: idx %d remove sync", j);
                    p_link->brs_char_tbl[j].sync_handle = NULL;
                }
            }
        }
    }
}

T_APP_LEA_SYNC_INFO *app_lea_add_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle,
                                             T_APP_LEA_BAAS_DEV_INFO *p_dev_info)
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
        memcpy(&p_sync_info->dev_info, p_dev_info, sizeof(T_APP_LEA_BAAS_DEV_INFO));
        app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_ADD_SYNC_HANDLE, 0,
                         (uint8_t *)&app_db.sync_handle_queue.count,
                         sizeof(app_db.sync_handle_queue.count));
        os_queue_in(&app_db.sync_handle_queue, p_sync_info);
        app_lea_link_brs_char_add_sync_handle(p_sync_info);
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

T_APP_LEA_SYNC_INFO *app_lea_find_sync_handle_by_addr(uint8_t *bd_addr, uint8_t bd_type,
                                                      uint8_t adv_sid, uint8_t broadcast_id[3])
{
    T_APP_LEA_SYNC_INFO *p_sync_info;
    for (uint8_t i = 0; i < app_db.sync_handle_queue.count; i++)
    {
        p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue, i);
        if (p_sync_info != NULL && p_sync_info->dev_info.adv_sid == adv_sid &&
            memcmp(p_sync_info->dev_info.broadcast_id, broadcast_id, 3) == 0)
        {
            if (p_sync_info->dev_info.bd_type == bd_type &&
                memcmp(p_sync_info->dev_info.bd_addr, bd_addr, 6) == 0)
            {
                return p_sync_info;
            }
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
        app_lea_link_brs_char_remove_sync_handle(sync_handle);
        os_queue_delete(&app_db.sync_handle_queue, p_sync_info);
        free(p_sync_info);
    }
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
        sync_handle = ble_audio_sync_allocate(app_lea_com_sync_cb, p_dev_info->bd_type,
                                              p_dev_info->bd_addr, p_dev_info->adv_sid, p_dev_info->broadcast_id);
        if (sync_handle)
        {
            p_sync_info = app_lea_add_sync_handle(sync_handle, p_dev_info);
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
            APP_PRINT_TRACE0("app_lea_pa_sync_by_dev_info: success");
            return true;
        }
    }
    return false;
}

void app_lea_com_bst_reception_stop(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                    T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    uint8_t dev_num;
    T_AUDIO_DEV_INFO *p_dev_tbl;

    dev_num = ble_audio_group_get_dev_num(group_handle);
    if (dev_num)
    {
        p_dev_tbl = calloc(1, dev_num * sizeof(T_AUDIO_DEV_INFO));
        if (p_dev_tbl)
        {
            if (ble_audio_group_get_info(group_handle, &dev_num, p_dev_tbl))
            {
                for (uint8_t j = 0; j < dev_num; j++)
                {
                    if (p_dev_tbl[j].is_used && p_dev_tbl[j].conn_state == GAP_CONN_STATE_CONNECTED)
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_dev_tbl[j].conn_handle);
                        if (p_link)
                        {
                            if (p_link->lea_disc_flags & LEA_LINK_DISC_BASS_DONE)
                            {
                                T_BASS_BRS_CHAR_DB *p_brs_char = app_lea_link_find_brs_char_by_sync_handle(p_link, sync_handle);
                                if (p_brs_char)
                                {
                                    bass_cp_modify_source_by_sync_info(sync_handle, p_link->conn_handle,
                                                                       p_brs_char->source_id, BASS_PA_NOT_SYNC, 0, false);
                                }
                            }
                        }
                    }
                }
            }
            free(p_dev_tbl);
        }
    }
}

void app_lea_com_bst_reception_remove(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                      T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    uint8_t dev_num;
    T_AUDIO_DEV_INFO *p_dev_tbl;

    dev_num = ble_audio_group_get_dev_num(group_handle);
    if (dev_num)
    {
        p_dev_tbl = calloc(1, dev_num * sizeof(T_AUDIO_DEV_INFO));
        if (p_dev_tbl)
        {
            if (ble_audio_group_get_info(group_handle, &dev_num, p_dev_tbl))
            {
                for (uint8_t j = 0; j < dev_num; j++)
                {
                    if (p_dev_tbl[j].is_used && p_dev_tbl[j].conn_state == GAP_CONN_STATE_CONNECTED)
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_dev_tbl[j].conn_handle);
                        if (p_link)
                        {
                            APP_PRINT_TRACE2("app_lea_com_bst_reception_remove: conn_handle 0x%x, lea_disc_flags 0x%x",
                                             p_dev_tbl[j].conn_handle, p_link->lea_disc_flags);
                            if (p_link->lea_disc_flags & LEA_LINK_DISC_BASS_DONE)
                            {
                                T_BASS_BRS_CHAR_DB *p_brs_char = app_lea_link_find_brs_char_by_sync_handle(p_link, sync_handle);
                                if (p_brs_char)
                                {
                                    T_BASS_CP_REMOVE_SOURCE remove_source;
                                    remove_source.source_id = p_brs_char->source_id;
                                    bass_cp_remove_source(p_link->conn_handle, &remove_source, false);
                                }
                            }
                        }
                    }
                }
            }
            free(p_dev_tbl);
        }
    }
}

void app_lea_com_bst_reception_start(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                     T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    uint8_t dev_num;
    T_AUDIO_DEV_INFO *p_dev_tbl;
    T_BASS_PA_SYNC pa_sync = BASS_PA_SYNC_NO_PAST;
    T_APP_LEA_SYNC_INFO *p_sync_info;

    p_sync_info = app_lea_find_sync_handle(sync_handle);
    if (p_sync_info == NULL)
    {
        return;
    }
    if (p_sync_info->pa_state == GAP_PA_SYNC_STATE_SYNCHRONIZED)
    {
        pa_sync = BASS_PA_SYNC_PAST;
    }

    dev_num = ble_audio_group_get_dev_num(group_handle);
    if (dev_num)
    {
        p_dev_tbl = calloc(1, dev_num * sizeof(T_AUDIO_DEV_INFO));
        if (p_dev_tbl)
        {
            if (ble_audio_group_get_info(group_handle, &dev_num, p_dev_tbl))
            {
                for (uint8_t j = 0; j < dev_num; j++)
                {
                    if (p_dev_tbl[j].is_used && p_dev_tbl[j].conn_state == GAP_CONN_STATE_CONNECTED)
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_dev_tbl[j].conn_handle);
                        if (p_link)
                        {
                            APP_PRINT_TRACE2("app_lea_com_bst_reception_start: conn_handle 0x%x, lea_disc_flags 0x%x",
                                             p_dev_tbl[j].conn_handle, p_link->lea_disc_flags);
                            if (p_link->lea_disc_flags & LEA_LINK_DISC_BASS_DONE)
                            {
                                T_BASS_BRS_CHAR_DB *p_brs_char;
                                uint32_t bis_array;

                                bis_array = bap_pacs_check_cfg_by_sync_info(sync_handle, p_link->conn_handle,
                                                                            (BA_PACS_CHECK_FILTER_CODEC | BA_PACS_CHECK_FILTER_ALLOCATION));
                                if (bis_array == 0)
                                {
                                    APP_PRINT_TRACE0("app_lea_com_bst_reception_start: bis_array == 0");
                                    break;
                                }
                                p_brs_char = app_lea_link_find_brs_char_by_sync_handle(p_link, sync_handle);
                                if (p_brs_char)
                                {
                                    bass_cp_modify_source_by_sync_info(sync_handle, p_link->conn_handle,
                                                                       p_brs_char->source_id, pa_sync,
                                                                       bis_array, false);
                                }
                                else
                                {
                                    bass_cp_add_source_by_sync_info(sync_handle, p_link->conn_handle,
                                                                    pa_sync,
                                                                    bis_array, false);
                                }
                            }
                        }
                    }
                }
            }
            free(p_dev_tbl);
        }
    }
}

static void  app_lea_com_sync_cb(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t cb_type, void *p_cb_data)
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

            }

            if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED ||
                p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)
            {
                app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_PA_SYNC_STATE, 0,
                                 (uint8_t *)&p_sync_cb->p_pa_sync_state->sync_state,
                                 sizeof(p_sync_cb->p_pa_sync_state->sync_state));
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
            }
        }
        break;

    case MSG_BLE_AUDIO_PA_BIGINFO:
        {

        }
        break;

    default:
        break;
    }
    return;
}
#endif

uint16_t app_lea_ini_bap_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_APP_LE_LINK *p_link = NULL;

    switch (msg)
    {
    case LE_AUDIO_MSG_BAP_DIS_ALL_DONE:
        {
            T_BAP_DIS_ALL_DONE *p_data = (T_BAP_DIS_ALL_DONE *)buf;

#if F_APP_AUTO_POWER_TEST_LOG
            TEST_PRINT_INFO1("LE_AUDIO_MSG_BAP_DIS_ALL_DONE: conn_handle 0x%x", p_data->conn_handle);
#else
            APP_PRINT_INFO4("LE_AUDIO_MSG_BAP_DIS_ALL_DONE: conn_handle 0x%x, pacs_is_found %d, sink_pac_num %d, source_pac_num %d",
                            p_data->conn_handle,
                            p_data->pacs_is_found,
                            p_data->sink_pac_num,
                            p_data->source_pac_num);
            APP_PRINT_INFO5("LE_AUDIO_MSG_BAP_DIS_ALL_DONE: ascs_is_found %d, sink_ase_num %d, source_ase_num %d, bass_is_found %d, brs_char_num %d",
                            p_data->ascs_is_found,
                            p_data->sink_ase_num,
                            p_data->source_ase_num,
                            p_data->bass_is_found,
                            p_data->brs_char_num);
#endif
            p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);
            if (p_link)
            {
                app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_BAP_DISCOVERY_DONE, 0, (uint8_t *)&p_link->id,
                                 sizeof(p_link->id));
                if (p_data->ascs_is_found)
                {
                    p_link->lea_disc_flags |= LEA_LINK_DISC_ASCS_DONE;
                    p_link->lea_srv_found_flags |= LEA_LINK_DISC_ASCS_DONE;
                }
                if (p_data->pacs_is_found)
                {
                    p_link->lea_disc_flags |= LEA_LINK_DISC_PACS_DONE;
                    p_link->lea_srv_found_flags |= LEA_LINK_DISC_PACS_DONE;
                }
#if BAP_BROADCAST_ASSISTANT
                if (p_data->bass_is_found)
                {
                    p_link->lea_disc_flags |= LEA_LINK_DISC_BASS_DONE;
                    p_link->lea_srv_found_flags |= LEA_LINK_DISC_BASS_DONE;
                    app_lea_link_init_brs_char(p_link, p_data->brs_char_num);
                }
#endif
            }
        }
        break;

#if BAP_BROADCAST_ASSISTANT
    case LE_AUDIO_MSG_BASS_CLIENT_CP_RESULT:
        {
            T_BASS_CLIENT_CP_RESULT *p_data = (T_BASS_CLIENT_CP_RESULT *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_CLIENT_CP_RESULT: conn_handle 0x%x, cause 0x%x",
                            p_data->conn_handle,
                            p_data->cause);
        }
        break;

    case LE_AUDIO_MSG_BASS_CLIENT_SYNC_INFO_REQ:
        {
            T_BASS_CLIENT_SYNC_INFO_REQ *p_data = (T_BASS_CLIENT_SYNC_INFO_REQ *)buf;

            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_CLIENT_SYNC_INFO_REQ: conn_handle 0x%x, char_instance_id %d",
                            p_data->conn_handle,
                            p_data->char_instance_id);
            p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);
            if (p_link)
            {
                T_BASS_BRS_CHAR_DB *p_brs_char = app_lea_link_find_brs_char(p_link, p_data->char_instance_id);
                if (p_brs_char != NULL && p_brs_char->sync_handle != NULL)
                {
                    if (ble_audio_check_remote_features(p_data->conn_handle, LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX3,
                                                        LE_SUPPORT_FEATURES_PAST_RECIPIENT_MASK_BIT))
                    {
                        if (le_check_supported_features(LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX3,
                                                        LE_SUPPORT_FEATURES_PAST_SENDER_MASK_BIT))
                        {
                            T_BASS_PAST_SRV_DATA srv_data;

                            srv_data.adv_a_match_ext_adv = 0;
                            srv_data.adv_a_match_src = 0;
                            srv_data.source_id = p_data->p_brs_data->source_id;
                            bass_past_by_local_src(p_brs_char->handle, p_data->conn_handle,
                                                   srv_data);
                            bass_past_by_remote_src(p_brs_char->sync_handle, p_data->conn_handle,
                                                    srv_data);
                        }
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_BASS_CLIENT_BRS_DATA:
        {
            T_BASS_CLIENT_BRS_DATA *p_data = (T_BASS_CLIENT_BRS_DATA *)buf;
            APP_PRINT_INFO4("LE_AUDIO_MSG_BASS_CLIENT_BRS_DATA: conn_handle 0x%x, notify %d, read_cause 0x%x, char_instance_id %d",
                            p_data->conn_handle, p_data->notify, p_data->read_cause, p_data->char_instance_id);
            if (p_data->p_brs_data)
            {
                if (p_data->p_brs_data->brs_is_used)
                {
                    APP_PRINT_INFO7("LE_AUDIO_MSG_BASS_CLIENT_BRS_DATA: source_id %d, source_address_type 0x%x, source_address %s, source_adv_sid %d, pa_sync_state %d, bis_sync_state 0x%x, big_encryption %d",
                                    p_data->p_brs_data->source_id,
                                    p_data->p_brs_data->source_address_type,
                                    TRACE_BDADDR(p_data->p_brs_data->source_address),
                                    p_data->p_brs_data->source_adv_sid,
                                    p_data->p_brs_data->pa_sync_state,
                                    p_data->p_brs_data->bis_sync_state,
                                    p_data->p_brs_data->big_encryption);
                    if (p_data->p_brs_data->num_subgroups)
                    {
                        APP_PRINT_INFO4("LE_AUDIO_MSG_BASS_CLIENT_BRS_DATA: num_subgroups %d, bis_info_size %d, bis_sync 0x%x, metadata_len %d",
                                        p_data->p_brs_data->num_subgroups,
                                        p_data->p_brs_data->bis_info_size,
                                        p_data->p_brs_data->p_cp_bis_info[0].bis_sync,
                                        p_data->p_brs_data->p_cp_bis_info[0].metadata_len);
                    }
                    if (p_data->p_brs_data->big_encryption == BIG_BROADCAST_CODE_REQUIRED)
                    {
                        T_BASS_CP_SET_BROADCAST_CODE bc_data;
                        bc_data.source_id = p_data->p_brs_data->source_id;
                        memcpy(bc_data.broadcast_code, brs_broadcast_code, BROADCAST_CODE_LEN);
                        bass_cp_set_broadcast_code(p_data->conn_handle, &bc_data, false);
                    }
                }
                p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);
                if (p_link)
                {
                    app_lea_link_update_brs_char(p_link, p_data);
                }
            }
        }
        break;
#endif
    default:
        break;
    }
    return cb_result;
}

void app_lea_ini_bap_init(void)
{
    T_BAP_ROLE_INFO role_info = {0};

    ble_audio_cback_register(app_lea_ini_bap_handle_msg);
#if BAP_UNICAST_CLIENT
    app_db.bap_role |= BAP_UNICAST_CLT_SRC_ROLE;
    app_db.bap_role |= BAP_UNICAST_CLT_SNK_ROLE;
    role_info.isoc_cig_max_num = APP_ISOC_CIG_MAX_NUM;
    role_info.isoc_cis_max_num = APP_ISOC_CIS_MAX_NUM;
#endif
#if BAP_BROADCAST_ASSISTANT
    app_db.bap_role |= BAP_BROADCAST_ASSISTANT_ROLE;
    role_info.pa_sync_num = APP_MAX_SYNC_HANDLE_NUM;
#endif
#if BAP_BROADCAST_SOURCE
    app_db.bap_role |= BAP_BROADCAST_SOURCE_ROLE;
    role_info.pa_adv_num = APP_MAX_PA_ADV_SET_NUM;
    role_info.isoc_big_broadcaster_num = APP_ISOC_BROADCASTER_MAX_BIG_HANDLE_NUM;
    role_info.isoc_bis_broadcaster_num = APP_ISOC_BROADCASTER_MAX_BIS_NUM;
#endif
    role_info.role_mask = app_db.bap_role;
    role_info.init_gap = true;
    bap_role_init(&role_info);
}
#endif
