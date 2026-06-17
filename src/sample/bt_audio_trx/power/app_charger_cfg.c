/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_charger_cfg.h"

T_APP_CHARGER_CFG app_charger_cfg;

void app_charger_cfg_init(void)
{
    app_charger_cfg.charger_support = 1;
    app_charger_cfg.discharger_support = 1;
    app_charger_cfg.battery_warning_percent = 20;
    app_charger_cfg.timer_low_bat_warning = 0;
    app_charger_cfg.low_bat_warning_count = 0;
    app_charger_cfg.enable_bat_report_when_phone_conn = false;
    app_charger_cfg.enable_new_low_bat_time_unit = 0; //0:sec, 1:min
    app_charger_cfg.enable_report_lower_battery_volume = false;
    app_charger_cfg.enable_bat_report_when_level_drop = false;
    app_charger_cfg.disable_bat_report_when_power_on = false;
}


