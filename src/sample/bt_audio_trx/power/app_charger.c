/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "rtk_charger.h"
#include "sysm.h"
#include "app_timer.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_io_msg.h"
#include "app_main.h"
#include "app_charger_cfg.h"
#include "app_charger.h"
#include "app_audio_policy.h"
#include "app_cmd.h"
#include "app_hfp.h"
#if F_APP_GUI_SUPPORT
#include "app_panel_device_db.h"
#endif
#if F_APP_CHARGING_CASE_CMD_SUPPORT
#include "app_le_transfer.h"
#endif


#define RTK_CHARGER_ENABLE 1

static uint8_t charger_timer_id = 0;
static uint8_t timer_idx_low_bat_warning = 0;
static uint8_t timer_idx_low_bat_led = 0;
static uint8_t battery_status = BATTERY_STATUS_NORMAL;
static uint8_t low_batt_warning_count = 1;
static uint8_t low_batt_led_count = 1;
static bool invalid_batt_rec = false;

typedef enum
{
    /*for discharger*/
    APP_TIMER_LOW_BAT_WARNING       = 0,
    APP_TIMER_LOW_BAT_LED           = 1,
    APP_TIMER_RTK_UNPLUG_DEBOUNCE   = 4,
    APP_TIMER_SMOOTH_BAT_REPORT     = 5,
} T_APP_CHARGER_TIMER;

/* App handle charger state or state of charge change message */
typedef enum
{
    APP_CHG_MSG_CHARGER_STATE_CHANGE,           /* charger state message*/
    APP_CHG_MSG_STATE_OF_CHARGE_CHANGE,         /* Battery percentage message*/

    APP_CHG_MSG_TOTAL
} T_APP_CHG_MSG;


#if F_APP_SMOOTH_BAT_REPORT

#define BAT_HISTORY_SIZE   10
#define BAT_SMOOTH_TIMEOUT 10

typedef enum
{
    APP_SMOOTH_BAT_VOL_CHANGE,
    APP_SMOOTH_BAT_VOL_NO_CHANGE,
    APP_SMOOTH_BAT_VOL_DISALLOW,
} T_APP_SMOOTH_BAT_VOL;

static void app_charger_handle_soc(uint8_t state_of_charge, uint8_t first_power_on);

uint8_t bat_history[BAT_HISTORY_SIZE] = {0};
static uint8_t min_discharge_bat_vol = 0;
static uint8_t last_report_discharge_bat_vol = 0;
static uint8_t timer_idx_smooth_bat_report = 0;

static void app_charger_bat_history_clear(void)
{
    uint8_t i = 0;

    for (i = 0; i < BAT_HISTORY_SIZE; i++)
    {
        bat_history[i] = 0;
    }
}

static void app_charger_clear_smooth_bat_handle(void)
{
    app_stop_timer(&timer_idx_smooth_bat_report);
    min_discharge_bat_vol = 0;
    last_report_discharge_bat_vol = 0;
}

static void app_charger_bat_history_enqueue(uint8_t bat_vol)
{
    uint8_t i = 0;
    uint8_t j = 0;

    for (i = 0; i < BAT_HISTORY_SIZE; i++)
    {
        if (bat_history[i] == 0)
        {
            bat_history[i] = bat_vol;
            break;
        }
    }

    if (i == BAT_HISTORY_SIZE)
    {
        for (j = 1; j < BAT_HISTORY_SIZE; j++)
        {
            bat_history[j - 1] = bat_history[j];
        }

        bat_history[BAT_HISTORY_SIZE - 1] = bat_vol;
    }
}

static uint8_t app_charger_bat_discharge_average(void)
{
    uint8_t i = 0;
    uint8_t cnt = 0;
    uint32_t bat_total = 0;
    uint8_t bat_average = 0;

    for (i = 0; i < BAT_HISTORY_SIZE; i++)
    {
        if (bat_history[i])
        {
            bat_total += bat_history[i];
            cnt++;
        }
        else
        {
            break;
        }
    }

    if (cnt != 0)
    {
        bat_average = (bat_total / cnt);
    }

    APP_PRINT_TRACE1("app_charger_bat_discharge_average: %d", bat_average);

    return bat_average;
}

static T_APP_SMOOTH_BAT_VOL app_charger_smooth_bat_discharge(uint8_t origin_vol, uint8_t *new_vol)
{
    T_APP_SMOOTH_BAT_VOL report = APP_SMOOTH_BAT_VOL_NO_CHANGE;
    T_APP_CHARGER_STATE charger_state = app_charger_get_charge_state();
    static uint8_t last_valid_discharge_vol = 0;
    static uint8_t ignore_cnt = 0;

    /* clear bat history when non discharge */
    if (charger_state != APP_CHARGER_STATE_NO_CHARGE)
    {
        last_valid_discharge_vol = 0;
        ignore_cnt = 0;

        app_charger_bat_history_clear();
    }
    else
    {
        if (last_valid_discharge_vol != 0)
        {
            if (((origin_vol < last_valid_discharge_vol) && ((last_valid_discharge_vol - origin_vol) > 3)) &&
                (ignore_cnt <= 2))
            {
                /* ignore this battery change */
                ignore_cnt++;
                report = APP_SMOOTH_BAT_VOL_DISALLOW;
                goto exit;
            }
        }

        ignore_cnt = 0;

        app_charger_bat_history_enqueue(origin_vol);
    }

    if (charger_state == APP_CHARGER_STATE_NO_CHARGE) //discharge
    {
        last_valid_discharge_vol = origin_vol;

        /* get bat average */
        *new_vol = app_charger_bat_discharge_average();
        report = APP_SMOOTH_BAT_VOL_CHANGE;

        if (min_discharge_bat_vol == 0) // first time get discharge bat vol
        {
            min_discharge_bat_vol = *new_vol;
            last_report_discharge_bat_vol = *new_vol;
        }
        else
        {
            if (*new_vol <= min_discharge_bat_vol)
            {
                if (*new_vol < min_discharge_bat_vol)
                {
                    min_discharge_bat_vol = *new_vol;
                }

                if (timer_idx_smooth_bat_report == 0)
                {
                    /* start a timer to smooth bat consumption report */
                    app_start_timer(&timer_idx_smooth_bat_report, "smooth_bat_report",
                                    charger_timer_id, APP_TIMER_SMOOTH_BAT_REPORT, 0, true,
                                    BAT_SMOOTH_TIMEOUT * 1000);
                }
            }
            else
            {
                /* ignore ping-ping effect */
            }
            report = APP_SMOOTH_BAT_VOL_DISALLOW;
        }
    }
    else
    {
        app_charger_clear_smooth_bat_handle();
    }

exit:
    return report;
}
#endif

static T_APP_CHARGER_STATE app_charger_state_convert(T_CHARGER_STATE rtk_charger_state)
{
    T_APP_CHARGER_STATE app_charger_state = APP_CHARGER_STATE_ERROR;

    switch (rtk_charger_state)
    {
    case STATE_CHARGER_START:
    case STATE_CHARGER_PRE_CHARGE:
    case STATE_CHARGER_FAST_CHARGE:
        app_charger_state = APP_CHARGER_STATE_CHARGING;
        break;

    case STATE_CHARGER_FINISH:
        app_charger_state = APP_CHARGER_STATE_CHARGE_FINISH;
        break;

    case STATE_CHARGER_ERROR:
        app_charger_state = APP_CHARGER_STATE_ERROR;
        break;

    case STATE_CHARGER_END:
        app_charger_state = APP_CHARGER_STATE_NO_CHARGE;
        break;

    default:
        break;
    }

    return app_charger_state;
}

static void app_charger_state_cb(T_CHARGER_STATE rtk_charger_state)
{
    T_IO_MSG adp_msg;
    T_APP_CHARGER_STATE app_charger_state = APP_CHARGER_STATE_ERROR;

    app_charger_state = app_charger_state_convert(rtk_charger_state);

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_CHARGER;
    adp_msg.u.param = APP_CHG_MSG_CHARGER_STATE_CHANGE + (app_charger_state << 8);

    app_io_msg_send(&adp_msg);
}

static void app_charger_soc_cb(uint8_t soc)
{
    T_IO_MSG adp_msg;

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_CHARGER;
    adp_msg.u.param = APP_CHG_MSG_STATE_OF_CHARGE_CHANGE + (soc << 8);

    app_io_msg_send(&adp_msg);
}

static void app_charger_enable_low_bat_warning(void)
{
    uint8_t multiplier = LOW_BAT_WARNING_TO_MULTIPLIER_SECOND;

    if (app_charger_cfg.enable_new_low_bat_time_unit)
    {
        multiplier = LOW_BAT_WARNING_TO_MULTIPLIER_MINUTE;
    }

    app_start_timer(&timer_idx_low_bat_warning, "low_bat_warning",
                    charger_timer_id, APP_TIMER_LOW_BAT_WARNING, 0, true,
                    (app_charger_cfg.timer_low_bat_warning * multiplier * 1000));
}

/**
    * @brief  charger related timer timeout process
    * @note
    * @param  timer_id distinguish io timer type
    * @param  timer_chann indicate which timer channel
    * @return void
    */
static void app_charger_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    bool stop_reload_timer = true;

    APP_PRINT_TRACE2("app_charger_timeout_cb: timer_evt 0x%02x, param %d ", timer_evt, param);
    switch (timer_evt)
    {
    case APP_TIMER_LOW_BAT_WARNING:
        {
            if (app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                /* timer is running on upperstack task and send interrupt msg to app task,
                so if timer timeout is little, when power off action running, timeout msg maybe has send to app task,
                there is happen power off action disable timer, but app task would run timer callback, */
                APP_PRINT_WARN0("discharger_timer_callback: device state is off, callback not run");
                app_stop_timer(&timer_idx_low_bat_warning);
                break;
            }

            T_APP_CHARGER_STATE app_charger_state;
            uint8_t bat_level;

            app_charger_state = app_charger_get_charge_state();

            if (app_charger_state == APP_CHARGER_STATE_NO_CHARGE)
            {
                if (battery_status == BATTERY_STATUS_LOW)
                {
                    bat_level = (app_db.local_batt_level + 9) / 10;

                    if (app_charger_cfg.low_bat_warning_count == 0)
                    {
                        app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                 false);
                        stop_reload_timer = false;
                    }
                    else
                    {
                        if (low_batt_warning_count < app_charger_cfg.low_bat_warning_count)
                        {
                            app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                     false);
                            low_batt_warning_count += 1;
                            stop_reload_timer = false;
                        }
                    }
                }
            }

            if (stop_reload_timer)
            {
                app_stop_timer(&timer_idx_low_bat_warning);
            }
        }
        break;

    case APP_TIMER_LOW_BAT_LED:
        {
            if (app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                /* timer is running on upperstack task and send interrupt msg to app task,
                so if timer timeout is little, when power off action running, timeout msg maybe has send to app task,
                there is happen power off action disable timer, but app task would run timer callback, */
                APP_PRINT_WARN0("discharger_timer_callback: device state is off, callback not run");
                app_stop_timer(&timer_idx_low_bat_led);
                break;
            }

            T_APP_CHARGER_STATE app_charger_state;

            app_charger_state = app_charger_get_charge_state();
            if (app_charger_state == APP_CHARGER_STATE_NO_CHARGE)
            {
                if (battery_status == BATTERY_STATUS_LOW)
                {
                    if (app_charger_cfg.low_bat_warning_count == 0)
                    {
                        stop_reload_timer = false;
                    }
                    else
                    {
                        if (low_batt_led_count < app_charger_cfg.low_bat_warning_count)
                        {
                            stop_reload_timer = false;
                            low_batt_led_count += 1;
                        }
                    }
                }
            }

            if (stop_reload_timer)
            {
                app_stop_timer(&timer_idx_low_bat_led);
            }
        }
        break;

#if F_APP_SMOOTH_BAT_REPORT
    case APP_TIMER_SMOOTH_BAT_REPORT:
        {
            app_charger_handle_soc(last_report_discharge_bat_vol, 0);

            if (last_report_discharge_bat_vol > min_discharge_bat_vol)
            {
                if (last_report_discharge_bat_vol > 0)
                {
                    last_report_discharge_bat_vol--;
                }
            }
            else
            {
                app_stop_timer(&timer_idx_smooth_bat_report);
            }
        }
        break;
#endif

    default:
        break;
    }
}

static void app_charger_handle_charger_state(T_APP_CHARGER_STATE app_charger_state)
{
    app_db.local_charger_state = app_charger_state;

    if (app_charger_state == APP_CHARGER_STATE_CHARGING)
    {
        app_device_unlock_vbat_disallow_power_on();
    }
}

/**
    * @brief  All the state of charge change message will be handled in this function.
    *         App will do some action according to charger battery capacity.
    * @param  state_of_charge Charger battery state, such as %10, 50% etc
    * @param  first_power_on The first time power-on
    * @return void
    */
static void app_charger_handle_soc(uint8_t state_of_charge, uint8_t first_power_on)
{
    T_APP_CHARGER_STATE app_charger_state;

    app_charger_state = app_charger_get_charge_state();

    if (app_charger_state == APP_CHARGER_STATE_NO_CHARGE && app_cfg_nv.is_dut_test_mode &&
        state_of_charge == 0)
    {
        /* dut mode direct power off */
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
        return;
    }

    if ((app_db.device_state != APP_DEVICE_STATE_OFF) || (first_power_on == 1))
    {
        uint8_t bat_level;
        uint8_t ori_state_of_charge = 0;

        APP_PRINT_TRACE5("app_charger_handle_soc: charger_state %d, battery_status %d, charger level %d, local_batt_level %d, first_power_on %d",
                         app_charger_state, battery_status,
                         state_of_charge, app_db.local_batt_level, first_power_on);

        if (first_power_on == 0)
        {
            // adc ready
            if (invalid_batt_rec)
            {
                // Charger module report normal batt level for the first time,need to update on time
                app_db.local_batt_level = state_of_charge;
                invalid_batt_rec = false;
            }
            else if (((app_charger_state == APP_CHARGER_STATE_NO_CHARGE) && //Unplug
                      (state_of_charge >= app_db.local_batt_level)) ||
                     ((app_charger_state <= APP_CHARGER_STATE_CHARGE_FINISH) && //Plug
                      (state_of_charge <= app_db.local_batt_level)))
            {
                //Avoid battry ping-pong detection
                // 1. Adaptor unplug: Detected battery level > current battery level
                // 2. Adaptor plug: Detected battery level < current battery level
                return;
            }
            else
            {
                if (app_charger_state != APP_CHARGER_STATE_ERROR)
                {
                    ori_state_of_charge = app_db.local_batt_level;
                    app_db.local_batt_level = state_of_charge;
#if F_APP_CHARGING_CASE_CMD_SUPPORT
                    app_le_transfer_send_battery(app_db.local_batt_level);
#endif
                }
            }
        }
        else
        {
            /*first get local batt,maybe oxFF*/
            if (state_of_charge > 100)
            {
                // Charger module report 0xFF when ADC not ready, assume nv is reliable
                app_db.local_batt_level = app_cfg_nv.local_level;
                invalid_batt_rec = true;
                return;
            }
            else
            {
                // adc ready
                app_db.local_batt_level = state_of_charge;
                app_cfg_nv.local_level  = state_of_charge;
            }
        }

        if (app_charger_state != APP_CHARGER_STATE_ERROR)
        {
            //Reset to 10% base
            if ((state_of_charge > 0) && (state_of_charge <= app_charger_cfg.battery_warning_percent))
            {
                state_of_charge = ((state_of_charge + 10) / 10 * 10);
                ori_state_of_charge = ((state_of_charge + 10) / 10 * 10);
            }
            else
            {
                state_of_charge     = MULTIPLE_OF_TEN(state_of_charge);
                ori_state_of_charge = MULTIPLE_OF_TEN(ori_state_of_charge);
            }

            bat_level = state_of_charge / 10;

            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].connected_profile & HFP_PROFILE_MASK)
                {
                    app_hfp_batt_level_report(app_db.br_link[i].bd_addr);
                }
            }

            app_report_rws_bud_info();
        }

        if (app_charger_state == APP_CHARGER_STATE_NO_CHARGE) //Adaptor unplug
        {
            if (state_of_charge == BAT_CAPACITY_0) //Shut down
            {
                battery_status = BATTERY_STATUS_EMPTY;

                app_db.power_off_cause = POWER_OFF_CAUSE_LOW_VOLTAGE;

                app_mmi_handle_action(MMI_DEV_POWER_OFF);
            }
            else if (state_of_charge <= app_charger_cfg.battery_warning_percent) //Battery low
            {
                if (!timer_idx_low_bat_warning)
                {
                    if (app_charger_cfg.timer_low_bat_warning)
                    {
                        if (battery_status != BATTERY_STATUS_LOW)
                        {
                            app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                     false);
                            app_charger_enable_low_bat_warning();
                        }
                    }
                    else
                    {
                        app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                 false);
                    }
                }

                battery_status = BATTERY_STATUS_LOW;
            }
            else
            {
                //Battry normal
                battery_status = BATTERY_STATUS_NORMAL;

                //Enable battery report when level drop or power on
                if ((app_charger_cfg.enable_bat_report_when_level_drop && !first_power_on &&
                     (ori_state_of_charge != state_of_charge)) ||
                    (!app_charger_cfg.disable_bat_report_when_power_on && first_power_on))
                {
                    app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                             false);
                }
            }
        }

#if F_APP_GUI_SUPPORT
        app_panel_device_db_set_bettery_level(app_db.local_batt_level);
#endif
    }
}

void app_charger_update(void)
{
    uint8_t state_of_charge;

    state_of_charge = app_charger_get_soc();
    app_charger_handle_soc(state_of_charge, 1);
}

bool app_charger_remote_battery_status_is_low(void)
{
    return (MULTIPLE_OF_TEN(app_db.remote_batt_level) <= app_charger_cfg.battery_warning_percent);
}

bool app_charger_local_battery_status_is_low(void)
{
    return (battery_status < BATTERY_STATUS_NORMAL);
}

static void app_charger_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    switch (event_type)
    {
    case SYS_EVENT_POWER_OFF:
        {
            battery_status = BATTERY_STATUS_NORMAL;

            app_stop_timer(&timer_idx_low_bat_warning);
            app_stop_timer(&timer_idx_low_bat_led);
#if F_APP_SMOOTH_BAT_REPORT
            app_charger_clear_smooth_bat_handle();
#endif
        }
        break;

    default:
        break;
    }
}

T_APP_CHARGER_STATE app_charger_get_charge_state(void)
{
    T_APP_CHARGER_STATE app_charger_state = APP_CHARGER_STATE_ERROR;

#if RTK_CHARGER_ENABLE
    T_CHARGER_STATE rtk_charger_state = rtk_charger_get_charge_state();

    app_charger_state = app_charger_state_convert(rtk_charger_state);
#endif

    return app_charger_state;
}

uint8_t app_charger_get_soc(void)
{
    uint8_t soc = 0;

#if RTK_CHARGER_ENABLE
    soc = rtk_charger_get_soc();
#endif

    return soc;
}

void app_charger_init(void)
{
#if RTK_CHARGER_ENABLE
    rtk_charger_init(app_charger_state_cb, app_charger_soc_cb);
#endif

    app_timer_reg_cb(app_charger_timeout_cb, &charger_timer_id);

    if (app_charger_cfg.discharger_support)
    {
        sys_mgr_cback_register(app_charger_dm_cback);
    }

}

void app_charger_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    uint8_t chg_msg;
    uint8_t chg_param;

    chg_msg = io_driver_msg_recv->u.param & 0xFF;
    chg_param = (io_driver_msg_recv->u.param >> 8) & 0xFF;

    switch (chg_msg)
    {
    case APP_CHG_MSG_CHARGER_STATE_CHANGE:
        {
            app_charger_handle_charger_state((T_APP_CHARGER_STATE)chg_param);
        }
        break;

    case APP_CHG_MSG_STATE_OF_CHARGE_CHANGE:
        {
            if (!app_charger_cfg.discharger_support)
            {
                break;
            }

#if F_APP_SMOOTH_BAT_REPORT
            if ((app_db.device_state == APP_DEVICE_STATE_ON) && (chg_param != 0))
            {
                uint8_t new_vol;
                T_APP_SMOOTH_BAT_VOL result;

                result = app_charger_smooth_bat_discharge(chg_param, &new_vol);

                if (result == APP_SMOOTH_BAT_VOL_DISALLOW)
                {
                    break;
                }
                else if (result == APP_SMOOTH_BAT_VOL_CHANGE)
                {
                    chg_param = new_vol;
                }
            }
#endif
            app_charger_handle_soc(chg_param, 0);
        }
        break;

    default:
        break;
    }
}
