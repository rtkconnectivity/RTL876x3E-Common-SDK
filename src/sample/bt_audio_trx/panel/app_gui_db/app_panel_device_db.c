/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "string.h"
#include "trace.h"
#include "app_panel_db_common.h"
#include "app_panel_device_db.h"
#include "app_panel_le_db.h"
#include "app_charger.h"

static T_APP_GUI_DEVICE_DATA app_gui_device_info = {0};

void app_panel_device_db_set_device_name(uint8_t *device_name, uint8_t length)
{
    memcpy(app_gui_device_info.device_name, device_name,
           length < APP_GUI_DEVICE_NAME_MAX_LENGTH ? length : APP_GUI_DEVICE_NAME_MAX_LENGTH);
}

char *app_panel_device_db_get_device_name(void)
{
    return app_gui_device_info.device_name;
}

void app_panel_device_db_set_bettery_level(uint8_t level)
{
    APP_PRINT_INFO1("app_panel_device_db_set_bettery_level: level %d", level);
    app_gui_device_info.battery_level = level;

#if F_GUI_CHARGEBOX_DEMO
    uint8_t remote_battery[2] = {0};
    bool result = false;
    result = app_panel_le_db_get_bettery_level(remote_battery);
    if (result)
    {
        app_tb_set_battery(app_gui_device_info.battery_level, remote_battery[0], remote_battery[1]);
    }
    else
    {
        app_tb_set_battery(app_gui_device_info.battery_level, BATTERY_INVALIE_VALUE, BATTERY_INVALIE_VALUE);
    }
#endif
}

uint8_t app_panel_device_db_get_bettery_level(void)
{
    return app_gui_device_info.battery_level;
}

void app_panel_device_db_init(void)
{
    app_gui_device_info.battery_level = app_charger_get_soc();
}

#endif


/*-----------------------------------------------------------*/
