/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include <string.h>

#include "stdlib.h"
#include "trace.h"
#include "app_timer.h"
#include "btm.h"
#include "audio.h"
#include "ringtone.h"
#include "voice_prompt.h"
#include "bt_avrcp.h"
#include "bt_hfp.h"
#include "bt_bond.h"
#include "bt_a2dp.h"
#include "audio_volume.h"
#include "bt_types.h"
#include "eq.h"
#include "eq_utils.h"
#include "app_dsp_cfg.h"
#include "app_tts.h"
#if F_APP_LINEIN_SUPPORT
#include "line_in.h"
#endif
#include "app_audio_cfg.h"
#include "app_audio_policy.h"
#include "app_main.h"
#include "app_report.h"

#include "app_cmd.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_hfp_cfg.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_avrcp.h"
#include "app_bond.h"
#include "app_eq_cfg.h"
#include "app_eq.h"
#include "audio_probe.h"
#include "app_bt_policy_api.h"
#include "app_auto_power_off.h"
#include "sysm.h"
#include "app_device.h"
#include "app_record.h"
#include "app_nrec.h"
#include "app_dlps.h"
#include "app_test.h"
#include "app_mmi.h"

#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif

#if (F_APP_SCO_XMIT_AG_SUPPORT || F_APP_SCO_XMIT_HF_SUPPORT)
#include "app_sco_xmit_mgr.h"
#endif

#if F_APP_A2DP_XMIT_SRC_SUPPORT
#include "app_a2dp_xmit_src.h"
#endif

#if (F_APP_A2DP_XMIT_SNK_SUPPORT || F_APP_A2DP_XMIT_SNK_LEA_SUPPORT)
#include "app_a2dp_xmit_mgr.h"
#endif

#if F_SOURCE_PLAY_SUPPORT
#include "app_src_play_a2dp.h"
#include "app_src_play_hfp.h"
#if F_APP_INTEGRATED_TRANSCEIVER
#include "app_src_play_pipe.h"
#endif
#endif

#include "app_audio_route.h"

#if F_APP_SIDETONE_SUPPORT
#include "app_sidetone.h"
#endif

#if F_APP_DATA_CAPTURE_SUPPORT
#include "app_data_capture_cs.h"
#endif

#if F_APP_SAIYAN_EQ_FITTING
#include "app_eq_fitting.h"
#endif

//for CMD_AUDIO_DSP_CTRL_SEND
#define CFG_H2D_DAC_GAIN                0x0C
#define CFG_H2D_VOICE_ADC_POST_GAIN     0x5D
#define CFG_H2D_APT_DAC_GAIN            0x4C

static T_APP_BUD_STREAM_STATE bud_stream_state = BUD_STREAM_STATE_IDLE;

#define APP_BT_AUDIO_A2DP_COUNT 7
#define MAX_MIN_VOL_REPEAT_INTERVAL  1500
#define A2DP_TRACK_PAUSE_INTERVAL    3000

typedef enum
{
    TIMER_LONG_PRESS_REPEAT = 0x00,
    TIMER_A2DP_TRACK_PAUSE  = 0x01,
    TIMER_TONE_VOL_ADJ_WAIT_SEC_RSP = 0x02,
    TIMER_TONE_VOL_GET_WAIT_SEC_RSP = 0x03,
    TIMER_IN_EAR_RESTORE_A2DP = 0x04,
    TIMER_MAX,
} T_APP_AUDIO_POLICY_TIMER;

typedef enum
{
    VOL_REPEAT_MAX = 0x00,
    VOL_REPEAT_MIN = 0x01,
} T_VOL_REPEAT;

static struct
{
    uint8_t id;
    uint8_t indices[TIMER_MAX];
} tmr = {};


static struct
{
    uint8_t is_mute;
    uint8_t switch_mode;
    uint8_t mp_dual_setting;
} mic = {};


static struct
{
    bool range_from_phone;
    bool max_min_tone_en_when_phone_ajust;
    uint8_t tone_min_level;
    uint8_t tone_max_level;
} vol =
{
    .range_from_phone = false,
    .max_min_tone_en_when_phone_ajust = false,
};

static const uint8_t codec_low_latency_table[9][LOW_LATENCY_LEVEL_MAX] =
{
    /* LOW_LATENCY_LEVEL1: 0;   LOW_LATENCY_LEVEL2: 1;  LOW_LATENCY_LEVEL3: 2;  LOW_LATENCY_LEVEL4: 3;  LOW_LATENCY_LEVEL5: 4. */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_PCM: 0 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_CVSD: 1 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_MSBC: 2 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_SBC: 3 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_AAC: 4 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_OPUS: 5 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_FLAC: 6 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_MP3: 7 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_LC3: 8 */
};

static void app_audio_judge_resume_a2dp_param(void)
{
    if (app_db.down_count >= APP_BT_AUDIO_A2DP_COUNT && app_db.recover_param == true)
    {
        app_db.down_count = 0;
        app_db.recover_param = false;
        //app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_ROLESWAP_DOWNSTREAM_CMPL);
    }
    else if (app_db.recover_param == true)
    {
        app_db.down_count++;
    }
}


static void app_audio_handle_vol_change(T_AUDIO_VOL_CHANGE vol_status)
{

}

void app_audio_set_connected_tone_need(bool need)
{
    app_db.connected_tone_need = need;

    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
    {
        //app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_CONNECTED_TONE_NEED);
    }
}

bool app_is_need_to_enable_circular_volume_up(void)
{
    if ((app_audio_cfg.enable_circular_vol_up == 1) ||
        ((app_audio_cfg.rws_circular_vol_up_when_solo_mode) &&
         (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)))
    {
        return true;
    }
    return false;
}

T_APP_BUD_STREAM_STATE app_audio_get_bud_stream_state(void)
{
    return bud_stream_state;
}

void app_audio_set_bud_stream_state(T_APP_BUD_STREAM_STATE state)
{
    APP_PRINT_TRACE1("app_audio_set_bud_stream_state: state %d", state);

    if (bud_stream_state != state)
    {
        bud_stream_state = state;
    }
}


void app_audio_update_detect_suspend_by_out_ear(bool flag)
{
    app_db.detect_suspend_by_out_ear = flag;

    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
    {
        //app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_SUSPEND_A2DP_BY_OUT_EAR);
    }
}

void app_audio_update_in_ear_recover_a2dp(bool flag)
{



    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        //app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_IN_EAR_RECOVER_A2DP);
    }
}


void app_audio_set_avrcp_status(uint8_t status)
{
    if (app_db.avrcp_play_status != status)
    {
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            if (status == BT_AVRCP_PLAY_STATUS_PLAYING)
            {
                app_audio_a2dp_play_status_update(APP_A2DP_STREAM_AVRCP_PLAY);

            }
            else if (app_db.avrcp_play_status ==
                     BT_AVRCP_PLAY_STATUS_PLAYING)  //other avrcp status dont issue playing LED
            {
                app_audio_a2dp_play_status_update(APP_A2DP_STREAM_AVRCP_PAUSE);
            }
        }

        app_db.avrcp_play_status = status;
    }
}

void app_audio_get_latency_value_by_level(T_AUDIO_FORMAT_TYPE format_type, uint8_t latency_level,
                                          uint16_t *latency)
{
    uint16_t latency_value;

    if (format_type == AUDIO_FORMAT_TYPE_AAC)
    {
        latency_value =
            codec_low_latency_table[AUDIO_FORMAT_TYPE_AAC][latency_level];
    }
    else
    {
        latency_value =
            codec_low_latency_table[AUDIO_FORMAT_TYPE_SBC][latency_level];
    }

    *latency = latency_value;
}

void app_audio_get_last_used_latency(uint16_t *latency)
{
    if (app_db.last_gaming_mode_audio_track_latency == 0)
    {
        uint16_t latency_value;

        // choose AUDIO_FORMAT_TYPE_AAC as default format type
        app_audio_get_latency_value_by_level(AUDIO_FORMAT_TYPE_AAC, app_cfg_nv.rws_low_latency_level_record,
                                             &latency_value);
        app_db.last_gaming_mode_audio_track_latency = latency_value;
    }

    *latency = app_db.last_gaming_mode_audio_track_latency;
}

uint16_t app_audio_set_latency(T_AUDIO_TRACK_HANDLE p_handle, uint8_t latency_level,
                               bool latency_fixed)
{
    uint16_t latency;
    T_AUDIO_TRACK_HANDLE *p_a2dp_handle = (T_AUDIO_TRACK_HANDLE *)p_handle;
    T_AUDIO_FORMAT_INFO format_info;

    audio_track_format_info_get(p_a2dp_handle, &format_info);
    app_audio_get_latency_value_by_level(format_info.type, latency_level, &latency);
    audio_track_latency_set(p_a2dp_handle, latency, GAMING_MODE_DYNAMIC_LATENCY_FIX);

    return latency;
}

uint16_t app_audio_update_audio_track_latency(T_AUDIO_TRACK_HANDLE p_handle, uint8_t latency_level)
{
    uint16_t latency_value;
    T_AUDIO_FORMAT_INFO format;
    T_AUDIO_TRACK_HANDLE *p_a2dp_handle = (T_AUDIO_TRACK_HANDLE *)p_handle;

    app_audio_get_last_used_latency(&latency_value);

    if (app_cfg_nv.rws_low_latency_level_record != latency_level) // need to update
    {
        if (audio_track_format_info_get(p_a2dp_handle, &format))
        {
            if (app_db.remote_is_8753bau)
            {
                latency_value = ULTRA_LOW_LATENCY_MS;
                audio_track_latency_set(p_a2dp_handle, latency_value,
                                        RTK_GAMING_MODE_DYNAMIC_LATENCY_FIX);
            }
            else
            {
                latency_value = app_audio_set_latency(p_a2dp_handle, latency_level,
                                                      GAMING_MODE_DYNAMIC_LATENCY_FIX);
            }
        }
        else // a2dp handle is not active
        {
            // choose AUDIO_FORMAT_TYPE_AAC as default format type
            app_audio_get_latency_value_by_level(AUDIO_FORMAT_TYPE_AAC, latency_level, &latency_value);
        }

        app_db.last_gaming_mode_audio_track_latency = latency_value;
        app_cfg_nv.rws_low_latency_level_record = latency_level;
        app_cfg_store(&app_cfg_nv.offset_listening_mode_cycle, 4);
    }
    else
    {
        APP_PRINT_TRACE1("app_audio_update_audio_track_latency: latency level %d, no need to update",
                         app_cfg_nv.rws_low_latency_level_record);
    }

    return latency_value;
}

void app_audio_report_low_latency_status(uint16_t latency_value)
{
    uint8_t buf[5];
    buf[0] = app_db.gaming_mode;
    buf[1] = (uint8_t)(latency_value);
    buf[2] = (uint8_t)(latency_value >> 8);
    buf[3] = LOW_LATENCY_LEVEL_MAX;
    buf[4] = app_cfg_nv.rws_low_latency_level_record;

    app_report_event_broadcast(EVENT_LOW_LATENCY_STATUS, buf, sizeof(buf));
}

void app_audio_update_latency_record(uint16_t latency_value)
{
    app_db.last_gaming_mode_audio_track_latency = latency_value;

    app_audio_report_low_latency_status(latency_value);
}


static void app_audio_policy_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            T_APP_BR_LINK *p_link;
            T_AUDIO_STREAM_TYPE stream_type;

            if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == false)
            {
                break;
            }

            switch (param->track_state_changed.state)
            {
            case AUDIO_TRACK_STATE_STARTED:
            case AUDIO_TRACK_STATE_RESTARTED:
                {
                    if ((param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED) &&
                        (stream_type == AUDIO_STREAM_TYPE_PLAYBACK))
                    {
                        p_link = &app_db.br_link[active_a2dp_idx];

#if F_APP_SAIYAN_MODE
                        app_data_capture_send_gain();
#endif

                        if (param->track_state_changed.handle == p_link->a2dp.track_handle)
                        {
                            T_AUDIO_STREAM_MODE  mode;
                            if (audio_track_mode_get(param->track_state_changed.handle, &mode) == true)
                            {
                                if (mode == AUDIO_STREAM_MODE_LOW_LATENCY || mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                                {
                                    app_start_timer(&tmr.indices[TIMER_A2DP_TRACK_PAUSE], "a2dp_track_pause",
                                                    tmr.id, TIMER_A2DP_TRACK_PAUSE, 0, false,
                                                    A2DP_TRACK_PAUSE_INTERVAL);
                                }
                            }
                        }
                    }

                    if (stream_type == AUDIO_STREAM_TYPE_VOICE)
                    {
                        p_link = &app_db.br_link[active_hf_idx];

                        if (p_link != NULL)
                        {
                            if (param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED)
                            {
                                if (app_dsp_cfg_sidetone.hw_enable)
                                {
                                    audio_volume_in_unmute(stream_type);
                                }

                                p_link->nrec_instance = app_nrec_attach(p_link->sco.track_handle, true);
                                p_link->sidetone_instance = app_sidetone_attach(p_link->sco.track_handle, app_dsp_cfg_sidetone);
                            }

                            p_link->sco.duplicate_fst_data = true;
                        }

                        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                        {

                            audio_volume_in_mute(AUDIO_STREAM_TYPE_VOICE);
                        }
                    }
                }
                break;

            case AUDIO_TRACK_STATE_STOPPED:
            case AUDIO_TRACK_STATE_PAUSED:
                {
                    if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                    {
                        app_stop_timer(&tmr.indices[TIMER_A2DP_TRACK_PAUSE]);
                    }

                    if (stream_type == AUDIO_STREAM_TYPE_VOICE)
                    {
                        p_link = &app_db.br_link[active_hf_idx];

                        if (p_link != NULL)
                        {
                            app_nrec_detach(p_link->sco.track_handle, p_link->nrec_instance);
                            app_sidetone_detach(p_link->sco.track_handle, p_link->sidetone_instance);
                        }

                        p_link = &app_db.br_link[active_a2dp_idx];

                        if (p_link->a2dp.is_streaming == true)
                        {
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
                }
                break;
            }
        }
        break;

    case AUDIO_EVENT_TRACK_DATA_IND:
        {

#if F_APP_DATA_CAPTURE_SUPPORT
            if (((app_data_capture_get_state() & DATA_CAPTURE_DATA_START_SCO_MODE) == 0) &&
                ((app_data_capture_get_state() & DATA_CAPTURE_DATA_SAIYAN_EXECUTING) == 0)
#if F_APP_SUPPORT_CAPTURE_ACOUSTICS_MP
                && ((app_data_capture_get_state() & DATA_CAPTURE_DATA_ACOUSTICS_MP_EXECUTING) == 0)
#endif
               )
#endif
            {
                {
                    T_APP_BR_LINK *p_link;

                    p_link = app_link_find_br_link(app_db.br_link[active_hf_idx].bd_addr);
                    if (p_link == NULL)
                    {
                        break;
                    }

                    // APP_PRINT_TRACE1("app_audio_policy_cback: data ind len %u", param->track_data_ind.len);
                    uint32_t timestamp;
                    uint16_t seq_num;
                    uint8_t frame_num;
                    T_AUDIO_STREAM_STATUS status;
                    uint16_t read_len;
                    uint8_t *buf;

                    buf = malloc(param->track_data_ind.len);

                    if (buf == NULL)
                    {
                        return;
                    }

                    if (audio_track_read(param->track_data_ind.handle,
                                         &timestamp,
                                         &seq_num,
                                         &status,
                                         &frame_num,
                                         buf,
                                         param->track_data_ind.len,
                                         &read_len) == true)
                    {
#if (F_APP_SCO_XMIT_HF_SUPPORT == 0 && F_APP_SCO_XMIT_AG_SUPPORT == 0)
                        if (p_link->sco.duplicate_fst_data)
                        {
                            p_link->sco.duplicate_fst_data = false;
                            bt_sco_data_send(p_link->bd_addr, seq_num - 1, buf, read_len);
                        }
                        bt_sco_data_send(p_link->bd_addr, seq_num, buf, read_len);
#endif
                    }
                    free(buf);
                }
            }
        }
        break;

    case AUDIO_EVENT_VOLUME_IN_MUTED:
        if ((param->volume_in_muted.type == AUDIO_STREAM_TYPE_VOICE)
           )
        {
            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_audio_tone_type_play(TONE_MIC_MUTE_ON, false, false);
            }
        }
        break;

    case AUDIO_EVENT_VOLUME_IN_UNMUTED:
        if ((param->volume_in_muted.type == AUDIO_STREAM_TYPE_VOICE)
           )
        {

        }
        break;

    case AUDIO_EVENT_TRACK_VOLUME_OUT_CHANGED:
        {
            T_AUDIO_STREAM_TYPE stream_type;
            uint8_t              pre_volume;
            uint8_t              cur_volume;
            T_AUDIO_VOL_CHANGE   vol_status;
            bool                 is_phone_real_level_0;

            if (audio_track_stream_type_get(param->track_volume_out_changed.handle, &stream_type) == false)
            {
                break;
            }

            pre_volume = param->track_volume_out_changed.prev_volume;
            cur_volume = param->track_volume_out_changed.curr_volume;
            vol_status = AUDIO_VOL_CHANGE_NONE;
            is_phone_real_level_0 = false;

            if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {


                if (cur_volume == app_dsp_cfg_vol.playback_volume_max)
                {
                    vol_status = AUDIO_VOL_CHANGE_MAX;
                }
                else if (cur_volume == app_dsp_cfg_vol.playback_volume_min)
                {
                    vol_status = AUDIO_VOL_CHANGE_MIN;
                }
                else
                {
                    if (cur_volume > pre_volume)
                    {
                        vol_status = AUDIO_VOL_CHANGE_UP;
                    }
                    else if (cur_volume < pre_volume)
                    {
                        vol_status = AUDIO_VOL_CHANGE_DOWN;
                    }
                }

                //if enable play_max_min_tone_when_adjust_vol_from_phone, just handle first vol change to max or min operation
                if (vol.range_from_phone == true)
                {
                    vol.max_min_tone_en_when_phone_ajust = false;

                    if (((vol_status == AUDIO_VOL_CHANGE_MAX) && (cur_volume == pre_volume + 1)) ||
                        ((vol_status == AUDIO_VOL_CHANGE_MIN) && (cur_volume == pre_volume - 1)) ||
                        ((vol_status == AUDIO_VOL_CHANGE_MIN) && is_phone_real_level_0))
                    {
                        vol.max_min_tone_en_when_phone_ajust = true;
                        vol.range_from_phone = false;
                    }
                }
            }
            else if (stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                if (cur_volume == app_dsp_cfg_vol.voice_out_volume_max)
                {
                    vol_status = AUDIO_VOL_CHANGE_MAX;
                }
                else if (cur_volume == app_dsp_cfg_vol.voice_out_volume_min)
                {
                    vol_status = AUDIO_VOL_CHANGE_MIN;
                }
                else
                {
                    if (cur_volume > pre_volume)
                    {
                        vol_status = AUDIO_VOL_CHANGE_UP;
                    }
                    else if (cur_volume < pre_volume)
                    {
                        vol_status = AUDIO_VOL_CHANGE_DOWN;
                    }
                }
            }

            app_audio_handle_vol_change(vol_status);
        }
        break;


    case AUDIO_EVENT_VOICE_PROMPT_STARTED:
        {
            app_db.tone_vp_status.state = APP_TONE_VP_STARTED;
            app_db.tone_vp_status.index = param->voice_prompt_started.index + VOICE_PROMPT_INDEX;

            if (((app_db.tone_vp_status.index == app_audio_tone_cfg.tone_power_off)) &&
                (app_db.device_state == APP_DEVICE_STATE_OFF_ING))
            {
                sys_mgr_power_off();
            }
        }
        break;

    case AUDIO_EVENT_VOICE_PROMPT_STOPPED:
        {
            app_db.tone_vp_status.state = APP_TONE_VP_STOP;
            app_db.tone_vp_status.index = TONE_INVALID_INDEX;
        }
        break;

    case AUDIO_EVENT_RINGTONE_STARTED:
        {
            app_db.tone_vp_status.state = APP_TONE_VP_STARTED;
            app_db.tone_vp_status.index = param->ringtone_started.index;

            if (((app_db.tone_vp_status.index == app_audio_tone_cfg.tone_power_off)) &&
                (app_db.device_state == APP_DEVICE_STATE_OFF_ING))
            {
                sys_mgr_power_off();
            }
        }
        break;

    case AUDIO_EVENT_RINGTONE_STOPPED:
        {
            app_db.tone_vp_status.state = APP_TONE_VP_STOP;
            app_db.tone_vp_status.index = TONE_INVALID_INDEX;
        }
        break;

    case AUDIO_EVENT_BUFFER_STATE_PLAYING:
        {
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_audio_policy_cback: event_type 0x%04x", event_type);
    }
}

static bool app_audio_set_voice_gain_when_sco_conn_cmpl(void *sco_track_handle,
                                                        uint8_t voice_gain_level)
{
    T_APP_HFP_CALL_STATUS call_status = app_hfp_get_call_status();

    if (sco_track_handle == NULL)
    {
        return false;
    }

    if (app_hfp_cfg.fixed_inband_tone_gain &&
        (call_status == APP_HFP_CALL_OUTGOING || call_status == APP_HFP_CALL_INCOMING))
    {
        voice_gain_level = app_hfp_cfg.inband_tone_gain_lv;
    }

    APP_PRINT_TRACE3("app_audio_set_voice_gain_when_sco_conn_cmpl: call_status %d level %d(%d)",
                     call_status, voice_gain_level, app_hfp_cfg.inband_tone_gain_lv);

    app_audio_vol_set(sco_track_handle, voice_gain_level);

    return true;
}

static void app_audio_save_a2dp_config(uint8_t *a2dp_cfg, T_AUDIO_FORMAT_INFO *format_info)
{
    T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL *cfg = (T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL *)a2dp_cfg;
    T_APP_BR_LINK *p_link = app_link_find_br_link(cfg->bd_addr);
    if (p_link == NULL)
    {
        return;
    }

    p_link->a2dp.codec.type = cfg->codec_type;

    if (cfg->codec_type == BT_A2DP_CODEC_TYPE_SBC)
    {
        format_info->type = AUDIO_FORMAT_TYPE_SBC;
        format_info->frame_num = 6;
        switch (cfg->codec_info.sbc.sampling_frequency)
        {
        case BT_A2DP_SBC_SAMPLING_FREQUENCY_16KHZ:
            format_info->attr.sbc.sample_rate = 16000;
            break;
        case BT_A2DP_SBC_SAMPLING_FREQUENCY_32KHZ:
            format_info->attr.sbc.sample_rate = 32000;
            break;
        case BT_A2DP_SBC_SAMPLING_FREQUENCY_44_1KHZ:
            format_info->attr.sbc.sample_rate = 44100;
            break;
        case BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ:
            format_info->attr.sbc.sample_rate = 48000;
            break;
        }

        switch (cfg->codec_info.sbc.channel_mode)
        {
        case BT_A2DP_SBC_CHANNEL_MODE_MONO:
            format_info->attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
            break;
        case BT_A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL:
            format_info->attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_DUAL;
            break;
        case BT_A2DP_SBC_CHANNEL_MODE_STEREO:
            format_info->attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_STEREO;
            break;
        case BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO:
            format_info->attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
            break;
        }
        switch (cfg->codec_info.sbc.block_length)
        {
        case BT_A2DP_SBC_BLOCK_LENGTH_4:
            format_info->attr.sbc.block_length = 4;
            break;
        case BT_A2DP_SBC_BLOCK_LENGTH_8:
            format_info->attr.sbc.block_length = 8;
            break;
        case BT_A2DP_SBC_BLOCK_LENGTH_12:
            format_info->attr.sbc.block_length = 12;
            break;
        case BT_A2DP_SBC_BLOCK_LENGTH_16:
            format_info->attr.sbc.block_length = 16;
            break;
        }
        switch (cfg->codec_info.sbc.subbands)
        {
        case BT_A2DP_SBC_SUBBANDS_4:
            format_info->attr.sbc.subband_num = 4;
            break;
        case BT_A2DP_SBC_SUBBANDS_8:
            format_info->attr.sbc.subband_num = 8;
            break;
        }
        switch (cfg->codec_info.sbc.allocation_method)
        {
        case BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS:
            format_info->attr.sbc.allocation_method = 0;
            break;
        case BT_A2DP_SBC_ALLOCATION_METHOD_SNR:
            format_info->attr.sbc.allocation_method = 1;
            break;
        }

        p_link->a2dp.codec.sbc.sampling_frequency = format_info->attr.sbc.sample_rate;
        p_link->a2dp.codec.sbc.channel_mode = format_info->attr.sbc.chann_mode;
        p_link->a2dp.codec.sbc.block_length = format_info->attr.sbc.block_length;
        p_link->a2dp.codec.sbc.subbands = format_info->attr.sbc.subband_num;
        p_link->a2dp.codec.sbc.allocation_method = format_info->attr.sbc.allocation_method;

        if (cfg->codec_info.sbc.max_bitpool == cfg->codec_info.sbc.min_bitpool)
        {
            format_info->attr.sbc.bitpool = cfg->codec_info.sbc.max_bitpool;
        }
    }
    else if (cfg->codec_type == BT_A2DP_CODEC_TYPE_AAC)
    {
        format_info->type = AUDIO_FORMAT_TYPE_AAC;
        format_info->frame_num = 1;
        switch (cfg->codec_info.aac.sampling_frequency)
        {
        case BT_A2DP_AAC_SAMPLING_FREQUENCY_8KHZ:
            format_info->attr.aac.sample_rate = 8000;
            break;
        case BT_A2DP_AAC_SAMPLING_FREQUENCY_16KHZ:
            format_info->attr.aac.sample_rate = 16000;
            break;

        case BT_A2DP_AAC_SAMPLING_FREQUENCY_44_1KHZ:
            format_info->attr.aac.sample_rate = 44100;
            break;

        case BT_A2DP_AAC_SAMPLING_FREQUENCY_48KHZ:
            format_info->attr.aac.sample_rate = 48000;
            break;

        case BT_A2DP_AAC_SAMPLING_FREQUENCY_96KHZ:
            format_info->attr.aac.sample_rate = 96000;
            break;
        default:
            break;
        }

        switch (cfg->codec_info.aac.channel_number)
        {
        case BT_A2DP_AAC_CHANNEL_NUMBER_1:
            format_info->attr.aac.chann_num = 1;
            break;

        case BT_A2DP_AAC_CHANNEL_NUMBER_2:
            format_info->attr.aac.chann_num = 2;
            break;

        default:
            break;
        }
        format_info->attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_LATM;
        format_info->attr.aac.object_type = (T_AUDIO_AAC_OBJECT_TYPE)cfg->codec_info.aac.object_type;
        format_info->attr.aac.vbr = cfg->codec_info.aac.vbr_supported;
        format_info->attr.aac.bitrate = cfg->codec_info.aac.bit_rate;

        p_link->a2dp.codec.aac.sampling_frequency = format_info->attr.aac.sample_rate;
        p_link->a2dp.codec.aac.channel_number = format_info->attr.aac.chann_num;
    }
#if F_APP_A2DP_CODEC_LDAC_SUPPORT
    else if (cfg->codec_type == BT_A2DP_CODEC_TYPE_LDAC)
    {
        format_info->type = AUDIO_FORMAT_TYPE_LDAC;
        format_info->frame_num = 1;
        switch (cfg->codec_info.ldac.sampling_frequency)
        {
        case BT_A2DP_LDAC_SAMPLING_FREQUENCY_44_1KHZ:
            format_info->attr.ldac.sample_rate = 44100;
            break;

        case BT_A2DP_LDAC_SAMPLING_FREQUENCY_48KHZ:
            format_info->attr.ldac.sample_rate = 48000;
            break;

        case BT_A2DP_LDAC_SAMPLING_FREQUENCY_88_2KHZ:
            format_info->attr.ldac.sample_rate = 88200;
            break;

        case BT_A2DP_LDAC_SAMPLING_FREQUENCY_96KHZ:
            format_info->attr.ldac.sample_rate = 96000;
            break;

        case BT_A2DP_LDAC_SAMPLING_FREQUENCY_176_4KHZ:
            format_info->attr.ldac.sample_rate = 176400;
            break;

        case BT_A2DP_LDAC_SAMPLING_FREQUENCY_192KHZ:
            format_info->attr.ldac.sample_rate = 192000;
            break;

        default:
            break;
        }

        switch (cfg->codec_info.ldac.channel_mode)
        {
        case BT_A2DP_LDAC_CHANNEL_MODE_MONO:
            format_info->attr.ldac.chann_mode = AUDIO_LDAC_CHANNEL_MODE_MONO;
            break;

        case BT_A2DP_LDAC_CHANNEL_MODE_DUAL:
            format_info->attr.ldac.chann_mode = AUDIO_LDAC_CHANNEL_MODE_DUAL;
            break;

        case BT_A2DP_LDAC_CHANNEL_MODE_STEREO:
            format_info->attr.ldac.chann_mode = AUDIO_LDAC_CHANNEL_MODE_STEREO;
            break;

        default:
            break;
        }

        p_link->a2dp.codec.ldac.sampling_frequency = format_info->attr.ldac.sample_rate;
        p_link->a2dp.codec.ldac.channel_mode = format_info->attr.ldac.chann_mode;
    }
#endif
    else
    {
        return;
    }
}

static void app_audio_a2dp_track_release(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;
    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->a2dp.track_handle != NULL)
        {
            audio_track_release(p_link->a2dp.track_handle);
            p_link->a2dp.track_handle = NULL;
        }

        if (p_link->eq.instance != NULL)
        {
            eq_release(p_link->eq.instance);
            p_link->eq.instance = NULL;
        }
        p_link->a2dp.muted = false;
    }
}

static void app_audio_sco_conn_cmpl_handle(uint8_t *bd_addr, uint8_t air_mode, uint8_t rx_pkt_len)
{
    uint8_t pair_idx_mapping;
    T_AUDIO_FORMAT_INFO format_info = {};
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);

    if (p_link == NULL)
    {
        return;
    }

    p_link->sco.seq_num = 0;

    app_bond_get_pair_idx_mapping(bd_addr, &pair_idx_mapping);

    if (air_mode == 3) /**< Air mode transparent data. */
    {
        format_info.type = AUDIO_FORMAT_TYPE_MSBC;
        format_info.frame_num = 1;
        format_info.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
        format_info.attr.msbc.sample_rate = 16000;
        format_info.attr.msbc.bitpool = 26;
        format_info.attr.msbc.allocation_method = 0;
        format_info.attr.msbc.subband_num = 8;
        format_info.attr.msbc.block_length = 15;
    }
    else if (air_mode == 2) /**< Air mode CVSD. */
    {
        format_info.type = AUDIO_FORMAT_TYPE_CVSD;
        format_info.frame_num = 1;
        format_info.attr.cvsd.chann_num = 1;
        format_info.attr.cvsd.sample_rate = 8000;
        if (rx_pkt_len == 30)
        {
            format_info.attr.cvsd.frame_duration = AUDIO_CVSD_FRAME_DURATION_3_75_MS;
        }
        else
        {
            format_info.attr.cvsd.frame_duration = AUDIO_CVSD_FRAME_DURATION_7_5_MS;
        }
    }
    else
    {
        return;
    }

#if (F_APP_SCO_XMIT_AG_SUPPORT || F_APP_SCO_XMIT_HF_SUPPORT)
    app_report_event(CMD_PATH_SPI, CMD_SCO_XMIT_CONFIG, 0, (uint8_t *)&format_info,
                     sizeof(T_AUDIO_FORMAT_INFO));
    app_sco_xmit_save_output_param(&format_info);
#endif

#if F_SOURCE_PLAY_SUPPORT
    app_src_play_save_hfp_format(&format_info, bd_addr);
#endif

    if (p_link->sco.track_handle != NULL)
    {
        audio_track_release(p_link->sco.track_handle);
        p_link->sco.track_handle = NULL;
    }

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (p_link->eq.voice_spk_eq_instance != NULL)
    {
        eq_release(p_link->eq.voice_spk_eq_instance);
        p_link->eq.voice_spk_eq_instance = NULL;
    }

    if (p_link->eq.voice_mic_eq_instance != NULL)
    {
        eq_release(p_link->eq.voice_mic_eq_instance);
        p_link->eq.voice_mic_eq_instance = NULL;
    }
#endif

#if F_APP_SUPPORT_CAPTURE_ACOUSTICS_MP
    if (app_data_capture_get_state() != 0)
    {
        return;
    }
#endif

#if (F_APP_SCO_XMIT_AG_SUPPORT || F_APP_SCO_XMIT_HF_SUPPORT || F_SOURCE_PLAY_SUPPORT)
    p_link->sco.track_handle = NULL;
#else
    uint32_t device = app_db.playback_device | AUDIO_DEVICE_IN_MIC;

    p_link->sco.track_handle = audio_track_create(AUDIO_STREAM_TYPE_VOICE,
                                                  AUDIO_STREAM_MODE_NORMAL,
                                                  AUDIO_STREAM_USAGE_SNOOP,
                                                  format_info,
                                                  app_cfg_nv.voice_gain_level[pair_idx_mapping],
                                                  app_dsp_cfg_vol.voice_volume_in_default,
                                                  device,
                                                  NULL,
                                                  NULL);
#endif

    if (p_link->sco.track_handle == NULL)
    {
        return;
    }

    if (app_link_get_b2s_link_num() > 1)
    {
        audio_track_latency_set(p_link->sco.track_handle, 150, false);
    }
    else
    {
        audio_track_latency_set(p_link->sco.track_handle, 100, false);
    }

    app_audio_set_voice_gain_when_sco_conn_cmpl(p_link->sco.track_handle,
                                                app_cfg_nv.voice_gain_level[pair_idx_mapping]);

#if F_APP_VOICE_SPK_EQ_SUPPORT
    app_eq_change_audio_eq_mode(true);

    app_eq_idx_check_accord_mode();

    p_link->eq.voice_spk_eq_instance = app_eq_create(EQ_CONTENT_TYPE_VOICE, EQ_STREAM_TYPE_VOICE,
                                                     SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx);

    if (p_link->eq.voice_spk_eq_instance != NULL)
    {
        eq_enable(p_link->eq.voice_spk_eq_instance);
        audio_track_effect_attach(p_link->sco.track_handle, p_link->eq.voice_spk_eq_instance);
    }

    p_link->eq.voice_mic_eq_instance = app_eq_create(EQ_CONTENT_TYPE_VOICE, EQ_STREAM_TYPE_VOICE,
                                                     MIC_SW_EQ, VOICE_MIC_MODE, app_cfg_nv.eq_idx);

    if (p_link->eq.voice_mic_eq_instance != NULL)
    {
        eq_enable(p_link->eq.voice_mic_eq_instance);
        audio_track_effect_attach(p_link->sco.track_handle, p_link->eq.voice_mic_eq_instance);
    }
#endif

    if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
    {
        audio_track_start(p_link->sco.track_handle);
    }
    else
    {
        p_link = &app_db.br_link[app_hfp_get_active_idx()];

        //HFP status may not be update by source, but SCO is created
        if (app_link_get_sco_conn_num() == 1)
        {
            app_hfp_set_active_idx(bd_addr);
        }
        else if (memcmp(bd_addr, p_link->bd_addr, 6) == 0)
        {
            audio_track_start(p_link->sco.track_handle);
        }
        else
        {
            APP_PRINT_TRACE2("app_audio_sco_conn_cmpl_handle: active link %s, current link %s",
                             TRACE_BDADDR(p_link->bd_addr),
                             TRACE_BDADDR(bd_addr));
        }

        if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
        {
            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_VOICE);
        }
    }
}

static void app_audio_a2dp_stream_start_handle(uint8_t *bd_addr, uint8_t *audio_cfg)
{
    T_APP_BR_LINK *p_link;
    T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL *cfg;
    uint8_t pair_idx_mapping;
    T_AUDIO_FORMAT_INFO format_info = {};
    T_AUDIO_STREAM_MODE mode = AUDIO_STREAM_MODE_NORMAL;
    uint16_t latency_value = A2DP_LATENCY_MS;
    uint8_t active_a2dp_idx = 0;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return;
    }

    app_bond_get_pair_idx_mapping(bd_addr, &pair_idx_mapping);
    cfg = (T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL *)audio_cfg;
    app_audio_save_a2dp_config((uint8_t *)cfg, &format_info);

#if (F_APP_A2DP_XMIT_SNK_SUPPORT || F_APP_A2DP_XMIT_SNK_LEA_SUPPORT)
    app_dlps_disable(APP_DLPS_ENTER_CHECK_XMIT);
    app_a2dp_xmit_mgr_save_a2dp_snk_param((uint8_t *)&format_info,
                                          sizeof(T_AUDIO_FORMAT_INFO));
    app_a2dp_xmit_mgr_report_a2dp_param();
    return;
#elif F_APP_INTEGRATED_TRANSCEIVER
    return;
#else

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

        app_audio_get_last_used_latency(&latency_value);
    }
    else
    {
        mode = AUDIO_STREAM_MODE_NORMAL;
    }

    if (p_link->a2dp.track_handle)
    {
        audio_track_release(p_link->a2dp.track_handle);
    }

    if (p_link->eq.instance != NULL)
    {
        eq_release(p_link->eq.instance);
        p_link->eq.instance = NULL;
    }

    {
        T_AUDIO_STREAM_USAGE stream;
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
        {
            stream = AUDIO_STREAM_USAGE_SNOOP;
        }
        else
        {
            if ((p_link->acl.role == BT_LINK_ROLE_MASTER) &&
                (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
            {
                stream = AUDIO_STREAM_USAGE_LOCAL;
                app_db.src_roleswitch_fail_no_sniffing = true;
            }
            else
            {
                stream = AUDIO_STREAM_USAGE_SNOOP;
            }

        }

        // APP_PRINT_INFO1("app_audio_a2dp_stream_start_handle, volume out %d",
        //                 app_cfg_nv.audio_gain_level[pair_idx_mapping]);
        uint32_t device = app_db.playback_device;

        p_link->a2dp.track_handle = audio_track_create(AUDIO_STREAM_TYPE_PLAYBACK,
                                                       mode,
                                                       stream,
                                                       format_info,
                                                       app_cfg_nv.audio_gain_level[pair_idx_mapping],
                                                       0,
                                                       device,
                                                       NULL,
                                                       NULL);
    }

    if (p_link->a2dp.track_handle != NULL)
    {
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
            }
            else
            {
                latency_value = app_audio_set_latency(p_link->a2dp.track_handle,
                                                      app_cfg_nv.rws_low_latency_level_record,
                                                      GAMING_MODE_DYNAMIC_LATENCY_FIX);
                bt_a2dp_stream_delay_report_req(p_link->bd_addr, latency_value);
            }

            app_audio_update_latency_record(latency_value);
        }
        else
        {
            audio_track_latency_set(p_link->a2dp.track_handle, A2DP_LATENCY_MS,
                                    NORMAL_MODE_DYNAMIC_LATENCY_FIX);
            bt_a2dp_stream_delay_report_req(p_link->bd_addr, A2DP_LATENCY_MS);
        }

        app_audio_vol_set(p_link->a2dp.track_handle, app_cfg_nv.audio_gain_level[pair_idx_mapping]);

        app_eq_idx_check_accord_mode();
        p_link->eq.instance = app_eq_create(EQ_CONTENT_TYPE_AUDIO, EQ_STREAM_TYPE_AUDIO, SPK_SW_EQ,
                                            app_db.spk_eq_mode, app_cfg_nv.eq_idx);
        p_link->eq.audio_eq_enabled = false;

        if (p_link->eq.instance != NULL)
        {
            //not set default eq when audio connect,enable when set eq para from SRC
            if (!app_db.eq_ctrl_by_src)
            {
                app_eq_audio_eq_enable(&p_link->eq.instance, &p_link->eq.audio_eq_enabled);
            }
#if F_APP_USER_EQ_SUPPORT
            else if (app_eq_is_valid_user_eq_index(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx))
            {
                app_eq_audio_eq_enable(&p_link->eq.instance, &p_link->eq.audio_eq_enabled);
            }
#endif

            audio_track_effect_attach(p_link->a2dp.track_handle, p_link->eq.instance);
        }

#if F_APP_MULTILINK_ENABLE
        if (p_link->id != active_a2dp_idx)
        {
            app_a2dp_set_active_idx(p_link->id);
            active_a2dp_idx = app_a2dp_get_active_idx();
        }
#endif

        if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
        {
            audio_track_start(p_link->a2dp.track_handle);
        }
        else
        {
            if (memcmp(app_db.br_link[active_a2dp_idx].bd_addr, p_link->bd_addr, 6) != 0)
            {
                APP_PRINT_TRACE1("app_audio_a2dp_stream_start_handle: not match active a2dp index %d",
                                 active_a2dp_idx);
            }

            else
            {
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

            if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
#if F_APP_USER_EQ_SUPPORT
                if (app_eq_is_valid_user_eq_index(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx))
                {
                    app_report_eq_idx(EQ_INDEX_REPORT_BY_GET_UNSAVED_EQ);
                }
                else
#endif
                {
                    app_report_eq_idx(EQ_INDEX_REPORT_BY_PLAYING);
                }
            }
        }
    }
#endif
}

void app_audio_a2dp_play_status_update(T_APP_A2DP_STREAM_EVENT event)
{
    bool need_to_sync = false;

    if (app_db.a2dp_play_status == false)
    {
        if (event == APP_A2DP_STREAM_AVRCP_PLAY || event == APP_A2DP_STREAM_A2DP_START)
        {
            app_db.a2dp_play_status = true;
            need_to_sync = true;
        }
    }
    else
    {
        if (event == APP_A2DP_STREAM_AVRCP_PAUSE || event == APP_A2DP_STREAM_A2DP_STOP)
        {
            app_db.a2dp_play_status = false;
            need_to_sync = true;
        }
    }

    if (need_to_sync)
    {
        if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
            (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
        {
            //app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_A2DP_PLAY_STATUS);
        }
    }

    APP_PRINT_TRACE2("app_audio_a2dp_play_status_update: event %d status %d", event,
                     app_db.a2dp_play_status);
}

static void app_audio_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    switch (event_type)
    {
    case BT_EVENT_SCO_CONN_CMPL:
        {
            int8_t temp_tx_power = app_audio_cfg.bud_to_phone_sco_tx_power_control;

            if (temp_tx_power > 7)
            {
                temp_tx_power -= 16;
            }

            bt_link_tx_power_set(param->sco_conn_cmpl.bd_addr, temp_tx_power);

            if (param->sco_conn_cmpl.cause != 0)
            {
                break;
            }
            app_audio_sco_conn_cmpl_handle(param->sco_conn_cmpl.bd_addr, param->sco_conn_cmpl.air_mode,
                                           param->sco_conn_cmpl.rx_pkt_len);
#if F_APP_SCO_XMIT_AG_SUPPORT
            app_sco_xmit_param_recfg();
#endif
        }
        break;

    case BT_EVENT_SCO_DATA_IND:
        {
            uint16_t written_len;
            T_APP_BR_LINK *p_link;
            T_APP_BR_LINK *p_active_link = &app_db.br_link[active_hf_idx];
            T_AUDIO_STREAM_STATUS  status;

            p_link = app_link_find_br_link(param->sco_data_ind.bd_addr);
            if (p_link == NULL)
            {
                break;
            }

            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (memcmp(p_active_link->bd_addr, p_link->bd_addr, 6) != 0)
                {
                    break;
                }
            }

            p_link->sco.seq_num++;

#if (F_APP_SCO_XMIT_HF_SUPPORT || F_APP_SCO_XMIT_AG_SUPPORT)
            uint8_t *p_buf = malloc(param->sco_data_ind.length + 1);
            if (p_buf != NULL)
            {
                p_buf[0] = param->sco_data_ind.status;
                memcpy(&p_buf[1], param->sco_data_ind.p_data, param->sco_data_ind.length);
                app_report_event(CMD_PATH_SPI, CMD_SCO_XMIT_AUDIO, 0, p_buf, param->sco_data_ind.length + 1);
                free(p_buf);
            }
#elif F_SOURCE_PLAY_SUPPORT

#else
            if (param->sco_data_ind.status == BT_SCO_PKT_STATUS_OK)
            {
                status = AUDIO_STREAM_STATUS_CORRECT;
            }
            else
            {
                status = AUDIO_STREAM_STATUS_LOST;
            }
            audio_track_write(p_link->sco.track_handle, param->sco_data_ind.bt_clk,
                              p_link->sco.seq_num,
                              status,
                              1,
                              param->sco_data_ind.p_data,
                              param->sco_data_ind.length,
                              &written_len);
#endif

        }
        break;

    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
            T_AUDIO_FORMAT_INFO format_info = {};
            app_audio_save_a2dp_config((uint8_t *)&param->a2dp_config_cmpl, &format_info);
#if F_APP_A2DP_XMIT_SRC_SUPPORT
            app_a2dp_xmit_src_save_a2dp_out_param((uint8_t *)&format_info);
#endif

#if F_SOURCE_PLAY_SUPPORT
            app_src_play_save_a2dp_format(&format_info, param->a2dp_config_cmpl.bd_addr,
                                          param->a2dp_config_cmpl.role);
#endif
        }
        break;

    case BT_EVENT_A2DP_SNIFFING_CONN_CMPL:
        {
            T_AUDIO_FORMAT_INFO format_info = {};
            app_audio_save_a2dp_config((uint8_t *)&param->a2dp_sniffing_conn_cmpl, &format_info);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            app_audio_a2dp_stream_start_handle(param->a2dp_stream_start_ind.bd_addr,
                                               (uint8_t *)&param->a2dp_stream_start_ind);
        }
        break;

    case BT_EVENT_A2DP_STREAM_DATA_IND:
        {
            uint16_t written_len;
            T_AUDIO_STREAM_MODE  mode;
            T_AUDIO_TRACK_STATE state;
            T_APP_BR_LINK *p_link;
            p_link = app_link_find_br_link(param->a2dp_stream_data_ind.bd_addr);
            if (p_link == NULL)
            {
                break;
            }
#if F_APP_DUAL_AUDIO_EFFECT
            if (dual_audio_get_a2dp_ignore())
            {
                break;
            }
#endif
#if F_APP_ANC_SUPPORT
            if (app_anc_ramp_tool_is_busy() || app_anc_tool_burn_gain_is_busy())
            {
                break;
            }
#endif
#if F_APP_LOCAL_PLAYBACK_SUPPORT
            if (app_db.sd_playback_switch == 1)
            {
                app_db.wait_resume_a2dp_flag = true;
                break;
            }
#endif


#if (F_APP_A2DP_XMIT_SNK_SUPPORT || F_APP_A2DP_XMIT_SNK_LEA_SUPPORT)
            T_A2DP_XMIT_AUDIO_INFO audio_info =
            {
                .frame_num  = param->a2dp_stream_data_ind.frame_num,
                .len = param->a2dp_stream_data_ind.len,
            };
            uint16_t total_len = sizeof(T_A2DP_XMIT_AUDIO_INFO) + audio_info.len;
            uint8_t *data_buf = malloc(total_len);
            if (data_buf == NULL)
            {
                APP_PRINT_ERROR0("app_a2dp_xmit_mgr_report_a2dp_data buf alloc fail");
                break;
            }

            memcpy(data_buf, &audio_info, sizeof(T_A2DP_XMIT_AUDIO_INFO));
            memcpy(&data_buf[sizeof(T_A2DP_XMIT_AUDIO_INFO)], param->a2dp_stream_data_ind.payload,
                   audio_info.len);
            app_a2dp_xmit_mgr_report_a2dp_data(data_buf, total_len);
            free(data_buf);
#endif

#if F_APP_INTEGRATED_TRANSCEIVER
            app_src_play_pipe_handle_a2dp_stream_data(param->a2dp_stream_data_ind.payload,
                                                      param->a2dp_stream_data_ind.len, param->a2dp_stream_data_ind.frame_num,
                                                      param->a2dp_stream_data_ind.seq_num);
#endif
            if (audio_track_mode_get(p_link->a2dp.track_handle, &mode) == true)
            {
                {
                    if (mode == AUDIO_STREAM_MODE_LOW_LATENCY || mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                    {
                        audio_track_state_get(p_link->a2dp.track_handle, &state);
                        if (state == AUDIO_TRACK_STATE_PAUSED)
                        {
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
                        else
                        {
                            app_start_timer(&tmr.indices[TIMER_A2DP_TRACK_PAUSE], "a2dp_track_pause",
                                            tmr.id, TIMER_A2DP_TRACK_PAUSE, 0, false,
                                            A2DP_TRACK_PAUSE_INTERVAL);
                            app_audio_judge_resume_a2dp_param();
                            audio_track_write(p_link->a2dp.track_handle, param->a2dp_stream_data_ind.bt_clock,
                                              param->a2dp_stream_data_ind.seq_num,
                                              AUDIO_STREAM_STATUS_CORRECT,
                                              param->a2dp_stream_data_ind.frame_num,
                                              param->a2dp_stream_data_ind.payload,
                                              param->a2dp_stream_data_ind.len,
                                              &written_len);
                        }
                    }
                    else
                    {
                        app_stop_timer(&tmr.indices[TIMER_A2DP_TRACK_PAUSE]);
                        app_audio_judge_resume_a2dp_param();
                        audio_track_write(p_link->a2dp.track_handle, param->a2dp_stream_data_ind.bt_clock,
                                          param->a2dp_stream_data_ind.seq_num,
                                          AUDIO_STREAM_STATUS_CORRECT,
                                          param->a2dp_stream_data_ind.frame_num,
                                          param->a2dp_stream_data_ind.payload,
                                          param->a2dp_stream_data_ind.len,
                                          &written_len);
                    }
                }
            }
            app_bt_policy_set_first_connect_sync_default_vol_flag(false);
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            app_audio_a2dp_track_release(param->a2dp_stream_stop.bd_addr);
#if (F_APP_A2DP_XMIT_SNK_SUPPORT || F_APP_A2DP_XMIT_SNK_LEA_SUPPORT)
            app_dlps_enable(APP_DLPS_ENTER_CHECK_XMIT);
#endif
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            app_audio_a2dp_track_release(param->a2dp_stream_close.bd_addr);
#if (F_APP_A2DP_XMIT_SNK_SUPPORT || F_APP_A2DP_XMIT_SNK_LEA_SUPPORT)
            app_dlps_enable(APP_DLPS_ENTER_CHECK_XMIT);
#endif
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
        }
        break;

    case BT_EVENT_AVRCP_TRACK_CHANGED:
    case BT_EVENT_AVRCP_ABSOLUTE_VOLUME_SET:
        {

        }
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_RSP:
    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED:
        {
            if (memcmp(app_db.br_link[active_a2dp_idx].bd_addr,
                       param->avrcp_play_status_changed.bd_addr, 6) == 0) //Only handle active a2dp index AVRCP status
            {
                if (event_type == BT_EVENT_AVRCP_PLAY_STATUS_CHANGED)
                {

                }

                if (app_db.avrcp_play_status == param->avrcp_play_status_changed.play_status)
                {
                    break;
                }

                app_audio_set_avrcp_status(param->avrcp_play_status_changed.play_status);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {

        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {

        }
        break;

    case BT_EVENT_A2DP_CONN_CMPL:
        {
            //app_bt_sniffing_process(param->a2dp_conn_cmpl.bd_addr);

            T_APP_BR_LINK *p_link;
            p_link = app_link_find_br_link(param->a2dp_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                p_link->a2dp.muted = false;
            }
        }
        break;


    case BT_EVENT_HFP_CONN_CMPL:
        {
            //app_bt_sniffing_process(param->hfp_conn_cmpl.bd_addr);
        }
        break;

    case BT_EVENT_HFP_SUPPORTED_FEATURES_IND:
        {
            T_APP_BR_LINK *p_link;
            p_link = app_link_find_br_link(param->hfp_supported_features_ind.bd_addr);
            if (p_link != NULL)
            {
                p_link->hfp.remote_brsf_capability = param->hfp_supported_features_ind.ag_bitmap;

                if (param->hfp_supported_features_ind.ag_bitmap & BT_HFP_HF_REMOTE_CAPABILITY_INBAND_RINGING)
                {
                    p_link->hfp.is_inband_ring = true;
                }

                APP_PRINT_INFO1("app_audio_bt_cback: BT_EVENT_HFP_SUPPORTED_FEATURES_IND, remote_hfp_brsf_capability 0x%04x",
                                p_link->hfp.remote_brsf_capability);
            }
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            app_audio_a2dp_track_release(param->a2dp_disconn_cmpl.bd_addr);
#if (F_APP_A2DP_XMIT_SNK_SUPPORT || F_APP_A2DP_XMIT_SNK_LEA_SUPPORT)
            app_dlps_enable(APP_DLPS_ENTER_CHECK_XMIT);
#endif
        }
        break;


    case BT_EVENT_SCO_DISCONNECTED:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->sco_disconnected.bd_addr);

            bt_link_tx_power_set(param->sco_disconnected.bd_addr, 0);

            if (p_link != NULL)
            {
                if (p_link->sco.track_handle != NULL)
                {
                    audio_track_release(p_link->sco.track_handle);
                    p_link->sco.track_handle = NULL;
                }

                app_nrec_detach(p_link->sco.track_handle, p_link->nrec_instance);
                app_sidetone_detach(p_link->sco.track_handle, p_link->sidetone_instance);

#if F_APP_VOICE_SPK_EQ_SUPPORT
                if (p_link->eq.voice_spk_eq_instance != NULL)
                {
                    eq_release(p_link->eq.voice_spk_eq_instance);
                    p_link->eq.voice_spk_eq_instance = NULL;
                }

                if (p_link->eq.voice_mic_eq_instance != NULL)
                {
                    eq_release(p_link->eq.voice_mic_eq_instance);
                    p_link->eq.voice_mic_eq_instance = NULL;
                }
#endif

                p_link->sco.muted = 0;
            }

#if F_APP_VOICE_SPK_EQ_SUPPORT
            app_eq_change_audio_eq_mode(true);
#endif

            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                p_link = &app_db.br_link[active_a2dp_idx];
                if (p_link == NULL || p_link->a2dp.track_handle == NULL)
                {
                    break;
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
        break;


    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_audio_bt_cback: event_type 0x%04x", event_type);
    }
}

static void app_audio_policy_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_audio_policy_timeout_cb: timer_evt %d, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case TIMER_A2DP_TRACK_PAUSE:
        {
            T_APP_BR_LINK *p_link;
            p_link = &app_db.br_link[app_a2dp_get_active_idx()];
            app_stop_timer(&tmr.indices[TIMER_A2DP_TRACK_PAUSE]);
            audio_track_pause(p_link->a2dp.track_handle);
        }
        break;

    case TIMER_LONG_PRESS_REPEAT:
        {
            if (app_db.play_min_max_vol_tone == true)
            {
                if (param == VOL_REPEAT_MAX) // vol_max
                {
                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                        app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                    {
                        app_audio_tone_type_play(TONE_VOL_MAX, false, false);
                    }
                }
                else if (param == VOL_REPEAT_MIN) // vol_min
                {
                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                        app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                    {
                        app_audio_tone_type_play(TONE_VOL_MIN, false, false);
                    }
                }
                app_db.play_min_max_vol_tone = false;
            }
            else
            {
                app_stop_timer(&tmr.indices[TIMER_LONG_PRESS_REPEAT]);
            }
        }
        break;

    case TIMER_IN_EAR_RESTORE_A2DP:
        {
            app_db.detect_suspend_by_out_ear = false;
            app_audio_update_in_ear_recover_a2dp(false);
        }
        break;

    default:
        break;
    }
}


void app_audio_cmd_cback(uint8_t msg_type, uint8_t *buf, uint16_t len)
{
    switch (msg_type)
    {
    case APP_AUDIO_CMD_MSG_SEND_DSP_CMD:
        {
            audio_probe_dsp_send(buf, len);
        }
        break;

    default:
        break;
    }

    return;
}

uint8_t app_audio_is_mic_mute(void)
{
    return mic.is_mute;
}

void app_audio_set_mic_mute_status(uint8_t status)
{
    mic.is_mute = status;
}

uint8_t app_audio_mic_switch(uint8_t param)
{
    uint8_t mic_switch_cmd[8];
    memset(mic_switch_cmd, 0, 8);

    if (param)
    {
        mic.switch_mode = param;
    }
    else
    {
        mic.switch_mode++;
    }

    mic_switch_cmd[0] = AUDIO_PROBE_TEST_MODE;
    mic_switch_cmd[2] = 0x1;

    switch (mic.switch_mode)
    {
    case 1:
        {
            mic_switch_cmd[4] = 0x3; // mic test + effect off
            mic_switch_cmd[6] = 0x1; // mic0 test
        }
        break;

    case 2:
        {
            mic_switch_cmd[4] = 0x3; // mic test + effect off
            mic_switch_cmd[6] = 0x2; // mic1 test
        }
        break;

    case 3:
        {
            // normal mode
            mic.switch_mode = 0;
        }
        break;

    default:
        break;
    }

    audio_probe_dsp_send(mic_switch_cmd, 8);

    return mic.switch_mode;
}

T_AUDIO_MP_DUAL_MIC_SETTING app_audio_mp_dual_mic_setting_check(uint8_t *ptr)
{
    T_AUDIO_MP_DUAL_MIC_SETTING result = AUDIO_MP_DUAL_MIC_SETTING_INVALID;
    uint8_t mp_dual_mic_l_pri_enable = ptr[0];
    uint8_t mp_dual_mic_l_sec_enable = ptr[1];
    uint8_t mp_dual_mic_r_pri_enable = ptr[2];
    uint8_t mp_dual_mic_r_sec_enable = ptr[3];

    /*  mp_dual_mic_setting bitmap
        bit0: mp_dual_mic_l_pri_enable, 0: disable; 1:enable.
        bit1: mp_dual_mic_l_sec_enable
        bit2: mp_dual_mic_r_pri_enable
        bit3: mp_dual_mic_r_sec_enable
        bit4~7: mp_dual_mic_setting, refer T_AUDIO_MP_DUAL_MIC_SETTING
    */
    mic.mp_dual_setting = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        if (ptr[i] == 1)
        {
            mic.mp_dual_setting |= BIT(i);
        }
    }

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        result = AUDIO_MP_DUAL_MIC_SETTING_VALID;
        mic.mp_dual_setting |= (result << 4);
    }
    else
    {
        if ((mp_dual_mic_l_pri_enable || mp_dual_mic_l_sec_enable) &&
            !(mp_dual_mic_r_pri_enable || mp_dual_mic_r_sec_enable))
        {
            if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
            {
                result = AUDIO_MP_DUAL_MIC_SETTING_VALID;
            }
            else
            {
                result = AUDIO_MP_DUAL_MIC_SETTING_ROLE_SWAP;
            }
        }
        else if ((mp_dual_mic_r_pri_enable | mp_dual_mic_r_sec_enable) &&
                 !(mp_dual_mic_l_pri_enable | mp_dual_mic_l_sec_enable))
        {
            if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)
            {
                result = AUDIO_MP_DUAL_MIC_SETTING_VALID;
            }
            else
            {
                result = AUDIO_MP_DUAL_MIC_SETTING_ROLE_SWAP;
            }
        }
        else
        {
            //this condition not support
            result = AUDIO_MP_DUAL_MIC_SETTING_INVALID;
        }

        mic.mp_dual_setting |= (result << 4);
//        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_MP_DUAL_MIC_SETTING);
    }

    APP_PRINT_TRACE2("app_audio_mp_dual_mic_setting_check %x, result = %x", mic.mp_dual_setting,
                     result);

    return result;
}

void app_audio_mp_dual_mic_switch_action(void)
{
    uint8_t mic_switch_param;

    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)
    {
        if ((mic.mp_dual_setting & AUDIO_MP_DUAL_MIC_R_PRI_ENABLE) &&
            (mic.mp_dual_setting & AUDIO_MP_DUAL_MIC_R_SEC_ENABLE))
        {
            //normal
            mic_switch_param = 3;
        }
        else if (mic.mp_dual_setting & AUDIO_MP_DUAL_MIC_R_PRI_ENABLE)
        {
            //non swap
            mic_switch_param = 1;
        }
        else if (mic.mp_dual_setting & AUDIO_MP_DUAL_MIC_R_SEC_ENABLE)
        {
            //swap
            mic_switch_param = 2;
        }
        else
        {
            //all mute
            mic_switch_param = 0;
        }
    }
    else if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
    {
        if ((mic.mp_dual_setting & AUDIO_MP_DUAL_MIC_L_PRI_ENABLE) &&
            (mic.mp_dual_setting & AUDIO_MP_DUAL_MIC_L_SEC_ENABLE))
        {
            //normal
            mic_switch_param = 3;
        }
        else if (mic.mp_dual_setting & AUDIO_MP_DUAL_MIC_L_PRI_ENABLE)
        {
            //non swap
            mic_switch_param = 1;
        }
        else if (mic.mp_dual_setting & AUDIO_MP_DUAL_MIC_L_SEC_ENABLE)
        {
            //swap
            mic_switch_param = 2;
        }
        else
        {
            //all mute
            mic_switch_param = 0;
        }
    }

    APP_PRINT_TRACE2("app_audio_mp_dual_mic_switch_action setting = %x, mic_switch_param = %x",
                     mic.mp_dual_setting, mic_switch_param);

    if (mic_switch_param)
    {
        app_mmi_handle_action(MMI_DEV_MIC_UNMUTE);
        app_audio_mic_switch(mic_switch_param);
    }
    else
    {
        app_mmi_handle_action(MMI_DEV_MIC_MUTE);
    }

    mic.mp_dual_setting = 0;

#if F_APP_TEST_SUPPORT
    uint8_t success_param = 0;
    uint8_t active_idx = app_hfp_get_active_idx();

    app_test_report_event(app_db.br_link[active_idx].bd_addr, EVENT_DUAL_MIC_MP_VERIFY,
                          &success_param,
                          1);
#endif

}

uint8_t app_audio_get_mp_dual_mic_setting(void)
{
    return mic.mp_dual_setting;
}

void app_audio_set_mp_dual_mic_setting(uint8_t param)
{
    mic.mp_dual_setting = param;
}

bool app_audio_check_mic_mute_enable(void)
{
    uint8_t i;
    bool enable_mic_mute = false;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].sco.conn_handle != 0)
        {
            enable_mic_mute = true;
            break;
        }
        else
        {
            enable_mic_mute = false;
        }
    }


    return enable_mic_mute;
}

void app_audio_tone_flush(bool relay)
{
    ringtone_flush(relay);
    voice_prompt_flush(relay);
}

bool app_audio_tone_type_cancel(T_APP_AUDIO_TONE_TYPE tone_type, bool relay)
{
    bool ret = false;
    uint8_t tone_index = TONE_INVALID_INDEX;

    tone_index = app_audio_get_tone_index(tone_type);

    if (tone_index < VOICE_PROMPT_INDEX)
    {
        ret = ringtone_cancel(tone_index, relay);
    }
    else if (tone_index < TONE_INVALID_INDEX)
    {
        ret = voice_prompt_cancel(tone_index - VOICE_PROMPT_INDEX, relay);
    }

    APP_PRINT_TRACE4("app_audio_tone_type_cancel: type 0x%02x, index 0x%02x, realy %d, ret %d",
                     tone_type,
                     tone_index, relay, ret);

    return ret;
}

static int8_t app_audio_tone_play_check(T_APP_AUDIO_TONE_TYPE tone_type, uint8_t tone_index)
{
    //bool is_ota_mode = app_ota_mode_check();
    int8_t ret = 0;

    /*
        if (is_ota_mode)
        {
            // Notification is deinit when OTA due to ram size issue,
            // It is forbidden to play tone during OTA processing
            ret = 1;
            goto exit;
        }
    */
    if (app_db.device_state == APP_DEVICE_STATE_OFF_ING)
    {
        if ((tone_type != TONE_POWER_OFF) && (tone_type != TONE_FACTORY_RESET))
        {
            ret = 2;
            goto exit;
        }
    }

    if ((app_db.tone_vp_status.state == APP_TONE_VP_STARTED)
        && (app_db.tone_vp_status.index == tone_index)
        && (tone_type == TONE_KEY_SHORT_PRESS))
    {
        ret = 3;
        goto exit;
    }

exit:

    return -ret;
}

uint8_t app_audio_get_tone_index(T_APP_AUDIO_TONE_TYPE tone_type)
{
    uint8_t tone_index = TONE_INVALID_INDEX;
    const uint8_t *tone_index_ptr = NULL;

    if (tone_type < TONE_CIS_LOST)
    {
        // T_APP_CFG_CONST Tone1: 128 bytes
        tone_index_ptr = &app_audio_tone_cfg.tone_power_on;
        tone_index = *(tone_index_ptr + (tone_type - TONE_POWER_ON));
    }
    else if (tone_type < TONE_CIS_LOST + 32)
    {
        // T_APP_CFG_CONST Tone2: 32 bytes
        tone_index_ptr = &app_audio_tone_cfg.cis_loss;
        tone_index = *(tone_index_ptr + (tone_type - TONE_CIS_LOST));
    }

    return tone_index;
}

/* NOTICE: Make sure T_APP_AUDIO_TONE_TYPE is align to app_cfg_const.tone_xxxx offset */
bool app_audio_tone_type_play(T_APP_AUDIO_TONE_TYPE tone_type, bool flush, bool relay)
{
    bool ret = false;
    uint8_t tone_index = TONE_INVALID_INDEX;
    int8_t check_result = 0;

#if F_APP_SAIYAN_MODE
    if ((data_capture_saiyan.saiyan_enable) && (tone_type != TONE_POWER_ON))
    {
        APP_PRINT_TRACE1("app_audio_tone_type_play: disallow 0x%x", tone_type);
        return true;
    }
#endif

    tone_index = app_audio_get_tone_index(tone_type);
    check_result = app_audio_tone_play_check(tone_type, tone_index);

    APP_PRINT_INFO6("app_audio_tone_type_play: tone_type 0x%02x, tone_index 0x%02x, state=%d, index=0x%02x, flush=%d, check_result = %d",
                    tone_type,
                    tone_index,
                    app_db.tone_vp_status.state,
                    app_db.tone_vp_status.index,
                    flush,
                    check_result);

    if (check_result != 0)
    {
        return ret;
    }


    if (tone_index < VOICE_PROMPT_INDEX)
    {
        if (flush)
        {
            ringtone_cancel(tone_index, true);
        }
        ret = ringtone_play(tone_index, relay);
    }
    else if (tone_index < TONE_INVALID_INDEX)
    {
        if (flush)
        {
            voice_prompt_cancel(tone_index - VOICE_PROMPT_INDEX, true);
        }
        ret = voice_prompt_play(tone_index - VOICE_PROMPT_INDEX,
                                (T_VOICE_PROMPT_LANGUAGE_ID)app_cfg_nv.voice_prompt_language,
                                relay);
    }

    return ret;
}

bool app_audio_tone_type_stop(void)
{
    bool ret = false;

    APP_PRINT_TRACE1("app_audio_tone_type_stop: tone_index 0x%02x", app_db.tone_vp_status.index);

    if (app_db.tone_vp_status.index < VOICE_PROMPT_INDEX)
    {
        ret = ringtone_stop();
    }
    else if (app_db.tone_vp_status.index < TONE_INVALID_INDEX)
    {
        ret = voice_prompt_stop();
    }

    return ret;
}

bool app_audio_tone_stop_sync(T_APP_AUDIO_TONE_TYPE tone_type)
{
    bool ret = false;
    uint8_t tone_index = TONE_INVALID_INDEX;
    int8_t check_result = 0;

    tone_index = app_audio_get_tone_index(tone_type);
    check_result = app_audio_tone_play_check(tone_type, tone_index);

    APP_PRINT_TRACE4("app_audio_tone_stop_sync: tone_type 0x%02x, tone_index 0x%02x, index 0x%02x, check_result %d",
                     tone_type, tone_index, app_db.tone_vp_status.index, check_result);

    if (tone_index != app_db.tone_vp_status.index ||
        check_result != 0)
    {
        return ret;
    }


    return ret;
}

void app_audio_volume_sync(void)
{
    T_APP_AUDIO_VOLUME *buf;
    uint8_t bond_num;
    uint8_t i;
    uint8_t num = 0;
    uint8_t bd_addr[6];
    uint8_t pair_idx_mapping;

    bond_num = app_bond_b2s_num_get();
    buf = malloc(bond_num * sizeof(T_APP_AUDIO_VOLUME));

    if (buf != NULL)
    {
        for (i = 1; i <= bond_num; i++)
        {
            if (app_bond_b2s_addr_get(i, bd_addr) == true)
            {
                app_bond_get_pair_idx_mapping(bd_addr, &pair_idx_mapping);
                buf[num].playback_volume = app_cfg_nv.audio_gain_level[pair_idx_mapping];
                buf[num].voice_volume = app_cfg_nv.voice_gain_level[pair_idx_mapping];
                memcpy(buf[num].bd_addr, bd_addr, 6);
            }
            else
            {
                buf[num].playback_volume = app_dsp_cfg_vol.playback_volume_default;
                buf[num].voice_volume = app_dsp_cfg_vol.voice_out_volume_default;
                memset(buf[num].bd_addr, 0, 6);
            }

            num++;
        }



        free(buf);
    }
}

void app_audio_speaker_channel_set(T_AUDIO_CHANCEL_SET set_mode, uint8_t spk_channel)
{
    uint8_t channel = 0;

    APP_PRINT_TRACE3("app_audio_speaker_channel_set: set_mode %d, spk_channel %d, specific spk_channel %d",
                     set_mode, app_cfg_nv.spk_channel, spk_channel);

    switch (set_mode)
    {
    case AUDIO_COUPLE_SET:
        {

        }
        break;

    case AUDIO_SINGLE_SET:
        {
            channel = app_audio_cfg.solo_speaker_channel;
        }
        break;

    case AUDIO_SPECIFIC_SET:
        {
            channel = spk_channel;
        }
        break;

    default:
        channel = 0;
        break;
    }

    if ((set_mode != AUDIO_SINGLE_SET) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        app_cfg_nv.spk_channel = channel;
        app_cfg_store(&app_cfg_nv.spk_channel, 1);


    }

    audio_volume_out_channel_set((T_AUDIO_VOLUME_CHANNEL_MASK)channel);
}

void app_audio_set_max_min_vol_from_phone_flag(bool status)
{
    vol.range_from_phone = status;
}

void app_audio_vol_set(T_AUDIO_TRACK_HANDLE track_handle, uint8_t vol)
{
    APP_PRINT_INFO3("app_audio_vol_set: voice_muted %d, playback_muted %d, vol %d",
                    app_db.voice_muted, app_db.playback_muted, vol);

    if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call) &&
        (app_hfp_get_call_status() == APP_HFP_CALL_INCOMING))
    {
        app_hfp_inband_tone_mute_ctrl();
    }
    else
    {
        audio_track_volume_out_set(track_handle, vol);
    }
}

bool app_audio_buffer_level_get(uint16_t *level)
{
    T_APP_BR_LINK *p_link = NULL;
    bool ret = false;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        p_link = &app_db.br_link[app_a2dp_get_active_idx()];

        if (p_link && p_link->a2dp.track_handle)
        {
            audio_track_buffer_level_get(p_link->a2dp.track_handle, level);
            ret = true;
        }
    }
    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            p_link = &app_db.br_link[i];

            if (p_link && p_link->a2dp.track_handle)
            {
                audio_track_buffer_level_get(p_link->a2dp.track_handle, level);
                ret = true;
                break;
            }
        }
    }

    return ret;
}

void app_audio_spk_mute_unmute(bool need)
{
    if (need)
    {
        app_db.playback_muted = true;
        app_db.voice_muted = true;

        audio_volume_out_mute(AUDIO_STREAM_TYPE_PLAYBACK);

        audio_volume_out_mute(AUDIO_STREAM_TYPE_VOICE);
    }
    else
    {
        app_db.playback_muted = false;
        app_db.voice_muted = false;

        if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call) &&
            (app_hfp_get_call_status() == APP_HFP_CALL_INCOMING))
        {
            app_hfp_inband_tone_mute_ctrl();
        }
        else
        {
            audio_volume_out_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
            audio_volume_out_unmute(AUDIO_STREAM_TYPE_VOICE);
        }
    }
}

void app_audio_track_spk_unmute(T_AUDIO_STREAM_TYPE stream_type)
{
    uint8_t active_idx;
    T_APP_BR_LINK *p_link = NULL;

    APP_PRINT_INFO2("app_audio_vol_unmute: voice_muted %d, playback_muted %d",
                    app_db.voice_muted, app_db.playback_muted);



    if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        active_idx = app_a2dp_get_active_idx();
        p_link = app_link_find_br_link(app_db.br_link[active_idx].bd_addr);
        if (p_link != NULL)
        {
            if (p_link->a2dp.muted)
            {
                p_link->a2dp.muted = false;
                audio_track_volume_out_unmute(p_link->a2dp.track_handle);
            }
        }
    }
    else if (stream_type == AUDIO_STREAM_TYPE_VOICE)
    {
        active_idx = app_hfp_get_active_idx();
        p_link = app_link_find_br_link(app_db.br_link[active_idx].bd_addr);
        if (p_link != NULL)
        {
            if (p_link->sco.muted)
            {
                p_link->sco.muted = false;
                audio_track_volume_out_unmute(p_link->sco.track_handle);
            }
        }
    }
}


void app_audio_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                          uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    switch (cmd_id)
    {
    case CMD_SET_VP_VOLUME:
        {
            voice_prompt_volume_set(cmd_ptr[2]);
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AUDIO_DSP_CTRL_SEND:
        {
            uint8_t *buf;
            T_AUDIO_STREAM_TYPE stream_type;
            T_DSP_TOOL_GAIN_LEVEL_DATA *gain_level_data;
            bool send_cmd_flag = true;

            buf = malloc(cmd_len - 2);
            if (buf == NULL)
            {
                return;
            }

            memcpy(buf, &cmd_ptr[2], (cmd_len - 2));

            gain_level_data = (T_DSP_TOOL_GAIN_LEVEL_DATA *)buf;

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (gain_level_data->cmd_id == CFG_H2D_DAC_GAIN ||
                gain_level_data->cmd_id == CFG_H2D_VOICE_ADC_POST_GAIN ||
                gain_level_data->cmd_id == CFG_H2D_APT_DAC_GAIN)
            {
                switch (gain_level_data->category)
                {
                case AUDIO_CATEGORY_TONE:
                case AUDIO_CATEGORY_VP:
                case AUDIO_CATEGORY_AUDIO:
                    {
                        stream_type = AUDIO_STREAM_TYPE_PLAYBACK;
                    }
                    break;

                case AUDIO_CATEGORY_APT:
                case AUDIO_CATEGORY_LLAPT:
                case AUDIO_CATEGORY_ANC:
                case AUDIO_CATEGORY_ANALOG:
                case AUDIO_CATEGORY_VOICE:
                    {
                        stream_type = AUDIO_STREAM_TYPE_VOICE;
                    }
                    break;

                case AUDIO_CATEGORY_RECORD:
                    {
                        stream_type = AUDIO_STREAM_TYPE_RECORD;
                    }
                    break;

                default:
                    {
                        stream_type = AUDIO_STREAM_TYPE_PLAYBACK;
                    }
                    break;
                }

                if (gain_level_data->cmd_id == CFG_H2D_DAC_GAIN)
                {
                    app_audio_route_dac_gain_set((T_AUDIO_CATEGORY)gain_level_data->category, gain_level_data->level,
                                                 gain_level_data->gain);
                    audio_volume_out_set(stream_type, gain_level_data->level);
                }
                else if (gain_level_data->cmd_id == CFG_H2D_APT_DAC_GAIN)
                {
                    app_audio_route_dac_gain_set((T_AUDIO_CATEGORY)gain_level_data->category, gain_level_data->level,
                                                 gain_level_data->gain);
                }
                else
                {
                    app_audio_route_adc_gain_set((T_AUDIO_CATEGORY)gain_level_data->category, gain_level_data->level,
                                                 gain_level_data->gain);
                    if (((T_AUDIO_CATEGORY)gain_level_data->category != AUDIO_CATEGORY_APT) &&
                        ((T_AUDIO_CATEGORY)gain_level_data->category != AUDIO_CATEGORY_LLAPT))
                    {
                        audio_volume_in_set(stream_type, gain_level_data->level);
                    }
                }

                send_cmd_flag = false;
            }

            if (send_cmd_flag)
            {
                audio_probe_dsp_send(buf, cmd_len - 2);
            }
            free(buf);
        }
        break;

    case CMD_SET_VOLUME:
        {
            uint8_t max_volume = 0;
            uint8_t min_volume = 0;
            uint8_t set_volume = cmd_ptr[2];

            T_AUDIO_STREAM_TYPE volume_type;

            if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
            {
                volume_type = AUDIO_STREAM_TYPE_VOICE;
            }
            else
            {
                volume_type = AUDIO_STREAM_TYPE_PLAYBACK;
            }

            max_volume = audio_volume_out_max_get(volume_type);
            min_volume = audio_volume_out_min_get(volume_type);

            if ((set_volume <= max_volume) && (set_volume >= min_volume))
            {
                audio_volume_out_set(volume_type, set_volume);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
            {
                uint8_t temp_buff[3];
                temp_buff[0] = GET_STATUS_VOLUME;
                temp_buff[1] = audio_volume_out_get(volume_type);
                temp_buff[2] = max_volume;

                app_report_event(cmd_path, EVENT_REPORT_STATUS, app_idx, temp_buff, sizeof(temp_buff));
            }
        }
        break;

    case CMD_DSP_TOOL_FUNCTION_ADJUSTMENT:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            uint8_t event_data[3];
            uint16_t function_type;

            LE_ARRAY_TO_UINT16(function_type, &cmd_ptr[2]);

            memcpy(event_data, &cmd_ptr[2], 2);
            event_data[2] = CMD_SET_STATUS_COMPLETE;

            switch (function_type)
            {
            case DSP_TOOL_FUNCTION_BRIGHTNESS:
                {
                }
                break;

            case DSP_TOOL_FUNCTION_HW_EQ:
                {
                    uint8_t eq_type = cmd_ptr[4];
                    uint8_t eq_channel = cmd_ptr[5];
                    uint16_t eq_len;

                    LE_ARRAY_TO_UINT16(eq_len, &cmd_ptr[10]);

                    audio_probe_codec_hw_eq_set(eq_type, eq_channel, &cmd_ptr[12], eq_len);
                }
                break;

            default:
                {
                    event_data[2] = CMD_SET_STATUS_UNKNOW_CMD;
                }
                break;
            }

            app_report_event(cmd_path, EVENT_DSP_TOOL_FUNCTION_ADJUSTMENT, app_idx, event_data,
                             sizeof(event_data));
        }
        break;

    case CMD_MIC_SWITCH:
        {
            uint8_t param = app_audio_mic_switch(cmd_ptr[2]);

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_IAP))
            {
                app_report_event(cmd_path, EVENT_MIC_SWITCH, app_idx, &param, 1);
            }
        }
        break;

    case CMD_DSP_TEST_MODE:
        {
            uint8_t dsp_cmd_len;
            uint8_t *dsp_cmd_buf;
            uint8_t dsp_param_len;

            dsp_cmd_len = cmd_len + 2;
            dsp_cmd_len = ((dsp_cmd_len + 3) >> 2) << 2; //transfer to 4-byte align
            dsp_cmd_buf = calloc(1, dsp_cmd_len);
            dsp_param_len = cmd_len - 2;

            if (dsp_cmd_buf == NULL)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                return;
            }
            else
            {
                dsp_cmd_buf[0] = AUDIO_PROBE_TEST_MODE;
                dsp_cmd_buf[2] = ((dsp_param_len + 3) >> 2); //word length
                memcpy(&dsp_cmd_buf[4], &cmd_ptr[2], dsp_param_len);

                audio_probe_dsp_send(dsp_cmd_buf, dsp_cmd_len);
                free(dsp_cmd_buf);

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_DUAL_MIC_MP_VERIFY:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            uint8_t error_code;

            if (app_audio_get_bud_stream_state() == BUD_STREAM_STATE_VOICE)
            {
                T_AUDIO_MP_DUAL_MIC_SETTING mp_dual_mic_setting;
                mp_dual_mic_setting = app_audio_mp_dual_mic_setting_check(&cmd_ptr[2]);

                if (mp_dual_mic_setting == AUDIO_MP_DUAL_MIC_SETTING_VALID)
                {
                    app_audio_mp_dual_mic_switch_action();
                    break;
                }
                else if (mp_dual_mic_setting == AUDIO_MP_DUAL_MIC_SETTING_ROLE_SWAP)
                {
                    app_mmi_handle_action(MMI_START_ROLESWAP);
                    break;
                }
                else
                {
                    error_code = 1;  //invalid mic setting
                }
            }
            else
            {
                error_code = 2;  //wrong dsp state
            }

            app_report_event(cmd_path, EVENT_DUAL_MIC_MP_VERIFY, app_idx, &error_code, 1);
        }
        break;

    case CMD_SOUND_PRESS_CALIBRATION:
        {

        }
        break;

    case CMD_GET_LOW_LATENCY_MODE_STATUS:
        {
            uint16_t latency_value = A2DP_LATENCY_MS; // default value
            T_APP_BR_LINK *p_link = NULL;

            if (app_db.gaming_mode)
            {
                if (app_link_check_b2s_link_by_id(active_a2dp_idx))
                {
                    p_link = &app_db.br_link[active_a2dp_idx];
                }

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                if ((p_link != NULL) && (p_link->a2dp.track_handle != NULL))
                {
                    audio_track_latency_get(p_link->a2dp.track_handle, &latency_value);
                }
                else
                {
                    // audio track is null, use last used latency value
                    app_audio_get_last_used_latency(&latency_value);
                }
            }

            app_audio_report_low_latency_status(latency_value);
        }
        break;

    case CMD_SET_LOW_LATENCY_LEVEL:
        {
            uint8_t event_data[3];
            uint8_t latency_level = cmd_ptr[2];
            uint16_t latency_value = A2DP_LATENCY_MS; // default value
            T_APP_BR_LINK *p_link = NULL;

            if (app_link_check_b2s_link_by_id(active_a2dp_idx))
            {
                p_link = &app_db.br_link[active_a2dp_idx];
            }

            if ((!app_db.gaming_mode) || (latency_level >= LOW_LATENCY_LEVEL_MAX) || (p_link == NULL))
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            latency_value = app_audio_update_audio_track_latency(p_link->a2dp.track_handle, latency_level);

            event_data[0] = app_cfg_nv.rws_low_latency_level_record;
            event_data[1] = (uint8_t)(latency_value);
            event_data[2] = (uint8_t)(latency_value >> 8);

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_LOW_LATENCY_LEVEL_SET, app_idx, event_data,
                             sizeof(event_data));
        }
        break;

    default:
        break;
    }
}

void app_audio_init(void)
{
    app_db.playback_device = AUDIO_DEVICE_OUT_SPK;

    audio_mgr_cback_register(app_audio_policy_cback);
    bt_mgr_cback_register(app_audio_bt_cback);
    app_timer_reg_cb(app_audio_policy_timeout_cb, &tmr.id);

    app_cmd_cback_register(app_audio_cmd_cback, CMD_MODULE_TYPE_AUDIO_POLICY,
                           APP_AUDIO_CMD_MSG_TOTAL);

    app_audio_route_gain_init();
    app_eq_cfg_init();
    app_eq_init();

#if F_APP_SAIYAN_EQ_FITTING
    app_eq_fitting_init();
#endif

    voice_prompt_language_set((T_VOICE_PROMPT_LANGUAGE_ID)app_cfg_nv.voice_prompt_language);


    audio_volume_out_max_set(AUDIO_STREAM_TYPE_PLAYBACK, app_dsp_cfg_vol.playback_volume_max);
    audio_volume_out_max_set(AUDIO_STREAM_TYPE_VOICE, app_dsp_cfg_vol.voice_out_volume_max);
    audio_volume_in_max_set(AUDIO_STREAM_TYPE_VOICE, app_dsp_cfg_vol.voice_volume_in_max);
    audio_volume_in_max_set(AUDIO_STREAM_TYPE_RECORD, app_dsp_cfg_vol.record_volume_max);
    audio_volume_out_min_set(AUDIO_STREAM_TYPE_PLAYBACK, app_dsp_cfg_vol.playback_volume_min);
    audio_volume_out_min_set(AUDIO_STREAM_TYPE_VOICE, app_dsp_cfg_vol.voice_out_volume_min);
    audio_volume_in_min_set(AUDIO_STREAM_TYPE_VOICE, app_dsp_cfg_vol.voice_volume_in_min);
    audio_volume_in_min_set(AUDIO_STREAM_TYPE_RECORD, app_dsp_cfg_vol.record_volume_min);


    app_cfg_nv.voice_prompt_volume_out_level = app_dsp_cfg_vol.voice_prompt_volume_default;
    app_cfg_nv.ringtone_volume_out_level = app_dsp_cfg_vol.ringtone_volume_default;

    voice_prompt_volume_max_set(app_dsp_cfg_vol.voice_prompt_volume_max);
    voice_prompt_volume_min_set(app_dsp_cfg_vol.voice_prompt_volume_min);
    voice_prompt_volume_set(app_cfg_nv.voice_prompt_volume_out_level);

    ringtone_volume_max_set(app_dsp_cfg_vol.ringtone_volume_max);
    ringtone_volume_min_set(app_dsp_cfg_vol.ringtone_volume_min);
    ringtone_volume_set(app_cfg_nv.ringtone_volume_out_level);

#if F_APP_LINEIN_SUPPORT
    if (app_cfg_const.line_in_support)
    {
        line_in_volume_out_max_set(app_dsp_cfg_vol.line_in_volume_out_max);
        line_in_volume_out_min_set(app_dsp_cfg_vol.line_in_volume_out_min);
        line_in_volume_out_set(app_cfg_nv.line_in_gain_level);
        line_in_volume_in_max_set(app_dsp_cfg_vol.line_in_volume_in_max);
        line_in_volume_in_min_set(app_dsp_cfg_vol.line_in_volume_in_min);
        line_in_volume_in_set(app_dsp_cfg_vol.line_in_volume_in_default);
    }
#endif

    app_record_init();
}
