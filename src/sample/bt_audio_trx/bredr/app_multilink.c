/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "app_timer.h"
#include "gap_br.h"
#include "btm.h"
#include "bt_bond.h"
#include "remote.h"
#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "bt_hfp.h"
#include "app_a2dp.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_hfp_cfg.h"
#include "app_hfp.h"
#include "audio_track.h"
#include "app_link_util_cs.h"
#include "app_main.h"
#include "app_bond.h"

#include "app_multilink.h"
#include "app_audio_policy.h"
#include "app_bt_policy_cfg.h"
#include "app_bt_policy_api.h"

#include "audio_volume.h"
#include "audio_track.h"

#include "app_auto_power_off.h"
#include "app_device.h"


#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif


typedef enum
{
    TIMER_DISC_VA_SCO,
    TIMER_MAX,
} T_MULTILINK_TIMER;


#define ERWS_MULTILINK_SUPPORT


static struct
{
    uint8_t id;
    uint8_t indices[TIMER_MAX];
} tmr = {};


static bool enabled = false;

// static struct
// {
//     uint8_t active_link_id;
// } hfp;

static struct
{
    uint8_t wait_resume_link_id;
} a2dp =
{
    .wait_resume_link_id    = MAX_BR_LINK_NUM,
};

static bool set_active_a2dp_avrcp(uint8_t *bd_addr)
{
    uint8_t active_a2dp_idx = 0;
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link)
    {
        app_a2dp_set_active_idx(p_link->id);

        if (p_link->a2dp.track_handle)
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

        app_audio_set_avrcp_status(p_link->avrcp.play_status);

        return bt_a2dp_active_link_set(bd_addr);
    }
    return false;
}


static void set_active_a2dp_avrcp_with_prio(uint8_t idx)
{
    app_db.br_link[idx].avrcp.play_status = BT_AVRCP_PLAY_STATUS_PLAYING;
    set_active_a2dp_avrcp(app_db.br_link[idx].bd_addr);
    app_bond_set_priority(app_db.br_link[idx].bd_addr);
}


static uint8_t find_other_link_by_bond_prio(uint8_t curr_idx)
{
    uint8_t priority;
    uint8_t bond_num;
    uint8_t bd_addr[6];
    T_APP_BR_LINK *p_link = NULL;
    uint8_t active_idx = 0;

    bond_num = app_bond_b2s_num_get();

    for (priority = 1; priority <= bond_num; priority++)
    {
        if (app_bond_b2s_addr_get(priority, bd_addr) == true)
        {
            p_link = app_link_find_b2s_link(bd_addr);
            if (p_link != NULL)
            {
                if (memcmp(p_link->bd_addr, app_db.br_link[curr_idx].bd_addr, 6))
                {
                    active_idx = p_link->id;
                    break;
                }
            }
        }
    }

    APP_PRINT_TRACE2("find_other_link_by_bond_prio: curr_idx %d, active_idx %d",
                     curr_idx, active_idx);

    return active_idx;
}

static void btm_evt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    T_APP_BR_LINK *p_link = NULL;
    T_BT_EVENT_PARAM *param = event_buf;
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    switch (event_type)
    {
    case BT_EVENT_SCO_CONN_IND:
        {
        }
        break;

    case BT_EVENT_SCO_CONN_RSP:
        {
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            p_link = app_link_find_br_link(param->sco_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (param->sco_conn_cmpl.cause == 0)
                {

#if !F_APP_INTEGRATED_TRANSCEIVER
                    if (app_link_get_sco_conn_num() == 1)
                    {
                        bt_sco_link_switch(param->sco_conn_cmpl.bd_addr);
                    }
                    else
                    {
                        APP_PRINT_TRACE2("btm_evt_cback: active sco link %s, current link %s",
                                         TRACE_BDADDR(app_db.br_link[active_hf_idx].bd_addr),
                                         TRACE_BDADDR(param->sco_conn_cmpl.bd_addr));

                        bt_sco_link_switch(app_db.br_link[active_hf_idx].bd_addr);
                    }
#endif

                    if (app_bt_policy_cfg.enable_multi_sco_disc_resume)
                    {
                        app_pause_other_a2dp_avrcp(p_link->id, true);
                    }
                    else
                    {
                        app_pause_other_a2dp_avrcp(p_link->id, false);
                    }

                    if ((active_hf_idx != p_link->id) && (p_link->sco.conn_handle))
                    {
                        bt_hfp_audio_disconnect_req(p_link->bd_addr);
                    }

                }
            }
            if (app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call)
            {
                app_hfp_inband_tone_mute_ctrl();
            }
        }
        break;

    case BT_EVENT_SCO_DISCONNECTED:
        {
            if (param->sco_disconnected.cause == (HCI_ERR | HCI_ERR_CMD_DISALLOWED))
            {
                break;
            }



            p_link = app_link_find_br_link(param->sco_disconnected.bd_addr);
            if (p_link != NULL)
            {
                p_link->sco.conn_handle = 0;

                //if (p_link->id == active_hf_idx)
                {
                    uint8_t i;

#if !F_APP_INTEGRATED_TRANSCEIVER
                    //set actvie sco
                    for (i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if (app_db.br_link[i].sco.conn_handle)
                        {
                            bt_sco_link_switch(app_db.br_link[i].bd_addr);
                            break;
                        }
                    }
#endif

                }

                if (app_bt_policy_cfg.enable_multi_sco_disc_resume)
                {
                    if ((app_bt_policy_cfg_nv.enable_multi_link) &&
                        (app_link_get_b2s_link_num() == MULTILINK_SRC_CONNECTED))
                    {
                        APP_PRINT_TRACE4("sco_disc_resume %d,%d,%d,%d", app_link_get_sco_conn_num(),
                                         active_hf_idx,
                                         p_link->id,
                                         app_hfp_get_call_status());
                        if ((app_link_get_sco_conn_num() == 0) &&
                            (app_hfp_get_call_status() == APP_HFP_CALL_IDLE))
                        {
                            app_resume_a2dp_avrcp(p_link->id);
                        }
                    }
                }
            }
            if (app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call)
            {
                app_hfp_inband_tone_mute_ctrl();
            }

            app_bt_policy_qos_param_update(param->sco_disconnected.bd_addr, BP_TPOLL_SCO_DISC_EVENT);
        }
        break;

    case BT_EVENT_A2DP_CONN_CMPL:
        {
            p_link = app_link_find_br_link(param->a2dp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_CONNECTED);
            }
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            p_link = app_link_find_br_link(param->a2dp_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_DISC);
            }
        }
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_RSP:
    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED:
        {
            p_link = app_link_find_br_link(param->avrcp_play_status_changed.bd_addr);
            if (p_link != NULL)
            {
                APP_PRINT_TRACE2("BT_EVENT_AVRCP_PLAY_STATUS_CHANGED %d ,%d", p_link->id,
                                 param->avrcp_play_status_changed.play_status);
                if (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    app_audio_set_bud_stream_state(BUD_STREAM_STATE_AUDIO);
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_MEDIAPLAYER_PLAYING);
                }
                else if (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PAUSED)
                {
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_MEDIAPLAYER_PAUSED);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            p_link = app_link_find_br_link(param->a2dp_stream_start_ind.bd_addr);
            if (p_link != NULL)
            {
                APP_PRINT_TRACE5("btm_evt_cback: p_link %p, id %d, active id %d, a2dp.is_streaming %d / %d",
                                 p_link, p_link->id, active_a2dp_idx, p_link->a2dp.is_streaming,
                                 app_db.br_link[active_a2dp_idx].a2dp.is_streaming);

                if (p_link->id != active_a2dp_idx)
                {
                    // not active link
                    if (app_db.br_link[active_a2dp_idx].a2dp.is_streaming == false)
                    {
                        bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
                        p_link->a2dp.is_streaming = true;
                        app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                        if (app_bt_policy_cfg.disable_multilink_preemptive)
                        {
                            app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                        }
                    }
                    else
                    {
                        bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
                        p_link->a2dp.is_streaming = true;
                        app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                    }
                }
                else
                {
                    // active link
                    if (p_link->a2dp.is_streaming == false)
                    {
                        bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
                        p_link->a2dp.is_streaming = true;
                        app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                    }
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            p_link = app_link_find_br_link(param->a2dp_stream_start_rsp.bd_addr);
            if (p_link != NULL)
            {
                p_link->a2dp.is_streaming = true;
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            p_link = app_link_find_br_link(param->a2dp_stream_stop.bd_addr);
            if (p_link != NULL)
            {
                p_link->a2dp.is_streaming = false;
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_STOP);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            p_link = app_link_find_br_link(param->a2dp_stream_close.bd_addr);
            if (p_link != NULL)
            {
                p_link->a2dp.is_streaming = false;
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_STOP);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            //b2b or b2s link disconnected => reset preemptive flag
            app_db.a2dp_preemptive_flag = false;
        }
        break;

    case BT_EVENT_HFP_VOICE_RECOGNITION_ACTIVATION:
        {
            if (param->hfp_voice_recognition_activation.result == BT_HFP_CMD_OK)
            {
                uint8_t cmd_ptr[7];
                p_link = &(app_db.br_link[active_a2dp_idx]);

                if (p_link != NULL)
                {
                    audio_track_volume_out_mute(p_link->a2dp.track_handle);
                }
            }
        }
        break;

    case BT_EVENT_HFP_VOICE_RECOGNITION_DEACTIVATION:
        {
            if (param->hfp_voice_recognition_deactivation.result == BT_HFP_CMD_OK)
            {
                uint8_t cmd_ptr[7];
                uint8_t inactive_hf_idx = MAX_BR_LINK_NUM;
                p_link = &(app_db.br_link[active_a2dp_idx]);

                if (p_link != NULL)
                {
                    audio_track_volume_out_unmute(p_link->a2dp.track_handle);
                    p_link->a2dp.muted = false;
                }
            }
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
            uint8_t cmd_ptr[7];
            T_APP_HFP_CALL_STATUS inactive_hf_status = APP_HFP_CALL_IDLE;
            p_link = &(app_db.br_link[active_a2dp_idx]);

            if (p_link != NULL)
            {
                if (param->hfp_call_status.curr_status != BT_HFP_CALL_IDLE)
                {
                    audio_track_volume_out_mute(p_link->a2dp.track_handle);
                }
                else
                {
                    audio_track_volume_out_unmute(p_link->a2dp.track_handle);
                    p_link->a2dp.muted = false;
                }
            }
        }
        break;

    case BT_EVENT_HFP_DISCONN_CMPL:
        {
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("btm_evt_cback: event_type 0x%04x", event_type);
    }
}


static void timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("timeout_cb: timer_evt %d, param %d", timer_evt, param);
    switch (timer_evt)
    {

    default:
        break;
    }
}

void app_multi_disc_inactive_link(uint8_t app_idx)
{
    uint8_t call_num = 0;
    uint8_t i;
    T_APP_BR_LINK *p_link = NULL;
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used == true &&
            app_db.br_link[i].hfp.call_status != APP_HFP_CALL_IDLE)
        {
            call_num++;
        }
    }

    APP_PRINT_TRACE4("app_multi_disc_inactive_link: active_a2dp_idx %d, app_idx %d, "
                     "call_num %d, max_legacy_multilink_devices %d",
                     app_a2dp_get_active_idx(), app_idx, call_num,
                     app_bt_policy_cfg.max_legacy_multilink_devices);

    if (call_num >= app_bt_policy_cfg.max_legacy_multilink_devices)
    {
        app_bt_policy_disconnect(app_db.br_link[app_idx].bd_addr, ALL_PROFILE_MASK);
        return;
    }

    if (call_num > 0)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_link_check_b2s_link_by_id(i))
            {
                if ((i != app_idx) && (i != active_hf_idx))
                {
                    app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                    return;
                }
            }
        }
    }
    else
    {
        uint8_t priority;
        uint8_t bond_num;
        uint8_t bd_addr[6];

        bond_num = app_bond_b2s_num_get();
        if (bond_num)
        {
            for (priority = bond_num; priority > 0; priority--)
            {
                if (app_bond_b2s_addr_get(priority, bd_addr))
                {
                    p_link = app_link_find_b2s_link(bd_addr);
                    if (p_link != NULL)
                    {
                        if ((p_link->id != app_idx))
                        {
                            app_bt_policy_disconnect(bd_addr, ALL_PROFILE_MASK);
                            return;
                        }
                    }
                }
            }
        }
        //disconnect one device if no bond and priority information when ACL connected
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_link_check_b2s_link_by_id(i))
            {
                if (i != app_idx)
                {
                    app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                    return;
                }
            }
        }
    }
}


void app_multi_enable(bool is_on_by_mmi)
{
    enabled = is_on_by_mmi;
}

bool app_multi_is_enabled(void)
{
    return enabled;
}


//multi-link a2dp
bool app_pause_other_a2dp_avrcp(uint8_t keep_active_a2dp_idx, uint8_t resume_fg)
{
    uint8_t i;
    bool play_pause = false;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if ((i != keep_active_a2dp_idx) &&
            (app_db.br_link[i].connected_profile & (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK)))
        {
            if (app_db.br_link[i].a2dp.is_streaming == true)
            {

                if (resume_fg)
                {
                    a2dp.wait_resume_link_id = i;
                }

                if (app_db.br_link[i].avrcp.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    bt_avrcp_pause(app_db.br_link[i].bd_addr);
                }
                audio_track_stop(app_db.br_link[i].a2dp.track_handle);
                app_db.br_link[i].avrcp.play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                play_pause = true;
                // Stop Sniffing Now
                //app_bt_sniffing_stop(app_db.br_link[i].bd_addr, BT_SNIFFING_TYPE_A2DP);
            }
        }
    }
    APP_PRINT_TRACE5("app_pause_other_a2dp_avrcp: active_a2dp_idx %d, a2dp.wait_resume_link_id %d, "
                     "i %d, keep_active_a2dp_idx %d, play_pause %d",
                     app_a2dp_get_active_idx(), a2dp.wait_resume_link_id, i, keep_active_a2dp_idx, play_pause);
    return play_pause;
}

void app_resume_a2dp_avrcp(uint8_t app_idx)
{
    if (a2dp.wait_resume_link_id < MAX_BR_LINK_NUM)
    {
        //if (a2dp.wait_resume_link_id != app_idx)
        {
            if (app_db.br_link[a2dp.wait_resume_link_id].connected_profile &
                (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK))
            {
                set_active_a2dp_avrcp(app_db.br_link[a2dp.wait_resume_link_id].bd_addr);
                app_bond_set_priority(app_db.br_link[a2dp.wait_resume_link_id].bd_addr);
                if (app_db.br_link[a2dp.wait_resume_link_id].avrcp.play_status !=
                    BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    app_db.br_link[a2dp.wait_resume_link_id].avrcp.play_status =
                        BT_AVRCP_PLAY_STATUS_PLAYING;
                    bt_avrcp_play(app_db.br_link[a2dp.wait_resume_link_id].bd_addr);
                }
            }
        }
        //all sco are removed, the variable must be clear
        a2dp.wait_resume_link_id = MAX_BR_LINK_NUM;
    }
    APP_PRINT_TRACE4("app_resume_a2dp_avrcp: active_a2dp_idx %d, a2dp.wait_resume_link_id %d, "
                     "app_idx %d, avrcp_play_status 0x%02x",
                     app_a2dp_get_active_idx(), a2dp.wait_resume_link_id, app_idx,
                     app_db.br_link[app_a2dp_get_active_idx()].avrcp.play_status);
}


void app_judge_active_a2dp_idx_and_qos(uint8_t app_idx, T_APP_JUDGE_A2DP_EVENT event)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    APP_PRINT_TRACE6("app_judge_active_a2dp_idx_and_qos: 1 event %d, active_a2dp_idx %d, app_idx %d, "
                     "a2dp.wait_resume_link_id %d, is_streaming %d, active_media_paused %d",
                     event, active_a2dp_idx, app_idx, a2dp.wait_resume_link_id,
                     app_db.br_link[active_a2dp_idx].a2dp.is_streaming, app_db.active_media_paused);
    switch (event)
    {
    case JUDGE_EVENT_A2DP_CONNECTED:
        {
            uint8_t link_number = app_connected_profile_link_num(A2DP_PROFILE_MASK);
            if (link_number <= 1)
            {
                set_active_a2dp_avrcp(app_db.br_link[app_idx].bd_addr);
                app_bond_set_priority(app_db.br_link[app_idx].bd_addr);
                if (link_number <= 0)
                {
                    //exception
                }
            }
            else
            {
                if ((app_db.br_link[active_a2dp_idx].a2dp.is_streaming == false) &&
                    (app_hfp_get_call_status() == APP_HFP_CALL_IDLE))
                {
                    if (app_bt_policy_cfg_nv.enable_multi_link)
                    {
                        app_bond_set_priority(app_db.br_link[app_idx].bd_addr);
                        app_bond_set_priority(app_db.br_link[find_other_link_by_bond_prio(app_idx)].bd_addr);
                    }
                    else
                    {
                        set_active_a2dp_avrcp(app_db.br_link[app_idx].bd_addr);
                        app_bond_set_priority(app_db.br_link[app_idx].bd_addr);
                    }
                }
            }
        }
        break;

    case JUDGE_EVENT_A2DP_START:
        {
            APP_PRINT_TRACE3("JUDGE_EVENT_A2DP_START: active_a2dp_idx %d, avrcp %d, stream %d",
                             active_a2dp_idx,
                             app_db.br_link[active_a2dp_idx].avrcp.play_status,
                             app_db.br_link[active_a2dp_idx].a2dp.is_streaming);

            if (app_db.br_link[active_a2dp_idx].hfp.call_status != APP_HFP_CALL_IDLE)
            {
                if (app_bt_policy_cfg.enable_multi_sco_disc_resume)
                {
                    app_pause_other_a2dp_avrcp(active_hf_idx, true);
                }
                else
                {
                    app_pause_other_a2dp_avrcp(active_hf_idx, false);
                }
            }

            app_bt_policy_qos_param_update(app_db.br_link[app_idx].bd_addr, BP_TPOLL_A2DP_PLAY_EVENT);
        }
        break;

    case JUDGE_EVENT_A2DP_DISC:
        {
            if (active_a2dp_idx == app_idx)
            {
                //app_bt_sniffing_stop(app_db.br_link[app_idx].bd_addr, BT_SNIFFING_TYPE_A2DP);
                if (app_connected_profile_link_num(A2DP_PROFILE_MASK))
                {
                    set_active_a2dp_avrcp(app_db.br_link[find_other_link_by_bond_prio(
                                                             app_idx)].bd_addr);
                    app_bond_set_priority(app_db.br_link[find_other_link_by_bond_prio(app_idx)].bd_addr);
                }
            }
            app_bt_policy_qos_param_update(app_db.br_link[app_idx].bd_addr, BP_TPOLL_A2DP_STOP_EVENT);
        }
        break;

    case JUDGE_EVENT_A2DP_STOP:
        {
            app_db.br_link[app_idx].avrcp.play_status = BT_AVRCP_PLAY_STATUS_PAUSED;

            if ((active_a2dp_idx == app_idx) && (a2dp.wait_resume_link_id == MAX_BR_LINK_NUM))
            {
                uint8_t i;
                for (i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    uint8_t idx = find_other_link_by_bond_prio(active_a2dp_idx);
                    if (app_db.br_link[idx].connected_profile &
                        (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK))
                    {
                        if ((idx != active_a2dp_idx) &&
                            (app_db.br_link[idx].a2dp.is_streaming == true))
                        {
                            APP_PRINT_TRACE2("JUDGE_EVENT_A2DP_STOP: active_a2dp_idx %d, idx %d", active_a2dp_idx, idx);
                            set_active_a2dp_avrcp_with_prio(idx);
                            break;
                        }
                    }
                }
            }
            app_bt_policy_qos_param_update(app_db.br_link[app_idx].bd_addr, BP_TPOLL_A2DP_STOP_EVENT);
        }
        break;

    case JUDGE_EVENT_MEDIAPLAYER_PLAYING:
        {
            if (app_bt_policy_cfg.disable_multilink_preemptive)
            {

            }
            else
            {
                APP_PRINT_TRACE3("JUDGE_EVENT_MEDIAPLAYER_PLAYING: preemptive active_a2dp_idx %d, app_idx %d, is_streaming %d",
                                 active_a2dp_idx, app_idx, app_db.br_link[app_idx].a2dp.is_streaming);
                if (app_db.br_link[app_idx].a2dp.is_streaming == true)
                {
                    if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
                    {
                        if (app_bt_policy_cfg.enable_multi_sco_disc_resume)
                        {
                            app_pause_other_a2dp_avrcp(active_hf_idx, true);
                        }
                        else
                        {
                            app_pause_other_a2dp_avrcp(active_hf_idx, false);
                        }
                    }
                    else
                    {
                        set_active_a2dp_avrcp(app_db.br_link[app_idx].bd_addr);
                        app_pause_other_a2dp_avrcp(app_idx, false);
                        app_bond_set_priority(app_db.br_link[app_idx].bd_addr);
                    }
                }
            }
            app_bt_policy_qos_param_update(app_db.br_link[app_idx].bd_addr, BP_TPOLL_A2DP_PLAY_EVENT);
        }
        break;

    case JUDGE_EVENT_MEDIAPLAYER_PAUSED:
        {
        }
        break;

    case JUDGE_EVENT_SNIFFING_STOP:
        {
        }
        break;

    default:
        break;
    }

    if (active_a2dp_idx == app_idx)
    {
        if (event == JUDGE_EVENT_A2DP_START)
        {
            app_audio_a2dp_play_status_update(APP_A2DP_STREAM_A2DP_START);
            if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_AUDIO);
            }
        }
        else if (event == JUDGE_EVENT_A2DP_STOP)
        {
            app_audio_a2dp_play_status_update(APP_A2DP_STREAM_A2DP_STOP);
            if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_AUDIO,
                                          app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
            }
        }
    }

    APP_PRINT_TRACE4("app_judge_active_a2dp_idx_and_qos: 2 event %d, active_a2dp_idx %d, app_idx %d, "
                     "a2dp.wait_resume_link_id %d",
                     event, active_a2dp_idx, app_idx, a2dp.wait_resume_link_id);
}


void app_multi_active_hfp_transfer(uint8_t idx)
{
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    if (idx != active_hf_idx)
    {
        if (app_db.br_link[active_hf_idx].hfp.call_status == APP_HFP_CALL_INCOMING)
        {

        }
        else
        {
            bt_hfp_audio_disconnect_req(app_db.br_link[active_hf_idx].bd_addr);
        }
    }
}


void app_multi_preempt_hfp(uint8_t inactive_idx)
{
    APP_PRINT_TRACE1("app_multi_preempt_hfp:%d", inactive_idx);
    bt_hfp_audio_disconnect_req(app_db.br_link[inactive_idx].bd_addr);
}


uint8_t app_multi_get_acl_connect_num(void)
{
    uint8_t conn_num = 0;
    uint8_t i = 0;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        APP_PRINT_TRACE3("get_acl_le_link_connect_num %d %d %d",
                         i,
                         app_db.le_link[i].state,
                         app_db.le_link[i].used);

        if ((app_db.le_link[i].state == LE_LINK_STATE_CONNECTED) &&
            (app_db.le_link[i].used == true) &&
            (app_db.le_link[i].cmd.enable == true))
        {
            conn_num++;
        }
    }
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        APP_PRINT_TRACE3("get_acl_br_link_connect_num %d %d 0x%x",
                         i,
                         app_db.br_link[i].acl.conn_handle,
                         app_db.br_link[i].connected_profile);

        if ((app_db.br_link[i].connected_profile & (SPP_PROFILE_MASK | IAP_PROFILE_MASK)) &&
            (app_db.br_link[i].cmd.enable == true))
        {
            conn_num++;
        }
    }
    APP_PRINT_TRACE1("CONN_NUM = %d", conn_num);
    return conn_num;
}


void app_multilink_init(void)
{
    bt_mgr_cback_register(btm_evt_cback);
    app_timer_reg_cb(timeout_cb, &tmr.id);
}
