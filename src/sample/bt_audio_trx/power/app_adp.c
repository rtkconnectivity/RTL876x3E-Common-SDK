/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "os_sched.h"
#include "system_status_api.h"
#include "hal_adp.h"
#include "app_dlps.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_main.h"
#include "app_io_msg.h"
#include "app_adp.h"

#if F_APP_IO_OUTPUT_SUPPORT
#include "app_io_output.h"
#endif
#if F_APP_ONE_WIRE_UART_SUPPORT
#include "app_one_wire_uart.h"
#endif

/** @defgroup  APP_ADP  App Adp
  * @brief
  * @{
  */
/**/

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_ADP_Exported_Variables App adp Variables
    * @{
    */
static uint8_t adaptor_plug_in = ADAPTOR_UNPLUG;
static bool disallow_enable_charger = false;

/** End of Exported_Variables
    * @}
    */
///@endcond

/*============================================================================*
 *                              Functions
 *============================================================================*/
#if F_APP_ONE_WIRE_UART_SUPPORT
void app_adp_io_wakeup_pol_set(void)
{
    uint8_t adp_1wire_pinmux = app_cfg_const.one_wire_uart_data_pinmux;
    T_ADP_STATE adp_io_state = adp_get_current_state(ADP_DETECT_IO);

    if (adp_io_state == ADP_STATE_IN)
    {
        System_WakeUpPinEnable(adp_1wire_pinmux, POL_LOW);
    }
    else if (adp_io_state == ADP_STATE_OUT)
    {
        System_WakeUpPinEnable(adp_1wire_pinmux, POL_HIGH);
    }
}
#endif

static void app_adp_wakeup_pol_set(void)
{
    adp_wake_up_enable(ADP_WAKE_UP_GENERAL);
}

void app_adp_smart_box_charger_control(bool enable)
{
    APP_PRINT_INFO2("app_adp_smart_box_charger_control: enable %d, disallow_enable_charger %d",
                    enable, disallow_enable_charger);

    if (disallow_enable_charger && enable)
    {
        return;
    }

    if (enable)
    {
        /* enable CHARGER_AUTO_ENABLE to enable charger interrupt again */
        sys_hall_charger_auto_enable(true);

        adp_wake_up_enable(ADP_WAKE_UP_GENERAL);
    }
    else
    {
        /* disable CHARGER_AUTO_ENABLE first to prevent interrupt to enable charger again */
        sys_hall_charger_auto_enable(false);

        adp_wake_up_enable(ADP_WAKE_UP_GENERAL);
    }
}

#if F_APP_LOCAL_PLAYBACK_SUPPORT
void app_adp_usb_start_handle(void)
{
    set_cpu_sleep_en(0);
    app_dlps_stop_power_down_wdg_timer();
    app_sd_card_power_down_disable(APP_SD_POWER_DOWN_ENTER_CHECK_USB_MSC);
    app_sd_card_power_on(); //??
    usb_start(NULL);
    app_dlps_disable(APP_DLPS_ENTER_CHECK_USB);
}

void app_adp_usb_stop_handle(void)
{
    usb_stop(NULL);
    audio_fs_update(NULL);
    set_cpu_sleep_en(1);
    app_dlps_enable(APP_DLPS_ENTER_CHECK_USB);
    /*for customer MIFO work around sd en pin*/
    app_sd_card_power_down_enable(APP_SD_POWER_DOWN_ENTER_CHECK_USB_MSC);
    app_dlps_start_power_down_wdg_timer();
}
#endif

uint8_t app_adp_get_plug_state(void)
{
    return adaptor_plug_in;
}

void app_adp_detect(void)
{
    T_ADP_STATE adp_state = ADP_STATE_UNKNOWN;

    while (adp_get_current_state(ADP_DETECT_5V) == ADP_STATE_DETECTING)
    {
        os_delay(10);
    }

    adp_state = adp_get_current_state(ADP_DETECT_5V);

    app_cfg_nv.adaptor_changed = 0;

    if (adp_state == ADP_STATE_IN)
    {
        adaptor_plug_in = ADAPTOR_PLUG;

        if (!app_cfg_nv.adaptor_is_plugged)
        {
            app_cfg_nv.adaptor_is_plugged = 1;
            app_cfg_nv.adaptor_changed = 1;
        }
    }
    else if (adp_state == ADP_STATE_OUT)
    {
        adaptor_plug_in = ADAPTOR_UNPLUG;

        if (app_cfg_nv.adaptor_is_plugged)
        {
            app_cfg_nv.adaptor_is_plugged = 0;
            app_cfg_nv.adaptor_changed = 1;
        }
    }

    if (app_cfg_nv.adaptor_changed)
    {
        //app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
        //app_cfg_store(&app_cfg_nv.offset_notification_vol, 8);
    }

    app_adp_wakeup_pol_set();
}

static void app_adp_plug_handle(void)
{

#if F_APP_IO_OUTPUT_SUPPORT
    if (app_cfg_const.enable_power_supply_adp_in)
    {
        app_io_output_power_supply(true);
    }
#endif

}

static void app_adp_unplug_handle(void)
{

}

void app_adp_msg_handler(uint16_t type)
{
    switch (type)
    {
    case IO_MSG_GPIO_ADAPTOR_PLUG:
        app_adp_plug_handle();
        break;

    case IO_MSG_GPIO_ADAPTOR_UNPLUG:
        app_adp_unplug_handle();
        break;
    }
}

static void app_adp_plug_in_out_handle(T_ADP_PLUG_EVENT status)
{
    T_IO_MSG adp_msg;
    bool loc_msg_handle = false;
    bool adp_msg_handle = false;

    app_cfg_nv.adaptor_changed = 0;

    // adp msg
    adp_msg.type = IO_MSG_TYPE_GPIO;

    switch (status)
    {
    case ADP_EVENT_PLUG_IN:
        {
            if (app_cfg_nv.adaptor_is_plugged == 0)
            {
                adp_msg_handle = true;
                app_cfg_nv.adaptor_is_plugged = 1;
                app_cfg_nv.adaptor_changed = 1;
                adaptor_plug_in = ADAPTOR_PLUG;

                adp_msg.subtype = IO_MSG_GPIO_ADAPTOR_PLUG;

                app_adp_wakeup_pol_set();
            }
        }
        break;

    case ADP_EVENT_PLUG_OUT:
        {
            if (app_cfg_nv.adaptor_is_plugged == 1)
            {
                adp_msg_handle = true;
                app_cfg_nv.adaptor_is_plugged = 0;
                app_cfg_nv.adaptor_changed = 1;
                adaptor_plug_in = ADAPTOR_UNPLUG;

                adp_msg.subtype = IO_MSG_GPIO_ADAPTOR_UNPLUG;

                //Should be able to power on during power off ing after debounce timeout
                //Keep adaptor_is_plugged to correct condition
                if (app_db.device_state == APP_DEVICE_STATE_OFF_ING)
                {
                    app_cfg_nv.adaptor_is_plugged = 1;
                }
            }
        }
        break;

    default:
        break;
    }

#if (F_APP_ONE_WIRE_UART_SUPPORT && !F_APP_ENABLE_TWO_ONE_WIRE_UART)
    if ((app_cfg_const.one_wire_uart_support &&
         app_one_wire_get_aging_test_state() == ONE_WIRE_AGING_TEST_STATE_TESTING))
    {
        /* don't handle in/out box when aging testing */
        loc_msg_handle = false;
    }
#endif

    if (adp_msg_handle)
    {
        app_io_msg_send(&adp_msg);
    }
}

/* Callback run in timer task, should not run too long */
static void app_adp_stage_change_cb(T_ADP_PLUG_EVENT status, void *user_data)
{
    uint32_t adp_type = (uint32_t)user_data;

    {
        app_adp_plug_in_out_handle(status);
    }
}

#if F_APP_ONE_WIRE_UART_SUPPORT
void app_adp_io_plug_out(void)
{
    APP_PRINT_INFO0("app_adp_io_plug_out");

    adp_open(ADP_DETECT_IO);

    app_adp_plug_in_out_handle(ADP_EVENT_PLUG_OUT);
}

static void app_adp_io_init(void)
{
    adp_register_state_change_cb(ADP_DETECT_IO, app_adp_stage_change_cb, (void *)ADP_DETECT_IO);
}
#endif

void app_adp_init(void)
{
    adp_wake_up_enable(ADP_WAKE_UP_GENERAL);
    adp_register_state_change_cb(ADP_DETECT_5V, app_adp_stage_change_cb, (void *)ADP_DETECT_5V);

#if F_APP_ONE_WIRE_UART_SUPPORT
    if (app_cfg_const.one_wire_uart_support)
    {
        app_adp_io_init();
    }
#endif
}

