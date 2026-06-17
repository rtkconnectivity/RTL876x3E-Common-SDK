/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "string.h"
#include "wdg.h"
#include "os_timer.h"
#include "trace.h"
#include "app_timer.h"
#include "gap_le.h"
#include "app_charger_cfg.h"
#include "app_charger.h"
#include "sysm.h"
#include "btm.h"
#include "ble_mgr.h"
#include "app_device.h"
#include "app_ble_device.h"
#include "app_ble_service.h"
#include "gap_vendor.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_dsp_cfg.h"
#include "app_dlps.h"
#include "app_report.h"

#include "app_charger.h"
#include "bt_hfp.h"
#include "bt_avrcp.h"
#include "bt_bond.h"
#include "remote.h"
#include "ringtone.h"
#include "voice_prompt.h"
#include "app_mmi.h"
#include "app_bt_policy_cfg.h"
#include "app_bt_policy_api.h"
#include "app_audio_cfg.h"
#include "app_audio_policy.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_bond.h"
#include "app_iap_rtk.h"
#include "eq_utils.h"
#include "pm.h"
#include "app_dlps.h"

#include "app_ota.h"
#include "app_auto_power_off.h"
#include "app_bt_policy_int.h"
#include "app_key_gpio.h"
#include "app_key_process.h"
#include "app_key_cfg.h"

#include "app_cmd.h"
#include "app_bt_policy_api.h"
#include "wdg.h"
#include "app_sniff_mode_cs.h"
#include "single_tone.h"

#include "app_ble_cfg.h"
#include "app_ble_common_adv.h"
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_gfps_cfg.h"
#include "app_gfps_device.h"
#include "app_gfps_finder.h"
#include "app_dult_device.h"
#include "app_dult.h"
#endif
#if F_APP_THROUGHPUT_SERVER_SUPPORT
#include "app_peripheral_adv.h"
#endif
#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
#include "app_findmy_ble.h"
#endif

#if F_APP_FINDMY_SUPPORT_CUSTOMIZED_APP
#include "custom_app.h"
#endif

#if F_APP_DATA_CAPTURE_SUPPORT
#include "app_data_capture_cs.h"
#endif

#include "app_ipc.h"

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_bond_manager.h"
#include "app_tri_dongle_usb_hid_ctrl.h"
#include "app_tri_dongle_cmd.h"
#include "app_tri_dongle_mgr.h"
#endif

#if F_APP_CHARGING_CASE_CMD_TEST_SUPPORT
#include "app_charging_case_cmd.h"
#endif

static bool app_device_power_on = false;

static bool linkback_disable_power_off_fg = false;
static bool pairing_disable_power_off_fg = false;
static bool enable_pairing_complete_led = false;

static uint8_t device_timer_id = 0;
static uint8_t timer_idx_device_reboot = 0;
static uint8_t timer_idx_dut_mode_wait_link_disc = 0;
static uint8_t timer_idx_play_bud_role_tone = 0;
#if F_APP_CHARGING_CASE_CMD_TEST_SUPPORT
static uint8_t timer_idx_factory_reset_cmd = 0;
static uint8_t timer_idx_open_case_cmd = 0;
static uint8_t timer_idx_close_case_cmd = 0;
static uint8_t timer_idx_enter_pairing_cmd = 0;
static uint8_t timer_idx_enter_dut_mode_cmd = 0;
static uint8_t timer_idx_case_charger_cmd = 0;
#endif

typedef enum
{
    APP_TIMER_DEVICE_REBOOT,
    APP_TIMER_DUT_MODE_WAIT_LINK_DISC,
    APP_TIMER_PLAY_BUD_ROLE_TONE,
#if F_APP_CHARGING_CASE_CMD_TEST_SUPPORT
    APP_TIMER_FACTORY_RESET_CMD,
    APP_TIMER_OPEN_CASE_CMD,
    APP_TIMER_CLOSE_CASE_CMD,
    APP_TIMER_ENTER_PAIRING_CMD,
    APP_TIMER_ENTER_DUT_MODE_CMD,
    APP_TIMER_CASE_CHARGER_CMD,
#endif
} T_APP_DEVICE_TIMER;


static void app_device_timerout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_device_timerout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_DEVICE_REBOOT:
        {
            app_stop_timer(&timer_idx_device_reboot);
            chip_reset((T_WDG_MODE)param);
        }
        break;

    case APP_TIMER_DUT_MODE_WAIT_LINK_DISC:
        {
            app_bt_policy_disconnect_all_link();
        }
        break;

    case APP_TIMER_PLAY_BUD_ROLE_TONE:
        {
            if (ringtone_remaining_count_get() == 0 &&
                voice_prompt_remaining_count_get() == 0 &&
                tts_remaining_count_get() == 0)
            {
                app_stop_timer(&timer_idx_play_bud_role_tone);
                //app_relay_async_single(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_REMOTE_SPK2_PLAY_SYNC);
            }
            else
            {

            }
        }
        break;

#if F_APP_CHARGING_CASE_CMD_TEST_SUPPORT
    case APP_TIMER_FACTORY_RESET_CMD:
        {
            app_stop_timer(&timer_idx_factory_reset_cmd);
            app_start_timer(&timer_idx_open_case_cmd, "open_case_cmd",
                            device_timer_id, APP_TIMER_OPEN_CASE_CMD, 0, false,
                            15 * 1000);
            app_charging_case_send_cmd(AD2B_CMD_FACTORY_RESET);
        }
        break;

    case APP_TIMER_OPEN_CASE_CMD:
        {
            app_stop_timer(&timer_idx_open_case_cmd);
            app_start_timer(&timer_idx_enter_pairing_cmd, "enter_pairing_cmd",
                            device_timer_id, APP_TIMER_ENTER_PAIRING_CMD, 0, false,
                            15 * 1000);
            app_charging_case_send_cmd(AD2B_CMD_OPEN_CASE);
        }
        break;

    case APP_TIMER_CLOSE_CASE_CMD:
        {
            app_stop_timer(&timer_idx_close_case_cmd);
            app_charging_case_send_cmd(AD2B_CMD_CLOSE_CASE);
        }
        break;

    case APP_TIMER_ENTER_PAIRING_CMD:
        {
            app_stop_timer(&timer_idx_enter_pairing_cmd);
            app_start_timer(&timer_idx_case_charger_cmd, "case_charger_cmd",
                            device_timer_id, APP_TIMER_CASE_CHARGER_CMD, 0, false,
                            15 * 1000);
            app_charging_case_send_cmd(AD2B_CMD_ENTER_PAIRING);
        }
        break;

    case APP_TIMER_ENTER_DUT_MODE_CMD:
        {
            app_stop_timer(&timer_idx_enter_dut_mode_cmd);
            app_start_timer(&timer_idx_close_case_cmd, "close_case_cmd",
                            device_timer_id, APP_TIMER_CLOSE_CASE_CMD, 0, false,
                            15 * 1000);
            app_charging_case_send_cmd(AD2B_CMD_ENTER_DUT_MODE);
        }
        break;

    case APP_TIMER_CASE_CHARGER_CMD:
        {
            app_stop_timer(&timer_idx_case_charger_cmd);
            app_start_timer(&timer_idx_enter_dut_mode_cmd, "enter_dut_mode_cmd",
                            device_timer_id, APP_TIMER_ENTER_DUT_MODE_CMD, 0, false,
                            15 * 1000);
            app_charging_case_send_cmd(AD2B_CMD_CASE_CHARGER);
        }
        break;
#endif

    default:
        break;
    }
}

#if F_APP_CHARGING_CASE_CMD_TEST_SUPPORT
void app_device_one_wire_uart_cmd_test(void)
{
    app_start_timer(&timer_idx_factory_reset_cmd, "factory_reset_cmd",
                    device_timer_id, APP_TIMER_FACTORY_RESET_CMD, 0, false,
                    15 * 1000);
}
#endif

bool app_device_is_power_on(void)
{
    return app_device_power_on;
}

void app_device_reboot(uint32_t timeout_ms, T_WDG_MODE wdg_mode)
{
    app_start_timer(&timer_idx_device_reboot, "device_reboot",
                    device_timer_id, APP_TIMER_DEVICE_REBOOT, wdg_mode, false,
                    timeout_ms);
    app_timer_register_pm_excluded(&timer_idx_device_reboot);
}

static bool app_device_disconn_need_play(uint8_t *bd_addr, uint16_t cause)
{
    bool need_play = true;

    if ((app_db.disconnect_inactive_link_actively == true) &&
        !(memcmp(app_db.resume_addr, bd_addr, 6)) &&
        (cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
    {
        //If link disconnection is initiated by roleswap module, it needn't play disconnect tone.
        need_play = false;
    }

    return need_play;
}

void app_device_factory_reset(void)
{
    uint8_t temp_first_engaged;
    uint16_t temp_magic;
    app_ble_device_handle_factory_reset();
    app_cfg_reset();

    if (app_db.factory_reset_clear_mode == FACTORY_RESET_CLEAR_ALL)
    {
        bt_bond_clear();
    }
    else if (app_db.factory_reset_clear_mode == FACTORY_RESET_CLEAR_NORMAL)
    {
        app_bond_clear_all_keys();
    }
    else if (app_db.factory_reset_clear_mode == FACTORY_RESET_CLEAR_PHONE)
    {
        app_bond_clear_all_keys();

    }
    else if (app_db.factory_reset_clear_mode == FACTORY_RESET_CLEAR_CFG)
    {

    }
    else
    {
        /* no action */
    }

    remote_session_role_set((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);

    voice_prompt_volume_set(app_dsp_cfg_vol.voice_prompt_volume_default);
    ringtone_volume_set(app_dsp_cfg_vol.ringtone_volume_default);

    app_cfg_nv.factory_reset_done = 1;
    if (app_cfg_nv.adp_factory_reset_power_on)
    {
        app_cfg_nv.power_off_cause_cmd = 0;
        app_cfg_nv.auto_power_on_after_factory_reset = 0;
    }
    else if (app_cfg_const.auto_power_on_after_factory_reset)
    {
        app_cfg_nv.auto_power_on_after_factory_reset = 1;
    }

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_bond_mgr_ftl_erase();
    app_tri_dongle_cmd_disc_erase();
    app_tri_dongle_usb_hid_ctrl_ble_erase();
#if F_APP_MULTI_2EP_1_BR_1_BLE_HID
    app_tri_dongle_usb_hid_ctrl_br_erase();
#endif
    app_tri_dongle_default_setting();
#endif

    if (app_cfg_store_all() != 0)
    {
        APP_PRINT_ERROR0("app_device_factory_reset: save nv cfg data error");
    }

    app_ipc_publish(APP_DEVICE_IPC_TOPIC, APP_DEVICE_IPC_EVT_FACTORY_RESET, NULL);
}

static void app_device_link_policy_ind(T_BP_EVENT event, T_BP_EVENT_PARAM *event_param)
{
    APP_PRINT_INFO1("app_device_link_policy_ind: event 0x%02x", event);

#if F_APP_AUTO_POWER_TEST_LOG
    TEST_PRINT_INFO1("app_device_link_policy_ind: event 0x%02x", event);
#endif

    switch (event)
    {
    case BP_EVENT_STATE_CHANGED:
        {
            if (event_param->is_shut_down)
            {
                if (app_db.device_state == APP_DEVICE_STATE_OFF_ING)
                {
                }
            }
            else
            {
                if (app_bt_policy_get_state() == BP_STATE_PAIRING_MODE)
                {
                    enable_pairing_complete_led = true;

                    pairing_disable_power_off_fg = true;

                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_PAIRING_MODE);
                    app_key_reset_enter_pairing();
                }
                else
                {
                    if (pairing_disable_power_off_fg)
                    {
                        pairing_disable_power_off_fg = false;

                        if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off &&
                            (app_bt_policy_get_b2s_connected_num() != 0))
                        {
                            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_PAIRING_MODE,
                                                      app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
                        }
                        else
                        {
                            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_PAIRING_MODE, app_cfg_const.timer_auto_power_off);
                        }
                    }

                    if (app_bt_policy_get_state() != BP_STATE_CONNECTED)
                    {
                        enable_pairing_complete_led = false;
                    }
                }

                if (app_bt_policy_get_state() == BP_STATE_LINKBACK)
                {
                    linkback_disable_power_off_fg = true;
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_LINKBACK);
                }
                else
                {
                    if (linkback_disable_power_off_fg)
                    {
                        linkback_disable_power_off_fg = false;

                        if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off &&
                            (app_bt_policy_get_b2s_connected_num() != 0))
                        {
                            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_LINKBACK,
                                                      app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
                        }
                        else
                        {
                            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_LINKBACK, app_cfg_const.timer_auto_power_off);
                        }

                    }

                    if (app_bt_policy_get_b2s_connected_num() > 1)
                    {
                        app_hfp_adjust_sco_window(NULL, APP_SCO_ADJUST_LINKBACK_END_EVT);
                    }
                }
            }
        }
        break;

    case BP_EVENT_RADIO_MODE_CHANGED:
        {
        }
        break;

    case BP_EVENT_BUD_AUTH_SUC:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BUD_COUPLING, app_cfg_const.timer_auto_power_off);
            }
            else
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BUD_COUPLING);
            }
        }
        break;

    case BP_EVENT_BUD_CONN_FAIL:
    case BP_EVENT_BUD_AUTH_FAIL:
        {

        }
        break;

    case BP_EVENT_BUD_DISCONN_NORMAL:
    case BP_EVENT_BUD_DISCONN_LOST:
        {
            if (event == BP_EVENT_BUD_DISCONN_NORMAL)
            {
                app_audio_tone_type_play(TONE_REMOTE_DISCONNECT, false, false);
            }
            else if (event == BP_EVENT_BUD_DISCONN_LOST)
            {
                app_audio_tone_type_play(TONE_REMOTE_LOSS, false, false);
            }

            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BUD_COUPLING, app_cfg_const.timer_auto_power_off);

            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].connected_profile & HFP_PROFILE_MASK)
                {
                    app_hfp_batt_level_report(app_db.br_link[i].bd_addr);
                }
            }

            app_report_rws_bud_info();

        }
        break;

    case BP_EVENT_BUD_LINKLOST_TIMEOUT:
        {
            app_db.power_off_cause = POWER_OFF_CAUSE_SEC_LINKLOST_TIMEOUT;
            app_mmi_handle_action(MMI_DEV_POWER_OFF);
        }
        break;

    case BP_EVENT_SRC_AUTH_SUC:
        {
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_link_find_br_link(event_param->bd_addr);
            if (p_link != NULL)
            {
                p_link->cmd.tx_mask |= TX_ENABLE_AUTHEN_BIT;

#if F_APP_GATT_OVER_BREDR_SUPPORT
                p_link->cmd.tx_mask |= TX_ENABLE_CCCD_BIT;
#endif
            }

#if F_APP_IAP_RTK_SUPPORT
            app_iap_rtk_create(event_param->bd_addr);
#endif
            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_SOURCE_LINK);

            if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK,
                                          app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
            }

            app_bt_policy_b2s_conn_start_timer();
        }
        break;

    case BP_EVENT_SRC_AUTH_FAIL:
        {
            app_audio_tone_type_play(TONE_PAIRING_FAIL, false, false);
        }
        break;

    case BP_EVENT_SRC_DISCONN_LOST:
        {
            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                bool link_loss_play = true;

                link_loss_play = app_device_disconn_need_play(event_param->bd_addr, event_param->cause);

                if (event_param->is_first_src)
                {
                    if (link_loss_play)
                    {
                        app_audio_tone_type_play(TONE_LINK_LOSS, false, false);
                    }
                }
                else
                {
                    app_audio_tone_type_play(TONE_LINK_LOSS2, false, false);
                }
            }

            if (0 == app_bt_policy_get_b2s_connected_num())
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK, app_cfg_const.timer_auto_power_off);
            }

            memcpy(app_db.bt_addr_disconnect, event_param->bd_addr, 6);

#if F_APP_IAP_RTK_SUPPORT
            app_iap_rtk_delete(event_param->bd_addr);
#endif
        }
        break;

    case BP_EVENT_SRC_DISCONN_NORMAL:
        {

            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                bool link_disconn_play = true;
                link_disconn_play = app_device_disconn_need_play(event_param->bd_addr, event_param->cause);
                if (link_disconn_play)
                {
                    if (event_param->is_first_src)
                    {
                        app_audio_tone_type_play(TONE_LINK_DISCONNECT, false, false);
                    }
                    else
                    {
                        app_audio_tone_type_play(TONE_LINK_DISCONNECT2, false, false);
                    }
                }
            }

            if (0 == app_bt_policy_get_b2s_connected_num())
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK, app_cfg_const.timer_auto_power_off);
            }

            memcpy(app_db.bt_addr_disconnect, event_param->bd_addr, 6);
        }
        break;

    case BP_EVENT_PAIRING_MODE_TIMEOUT:
        {
            app_audio_tone_type_play(TONE_PAIRING_TIMEOUT, false, false);
        }
        break;

    case BP_EVENT_DEDICATED_PAIRING:
        {
            if (app_db.avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
            {
                app_mmi_handle_action(MMI_AV_PLAY_PAUSE);
            }
            app_ble_device_handle_enter_pair_mode();
        }
        break;

    case BP_EVENT_PROFILE_CONN_SUC:
        {
            if (event_param->is_first_prof)
            {
                if (enable_pairing_complete_led)
                {
                    enable_pairing_complete_led = false;
                    //led_change_mode(LED_MODE_PAIRING_COMPLETE, true, false);
                }

                T_APP_BR_LINK *p_link = app_link_find_br_link(event_param->bd_addr);
                if (p_link)
                {
                    if (memcmp(app_db.resume_addr, event_param->bd_addr, 6) != 0 &&
                        app_db.disallow_connected_tone_after_inactive_connected == false)
                    {
                        if (!p_link->b2s_connected_vp_is_played)
                        {
                            app_audio_tone_type_play(TONE_LINK_CONNECTED, false, false);

                            //Enable battery report when first phone connected
                            if (app_charger_cfg.enable_bat_report_when_phone_conn && event_param->is_first_src)
                            {
                                uint8_t bat_level = 0;
                                uint8_t state_of_charge = app_db.local_batt_level;

                                state_of_charge = ((state_of_charge + 9) / 10) * 10;
                                bat_level = state_of_charge / 10;

                                app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                         false);
                            }
                        }
                    }
                    else
                    {
                        memset(app_db.resume_addr, 0, 6);
                    }

                    app_db.disallow_connected_tone_after_inactive_connected = false;
                    p_link->b2s_connected_vp_is_played = true;
                }
            }
        }
        break;

    case BP_EVENT_PROFILE_DISCONN:
        {
            ENGAGE_PRINT_TRACE3("app_device_link_policy_ind: disconn %s, prof 0x%08x, is_last %d",
                                TRACE_BDADDR(event_param->bd_addr), event_param->prof, event_param->is_last_prof);

            if (memcmp(app_db.br_link[app_a2dp_get_active_idx()].bd_addr, event_param->bd_addr, 6) == 0)
            {
                if (event_param->prof == A2DP_PROFILE_MASK)
                {
                    app_audio_set_avrcp_status(BT_AVRCP_PLAY_STATUS_STOPPED);
                    //app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_PLAY_STATUS);

                    if (app_hfp_get_call_status() == APP_HFP_CALL_IDLE)
                    {
                        app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
                    }
                }
            }

            if ((app_connected_profiles() & (HFP_PROFILE_MASK | A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK)) == 0)
            {
                if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
                {
                    app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
                }
            }

            if (event_param->prof == SPP_PROFILE_MASK)
            {
                if (app_ota_link_loss_stop())
                {
                    app_ota_error_clear_local(OTA_SPP_DISC);
                }

                app_cmd_update_eq_ctrl(app_cmd_get_tool_connect_status(), true);
            }

            T_APP_BR_LINK *p_link;
            p_link = app_link_find_br_link(event_param->bd_addr);

            if (p_link != NULL &&
                ((p_link->connected_profile & (HFP_PROFILE_MASK | A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK)) == 0))
            {
                if (((p_link->connected_profile & SPP_PROFILE_MASK) != 0)
#if F_APP_DATA_CAPTURE_SUPPORT
                    && (app_data_capture_get_state() == 0)
#endif
                   )
                {
                    app_bt_policy_disconnect(app_db.br_link[p_link->id].bd_addr, SPP_PROFILE_MASK);
                }
            }
            if (p_link && event_param->prof == A2DP_PROFILE_MASK)
            {
                p_link->a2dp.is_streaming = false;
                if (app_link_get_a2dp_start_num() == 0)
                {
                    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_A2DP);
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_device_state_change(T_APP_DEVICE_STATE state)
{
    uint8_t report_data = 0;
    APP_PRINT_INFO2("app_device_state_change: cur_state 0x%02x, next_state 0x%02x",
                    app_db.device_state, state);

    app_db.device_state = state;
    report_data = state;
    if (app_cfg_const.enable_data_uart)
    {
#if (F_APP_NXP_UWB_CALIBRATION_DATA_DUMP == 0)
        app_report_event(CMD_PATH_UART, EVENT_DEVICE_STATE, 0, &report_data, sizeof(report_data));
#endif
    }
}

static void app_device_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            app_cfg_nv.gpio_out_detected = 0;
            app_cfg_nv.app_is_power_on = 1;
            app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

            APP_PRINT_INFO4("app_device_dm_cback: bud_role %d, factory_reset_done %d, key enter pairing %d, %d",
                            app_cfg_nv.bud_role, app_cfg_nv.factory_reset_done,
                            app_key_is_enter_pairing(),
                            app_bt_policy_cfg_nv.enable_multi_link);

            app_device_state_change(APP_DEVICE_STATE_ON);

            app_sniff_mode_startup();

            pairing_disable_power_off_fg = false;
            linkback_disable_power_off_fg = false;
            app_dlps_enable_auto_poweroff_stop_wdg_timer();

            app_db.mic_mp_verify_pri_sel_ori = APP_MIC_SEL_DISABLE;
            app_db.power_off_cause = POWER_OFF_CAUSE_UNKNOWN;
            app_db.play_min_max_vol_tone = true;

            app_db.disallow_play_gaming_mode_vp = false;
            app_db.hfp_report_batt = true;

            if (app_db.ignore_led_mode)
            {
                app_db.ignore_led_mode = false;
            }
            else
            {
                //led_change_mode(LED_MODE_POWER_ON, true, false);
            }

            //Update battery volume after power on
            if (app_charger_cfg.discharger_support)
            {
                app_charger_update();
            }

            //reset to default eq after power on
            if (app_cfg_const.reset_eq_when_power_on)
            {
                app_cfg_nv.eq_idx_normal_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, NORMAL_MODE);
                app_cfg_nv.eq_idx_gaming_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, GAMING_MODE);
                app_cfg_nv.eq_idx_anc_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, ANC_MODE);
#if F_APP_LINEIN_SUPPORT
                app_cfg_nv.eq_idx_line_in_mode_record = eq_utils_get_default_idx(MIC_SW_EQ, APT_MODE);
#endif

                app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_normal_mode_record;
            }

            app_ble_device_handle_power_on();

            if (app_cfg_const.enable_dlps)
            {
                power_mode_set(POWER_DLPS_MODE);
            }

            if (!app_cfg_nv.factory_reset_done || app_key_is_enter_pairing())
            {
                app_bt_policy_startup(app_device_link_policy_ind, false);
            }
            else
            {
                app_bt_policy_startup(app_device_link_policy_ind, true);
            }

            if (!app_cfg_nv.factory_reset_done
                && app_cfg_const.auto_power_on_and_enter_pairing_mode_before_factory_reset)
            {
                app_bt_policy_enter_pairing_mode(false, true);
            }

#if F_APP_DASHBOARD_DEMO_SUPPORT
            app_bt_policy_enter_pairing_mode(false, true);
#endif
            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_POWER_ON, app_cfg_const.timer_auto_power_off);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_WAIT_RESET);
            app_device_power_on = true;

            if (app_cfg_const.enable_power_on_linkback)
            {
                app_bt_policy_stop();
                app_bt_policy_restore();
            }
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            mp_hci_test_set_mode(false);

            app_ble_device_handle_power_off();
            app_auto_power_off_enable(~AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF,
                                      app_cfg_const.timer_auto_power_off);

            if (app_charger_cfg.discharger_support)
            {
                app_cfg_nv.local_level = app_db.local_batt_level;
            }

            app_cfg_nv.app_is_power_on = 0;
            app_cfg_store_all();

            app_bt_stop_a2dp_and_sco();
            app_bt_policy_shutdown();

            app_bond_clear_sec_diff_link_record();

            app_dlps_power_off();

            app_device_power_on = false;
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_device_dm_cback: event_type 0x%04x", event_type);
    }
}

/* @brief  wheather the test equipment is required device
*
* @param  name and addr
* @return true/false
*/
static bool app_device_is_test_equipment(const char *name, uint8_t *addr)
{
    T_APP_TEST_EQUIPMENT_INFO equipment_info[] =
    {
        {{0x00, 0x30, 0xD3}, "AGILENT TECHNOLOGIES"},
        {{0x8C, 0x14, 0x7D}, "AGILENT TECHNOLOGIES"},
        {{0x00, 0x04, 0x43}, "AGILENT TECHNOLOGIES"},
        {{0xBD, 0xBD, 0xBD}, "AGILENT TECHNOLOGIES"},
        {{0x00, 0x02, 0xB1}, "ANRITSU MT8852"},
        {{0x00, 0x00, 0x91}, "ANRITSU MT8852"},
        {{0x00, 0x00, 0x91}, "ANRITSU MT8850A Bluetooth Test set"},
        {{0x70, 0xB3, 0xD5}, "R&S CMW"},
        {{0x00, 0x90, 0xB8}, "R&S CMW"},
        {{0xDC, 0x44, 0x27}, "R&S CMW"},
        {{0x12, 0x34, 0x56}, "R&S CMW"},
        {{0x70, 0xB3, 0xD5}, "R&S CMU"},
        {{0x00, 0x90, 0xB8}, "R&S CMU"},
        {{0xDC, 0x44, 0x27}, "R&S CMU"},
        {{0x70, 0xB3, 0xD5}, "R&S CBT"},
        {{0x00, 0x90, 0xB8}, "R&S CBT"},
        {{0xDC, 0x44, 0x27}, "R&S CBT"},
        {{0x12, 0x34, 0x56}, "R&S CBT"},
    };

    bool ret = false;
    uint8_t i = 0;
    uint8_t device_oui[3] = {0};

    device_oui[0] = addr[5];
    device_oui[1] = addr[4];
    device_oui[2] = addr[3];

    for (i = 0; i < sizeof(equipment_info) / sizeof(T_APP_TEST_EQUIPMENT_INFO); i++)
    {
        if (!strncmp(equipment_info[i].name, name, strlen(equipment_info[i].name)) &&
            !memcmp(device_oui, equipment_info[i].oui, sizeof(equipment_info[i].oui)))
        {
            ret = true;
            break;
        }
    }

    APP_PRINT_TRACE3("app_device_is_test_equipment: ret %d name %s oui %b", ret, TRACE_STRING(name),
                     TRACE_BINARY(3, device_oui));

    return ret;
}

static bool app_device_auto_power_on_before_factory_reset(void)
{
    bool ret = false;
    uint8_t state_of_charge;

    state_of_charge = app_charger_get_soc();

    if (state_of_charge > BAT_CAPACITY_0)
    {
        if (app_cfg_const.auto_power_on_and_enter_pairing_mode_before_factory_reset &&
            !app_cfg_nv.factory_reset_done)
        {
            ret = true;
        }
    }

    APP_PRINT_INFO1("app_device_auto_power_on_before_factory_reset: ret %d", ret);
    return ret;
}

static bool app_device_boot_up_directly(void)
{
    bool ret = false;
    uint8_t bat_capacity = app_charger_get_soc();
    APP_PRINT_INFO3("app_device_boot_up_directly: adp_factory_reset_power_on %d, power_off_cause_cmd %d, bat_capacity: %d",
                    app_cfg_nv.adp_factory_reset_power_on,
                    app_cfg_nv.power_off_cause_cmd, bat_capacity);

    if (bat_capacity == BAT_CAPACITY_0)
    {
        goto boot_up_directly;
    }

    if (!app_key_cfg.mfb_replace_key0)
    {
        if (app_dlps_check_short_press_power_on())
        {
            ret = true;
            goto boot_up_directly;
        }
    }

    if (app_device_auto_power_on_before_factory_reset())
    {
        ret = true;
        goto boot_up_directly;
    }

    if (app_cfg_nv.auto_power_on_after_factory_reset)
    {
        app_cfg_nv.auto_power_on_after_factory_reset = 0;
        app_cfg_store(&app_cfg_nv.eq_idx_gaming_mode_record, 4);

        ret = true;
        goto boot_up_directly;
    }

    if (app_cfg_nv.app_is_power_on)
    {
        ret = true;
    }

    app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

boot_up_directly:
    APP_PRINT_INFO1("app_device_boot_up_directly: ret %d", ret);
    return ret;
}

static bool app_device_power_on_check(void)
{
    bool ret = false;
    uint8_t bat_capacity = app_charger_get_soc();
    APP_PRINT_INFO2("app_device_power_on_check: gpio_out_detected %d, bat_capacity: %d",
                    app_cfg_nv.gpio_out_detected, bat_capacity);

    if (bat_capacity == BAT_CAPACITY_0)
    {
        return ret;
    }

    return ret;
}

static bool app_device_reboot_handle(void)
{
    bool ret = false;
    APP_PRINT_INFO2("app_device_reboot_handle: power_on_cause_cmd %d, auto_power_on_after_factory_reset %d",
                    app_cfg_nv.power_on_cause_cmd,
                    app_cfg_nv.auto_power_on_after_factory_reset);

    if ((!app_db.peri_wake_up)
        && (app_cfg_nv.adp_factory_reset_power_on
            || app_cfg_nv.auto_power_on_after_factory_reset))
    {
        if (app_device_boot_up_directly())
        {
            ret = true;
        }
    }
    else if (app_device_power_on_check())
    {
        ret = true;
    }

    return ret;
}


void app_device_handle_power_on_cmd(void)
{
    if (app_db.bt_is_ready && app_db.ble_is_ready)
    {
        app_device_unlock_vbat_disallow_power_on();
        app_mmi_handle_action(MMI_DEV_POWER_ON);
    }
    else
    {
        //delay power on when stack is ready
        app_db.delay_power_on = true;
    }
    APP_PRINT_INFO3("app_device_handle_power_on_cmd: bt_is_ready %d, ble_is_ready %d, delay_power_on %d",
                    app_db.bt_is_ready, app_db.ble_is_ready, app_db.delay_power_on);
}

static void app_device_bt_and_ble_ready_check(void)
{
    bool power_on_flg = false;

    if (app_db.bt_is_ready && app_db.ble_is_ready)
    {
        APP_PRINT_INFO5("app_device_bt_and_ble_ready_check: is_dut_test_mode %d, app_is_power_on %d, peri_wake_up %d, factory_reset_done %d, adaptor_changed %d",
                        app_cfg_nv.is_dut_test_mode,
                        app_cfg_nv.app_is_power_on,
                        app_db.peri_wake_up,
                        app_cfg_nv.factory_reset_done,
                        app_cfg_nv.adaptor_changed);

        if (app_ble_cfg.rtk_app_adv_support)
        {
            /* init here to avoid app_cfg_nv.bud_local_addr no mac info (due to factory reset) */
#if F_APP_THROUGHPUT_SERVER_SUPPORT
            app_peripheral_adv_init_conn_public();
#else
            app_ble_common_adv_init(app_db.factory_addr);
#endif
        }

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
        app_gfps_module_adv_init();
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
        app_findmy_on_ble_ready();
#endif

#if F_APP_DEVICE_CMD_SUPPORT
        struct
        {
            uint8_t factory_addr[6];
        } __attribute__((packed)) rpt = {};
        memcpy(rpt.factory_addr, app_db.factory_addr, 6);

        app_report_event(CMD_PATH_UART, EVENT_BT_READY, 0, (uint8_t *)&rpt, sizeof(rpt));
#endif

        if (app_cfg_nv.disallow_power_on_when_vbat_in)
        {
            // Power on is not allowed for the first time when vbat in after factory reset by mppgtool;
            // Unlock disallow power on mode by MFB power on or slide switch power on or bud starts chargring

        }
        else if (app_cfg_nv.is_dut_test_mode)
        {
            app_cfg_nv.is_dut_test_mode = 0;
            app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);
        }
        else if (app_cfg_nv.is_app_reboot)
        {
            if (app_device_reboot_handle())
            {
                power_on_flg = true;
            }

            app_cfg_nv.is_app_reboot = 0;
            app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
        }
        else
        {
            if (app_device_auto_power_on_before_factory_reset()
                || ((!app_db.peri_wake_up) &&
                    (app_cfg_nv.app_is_power_on
                     || app_cfg_nv.adp_factory_reset_power_on
                     || app_cfg_nv.auto_power_on_after_factory_reset)))
            {
                if (app_device_boot_up_directly())
                {
                    power_on_flg = true;
                }
            }
            else if (app_device_power_on_check())
            {
                power_on_flg = true;
            }

#if F_APP_SLIDE_SWITCH_POWER_ON_OFF
            if (!app_slide_switch_power_on_valid_check())
            {
                power_on_flg = false;
            }
#endif

        }

        if (app_db.delay_power_on)
        {
            app_db.delay_power_on = false;
            power_on_flg = true;
        }

        if (app_key_cfg.key_gpio_support && (app_key_cfg.key_power_on_interval == 0) &&
            key_gpio_get_press_state(0))
        {
            power_on_flg = false;
        }

#if F_APP_DASHBOARD_DEMO_SUPPORT
        power_on_flg = true;
#endif

        app_ipc_publish(APP_DEVICE_IPC_TOPIC, APP_DEVICE_IPC_EVT_STACK_READY, &power_on_flg);

        if (power_on_flg)
        {
            app_mmi_handle_action(MMI_DEV_POWER_ON);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
        }
    }

#if F_APP_AUTO_POWER_TEST_LOG
    //Don't enable log for power test
#else
    log_module_trace_set(MODULE_HCI, LEVEL_TRACE, true);
    log_module_trace_set(MODULE_BTIF, LEVEL_TRACE, true);
    log_module_trace_set(MODULE_GAP, LEVEL_TRACE, true);
#endif
}

static void app_device_ble_cback(uint8_t subtype, T_LE_GAP_MSG *gap_msg)
{
    switch (subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            if (!app_db.ble_is_ready)
            {
                if (gap_msg->msg_data.gap_dev_state_change.new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
                {
                    app_db.ble_is_ready = true;
                    app_device_bt_and_ble_ready_check();
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_device_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            if (!app_db.bt_is_ready)
            {
                app_db.bt_is_ready = true;
                app_device_bt_and_ble_ready_check();
            }

#if (F_APP_EXT_PA_SUPPORT && CONFIG_SOC_SERIES_RTL8763E)
            if (app_cfg_const.ext_pa_enable)
            {
                uint16_t aon_1_0 = btaon_fast_read_safe(0x1568);
                uint16_t aon_1_1 = btaon_fast_read_safe(0x156A);
                uint8_t p_buf[3] = {0x0B, 0x0E, 0x0};

                APP_PRINT_TRACE5("app_device_bt_cback: before init 0x40058030 = 0x%x, 0x400002A8 = 0x%x, aon_1_0 = 0x%x, aon_1_1 = 0x%x, pinmux = 0x%x",
                                 *((volatile uint32_t *)0x40058030), *((volatile uint32_t *)0x400002A8), \
                                 aon_1_0, aon_1_1, PINMUX0->CFG[2]);

                gap_vendor_cmd_req(0xFC6E, sizeof(p_buf), p_buf);

                *((volatile uint32_t *)0x40058030) &= 0xFFFFFC00;
                *((volatile uint32_t *)0x40058030) |= 0xA0;

                *((volatile uint32_t *)0x400002A8) |= BIT(28);
                *((volatile uint32_t *)0x400002A8) &= 0xFFF1FFFF;
                *((volatile uint32_t *)0x400002A8) |= BIT(16);

                Pinmux_Config(P1_0, DIGI_DEBUG);
                Pinmux_Config(P1_1, DIGI_DEBUG);
                Pad_Config(P1_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
                Pad_Config(P1_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);

                aon_1_0 = btaon_fast_read_safe(0x1568);
                aon_1_1 = btaon_fast_read_safe(0x156A);

                APP_PRINT_TRACE5("app_device_bt_cback: after init 0x40058030 = 0x%x, 0x400002A8 = 0x%x, aon_1_0 = 0x%x, aon_1_1 = 0x%x, pinmux = 0x%x",
                                 *((volatile uint32_t *)0x40058030), *((volatile uint32_t *)0x400002A8), \
                                 aon_1_0, aon_1_1, PINMUX0->CFG[2]);
            }
#endif

        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            //we need to get src name to judge whether enter dut mode or not.
            app_db.force_enter_dut_mode_when_acl_connected = false;
            gap_br_get_remote_name(param->acl_conn_success.bd_addr);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        {
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            if (mp_hci_test_mode_is_running())
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_DUT_MODE, app_cfg_const.timer_auto_power_off);
            }
        }
        break;

    case BT_EVENT_REMOTE_NAME_RSP:
        {
            if (app_device_is_test_equipment(param->remote_name_rsp.name,
                                             param->remote_name_rsp.bd_addr))
            {
                app_db.force_enter_dut_mode_when_acl_connected = true;

                app_mmi_handle_action(MMI_DUT_TEST_MODE);
                app_db.force_enter_dut_mode_when_acl_connected = false;
            }

        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
            switch (param->hfp_call_status.curr_status)
            {
            case BT_HFP_CALL_INCOMING:
                break;

            case BT_HFP_CALL_ACTIVE:
                {
                }
                break;

            default:
                break;
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {

        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {

        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {

        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {

        }
        break;


    case BT_EVENT_REMOTE_DISCONN_CMPL:
        {
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_device_bt_cback: event_type 0x%04x", event_type);
    }
}

uint8_t app_device_get_bud_channel(void)
{
    uint8_t bud_channel;

    // the single bud role key map data was stored at channel left
    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        bud_channel = CHANNEL_L;
    }
    else
    {
        if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
        {
            bud_channel = CHANNEL_L;
        }
        else
        {
            bud_channel = CHANNEL_R;
        }
    }

    return bud_channel;
}

void app_device_bt_policy_startup(bool at_once_trigger)
{
    app_bt_policy_startup(app_device_link_policy_ind, at_once_trigger);
}

void app_device_enter_dut_mode(void)
{
    app_bt_policy_disconnect_all_link();
    app_start_timer(&timer_idx_dut_mode_wait_link_disc, "dut_mode_wait_link_disc",
                    device_timer_id, APP_TIMER_DUT_MODE_WAIT_LINK_DISC, 0, true,
                    2000);
}

void app_device_unlock_vbat_disallow_power_on(void)
{
    if (app_cfg_nv.disallow_power_on_when_vbat_in)
    {
        app_cfg_nv.disallow_power_on_when_vbat_in = 0;
        app_cfg_store(&app_cfg_nv.offset_listening_mode_cycle, 1);
    }
}


void app_device_init(void)
{
    ble_mgr_msg_cback_register(app_device_ble_cback);
    sys_mgr_cback_register(app_device_dm_cback);
    bt_mgr_cback_register(app_device_bt_cback);
    app_timer_reg_cb(app_device_timerout_cb, &device_timer_id);
}
