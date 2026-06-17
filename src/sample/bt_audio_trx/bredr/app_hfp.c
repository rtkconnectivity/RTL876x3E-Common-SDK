/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include "string.h"
#include "trace.h"
#include "stdlib.h"
#include "app_timer.h"
#include "btm.h"
#include "audio.h"
#include "remote.h"
#include "bt_bond.h"
#include "app_charger_cfg.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_dsp_cfg.h"
#include "audio_probe.h"
#include "app_main.h"
#include "app_report.h"


#include "app_link_util_cs.h"
#include "app_sdp.h"
#include "app_hfp_cfg.h"
#include "app_hfp.h"
#include "app_audio_policy.h"
#include "app_bt_policy_cfg.h"
#include "app_bt_policy_api.h"
#include "audio_volume.h"
#include "app_mmi.h"
#include "app_bond.h"
#include "app_line_in.h"
#include "app_auto_power_off.h"
#include "audio_track.h"
#include "app_multilink.h"
#include "app_cmd.h"
#include "bt_types.h"

typedef enum
{
    TIMER_AUTO_ANSWER_CALL  = 0x00,
    TIMER_RING              = 0x01,
    TIMER_MIC_MUTE_ALARM    = 0x02,
    TIMER_NO_SERVICE        = 0x03,
    TIMER_ADD_SCO           = 0x04,
    TIMER_CANCEL_VOICE_DAIL = 0x05,
    TIMER_MAX               = 0x06,
} T_APP_HFP_TIMER;


static struct
{
    bool ring_active;
    uint32_t auto_answer_call_interval;
    T_APP_HFP_CALL_STATUS app_call_status;
    uint8_t active_link_id;

    struct
    {
        uint8_t id;
        uint8_t indices[TIMER_MAX];
    } tmr;
} hfp =
{
    .ring_active = false,
    .active_link_id = 0,
    .tmr =
    {
        .id = 0xff,
        .indices = {0},
    }
};


void app_hfp_voice_nr_enable(void)
{
    uint8_t enable;

    enable = (app_hfp_get_call_status() >= APP_HFP_CALL_ACTIVE) ? 1 : 0;

    audio_probe_dsp_ipc_send_call_status(enable);
}

void app_hfp_inband_tone_mute_ctrl(void)
{
    uint8_t cmd_ptr[8];
    T_APP_BR_LINK *p_link;

    p_link = &(app_db.br_link[hfp.active_link_id]);

    if (p_link != NULL)
    {
        if (p_link->connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
        {
            if (p_link->hfp.call_status == APP_HFP_CALL_INCOMING)
            {
                p_link->sco.muted = true;
            }
            else
            {
                p_link->sco.muted = false;
            }

            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {

            }
            else
            {
                if (!p_link->sco.muted)
                {
                    audio_track_volume_out_unmute(p_link->sco.track_handle);
                }
            }
        }
    }

    APP_PRINT_INFO1("app_hfp_inband_tone_mute_ctrl voice_muted: %d", app_db.voice_muted);
}

static void app_hfp_ring_alert(T_APP_BR_LINK *p_link)
{
    T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_MAX;
    tone_type = (T_APP_AUDIO_TONE_TYPE)app_hfp_get_call_in_tone_type(p_link);

    app_audio_tone_type_play(tone_type, false, false);

    if (app_cfg_const.timer_hfp_ring_period != 0)
    {
        app_start_timer(&hfp.tmr.indices[TIMER_RING], "hfp_ring",
                        hfp.tmr.id, TIMER_RING, p_link->id, true,
                        app_cfg_const.timer_hfp_ring_period * 1000);
    }
    else
    {
        hfp.ring_active = false;
    }
}

void app_hfp_always_playing_outband_tone(void)
{
    T_APP_BR_LINK *p_link;

    p_link = &(app_db.br_link[hfp.active_link_id]);

    if (p_link != NULL)
    {
        app_hfp_inband_tone_mute_ctrl();

        if (p_link->hfp.call_status == APP_HFP_CALL_INCOMING)
        {
            app_hfp_ring_alert(p_link);
        }

        APP_PRINT_INFO2("app_hfp_always_playing_outband_tone: active_hf_idx %d, call_status %d",
                        hfp.active_link_id,
                        p_link->hfp.call_status);
    }
}

void app_hfp_update_call_status(void)
{
    uint8_t i = 0;
    uint8_t inactive_hf_idx = 0;
    uint8_t exchange_active_inactive_index_fg = 0;
    uint8_t call_status_old;
    T_APP_HFP_CALL_STATUS active_hf_status = APP_HFP_CALL_IDLE;
    T_APP_HFP_CALL_STATUS inactive_hf_status = APP_HFP_CALL_IDLE;

    call_status_old = app_hfp_get_call_status();

    active_hf_status = (T_APP_HFP_CALL_STATUS)app_db.br_link[hfp.active_link_id].hfp.call_status;

    APP_PRINT_INFO3("app_hfp_update_call_status: call_status_old 0x%04x, active_hf_status 0x%04x, active_hf_idx 0x%04x",
                    call_status_old, active_hf_status, hfp.active_link_id);

    //find inactive device
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (i != hfp.active_link_id)
        {
            if ((app_db.br_link[i].connected_profile & HFP_PROFILE_MASK) ||
                (app_db.br_link[i].connected_profile & HSP_PROFILE_MASK))
            {
                inactive_hf_idx = i;
                inactive_hf_status = (T_APP_HFP_CALL_STATUS)app_db.br_link[inactive_hf_idx].hfp.call_status;
                break;
            }
        }
    }

    if (active_hf_status == APP_HFP_CALL_IDLE)
    {
        if (inactive_hf_status != APP_HFP_CALL_IDLE)
        {
            exchange_active_inactive_index_fg = 1;
            if (app_link_get_b2s_link_num() == MULTILINK_SRC_CONNECTED)
            {
                if (app_link_get_sco_conn_num() &&
                    (inactive_hf_status == APP_HFP_CALL_INCOMING) &&
                    (app_db.br_link[inactive_hf_idx].sco.track_handle != NULL))
                {
                    audio_track_start(app_db.br_link[inactive_hf_idx].sco.track_handle);
                }
            }
        }
    }
    else if ((active_hf_status != APP_HFP_CALL_ACTIVE) &&
             (active_hf_status != APP_HFP_CALL_ACTIVE_WITH_CALL_HELD) &&
             (active_hf_status != APP_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
    {
        if ((inactive_hf_status == APP_HFP_CALL_ACTIVE) ||
            (inactive_hf_status == APP_HFP_CALL_ACTIVE_WITH_CALL_HELD) ||
            (inactive_hf_status == APP_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
        {
            exchange_active_inactive_index_fg = 1;
        }
    }

    if (exchange_active_inactive_index_fg)
    {
        uint8_t i;
        //exchange index
        i = inactive_hf_idx;
        inactive_hf_idx = hfp.active_link_id;
        app_hfp_set_active_idx(app_db.br_link[i].bd_addr);
        app_bond_set_priority(app_db.br_link[i].bd_addr);

        //exchange status
        i = inactive_hf_status;
        inactive_hf_status = active_hf_status;
        active_hf_status = (T_APP_HFP_CALL_STATUS)i;
    }

    //update app_call_status
    switch (active_hf_status)
    {
    case APP_HFP_CALL_INCOMING:
        if (inactive_hf_status >= APP_HFP_CALL_ACTIVE)
        {
            app_hfp_set_call_status(APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT);
        }
        else
        {
            app_hfp_set_call_status(APP_HFP_CALL_INCOMING);
        }
        break;

    case APP_HFP_CALL_ACTIVE:
        if (inactive_hf_status >= APP_HFP_CALL_ACTIVE)
        {
            if (app_db.br_link[inactive_hf_idx].sco.conn_handle)
            {
                app_multi_preempt_hfp(inactive_hf_idx);
            }
            app_hfp_set_call_status(APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD);
        }
        else if (inactive_hf_status == APP_HFP_CALL_INCOMING)
        {
            app_hfp_set_call_status(APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT);
        }
        else
        {
            app_hfp_set_call_status(APP_HFP_CALL_ACTIVE);
        }
        break;

    default:
        app_hfp_set_call_status(active_hf_status);
        break;
    }

    APP_PRINT_INFO5("app_hfp_update_call_status: call_status 0x%04x, active_hf_status 0x%04x, mac : %s,"
                    "inactive_hf_status 0x%04x, inactive_hf_idx 0x%04x",
                    app_hfp_get_call_status(), active_hf_status,
                    TRACE_BDADDR(app_db.br_link[hfp.active_link_id].bd_addr), inactive_hf_status,
                    inactive_hf_idx);

    if (app_bt_policy_cfg.enable_multi_sco_disc_resume)
    {
        if ((app_bt_policy_cfg_nv.enable_multi_link) &&
            (app_link_get_b2s_link_num() == MULTILINK_SRC_CONNECTED))
        {
            APP_PRINT_TRACE4("call_idle_resume %d,%d,%d,%d", app_link_get_sco_conn_num(),
                             active_hf_status,
                             inactive_hf_status,
                             app_hfp_get_call_status());
            if ((app_link_get_sco_conn_num() == 0) &&
                (active_hf_status == APP_HFP_CALL_IDLE) &&
                inactive_hf_status == APP_HFP_CALL_IDLE)
            {
                app_resume_a2dp_avrcp(hfp.active_link_id);
            }
        }
    }
    if (call_status_old != app_hfp_get_call_status())
    {
        if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
            (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
        {
            //app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_CALL_STATUS);
        }

        if (app_hfp_get_call_status() == APP_HFP_CALL_IDLE)
        {
            if (app_audio_is_mic_mute())
            {
                app_audio_set_mic_mute_status(0);
                audio_volume_in_unmute(AUDIO_STREAM_TYPE_VOICE);
            }
        }

        if ((call_status_old == APP_HFP_CALL_INCOMING) ||
            (call_status_old == APP_HFP_CALL_ACTIVE_WITH_CALL_WAITING) ||
            (call_status_old == APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT))
        {
            T_APP_BR_LINK *p_link;
            T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_MAX;

            if (exchange_active_inactive_index_fg)
            {
                p_link = &(app_db.br_link[inactive_hf_idx]);
            }
            else
            {
                p_link = &(app_db.br_link[hfp.active_link_id]);
            }

            tone_type = (T_APP_AUDIO_TONE_TYPE)app_hfp_get_call_in_tone_type(p_link);
            app_audio_tone_type_cancel(tone_type, false);
        }


#if F_APP_LINEIN_SUPPORT
        if (app_cfg_const.line_in_support)
        {
            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                app_line_in_call_status_update(app_hfp_get_call_status() == APP_HFP_CALL_IDLE);
            }
        }
#endif

        if (hfp.ring_active && (active_hf_status != APP_HFP_CALL_INCOMING))
        {
            hfp.ring_active = false;
        }

        if (app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call)
        {
            app_hfp_always_playing_outband_tone();
        }
    }

    if (app_link_get_b2s_link_num() == MULTILINK_SRC_CONNECTED)
    {
        if ((app_bt_policy_cfg.enable_multi_outband_call_tone) &&
            (app_hfp_get_call_status() == APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT) &&
            (app_link_get_sco_conn_num()))
        {
            T_APP_BR_LINK *p_link;

            p_link = &(app_db.br_link[hfp.active_link_id]);
            if (p_link != NULL)
            {
                app_hfp_ring_alert(p_link);
            }
        }
    }

    app_hfp_voice_nr_enable();
}

void app_hfp_set_no_service_timer(bool all_service_status)
{
    app_stop_timer(&hfp.tmr.indices[TIMER_NO_SERVICE]);

    if (all_service_status == false)
    {
        app_start_timer(&hfp.tmr.indices[TIMER_NO_SERVICE], "hfp_no_service",
                        hfp.tmr.id, TIMER_NO_SERVICE, 0, true,
                        app_hfp_cfg.timer_no_service_warning * 1000);
    }
}

static void app_hfp_check_service_status()
{
    if (app_hfp_cfg.timer_no_service_warning > 0)
    {
        bool all_service_status = true;
        uint8_t i;

        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            for (i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if ((app_db.br_link[i].connected_profile & HFP_PROFILE_MASK) &&
                    (app_link_check_b2s_link_by_id(i) == true))
                {
                    if (app_db.br_link[i].hfp.service_status == false)
                    {
                        all_service_status = false;
                        break;
                    }
                }
            }

            if ((all_service_status == false) &&
                (hfp.tmr.indices[TIMER_NO_SERVICE] == 0))
            {
                app_audio_tone_type_play(TONE_HF_NO_SERVICE, false, false);
            }

            if (((all_service_status == true) && (hfp.tmr.indices[TIMER_NO_SERVICE] != 0)) ||
                ((all_service_status == false) && (hfp.tmr.indices[TIMER_NO_SERVICE] == 0)))
            {
                app_hfp_set_no_service_timer(all_service_status);


            }
        }
    }
}

void app_hfp_batt_level_report(uint8_t *bd_addr)
{
    uint8_t batt_level = app_db.local_batt_level;
    bool need_report = false;

    if ((app_charger_cfg.enable_report_lower_battery_volume) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        if ((app_db.remote_batt_level < app_db.local_batt_level) &&
            (app_db.remote_batt_level != 0))
        {
            batt_level = app_db.remote_batt_level;
        }
    }

    if (app_db.hfp_report_batt)
    {

        need_report = true;

    }

    if (need_report)
    {
        bt_hfp_batt_level_report(bd_addr, batt_level);
    }
}

static void app_hfp_set_mic_mute(void)
{
    APP_PRINT_INFO1("app_hfp_set_mic_mute is_mic_mute %d", app_audio_is_mic_mute());
    uint8_t app_idx_active = app_hfp_get_active_idx();

    T_AUDIO_STREAM_TYPE stream_type = AUDIO_STREAM_TYPE_VOICE;


    if (app_audio_is_mic_mute())
    {

        bt_hfp_microphone_gain_level_report(app_db.br_link[app_idx_active].bd_addr, 0);
        audio_volume_in_mute(stream_type);

        if (app_hfp_cfg.timer_mic_mute_alarm != 0)
        {
            app_start_timer(&hfp.tmr.indices[TIMER_MIC_MUTE_ALARM], "mic_mute_alarm",
                            hfp.tmr.id, TIMER_MIC_MUTE_ALARM, 0, true,
                            app_hfp_cfg.timer_mic_mute_alarm * 1000);
        }
    }
    else
    {
        app_stop_timer(&hfp.tmr.indices[TIMER_MIC_MUTE_ALARM]);

        bt_hfp_microphone_gain_level_report(app_db.br_link[app_idx_active].bd_addr, 10);
        audio_volume_in_unmute(stream_type);
    }
}


void app_hfp_call_id_rpt(T_CMD_PATH cmd_path, uint8_t app_link_id,
                         T_CALLER_ID_TYPE call_id_type, uint8_t data_len, uint8_t data[data_len])
{
    struct
    {
        uint8_t app_link_id;
        uint8_t call_id_type;
        uint8_t data_len;
        uint8_t data[];
    } __attribute__((packed)) *rpt = NULL;

    uint16_t rpt_len = data_len + sizeof(*rpt);
    rpt = calloc(1, rpt_len);
    if (rpt != NULL)
    {
        rpt->app_link_id = app_link_id;
        rpt->call_id_type = call_id_type;
        rpt->data_len = data_len;
        memcpy(rpt->data, data, data_len);
        app_report_event(cmd_path, EVENT_HFP_CALLER_ID, app_link_id, (uint8_t *)rpt, rpt_len);
        free(rpt);
    }

}


static void app_hfp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_HFP_CONN_IND:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->hfp_conn_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_hfp_bt_cback: no acl link found");
                return;
            }
            bt_hfp_connect_cfm(p_link->bd_addr, true);
        }
        break;

    case BT_EVENT_HFP_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->hfp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                uint8_t link_number;
                uint8_t pair_idx_mapping;

                p_link->hfp.call_id_type_chk = true;
                p_link->hfp.call_id_type_num = false;

                app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);
                bt_hfp_speaker_gain_level_report(p_link->bd_addr, app_cfg_nv.voice_gain_level[pair_idx_mapping]);

                bt_hfp_microphone_gain_level_report(p_link->bd_addr, 0x0a);

                app_hfp_batt_level_report(p_link->bd_addr);

                bt_hfp_nrec_disable(p_link->bd_addr);

                link_number = app_connected_profile_link_num(HFP_PROFILE_MASK | HSP_PROFILE_MASK);
                if (link_number == 1)
                {
                    app_hfp_set_active_idx(p_link->bd_addr);
                    app_bond_set_priority(p_link->bd_addr);
                }
                if (app_db.br_link[app_db.first_hf_index].hfp.state == APP_HF_STATE_STANDBY)
                {
                    app_db.first_hf_index = p_link->id;
                }
                else
                {
                    app_db.last_hf_index = p_link->id;
                }
                p_link->hfp.state = APP_HF_STATE_CONNECTED;
            }
        }
        break;

    case BT_EVENT_HFP_VOICE_RECOGNITION_ACTIVATION:
        {
            if (param->hfp_voice_recognition_activation.result == BT_HFP_CMD_OK)
            {
                T_APP_BR_LINK *p_link;

                p_link = app_link_find_br_link(param->hfp_voice_recognition_activation.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->hfp.call_status == APP_HFP_CALL_IDLE)
                    {
                        p_link->hfp.call_status = APP_HFP_VOICE_ACTIVATION_ONGOING;
                    }

                    app_hfp_update_call_status();

                    //Workaround: iOS may not create SCO and not issue +BVRA:0 when trigger siri repeatedly
                    //Add a protection timer to exit voice activation state
                    if (p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS)
                    {
                        app_start_timer(&hfp.tmr.indices[TIMER_CANCEL_VOICE_DAIL], "cancel_iphone_voice_dail",
                                        hfp.tmr.id, TIMER_CANCEL_VOICE_DAIL, false,
                                        p_link->id, 1000);
                    }
                }
            }
        }
        break;

    case BT_EVENT_HFP_VOICE_RECOGNITION_DEACTIVATION:
        {
            if (param->hfp_voice_recognition_deactivation.result == BT_HFP_CMD_OK)
            {
                T_APP_BR_LINK *p_link;

                p_link = app_link_find_br_link(param->hfp_voice_recognition_deactivation.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->hfp.call_status == APP_HFP_VOICE_ACTIVATION_ONGOING)
                    {
                        p_link->hfp.call_status = APP_HFP_CALL_IDLE;
                    }

                    app_hfp_update_call_status();
                }
            }
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
            T_APP_BR_LINK *p_link;
#if C_APP_END_OUTGOING_CALL_PLAY_CALL_END_TONE == 0
            uint8_t temp_idx = hfp.active_link_id;
#endif
            p_link = app_link_find_br_link(param->hfp_call_status.bd_addr);
            if (p_link != NULL)
            {
                switch (param->hfp_call_status.curr_status)
                {
                case BT_HFP_CALL_IDLE:
                    {
                        p_link->hfp.call_status = APP_HFP_CALL_IDLE;
                    }
                    break;

                case BT_HFP_CALL_INCOMING:
                    {
                        p_link->hfp.call_status = APP_HFP_CALL_INCOMING;
                    }
                    break;

                case BT_HFP_CALL_OUTGOING:
                    {
                        p_link->hfp.call_status = APP_HFP_CALL_OUTGOING;
                    }
                    break;

                case BT_HFP_CALL_ACTIVE:
                    {
                        p_link->hfp.call_status = APP_HFP_CALL_ACTIVE;
                    }
                    break;

                case BT_HFP_CALL_HELD:
                    {
                        //p_link->call_status = APP_HFP_CALL_HELD;
                    }
                    break;

                case BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING:
                    {
                        p_link->hfp.call_status = APP_HFP_CALL_ACTIVE_WITH_CALL_WAITING;
                    }
                    break;

                case BT_HFP_CALL_ACTIVE_WITH_CALL_HELD:
                    {
                        p_link->hfp.call_status = APP_HFP_CALL_ACTIVE_WITH_CALL_HELD;
                    }
                    break;

                default:
                    break;
                }

                if ((app_hfp_cfg.enable_auto_answer_incoming_call == 1) &&
                    (p_link->hfp.call_status == APP_HFP_CALL_INCOMING))
                {
                    hfp.auto_answer_call_interval = app_hfp_cfg.timer_hfp_auto_answer_call * 1000;
                    app_start_timer(&hfp.tmr.indices[TIMER_AUTO_ANSWER_CALL], "auto_answer_call",
                                    hfp.tmr.id, TIMER_AUTO_ANSWER_CALL, p_link->id, false,
                                    hfp.auto_answer_call_interval);
                }

                if (p_link->hfp.call_status == APP_HFP_CALL_IDLE)
                {
                    p_link->hfp.call_id_type_chk = true;
                    p_link->hfp.call_id_type_num = false;
                }

                app_hfp_update_call_status();

                //Workaround: iOS may not create SCO and not issue +BVRA:0 when trigger siri repeatedly
                //Add a protection timer to exit voice activation state
                if ((p_link->hfp.call_status == APP_HFP_VOICE_ACTIVATION_ONGOING) &&
                    (p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS))
                {
                    app_start_timer(&hfp.tmr.indices[TIMER_CANCEL_VOICE_DAIL], "cancel_iphone_voice_dail",
                                    hfp.tmr.id, TIMER_CANCEL_VOICE_DAIL, p_link->id, false,
                                    1000);
                }

                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    if (param->hfp_call_status.prev_status == BT_HFP_CALL_INCOMING &&
                        param->hfp_call_status.curr_status == BT_HFP_CALL_IDLE)
                    {
                        if (app_db.reject_call_by_key)
                        {
                            app_db.reject_call_by_key = false;
                            app_audio_tone_type_play(TONE_HF_CALL_REJECT, false, false);
                        }
                    }

                    if (p_link->id == temp_idx &&
                        param->hfp_call_status.prev_status == BT_HFP_CALL_ACTIVE &&
                        param->hfp_call_status.curr_status == BT_HFP_CALL_IDLE)
                    {
                        app_audio_tone_type_play(TONE_HF_CALL_END, false, false);
                    }

                    if (p_link->id == hfp.active_link_id &&
                        param->hfp_call_status.prev_status != BT_HFP_CALL_ACTIVE &&
                        param->hfp_call_status.curr_status == BT_HFP_CALL_ACTIVE)
                    {
                        app_audio_tone_type_play(TONE_HF_CALL_ACTIVE, false, false);
                    }

                    if (param->hfp_call_status.prev_status != BT_HFP_CALL_OUTGOING &&
                        param->hfp_call_status.curr_status == BT_HFP_CALL_OUTGOING)
                    {
                        app_audio_tone_type_play(TONE_HF_OUTGOING_CALL, false, false);
                    }
                }
            }

            if (param->hfp_call_status.curr_status == BT_HFP_CALL_ACTIVE ||
                param->hfp_call_status.curr_status == BT_HFP_CALL_INCOMING ||
                param->hfp_call_status.curr_status == BT_HFP_CALL_OUTGOING)
            {
                if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
                {
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_VOICE);
                }

                app_audio_set_bud_stream_state(BUD_STREAM_STATE_VOICE);
            }
            else if (param->hfp_call_status.curr_status == BT_HFP_CALL_IDLE)
            {
                if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
                {
                    app_auto_power_off_enable(AUTO_POWER_OFF_MASK_VOICE,
                                              app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
                }

                app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
            }


        }
        break;

    case BT_EVENT_HFP_SERVICE_STATUS:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->hfp_service_status.bd_addr);

            if (p_link != NULL)
            {
                p_link->hfp.service_status = param->hfp_service_status.status;
                app_hfp_check_service_status();
            }
        }
        break;

    case BT_EVENT_HFP_CALL_WAITING_IND:
    case BT_EVENT_HFP_CALLER_ID_IND:
        {
            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                T_APP_BR_LINK *br_link;
                T_APP_LE_LINK *le_link;
                char *number;
                uint16_t num_len;

                if (event_type == BT_EVENT_HFP_CALLER_ID_IND)
                {
                    br_link = app_link_find_br_link(param->hfp_caller_id_ind.bd_addr);
                    le_link = app_link_find_le_link_by_addr(param->hfp_caller_id_ind.bd_addr);
                    number = (char *)param->hfp_caller_id_ind.number;
                    num_len = strlen(param->hfp_caller_id_ind.number);
                }
                else
                {
                    br_link = app_link_find_br_link(param->hfp_call_waiting_ind.bd_addr);
                    le_link = app_link_find_le_link_by_addr(param->hfp_call_waiting_ind.bd_addr);
                    number = (char *)param->hfp_call_waiting_ind.number;
                    num_len = strlen(param->hfp_call_waiting_ind.number);
                }

                if (br_link != NULL)
                {

                    if (br_link->hfp.call_id_type_chk == true)
                    {
                        if (br_link->connected_profile & PBAP_PROFILE_MASK)
                        {
                            if (bt_pbap_vcard_listing_by_number_pull(br_link->bd_addr, number) == false)
                            {
                                br_link->hfp.call_id_type_chk = false;
                                br_link->hfp.call_id_type_num = true;
                            }
                        }
                        else
                        {
                            br_link->hfp.call_id_type_chk = false;
                            br_link->hfp.call_id_type_num = true;
                        }
                    }

                    if (br_link->hfp.call_id_type_chk == false)
                    {
                        if (br_link->hfp.call_id_type_num == true)
                        {
                            uint8_t *data;
                            uint16_t len;
                            len = num_len + 3;
                            data = malloc(len);

                            if (data != NULL)
                            {
                                data[1] = CALLER_ID_TYPE_NUMBER;
                                data[2] = num_len;
                                memcpy(data + 3, number, num_len);

                                if (br_link->connected_profile & SPP_PROFILE_MASK)
                                {
                                    data[0] = br_link->id;
                                    app_report_event(CMD_PATH_SPP, EVENT_HFP_CALLER_ID, br_link->id, data, len);
                                }
                                else if (br_link->connected_profile & IAP_PROFILE_MASK)
                                {
                                    data[0] = br_link->id;
                                    app_report_event(CMD_PATH_IAP, EVENT_HFP_CALLER_ID, br_link->id, data, len);
                                }
                                else if (br_link->connected_profile & GATT_PROFILE_MASK)
                                {
                                    data[0] = br_link->id;
                                    app_report_event(CMD_PATH_GATT_OVER_BREDR, EVENT_HFP_CALLER_ID, br_link->id, data, len);
                                }
                                else if (le_link != NULL)
                                {
                                    data[0] = le_link->id;
                                    app_report_event(CMD_PATH_LE, EVENT_HFP_CALLER_ID, le_link->id, data, len);
                                }

                                free(data);
                            }
                        }
                    }
                }
            }
        }
        break;

    case BT_EVENT_HFP_RING_ALERT:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->hfp_ring_alert.bd_addr);
            if (p_link != NULL)
            {
                p_link->hfp.is_inband_ring = param->hfp_ring_alert.is_inband;

                if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call == false) &&
                    ((p_link->hfp.is_inband_ring == false) ||
                     (p_link->id != hfp.active_link_id))) /* TODO check active sco link */
                {
                    if (hfp.ring_active == false && p_link->hfp.call_status == APP_HFP_CALL_INCOMING)
                    {
                        hfp.ring_active = true;

                        app_hfp_ring_alert(p_link);
                    }
                }
            }
        }
        break;

    case BT_EVENT_HFP_SPK_VOLUME_CHANGED:
        {
            T_APP_BR_LINK *p_link;
            p_link = app_link_find_br_link(param->hfp_spk_volume_changed.bd_addr);
            uint8_t temp_buff[7];

            if (p_link != NULL)
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    uint8_t pair_idx_mapping;

                    app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);

                    app_cfg_nv.voice_gain_level[pair_idx_mapping] = (param->hfp_spk_volume_changed.volume *
                                                                     app_dsp_cfg_vol.voice_out_volume_max +
                                                                     0x0f / 2) / 0x0f;

                    memcpy(&temp_buff[0], &param->hfp_spk_volume_changed.bd_addr, 6);
                    temp_buff[6] = app_cfg_nv.voice_gain_level[pair_idx_mapping];
                    app_report_event(CMD_PATH_UART, EVENT_VOLUME_SYNC, 0, temp_buff, sizeof(temp_buff));

                    app_audio_vol_set(p_link->sco.track_handle, app_cfg_nv.voice_gain_level[pair_idx_mapping]);
                    app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
                }
                else
                {

                }
            }
        }
        break;

    case BT_EVENT_HFP_MIC_VOLUME_CHANGED:
        {
        }
        break;

    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_hfp_voice_nr_enable();
            }
        }
        break;

    case BT_EVENT_HFP_DISCONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->hfp_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (param->hfp_disconn_cmpl.cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                {
                    //do nothing
                }
                else
                {
                    p_link->hfp.call_status = APP_HFP_CALL_IDLE;
                    p_link->hfp.state = APP_HF_STATE_STANDBY;
                    if (app_db.first_hf_index == p_link->id)
                    {
                        app_db.first_hf_index = app_db.last_hf_index;
                    }

                    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if (app_db.br_link[i].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                        {
                            app_hfp_set_active_idx(app_db.br_link[i].bd_addr);
                            app_bond_set_priority(app_db.br_link[i].bd_addr);
                            break;
                        }
                    }

                    if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
                    {
                        app_hfp_update_call_status();
                    }
                }

                if ((param->hfp_disconn_cmpl.cause & ~HCI_ERR) != HCI_ERR_CONN_ROLESWAP)
                {
                    app_hfp_check_service_status();
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
        APP_PRINT_INFO1("app_hfp_bt_cback: event_type 0x%04x", event_type);
    }
}

T_APP_HFP_CALL_STATUS app_hfp_get_call_status(void)
{
    return hfp.app_call_status;
}

void app_hfp_set_call_status(T_APP_HFP_CALL_STATUS call_status)
{
    static T_APP_HFP_CALL_STATUS pre_call_status = APP_HFP_CALL_IDLE;
    uint8_t active_idx = hfp.active_link_id;
    uint8_t pair_idx_mapping = 0;
    bool get_pair_idx = app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr,
                                                      &pair_idx_mapping);

    APP_PRINT_TRACE2("app_hfp_set_call_status: %d -> %d", pre_call_status, call_status);

    hfp.app_call_status = call_status;

    //app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_HFP_CALL_STATUS_CHAGNED);

    if (app_hfp_cfg.fixed_inband_tone_gain && get_pair_idx == true)
    {
        uint8_t voice_gain = app_cfg_nv.voice_gain_level[pair_idx_mapping];

        if (pre_call_status == APP_HFP_CALL_OUTGOING ||
            pre_call_status == APP_HFP_CALL_INCOMING)
        {
            if (call_status == APP_HFP_CALL_ACTIVE)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {

                    }
                    else
                    {
                        app_hfp_set_link_voice_gain(app_db.br_link[active_idx].bd_addr, voice_gain);
                    }
                }
            }
        }
        else if (call_status == APP_HFP_CALL_OUTGOING ||
                 call_status == APP_HFP_CALL_INCOMING)
        {
            app_hfp_set_link_voice_gain(app_db.br_link[active_idx].bd_addr,
                                        app_hfp_cfg.inband_tone_gain_lv);
        }
    }

    pre_call_status = call_status;
}

uint8_t app_hfp_get_active_idx(void)
{
#if F_APP_MULTILINK_ENABLE
    return hfp.active_link_id;
#else
    return 0;
#endif
}

void app_hfp_only_set_active_idx(uint8_t idx)
{
    hfp.active_link_id = idx;
}

bool app_hfp_sco_is_connected(void)
{
    return (app_db.br_link[hfp.active_link_id].sco.conn_handle == 0) ? false : true;
}

bool app_hfp_set_link_voice_gain(uint8_t *addr, uint8_t gain)
{
    T_APP_BR_LINK *p_link = NULL;
    bool ret = false;

    p_link = app_link_find_br_link(addr);


    if (p_link && p_link->sco.track_handle)
    {
        app_audio_vol_set(p_link->sco.track_handle, gain);
        app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
        ret = true;
    }

    return ret;
}

bool app_hfp_set_active_idx(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link)
    {
        hfp.active_link_id = p_link->id;
        if (p_link->sco.track_handle)
        {
            audio_track_start(p_link->sco.track_handle);
        }
        return bt_sco_link_switch(bd_addr);
    }
    return false;
}

void app_hfp_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_hfp_timeout_cb: timer_evt %d, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case TIMER_AUTO_ANSWER_CALL:
        {
            T_APP_BR_LINK *p_link;
            p_link = &(app_db.br_link[param]);
            app_stop_timer(&hfp.tmr.indices[TIMER_AUTO_ANSWER_CALL]);
            if (p_link->hfp.call_status == APP_HFP_CALL_INCOMING)
            {
                bt_hfp_call_answer_req(p_link->bd_addr);
            }
        }
        break;

    case TIMER_RING:
        {
            T_APP_BR_LINK *link1 = NULL;
            bool           ring_trigger = false;
            uint8_t        i;

            for (i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].hfp.call_status == APP_HFP_CALL_INCOMING)
                {
                    link1 = &(app_db.br_link[i]);

                    i++;
                    break;
                }
            }

#if F_APP_MULTILINK_ENABLE
            T_APP_BR_LINK *link2 = NULL;
            for (; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].hfp.call_status == APP_HFP_CALL_INCOMING)
                {
                    link2 = &(app_db.br_link[i]);
                    break;
                }
            }
            if (link1 != NULL || link2 != NULL)
            {
#endif
                if (link1 != NULL)
                {
                    if (app_bt_policy_cfg.enable_multi_outband_call_tone)
                    {
                        if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call) ||
                            (link1->hfp.is_inband_ring == false ||
                             link1->id != hfp.active_link_id)) /* TODO check active sco link */
                        {
                            ring_trigger = true;
                        }
                    }
                    else
                    {
                        if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call) ||
                            ((link1->hfp.is_inband_ring == false) &&
                             (link1->id == hfp.active_link_id))) /* TODO check active sco link */
                        {
                            ring_trigger = true;
                        }
                    }
                }

#if F_APP_MULTILINK_ENABLE
                if (link2 != NULL)
                {
                    if (app_bt_policy_cfg.enable_multi_outband_call_tone)
                    {
                        if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call) ||
                            (link2->hfp.is_inband_ring == false ||
                             link2->id != hfp.active_link_id)) /* TODO check active sco link */
                        {
                            ring_trigger = true;
                        }
                    }
                    else
                    {
                        if ((app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call) ||
                            ((link2->hfp.is_inband_ring == false) &&
                             (link2->id == hfp.active_link_id))) /* TODO check active sco link */
                        {
                            ring_trigger = true;
                        }
                    }
                }
            }
#endif

            if ((ring_trigger == true) && (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY))
            {
                app_audio_tone_type_play(TONE_HF_CALL_IN, true, false);
            }
            else
            {
                app_stop_timer(&hfp.tmr.indices[TIMER_RING]);
                hfp.ring_active = false;
            }
        }
        break;

    case TIMER_MIC_MUTE_ALARM:
        {
            T_APP_BR_LINK *p_link;

            p_link = &(app_db.br_link[hfp.active_link_id]);

            if (app_audio_is_mic_mute() &&
                ((p_link->hfp.call_status != APP_HFP_CALL_IDLE)
                ))
            {
                app_audio_tone_type_play(TONE_MIC_MUTE_ALARM, false, false);
            }
            else
            {
                app_stop_timer(&hfp.tmr.indices[TIMER_MIC_MUTE_ALARM]);
            }
        }
        break;

    case TIMER_NO_SERVICE:
        {
            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_audio_tone_type_play(TONE_HF_NO_SERVICE, false, false);
            }
        }
        break;

    case TIMER_ADD_SCO:
        {
            app_stop_timer(&hfp.tmr.indices[TIMER_ADD_SCO]);
        }
        break;

    case TIMER_CANCEL_VOICE_DAIL:
        {
            T_APP_BR_LINK *p_link;
            p_link = &(app_db.br_link[hfp.active_link_id]);

            app_stop_timer(&hfp.tmr.indices[TIMER_CANCEL_VOICE_DAIL]);
            if ((p_link->hfp.call_status == APP_HFP_VOICE_ACTIVATION_ONGOING) &&
                (p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) &&
                (app_hfp_sco_is_connected() == false))
            {
                bt_hfp_voice_recognition_disable_req(p_link->bd_addr);
            }
        }
        break;

    default:
        break;
    }
}

static void app_hfp_audio_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            T_AUDIO_STREAM_TYPE stream_type;

            if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == false)
            {
                break;
            }

            if (stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                if (param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED ||
                    param->track_state_changed.state == AUDIO_TRACK_STATE_RESTARTED)
                {
                    if (app_hfp_get_call_status() >= APP_HFP_CALL_ACTIVE)
                    {
                        audio_probe_dsp_ipc_send_call_status(true);
                    }
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
        APP_PRINT_INFO1("app_hfp_policy_cback: event_type 0x%04x", event_type);
    }
}

void app_hfp_hf_init(void)
{
    if (app_cfg_const.supported_profile_mask & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
    {
        bt_hfp_init(RFC_HFP_CHANN_NUM, RFC_HSP_CHANN_NUM, app_hfp_cfg.hfp_hf_brsf_capability,
                    BT_HFP_HF_CODEC_TYPE_CVSD | BT_HFP_HF_CODEC_TYPE_MSBC);
        audio_mgr_cback_register(app_hfp_audio_cback);
        bt_mgr_cback_register(app_hfp_bt_cback);

        app_timer_reg_cb(app_hfp_timeout_cb, &hfp.tmr.id);
    }
}

void app_hfp_mute_ctrl(void)
{
    uint8_t hf_active = 0;
    T_APP_BR_LINK *p_link;

    p_link = &(app_db.br_link[hfp.active_link_id]);

    if (p_link->hfp.state == APP_HF_STATE_CONNECTED)
    {
        if (p_link->sco.conn_handle != 0)
        {
            hf_active = 1;
        }
    }
    else
    {
        if (p_link->sco.conn_handle)
        {
            hf_active = 1;
        }
    }


    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        if (hf_active)
        {
            app_hfp_set_mic_mute();
            if (app_audio_is_mic_mute())
            {
#if (CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT || F_APP_CONFERENCE_HEADSET_SUPPORT)
                bt_hfp_microphone_gain_level_report(p_link->bd_addr, 0);
#endif
            }
            else
            {
#if (CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT || F_APP_CONFERENCE_HEADSET_SUPPORT)
                bt_hfp_microphone_gain_level_report(p_link->bd_addr, 0x0f);
#endif
            }
        }
    }
}

void app_hfp_stop_ring_alert_timer(void)
{
    app_stop_timer(&hfp.tmr.indices[TIMER_RING]);
}

uint8_t app_hfp_get_call_in_tone_type(T_APP_BR_LINK *p_link)
{
    T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_MAX;

    if (app_audio_tone_cfg.tone_hf_call_in != TONE_TYPE_MAX)
    {
        tone_type = TONE_HF_CALL_IN;
    }
    else if ((p_link == &(app_db.br_link[app_db.first_hf_index])) &&
             (app_audio_tone_cfg.tone_first_hf_call_in != TONE_TYPE_MAX))
    {
        tone_type = TONE_FIRST_HF_CALL_IN;
    }
    else if ((p_link == &(app_db.br_link[app_db.last_hf_index])) &&
             (app_audio_tone_cfg.tone_last_hf_call_in != TONE_TYPE_MAX))
    {
        tone_type = TONE_LAST_HF_CALL_IN;
    }

    APP_PRINT_TRACE1("app_hfp_get_call_in_tone_type: tone_type 0x%02x", tone_type);

    return tone_type;
}

void app_hfp_adjust_sco_window(uint8_t *bd_addr, T_APP_HFP_SCO_ADJUST_EVT evt)
{
    uint8_t active_hf_addr[6];
    uint8_t need_set = 0;

    if ((evt != APP_SCO_ADJUST_LINKBACK_END_EVT) && (bd_addr == NULL))
    {
        return;
    }

    if (app_hfp_sco_is_connected())
    {
        memcpy(active_hf_addr, app_db.br_link[hfp.active_link_id].bd_addr, 6);

        if (bd_addr == NULL)
        {
            // evt == APP_SCO_ADJUST_LINKBACK_END_EVT, linkback end
            need_set = 0;
        }
        else if ((app_bt_policy_get_state() == BP_STATE_LINKBACK) && (memcmp(active_hf_addr, bd_addr, 6)))
        {
            //bud is paging
            if (memcmp(active_hf_addr, bd_addr, 6))
            {
                need_set = 1;
            }
        }
        else if (src_conn_ind_check(bd_addr) && memcmp(active_hf_addr, bd_addr, 6))
        {
            //src is connecting bud
            need_set = 1;
        }
        else if ((evt == APP_SCO_ADJUST_SCO_CONN_IND_EVT) && (app_link_get_sco_conn_num() == 1) &&
                 memcmp(active_hf_addr, bd_addr, 6))
        {
            //sco conn ind from inactive hf link
            need_set = 1;
        }
        else
        {
            need_set = 0;
        }

        if (bd_addr != NULL)
        {
            APP_PRINT_TRACE4("app_hfp_adjust_sco_window bd_addr %s, active hf addr %s, evt 0x%04x, need_set %d",
                             TRACE_BDADDR(bd_addr), TRACE_BDADDR(active_hf_addr), evt, need_set);
        }

        bt_sco_link_retrans_window_set(active_hf_addr, need_set);
    }
}

bool app_hfp_sco_is_need(void)
{
    bool ret = false;

    T_APP_HFP_CALL_STATUS call_status = app_hfp_get_call_status();

    if ((call_status >= APP_HFP_CALL_OUTGOING) ||
        (call_status == APP_HFP_VOICE_ACTIVATION_ONGOING) ||
        (call_status == APP_HFP_CALL_INCOMING && app_db.br_link[hfp.active_link_id].hfp.is_inband_ring))
    {
        ret = true;
    }

    APP_PRINT_TRACE1("app_hfp_call_status_sco_is_need %d", ret);

    return ret;
}

#if F_APP_HFP_CMD_SUPPORT
void app_hfp_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                        uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    T_APP_BR_LINK *p_link = &app_db.br_link[hfp.active_link_id];

    switch (cmd_id)
    {
    case CMD_SEND_DTMF:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t number;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            else
            {
                if ((p_link->hfp.state == APP_HF_STATE_CONNECTED) &&
                    (app_hfp_get_call_status() >= APP_HFP_CALL_ACTIVE))
                {
                    if (bt_hfp_dtmf_code_transmit(p_link->bd_addr, cmd->number) == false)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
            {
                struct
                {
                    uint8_t number;
                } __attribute__((packed)) rpt = {};

                rpt.number = cmd->number;

                app_report_event(cmd_path, EVENT_DTMF, app_idx, (uint8_t *)&rpt, sizeof(rpt));
            }
        }
        break;

    case CMD_GET_SUBSCRIBER_NUM:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            else
            {
                if (p_link->hfp.state == APP_HF_STATE_CONNECTED)
                {
                    if (bt_hfp_subscriber_number_query(p_link->bd_addr) == false)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_GET_OPERATOR:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            else
            {
                if (p_link->hfp.state == APP_HF_STATE_CONNECTED)
                {
                    if (bt_hfp_network_operator_name_query(p_link->bd_addr) == false)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_SEND_VGM:
        {
            uint8_t app_idx_active = app_hfp_get_active_idx();
            uint8_t level = cmd_ptr[2];
            if (bt_hfp_microphone_gain_level_report(app_db.br_link[app_idx_active].bd_addr, level) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            if (level > 15)
            {
                level = 15;
            }
            APP_PRINT_TRACE2("CMD_SEND_VGM,volume:%d,max:%d", level, 15);

            if (p_link != NULL)
            {
                audio_track_volume_in_set(p_link->sco.track_handle, level);
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SEND_VGS:
        {
            uint8_t app_idx_active = app_hfp_get_active_idx();
            uint8_t level = cmd_ptr[2];
            if (bt_hfp_speaker_gain_level_report(app_db.br_link[app_idx_active].bd_addr, level) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            if (level > 15)
            {
                level = 15;
            }
            APP_PRINT_TRACE2("CMD_SEND_VGS,volume:%d,max:%d", level, 15);
            if (p_link != NULL)
            {
                audio_track_volume_out_set(p_link->sco.track_handle, level);
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        break;
    }
}
#endif

