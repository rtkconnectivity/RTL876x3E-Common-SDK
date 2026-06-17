/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_key_cfg.h"
#include "rtl876x.h"
#include "app_mmi.h"
#include "string.h"
#include "board.h"

T_APP_KEY_CFG app_key_cfg;

enum key_table_click
{
    SHORT_CLICK = 0,
    LONG_PRESS = 1,
};

enum key_table_call_status
{
    CALL_IDLE,
    VOICE_DIAL,
    INCOMING_CALL,
    OUT_GOING_CALL,
    CALL_ACTIVE,
    CALL_ACTIVE_WITH_CALL_WAITING,
    CALL_ACTIVE_WITH_CALL_HOLD,
    MULTI_CALL_ACTIVE_WITH_CALL_WATING,
    MULTI_CALL_ACTIVE_WITH_CALL_HOLD,
};

enum key_table_key_idx
{
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
};

enum key_table_hybrid_key_idx
{
    HYBRID_TYPE_0,
    HYBRID_TYPE_1,
    HYBRID_TYPE_2,
    HYBRID_TYPE_3,
    HYBRID_TYPE_4,
    HYBRID_TYPE_5,
    HYBRID_TYPE_6,
    HYBRID_TYPE_7,
};

enum key_table_hybrid_key_mapping
{
    HYBRID_MASK,
    HYBRID_SCENARIO,
};

void app_key_cfg_init(void)
{
#if (F_APP_BT_AUDIO_TRANSMITTER_DEMO_SUPPORT || F_APP_CHARGE_CASE_DEMO_SUPPORT || F_GUI_BENCHMARK_SUPPORT)
    app_key_cfg.key_gpio_support = 1;
    app_key_cfg.mfb_replace_key0 = 1;
#else
    app_key_cfg.key_gpio_support = 0;
    app_key_cfg.mfb_replace_key0 = 0;
#endif
    app_key_cfg.key_disable_power_on_off = 0;

    // interval related to GPIO Key
    app_key_cfg.key_multi_click_interval = 3;
    app_key_cfg.key_long_press_interval = 20;

    // interval related to MFB
    app_key_cfg.key_power_on_interval = 25;
    app_key_cfg.key_power_off_interval = 30;
    app_key_cfg.key_enter_pairing_interval = 60;
    app_key_cfg.key_factory_reset_interval = 90;

    // HW settings
    uint8_t key_pinmux[8] = {PIN_KEY0, PIN_KEY1, PIN_KEY2, PIN_KEY3, PIN_KEY4, PIN_KEY5, PIN_KEY6, PIN_KEY7};
    memcpy(app_key_cfg.key_pinmux, key_pinmux, sizeof(key_pinmux));
#if F_APP_BT_AUDIO_TRANSMITTER_DEMO_SUPPORT
    app_key_cfg.key_enable_mask = KEY1_MASK;
#endif

#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
    app_key_cfg.key_high_active_mask = KEY1_MASK | KEY2_MASK | KEY3_MASK | KEY4_MASK | KEY5_MASK |
                                       KEY6_MASK | KEY7_MASK;
#else
    app_key_cfg.key_high_active_mask = KEY2_MASK | KEY4_MASK | KEY6_MASK;
#endif

    // long repeat
    app_key_cfg.key_long_press_repeat_mask = KEY_NULL;
    app_key_cfg.key_long_press_repeat_interval = 4;

    if (app_key_cfg.mfb_replace_key0)
    {
        app_key_cfg.key_enable_mask &= 0xfe;
        app_key_cfg.key_pinmux[0] = 0xff;
    }

    // Samples
    // sample of single key short click
#if F_GUI_BENCHMARK_SUPPORT
    app_key_cfg.key_table[SHORT_CLICK][CALL_IDLE][KEY_0] = MMI_GUI_BENCHMARK_START;
    app_key_cfg.key_table[SHORT_CLICK][INCOMING_CALL][KEY_0] = MMI_GUI_BENCHMARK_START;
    app_key_cfg.key_table[SHORT_CLICK][OUT_GOING_CALL][KEY_0] = MMI_GUI_BENCHMARK_START;
    app_key_cfg.key_table[SHORT_CLICK][CALL_ACTIVE][KEY_0] = MMI_GUI_BENCHMARK_START;
#endif
    app_key_cfg.key_table[SHORT_CLICK][CALL_IDLE][KEY_1] = MMI_DEV_MIC_VOL_UP;

    // sample of single key long press
    app_key_cfg.key_table[LONG_PRESS][CALL_IDLE][KEY_1] = MMI_DEV_MIC_VOL_DOWN;

    // sample of double click
    app_key_cfg.hybrid_key_table[CALL_IDLE][HYBRID_TYPE_0] = MMI_DEV_MIC_VOL_DOWN;
    app_key_cfg.hybrid_key_mapping[HYBRID_TYPE_0][HYBRID_MASK] = KEY1_MASK;
    app_key_cfg.hybrid_key_mapping[HYBRID_TYPE_0][HYBRID_SCENARIO] = HYBRID_KEY_2_CLICK;

    // sample of triple click
    app_key_cfg.hybrid_key_table[CALL_IDLE][HYBRID_TYPE_1] = MMI_START_ROLESWAP;
    app_key_cfg.hybrid_key_mapping[HYBRID_TYPE_1][HYBRID_MASK] = KEY1_MASK;
    app_key_cfg.hybrid_key_mapping[HYBRID_TYPE_1][HYBRID_SCENARIO] = HYBRID_KEY_3_CLICK;

    // sample of combine key single click
    app_key_cfg.hybrid_key_table[CALL_IDLE][HYBRID_TYPE_2] = MMI_DEV_MIC_VOL_DOWN;
    app_key_cfg.hybrid_key_mapping[HYBRID_TYPE_2][HYBRID_MASK] = KEY1_MASK | KEY2_MASK;
    app_key_cfg.hybrid_key_mapping[HYBRID_TYPE_2][HYBRID_SCENARIO] = HYBRID_KEY_SHORT_PRESS;

}


