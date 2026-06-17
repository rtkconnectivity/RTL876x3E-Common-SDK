/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "trace.h"
#include "app_panel_le_db.h"
#include "app_panel_device_db.h"
#include "app_panel_db_common.h"
#if F_GUI_CHARGEBOX_DEMO
#include "app_chargebox.h"
#endif

static T_APP_GUI_LE_DATA app_gui_le_data = {0};
static uint8_t active_bd_addr[6] = {0};

T_APP_GUI_LE_LINK_DATA *app_panel_le_find_active_le_link(void)
{
    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_gui_le_data.gui_le_link_data[i].active == true)
        {
            return &app_gui_le_data.gui_le_link_data[i];
        }
    }
    return NULL;
}

T_APP_GUI_LE_LINK_DATA *app_panel_le_find_le_link(uint8_t *bd_addr)
{
    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if ((app_gui_le_data.gui_le_link_data[i].active == true) &&
            (memcmp(app_gui_le_data.gui_le_link_data[i].bd_addr, bd_addr, 6) == 0))
        {
            return &app_gui_le_data.gui_le_link_data[i];
        }
    }
    return NULL;
}

void app_panel_le_alloc_link(uint8_t *bd_addr)
{
    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_gui_le_data.gui_le_link_data[i].active == false)
        {
            app_gui_le_data.gui_le_link_data[i].active = true;
            memcpy(app_gui_le_data.gui_le_link_data[i].bd_addr, bd_addr, 6);
            app_panel_le_check_awake_gui();
            APP_PRINT_INFO2("app_panel_le_alloc_link: i %d, bd_addr %s", i, TRACE_BDADDR(bd_addr));
            return;
        }
    }
}

void app_panel_le_free_link(uint8_t *bd_addr)
{
    T_APP_GUI_LE_LINK_DATA *gui_le_link = NULL;

    gui_le_link = app_panel_le_find_le_link(bd_addr);
    APP_PRINT_INFO2("app_panel_le_free_link: bd_addr %s, gui_le_link %p", TRACE_BDADDR(bd_addr),
                    gui_le_link);
    if (gui_le_link)
    {
        memset((uint8_t *)gui_le_link, 0, sizeof(T_APP_GUI_LE_LINK_DATA));
    }

#if F_GUI_CHARGEBOX_DEMO
    app_tb_set_bt_volume_value(bd_addr, 0xff);
#endif
    app_panel_le_check_awake_gui();
}

void app_panel_le_check_awake_gui(void)
{
    uint8_t temp_addr[6] = {0};
    bool update_flag = false;

    T_APP_GUI_LE_LINK_DATA *le_link = app_panel_le_find_active_le_link();

    APP_PRINT_INFO2("app_panel_le_check_awake_gui: le_link %p, active_bd_addr %s", le_link,
                    TRACE_BDADDR(active_bd_addr));

    if ((memcmp(active_bd_addr, temp_addr, 6) == 0) && le_link)
    {
        memcpy(active_bd_addr, le_link->bd_addr, 6);
        update_flag = true;
    }
    else if ((memcmp(active_bd_addr, temp_addr, 6) != 0) && !le_link)
    {
        memset(active_bd_addr, 0, sizeof(active_bd_addr));
        update_flag = true;
    }
    else if (le_link && (memcmp(le_link->bd_addr, active_bd_addr, 6) != 0))
    {
        memcpy(active_bd_addr, le_link->bd_addr, 6);
        update_flag = true;
    }

    APP_PRINT_INFO1("app_panel_le_check_awake_gui: update %d", update_flag);

    if (update_flag)
    {
        app_panel_db_common_awake_gui_task();
    }
}

void app_panel_le_db_set_bettery_level(uint8_t *bd_addr, uint8_t *level)
{
    T_APP_GUI_LE_LINK_DATA *gui_le_link = app_panel_le_find_le_link(bd_addr);
    if (gui_le_link)
    {
        memcpy(gui_le_link->battery_level, level, sizeof(gui_le_link->battery_level));
    }
#if F_GUI_CHARGEBOX_DEMO
    app_tb_set_battery(app_panel_device_db_get_bettery_level(), level[0], level[1]);
#endif
}

bool app_panel_le_db_get_bettery_level(uint8_t *level)
{
    T_APP_GUI_LE_LINK_DATA *gui_le_link = app_panel_le_find_active_le_link();
    if (gui_le_link)
    {
        memcpy(level, gui_le_link->battery_level, sizeof(gui_le_link->battery_level));
    }
    else
    {
        return false;
    }
    return true;
}

void app_panel_le_db_set_bud_side(uint8_t *bd_addr, T_APP_GUI_LE_BUD_SIDE side)
{
    T_APP_GUI_LE_LINK_DATA *gui_le_link = app_panel_le_find_le_link(bd_addr);
    if (gui_le_link)
    {
        gui_le_link->bud_side = side;
    }
}

void app_panel_le_db_set_volume(uint8_t *bd_addr, uint8_t volume_value)
{
    T_APP_GUI_LE_LINK_DATA *gui_le_link = app_panel_le_find_le_link(bd_addr);
    if (gui_le_link)
    {
        gui_le_link->volume = volume_value;
#if F_GUI_CHARGEBOX_DEMO
        if (volume_value <= 127)
        {
            app_tb_set_bt_volume_value(bd_addr, volume_value * 100 / 127);
        }
#endif
    }
}

void app_panel_le_db_set_anc_status(uint8_t *bd_addr, uint8_t anc_status)
{
    T_APP_GUI_LE_LINK_DATA *gui_le_link = app_panel_le_find_le_link(bd_addr);
    if (gui_le_link)
    {
        gui_le_link->anc_status = (T_APP_GUI_LISTENING_MODE)anc_status;
#if F_GUI_CHARGEBOX_DEMO
        app_tb_set_bt_anc_status(bd_addr, (T_APP_GUI_LISTENING_MODE)anc_status);
#endif
    }
}

void app_panel_le_db_set_remote_name(uint8_t *bd_addr, uint8_t *name, uint8_t length)
{
    T_APP_GUI_LE_LINK_DATA *gui_le_link = app_panel_le_find_le_link(bd_addr);
    if (gui_le_link)
    {
        memcpy(gui_le_link->remote_device_name, name, length < 40 ? length : 40);
#if F_GUI_CHARGEBOX_DEMO
        app_tb_set_device_name(gui_le_link->remote_device_name, length < 40 ? length : 40);
#endif
    }
}

void app_panel_le_db_set_remote_link_status(uint8_t *bd_addr, uint8_t status)
{
    T_APP_GUI_LE_LINK_DATA *gui_le_link = app_panel_le_find_le_link(bd_addr);
    if (gui_le_link)
    {
        gui_le_link->remote_device_link_status = (T_APP_GUI_LE_LINK_STATUS)status;
#if F_GUI_CHARGEBOX_DEMO
        app_tb_set_bt_link_status(bd_addr, (T_APP_GUI_LE_LINK_STATUS)status);
#endif
    }
}

void app_panel_le_db_add_bass_scan_dev(T_APP_GUI_LE_BASS_SCAN_RSP_DATA *scan_info)
{
#if F_GUI_CHARGEBOX_DEMO
    if (scan_info->index < BASS_SCAN_DEV_NUM_MAX)
    {
        app_gui_le_data.dev_list_num = scan_info->index + 1;
        app_tb_add_scan_dev(scan_info->index, scan_info->device_name);
    }
    else
    {
        APP_PRINT_ERROR0("app_panel_le_db_add_bass_scan_dev: dev num reach max num");
    }
#endif
}

#endif


/*-----------------------------------------------------------*/
