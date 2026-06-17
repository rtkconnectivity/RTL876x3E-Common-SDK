/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "string.h"
#include "app_bt_policy_cfg.h"
#include "app_bt_policy_api.h"

const T_APP_BT_POLICY_CFG_CONST app_bt_policy_cfg =
{
    .pin_code[0] = 0,
    .pin_code[1] = 0,
    .pin_code[2] = 0,
    .pin_code[3] = 0,
    .pin_code[4] = 0,
    .pin_code[5] = 0,
    .pin_code[6] = 0,
    .pin_code[7] = 0,
    .pin_code_size = 0,

#if F_APP_MULTILINK_ENABLE
#if F_APP_A2DP_MULTI_SINK_SUPPORT
    .enable_multi_sink_devices = 1,
#endif
#else
    .enable_multi_sink_devices = 0,
#endif
    .max_legacy_multilink_devices = 2,

#if APP_DASHBOARD_DEMO_SUPPORT
    .link_scenario = LINKBACK_SCENARIO_A2DP_BASE,
#else
    .link_scenario = LINKBACK_SCENARIO_HF_A2DP_LAST_DEVICE,
#endif

    .disable_multilink_preemptive = true,
    .maximum_linkback_number = 8,

    .enable_multi_sco_disc_resume = false,
    .enable_not_discoverable_when_linkback = false,
    .enable_multi_outband_call_tone = false,
    .enable_always_discoverable = false,
    .enable_discoverable_in_standby_mode = false,
    .enter_pairing_while_only_one_device_connected = false,

    .timer_linkback_timeout = 30,
    .timer_link_avrcp = 1500,
    .timer_pairing_timeout = 0,
    .timer_link_back_loss = 30,
};

T_APP_BT_POLICY_CFG app_bt_policy_cfg_nv;

void app_bt_policy_cfg_init(void)
{
#if F_APP_MULTILINK_ENABLE
    app_bt_policy_cfg_nv.enable_multi_link = 1;
#endif
}





