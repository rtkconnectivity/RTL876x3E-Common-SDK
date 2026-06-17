/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_avrcp_cfg.h"
#include "app_bt_policy_cfg.h"

T_APP_AVRCP_CFG app_avrcp_cfg;

void app_avrcp_cfg_init(void)
{
    if (app_bt_policy_cfg_nv.enable_multi_link)
    {
        app_avrcp_cfg.avrcp_link_number = 2;
    }
    else
    {
        app_avrcp_cfg.avrcp_link_number = 1;
    }
    app_avrcp_cfg.enable_av_fwd_bwd_only_when_playing = false;
    app_avrcp_cfg.disallow_adjust_volume_when_idle = false;

}


