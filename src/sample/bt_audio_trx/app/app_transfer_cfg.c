/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "rtl876x.h"
#include "app_transfer_cfg.h"

T_APP_TRANSFER_CFG app_transfer_cfg;

void app_transfer_cfg_init(void)
{
    app_transfer_cfg.enable_embedded_cmd = 1;

#if F_APP_ENABLE_TWO_ONE_WIRE_UART
    app_transfer_cfg.data_uart_baud_rate = BAUD_RATE_115200;
#elif CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_transfer_cfg.data_uart_baud_rate = BAUD_RATE_921600;
#else
    app_transfer_cfg.data_uart_baud_rate = BAUD_RATE_2000000;
#endif
    app_transfer_cfg.dt_resend_num = 20;
#if F_APP_NXP_UWB_CALIBRATION_DATA_DUMP
    app_transfer_cfg.report_uart_event_only_once = true;
#else
    app_transfer_cfg.report_uart_event_only_once = false;
#endif
}


