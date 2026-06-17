/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_AMP_SUPPORT
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "app_timer.h"
#include "hal_gpio.h"
#include "app_amp.h"
#include "audio_plugin.h"
#include "app_flags.h"
#include "app_cfg.h"
#if F_APP_CUSTOMER_EXT_PA_SUPPORT_AW87390
#include "aw87390_driver_interface.h"
#endif

typedef struct
{
    //AMP timer
    uint8_t amp_post_on_guard_time;
    uint8_t amp_pre_on_guard_time;
    uint8_t amp_off_guard_time;

    uint8_t timer_ctrl_ext_amp_ready;
    uint8_t timer_ctrl_ext_amp_on;
    uint8_t timer_ctrl_ext_amp_off;

} T_APP_AMP_CFG_CONST;

T_APP_AMP_CFG_CONST app_amp_cfg_const;

typedef enum t_app_amp_state
{
    APP_AMP_STATE_OFF = 0,
    APP_AMP_STATE_WAIT_OFF,
    APP_AMP_STATE_WAIT_ON,
    APP_AMP_STATE_ON,
    APP_AMP_STATE_NUM,
} T_APP_AMP_STATE;

typedef enum t_app_amp_timer
{
    APP_TIMER_AMP_PRE_ON_GUARD_TIME = 0,
    APP_TIMER_AMP_POST_ON_GUARD_TIME,
    APP_TIMER_AMP_OFF_GUARD_TIME,
    APP_TIMER_AMP_NUM,
} T_APP_AMP_TIMER;

typedef struct t_app_amp_db
{
    T_APP_AMP_STATE             state;
    uint8_t                     ref_cnt;
    T_AUDIO_PLUGIN_HANDLE       plugin_handle;
    void                       *poweron_context;
    void                       *poweroff_context;
} T_APP_AMP_DB;

static T_APP_AMP_DB app_amp_db = { .state = APP_AMP_STATE_OFF, 0 };

static uint8_t timer_idx_amp = 0;
static uint8_t amp_timer_id = 0;

static void app_amp_send_to_plugin(void *context)
{
    APP_PRINT_TRACE1("app_amp_send_to_plugin: context %p", context);
    audio_plugin_msg_send(app_amp_db.plugin_handle, context);
}

static void app_amp_control(uint8_t activate_fg)
{
    APP_PRINT_INFO1("app_amp_control, activate_fg: 0x%x", activate_fg);

#if F_APP_CUSTOMER_EXT_PA_SUPPORT_AW87390
    if (activate_fg)
    {
//        app_dlps_disable(APP_DLPS_ENTER_CHECK_EXT_PA);
        aw87390_start(0);
    }
    else //Turn off
    {
        aw87390_stop();
//        app_dlps_enable(APP_DLPS_ENTER_CHECK_EXT_PA);
    }
#else
    T_GPIO_LEVEL level = GPIO_LEVEL_LOW;

    if (activate_fg)
    {
        if (app_cfg_const.enable_ext_amp_high_active)
        {
            level = GPIO_LEVEL_HIGH;
        }
    }
    else //Turn off
    {
        if (app_cfg_const.enable_ext_amp_high_active == 0)
        {
            level = GPIO_LEVEL_HIGH;
        }
    }
    hal_gpio_set_level(app_cfg_const.ext_amp_pinmux, level);
#endif
}

static void app_amp_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_amp_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    app_stop_timer(&timer_idx_amp);
    switch (timer_evt)
    {
    case APP_TIMER_AMP_PRE_ON_GUARD_TIME:
        {
            app_amp_control(1);
            app_start_timer(&timer_idx_amp, "amp_post_on_guard",
                            amp_timer_id, APP_TIMER_AMP_POST_ON_GUARD_TIME, 0, false,
                            app_amp_cfg_const.amp_post_on_guard_time * 100);
        }
        break;

    case APP_TIMER_AMP_POST_ON_GUARD_TIME:
        {
            app_amp_db.state = APP_AMP_STATE_ON; /* amp is ready to run */
            app_amp_send_to_plugin(app_amp_db.poweron_context);
        }
        break;

    case APP_TIMER_AMP_OFF_GUARD_TIME:
        {
            app_amp_db.state = APP_AMP_STATE_OFF;
            app_amp_send_to_plugin(app_amp_db.poweroff_context);
        }
        break;

    default:
        break;
    }
}

static void app_amp_poweron(T_AUDIO_PLUGIN_HANDLE handle, T_AUDIO_PLUGIN_PARAM param, void *context)
{
    /* Action description: Check whether amp is enabled. If so, inform back to audio path immediately.
                           Otherwise, start the ENABLE procedure and inform back when
                           amp becomes enabled. */
    app_amp_db.ref_cnt++;
    if (app_amp_db.state == APP_AMP_STATE_ON)
    {
        app_amp_send_to_plugin(context);
    }
    else
    {
        app_stop_timer(&timer_idx_amp);
        app_amp_db.state = APP_AMP_STATE_WAIT_ON;
        app_amp_db.poweron_context = context;
        app_start_timer(&timer_idx_amp, "amp_pre_on_guard",
                        amp_timer_id, APP_TIMER_AMP_PRE_ON_GUARD_TIME, 0, false,
                        app_amp_cfg_const.amp_pre_on_guard_time * 100);
    }
}

static void app_amp_run(T_AUDIO_PLUGIN_HANDLE handle, T_AUDIO_PLUGIN_PARAM param, void *context)
{
    /* Action description: Inform back to audio path when amp is enabled. */
    if (app_amp_db.state == APP_AMP_STATE_ON)
    {
        app_amp_send_to_plugin(context);
    }
}

static void app_amp_poweroff(T_AUDIO_PLUGIN_HANDLE handle, T_AUDIO_PLUGIN_PARAM param,
                             void *context)
{
    /* Action description: If the condition to power off is satisfied, start the POWEROFF procedure
                           and inform back when the procedure is done. Otherwise, inform back immediatelly.
    */
    if (app_amp_db.ref_cnt)
    {
        app_amp_db.ref_cnt--;
        if (app_amp_db.ref_cnt == 0)
        {
            app_amp_control(0);
            app_stop_timer(&timer_idx_amp);
            app_amp_db.state = APP_AMP_STATE_WAIT_OFF;
            app_amp_db.poweroff_context = context;
            app_start_timer(&timer_idx_amp, "amp_off_guard",
                            amp_timer_id, APP_TIMER_AMP_OFF_GUARD_TIME, 0, false,
                            app_amp_cfg_const.amp_off_guard_time * 100);
        }
        else
        {
            app_amp_send_to_plugin(context);
        }
    }
}

static const T_AUDIO_PLUGIN_POLICY app_amp_policies[] =
{
    /* category */          /* occasion */                           /* action handler */
    { AUDIO_CATEGORY_AUDIO, AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON, app_amp_poweron },
    { AUDIO_CATEGORY_VOICE, AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON, app_amp_poweron },
    { AUDIO_CATEGORY_ANALOG, AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON, app_amp_poweron },
    { AUDIO_CATEGORY_TONE, AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON, app_amp_poweron },
    { AUDIO_CATEGORY_VP, AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON, app_amp_poweron },
    { AUDIO_CATEGORY_APT, AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON, app_amp_poweron },
    { AUDIO_CATEGORY_LLAPT, AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON, app_amp_poweron },
    { AUDIO_CATEGORY_ANC, AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON, app_amp_poweron },

    { AUDIO_CATEGORY_AUDIO, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON, app_amp_run },
    { AUDIO_CATEGORY_VOICE, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON, app_amp_run },
    { AUDIO_CATEGORY_ANALOG, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON, app_amp_run },
    { AUDIO_CATEGORY_TONE, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON, app_amp_run },
    { AUDIO_CATEGORY_VP, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON, app_amp_run },
    { AUDIO_CATEGORY_APT, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON, app_amp_run },
    { AUDIO_CATEGORY_LLAPT, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON, app_amp_run },
    { AUDIO_CATEGORY_ANC, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON, app_amp_run },

    { AUDIO_CATEGORY_AUDIO, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF, app_amp_poweroff },
    { AUDIO_CATEGORY_VOICE, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF, app_amp_poweroff },
    { AUDIO_CATEGORY_ANALOG, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF, app_amp_poweroff },
    { AUDIO_CATEGORY_TONE, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF, app_amp_poweroff },
    { AUDIO_CATEGORY_VP, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF, app_amp_poweroff },
    { AUDIO_CATEGORY_APT, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF, app_amp_poweroff },
    { AUDIO_CATEGORY_LLAPT, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF, app_amp_poweroff },
    { AUDIO_CATEGORY_ANC, AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF, app_amp_poweroff },
};

bool app_amp_is_off_state(void)
{
    if (app_amp_db.state == APP_AMP_STATE_OFF)
    {
        return true;
    }
    return false;
}

void app_amp_cfg_init(void)
{
    app_amp_cfg_const.amp_post_on_guard_time = 1;
    app_amp_cfg_const.amp_pre_on_guard_time = 1;
    app_amp_cfg_const.amp_off_guard_time = 1;

#if F_APP_CUSTOMER_EXT_PA_SUPPORT_AW87390
    app_amp_cfg_const.timer_ctrl_ext_amp_on = 1; // *100ms code on to amp_en timer
    app_amp_cfg_const.timer_ctrl_ext_amp_ready = 1; // *100ms amp_en to stream out timer
    app_amp_cfg_const.timer_ctrl_ext_amp_off = 1; // *100ms amp_disable to codec off timer
#endif

}

bool app_amp_init(void)
{
    int32_t ret = 0;

#if F_APP_CUSTOMER_EXT_PA_SUPPORT_AW87390
    aw87390_driver_init();
#else
    hal_gpio_init_pin(app_cfg_const.ext_amp_pinmux, GPIO_TYPE_AON, GPIO_DIR_OUTPUT, GPIO_PULL_NONE);
    if (app_cfg_const.enable_ext_amp_high_active)
    {
        hal_gpio_set_level(app_cfg_const.ext_amp_pinmux, GPIO_LEVEL_LOW);
    }
    else
    {
        hal_gpio_set_level(app_cfg_const.ext_amp_pinmux, GPIO_LEVEL_HIGH);
    }
#endif

    if (app_timer_reg_cb(app_amp_timeout_cb, &amp_timer_id) != 0)
    {
        ret = 1;
        goto fail_reg_amp_timer;
    }

    app_amp_db.plugin_handle = audio_plugin_create(app_amp_policies,
                                                   sizeof(app_amp_policies) / sizeof(app_amp_policies[0]));
    if (app_amp_db.plugin_handle == NULL)
    {
        ret = 2;
        goto fail_create_plugin;
    }

    return true;

fail_create_plugin:
fail_reg_amp_timer:
    APP_PRINT_TRACE1("app_amp_init: failed %d", -ret);
    return false;
}
#endif
