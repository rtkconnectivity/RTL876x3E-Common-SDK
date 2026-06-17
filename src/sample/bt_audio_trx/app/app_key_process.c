/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "trace.h"
#include "board.h"
#include "ftl.h"
#include "gap_vendor.h"
#include "remote.h"
#include "single_tone.h"
#include "app_key_process.h"
#include "app_charger.h"
#include "app_mmi.h"
#include "app_main.h"
#include "app_hfp.h"
#include "app_dlps.h"
#include "app_timer.h"
#include "app_auto_power_off.h"
#include "app_cmd.h"
#include "app_bt_policy_api.h"
#include "app_key_cfg.h"
#include "app_charger_cfg.h"


#define LONG_PRESS                          1
#define LONG_PRESS_POWER_ON                 2
#define LONG_PRESS_POWER_OFF                3
#define LONG_PRESS_ENTER_PAIRING            4
#define LONG_PRESS_FACTORY_RESET            5
#define LONG_PRESS_REPEAT                   6

#define KEY_TIMER_UNIT_MS                   100

typedef enum t_app_key_timer
{
    APP_TIMER_KEY_MULTI_CLICK,
    APP_TIMER_KEY_LONG_PRESS,
    APP_TIMER_KEY_LONG_PRESS_REPEAT,
    APP_TIMER_KEY_POWER_ON_LONG_PRESS,
    APP_TIMER_KEY_POWER_OFF_LONG_PRESS,
    APP_TIMER_KEY_ENTER_PAIRING,
    APP_TIMER_KEY_FACTORY_RESET,
} T_APP_KEY_TIMER;

/* Key data for record key various status */
typedef struct t_key_data
{
    uint8_t         mfb_key;                /* MFB key (power on)*/
    uint8_t         pre_key;                /* previous key value */
    uint8_t         combine_key_bits;       /* record current hybrid press key value*/
    uint8_t         pre_combine_key_bits;   /* record previous hybrid press key value*/
    uint8_t         key_bit_mask;           /* key mask */
    uint8_t         key_long_pressed;       /* key long press category*/
    uint8_t         key_detected_num;       /* detecte key pressed num*/
    uint8_t         key_click;              /* click key num */
    uint8_t         key_action;             /* action of key value corresponding*/
    uint8_t         key_enter_pairing;      /* long press enter pairing*/
    uint8_t         key_bit_mask_airplane;  /* key mask of airplane mode combine key*/
} T_KEY_DATA;

/* Key check parameters */
typedef struct t_key_check
{
    uint8_t         key;                /* key Mask */
    uint8_t         key_pressed;        /* key action KEY_RELEASE or  KEY_PRESS*/
} T_KEY_CHECK;

static uint8_t key_timer_id = 0;
static uint8_t timer_idx_key_multi_click = 0;
static uint8_t timer_idx_key_long_press = 0;
static uint8_t timer_idx_key_long_press_repeat = 0;
static uint8_t timer_idx_key_power_on_long_press = 0;
static uint8_t timer_idx_key_power_off_long_press = 0;
static uint8_t timer_idx_key_enter_pairing = 0;
static uint8_t timer_idx_key_factory_reset = 0;

static T_KEY_DATA key_data = {0}; /**<record key variable */
static bool long_key_power_off_pressed = false;

/*  Clear key parameters.
 * After perform the corresponding action,app will call this function to clear key parameters.
 */
static void app_key_clear(void)
{
    key_data.pre_key = KEY_NULL;
    key_data.key_click = 0;
    key_data.pre_combine_key_bits = 0;
}

/*
 * Find out the key index by key mask.
 * For example: The key index 0 correspond to key0, such as KEY0_MASK, KEY1_MASK etc.
 */
uint8_t app_key_search_index(uint8_t key)
{
    uint8_t key_index = __builtin_ctz(key);
    key_index = (key_index >= MAX_KEY_NUM) ? 0 : key_index;
    return key_index;
}

/**
    * @brief  Depending on the current connection state, single or multi-spk mode,
    *         app hanldes the corresponding procedures.
    * @param  action The MMI action to be executed.
    * @return void
    */
void app_key_execute_action(uint8_t action)
{
    APP_PRINT_INFO1("app_key_execute_action: action = 0x%x, ", action);

    if ((action == MMI_DEV_FACTORY_RESET) || (action == MMI_DEV_PHONE_RECORD_RESET))
    {
        if (app_mmi_is_allow_factory_reset() == false)
        {
            return;
        }
    }

    app_mmi_handle_action(action);
}

/**
    * @brief  Check if current pressed key has multi-click behavior.
    * @param  key  Current pressed key
    * @return True if there's multi-click behavior definition for this key. False otherwise.
    */
static bool app_key_multi_click_match(uint8_t key)
{
    bool ret = false;
    uint8_t i;

    for (i = 0; i < MAX_KEY_NUM; i++)
    {
        if (app_key_cfg.hybrid_key_mapping[i][0] == key)
        {
            if ((app_key_cfg.hybrid_key_mapping[i][1] == HYBRID_KEY_2_CLICK) ||
                (app_key_cfg.hybrid_key_mapping[i][1] == HYBRID_KEY_3_CLICK) ||
                (app_key_cfg.hybrid_key_mapping[i][1] == HYBRID_KEY_4_CLICK) ||
                (app_key_cfg.hybrid_key_mapping[i][1] == HYBRID_KEY_5_CLICK) ||
                (app_key_cfg.hybrid_key_mapping[i][1] == HYBRID_KEY_6_CLICK) ||
                (app_key_cfg.hybrid_key_mapping[i][1] == HYBRID_KEY_7_CLICK)
               )
            {
                ret = true;
                break;
            }
        }
    }

    APP_PRINT_INFO1("app_key_multi_click_match: ret %d", ret);
    return ret;
}

static void app_key_report_action(uint8_t key_mask, uint8_t scenario, uint8_t action)
{
    if (action == MMI_NULL)
    {
        return;
    }

    struct
    {
        uint8_t key_mask;
        uint8_t scenario;
        uint8_t action;
    } __attribute__((packed)) rpt = {};
    rpt.key_mask = key_mask;
    rpt.scenario = scenario;
    rpt.action = action;
    app_report_event(CMD_PATH_UART, EVENT_REPORT_KEY_ACTION, 0, (uint8_t *)&rpt, sizeof(rpt));
}

static uint8_t app_key_get_call_idx(void)
{
    uint8_t call_idx = app_hfp_get_call_status();
    call_idx = (call_idx >= CALL_TYPES_NUM) ? 0xff : call_idx;
    return call_idx;
}

/**
    * @brief  Handle single key actions.
    *         This includes matching the action from the key table and executing it.
    * @param  key key mask, such as KEY0_MASK, KEY1_MASK etc.
    * @return void
    */
void app_key_single_click(uint8_t key)
{
    uint8_t long_press = 0;

    if (key_data.key_long_pressed != 0)
    {
        long_press = 1;
    }

    APP_PRINT_INFO1("app_key_single_click: device_state = %d",
                    app_db.device_state);

    if (app_db.device_state == APP_DEVICE_STATE_ON) //APP in non-off state
    {
        uint8_t call_idx = app_key_get_call_idx();
        if (call_idx != 0xff)
        {
            key_data.key_action = app_key_cfg.key_table[long_press][call_idx][app_key_search_index(key)];
            app_key_execute_action(key_data.key_action);

            app_key_report_action(key, long_press, key_data.key_action);
        }

    }

    app_key_clear();
}

/**
    * @brief  Handle hybrid key actions.
    *         This includes matching the action from the key table and executing it.
    * @param  key key mask, such as KEY0_MASK, KEY1_MASK etc.
    * @return void
    */
static void app_key_hybrid_click(uint8_t key)
{
    uint8_t i;
    uint8_t hybrid_type = HYBRID_KEY_SHORT_PRESS;
    bool is_only_allow_factory_reset = false;

    APP_PRINT_INFO1("app_key_hybrid_click: device_state = %d",
                    app_db.device_state);


    if ((app_db.device_state == APP_DEVICE_STATE_ON))
    {
        if (key_data.key_long_pressed == LONG_PRESS)
        {
            hybrid_type = HYBRID_KEY_LONG_PRESS;
        }
        else if (key_data.key_click >= 2)
        {
            hybrid_type = key_data.key_click + 1;
            if (key_data.key_click >= 6)
            {
                hybrid_type = key_data.key_click + 4;
            }
        }

        for (i = 0; i < HYBRID_KEY_NUM; i++)
        {
            if (((app_key_cfg.hybrid_key_mapping[i][0] == key) &&
                 (app_key_cfg.hybrid_key_mapping[i][1] == hybrid_type)))
            {
                uint8_t call_idx = app_key_get_call_idx();
                if (call_idx != 0xff)
                {
                    uint8_t action = app_key_cfg.hybrid_key_table[call_idx][i];

                    if (action == MMI_DEV_FACTORY_RESET || (app_db.device_state == APP_DEVICE_STATE_ON))
                    {
                        key_data.key_action = action;
                        app_key_execute_action(key_data.key_action);
                        app_key_report_action(key, hybrid_type, key_data.key_action);
                    }
                }
                break;
            }
        }
    }

    app_key_clear();
}

void app_key_long_key_power_off_pressed_set(bool value)
{
    long_key_power_off_pressed = value;
}

bool app_key_long_key_power_off_pressed_get(void)
{
    return long_key_power_off_pressed;
}

/**
    * @brief  Register timer callback to gap layer. Called by app when gap timer is timeout.
    * @note
    * @param  timer_id distinguish io timer type
    * @param  timer_chann indicate which key
    * @return void
    */
static void app_key_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_key_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_KEY_MULTI_CLICK:
        {
            APP_PRINT_TRACE2("app_key_timeout_cb: key_click %d, pre_combine_key_bits 0x%1x",
                             key_data.key_click, key_data.pre_combine_key_bits);
            if ((key_data.key_click == 1) && (key_data.pre_combine_key_bits == 0))
            {
                app_key_single_click(param);
            }
            else
            {
                app_key_hybrid_click(param);
            }
            app_stop_timer(&timer_idx_key_multi_click);
        }
        break;

    case APP_TIMER_KEY_LONG_PRESS:
        {
            app_stop_timer(&timer_idx_key_long_press);
            key_data.key_long_pressed = LONG_PRESS;

            if (key_data.combine_key_bits == 0)
            {
                app_key_single_click(param);
            }
            else
            {
                app_key_hybrid_click(param);
            }

            if ((key_data.combine_key_bits == 0) &&
                (app_key_cfg.key_long_press_repeat_interval != 0) &&
                ((app_key_cfg.key_long_press_repeat_mask & param) == param))
            {
                //Check long pressed key repeat
                app_start_timer(&timer_idx_key_long_press_repeat, "key_long_press_repeat",
                                key_timer_id, APP_TIMER_KEY_LONG_PRESS_REPEAT, param,  true,
                                app_key_cfg.key_long_press_repeat_interval * KEY_TIMER_UNIT_MS);
            }
        }
        break;

    case APP_TIMER_KEY_LONG_PRESS_REPEAT:
        {
            key_data.key_long_pressed = LONG_PRESS_REPEAT;

            if (key_data.combine_key_bits == 0)
            {
                app_key_single_click(param);
            }
            else
            {
                app_key_hybrid_click(param);
            }
        }
        break;

    case APP_TIMER_KEY_POWER_ON_LONG_PRESS:
        {
            app_stop_timer(&timer_idx_key_power_on_long_press);

            if (app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                app_dlps_stop_power_down_wdg_timer();
                key_data.key_long_pressed = LONG_PRESS_POWER_ON;
            }
            else
            {
                app_stop_timer(&timer_idx_key_enter_pairing);
                app_stop_timer(&timer_idx_key_factory_reset);
            }

            app_device_unlock_vbat_disallow_power_on();
        }
        break;

    case APP_TIMER_KEY_POWER_OFF_LONG_PRESS:
        {
            app_stop_timer(&timer_idx_key_power_off_long_press);

            key_data.key_long_pressed = LONG_PRESS_POWER_OFF;
            app_db.power_off_cause = POWER_OFF_CAUSE_LONG_KEY;

            app_key_long_key_power_off_pressed_set(true);

            app_mmi_handle_action(MMI_DEV_POWER_OFF);
        }
        break;

    case APP_TIMER_KEY_ENTER_PAIRING:
        {
            key_data.key_long_pressed = LONG_PRESS_ENTER_PAIRING;
            key_data.key_enter_pairing = 1;
            app_stop_timer(&timer_idx_key_enter_pairing);
        }
        break;

    case APP_TIMER_KEY_FACTORY_RESET:
        {
            app_key_reset_enter_pairing();
            app_stop_timer(&timer_idx_key_factory_reset);
            key_data.key_long_pressed = LONG_PRESS_FACTORY_RESET;
        }
        break;

    default:
        break;
    }
}

static void app_key_start_factory_reset_timer(void)
{
    if (app_key_cfg.key_factory_reset_interval != 0)
    {
        app_start_timer(&timer_idx_key_factory_reset, "key_factory_reset",
                        key_timer_id, APP_TIMER_KEY_FACTORY_RESET, 0, false,
                        app_key_cfg.key_factory_reset_interval * KEY_TIMER_UNIT_MS);
    }
}

void app_key_stop_timer(void)
{
    app_stop_timer(&timer_idx_key_power_on_long_press);
    app_stop_timer(&timer_idx_key_power_off_long_press);
    app_stop_timer(&timer_idx_key_enter_pairing);
    app_stop_timer(&timer_idx_key_factory_reset);
    app_stop_timer(&timer_idx_key_long_press);
    app_stop_timer(&timer_idx_key_long_press_repeat);
}

static void app_key_check_press(T_KEY_CHECK key_check)
{
    uint8_t key = key_check.key;
    uint8_t i;

    APP_PRINT_TRACE4("app_key_check_press: key mask %02x, key_pressed %d, pre_clicks %d, long %d",
                     key, key_check.key_pressed, key_data.key_click, key_data.key_long_pressed);

    if (key_check.key_pressed == KEY_PRESS)
    {

        if (app_db.device_state == APP_DEVICE_STATE_OFF_ING) //power off ing, not response key press
        {
            APP_PRINT_INFO0("app_key_check_press: device is power off ing!");
            return;
        }

        key_data.key_bit_mask |= key;
        key_data.key_detected_num++;

        for (i = 0; i < HYBRID_KEY_NUM; i++)
        {
            if ((app_key_cfg.hybrid_key_mapping[i][0] == key_data.key_bit_mask) &&
                (app_key_cfg.hybrid_key_mapping[i][1] == HYBRID_KEY_COMBINEKEY_POWER_ONOFF))
            {
                key_data.combine_key_bits = key_data.key_bit_mask;
                break;
            }
        }

        if ((key == key_data.mfb_key) && (key_data.key_detected_num == 1))
        {
            app_dlps_disable(APP_DLPS_ENTER_CHECK_MFB_KEY);

            if (app_db.device_state == APP_DEVICE_STATE_OFF) //Power on press
            {
                if (app_charger_cfg.discharger_support)
                {
                    T_APP_CHARGER_STATE app_charger_state;
                    uint8_t state_of_charge;

                    app_charger_state = app_charger_get_charge_state();
                    state_of_charge = app_charger_get_soc();
                    if ((app_charger_state == APP_CHARGER_STATE_NO_CHARGE) && (state_of_charge == BAT_CAPACITY_0))
                    {
                        return;
                    }
                }

                if ((app_key_cfg.key_power_on_interval != 0) &&
                    ((app_key_cfg.key_disable_power_on_off == 0) ||
                     (app_cfg_nv.factory_reset_done == 0))) //Long press power on
                {
                    app_start_timer(&timer_idx_key_power_on_long_press, "key_power_on_long_press",
                                    key_timer_id, APP_TIMER_KEY_POWER_ON_LONG_PRESS, key, false,
                                    app_key_cfg.key_power_on_interval * KEY_TIMER_UNIT_MS);
                }

                if ((app_key_cfg.key_enter_pairing_interval != 0) &&
                    ((app_key_cfg.key_disable_power_on_off == 0) ||
                     (app_cfg_nv.factory_reset_done == 0))) //Long press power enter pairing
                {
                    app_start_timer(&timer_idx_key_enter_pairing, "key_enter_pairing",
                                    key_timer_id, APP_TIMER_KEY_ENTER_PAIRING, key, false,
                                    app_key_cfg.key_enter_pairing_interval * KEY_TIMER_UNIT_MS);
                }

                app_key_start_factory_reset_timer();
            }
            else if (app_db.device_state == APP_DEVICE_STATE_ON) //Power off press
            {

                if ((app_key_cfg.key_power_off_interval != 0) &&
                    ((app_key_cfg.key_disable_power_on_off == 0) ||
                     (app_cfg_nv.factory_reset_done == 0))) //Long press power off
                {
                    app_start_timer(&timer_idx_key_power_off_long_press, "key_power_off_long_press",
                                    key_timer_id, APP_TIMER_KEY_POWER_OFF_LONG_PRESS, key, false,
                                    app_key_cfg.key_power_off_interval * KEY_TIMER_UNIT_MS);
                }

                if (app_key_cfg.key_long_press_interval != 0)
                {
                    app_start_timer(&timer_idx_key_long_press, "key_long_press",
                                    key_timer_id, APP_TIMER_KEY_LONG_PRESS, key, false,
                                    app_key_cfg.key_long_press_interval * KEY_TIMER_UNIT_MS);
                }
            }
        }
        else
        {
            //Combine Key pressed
            if (key_data.key_detected_num >= 2)
            {
                key = key_data.key_bit_mask;
                key_data.combine_key_bits = key_data.key_bit_mask;
                app_key_stop_timer();
            }

            if (app_key_cfg.key_long_press_interval != 0)
            {
                app_start_timer(&timer_idx_key_long_press, "key_long_press",
                                key_timer_id, APP_TIMER_KEY_LONG_PRESS, key, false,
                                app_key_cfg.key_long_press_interval * KEY_TIMER_UNIT_MS);
            }
        }
    }
    else //Key release
    {
        key_data.key_bit_mask &= ~key;

        //fix hall switch not power off issue
        if (key_data.key_detected_num == 0)
        {
            key_data.key_detected_num = 0;
        }
        else
        {
            key_data.key_detected_num--;
        }

        if (key_data.combine_key_bits)
        {
            if (key_data.key_detected_num != 0)
            {
                //once combine key was pressed, detect key/action after all key release
                app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                return;
            }
            else
            {
                key = key_data.combine_key_bits;
            }
        }

        app_key_stop_timer();

        if (key == key_data.mfb_key)
        {
            app_dlps_enable(APP_DLPS_ENTER_CHECK_MFB_KEY);

            if (app_db.device_state == APP_DEVICE_STATE_OFF) //Power on release
            {
                if (app_charger_cfg.discharger_support)
                {
                    T_APP_CHARGER_STATE app_charger_state;
                    uint8_t state_of_charge;

                    app_charger_state = app_charger_get_charge_state();
                    state_of_charge = app_charger_get_soc();
                    if ((app_charger_state == APP_CHARGER_STATE_NO_CHARGE) && (state_of_charge == BAT_CAPACITY_0))
                    {
                        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                        return;
                    }
                }

                if (app_key_cfg.key_power_on_interval != 0) //Long press power on
                {
                    if (key_data.key_long_pressed == LONG_PRESS_POWER_ON)
                    {
                        app_mmi_handle_action(MMI_DEV_POWER_ON);
                    }
                }
                else
                {
                    if (key_data.key_long_pressed == 0) //Short press power on
                    {
                        // Before factory reset, allow long press MFB to power on and enter pairing when key_disable_power_on_off
                        if ((app_key_cfg.key_disable_power_on_off == 0) ||
                            (app_cfg_nv.factory_reset_done == 0))
                        {
                            app_mmi_handle_action(MMI_DEV_POWER_ON);

                            app_device_unlock_vbat_disallow_power_on();
                        }
                    }
                }

                if (key_data.key_long_pressed == LONG_PRESS_ENTER_PAIRING)
                {
                    app_key_report_action(key, 1, MMI_DEV_ENTER_PAIRING_MODE);
                    app_mmi_handle_action(MMI_DEV_ENTER_PAIRING_MODE);
                }

                if (key_data.key_long_pressed == LONG_PRESS_FACTORY_RESET)
                {
                    app_key_report_action(key, 1, MMI_DEV_FACTORY_RESET);
                    app_mmi_handle_action(MMI_DEV_FACTORY_RESET);
                }

                app_key_reset_enter_pairing();
                key_data.key_long_pressed = 0;
                app_db.is_long_press_power_on_play_vp = 0;
                app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);

                if (key_data.combine_key_bits)
                {
                    goto exit;
                }
                return;
            }
            else if ((app_db.device_state == APP_DEVICE_STATE_OFF_ING) ||
                     (app_db.device_state == APP_DEVICE_STATE_ON))   //Power off release
            {
                if (app_key_cfg.key_power_off_interval != 0) //Long press power off
                {
                    app_key_long_key_power_off_pressed_set(false);

                    if (key_data.key_long_pressed == LONG_PRESS_POWER_OFF)
                    {
                        key_data.key_long_pressed = 0;
                        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                        return;
                    }
                    else if (key_data.key_long_pressed == LONG_PRESS)
                    {
                        app_key_single_click(key);
                        key_data.key_long_pressed = 0;
                        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);

                        return;
                    }
                }
                else if ((app_key_cfg.key_disable_power_on_off == 0) ||
                         (app_cfg_nv.factory_reset_done == 0)) //Short press power off
                {
                    app_mmi_handle_action(MMI_DEV_POWER_OFF);
                    key_data.key_long_pressed = 0;
                    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                    return;
                }
            }
        }

        if ((key_data.key_long_pressed == LONG_PRESS) ||
            (key_data.key_long_pressed == LONG_PRESS_REPEAT))
        {
            if (key_data.key_action == MMI_AV_FASTFORWARD)
            {
                // Special handling Fast-forward release MMI, maybe triggered from SPK2
                app_key_execute_action(MMI_AV_FASTFORWARD_STOP);
            }
            else if (key_data.key_action == MMI_AV_REWIND)
            {
                // Special handling Rewind release MMI, maybe triggered from SPK2
                app_key_execute_action(MMI_AV_REWIND_STOP);
            }
        }
#if 0
hybrid:
#endif
        if (key_data.key_long_pressed == 0) //Short press
        {
            if (key_data.key_click == 0) //First key
            {
                key_data.key_click++;
                if ((app_key_cfg.key_multi_click_interval != 0) && app_key_multi_click_match(key))
                {
                    app_start_timer(&timer_idx_key_multi_click, "key_multi_click",
                                    key_timer_id, APP_TIMER_KEY_MULTI_CLICK, key, false,
                                    app_key_cfg.key_multi_click_interval * KEY_TIMER_UNIT_MS);
                }
                else
                {
                    if (key_data.combine_key_bits == 0) //Single Key press
                    {
                        app_key_single_click(key);
                    }
                    else
                    {
                        app_key_hybrid_click(key_data.combine_key_bits);
                    }
                }
            }
            else
            {
                if (key_data.pre_key == key) //Same key repeat click
                {
                    key_data.key_click++;

                    if ((app_key_cfg.key_multi_click_interval != 0) && app_key_multi_click_match(key))
                    {
                        app_stop_timer(&timer_idx_key_multi_click);

                        app_start_timer(&timer_idx_key_multi_click, "key_multi_click",
                                        key_timer_id, APP_TIMER_KEY_MULTI_CLICK, key, false,
                                        app_key_cfg.key_multi_click_interval * KEY_TIMER_UNIT_MS);
                    }
                }
                else //Different keys, process directly
                {
                    //Previous key
                    if ((key_data.key_click == 1) && (key_data.pre_combine_key_bits == 0))
                    {
                        app_key_single_click(key_data.pre_key);
                    }
                    else
                    {
                        app_key_hybrid_click(key_data.pre_key);
                    }

                    //Current key
                    if (key_data.combine_key_bits == 0)
                    {
                        app_key_single_click(key);
                    }
                    else
                    {
                        app_key_hybrid_click(key_data.combine_key_bits);
                    }

                    app_stop_timer(&timer_idx_key_multi_click);
                }
            }
        }
exit:
        if (key_data.key_bit_mask == 0) //All key release
        {
            key_data.pre_key = key;
            key_data.pre_combine_key_bits = key_data.combine_key_bits;
            key_data.key_long_pressed = 0;
            key_data.combine_key_bits = 0;
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_MFB_KEY);
            // if (app_key_cfg.timer_auto_power_off_while_phone_connected_and_anc_apt_off &&
            //     (app_link_get_b2s_link_num() != 0))
            // {
            //     app_auto_power_off_enable(AUTO_POWER_OFF_MASK_KEY,
            //                               app_key_cfg.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
            // }
        }
    }
}

void app_key_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    T_KEY_CHECK key_check = {0};

    key_check.key = io_driver_msg_recv->u.param >> 8;
    key_check.key_pressed = io_driver_msg_recv->u.param & 0xFF;

    app_key_check_press(key_check);
#if F_APP_CLI_BINARY_MP_SUPPORT
    if (mp_test_get_mode_flag())
    {
        mp_test_vendor_send_key_event(key_check.key);
    }
#endif
}

bool app_key_is_enter_pairing(void)
{
    return (key_data.key_enter_pairing == 1);
}

void app_key_reset_enter_pairing(void)
{
    key_data.key_enter_pairing = 0;
}

void app_key_init(void)
{
    key_data.mfb_key = KEY0_MASK;

    app_timer_reg_cb(app_key_timeout_cb, &key_timer_id);
}

