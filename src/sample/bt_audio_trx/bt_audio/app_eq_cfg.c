/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_eq_cfg.h"

T_APP_EQ_CFG app_eq_cfg;

void app_eq_cfg_init(void)
{
    app_eq_cfg.user_eq_spk_eq_num = 10;
    app_eq_cfg.user_eq_mic_eq_num = 10;
}


