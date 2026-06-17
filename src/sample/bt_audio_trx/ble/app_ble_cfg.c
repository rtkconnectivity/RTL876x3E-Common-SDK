/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_ble_cfg.h"

T_APP_BLE_CFG app_ble_cfg;

void app_ble_cfg_init(void)
{
    app_ble_cfg.rtk_app_adv_support = 1;
    app_ble_cfg.timer_ota_adv_timeout = 0; //unit is 10ms, 0 - always advertising
}


