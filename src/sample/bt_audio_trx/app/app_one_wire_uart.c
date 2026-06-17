/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_ONE_WIRE_UART_SUPPORT
#include <string.h>
#include "trace.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_uart.h"
#include "board.h"
#include "platform_utils.h"
#include "pm.h"
#include "sysm.h"
#include "os_sched.h"
#include "os_mem.h"
#include "ble_ext_adv.h"
#include "bt_bond.h"
#include "audio_track.h"
#include "app_timer.h"
#include "app_one_wire_uart.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_dsp_cfg.h"
#include "app_cmd.h"
#include "app_report.h"
#include "app_bt_policy_api.h"
#include "app_audio_policy.h"
#include "app_bond.h"
#include "wdg.h"
#include "app_ipc.h"
#include "app_dlps.h"
//#include "app_adp_cmd.h"
#include "app_console.h"

typedef enum
{
    APP_TIMER_ONE_WIRE_DELAY_WDG_RESET,
    APP_TIMER_ONE_WIRE_AGING_TEST_TONE_REPEAT,
} T_APP_ONE_WIRE_TIMER;

T_ONE_WIRE_UART_STATE one_wire_uart0_state = ONE_WIRE_STATE_STOPPED;
T_ONE_WIRE_UART_STATE one_wire_uart2_state = ONE_WIRE_STATE_STOPPED;

static uint8_t app_one_wire_timer_id = 0;
static uint8_t timer_idx_one_wire_delay_wdg_reset = 0;
static uint8_t timer_idx_one_wire_aging_test_tone_repeat = 0;

static uint8_t aging_test_adv_handle = 0xff;
T_ONE_WIRE_AGING_TEST_STATE one_wire_aging_test_state = ONE_WIRE_AGING_TEST_STATE_STOPED;
T_AUDIO_TRACK_HANDLE aging_test_audio_handle;

static void app_one_wire_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_one_wire_timeout_cb: timer_evt %d, param %d ", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_ONE_WIRE_DELAY_WDG_RESET:
        {
            app_stop_timer(&timer_idx_one_wire_delay_wdg_reset);
            chip_reset(RESET_ALL);
        }
        break;

    case APP_TIMER_ONE_WIRE_AGING_TEST_TONE_REPEAT:
        {
            app_audio_tone_type_play(TONE_AUDIO_EQ_9, false, false);
        }
        break;
    }
}

#if 0
void app_one_wire_start_aging_test(void)
{
    one_wire_aging_test_state = ONE_WIRE_AGING_TEST_STATE_TESTING;

    /* Start BLE aging test adv with minumum adv interval */
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop =
        LE_EXT_ADV_LEGACY_ADV_NON_SCAN_NON_CONN_UNDIRECTED;
    uint16_t adv_interval_min = 0x20;
    uint16_t adv_interval_max = 0x20;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    ble_ext_adv_mgr_init_adv_params(&aging_test_adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, 0, NULL,
                                    0, NULL, NULL);
    ble_ext_adv_mgr_change_adv_tx_power(aging_test_adv_handle, 10);

    if (ble_ext_adv_mgr_get_adv_state(aging_test_adv_handle) == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        ble_ext_adv_mgr_enable(aging_test_adv_handle, 0);
    }

    /* Push VP with maximum volume */
    voice_prompt_volume_set(app_dsp_cfg_vol.voice_prompt_volume_max);

    app_audio_tone_type_play(TONE_AUDIO_EQ_9, false, false);

    app_start_timer(&timer_idx_one_wire_aging_test_tone_repeat, "one_wire_aging_test_tone_repeat",
                    app_one_wire_timer_id, APP_TIMER_ONE_WIRE_AGING_TEST_TONE_REPEAT, 0, true,
                    1000);

    /* Enable microphone bias*/
    T_AUDIO_FORMAT_INFO format_info = {};
    format_info.type = AUDIO_FORMAT_TYPE_SBC;
    format_info.attr.sbc.sample_rate = 16000;
    format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
    format_info.attr.sbc.block_length = 16;
    format_info.attr.sbc.subband_num = 8;
    format_info.attr.sbc.allocation_method = 0;
    format_info.attr.sbc.bitpool = 22;

    aging_test_audio_handle = audio_track_create(AUDIO_STREAM_TYPE_RECORD,
                                                 AUDIO_STREAM_MODE_NORMAL,
                                                 AUDIO_STREAM_USAGE_LOCAL,
                                                 format_info,
                                                 0,
                                                 app_dsp_cfg_vol.record_volume_default,
                                                 AUDIO_DEVICE_IN_MIC,
                                                 NULL,
                                                 NULL);
    audio_track_start(aging_test_audio_handle);
}

void app_one_wire_stop_aging_test(bool aging_test_done)
{
    one_wire_aging_test_state = ONE_WIRE_AGING_TEST_STATE_STOPED;

    if (aging_test_done)
    {
        app_cfg_nv.one_wire_aging_test_done = true;

        app_cfg_store(&app_cfg_nv.offset_one_wire_aging_test_done, 1);
    }

    /* Stop BLE aging test adv*/
    ble_ext_adv_mgr_disable(aging_test_adv_handle, 0);

    /* Reset to default tone/VP volume */
    app_stop_timer(&timer_idx_one_wire_aging_test_tone_repeat);

    app_audio_tone_type_stop();

    voice_prompt_volume_set(app_dsp_cfg_vol.voice_prompt_volume_default);

    /* Disable microphone bias*/
    audio_track_release(aging_test_audio_handle);

    app_device_reboot(1000, RESET_ALL);
}

void app_one_wire_enter_shipping_mode(void)
{
    app_cfg_nv.need_set_lps_mode = 1;

    power_mode_set(POWER_SHIP_MODE);

    app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

    // clear phone record
    app_bond_clear_non_rws_keys();

    // power off
    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        app_mmi_handle_action(MMI_DEV_POWER_OFF);

        app_start_timer(&timer_idx_one_wire_delay_wdg_reset, "one_wire_delay_wdg_reset",
                        app_one_wire_timer_id, APP_TIMER_ONE_WIRE_DELAY_WDG_RESET, NULL, false,
                        6000);
    }
}

void app_one_wire_reset_to_uninitial_state(void)
{
    bt_bond_clear();

    memset(app_cfg_nv.bud_peer_addr, 0, 6);
    memset(app_cfg_nv.bud_peer_factory_addr, 0, 6);
    app_cfg_nv.factory_reset_done = 0;

    app_cfg_reset();

    // for auto enter pairing after reboot
    app_cfg_nv.one_wire_reset_to_uninitial_state = 1;
    app_cfg_store(&app_cfg_nv.offset_one_wire_aging_test_done, 1);

    // for auto power on after reboot
    app_cfg_nv.auto_power_on_after_factory_reset = 1;
    app_cfg_store(&app_cfg_nv.offset_is_dut_test_mode, 1);

    app_device_reboot(2000, RESET_ALL);
}

void app_one_wire_start_force_engage(uint8_t *target_bud_addr)
{
    memcpy(app_cfg_nv.bud_peer_factory_addr, target_bud_addr, 6);

    if (app_db.device_state != APP_DEVICE_STATE_OFF)
    {
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
    }

    app_bond_clear_non_rws_keys();
    app_device_factory_reset();

    // for auto power on after reboot
    app_cfg_nv.auto_power_on_after_factory_reset = 1;

    app_cfg_nv.one_wire_start_force_engage = 1;

    // save all config to protect
    app_cfg_store_all();

    app_device_reboot(2000, RESET_ALL);
}
#endif

T_ONE_WIRE_AGING_TEST_STATE app_one_wire_get_aging_test_state(void)
{
    return one_wire_aging_test_state;
}

static void app_one_wire_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            if (app_cfg_nv.one_wire_reset_to_uninitial_state)
            {
                app_bt_policy_enter_pairing_mode(false, true);
                app_cfg_nv.one_wire_reset_to_uninitial_state = 0;
                app_cfg_store(&app_cfg_nv.offset_one_wire_aging_test_done, 1);
            }
        }
        break;

    default:
        break;
    }
}

void app_one_wire_uart_open(uint8_t index)
{
    switch (index)
    {
    case T_IDX_UART0:
        {
            if (one_wire_uart0_state != ONE_WIRE_STATE_STOPPED)
            {
                return;
            }

            one_wire_uart0_state = ONE_WIRE_STATE_IN_ONE_WIRE;

            APP_PRINT_TRACE1("app_one_wire_uart0_open, one_wire_state: %d", one_wire_uart0_state);
            Pinmux_Deinit(ONE_WIRE_UART0_PINMUX);
            Pinmux_Config(ONE_WIRE_UART0_PINMUX, UART0_TX);
            Pad_Config(ONE_WIRE_UART0_PINMUX, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
                       PAD_OUT_LOW);
            Pad_PullConfigValue(ONE_WIRE_UART0_PINMUX, PAD_STRONG_PULL);

            app_console_uart_driver_init(T_IDX_UART0);

            UART_ClearRxFifo(UART0);
            UART_INTConfig(UART0, UART_INT_RD_AVA | UART_INT_IDLE,
                           ENABLE);//for normal pin, line status intr unnecessary

            platform_delay_ms(5);
        }
        break;

    case T_IDX_UART2:
        {
            if (one_wire_uart2_state != ONE_WIRE_STATE_STOPPED)
            {
                return;
            }

            one_wire_uart2_state = ONE_WIRE_STATE_IN_ONE_WIRE;

            APP_PRINT_TRACE1("app_one_wire_uart2_open, one_wire_state: %d", one_wire_uart2_state);
            Pinmux_Deinit(ONE_WIRE_UART2_PINMUX);
            Pinmux_Config(ONE_WIRE_UART2_PINMUX, UART2_TX);
            Pad_Config(ONE_WIRE_UART2_PINMUX, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
                       PAD_OUT_LOW);
            Pad_PullConfigValue(ONE_WIRE_UART2_PINMUX, PAD_STRONG_PULL);

            app_console_uart_driver_init(T_IDX_UART2);

            UART_ClearRxFifo(UART2);
            UART_INTConfig(UART2, UART_INT_RD_AVA | UART_INT_IDLE,
                           ENABLE);//for normal pin, line status intr unnecessary

            platform_delay_ms(5);
        }
        break;

    default:
        break;
    }
}

void app_one_wire_uart_close(uint8_t index)
{
    switch (index)
    {
    case T_IDX_UART0:
        {
            if (one_wire_uart0_state != ONE_WIRE_STATE_IN_ONE_WIRE)
            {
                return;
            }
            one_wire_uart0_state = ONE_WIRE_STATE_STOPPED;

            Pinmux_Deinit(ONE_WIRE_UART0_PINMUX);
            Pad_Config(ONE_WIRE_UART0_PINMUX, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
                       PAD_OUT_LOW);

        }
        break;

    case T_IDX_UART2:
        {
            if (one_wire_uart2_state != ONE_WIRE_STATE_STOPPED)
            {
                return;
            }

            one_wire_uart2_state = ONE_WIRE_STATE_STOPPED;

            Pinmux_Deinit(ONE_WIRE_UART2_PINMUX);
            Pad_Config(ONE_WIRE_UART2_PINMUX, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
                       PAD_OUT_LOW);
        }
        break;

    default:
        break;
    }
}

void app_one_wire_uart_init(void)
{
    app_timer_reg_cb(app_one_wire_timeout_cb, &app_one_wire_timer_id);
    sys_mgr_cback_register(app_one_wire_dm_cback);
}
#endif
