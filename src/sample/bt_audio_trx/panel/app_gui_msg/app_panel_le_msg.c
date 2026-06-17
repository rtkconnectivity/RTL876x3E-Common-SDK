/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "trace.h"
#include "app_main.h"
#include "app_panel_le_msg.h"
#include "app_le_transfer.h"
#include "app_lea_ini_scan.h"
#include "app_lea_ini_cap.h"
#include "app_lea_ini_bap.h"

#if BAP_BROADCAST_ASSISTANT
T_APP_LEA_BAAS_SCAN_DEV *pa_sync_dev = NULL;
T_APP_LEA_GROUP_INFO *p_select_dev_group_info = NULL;
T_APP_LEA_SYNC_INFO *p_select_dev_sync_info = NULL;

bool app_panel_le_handle_pa_sync(uint16_t idx)
{
    T_APP_LEA_BAAS_SCAN_DEV *dev = NULL;
    dev = (T_APP_LEA_BAAS_SCAN_DEV *)os_queue_peek(
              &app_db.scan_baas_dev_queue,
              idx);

    if (dev)
    {
        APP_PRINT_INFO3("app_panel_le_handle_pa_sync: idx %d, dev addr  %s, dev point %p", idx,
                        TRACE_BDADDR(dev->dev_info.bd_addr), dev);
        if (dev != pa_sync_dev)
        {
            if (pa_sync_dev && p_select_dev_group_info && p_select_dev_sync_info)
            {
                app_lea_com_bst_reception_stop(p_select_dev_group_info->group_handle,
                                               p_select_dev_sync_info->sync_handle);
            }
            pa_sync_dev = dev;
            app_lea_pa_sync_by_dev_info(&pa_sync_dev->dev_info);
        }
        else
        {
        }
        return true;
    }
    else
    {
        APP_PRINT_ERROR1("app_panel_le_handle_pa_sync: no scan device found, scan queue num %d",
                         app_db.scan_baas_dev_queue.count);
        return false;
    }
}

T_APP_LEA_SYNC_INFO *app_panel_le_find_sync_handle_info(uint8_t *bd_addr)
{
    T_APP_LEA_SYNC_INFO *p_sync_info = NULL;
    for (uint8_t i = 0; i < app_db.sync_handle_queue.count; i++)
    {
        p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue, i);
        if (p_sync_info && (memcmp(p_sync_info->dev_info.bd_addr, bd_addr, 6) == 0))
        {
            APP_PRINT_INFO1("app_panel_le_find_sync_handle_info: sync queue dev bd_addr %s",
                            TRACE_BDADDR(p_sync_info->dev_info.bd_addr));
            return p_sync_info;
        }
    }

    return NULL;
}

void app_panel_le_handle_bst_start(void)
{
    int8_t error_code = 0;
    T_APP_LEA_SYNC_INFO *sync_handle_info = NULL;

    if (app_db.group_handle_queue.count == 0)
    {
        p_select_dev_group_info = NULL;
        error_code = -1;
        goto FAILED;
    }
    p_select_dev_group_info = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                    0);
    APP_PRINT_INFO2("app_panel_le_handle_bst_start: group_handle %p, pa_sync_dev %p",
                    p_select_dev_group_info->group_handle, pa_sync_dev);
    if (pa_sync_dev)
    {
        sync_handle_info = app_panel_le_find_sync_handle_info(pa_sync_dev->dev_info.bd_addr);
        if (sync_handle_info)
        {
            p_select_dev_sync_info = sync_handle_info;
            app_lea_com_bst_reception_start(p_select_dev_group_info->group_handle,
                                            p_select_dev_sync_info->sync_handle);
        }
        else
        {
            error_code = -2;
            goto FAILED;
        }
    }
    else
    {
        error_code = -3;
        goto FAILED;
    }
    return;
FAILED:
    APP_PRINT_ERROR1("app_panel_le_handle_bst_start: error code %d", error_code);
}

void app_panel_le_handle_bst_stop(void)
{
    if (p_select_dev_group_info && p_select_dev_sync_info)
    {
        app_lea_com_bst_reception_stop(p_select_dev_group_info->group_handle,
                                       p_select_dev_sync_info->sync_handle);
    }
    else
    {
        APP_PRINT_ERROR2("app_panel_le_handle_bst_stop: p_select_dev_group_info %p, p_select_dev_sync_info %p",
                         p_select_dev_group_info, p_select_dev_sync_info);
    }
}
#endif

void app_panel_le_msg_handle(T_APP_GUI_MSG *data)
{
    APP_PRINT_INFO2("app_panel_le_msg_handle: subtype %d, param %x", data->subtype, data->u.param);
    switch (data->subtype)
    {
    case GUI_LE_SUBEVENT_ADJUST_VOLUME:
        if (data->u.param == GUI_ADJUST_VOLUME_UP)
        {
#if TRANSMIT_CLIENT_SUPPORT
            app_le_transfer_volume_control(LE_TRANSFER_CONTROL_VOLUME_UP);
#endif
        }
        else
        {
#if TRANSMIT_CLIENT_SUPPORT
            app_le_transfer_volume_control(LE_TRANSFER_CONTROL_VOLUME_DOWN);
#endif
        }
        break;
#if BAP_BROADCAST_ASSISTANT
    case GUI_LE_SUBEVENT_BASS_SCAN:
        if (data->u.param == GUI_BASS_SCAN_START)
        {
            app_lea_baas_scan_start();
        }
        else
        {
            app_lea_baas_scan_stop();
            app_lea_clear_baas_scan_dev();
        }
        break;
    case GUI_LE_SUBEVENT_PA_SYNC:
        app_panel_le_handle_pa_sync((uint16_t)data->u.param);
        break;
    case GUI_LE_SUBEVENT_BST_RECEPTION:
        if (data->u.param == GUI_LE_BST_RECEPTION_START)
        {
            app_panel_le_handle_bst_start();
        }
        else
        {
            app_panel_le_handle_bst_stop();
        }
        break;
#endif
    default:
        break;
    }
}

#endif


/*-----------------------------------------------------------*/
