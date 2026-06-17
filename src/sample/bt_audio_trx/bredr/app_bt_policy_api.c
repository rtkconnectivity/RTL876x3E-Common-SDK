/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "gap_le.h"
#include "btm.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_main.h"
#include "app_bt_policy_cfg.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_linkback.h"

#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_multilink.h"

extern T_LINKBACK_ACTIVE_NODE linkback_active_node;

extern T_BP_STATE bp_state;
extern T_EVENT cur_event;

extern T_BT_DEVICE_MODE radio_mode;

extern T_B2S_CONNECTED b2s_connected;
extern bool first_connect_sync_default_volume_to_src;
static uint8_t enlarge_tpoll_state = false;

const T_BP_TPOLL_MAPPING tpoll_table[BP_TPOLL_MAX] =
{
    {BP_TPOLL_INIT, 0},
    {BP_TPOLL_IDLE, 40},

#if F_APP_INTEGRATED_TRANSCEIVER
    {BP_TPOLL_A2DP, 24},
#else
    {BP_TPOLL_A2DP, 26},
#endif

    {BP_TPOLL_IDLE_SINGLE_LINKBACK, 120},
};

void app_bt_policy_startup(T_BP_IND_FUN fun, bool at_once_trigger)
{
    T_STARTUP_PARAM param;

    param.ind_fun = fun;
    param.at_once_trigger = at_once_trigger;
    app_bt_policy_state_machine(EVENT_STARTUP, &param);
}

void app_bt_policy_shutdown(void)
{
    app_bt_policy_state_machine(EVENT_SHUTDOWN, NULL);
}

void app_bt_policy_stop(void)
{
    app_bt_policy_state_machine(EVENT_STOP, NULL);
}

void app_bt_policy_restore(void)
{
    app_bt_policy_state_machine(EVENT_RESTORE, NULL);
}

void app_bt_policy_msg_prof_conn(uint8_t *bd_addr, uint32_t prof)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = prof;

    app_bt_policy_state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
}

void app_bt_policy_msg_prof_disconn(uint8_t *bd_addr, uint32_t prof, uint16_t cause)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = prof;
    bt_param.cause = cause;

    app_bt_policy_state_machine(EVENT_PROFILE_DISCONN, &bt_param);
}

void app_bt_policy_enter_pairing_mode(bool force, bool visiable)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.is_force = force;
    bt_param.is_visiable = visiable;

    app_bt_policy_state_machine(EVENT_DEDICATED_ENTER_PAIRING_MODE, &bt_param);
}

void app_bt_policy_exit_pairing_mode(void)
{
    app_bt_policy_state_machine(EVENT_DEDICATED_EXIT_PAIRING_MODE, NULL);
}

void app_bt_policy_enter_dut_test_mode(void)
{
    app_bt_policy_state_machine(EVENT_ENTER_DUT_TEST_MODE, NULL);
}

void app_bt_policy_default_connect(uint8_t *bd_addr, uint32_t plan_profs, bool check_bond_flag)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = plan_profs;
    bt_param.is_special = false;
    bt_param.check_bond_flag = check_bond_flag;
    app_bt_policy_state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
}

void app_bt_policy_special_connect(uint8_t *bd_addr, uint32_t plan_prof)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = plan_prof;
    bt_param.is_special = true;
    bt_param.check_bond_flag = false;
    app_bt_policy_state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
}

void app_bt_policy_disconnect(uint8_t *bd_addr, uint32_t plan_profs)
{
    T_BT_PARAM bt_param;

    ENGAGE_PRINT_TRACE2("app_bt_policy_disconnect: bd_addr %s, prof 0x%08x", TRACE_BDADDR(bd_addr),
                        plan_profs);

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = plan_profs;
    app_bt_policy_state_machine(EVENT_DEDICATED_DISCONNECT, &bt_param);
}

void app_bt_policy_disconnect_all_link(void)
{
    app_bt_policy_state_machine(EVENT_DISCONNECT_ALL, NULL);
}

T_BP_STATE app_bt_policy_get_state(void)
{
    return bp_state;
}

T_BT_DEVICE_MODE app_bt_policy_get_radio_mode(void)
{
    return radio_mode;
}


uint8_t app_bt_policy_get_b2s_connected_num(void)
{
    return b2s_connected.num;
}

uint8_t app_bt_policy_get_b2s_connected_num_with_profile(void)
{
    uint8_t phone_link_num = 0;

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].connected_profile & (~RDTP_PROFILE_MASK))
        {
            phone_link_num++;
        }
    }

    return phone_link_num;
}

void app_bt_policy_set_b2s_connected_num_max(uint8_t num_max)
{
    b2s_connected.num_max = num_max;
}

void app_bt_policy_sync_b2s_connected(void)
{
    app_db.b2s_connected_num = app_bt_policy_get_b2s_connected_num();

    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
    {
        //app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_PHONE_CONNECTED);
    }
}

bool app_bt_policy_get_first_connect_sync_default_vol_flag(void)
{
    return first_connect_sync_default_volume_to_src;
}

void app_bt_policy_set_first_connect_sync_default_vol_flag(bool flag)
{
    first_connect_sync_default_volume_to_src = flag;
}

void app_bt_policy_set_idle_tpoll(T_BP_TPOLL_STATE tpoll_state)
{
    uint8_t inactive_a2dp_idx;
    bool ret = app_a2dp_get_inactive_idx(&inactive_a2dp_idx);
    if (ret == false)
    {
        return;
    }

    if (tpoll_state == BP_TPOLL_A2DP)
    {
        if (app_db.br_link[inactive_a2dp_idx].tpoll.status !=
            tpoll_table[BP_TPOLL_IDLE].state)
        {
            bt_link_qos_set(app_db.br_link[inactive_a2dp_idx].bd_addr, BT_QOS_TYPE_GUARANTEED,
                            (tpoll_table[BP_TPOLL_IDLE].tpoll_value + (inactive_a2dp_idx * 2)));
            app_db.br_link[inactive_a2dp_idx].tpoll.status = tpoll_table[BP_TPOLL_IDLE].state;
        }
    }
    else
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_link_check_b2s_link_by_id(i))
            {
                if (app_db.br_link[i].tpoll.status != tpoll_table[tpoll_state].state)
                {
                    bt_link_qos_set(app_db.br_link[i].bd_addr, BT_QOS_TYPE_GUARANTEED,
                                    (tpoll_table[tpoll_state].tpoll_value + (i * 2)));
                    app_db.br_link[i].tpoll.status = tpoll_table[tpoll_state].state;
                }
            }
        }
    }
}

void app_bt_policy_set_active_tpoll_only(uint8_t idx, T_BP_TPOLL_STATE tpoll_state)
{
    if (app_link_check_b2s_link_by_id(idx))
    {
        if (app_db.br_link[idx].tpoll.status != tpoll_table[tpoll_state].state)
        {
            bt_link_qos_set(app_db.br_link[idx].bd_addr, BT_QOS_TYPE_GUARANTEED,
                            tpoll_table[tpoll_state].tpoll_value);
            app_db.br_link[idx].tpoll.status = tpoll_table[tpoll_state].state;
        }
    }
}

uint8_t app_bt_policy_get_enlarge_tpoll_state(void)
{
    return enlarge_tpoll_state;
}

void app_bt_policy_set_enlarge_tpoll_state(T_BP_TPOLL_EVENT event)
{
    if (((event == BP_TPOLL_ACL_CONN_EVENT) ||
         (event == BP_TPOLL_LINKBACK_START)) &&
        (app_link_get_b2s_link_num() == 1) &&
        (app_bt_policy_get_state() == BP_STATE_LINKBACK) &&
        (app_db.br_link[app_a2dp_get_active_idx()].a2dp.is_streaming == false))
    {
        //page b2s when already conn_1_b2s
        enlarge_tpoll_state = true;
    }
    else
    {
        enlarge_tpoll_state = false;
    }

    APP_PRINT_TRACE2("app_bt_policy_get_enlarge_tpoll_state %d,%d", event,
                     enlarge_tpoll_state);
}

void app_bt_policy_qos_param_update(uint8_t *bd_addr, T_BP_TPOLL_EVENT event)
{
    uint8_t find_bau_app_idx = MAX_BR_LINK_NUM;
    uint8_t app_idx = app_a2dp_get_active_idx();
    uint8_t inactive_a2dp_idx = 0xFF; // invalid value

    uint8_t j;
    //ignore cmd
    if (event == BP_TPOLL_ACL_CONN_EVENT)
    {
        if (app_link_get_b2s_link_num() == MULTILINK_SRC_CONNECTED)
        {
            for (j = 0; j < MAX_BR_LINK_NUM; j++)
            {
                if (app_link_check_b2s_link_by_id(j))
                {
                    bt_link_tpoll_range_set(app_db.br_link[j].bd_addr, 0, 0);
                }
            }
        }
    }
    else if (event == BP_TPOLL_ACL_DISC_EVENT)
    {
        if (app_link_get_b2s_link_num() != MULTILINK_SRC_CONNECTED)
        {
            for (j = 0; j < MAX_BR_LINK_NUM; j++)
            {
                if (app_link_check_b2s_link_by_id(j))
                {
                    bt_link_tpoll_range_set(app_db.br_link[j].bd_addr, 0x06, 0x1000);
                }
            }
        }
    }

    app_bt_policy_set_enlarge_tpoll_state(event);

    if ((app_bt_policy_cfg_nv.enable_multi_link) &&
        (app_link_get_b2s_link_num() == MULTILINK_SRC_CONNECTED))
    {
        app_bt_policy_set_idle_tpoll(BP_TPOLL_IDLE);
    }
    else
    {
        if (app_bt_policy_get_enlarge_tpoll_state() == true)
        {
            T_APP_BR_LINK *p_link;
            p_link = app_link_find_br_link(bd_addr);
            if (p_link)
            {
                app_bt_policy_set_active_tpoll_only(p_link->id, BP_TPOLL_IDLE_SINGLE_LINKBACK);
            }
        }
        else
        {
            app_bt_policy_set_idle_tpoll(BP_TPOLL_IDLE);
        }
    }

    if (app_db.br_link[app_idx].a2dp.is_streaming == true)//a2dp
    {
        APP_PRINT_TRACE0("app_bt_policy_qos_param_update: a2dp streaming!");
        app_bt_policy_set_idle_tpoll(BP_TPOLL_A2DP);

        app_bt_policy_set_active_tpoll_only(app_idx, BP_TPOLL_A2DP);

    }
    else //idle
    {
        app_bt_policy_set_idle_tpoll(BP_TPOLL_IDLE);
    }

    app_a2dp_get_inactive_idx(&inactive_a2dp_idx);

    APP_PRINT_TRACE8("app_bt_policy_qos_param_update:event %u, sco %d, a2dp_idx %d, avrcp.play_status %d, a2dp.is_streaming %d, tpoll.status %d, inactive_idx %d, bau_idx %d",
                     event,
                     app_hfp_sco_is_connected(),
                     app_idx,
                     app_db.br_link[app_idx].avrcp.play_status,
                     app_db.br_link[app_idx].a2dp.is_streaming,
                     app_db.br_link[app_idx].tpoll.status,
                     inactive_a2dp_idx,
                     find_bau_app_idx);
}

uint8_t *app_bt_policy_get_linkback_device(void)
{
    if (linkback_active_node.is_valid)
    {
        return linkback_active_node.linkback_node.bd_addr;
    }
    else
    {
        return NULL;
    }
}
