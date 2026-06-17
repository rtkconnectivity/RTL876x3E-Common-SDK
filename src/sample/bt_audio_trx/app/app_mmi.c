/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "os_sched.h"
#include "test_mode.h"
#include "app_timer.h"
#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "bt_hid_device.h"
#include "bt_bond.h"
#include "system_status_api.h"
#include "sysm.h"
#include "eq.h"
#include "audio.h"
#include "audio_track.h"
#include "remote.h"
//#include "platform_utils.h"
#include "eq_utils.h"
#include "app_mmi.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_dsp_cfg.h"
#include "app_cmd.h"
#include "app_multilink.h"
#include "app_hfp_cfg.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_avrcp_cfg.h"
#include "app_audio_cfg.h"
#include "app_audio_policy.h"
#include "app_link_util_cs.h"
#include "app_bt_policy_cfg.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_dlps.h"
#include "wdg.h"
#include "pm.h"
#include "ringtone.h"
#include "wdg.h"
#include "app_ble_gap.h"
#include "app_eq.h"
#include "rtl876x_pinmux.h"
#include "app_bond.h"

#include "app_key_process.h"

#if F_APP_LINEIN_SUPPORT
#include "line_in.h"
#endif
#include "app_line_in.h"

#include "app_device.h"
#include "app_sniff_mode_cs.h"

#include "eq.h"
#include "single_tone.h"
#include "gap_vendor.h"
#include "app_auto_power_off.h"
#include "gap_ext_scan.h"

#include "hal_debug.h"

#if F_APP_AMP_SUPPORT
#include "app_amp.h"
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
#include "app_findmy.h"
#endif

#if F_APP_SAIYAN_MODE
#include "app_data_capture_cs.h"
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_gfps_cfg.h"
#include "app_gfps_finder.h"
#include "app_gfps_msg.h"
#include "bt_gfps.h"
#include "app_dult_device.h"
#include "app_gfps_finder_adv.h"
#endif

#if F_GUI_BENCHMARK_SUPPORT
#include "benchmark_common.h"
#endif

#define A2DP_MUTE_WHEN_PAUSE_TIMER_MS  2000

static uint8_t app_mmi_timer_id = 0;
static uint8_t timer_idx_reboot_check = 0;
static uint8_t reboot_check_times = REBOOT_CHECK_MAX_TIMES;

typedef enum
{
    APP_TIMER_REBOOT_CHECK,
} T_APP_MMI_TIMER;

bool app_mmi_reboot_check_timer_started(void)
{
    return ((timer_idx_reboot_check != 0) ? true : false);
}

static void app_mmi_power_off(void)
{
    if (app_db.waiting_factory_reset)
    {
        sys_mgr_power_off();
    }
    else
    {
        app_audio_tone_flush(false);
        app_audio_tone_type_stop();

        /* when setting VOICE_PROMPT_MODE_SILENT, Play vp do not trigger AUDIO_EVENT_VOICE_PROMPT_STARTED*/
        if (!app_audio_tone_type_play(TONE_POWER_OFF, false, false))
        {
            sys_mgr_power_off();
        }
    }
}

static void app_mmi_spk_mute_set(uint8_t action)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();
    uint8_t active_hf_idx = app_hfp_get_active_idx();
    uint8_t pair_idx_mapping;
    T_APP_BR_LINK *p_link = NULL;

    APP_PRINT_INFO4("app_mmi_spk_mute_set: bud_role %d, app_hfp_get_call_status %d, active_a2dp_idx %d, active_hf_idx %d",
                    app_cfg_nv.bud_role, app_hfp_get_call_status(), active_a2dp_idx, active_hf_idx);

    if (action == MMI_DEV_SPK_MUTE)
    {
        if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
        {
            app_bond_get_pair_idx_mapping(app_db.br_link[active_hf_idx].bd_addr, &pair_idx_mapping);

            p_link = app_link_find_br_link(app_db.br_link[active_hf_idx].bd_addr);
            if (p_link != NULL)
            {
                if (!p_link->sco.muted)
                {
                    p_link->sco.muted = true;
                    audio_track_volume_out_mute(p_link->sco.track_handle);
                }
            }
        }
        else
        {
            if (app_db.avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
            {
                p_link = &app_db.br_link[active_a2dp_idx];
                if (p_link != NULL)
                {
                    if (!p_link->a2dp.muted)
                    {
                        p_link->a2dp.muted = true;
                        audio_track_volume_out_mute(p_link->a2dp.track_handle);
                    }
                }
            }
        }
    }
    else
    {
        if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
        {
            if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call) &&
                (app_hfp_get_call_status() == APP_HFP_CALL_INCOMING))
            {
                //do nothing
            }
            else
            {
                if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
                {
                    p_link = app_link_find_br_link(app_db.br_link[active_hf_idx].bd_addr);
                    if (p_link != NULL)
                    {
                        app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
                    }
                }
                else
                {
                    p_link = app_link_find_br_link(app_db.br_link[active_a2dp_idx].bd_addr);
                    if (p_link != NULL)
                    {
                        if (p_link->a2dp.muted)
                        {
                            app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
                        }
                    }
                }
            }
        }
        else
        {
        }
    }

}

void app_mmi_reboot_check_timer_start(uint32_t period_timer)
{
    app_start_timer(&timer_idx_reboot_check, "reboot_check",
                    app_mmi_timer_id, APP_TIMER_REBOOT_CHECK, 0, false,
                    period_timer);
}

static void app_mmi_reboot(void)
{
    if (app_db.waiting_factory_reset)
    {
        app_device_factory_reset();
        app_db.waiting_factory_reset = false;
    }

    app_device_state_change(APP_DEVICE_STATE_OFF);

    if (app_cfg_const.disable_power_off_wdt_reset)
    {
        // power down mode
        app_dlps_enable(APP_DLPS_ENTER_CHECK_WAIT_RESET);
        app_dlps_power_off();
    }
    else
    {
        app_cfg_nv.is_app_reboot = 1;
        app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

        os_delay(20);
        chip_reset(RESET_ALL_EXCEPT_AON);
    }
}

void app_mmi_modify_reboot_check_times(uint32_t times)
{
    if (reboot_check_times > times)
    {
        reboot_check_times = times;
    }
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

static uint32_t check_all(void)
{
    typedef uint8_t (*CHK_COND)(void);

    static const struct
    {
        CHK_COND cond_func;
        bool     is_negative;
    } conds[] =
    {
        {ringtone_remaining_count_get, false},
        {voice_prompt_remaining_count_get, false},
        {app_bt_policy_get_b2s_connected_num, false},
#if F_APP_AMP_SUPPORT
        {(CHK_COND)app_amp_is_off_state, true},
#endif
    };

    uint32_t chk_bits = 0;

    for (uint32_t i = 0; i < ARRAY_SIZE(conds); i++)
    {
        bool ret = ((bool)conds[i].cond_func()) ^ conds[i].is_negative;
        chk_bits |= (uint32_t)(ret) << i;
    }

    return chk_bits;
}


static void app_mmi_reboot_check(void)
{
    app_stop_timer(&timer_idx_reboot_check);

    if (reboot_check_times)
    {
        reboot_check_times--;
    }

    if (app_key_long_key_power_off_pressed_get() &&
        (app_db.power_off_cause == POWER_OFF_CAUSE_LONG_KEY ||
         app_db.power_off_cause == POWER_OFF_CAUSE_SHORT_KEY))
    {
        app_mmi_reboot_check_timer_start(100);
        return;
    }

    if (!reboot_check_times)
    {
        app_mmi_reboot();
    }
    else
    {
        uint32_t chk_bits = check_all();
        APP_PRINT_INFO1("app_mmi_reboot_check: check fail reason 0x%x", chk_bits);

        if (chk_bits)
        {
            app_mmi_reboot_check_timer_start(100);
        }
        else
        {
            app_mmi_reboot();
        }
    }
}

static void app_mmi_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_REBOOT_CHECK:
        {

            app_mmi_reboot_check();

        }
        break;

    default:
        break;
    }
}

void app_mmi_init(void)
{
    app_timer_reg_cb(app_mmi_timeout_cb, &app_mmi_timer_id);

}

static void mmi_voice_recognition(void)
{
    uint8_t app_idx;
    {
        app_idx = app_hfp_get_active_idx();
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    if (app_db.br_link[app_idx].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
    {
        bt_hfp_voice_recognition_enable_req(app_db.br_link[app_idx].bd_addr);

        app_audio_tone_type_play(TONE_HF_CALL_VOICE_DIAL, false, false);
    }
    else
    {
        uint8_t bd_addr[6];

        if (app_bond_b2s_addr_get(1, bd_addr) == true)
        {
            app_bt_policy_default_connect(bd_addr,
                                          HSP_PROFILE_MASK | HFP_PROFILE_MASK, true);
        }
    }
}

static void app_volume_max_min_tone_play_status_sync(void)
{

}

static void app_volume_up()
{
    uint8_t max_volume = 0;
    uint8_t curr_volume = 0;
    uint8_t active_idx;
    uint8_t pair_idx_mapping;
    T_AUDIO_STREAM_TYPE volume_type;

#if F_APP_LINEIN_SUPPORT
    if (app_line_in_playing_state_get())
    {
        app_line_in_volume_up_handle();
        return;
    }
#endif

    if (app_avrcp_cfg.disallow_adjust_volume_when_idle == 0)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            return;
        }
    }
    else
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY ||
            (app_hfp_get_call_status() == APP_HFP_CALL_IDLE &&
             app_db.avrcp_play_status != BT_AVRCP_PLAY_STATUS_PLAYING))
        {
            return;
        }
    }

    if ((app_hfp_get_call_status() != APP_HFP_CALL_IDLE))
    {
        volume_type = AUDIO_STREAM_TYPE_VOICE;
        active_idx = app_hfp_get_active_idx();

        if (app_db.br_link[active_idx].hfp.call_status == APP_HFP_CALL_INCOMING &&
            app_db.br_link[active_idx].hfp.is_inband_ring == false)
        {
            APP_PRINT_INFO0("app_volume_up: do not change vol when outband ringtone");

            app_volume_max_min_tone_play_status_sync();
            return;
        }

        if (app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping) == false)
        {
            APP_PRINT_ERROR0("app_volume_up: find active hfp pair_index fail");

            app_volume_max_min_tone_play_status_sync();
            return;
        }
        curr_volume = app_cfg_nv.voice_gain_level[pair_idx_mapping];
        max_volume = app_dsp_cfg_vol.voice_out_volume_max;
    }
    else
    {
        volume_type = AUDIO_STREAM_TYPE_PLAYBACK;
        active_idx = app_a2dp_get_active_idx();

        if (app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping) == false)
        {
            APP_PRINT_ERROR0("app_volume_up: find active a2dp pair_index fail");

            app_volume_max_min_tone_play_status_sync();
            return;
        }
        curr_volume = app_cfg_nv.audio_gain_level[pair_idx_mapping];
        max_volume = app_dsp_cfg_vol.playback_volume_max;
    }

    if (curr_volume < max_volume)
    {
        curr_volume++;
    }
    else
    {
        if (app_is_need_to_enable_circular_volume_up())
        {
            curr_volume = 0;
            app_db.is_circular_vol_up = true;
        }
        else
        {
            curr_volume = max_volume;
        }
    }

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        uint8_t cmd_ptr[9];
        memcpy(&cmd_ptr[0], app_db.br_link[active_idx].bd_addr, 6);
        cmd_ptr[6] = volume_type;
        cmd_ptr[7] = curr_volume;
        cmd_ptr[8] = 0;

        if (volume_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            uint8_t level;

            level = (curr_volume * 0x7F + app_dsp_cfg_vol.playback_volume_max / 2) /
                    app_dsp_cfg_vol.playback_volume_max;

            app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;

            bt_avrcp_volume_change_req(app_db.br_link[active_idx].bd_addr, level);
        }

        else if (volume_type == AUDIO_STREAM_TYPE_VOICE)
        {
            uint8_t level = (curr_volume * 0x0F + app_dsp_cfg_vol.voice_out_volume_max / 2) /
                            app_dsp_cfg_vol.voice_out_volume_max;

            app_cfg_nv.voice_gain_level[pair_idx_mapping] = curr_volume;


            bt_hfp_speaker_gain_level_report(app_db.br_link[active_idx].bd_addr, level);
        }
    }
    else
    {
        if (volume_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            uint8_t level;

            level = (curr_volume * 0x7F + app_dsp_cfg_vol.playback_volume_max / 2) /
                    app_dsp_cfg_vol.playback_volume_max;

            audio_track_volume_out_set(app_db.br_link[active_idx].a2dp.track_handle, curr_volume);

            app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);

            app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;
            bt_avrcp_volume_change_req(app_db.br_link[active_idx].bd_addr, level);

        }
        else if (volume_type == AUDIO_STREAM_TYPE_VOICE)
        {
            uint8_t level = (curr_volume * 0x0F + app_dsp_cfg_vol.voice_out_volume_max /
                             2) / app_dsp_cfg_vol.voice_out_volume_max;

            if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call) &&
                (app_hfp_get_call_status() == APP_HFP_CALL_INCOMING))
            {
                app_hfp_inband_tone_mute_ctrl();
            }
            else
            {
                audio_track_volume_out_set(app_db.br_link[active_idx].sco.track_handle, curr_volume);

                app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
            }

            app_cfg_nv.voice_gain_level[pair_idx_mapping] = curr_volume;
            bt_hfp_speaker_gain_level_report(app_db.br_link[active_idx].bd_addr, level);
        }
    }
}

static void app_volume_down()
{
    bool already_vgs0 = false;
    uint8_t min_volume = 0;
    uint8_t curr_volume = 0;
    uint8_t active_idx;
    uint8_t pair_idx_mapping;
    T_AUDIO_STREAM_TYPE volume_type;

#if F_APP_LINEIN_SUPPORT
    if (app_line_in_playing_state_get())
    {
        app_line_in_volume_down_handle();
        return;
    }
#endif

    if (app_avrcp_cfg.disallow_adjust_volume_when_idle == 0)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            return;
        }
    }
    else
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY ||
            (app_hfp_get_call_status() == APP_HFP_CALL_IDLE &&
             app_db.avrcp_play_status != BT_AVRCP_PLAY_STATUS_PLAYING))
        {
            return;
        }
    }

    if ((app_hfp_get_call_status() != APP_HFP_CALL_IDLE))
    {
        volume_type = AUDIO_STREAM_TYPE_VOICE;
        active_idx = app_hfp_get_active_idx();

        if (app_db.br_link[active_idx].hfp.call_status == APP_HFP_CALL_INCOMING &&
            app_db.br_link[active_idx].hfp.is_inband_ring == false)
        {
            APP_PRINT_INFO0("app_volume_down: do not change vol when outband ringtone");

            app_volume_max_min_tone_play_status_sync();
            return;
        }

        if (app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping) == false)
        {
            APP_PRINT_ERROR0("app_volume_down: find active hfp pair_index fail");

            app_volume_max_min_tone_play_status_sync();
            return;
        }
        curr_volume = app_cfg_nv.voice_gain_level[pair_idx_mapping];
        min_volume = app_dsp_cfg_vol.voice_out_volume_min;
    }
    else
    {
        volume_type = AUDIO_STREAM_TYPE_PLAYBACK;
        active_idx = app_a2dp_get_active_idx();

        if (app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping) == false)
        {
            APP_PRINT_ERROR0("app_volume_down: find active a2dp pair_index fail");

            app_volume_max_min_tone_play_status_sync();
            return;
        }
        curr_volume = app_cfg_nv.audio_gain_level[pair_idx_mapping];
        min_volume = app_dsp_cfg_vol.playback_volume_min;
    }

    if (curr_volume > min_volume)
    {
        curr_volume--;
    }
    else
    {
        curr_volume = min_volume;

        /*if ios version is 13, send AT+VGS=0 repeatedly will make voice mute*/
        if (curr_volume == 0)
        {
            already_vgs0 = true;
        }
    }

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        uint8_t cmd_ptr[9];
        memcpy(&cmd_ptr[0], app_db.br_link[active_idx].bd_addr, 6);
        cmd_ptr[6] = volume_type;
        cmd_ptr[7] = curr_volume;
        cmd_ptr[8] = 0;

        if (volume_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            uint8_t level;

            level = (curr_volume * 0x7F + app_dsp_cfg_vol.playback_volume_max / 2) /
                    app_dsp_cfg_vol.playback_volume_max;

            app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;

            bt_avrcp_volume_change_req(app_db.br_link[active_idx].bd_addr, level);
        }
        else if (volume_type == AUDIO_STREAM_TYPE_VOICE)
        {
            uint8_t level = (curr_volume * 0x0F + app_dsp_cfg_vol.voice_out_volume_max / 2) /
                            app_dsp_cfg_vol.voice_out_volume_max;

            app_cfg_nv.voice_gain_level[pair_idx_mapping] = curr_volume;



            if (already_vgs0 == false)
            {
                bt_hfp_speaker_gain_level_report(app_db.br_link[active_idx].bd_addr, level);
            }
        }
    }
    else
    {
        if (volume_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            uint8_t level;

            level = (curr_volume * 0x7F + app_dsp_cfg_vol.playback_volume_max / 2) /
                    app_dsp_cfg_vol.playback_volume_max;


            audio_track_volume_out_set(app_db.br_link[active_idx].a2dp.track_handle, curr_volume);

            app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);

            bt_avrcp_volume_change_req(app_db.br_link[active_idx].bd_addr, level);
            app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;
        }
        else if (volume_type == AUDIO_STREAM_TYPE_VOICE)
        {
            uint8_t level = (curr_volume * 0x0F + app_dsp_cfg_vol.voice_out_volume_max /
                             2) / app_dsp_cfg_vol.voice_out_volume_max;

            if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call) &&
                (app_hfp_get_call_status() == APP_HFP_CALL_INCOMING))
            {
                app_hfp_inband_tone_mute_ctrl();
            }
            else
            {
                audio_track_volume_out_set(app_db.br_link[active_idx].sco.track_handle, curr_volume);

                app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
            }

            if (already_vgs0 == false)
            {
                bt_hfp_speaker_gain_level_report(app_db.br_link[active_idx].bd_addr, level);
            }

            app_cfg_nv.voice_gain_level[pair_idx_mapping] = curr_volume;
        }
    }
}

void app_mmi_switch_gaming_mode()
{
    APP_PRINT_TRACE4("app_mmi_switch_gaming_mode: old status %d, b2b state %d, disallow tone %d dongle %d",
                     app_db.gaming_mode,
                     app_db.remote_session_state, app_db.disallow_play_gaming_mode_vp,
                     app_db.remote_is_8753bau);
    T_AUDIO_STREAM_USAGE usage;
    T_AUDIO_STREAM_MODE mode;
    uint8_t vol;
    bool vol_muted;
    T_AUDIO_FORMAT_INFO format;
    T_APP_BR_LINK *p_link = NULL;
    uint16_t latency_value = A2DP_LATENCY_MS;
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    if (app_db.gaming_mode)
    {
        app_db.gaming_mode = false;

        if (app_db.remote_is_8753bau)
        {
            uint32_t cpu_freq;
            pm_cpu_freq_set(40, &cpu_freq);
        }

        if (app_db.spk_eq_mode == GAMING_MODE)
        {
            app_cfg_nv.eq_idx_gaming_mode_record = app_cfg_nv.eq_idx;
        }



        if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
             app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
           )
        {
            app_audio_tone_type_play(TONE_EXIT_GAMING_MODE, false, false);
        }
    }
    else
    {
        app_db.gaming_mode = true;
        if (app_db.remote_is_8753bau)
        {
            uint32_t cpu_freq;
            pm_cpu_freq_set(80, &cpu_freq);
        }


        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                app_audio_tone_type_play(TONE_ENTER_GAMING_MODE, false, false);
            }
            else if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                app_audio_tone_type_play(TONE_ENTER_GAMING_MODE, false, false);
            }
        }

        app_audio_get_last_used_latency(&latency_value);
    }


    //app_bt_sniffing_set_low_latency();

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_link_check_b2s_link_by_id(i))
            {
                p_link = &app_db.br_link[i];
                break;
            }
        }
    }
    else
    {
        if (app_link_check_b2s_link_by_id(active_a2dp_idx))
        {
            p_link = &app_db.br_link[active_a2dp_idx];
        }
    }


    if (p_link == NULL || p_link->a2dp.track_handle == NULL)
    {
        app_eq_change_audio_eq_mode(false);
    }

    if (p_link != NULL)
    {
        if (p_link->a2dp.track_handle != NULL)
        {
            APP_PRINT_INFO2("mmi set gaming mode: active link %s, mode %u", TRACE_BDADDR(p_link->bd_addr),
                            app_db.gaming_mode);
            audio_track_format_info_get(p_link->a2dp.track_handle, &format);
            audio_track_usage_get(p_link->a2dp.track_handle, &usage);
            audio_track_volume_out_is_muted(p_link->a2dp.track_handle, &vol_muted);
            audio_track_volume_out_get(p_link->a2dp.track_handle, &vol);

            if (p_link->eq.instance != NULL)
            {
                audio_track_effect_detach(p_link->a2dp.track_handle, p_link->eq.instance);
            }

            audio_track_release(p_link->a2dp.track_handle);

            if (app_db.gaming_mode)
            {
                if (app_db.remote_is_8753bau)
                {
                    mode = AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY;
                }
                else
                {
                    mode = AUDIO_STREAM_MODE_LOW_LATENCY;
                }
            }
            else
            {
                mode = AUDIO_STREAM_MODE_NORMAL;
            }

            p_link->a2dp.track_handle = audio_track_create(AUDIO_STREAM_TYPE_PLAYBACK,
                                                           mode,
                                                           AUDIO_STREAM_USAGE_SNOOP,
                                                           format,
                                                           vol,
                                                           0,
                                                           app_db.playback_device,
                                                           NULL,
                                                           NULL);

            if (vol_muted)
            {
                audio_track_volume_out_mute(p_link->a2dp.track_handle);
                p_link->a2dp.muted = true;
            }

            if (app_db.gaming_mode)
            {
                if (app_db.remote_is_8753bau)
                {
                    latency_value = (remote_session_role_get() == REMOTE_SESSION_ROLE_SINGLE ? ULTRA_NONE_LATENCY_MS :
                                     ULTRA_LOW_LATENCY_MS);
                    audio_track_latency_set(p_link->a2dp.track_handle,
                                            latency_value,
                                            RTK_GAMING_MODE_DYNAMIC_LATENCY_FIX);
                    bt_a2dp_stream_delay_report_req(p_link->bd_addr, latency_value);
                    app_sniff_mode_b2s_disable(p_link->bd_addr, SNIFF_DISABLE_MASK_GAMINGMODE_DONGLE);
                }
                else
                {
                    latency_value = app_audio_set_latency(p_link->a2dp.track_handle,
                                                          app_cfg_nv.rws_low_latency_level_record,
                                                          GAMING_MODE_DYNAMIC_LATENCY_FIX);
                    bt_a2dp_stream_delay_report_req(p_link->bd_addr, latency_value);
                }

                app_db.last_gaming_mode_audio_track_latency = latency_value;
            }
            else
            {
                audio_track_latency_set(p_link->a2dp.track_handle, latency_value,
                                        NORMAL_MODE_DYNAMIC_LATENCY_FIX);
                bt_a2dp_stream_delay_report_req(p_link->bd_addr, latency_value);
            }

            if (p_link->eq.instance != NULL)
            {
                /* when streaming, changing eq after audio track recreate */
                app_eq_change_audio_eq_mode(false);
                audio_track_effect_attach(p_link->a2dp.track_handle, p_link->eq.instance);
            }

#if F_APP_LINEIN_SUPPORT
            if (app_cfg_const.line_in_support)
            {
                if (app_line_in_start_a2dp_check())
                {
                    audio_track_start(p_link->a2dp.track_handle);
                }
            }
            else
#endif
            {
                audio_track_start(p_link->a2dp.track_handle);
            }
        }
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
        app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        app_audio_report_low_latency_status(latency_value);

        if (app_db.spk_eq_mode != LINE_IN_MODE)
        {
            app_report_eq_idx(EQ_INDEX_REPORT_BY_CHANGE_MODE);
        }
    }

}

void app_mmi_execute_output_indication_action(uint8_t action)
{

}


static void app_mmi_check_factory_reset_behavior(uint8_t action)
{
    app_db.factory_reset_ignore_led_and_vp = false;

    switch (action)
    {
    case MMI_DEV_TOOLING_FACTORY_RESET:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_NORMAL;
        app_db.factory_reset_ignore_led_and_vp = true;
        break;

    case MMI_DEV_CFG_FACTORY_RESET:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_CFG;
        break;

    case MMI_DEV_FACTORY_RESET:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_NORMAL;
        if (app_db.ignore_led_mode)
        {
            app_db.factory_reset_ignore_led_and_vp = true;
        }
        break;

    case MMI_DEV_PHONE_RECORD_RESET:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_PHONE;
        if (app_db.ignore_led_mode)
        {
            app_db.factory_reset_ignore_led_and_vp = true;
        }
        break;

    case MMI_DEV_FACTORY_RESET_BY_SPP:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_NORMAL;
        break;

    case MMI_DEV_PHONE_RECORD_RESET_BY_SPP:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_PHONE;
        break;

    case MMI_DEV_RESET_TO_UNINITIAL_STATE:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_ALL;
        break;
    }
}

bool app_mmi_is_allow_factory_reset(void)
{
    bool allow_factory_reset = false;

    if (app_cfg_const.enable_factory_reset_whitout_limit == 1)
    {
        allow_factory_reset = true;
    }
    else if (app_bt_policy_get_state() == BP_STATE_PAIRING_MODE)
    {
        allow_factory_reset = true;
    }

    return allow_factory_reset;
}

void app_mmi_hf_reject_call(void)
{
    T_APP_BR_LINK *p_link;
    T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_MAX;
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    p_link = &(app_db.br_link[active_hf_idx]);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    if (p_link != NULL)
    {
        app_hfp_stop_ring_alert_timer();

        tone_type = (T_APP_AUDIO_TONE_TYPE)app_hfp_get_call_in_tone_type(p_link);
        app_audio_tone_type_cancel(tone_type, false);
        if (bt_hfp_call_terminate_req(p_link->bd_addr))
        {
            app_db.reject_call_by_key = true;
        }
    }
}

void app_mmi_hf_cancel_voice_dial(void)
{
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    bt_hfp_voice_recognition_disable_req(app_db.br_link[active_hf_idx].bd_addr);
}

void app_mmi_hf_end_outgoing_call(void)
{
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    bt_hfp_call_terminate_req(app_db.br_link[active_hf_idx].bd_addr);
}

void app_mmi_hf_end_active_call(void)
{
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    if (app_db.br_link[app_hfp_get_active_idx()].hfp.call_status > APP_HFP_CALL_ACTIVE)
    {
        bt_hfp_release_active_call_accept_held_or_waiting_call_req(app_db.br_link[active_hf_idx].bd_addr);
    }
    else
    {
        bt_hfp_call_terminate_req(app_db.br_link[active_hf_idx].bd_addr);
    }
}

void app_mmi_handle_action(uint8_t action)
{
    APP_PRINT_INFO1("app_mmi_handle_action: action 0x%02x", action);

#if F_APP_AUTO_POWER_TEST_LOG
    TEST_PRINT_INFO1("app_mmi_handle_action: action 0x%02x", action);
#endif

    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    switch (action)
    {
    case MMI_DEV_GAMING_MODE_SWITCH:
        {
            if (app_db.b2s_connected_num == 0)
            {
                return;
            }

            app_mmi_switch_gaming_mode();
        }
        break;

    case MMI_AV_PLAY_PAUSE:
        {
            app_avcrp_play_pause();
        }
        break;

    case MMI_AV_STOP:
        {
            app_avrcp_stop();
        }
        break;

    case MMI_AV_FWD:
        {
            app_avcrp_forward();
        }
        break;

    case MMI_AV_BWD:
        {
            app_avcrp_backward();
        }
        break;

    case MMI_AV_FASTFORWARD:
        {
            app_avrcp_fastforward();
        }
        break;

    case MMI_AV_REWIND:
        {
            app_avrcp_rewind();
        }
        break;

    case MMI_AV_FASTFORWARD_STOP:
        {
            app_avrcp_fast_forward_stop();
        }
        break;

    case MMI_AV_REWIND_STOP:
        {
            app_avrcp_rewind_stop();
        }
        break;

    case MMI_HF_ANSWER_CALL:
        {
            T_APP_BR_LINK *p_link;
            T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_MAX;
            p_link = &(app_db.br_link[active_hf_idx]);

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if (p_link != NULL)
            {
                app_hfp_stop_ring_alert_timer();

                tone_type = (T_APP_AUDIO_TONE_TYPE)app_hfp_get_call_in_tone_type(p_link);
                app_audio_tone_type_cancel(tone_type, false);
                bt_hfp_call_answer_req(p_link->bd_addr);
            }
        }
        break;

    case MMI_HF_REJECT_CALL:
        {
            app_mmi_hf_reject_call();
        }
        break;

    case MMI_HF_END_ACTIVE_CALL:
        {
            app_mmi_hf_end_active_call();
        }
        break;

    case MMI_HF_END_OUTGOING_CALL:
        {
            app_mmi_hf_end_outgoing_call();
        }
        break;

    case MMI_HF_TRANSFER_CALL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if ((app_hfp_get_call_status() == APP_HFP_CALL_INCOMING) &&
                !app_db.br_link[active_hf_idx].hfp.is_inband_ring)
            {
                //nothing
            }
            else
            {
                if (app_db.br_link[active_hf_idx].sco.conn_handle)// to phone
                {
                    app_audio_tone_type_play(TONE_HF_TRANSFER_TO_PHONE, false, false);
                    bt_hfp_audio_disconnect_req(app_db.br_link[active_hf_idx].bd_addr);
                }
                else //to bud
                {
                    app_audio_tone_type_play(TONE_HF_TRANSFER_TO_PHONE, false, false);
                    bt_hfp_audio_connect_req(app_db.br_link[active_hf_idx].bd_addr);
                }
            }
        }
        break;

    case MMI_HF_CANCEL_VOICE_DIAL:
        {
            app_mmi_hf_cancel_voice_dial();
        }
        break;

    case MMI_HF_LAST_NUMBER_REDIAL:
        {
            uint8_t app_idx = MAX_BR_LINK_NUM - 1;
            uint8_t link_number;
            link_number = app_connected_profile_link_num(HFP_PROFILE_MASK | HSP_PROFILE_MASK);
            if (link_number > 1)
            {
                if (app_hfp_cfg.enable_last_num_redial_always_by_first_phone)
                {
                    app_idx = app_db.first_hf_index;
                }
                else if (app_hfp_cfg.enable_last_num_redial_always_by_last_phone)
                {
                    app_idx = app_db.last_hf_index;
                }
            }
            else if (link_number == 1)
            {
                app_idx = app_hfp_get_active_idx();
            }

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if (app_db.br_link[app_idx].hfp.state == APP_HF_STATE_CONNECTED)
            {
                if (app_hfp_get_call_status() == APP_HFP_CALL_IDLE)
                {
                    bt_hfp_dial_last_number_req(app_db.br_link[app_idx].bd_addr);
                    app_audio_tone_type_play(TONE_HF_CALL_REDIAL, false, false);
                }
            }
            else
            {
                uint8_t bd_addr[6];

                if (app_bond_b2s_addr_get(1, bd_addr) == true)
                {
                    app_bt_policy_default_connect(bd_addr,
                                                  HSP_PROFILE_MASK | HFP_PROFILE_MASK, true);
                }
            }
        }
        break;

    case MMI_HF_JOIN_TWO_CALLS:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY ||
                (app_db.br_link[app_hfp_get_active_idx()].hfp.call_status <= APP_HFP_CALL_ACTIVE))
            {
                return;
            }

            bt_hfp_join_two_calls_req(app_db.br_link[active_hf_idx].bd_addr);
        }
        break;

    case MMI_HF_RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if (app_hfp_get_call_status() == APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT)
            {
                uint8_t inactive_hf_idx;

                for (inactive_hf_idx = 0; inactive_hf_idx < MAX_BR_LINK_NUM; inactive_hf_idx++)
                {
                    if (inactive_hf_idx != active_hf_idx)
                    {
                        if (app_db.br_link[inactive_hf_idx].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                        {
                            break;
                        }
                    }
                }

                if (inactive_hf_idx < MAX_BR_LINK_NUM)
                {
                    bt_hfp_call_terminate_req(app_db.br_link[active_hf_idx].bd_addr);
                    bt_hfp_call_answer_req(app_db.br_link[inactive_hf_idx].bd_addr);
                    app_hfp_set_active_idx(app_db.br_link[inactive_hf_idx].bd_addr);
                    app_bond_set_priority(app_db.br_link[inactive_hf_idx].bd_addr);
                }

            }
            else if (app_hfp_get_call_status() == APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                uint8_t inactive_hf_idx;

                for (inactive_hf_idx = 0; inactive_hf_idx < MAX_BR_LINK_NUM; inactive_hf_idx++)
                {
                    if (inactive_hf_idx != active_hf_idx)
                    {
                        if (app_db.br_link[inactive_hf_idx].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                        {
                            break;
                        }
                    }
                }

                if (inactive_hf_idx < MAX_BR_LINK_NUM)
                {
                    bt_hfp_call_terminate_req(app_db.br_link[active_hf_idx].bd_addr);
                    app_hfp_set_active_idx(app_db.br_link[inactive_hf_idx].bd_addr);
                    app_bond_set_priority(app_db.br_link[inactive_hf_idx].bd_addr);
                }
            }
            else
            {
                bt_hfp_release_active_call_accept_held_or_waiting_call_req(app_db.br_link[active_hf_idx].bd_addr);
            }
        }
        break;

    case MMI_HF_RELEASE_HELD_OR_WAITING_CALL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if (app_hfp_get_call_status() == APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT)
            {
                uint8_t inactive_hf_idx;

                for (inactive_hf_idx = 0; inactive_hf_idx < MAX_BR_LINK_NUM; inactive_hf_idx++)
                {
                    if (inactive_hf_idx != active_hf_idx)
                    {
                        if (app_db.br_link[inactive_hf_idx].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                        {
                            break;
                        }
                    }
                }

                if (inactive_hf_idx < MAX_BR_LINK_NUM)
                {
                    bt_hfp_call_terminate_req(app_db.br_link[inactive_hf_idx].bd_addr);
                }
            }
            else if (app_hfp_get_call_status() == APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                uint8_t inactive_hf_idx;

                for (inactive_hf_idx = 0; inactive_hf_idx < MAX_BR_LINK_NUM; inactive_hf_idx++)
                {
                    if (inactive_hf_idx != active_hf_idx)
                    {
                        if (app_db.br_link[inactive_hf_idx].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                        {
                            break;
                        }
                    }
                }

                if (inactive_hf_idx < MAX_BR_LINK_NUM)
                {
                    bt_hfp_call_terminate_req(app_db.br_link[inactive_hf_idx].bd_addr);
                }
            }
            else
            {
                bt_hfp_release_held_or_waiting_call_req(app_db.br_link[active_hf_idx].bd_addr);
            }
        }
        break;

    case MMI_HF_SWITCH_TO_SECOND_CALL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if (app_hfp_get_call_status() == APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT)
            {
                uint8_t inactive_hf_idx;

                for (inactive_hf_idx = 0; inactive_hf_idx < MAX_BR_LINK_NUM; inactive_hf_idx++)
                {
                    if (inactive_hf_idx != active_hf_idx)
                    {
                        if (app_db.br_link[inactive_hf_idx].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                        {
                            break;
                        }
                    }
                }

                if (inactive_hf_idx < MAX_BR_LINK_NUM)
                {
                    app_multi_active_hfp_transfer(inactive_hf_idx);
                    bt_hfp_call_answer_req(app_db.br_link[inactive_hf_idx].bd_addr);
                }
            }
            else if (app_hfp_get_call_status() == APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                uint8_t inactive_hf_idx;

                for (inactive_hf_idx = 0; inactive_hf_idx < MAX_BR_LINK_NUM; inactive_hf_idx++)
                {
                    if (inactive_hf_idx != active_hf_idx)
                    {
                        if (app_db.br_link[inactive_hf_idx].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                        {
                            break;
                        }
                    }
                }

                if (inactive_hf_idx < MAX_BR_LINK_NUM)
                {
                    app_multi_active_hfp_transfer(inactive_hf_idx);
                }
            }
            else
            {
                bt_hfp_hold_active_call_accept_held_or_waiting_call_req(app_db.br_link[active_hf_idx].bd_addr);
            }
        }
        break;

    case MMI_HF_QUERY_CURRENT_CALL_LIST:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            bt_hfp_current_call_list_req(app_db.br_link[active_hf_idx].bd_addr);
        }
        break;

    case MMI_HF_INITIATE_VOICE_DIAL:
        {
            mmi_voice_recognition();
        }
        break;

    case MMI_DEV_POWER_ON:
        {
            if (app_db.device_state == APP_DEVICE_STATE_OFF_ING)
            {
                if (app_bt_policy_get_b2s_connected_num())
                {
                    app_bt_policy_disconnect_all_link();
                }

                app_mmi_modify_reboot_check_times(REBOOT_CHECK_POWER_ON_MAX_TIMES);
            }
            else if (app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                if (!app_db.is_long_press_power_on_play_vp && !mp_hci_test_mode_is_running())
                {
                    app_audio_tone_type_play(TONE_POWER_ON, false, false);
                }
                app_db.is_long_press_power_on_play_vp = false;

                sys_mgr_power_on();

#if F_APP_CHARGING_CASE_CMD_TEST_SUPPORT
                app_device_one_wire_uart_cmd_test();
#endif
            }
        }
        break;

    case MMI_DEV_POWER_OFF:
        {
            APP_PRINT_TRACE2("app_mmi_handle_action: device_state %d, power_off_cause 0x%x",
                             app_db.device_state,
                             app_db.power_off_cause);
            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
#if F_APP_SAIYAN_MODE
                app_data_capture_saiyan_mode_ctl(0, 0x0);
#endif
                app_device_state_change(APP_DEVICE_STATE_OFF_ING);

                app_dlps_disable(APP_DLPS_ENTER_CHECK_WAIT_RESET);

                app_stop_timer(&timer_idx_reboot_check);
                app_mmi_reboot_check_timer_start(500);
                app_timer_register_pm_excluded(&timer_idx_reboot_check);

                if (mp_hci_test_mode_is_running())
                {
                    mp_hci_test_mmi_handle_action(action);
                }
                else
                {
                    app_sniff_mode_disable_all();
                    app_mmi_power_off();
                }
            }
        }
        break;

    case MMI_DEV_FORCE_ENTER_PAIRING_MODE:
    case MMI_DEV_ENTER_PAIRING_MODE:
        {
            if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
            {
                //Not allow to enter pairing mode when calling
                break;
            }
            else
            {
                if (app_db.device_state == APP_DEVICE_STATE_OFF)
                {
                    sys_mgr_power_on();
                }

                if (action == MMI_DEV_FORCE_ENTER_PAIRING_MODE)
                {
                    app_bt_policy_enter_pairing_mode(true, true);
                }
                else
                {
                    app_bt_policy_enter_pairing_mode(false, true);
                }
            }

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
//          app_gfps_force_enter_pairing_mode(GFPS_KEY_FORCE_ENTER_PAIR_MODE);
#endif
        }
        break;

    case MMI_DEV_EXIT_PAIRING_MODE:
        {
            app_bt_policy_exit_pairing_mode();

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
//          app_gfps_force_enter_pairing_mode(GFPS_EXIT_PAIR_MODE);
#endif
        }
        break;

    case MMI_DEV_MULTILINK_ON:
        {
            if (app_bt_policy_cfg_nv.enable_multi_link == 0)
            {
                app_multi_enable(true);
                app_audio_tone_type_play(TONE_DEV_MULTILINK_ON, false, false);
                //app_adp_factory_reset_link_dis(1000);
            }
        }
        break;

    case MMI_DEV_LINK_LAST_DEVICE:
        {
            uint32_t bond_flag;
            uint32_t plan_profs;

            uint8_t bond_num = app_bond_b2s_num_get();
            if (app_bt_policy_get_state() == BP_STATE_STANDBY)
            {
                for (uint8_t i = 1; i <= bond_num; i++)
                {
                    if (app_bond_b2s_addr_get(i, app_db.bt_addr_disconnect))
                    {
                        bt_bond_flag_get(app_db.bt_addr_disconnect, &bond_flag);
                        if (bond_flag & (BOND_FLAG_HFP | BOND_FLAG_HSP | BOND_FLAG_A2DP))
                        {
                            plan_profs = get_profs_by_bond_flag(bond_flag);

                            app_bt_policy_default_connect(app_db.bt_addr_disconnect,  plan_profs, false);
                            break;
                        }
                    }
                }
            }
        }
        break;

    case MMI_DEV_TOOLING_FACTORY_RESET:
    case MMI_DEV_CFG_FACTORY_RESET:
    case MMI_DEV_FACTORY_RESET:
    case MMI_DEV_FACTORY_RESET_BY_SPP:
    case MMI_DEV_PHONE_RECORD_RESET:
    case MMI_DEV_PHONE_RECORD_RESET_BY_SPP:
    case MMI_DEV_RESET_TO_UNINITIAL_STATE:
        {
            app_dlps_disable(APP_DLPS_ENTER_CHECK_WAIT_RESET);
            app_mmi_check_factory_reset_behavior(action);


            if (!app_db.factory_reset_ignore_led_and_vp)
            {
                app_audio_tone_type_play(TONE_FACTORY_RESET, false, false);
            }

            app_stop_timer(&timer_idx_reboot_check);
            app_mmi_reboot_check_timer_start(500);

            app_db.waiting_factory_reset = true;
            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                app_device_state_change(APP_DEVICE_STATE_OFF_ING);
                app_sniff_mode_disable_all();
                sys_mgr_power_off();
            }
        }
        break;

    case MMI_MEMORY_DUMP:
        {
            hal_debug_memory_dump();
        }
        break;

    case MMI_DEV_SPK_VOL_UP:
        {
            app_volume_up();
        }
        break;

    case MMI_DEV_SPK_VOL_DOWN:
        {
            app_volume_down();
        }
        break;

    case MMI_DEV_SPK_MUTE:
    case MMI_DEV_SPK_UNMUTE:
        {
            app_mmi_spk_mute_set(action);
        }
        break;

    case MMI_DEV_MIC_MUTE_UNMUTE:
        {
            if (app_audio_check_mic_mute_enable() == true)
            {
                if (app_audio_is_mic_mute())
                {
                    app_audio_set_mic_mute_status(0);
                }
                else
                {
                    app_audio_set_mic_mute_status(1);
                }
                app_hfp_mute_ctrl();
            }
        }
        break;

    case MMI_DEV_MIC_MUTE:
        {
            if (app_audio_check_mic_mute_enable() == true)
            {
                app_audio_set_mic_mute_status(1);
                app_hfp_mute_ctrl();

            }
        }
        break;

    case MMI_DEV_MIC_UNMUTE:
        {
            if (app_audio_check_mic_mute_enable() == true)
            {
                app_audio_set_mic_mute_status(0);
                app_hfp_mute_ctrl();
            }
        }
        break;

    case MMI_DEV_MIC_VOL_UP:
        {

        }
        break;

    case MMI_DEV_MIC_VOL_DOWN:
        {

        }
        break;

    case MMI_DEV_MIC_SWITCH:
        {
            app_audio_mic_switch(0);
        }
        break;

    case MMI_AUDIO_EQ_SWITCH:
        {
            uint8_t eq_num = eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode);

            if (eq_num != 0)
            {
                app_cfg_nv.eq_idx++;

                if (app_cfg_nv.eq_idx >= eq_num)
                {
                    app_cfg_nv.eq_idx = 0;
                }

                app_eq_sync_idx_accord_eq_mode(app_db.spk_eq_mode, app_cfg_nv.eq_idx);

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_eq_play_audio_eq_tone();
                }

#if F_APP_USER_EQ_SUPPORT
                if (app_eq_is_valid_user_eq_index(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx))
                {
                    app_eq_index_set(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx);
                    app_report_eq_idx(EQ_INDEX_REPORT_BY_UPDATE_EQ_INDEX);
                }
                else
#endif
                {
                    //not set default eq when audio connect,enable when set eq para from phone
                    if (!app_db.eq_ctrl_by_src
#if F_APP_LINEIN_SUPPORT
                        || (app_db.spk_eq_mode == LINE_IN_MODE)    /* line in eq not support adjust from phone */
#endif
                       )
                    {
                        app_eq_index_set(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx);
                    }
                    else
                    {
                        app_report_eq_idx(EQ_INDEX_REPORT_BY_SWITCH_EQ_INDEX);
                    }
                }
            }
            else
            {
                APP_PRINT_ERROR0("app_mmi_handle_action: eq need to enable");
            }
        }
        break;


    /*
    #if BISTO_FEATURE_SUPPORT
        case MMI_BISTO_FETCH_DOWN:
        case MMI_BISTO_FETCH_UP:
        case MMI_BISTO_FETCH_CLICK:
        case MMI_BISTO_PTT_DOWN:
        case MMI_BISTO_PTT_LONG_PRESS:
        case MMI_BISTO_PTT_UP:
        case MMI_BISTO_FETCH:
        case MMI_BISTO_PTT:
            app_bisto_btns_action((T_MMI_ACTION)action);
            break;
    #endif
    */

    case MMI_DEV_NEXT_VOICE_PROMPT_LANGUAGE:
        {
            uint8_t buf[2];

            if (voice_prompt_language_roll() == true)
            {
                app_cfg_nv.voice_prompt_language = voice_prompt_language_get();

                app_cfg_store(&app_cfg_nv.voice_prompt_language, 1);

                buf[0] = (uint8_t)app_cfg_nv.voice_prompt_language;
                buf[1] = voice_prompt_supported_languages_get();

                app_report_event_broadcast(EVENT_LANGUAGE_REPORT, buf, 2);

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_audio_tone_type_play(TONE_SWITCH_VP_LANGUAGE, false, false);
                }
            }
        }
        break;

    case MMI_OTA_OVER_BLE_START:
        {
            if (app_db.device_state != APP_DEVICE_STATE_OFF)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_audio_tone_type_play(TONE_OTA_OVER_BLE_START, false, false);
                }
            }
        }
        break;

    case MMI_START_ROLESWAP:
        {

        }
        break;

    case MMI_ENTER_SINGLE_MODE:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
            {
                break;
            }
            app_cfg_nv.bud_role = REMOTE_SESSION_ROLE_SINGLE;

            remote_session_role_set(REMOTE_SESSION_ROLE_SINGLE);
        }
        break;

    case MMI_A2DP_FOCUS_TOGGLE:
        {

        }
        break;


    case MMI_ENTER_DUT_FROM_SPP:
        {
            app_device_enter_dut_mode();
        }
        break;

    case MMI_DUT_TEST_MODE:
        {
            if (app_db.force_enter_dut_mode_when_acl_connected)
            {
                app_bt_policy_enter_dut_test_mode();

                /* WROKAROUND: need to stop le ext scan before entering dut test mode otherwise it would test fail. */
                le_ext_scan_stop();
                ble_ext_adv_mgr_disable_all(APP_STOP_ADV_CAUSE_DUT_TEST);

                app_cfg_nv.is_dut_test_mode = 1;
                app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);

                mp_hci_test_auto_enter_dut_init();
                mp_hci_test_set_mode(true);

                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_DUT_MODE);
                app_dlps_disable(APP_DLPS_ENTER_CHECK_DUT_TEST_MODE);

#if F_APP_DUT_MODE_AUTO_POWER_OFF
                if (app_cfg_const.enable_5_mins_auto_power_off_under_dut_test_mode)
                {
                    dut_test_start_auto_power_off_timer();
                }
#endif

                //Enable Device Under Test mode
                gap_vendor_cmd_req(0x1803, 0, NULL);
            }
            else if ((app_bt_policy_get_radio_mode() == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
                     && (app_bt_policy_get_state() != BP_STATE_LINKBACK)
                     && (app_bt_policy_get_b2s_connected_num() == 0)
                     && (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED) ||
                     (app_cfg_nv.trigger_dut_mode_from_power_off))
            {
                switch_into_single_tone_test_mode();
            }
        }
        break;



    case MMI_TAKE_PICTURE:
        {
            uint8_t keyboard_vol_up[4] = {0x02, 0, 0, 0x80};
            uint8_t keyboard_release[4] = {0x02, 0, 0, 0};

            bt_hid_device_interrupt_data_send(app_db.br_link[active_a2dp_idx].bd_addr,
                                              BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                              keyboard_vol_up, sizeof(keyboard_vol_up));
            bt_hid_device_interrupt_data_send(app_db.br_link[active_a2dp_idx].bd_addr,
                                              BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                              keyboard_release, sizeof(keyboard_release));
        }
        break;

    case MMI_AUDIO_SWITCH_CHANNEL:
        {
            uint8_t channel = 0;

            /*L<->R; R/L->(L+R)/2; (L+R)/2 -> L/R*/
            if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
            {
                if (app_cfg_nv.spk_channel == CHANNEL_L)
                {
                    channel = CHANNEL_R;
                }
                else if (app_cfg_nv.spk_channel == CHANNEL_R)
                {
                    channel = CHANNEL_LR_HELF;
                }
                else if (app_cfg_nv.spk_channel == CHANNEL_LR_HELF)
                {
                    channel = CHANNEL_L;
                }
            }
            else if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)
            {
                if (app_cfg_nv.spk_channel == CHANNEL_R)
                {
                    channel = CHANNEL_L;
                }
                else if (app_cfg_nv.spk_channel == CHANNEL_L)
                {
                    channel = CHANNEL_LR_HELF;
                }
                else if (app_cfg_nv.spk_channel == CHANNEL_LR_HELF)
                {
                    channel = CHANNEL_R;
                }
            }

            app_audio_speaker_channel_set(AUDIO_SPECIFIC_SET, channel);
        }
        break;


    case MMI_OUTPUT_INDICATION1_TOGGLE:
        {
            app_mmi_execute_output_indication_action(MMI_OUTPUT_INDICATION1_TOGGLE);

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                app_audio_tone_type_play(TONE_OUTPUT_INDICATION_1, false, false);
            }
        }
        break;

    case MMI_OUTPUT_INDICATION2_TOGGLE:
        {
            app_mmi_execute_output_indication_action(MMI_OUTPUT_INDICATION2_TOGGLE);
        }
        break;

    case MMI_OUTPUT_INDICATION3_TOGGLE:
        {
            app_mmi_execute_output_indication_action(MMI_OUTPUT_INDICATION3_TOGGLE);
        }
        break;

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    case MMI_FINDMY_PUT_SERIAL_NUMBER_STATE:
        {
            app_findmy_enter_serial_number_state();
        }
        break;
#endif

#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
    case MMI_GFPS_FINDER_STOP_RING:
        {
            app_gfps_finder_handle_mmi_action_stop_ring();
        }
        break;

    case MMI_GFPS_FINDER_STOP_ADV:
        {
            if (extend_app_cfg_const.gfps_finder_support)
            {
                APP_PRINT_INFO1("app_handle_mmi_message: MMI_GFPS_FINDER_STOP_ADV bud_role %d",
                                app_cfg_nv.bud_role);

                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY && extend_app_cfg_const.gfps_support)
                {
                    /*user keypress to disable the device (temporarily stop advertising) as required in the spec
                    to ensure compliance with unwanted tracking protections*/
                    gfps_finder_adv_stop(APP_STOP_ADV_CAUSE_GFPS_FINDER_KEY_PRESS);
                    gfps_finder_adv_set_key_stop_tracking_flag(true);
                }
            }
        }
        break;

    case MMI_DULT_ENTER_ID_READ_STATE:
        {
            if (extend_app_cfg_const.gfps_finder_support)
            {
                app_dult_id_read_state_enable();
            }
        }
        break;
#endif

#if F_GUI_BENCHMARK_SUPPORT
    case MMI_GUI_BENCHMARK_START:
        {
            app_benchmark_start();
        }
        break;
#endif




    default:
        break;
    }
}

bool app_mmi_is_local_execute(uint8_t action)
{
    bool ret = false;

    if (action == MMI_DEV_FACTORY_RESET)
    {
        ret = true;
    }

    return ret;
}
