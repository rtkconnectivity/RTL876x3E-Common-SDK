/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "os_sched.h"
#include "app_timer.h"
#include "gap_br.h"
#include "gap_le.h"
#include "bt_avrcp.h"
#include "bt_spp.h"
#include "bt_bond.h"
#include "bt_rdtp.h"
#include "bt_hfp.h"
#include "sysm.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_dsp_cfg.h"
#include "app_main.h"

#include "app_bt_policy_cfg.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_sniff_mode_cs.h"
#include "app_ble_gap.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_linkback.h"
#include "app_multilink.h"

#include "app_audio_policy.h"
#include "app_mmi.h"

#include "app_bond.h"
#include "app_auto_power_off.h"
#include "app_cmd.h"
#include "app_cmd_br.h"
#include "app_adv_stop_cause.h"
#include "app_test.h"
#include "app_link_util_cs.h"

#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif

#include "audio_type.h"

#include "app_sniff_mode_cs.h"
#include "app_sdp.h"
#include "app_hfp.h"

#include "app_ipc.h"

#if F_APP_GUI_SUPPORT
#include "app_panel_bredr_db.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_cmd.h"
#include "app_tri_dongle_mgr.h"
#include "app_tri_dongle_bond_manager.h"
#endif

#define ROLE_SWITCH_COUNT_MAX                 (5)
#define ROLE_SWITCH_DELAY_MS                  (250)
#define ROLE_SWITCH_WINDOW_DELAY_MS           (2000)

#define NORMAL_PAGESCAN_INTERVAL              (0x800)
#define NORMAL_PAGESCAN_WINDOW                (0x12)

#define PRI_FAST_PAGESCAN_INTERVAL            (0x200)
#define PRI_FAST_PAGESCAN_WINDOW              (0x12)

#define PRI_VERY_FAST_PAGESCAN_INTERVAL       (0x40)
#define PRI_VERY_FAST_PAGESCAN_WINDOW         (0x12)

#define SEC_FAST_PAGESCAN_INTERVAL            (0x300)
#define SEC_FAST_PAGESCAN_WINDOW              (0x90)

#define PRIMARY_BLE_ADV_INTERVAL_VOICE        (0x20)
#define PRIMARY_BLE_ADV_INTERVAL_AUDIO        (0xa0)
#define PRIMARY_BLE_ADV_INTERVAL_NORMAL       (0xd0)

#define SECONDARY_BLE_ADV_INTERVAL_NORMAL     (0xb0)

#define PRIMARY_WAIT_COUPLING_TIMEOUT_MS      (500)

#define SRC_CONN_IND_MAX_NUM                  (2)
#define SRC_CONN_IND_DELAY_MS                 (1500)

#define SHUT_DOWN_STEP_TIMER_MS               (100)
#define DISCONN_B2S_PROFILE_WAIT_TIMES        (4) /* wait time(ms): 4 * SHUT_DOWN_STEP_TIMER_MS */
#define DISCONN_B2S_LINK_WAIT_TIMES_MAX       (4) /* max wait time(ms): 4 * SHUT_DOWN_STEP_TIMER_MS */
#define TPOLL_RESET_MAX_NUM                   (1) /* max number for reset tpoll config */

#define GOLDEN_RANGE_B2S_MAX                  (10)
#define GOLDEN_RANGE_B2S_MIN                  (0)

#define A2DP_CONN_IND_DELAY_MS                (8000)

extern T_LINKBACK_ACTIVE_NODE linkback_active_node;

T_STATE cur_state = STATE_STARTUP;
T_BP_STATE bp_state = BP_STATE_IDLE;
T_BP_STATE pri_bp_state = BP_STATE_IDLE;
T_EVENT cur_event;

T_BT_DEVICE_MODE radio_mode = BT_DEVICE_MODE_IDLE;

typedef enum
{
    APP_TIMER_SHUTDOWN_STEP                   = 0x00,
    APP_TIMER_PAIRING_MODE                    = 0x03,
    APP_TIMER_DISCOVERABLE                    = 0x04,
    APP_TIMER_ROLE_SWITCH                     = 0x05,
    APP_TIMER_LINKBACK                        = 0x07,
    APP_TIMER_LINKBACK_DELAY                  = 0x09,
    APP_TIMER_LATER_AVRCP                     = 0x0a,
    APP_TIMER_LATER_HID                       = 0x0d,
    APP_TIMER_RECONNECT                       = 0x0e,
    APP_TIMER_SRC_CONN_IND                    = 0x10,
    APP_TIMER_B2S_CONN                        = 0x11,
    APP_TIMER_LATER_A2DP                      = 0x12,
    APP_TIMER_LINKBACK_POWER_ON               = 0x13,
} T_APP_BT_POLICY_TIMER;

typedef struct
{
    uint8_t timer_idx;
    uint8_t bd_addr[6];
} T_SRC_CONN_IND;

T_B2S_CONNECTED b2s_connected = {0};
bool first_connect_sync_default_volume_to_src = false;

static bool dedicated_enter_pairing_mode_flag = false;
static bool is_visiable_flag = false;
static bool is_force_flag = false;
static bool startup_linkback_done_flag = false;
static bool is_pairing_timeout = false;
static bool bond_list_load_flag = false;
static bool linkback_flag = false;
static bool is_user_action_pairing_mode = false;
static uint16_t last_src_conn_idx = 0;
static bool disconnect_for_pairing_mode = false;
static uint16_t linkback_retry_timeout = 0;
static bool after_stop_sdp_todo_linkback_run_flag = false;
static bool discoverable_when_one_link = false;
static bool is_src_authed = false;
static bool roleswap_suc_flag = false;

static uint8_t bt_policy_timer_id = 0;
static uint8_t timer_idx_shutdown_step = 0;
static uint8_t timer_idx_pairing_mode = 0;
static uint8_t timer_idx_discoverable = 0;
static uint8_t timer_idx_linkback = 0;
static uint8_t timer_idx_linkback_delay = 0;
static uint8_t timer_idx_reconnect = 0;
static uint8_t timer_idx_b2s_conn = 0;
static uint8_t timer_idx_role_switch[MAX_BR_LINK_NUM] = {0};
static uint8_t timer_idx_later_avrcp[MAX_BR_LINK_NUM] = {0};
static uint8_t timer_idx_check_role_switch[MAX_BR_LINK_NUM] = {0};
static uint8_t timer_idx_later_hid[MAX_BR_LINK_NUM] = {0};
static uint8_t timer_idx_later_a2dp = 0;
static uint8_t timer_idx_linkback_power_on = 0;

static T_SRC_CONN_IND src_conn_ind[SRC_CONN_IND_MAX_NUM];

static T_BP_IND_FUN ind_fun = NULL;

static T_SHUTDOWN_STEP shutdown_step;
static uint8_t shutdown_step_retry_cnt;

static uint8_t old_peer_factory_addr[6];
static bool htpoll_set = false;
static uint8_t later_a2dp_addr[6];

static void linkback_sched(void);
static void stable_sched(T_STABLE_ENTER_MODE mode);
static void disconnect_all_event_handle(void);
static bool judge_dedicated_enter_pairing_mode(void);
static void prepare_for_dedicated_enter_pairing_mode(void);
static void timer_cback(uint8_t timer_evt, uint16_t param);
static void stop_all_active_action(void);
bool authentic_fail = false;
bool is_later_a2dp_connected = false;

static void connected(void)
{
    uint32_t i;

    ENGAGE_PRINT_TRACE2("connected: bud_role %d, b2s %d",
                        app_cfg_nv.bud_role, b2s_connected.num);

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_link_check_b2s_link_by_id(i))
        {
            ENGAGE_PRINT_TRACE2("connected: b2s, bd_addr %s, profs 0x%08x",
                                TRACE_BDADDR(app_db.br_link[i].bd_addr),
                                app_db.br_link[i].connected_profile);
        }
    }
}

static void new_state(T_STATE state)
{
    ENGAGE_PRINT_TRACE1("new_state: state 0x%02x", state);
}

static void event_info(T_EVENT event)
{
    ENGAGE_PRINT_TRACE1("event_info: event 0x%02x", event);
}

static void new_radio_mode(T_BT_DEVICE_MODE mode)
{
    ENGAGE_PRINT_TRACE1("new_radio_mode: radio_mode 0x%02x", mode);
}

static void stable_enter_mode(uint8_t mode)
{
    ENGAGE_PRINT_TRACE1("stable_enter_mode: mode 0x%02x", mode);
}

static void shutdown_step_info(uint8_t step)
{
    ENGAGE_PRINT_TRACE1("shutdown_step_state: step 0x%02x", step);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void app_bt_policy_b2s_conn_start_timer(void)
{
    app_start_timer(&timer_idx_b2s_conn, "b2s_conn",
                    bt_policy_timer_id, APP_TIMER_B2S_CONN, 0, false,
                    APP_BT_POLICY_B2S_CONN_TIMER_MS);
}

uint32_t get_profs_by_bond_flag(uint32_t bond_flag)
{
    uint32_t profs = 0;

    if ((T_LINKBACK_SCENARIO)app_bt_policy_cfg.link_scenario == LINKBACK_SCENARIO_HFP_BASE)
    {
        if (bond_flag & BOND_FLAG_HFP)
        {
            profs |= HFP_PROFILE_MASK;

            if (app_cfg_const.supported_profile_mask & PBAP_PROFILE_MASK)
            {
                profs |= PBAP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_HSP)
        {
            profs |= HSP_PROFILE_MASK;

            if (app_cfg_const.supported_profile_mask & PBAP_PROFILE_MASK)
            {
                profs |= PBAP_PROFILE_MASK;
            }
        }

        if (bond_flag & BOND_FLAG_SPP)
        {
            if (app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK)
            {
                profs |= SPP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_IAP)
        {
            if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK)
            {
                profs |= IAP_PROFILE_MASK;
            }
        }

        if (bond_flag & (BOND_FLAG_GATT))
        {
            if (app_cfg_const.supported_profile_mask & GATT_PROFILE_MASK)
            {
                profs |= GATT_PROFILE_MASK;
            }
        }
    }
    else if ((T_LINKBACK_SCENARIO)app_bt_policy_cfg.link_scenario == LINKBACK_SCENARIO_A2DP_BASE)
    {
        if (bond_flag & (BOND_FLAG_A2DP))
        {
            profs |= A2DP_PROFILE_MASK;
            profs |= AVRCP_PROFILE_MASK;
        }

#if F_APP_BT_HID_DEVICE_SUPPORT
        if (bond_flag & (BOND_FLAG_HID))
        {
            if (app_cfg_const.supported_profile_mask & HID_PROFILE_MASK)
            {
                profs |= HID_PROFILE_MASK;
            }
        }
#endif

        if (bond_flag & (BOND_FLAG_GATT))
        {
            if (app_cfg_const.supported_profile_mask & GATT_PROFILE_MASK)
            {
                profs |= GATT_PROFILE_MASK;
            }
        }

    }
    else if ((T_LINKBACK_SCENARIO)app_bt_policy_cfg.link_scenario ==
             LINKBACK_SCENARIO_HF_A2DP_LAST_DEVICE)
    {
        if (bond_flag & BOND_FLAG_HFP)
        {
            profs |= HFP_PROFILE_MASK;

            if (app_cfg_const.supported_profile_mask & PBAP_PROFILE_MASK)
            {
                profs |= PBAP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_HSP)
        {
            profs |= HSP_PROFILE_MASK;

            if (app_cfg_const.supported_profile_mask & PBAP_PROFILE_MASK)
            {
                profs |= PBAP_PROFILE_MASK;
            }
        }

        if (bond_flag & (BOND_FLAG_A2DP))
        {
            profs |= A2DP_PROFILE_MASK;
            profs |= AVRCP_PROFILE_MASK;
        }

#if F_APP_BT_HID_DEVICE_SUPPORT
        if (bond_flag & (BOND_FLAG_HID))
        {
            if (app_cfg_const.supported_profile_mask & HID_PROFILE_MASK)
            {
                profs |= HID_PROFILE_MASK;
            }
        }
#endif

        if (bond_flag & BOND_FLAG_SPP)
        {
            if (app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK)
            {
                profs |= SPP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_IAP)
        {
            if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK)
            {
                profs |= IAP_PROFILE_MASK;
            }
        }

        if (bond_flag & (BOND_FLAG_GATT))
        {
            if (app_cfg_const.supported_profile_mask & GATT_PROFILE_MASK)
            {
                profs |= GATT_PROFILE_MASK;
            }
        }

    }
    else if ((T_LINKBACK_SCENARIO)app_bt_policy_cfg.link_scenario == LINKBACK_SCENARIO_SPP_BASE)
    {
        if (bond_flag & BOND_FLAG_SPP)
        {
            if (app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK)
            {
                profs |= SPP_PROFILE_MASK;
            }
        }
        else if (bond_flag & BOND_FLAG_IAP)
        {
            if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK)
            {
                profs |= IAP_PROFILE_MASK;
            }
        }
    }
#if F_APP_BT_HID_DEVICE_SUPPORT
    else if ((T_LINKBACK_SCENARIO)app_bt_policy_cfg.link_scenario == LINKBACK_SCENARIO_HID_BASE)
    {
        if (bond_flag & (BOND_FLAG_HID))
        {
            if (app_cfg_const.supported_profile_mask & HID_PROFILE_MASK)
            {
                profs |= HID_PROFILE_MASK;
            }
        }
    }
#endif

    return profs;
}

static void set_bd_addr(void)
{
    ENGAGE_PRINT_TRACE2("set_bd_addr: role %d, local_addr %s",
                        app_cfg_nv.bud_role,
                        TRACE_BDADDR(app_cfg_nv.bud_local_addr));
#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE == 0)
    gap_set_bd_addr(app_cfg_nv.bud_local_addr);

    remote_local_addr_set(app_cfg_nv.bud_local_addr);
    remote_session_role_set((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);

#if F_APP_INTEGRATED_TRANSCEIVER
    gap_br_vendor_data_rate_set(0); //link for 2M/3M
#else
    gap_br_vendor_data_rate_set(1); //link for 2M
#endif
#endif
}

#if 0
static void modify_white_list(void)
{
    uint8_t wl_addr_type = GAP_REMOTE_ADDR_LE_PUBLIC;

    app_ble_gap_modify_white_list(GAP_WHITE_LIST_OP_ADD, app_cfg_nv.bud_local_addr,
                                  (T_GAP_REMOTE_ADDR_TYPE)wl_addr_type);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void src_conn_ind_init(void)
{
    memset(src_conn_ind, 0, sizeof(src_conn_ind));
}

static void src_conn_ind_add(uint8_t *bd_addr, bool is_update)
{
    uint16_t index;

    for (index = 0; index < SRC_CONN_IND_MAX_NUM; index++)
    {
        if (src_conn_ind[index].timer_idx != 0)
        {
            if (!memcmp(src_conn_ind[index].bd_addr, bd_addr, 6))
            {
                break;
            }
        }
    }

    if (!is_update)
    {
        if (index == SRC_CONN_IND_MAX_NUM)
        {
            for (index = 0; index < SRC_CONN_IND_MAX_NUM; index++)
            {
                if (src_conn_ind[index].timer_idx == 0)
                {
                    memcpy(src_conn_ind[index].bd_addr, bd_addr, 6);
                    break;
                }
            }
        }
    }

    if (index < SRC_CONN_IND_MAX_NUM)
    {
        app_stop_timer(&src_conn_ind[index].timer_idx);
        app_start_timer(&src_conn_ind[index].timer_idx, "src_conn_ind",
                        bt_policy_timer_id, APP_TIMER_SRC_CONN_IND, index, false,
                        SRC_CONN_IND_DELAY_MS);
    }
}

static void src_conn_ind_del(uint16_t index)
{
    memset(&src_conn_ind[index], 0, sizeof(T_SRC_CONN_IND));
}

bool src_conn_ind_check(uint8_t *bd_addr)
{
    uint16_t index;
    bool found = false;

    for (index = 0; index < SRC_CONN_IND_MAX_NUM; index++)
    {
        if (src_conn_ind[index].timer_idx != 0)
        {
            if (!memcmp(src_conn_ind[index].bd_addr, bd_addr, 6))
            {
                ENGAGE_PRINT_TRACE1("src_conn_ind_check: found index %d", index);
                found = true;
                break;
            }
        }
    }

    return found;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void b2s_connected_init(void)
{
    b2s_connected.num = 0;
    if (app_bt_policy_cfg_nv.enable_multi_link)
    {
        b2s_connected.num_max = app_bt_policy_cfg.max_legacy_multilink_devices;
    }
    else
    {
        b2s_connected.num_max = 1;
    }
}

uint8_t b2s_connected_get_num_max(void)
{
    return b2s_connected.num_max;
}

static uint8_t b2s_connected_is_empty(void)
{
    if (b2s_connected.num == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_is_full(void)
{
    if (b2s_connected.num >= b2s_connected.num_max)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_is_over(void)
{
    if (b2s_connected.num > b2s_connected.num_max)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_add_node(uint8_t *bd_addr, uint8_t *id)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = app_link_alloc_br_link(bd_addr);
        if (p_link != NULL)
        {
            b2s_connected.num++;
        }
    }
    if (p_link != NULL)
    {
        *id = p_link->id;
        return true;
    }
    else
    {
        return false;
    }
}

static void b2s_connected_add_prof(uint8_t *bd_addr, uint32_t prof)
{
    T_APP_BR_LINK *p_link;
    bool sync_flag = true;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile |= prof;

        switch (prof)
        {
        case A2DP_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_A2DP);
            break;

        case HFP_PROFILE_MASK:
            {
                bt_bond_flag_remove(bd_addr, BOND_FLAG_HSP);
                bt_bond_flag_add(bd_addr, BOND_FLAG_HFP);
            }
            break;

        case HSP_PROFILE_MASK:
            {
                bt_bond_flag_remove(bd_addr, BOND_FLAG_HFP);
                bt_bond_flag_add(bd_addr, BOND_FLAG_HSP);
            }
            break;

        case SPP_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_SPP);
            break;

        case PBAP_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_PBAP);
            break;

#if (F_APP_BT_HID_DEVICE_SUPPORT | F_APP_MULTI_2EP_1_BR_1_BLE_HID)
        case HID_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_HID);
            break;
#endif

        case IAP_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_IAP);
            break;

        case GATT_PROFILE_MASK:
            bt_bond_flag_add(bd_addr, BOND_FLAG_GATT);
            break;

        default:
            sync_flag = false;
            break;
        }

        if (sync_flag == true)
        {

        }
    }
}

static void b2s_connected_del_prof(uint8_t *bd_addr, uint32_t prof)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile &= ~prof;
    }
}

static bool b2s_connected_del_node(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        app_stop_timer(&timer_idx_role_switch[p_link->id]);
        app_stop_timer(&timer_idx_later_avrcp[p_link->id]);
        app_stop_timer(&timer_idx_check_role_switch[p_link->id]);
        app_stop_timer(&timer_idx_later_hid[p_link->id]);
        app_link_free_br_link(p_link);


        if (b2s_connected.num > 0)
        {
            b2s_connected.num--;
        }
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_find_node(uint8_t *bd_addr, uint32_t *profs)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (profs != NULL)
        {
            *profs = p_link->connected_profile;
        }
        return true;
    }
    else
    {
        return false;
    }
}

static bool b2s_connected_no_profs(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;
    bool ret = true;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile != 0)
        {
            ret = false;
        }
    }

    return ret;
}

void b2s_connected_set_last_conn_index(uint8_t conn_idx)
{
    last_src_conn_idx = conn_idx;
}

uint8_t b2s_connected_get_last_conn_index(void)
{
    return last_src_conn_idx;
}

static void b2s_connected_mark_index(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        ++last_src_conn_idx;

        if (b2s_connected.num > 1)
        {
            p_link->src_conn_idx = last_src_conn_idx;
        }
        else if (b2s_connected.num == 1)
        {
            p_link->src_conn_idx = last_src_conn_idx - 1;
        }
    }
}

static bool b2s_connected_is_first_src(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;
    bool ret = true;

    if (b2s_connected.num_max >= 2)
    {
        p_link = app_link_find_br_link(bd_addr);
        if (p_link != NULL)
        {
            if (p_link->src_conn_idx >= last_src_conn_idx)
            {
                ret = false;
            }
        }
    }

    return ret;
}

static void connected_node_auth_suc(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->acl.authed = true;
    }
}

static bool connected_node_is_authed(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;
    bool ret = false;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        ret = p_link->acl.authed;
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static void event_ind(T_BP_EVENT event, T_BP_EVENT_PARAM *event_param)
{
    if (NULL == ind_fun)
    {
        return;
    }

    if (cur_state >= STATE_SHUTDOWN_STEP)
    {
        switch (event)
        {
        case BP_EVENT_STATE_CHANGED:
        case BP_EVENT_RADIO_MODE_CHANGED:
        case BP_EVENT_PROFILE_DISCONN:
        case BP_EVENT_SRC_DISCONN_LOST:
        case BP_EVENT_SRC_DISCONN_NORMAL:
            ind_fun(event, event_param);
            break;

        default:
            ENGAGE_PRINT_TRACE0("event_ind: cur_state >= STATE_SHUTDOWN_STEP, not ind");
            break;
        }
    }
    else
    {
        ind_fun(event, event_param);
    }
}

static void enter_state(T_STATE state, T_BT_DEVICE_MODE mode)
{
    bool state_changed = false;
    T_BP_EVENT_PARAM event_param;
    uint8_t to;

    if (state != STATE_AFE_LINKBACK)
    {
        app_stop_timer(&timer_idx_linkback_delay);
    }

    if (cur_state != state || roleswap_suc_flag)
    {
        roleswap_suc_flag = false;

        new_state(state);

        cur_state = state;
        state_changed = true;
    }

    if (STATE_AFE_LINKBACK == cur_state)
    {
        if (!app_bt_policy_cfg.enable_not_discoverable_when_linkback)
        {
            mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
        }
    }

    if (STATE_AFE_STANDBY == cur_state)
    {
        if (app_bt_policy_cfg.enable_discoverable_in_standby_mode)
        {
            mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
        }
    }

    if (b2s_connected.num == 1)
    {
        if (app_bt_policy_cfg_nv.enable_multi_link && app_bt_policy_cfg.enable_always_discoverable)
        {
            mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
        }
    }

    if (STATE_AFE_CONNECTED == cur_state ||
        STATE_AFE_PAIRING_MODE == cur_state ||
        STATE_AFE_STANDBY == cur_state)
    {
        if (discoverable_when_one_link)
        {
            discoverable_when_one_link = false;

            if (mode != BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE &&
                app_hfp_get_call_status() == APP_HFP_CALL_IDLE)
            {
                mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;

                app_stop_timer(&timer_idx_discoverable);

                app_start_timer(&timer_idx_discoverable, "discoverable",
                                bt_policy_timer_id, APP_TIMER_DISCOVERABLE, 0, false,
                                app_cfg_const.timer_pairing_while_one_conn_timeout * 1000);
            }
        }

        if ((app_hfp_get_call_status() != APP_HFP_CALL_IDLE) ||
            b2s_connected_is_full())
        {
            app_stop_timer(&timer_idx_discoverable);
        }
    }
    else
    {
        app_stop_timer(&timer_idx_discoverable);
    }

    if (timer_idx_discoverable != 0)
    {
        mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
    }

    if (radio_mode != mode)
    {
        new_radio_mode(mode);
        radio_mode = mode;

        bt_device_mode_set(mode);

        event_ind(BP_EVENT_RADIO_MODE_CHANGED, NULL);
    }

    if (is_user_action_pairing_mode)
    {
        is_user_action_pairing_mode = false;
        state_changed = true;
    }

    if (state_changed)
    {
        switch (cur_state)
        {
        case STATE_STARTUP:
        case STATE_SHUTDOWN:
            bp_state = BP_STATE_IDLE;
            break;

        case STATE_AFE_LINKBACK:
            bp_state = BP_STATE_LINKBACK;
            app_bt_policy_qos_param_update(app_db.br_link[app_a2dp_get_active_idx()].bd_addr,
                                           BP_TPOLL_LINKBACK_START);
            break;

        case STATE_AFE_CONNECTED:
            bp_state = BP_STATE_CONNECTED;
            break;

        case STATE_AFE_PAIRING_MODE:
            bp_state = BP_STATE_PAIRING_MODE;
            break;

        case STATE_AFE_STANDBY:
            bp_state = BP_STATE_STANDBY;
            break;

        case STATE_DUT_TEST_MODE:
            bp_state = BP_STATE_CONNECTED;
            break;

        case STATE_SHUTDOWN_STEP:
        case STATE_STOP:
            bp_state = BP_STATE_IDLE;
            break;

        default:
            break;
        }

        if (app_bt_policy_cfg.timer_pairing_timeout != 0)
        {
            app_stop_timer(&timer_idx_pairing_mode);
            if (STATE_AFE_PAIRING_MODE == cur_state)
            {
                app_start_timer(&timer_idx_pairing_mode, "pairing_mode",
                                bt_policy_timer_id, APP_TIMER_PAIRING_MODE, 0, false,
                                app_bt_policy_cfg.timer_pairing_timeout * 1000);
            }
        }

        event_param.is_shut_down = false;
        if (STATE_SHUTDOWN == cur_state)
        {
            event_param.is_shut_down = true;
        }

        event_param.is_ignore = true;
        if (STATE_AFE_PAIRING_MODE == cur_state)
        {
            event_param.is_ignore = false;
        }

        event_ind(BP_EVENT_STATE_CHANGED, &event_param);

        if (STATE_AFE_LINKBACK == cur_state)
        {
            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_LINKBACK);

            linkback_run();
        }
        else
        {
            app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_LINKBACK);
        }
    }
}

static void linkback_sched(void)
{
    ENGAGE_PRINT_TRACE3("linkback_sched: enable_power_on_linkback %d, startup_linkback_done_flag %d, bond_list_load_flag %d",
                        app_cfg_const.enable_power_on_linkback,
                        startup_linkback_done_flag,
                        bond_list_load_flag);

    linkback_flag = false;

    if (dedicated_enter_pairing_mode_flag)
    {
        dedicated_enter_pairing_mode_flag = false;

        if (judge_dedicated_enter_pairing_mode())
        {
            prepare_for_dedicated_enter_pairing_mode();
            return;
        }
    }

    if (app_cfg_const.enable_power_on_linkback)
    {
        if (!startup_linkback_done_flag)
        {
            if (!bond_list_load_flag)
            {
                bond_list_load_flag = true;
                ENGAGE_PRINT_TRACE1("linkback_sched: linkback_retry_timeout %d , not use", linkback_retry_timeout);
                linkback_load_bond_list(0, linkback_retry_timeout);
            }
        }
    }

    if (0 != linkback_todo_queue_node_num() || linkback_active_node.is_valid)
    {
        if (app_cfg_const.enable_power_on_linkback)
        {
            enter_state(STATE_AFE_LINKBACK, BT_DEVICE_MODE_CONNECTABLE);
        }
    }
    else
    {
        stable_sched(STABLE_ENTER_MODE_NORMAL);
    }
}

void linkback_run(void)
{
    T_LINKBACK_NODE node;
    uint32_t profs;

    ENGAGE_PRINT_TRACE0("linkback_run: start");

RETRY:
    if (!linkback_active_node.is_valid)
    {
        if (linkback_todo_queue_take_first_node(&node))
        {
            linkback_active_node_load(&node);

            if (linkback_active_node.is_valid)
            {
                linkback_active_node.linkback_node.gtiming = os_sys_time_get();

                app_stop_timer(&timer_idx_linkback);
                app_start_timer(&timer_idx_linkback, "linkback",
                                bt_policy_timer_id, APP_TIMER_LINKBACK, 0, false,
                                linkback_active_node.retry_timeout);
            }
        }
    }

    if (!linkback_active_node.is_valid)
    {
        ENGAGE_PRINT_TRACE0("linkback_run: have no valid node, finish");

        stable_sched(STABLE_ENTER_MODE_NORMAL);

        goto EXIT;
    }

    if (linkback_active_node.is_exit)
    {
        linkback_active_node.is_valid = false;

        goto RETRY;
    }

    if (0 == linkback_active_node.remain_profs)
    {
        linkback_active_node.is_valid = false;

        goto RETRY;
    }

    if (src_conn_ind_check(linkback_active_node.linkback_node.bd_addr))
    {
        app_stop_timer(&timer_idx_linkback_delay);
        app_start_timer(&timer_idx_linkback_delay, "linkback_delay",
                        bt_policy_timer_id, APP_TIMER_LINKBACK_DELAY, 0, false,
                        500);

        goto EXIT;
    }

    if (b2s_connected_find_node(linkback_active_node.linkback_node.bd_addr, &profs))
    {
        if (profs & linkback_active_node.doing_prof)
        {
            ENGAGE_PRINT_TRACE1("linkback_run: prof 0x%08x, already connected",
                                linkback_active_node.doing_prof);

            linkback_active_node_step_suc_adjust_remain_profs();

            goto RETRY;
        }
        else
        {
            goto LINKBACK;
        }
    }
    else
    {
        if (linkback_active_node.linkback_node.is_force)
        {
            goto LINKBACK;
        }
        else
        {
            if (b2s_connected_is_full())
            {
                ENGAGE_PRINT_TRACE0("linkback_run: b2s is full, abort this node");

                linkback_active_node.is_valid = false;

                goto RETRY;
            }
            else
            {
                goto LINKBACK;
            }
        }
    }

LINKBACK:
    linkback_flag = true;

    if (linkback_active_node.linkback_node.check_bond_flag)
    {
        if (!linkback_check_bond_flag(linkback_active_node.linkback_node.bd_addr,
                                      linkback_active_node.doing_prof))
        {
            linkback_active_node_step_fail_adjust_remain_profs();

            goto RETRY;
        }
    }

    if (linkback_profile_is_need_search(linkback_active_node.doing_prof))
    {
        app_hfp_adjust_sco_window(linkback_active_node.linkback_node.bd_addr, APP_SCO_ADJUST_LINKBACK_EVT);

        if (!linkback_profile_search_start(linkback_active_node.linkback_node.bd_addr,
                                           linkback_active_node.doing_prof, linkback_active_node.linkback_node.is_special))
        {
            linkback_active_node_step_fail_adjust_remain_profs();

            goto RETRY;
        }
        else
        {
            ENGAGE_PRINT_TRACE0("linkback_run: wait search result");

            goto EXIT;
        }
    }
    else
    {
        linkback_active_node.is_sdp_ok = true;

        app_hfp_adjust_sco_window(linkback_active_node.linkback_node.bd_addr, APP_SCO_ADJUST_LINKBACK_EVT);

        if (linkback_profile_connect_start(linkback_active_node.linkback_node.bd_addr,
                                           linkback_active_node.doing_prof, &linkback_active_node.linkback_conn_param))
        {
            goto EXIT;
        }
        else
        {
            linkback_active_node_step_fail_adjust_remain_profs();

            goto RETRY;
        }
    }

EXIT:
    return;
}

#if 0
static void roleswitch_fail_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->link_role_switch_count++;

        if (p_link->link_role_switch_count < ROLE_SWITCH_COUNT_MAX)
        {
            app_start_timer(&timer_idx_role_switch[p_link->id], "role_switch",
                            bt_policy_timer_id, APP_TIMER_ROLE_SWITCH, p_link->id, false,
                            ROLE_SWITCH_DELAY_MS);
        }
    }
}
#endif

static void role_master_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->acl.role = BT_LINK_ROLE_MASTER;
    }
}

static void role_slave_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->acl.role = BT_LINK_ROLE_SLAVE;
    }
}


static void conn_sniff_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->acl.in_sniffmode = true;
    }
}

static void conn_active_event_handle(T_BT_PARAM *param)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(param->bd_addr);
    if (p_link != NULL)
    {
        p_link->acl.in_sniffmode = false;
    }
}

void app_stop_reconnect_timer()
{
    APP_PRINT_TRACE1("app_stop_reconnect_timer: connected_num_before_roleswap %d",
                     app_db.connected_num_before_roleswap);
    app_stop_timer(&timer_idx_reconnect);
}

static void startup_event_handle(T_STARTUP_PARAM *param)
{
    if (NULL == param)
    {
        return;
    }


    ind_fun = param->ind_fun;

    radio_mode = BT_DEVICE_MODE_IDLE;

    bp_state = BP_STATE_IDLE;
    pri_bp_state = BP_STATE_IDLE;

    dedicated_enter_pairing_mode_flag = false;
    startup_linkback_done_flag = false;
    bond_list_load_flag = false;
    linkback_flag = false;
    is_user_action_pairing_mode = false;
    last_src_conn_idx = 0;
    disconnect_for_pairing_mode = false;
    linkback_retry_timeout = app_bt_policy_cfg.timer_linkback_timeout;
    after_stop_sdp_todo_linkback_run_flag = false;
    discoverable_when_one_link = false;

    timer_idx_shutdown_step = 0;
    timer_idx_pairing_mode = 0;
    timer_idx_discoverable = 0;
    timer_idx_linkback = 0;
    timer_idx_linkback_delay = 0;
    memset(timer_idx_role_switch, 0, MAX_BR_LINK_NUM);
    memset(timer_idx_later_avrcp, 0, MAX_BR_LINK_NUM);
    memset(timer_idx_check_role_switch, 0, MAX_BR_LINK_NUM);
    memset(timer_idx_later_hid, 0, MAX_BR_LINK_NUM);

    src_conn_ind_init();

    enter_state(STATE_STARTUP, BT_DEVICE_MODE_IDLE);

    memcpy(app_cfg_nv.bud_local_addr, app_db.factory_addr, 6);

    app_cfg_nv.bud_role = app_cfg_const.bud_role;
    set_bd_addr();

    b2s_connected_init();

    linkback_todo_queue_init();
    linkback_active_node_init();
}

void enter_dut_test_mode_event_handle(void)
{
    enter_state(STATE_DUT_TEST_MODE, BT_DEVICE_MODE_IDLE);
}

static void shutdown_step_handle(void)
{
    uint32_t i;

    shutdown_step_info(shutdown_step);

    switch (shutdown_step)
    {
    case SHUTDOWN_STEP_START_DISCONN_B2S_PROFILE:
        {
            for (i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_link_check_b2s_link_by_id(i))
                {
                    app_db.br_link[i].acl.disconnected = false;
                    linkback_profile_disconnect_start(app_db.br_link[i].bd_addr, app_db.br_link[i].connected_profile);
                }
            }
            shutdown_step = SHUTDOWN_STEP_WAIT_DISCONN_B2S_PROFILE;
            shutdown_step_retry_cnt = 0;
        }
        break;

    case SHUTDOWN_STEP_WAIT_DISCONN_B2S_PROFILE:
        {
            if (++shutdown_step_retry_cnt >= DISCONN_B2S_PROFILE_WAIT_TIMES)
            {
                shutdown_step = SHUTDOWN_STEP_START_DISCONN_B2S_LINK;
            }
        }
        break;

    case SHUTDOWN_STEP_START_DISCONN_B2S_LINK:
        {
            for (i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_link_check_b2s_link_by_id(i))
                {
                    if (app_db.br_link[i].sco.conn_handle)
                    {
                        gap_br_send_sco_disconn_req(app_db.br_link[i].bd_addr);
                    }
                    gap_br_send_acl_disconn_req(app_db.br_link[i].bd_addr);
                }
            }
            shutdown_step = SHUTDOWN_STEP_WAIT_DISCONN_B2S_LINK;
            shutdown_step_retry_cnt = 0;
        }
        break;

    case SHUTDOWN_STEP_WAIT_DISCONN_B2S_LINK:
        {
            ++shutdown_step_retry_cnt;
            if ((0 == b2s_connected.num) || shutdown_step_retry_cnt >= DISCONN_B2S_LINK_WAIT_TIMES_MAX)
            {
                shutdown_step = SHUTDOWN_STEP_END;
            }
        }
        break;

    default:
        break;
    }

    if (SHUTDOWN_STEP_END == shutdown_step)
    {
        enter_state(STATE_SHUTDOWN, BT_DEVICE_MODE_IDLE);
    }
    else
    {
        app_start_timer(&timer_idx_shutdown_step, "shutdown_step",
                        bt_policy_timer_id, APP_TIMER_SHUTDOWN_STEP, 0, false,
                        SHUT_DOWN_STEP_TIMER_MS);
    }
}

static void stop_all_active_action(void)
{
    uint8_t *bd_addr;

    app_stop_timer(&timer_idx_pairing_mode);
    app_stop_timer(&timer_idx_discoverable);
    app_stop_timer(&timer_idx_linkback);
    app_stop_timer(&timer_idx_linkback_delay);

    if (linkback_active_node.is_valid)
    {
        bd_addr = linkback_active_node.linkback_node.bd_addr;

        after_stop_sdp_todo_linkback_run_flag = false;
        gap_br_stop_sdp_discov(bd_addr);
        if (!b2s_connected_find_node(bd_addr, NULL))
        {
            gap_br_send_acl_disconn_req(bd_addr);
        }

        linkback_active_node_use_left_time_insert_to_queue_again(false);

        linkback_active_node.is_valid = false;
    }
}

static void shutdown_event_handle(void)
{
    stop_all_active_action();

    enter_state(STATE_SHUTDOWN_STEP, BT_DEVICE_MODE_IDLE);

    if (b2s_connected.num == 0)
    {
        shutdown_step = SHUTDOWN_STEP_END;
    }
    else
    {
        shutdown_step = SHUTDOWN_STEP_START_DISCONN_B2S_PROFILE;
    }
    shutdown_step_handle();
}

static void stop_event_handle(void)
{
    stop_all_active_action();

    enter_state(STATE_STOP, BT_DEVICE_MODE_IDLE);
}

static void restore_event_handle(void)
{
    if (cur_state == STATE_STOP)
    {
        startup_linkback_done_flag = false;
        bond_list_load_flag = false;

        linkback_todo_queue_init();
        linkback_active_node_init();

        if (app_cfg_const.enable_power_on_linkback)
        {
            app_start_timer(&timer_idx_linkback_power_on, "linkback_power_on",
                            bt_policy_timer_id, APP_TIMER_LINKBACK_POWER_ON, 0, false, 1000);
        }
    }
}

static void state_shutdown_step_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_SHUTDOWN_STEP_TIMEOUT:
        {
            shutdown_step_handle();
        }
        break;

    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void state_afe_linkback_event_handle(T_EVENT event, T_BT_PARAM *param)
{
    uint8_t *bd_addr;

    if (!param->not_check_addr_flag)
    {
        if (!linkback_active_node_judge_cur_conn_addr(param->bd_addr))
        {
            return;
        }
    }

    switch (event)
    {
    case EVENT_SRC_CONN_FAIL:
        {
            if (after_stop_sdp_todo_linkback_run_flag)
            {
                after_stop_sdp_todo_linkback_run_flag = false;
                linkback_active_node.is_valid = false;
            }

            if (0 == linkback_todo_queue_node_num())
            {
                linkback_active_node_src_conn_fail_adjust_remain_profs();

                linkback_run();
            }
            else
            {
                app_stop_timer(&timer_idx_linkback);

                linkback_active_node_use_left_time_insert_to_queue_again(true);

                linkback_active_node.is_valid = false;

                linkback_run();
            }
        }
        break;

    case EVENT_SRC_CONN_FAIL_ACL_EXIST:
        {
            app_stop_timer(&timer_idx_linkback_delay);
            app_start_timer(&timer_idx_linkback_delay, "linkback_delay",
                            bt_policy_timer_id, APP_TIMER_LINKBACK_DELAY, event, false,
                            1300);
        }
        break;

    case EVENT_SRC_CONN_FAIL_CONTROLLER_BUSY:
        {
            app_stop_timer(&timer_idx_linkback_delay);
            app_start_timer(&timer_idx_linkback_delay, "linkback_delay",
                            bt_policy_timer_id, APP_TIMER_LINKBACK_DELAY, event, false,
                            500);
        }
        break;

    case EVENT_LINKBACK_DELAY_TIMEOUT:
        {
            linkback_active_node_src_conn_fail_adjust_remain_profs();

            linkback_run();
        }
        break;

    case EVENT_SRC_AUTH_SUC:
        {
            if (!linkback_active_node_judge_cur_conn_addr(param->bd_addr))
            {
                if (b2s_connected_is_full())
                {
                    if (linkback_active_node.is_valid)
                    {
                        bd_addr = linkback_active_node.linkback_node.bd_addr;

                        if (!b2s_connected_find_node(bd_addr, NULL))
                        {
                            ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: bring forward the linkback timout");

                            timer_cback(APP_TIMER_LINKBACK, 0);
                        }
                    }
                }
            }
        }
        break;

    case EVENT_SRC_AUTH_FAIL:
        {
            ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: wait EVENT_PROFILE_CONN_FAIL");
        }
        break;

    case EVENT_SRC_DISCONN_LOST:
    case EVENT_SRC_DISCONN_NORMAL:
    case EVENT_SRC_DISCONN_ROLESWAP:
        {
            linkback_active_node.is_exit = true;
            after_stop_sdp_todo_linkback_run_flag = false;

            linkback_run();
        }
        break;

    case EVENT_SRC_CONN_TIMEOUT:
        {
            if (linkback_active_node.is_valid)
            {
                bd_addr = linkback_active_node.linkback_node.bd_addr;

                if (!memcmp(bd_addr, app_db.resume_addr, 6))
                {
                    memset(app_db.resume_addr, 0, 6);
                    app_audio_set_connected_tone_need(false);
                }

                if (!b2s_connected_find_node(bd_addr, NULL))
                {
                    after_stop_sdp_todo_linkback_run_flag = true;
                    gap_br_stop_sdp_discov(bd_addr);
                    gap_br_send_acl_disconn_req(bd_addr);
                }
                else
                {
                    linkback_active_node.linkback_node.plan_profs = 0;
                }
            }
            else
            {
                linkback_run();
            }
        }
        break;

    case EVENT_PROFILE_SDP_ATTR_INFO:
        {
            linkback_active_node.linkback_conn_param.protocol_version = param->sdp_info->protocol_version;
            uint8_t server_channel = param->sdp_info->server_channel;

            if (SPP_PROFILE_MASK == linkback_active_node.doing_prof)
            {
                uint8_t local_server_chann;

                if (bt_spp_registered_uuid_check((T_BT_SPP_UUID_TYPE)param->sdp_info->srv_class_uuid_type,
                                                 (T_BT_SPP_UUID_DATA *)(&param->sdp_info->srv_class_uuid_data), &local_server_chann))
                {
                    bool is_sdp_ok = false;
                    if (server_channel == RFC_SPP_CHANN_NUM)
                    {
                        {
                            {
                                if (linkback_active_node.linkback_conn_param.server_channel != RFC_RTK_VENDOR_CHANN_NUM)
                                {
                                    is_sdp_ok = true;
                                }
                            }
                        }
                    }

                    else if (server_channel == RFC_RTK_VENDOR_CHANN_NUM)
                    {
                        is_sdp_ok = true;
                    }

                    if (is_sdp_ok)
                    {
                        linkback_active_node.is_sdp_ok = true;
                        linkback_active_node.linkback_conn_param.server_channel = server_channel;
                        linkback_active_node.linkback_conn_param.local_server_chann = local_server_chann;
                    }
                }
            }
            else
            {
                linkback_active_node.is_sdp_ok = true;
                linkback_active_node.linkback_conn_param.server_channel = server_channel;

                if (PBAP_PROFILE_MASK == linkback_active_node.doing_prof)
                {
                    linkback_active_node.linkback_conn_param.feature = (param->sdp_info->supported_feat) ? true : false;
                }
            }
        }
        break;

    case EVENT_PROFILE_DID_ATTR_INFO:
        {
            linkback_active_node.is_sdp_ok = true;
        }
        break;

    case EVENT_PROFILE_SDP_DISCOV_CMPL:
        {
            if ((!b2s_connected_find_node(param->bd_addr, NULL)) ||
                (param->cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) ||
                (param->cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)) ||
                (param->cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                (param->cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
            {
                ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: wait EVENT_SRC_CONN_FAIL");
            }
            else
            {
                if (linkback_active_node.is_sdp_ok)
                {
                    uint32_t profs = 0;

                    if ((DID_PROFILE_MASK == linkback_active_node.doing_prof) ||
                        (b2s_connected_find_node(param->bd_addr, &profs) && (profs & linkback_active_node.doing_prof)))
                    {
                        linkback_active_node_step_suc_adjust_remain_profs();
                        linkback_run();
                    }
                    else
                    {
                        if (linkback_profile_connect_start(linkback_active_node.linkback_node.bd_addr,
                                                           linkback_active_node.doing_prof, &linkback_active_node.linkback_conn_param))
                        {
                            if (PBAP_PROFILE_MASK == linkback_active_node.doing_prof)
                            {
                                linkback_active_node_step_suc_adjust_remain_profs();
                                linkback_run();
                            }
                        }
                        else
                        {
                            linkback_active_node_step_fail_adjust_remain_profs();
                            linkback_run();
                        }
                    }
                }
                else
                {
                    linkback_active_node_step_fail_adjust_remain_profs();
                    linkback_run();
                }
            }
        }
        break;

    case EVENT_PROFILE_SDP_DISCOV_STOP:
        {
        }
        break;

    case EVENT_PROFILE_CONN_SUC:
        {
            if (linkback_active_node_judge_cur_conn_prof(param->bd_addr, param->prof))
            {
                linkback_active_node_step_suc_adjust_remain_profs();
                linkback_run();
            }
        }
        break;

    case EVENT_PROFILE_CONN_FAIL:
        {
            ENGAGE_PRINT_TRACE1("state_afe_linkback_event_handle: EVENT_PROFILE_CONN_FAIL cause %x",
                                param->cause);
            if ((param->cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) ||
                (param->cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                (param->cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
            {
            }
            else if (param->cause == (L2C_ERR | L2C_ERR_SECURITY_BLOCK))
            {
                // ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: L2C_ERR_SECURITY_BLOCK");

                gap_br_send_acl_disconn_req(param->bd_addr);
            }
            else if (param->cause == (L2C_ERR | L2C_ERR_NO_RESOURCE))
            {
                // ENGAGE_PRINT_TRACE0("state_afe_linkback_event_handle: L2C_ERR_NO_RESOURCE");

                app_stop_timer(&timer_idx_linkback_delay);
                app_start_timer(&timer_idx_linkback_delay, "linkback_delay",
                                bt_policy_timer_id, APP_TIMER_LINKBACK_DELAY, event, false,
                                500);
            }
            else
            {
                if (linkback_active_node_judge_cur_conn_prof(param->bd_addr, param->prof))
                {
                    linkback_active_node_step_fail_adjust_remain_profs();
                    linkback_run();
                }
            }
        }
        break;

    default:
        break;
    }
}

static void state_afe_stable_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_SRC_AUTH_SUC:
        {
            stable_sched(STABLE_ENTER_MODE_AGAIN);
        }
        break;

    case EVENT_SRC_DISCONN_NORMAL:
        {
            if (is_src_authed || (cur_state == STATE_AFE_CONNECTED && b2s_connected_is_empty()))
            {
                is_src_authed = false;

                stable_sched(STABLE_ENTER_MODE_AGAIN);
            }
        }
        break;

    case EVENT_SRC_DISCONN_LOST:
        {
            enter_state(STATE_AFE_LINKBACK, BT_DEVICE_MODE_CONNECTABLE);
        }
        break;

    case EVENT_DISCOVERABLE_TIMEOUT:
        {
            stable_sched(STABLE_ENTER_MODE_AGAIN);
        }
        break;

    default:
        break;
    }
}

static void state_dut_test_mode_event_handle(T_EVENT event)
{
    switch (event)
    {
    case EVENT_SRC_CONN_SUC:
        {
            enter_state(STATE_DUT_TEST_MODE, BT_DEVICE_MODE_IDLE);
        }
        break;

    case EVENT_SRC_DISCONN_NORMAL:
    case EVENT_SRC_DISCONN_LOST:
        {
            enter_state(STATE_DUT_TEST_MODE, BT_DEVICE_MODE_CONNECTABLE);
        }
        break;

    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool judge_dedicated_enter_pairing_mode(void)
{
    bool ret = true;

    if (!is_force_flag)
    {
        if (b2s_connected_is_full())
        {
            ret = false;
        }
    }

    ENGAGE_PRINT_TRACE1("judge_dedicated_enter_pairing_mode: ret %d", ret);

    return ret;
}

static void prepare_for_dedicated_enter_pairing_mode(void)
{
    ENGAGE_PRINT_TRACE0("prepare_for_dedicated_enter_pairing_mode");

    disconnect_for_pairing_mode = false;

    if (!b2s_connected_is_full())
    {
        if (!disconnect_for_pairing_mode)
        {
            stable_sched(STABLE_ENTER_MODE_DEDICATED_PAIRING);
        }
    }
    else
    {
        if (is_force_flag)
        {
            uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

            if (((app_db.br_link[active_a2dp_idx].avrcp.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                 || (app_db.br_link[active_a2dp_idx].a2dp.is_streaming == true))
                && (app_bt_policy_cfg_nv.enable_multi_link))
            {
                for (uint8_t inactive_a2dp_idx = 0; inactive_a2dp_idx < MAX_BR_LINK_NUM; inactive_a2dp_idx++)
                {
                    if (app_link_check_b2s_link_by_id(inactive_a2dp_idx))
                    {
                        if (inactive_a2dp_idx != active_a2dp_idx)
                        {
                            app_bt_policy_disconnect(app_db.br_link[inactive_a2dp_idx].bd_addr, ALL_PROFILE_MASK);
                            disconnect_for_pairing_mode = true;
                            break;
                        }
                    }
                }
            }
            else
            {
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_link_check_b2s_link_by_id(i))
                    {
                        if (b2s_connected_is_first_src(app_db.br_link[i].bd_addr))
                        {
                            app_bt_policy_disconnect(app_db.br_link[i].bd_addr, ALL_PROFILE_MASK);
                            disconnect_for_pairing_mode = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    is_force_flag = false;
}

static void dedicated_enter_pairing_mode_event_handle(T_BT_PARAM *param)
{
    is_visiable_flag = param->is_visiable;
    is_force_flag = param->is_force;

    switch (cur_state)
    {
    case STATE_STARTUP:
        {
            stable_sched(STABLE_ENTER_MODE_DIRECT_PAIRING);
        }
        break;

    case STATE_AFE_LINKBACK:
        {
            if (judge_dedicated_enter_pairing_mode())
            {
                if (!b2s_connected_find_node(linkback_active_node.linkback_node.bd_addr, NULL))
                {
                    app_stop_timer(&timer_idx_linkback);

                    if (linkback_active_node.is_valid)
                    {
                        after_stop_sdp_todo_linkback_run_flag = false;
                        gap_br_stop_sdp_discov(linkback_active_node.linkback_node.bd_addr);
                        gap_br_send_acl_disconn_req(linkback_active_node.linkback_node.bd_addr);
                    }

                    linkback_todo_queue_init();
                    linkback_active_node_init();

                    prepare_for_dedicated_enter_pairing_mode();
                }
                else
                {
                    dedicated_enter_pairing_mode_flag = true;
                }
            }
        }
        break;

    case STATE_AFE_CONNECTED:
    case STATE_AFE_PAIRING_MODE:
    case STATE_AFE_STANDBY:
        {
            if (judge_dedicated_enter_pairing_mode())
            {
                prepare_for_dedicated_enter_pairing_mode();
            }
        }
        break;

    default:
        {
            dedicated_enter_pairing_mode_flag = true;
        }
        break;
    }
}

static void dedicated_exit_pairing_mode_event_handle(void)
{
    dedicated_enter_pairing_mode_flag = false;

    switch (cur_state)
    {
    case STATE_AFE_CONNECTED:
    case STATE_AFE_PAIRING_MODE:
    case STATE_AFE_STANDBY:
        {
            stable_sched(STABLE_ENTER_MODE_NOT_PAIRING);
        }
        break;

    default:
        break;
    }
}

static void dedicated_connect_event_handle(T_BT_PARAM *bt_param)
{
    T_APP_BR_LINK *p_link;

    switch (cur_state)
    {
    case STATE_AFE_LINKBACK:
        {
            if (!bt_param->is_special && linkback_active_node_judge_cur_conn_addr(bt_param->bd_addr))
            {
                linkback_active_node_remain_profs_add(bt_param->prof, bt_param->check_bond_flag,
                                                      app_bt_policy_cfg.timer_linkback_timeout * 1000);

                app_stop_timer(&timer_idx_linkback);
                app_start_timer(&timer_idx_linkback, "linkback",
                                bt_policy_timer_id, APP_TIMER_LINKBACK, 0, false,
                                linkback_active_node.retry_timeout);
            }
            else
            {
                linkback_todo_queue_insert_force_node(bt_param->bd_addr, bt_param->prof, bt_param->is_special,
                                                      bt_param->check_bond_flag, app_bt_policy_cfg.timer_linkback_timeout * 1000,
                                                      false);
            }
        }
        break;

    case STATE_AFE_CONNECTED:
    case STATE_AFE_PAIRING_MODE:
    case STATE_AFE_STANDBY:
        {
            if (bt_param->is_later_avrcp)
            {
                p_link = app_link_find_br_link(bt_param->bd_addr);
                if (p_link != NULL)
                {
                    if (0 == (AVRCP_PROFILE_MASK & p_link->connected_profile))
                    {
                        linkback_profile_connect_start(bt_param->bd_addr, AVRCP_PROFILE_MASK, NULL);
                    }
                }
            }
            else if (DID_PROFILE_MASK == bt_param->prof)
            {
                p_link = app_link_find_br_link(bt_param->bd_addr);
                if (p_link != NULL)
                {
                    gap_br_start_did_discov(bt_param->bd_addr);
                }
            }
            else if (bt_param->is_later_hid)
            {
                p_link = app_link_find_br_link(bt_param->bd_addr);
                if (p_link != NULL)
                {
                    if (0 == (HID_PROFILE_MASK & p_link->connected_profile))
                    {
                        linkback_profile_connect_start(bt_param->bd_addr, HID_PROFILE_MASK, NULL);
                    }
                }
            }
            else
            {
                linkback_todo_queue_insert_force_node(bt_param->bd_addr, bt_param->prof, bt_param->is_special,
                                                      bt_param->check_bond_flag, app_bt_policy_cfg.timer_linkback_timeout * 1000,
                                                      false);
            }
        }
        break;

    default:
        break;
    }
}

static void dedicated_disconnect_event_handle(T_BT_PARAM *bt_param)
{
    uint8_t *bd_addr;
    uint32_t plan_profs;
    uint32_t exist_profs;
    T_APP_BR_LINK *p_link;

    bd_addr = bt_param->bd_addr;
    plan_profs = bt_param->prof;

    if (linkback_active_node_judge_cur_conn_addr(bd_addr))
    {
        linkback_active_node_remain_profs_sub(plan_profs);
    }

    linkback_todo_queue_remove_plan_profs(bd_addr, plan_profs);

    if (b2s_connected_find_node(bd_addr, &exist_profs))
    {
        p_link = app_link_find_br_link(bd_addr);

        if (p_link != NULL)
        {
            ENGAGE_PRINT_TRACE3("dedicated_disconnect_event_handle: bd_addr %s, prof 0x%08x, exist_profs 0x%08x",
                                TRACE_BDADDR(bd_addr), plan_profs, exist_profs);
            if (exist_profs & HID_PROFILE_MASK)
            {
                plan_profs &= exist_profs;
                plan_profs |= HID_PROFILE_MASK;
            }
            else
            {
                plan_profs &= exist_profs;
            }
            if (plan_profs & (AVRCP_CONTROLLER_PROFILE_MASK | AVRCP_TARGET_PROFILE_MASK))
            {
                app_stop_timer(&timer_idx_later_avrcp[p_link->id]);
            }

            if (plan_profs & HID_PROFILE_MASK)
            {
                app_stop_timer(&timer_idx_later_hid[p_link->id]);
            }

            p_link->acl.disconnected = false;
            if (plan_profs == exist_profs)
            {
                p_link->acl.disconnected = true;
                p_link->plan_disconnect_profs = plan_profs;
            }

            linkback_profile_disconnect_start(bd_addr, plan_profs);
        }

    }
}

static void disconnect_all_event_handle(void)
{
    uint32_t i;

    ENGAGE_PRINT_TRACE0("disconnect_all_event_handle");

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_link_check_b2s_link_by_id(i))
            {
                if (app_db.br_link[i].sco.conn_handle)
                {
                    gap_br_send_sco_disconn_req(app_db.br_link[i].bd_addr);
                }
                gap_br_send_acl_disconn_req(app_db.br_link[i].bd_addr);
            }
        }
    }
}

static bool enable_b2s_disconnect_enter_pairing(void)
{
    bool ret = true;

    if (app_db.disconnect_inactive_link_actively ||
        ((!b2s_connected_is_empty()) &&
         (!app_bt_policy_cfg.enter_pairing_while_only_one_device_connected)))
    {
        ret = false;
    }

    return ret;
}


static void stable_sched(T_STABLE_ENTER_MODE mode)
{
    stable_enter_mode(mode);

    app_stop_timer(&timer_idx_linkback);

    startup_linkback_done_flag = true;

    switch (mode)
    {
    case STABLE_ENTER_MODE_NORMAL:
        {
            if (b2s_connected_is_empty())
            {
                if ((!linkback_flag || app_cfg_const.enable_power_on_linkback_fail_enter_pairing) &&
                    (app_hfp_get_call_status() == APP_HFP_CALL_IDLE))
                {
                    enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
                }
                else
                {
                    enter_state(STATE_AFE_STANDBY, BT_DEVICE_MODE_CONNECTABLE);
                }
            }
            else
            {
                if (b2s_connected_is_full())
                {
                    enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_IDLE);
                }
                else
                {
                    enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_CONNECTABLE);
                }
            }
            app_bt_policy_qos_param_update(app_db.br_link[app_a2dp_get_active_idx()].bd_addr,
                                           BP_TPOLL_LINKBACK_STOP);
            linkback_flag = false;
        }
        break;

    case STABLE_ENTER_MODE_AGAIN:
        {
            if (b2s_connected_is_empty())
            {
                if (((cur_state == STATE_AFE_PAIRING_MODE) ||
                     (cur_event == EVENT_SRC_DISCONN_NORMAL && enable_b2s_disconnect_enter_pairing())) &&
                    (app_hfp_get_call_status() == APP_HFP_CALL_IDLE))
                {
                    if (cur_event == EVENT_SRC_DISCONN_NORMAL && enable_b2s_disconnect_enter_pairing())
                    {
                        is_user_action_pairing_mode = true;
                    }
                    else
                    {
                        is_user_action_pairing_mode = false;
                    }
                    enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
                }
                else
                {
                    enter_state(STATE_AFE_STANDBY, BT_DEVICE_MODE_CONNECTABLE);
                }
            }
            else
            {
                if (((cur_state == STATE_AFE_PAIRING_MODE && cur_event != EVENT_SRC_AUTH_SUC) ||
                     (cur_event == EVENT_SRC_DISCONN_NORMAL && enable_b2s_disconnect_enter_pairing())) &&
                    (app_hfp_get_call_status() == APP_HFP_CALL_IDLE))
                {
                    if (cur_event == EVENT_SRC_DISCONN_NORMAL)
                    {
                        is_user_action_pairing_mode = true;
                    }
                    else
                    {
                        is_user_action_pairing_mode = false;
                    }
                    enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
                }
                else
                {
                    if (b2s_connected_is_full())
                    {
                        enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_IDLE);
                    }
                    else
                    {
                        enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_CONNECTABLE);
                    }
                }
                app_db.disconnect_inactive_link_actively = false;
            }
        }
        break;

    case STABLE_ENTER_MODE_DEDICATED_PAIRING:
        {
            event_ind(BP_EVENT_DEDICATED_PAIRING, NULL);

            is_user_action_pairing_mode = true;

            if (is_visiable_flag)
            {
                enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
            }
            else
            {
                enter_state(STATE_AFE_STANDBY, BT_DEVICE_MODE_CONNECTABLE);
            }
        }
        break;

    case STABLE_ENTER_MODE_NOT_PAIRING:
        {
            if ((is_pairing_timeout && app_cfg_const.enable_pairing_timeout_to_power_off) &&
                b2s_connected_is_empty()
#if F_APP_LINEIN_SUPPORT
                && !app_line_in_plug_state_get()
#endif
               )
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_db.power_off_cause = POWER_OFF_CAUSE_EXIT_PAIRING_MODE;
                    app_mmi_handle_action(MMI_DEV_POWER_OFF);
                }
                else
                {
                    uint8_t action = MMI_DEV_POWER_OFF;


                }
            }
            else
            {
                if (b2s_connected_is_empty())
                {
                    enter_state(STATE_AFE_STANDBY, BT_DEVICE_MODE_CONNECTABLE);
                }
                else
                {
                    if (b2s_connected_is_full())
                    {
                        enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_IDLE);
                    }
                    else
                    {
                        enter_state(STATE_AFE_CONNECTED, BT_DEVICE_MODE_CONNECTABLE);
                    }
                }
            }

            is_pairing_timeout = false;
        }
        break;

    case STABLE_ENTER_MODE_DIRECT_PAIRING:
        {
            enter_state(STATE_AFE_PAIRING_MODE, BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
        }
        break;

    default:
        break;
    }
}

static bool bt_event_handle(T_EVENT event, T_BT_PARAM *param)
{
    bool is_done = false;

    switch (event)
    {
    case EVENT_SRC_DISCONN_NORMAL:
        {
            if (disconnect_for_pairing_mode)
            {
                disconnect_for_pairing_mode = false;

                stable_sched(STABLE_ENTER_MODE_DEDICATED_PAIRING);

                is_done = true;
            }
        }
        break;

    case EVENT_PAIRING_MODE_TIMEOUT:
        {
            stable_sched(STABLE_ENTER_MODE_NOT_PAIRING);
        }
        break;

    default:
        break;
    }

    return is_done;
}

static bool bt_event_pre_handle(T_EVENT event, T_BT_PARAM *param)
{
    bool is_done = true;
    T_BP_EVENT_PARAM event_param;
    T_APP_BR_LINK *p_link;

    event_param.bd_addr = param->bd_addr;
    event_param.cause = param->cause;

    switch (event)
    {
    case EVENT_SRC_CONN_SUC:
        {
            bool suc;
            uint8_t id;

            suc = b2s_connected_add_node(param->bd_addr, &id);
            if (suc)
            {
                if (b2s_connected_is_over())
                {
                    app_multi_disc_inactive_link(id);
                }
                app_bt_policy_sync_b2s_connected();
            }

            if (BT_DEVICE_MODE_IDLE == radio_mode)
            {
                gap_br_send_acl_disconn_req(param->bd_addr);
            }

        }
        break;

    case EVENT_SRC_CONN_TIMEOUT:
        {
            if (linkback_active_node.is_valid)
            {
                if (linkback_active_node.linkback_node.is_group_member)
                {
                    linkback_todo_queue_delete_group_member();
                }
            }

            if (STATE_AFE_LINKBACK != cur_state)
            {
                linkback_active_node.is_valid = false;
            }

            app_stop_timer(&timer_idx_linkback_delay);
        }
        break;

    case EVENT_SRC_AUTH_LINK_KEY_INFO:
        {
            T_APP_REMOTE_MSG_PAYLOAD_LINK_KEY_ADDED link_key;

            app_bond_key_set(param->bd_addr, param->link_key, param->key_type);

            link_key.key_type = param->key_type;
            memcpy(link_key.bd_addr, param->bd_addr, 6);
            memcpy(link_key.link_key, param->link_key, 16);

            uint8_t mapping_idx;
            app_bt_update_pair_idx_mapping();
            if (app_bond_get_pair_idx_mapping(param->bd_addr, &mapping_idx))
            {
                app_cfg_nv.audio_gain_level[mapping_idx] = app_dsp_cfg_vol.playback_volume_default;
                app_cfg_nv.voice_gain_level[mapping_idx] = app_dsp_cfg_vol.voice_out_volume_default;
            }


        }
        break;

    case EVENT_SRC_AUTH_LINK_KEY_REQ:
        {
            uint8_t link_key[16];
            T_BT_LINK_KEY_TYPE type;

            if (bt_bond_key_get(param->bd_addr, link_key, (uint8_t *)&type))
            {
                bt_link_key_cfm(param->bd_addr, true, type, link_key);
            }
            else
            {
                bt_link_key_cfm(param->bd_addr, false, type, link_key);
            }
        }
        break;

    case EVENT_SRC_AUTH_LINK_PIN_CODE_REQ:
        {
            uint8_t pin_code[4] = {1, 2, 3, 4};

            bt_link_pin_code_cfm(param->bd_addr, pin_code, 4, true);
        }
        break;

    case EVENT_SRC_AUTH_FAIL:
        {
            if ((param->cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)) ||
                (param->cause == (HCI_ERR | HCI_ERR_KEY_MISSING)))
            {
                T_APP_LINK_RECORD link_record;

                if (app_bond_pop_sec_diff_link_record(param->bd_addr, &link_record))
                {
                    app_bond_key_set(param->bd_addr, link_record.link_key, link_record.key_type);
                    bt_bond_flag_set(param->bd_addr, link_record.bond_flag);
                }
                else
                {
                    bt_bond_delete(param->bd_addr);

                }
            }

            event_ind(BP_EVENT_SRC_AUTH_FAIL, &event_param);
        }
        break;

    case EVENT_SRC_AUTH_SUC:
        {
            if ((app_bt_policy_cfg.enter_pairing_while_only_one_device_connected) &&
                (b2s_connected.num == 1))
            {
                discoverable_when_one_link = true;
            }
            else
            {
                discoverable_when_one_link = false;
            }
#if F_APP_GUI_SUPPORT
            app_panel_bredr_alloc_link(param->bd_addr);
#endif
            connected_node_auth_suc(param->bd_addr);
            b2s_connected_mark_index(param->bd_addr);
            event_ind(BP_EVENT_SRC_AUTH_SUC, &event_param);
        }
        break;

    case EVENT_SRC_DISCONN_LOST:
        {
            bool is_authed;
            bool is_first_src;
            uint32_t bond_flag = 0;
            uint32_t plan_profs = 0;

            is_authed = connected_node_is_authed(param->bd_addr);
            is_first_src = b2s_connected_is_first_src(param->bd_addr);


            app_sniff_mode_b2s_check_left_flag_when_disconnect(param->bd_addr);

            if (b2s_connected_del_node(param->bd_addr))
            {
                bt_bond_flag_get(param->bd_addr, &bond_flag);

                if (bond_flag & (BOND_FLAG_HFP | BOND_FLAG_HSP | BOND_FLAG_A2DP))
                {
                    plan_profs = get_profs_by_bond_flag(bond_flag);

                    linkback_todo_queue_insert_normal_node(param->bd_addr, plan_profs,
                                                           app_bt_policy_cfg.timer_link_back_loss * 1000, false);
                }

                ENGAGE_PRINT_TRACE2("bt_event_pre_handle: bond_flag 0x%x, plan_profs 0x%x", bond_flag, plan_profs);

                if (is_authed)
                {
                    event_param.is_first_src = is_first_src;
                    event_ind(BP_EVENT_SRC_DISCONN_LOST, &event_param);
                }
                app_bt_policy_sync_b2s_connected();



            }
        }
        break;

    case EVENT_SRC_DISCONN_NORMAL:
        {
            bool is_first_src;

            is_src_authed = connected_node_is_authed(param->bd_addr);
            is_first_src = b2s_connected_is_first_src(param->bd_addr);


            app_sniff_mode_b2s_check_left_flag_when_disconnect(param->bd_addr);

            if (b2s_connected_del_node(param->bd_addr))
            {
                if (is_src_authed)
                {
                    event_param.is_first_src = is_first_src;
                    event_ind(BP_EVENT_SRC_DISCONN_NORMAL, &event_param);
                }
                app_bt_policy_sync_b2s_connected();
            }
        }
        break;

    case EVENT_SRC_DISCONN_ROLESWAP:
        {
            b2s_connected_del_node(param->bd_addr);

            linkback_todo_queue_delete_all_node();

            linkback_active_node.is_valid = false;

            event_ind(BP_EVENT_SRC_DISCONN_ROLESWAP, &event_param);
        }
        break;

    case EVENT_PROFILE_CONN_SUC:
        {
            event_param.is_first_prof = b2s_connected_no_profs(param->bd_addr);
            event_param.is_first_src = b2s_connected_is_first_src(param->bd_addr);

            b2s_connected_add_prof(param->bd_addr, param->prof);

            p_link = app_link_find_br_link(param->bd_addr);

            if (p_link != NULL)
            {
                src_conn_ind_add(param->bd_addr, true);

                if (A2DP_PROFILE_MASK == param->prof)
                {
                    if ((0 == (AVRCP_PROFILE_MASK & p_link->connected_profile)) && (!p_link->acl.disconnected))
                    {
                        app_stop_timer(&timer_idx_later_avrcp[p_link->id]);
                        app_start_timer(&timer_idx_later_avrcp[p_link->id], "later_avrcp",
                                        bt_policy_timer_id, APP_TIMER_LATER_AVRCP, p_link->id, false,
                                        app_bt_policy_cfg.timer_link_avrcp);
                    }
                }
                else if (AVRCP_PROFILE_MASK == param->prof)
                {
                    app_stop_timer(&timer_idx_later_avrcp[p_link->id]);
                }
#if F_APP_BT_HID_DEVICE_SUPPORT
                else if (HID_PROFILE_MASK == param->prof)
                {
                    app_stop_timer(&timer_idx_later_hid[p_link->id]);
                }
#endif

                if (event_param.is_first_prof)
                {
                    uint32_t bond_flag = 0;

                    app_sniff_mode_b2s_enable(param->bd_addr, SNIFF_DISABLE_MASK_ACL);

                    bt_bond_flag_get(param->bd_addr, &bond_flag);
#if F_APP_BT_HID_DEVICE_SUPPORT
                    if (bond_flag & BOND_FLAG_HID)
                    {
                        if ((0 == (HID_PROFILE_MASK & p_link->connected_profile)) && (!p_link->acl.disconnected))
                        {
                            app_stop_timer(&timer_idx_later_hid[p_link->id]);
                            app_start_timer(&timer_idx_later_hid[p_link->id], "later_hid",
                                            bt_policy_timer_id, APP_TIMER_LATER_HID, p_link->id, false,
                                            2000);
                        }
                    }
#endif
                    app_bt_policy_default_connect(param->bd_addr, DID_PROFILE_MASK, false);
                }
            }

            event_ind(BP_EVENT_PROFILE_CONN_SUC, &event_param);
        }
        break;

    case EVENT_PROFILE_DISCONN:
        {
            if (param->cause != (L2C_ERR | L2C_ERR_NO_RESOURCE))
            {
                if (A2DP_PROFILE_MASK == param->prof)
                {
                    bt_avrcp_disconnect_req(param->bd_addr);
                }

                b2s_connected_del_prof(param->bd_addr, param->prof);

                event_param.is_last_prof = false;
                event_param.prof = param->prof;

                p_link = app_link_find_br_link(param->bd_addr);
                if (p_link != NULL)
                {
                    if ((p_link->connected_profile & ~HID_PROFILE_MASK) == 0)
                    {
                        event_param.is_last_prof = true;

                        if (p_link->acl.disconnected
#if F_APP_ANC_SUPPORT
                            || app_anc_get_measure_mode()
#endif
                           )
                        {
                            p_link->acl.disconnected = false;
                            gap_br_send_acl_disconn_req(param->bd_addr);
                        }
                    }
                    else
                    {
                        if (p_link->acl.disconnected)
                        {
                            if ((p_link->connected_profile & p_link->plan_disconnect_profs) == 0)
                            {
                                p_link->acl.disconnected = false;
                                gap_br_send_acl_disconn_req(param->bd_addr);
                            }
                        }
                    }
                }

                event_ind(BP_EVENT_PROFILE_DISCONN, &event_param);
            }
        }
        break;

    case EVENT_PAIRING_MODE_TIMEOUT:
        {
            is_pairing_timeout = true;

            event_ind(BP_EVENT_PAIRING_MODE_TIMEOUT, NULL);
        }
        break;

    case EVENT_SRC_CONN_IND_TIMEOUT:
        {
            src_conn_ind_del(param->cause);
        }
        break;

    default:
        {
            is_done = false;
        }
        break;
    }

    return is_done;
}

#if 0
static bool sec_src_handle(T_EVENT event, T_BT_PARAM *param)
{
    bool is_done = true;

    switch (event)
    {
    case EVENT_SRC_CONN_SUC:
        {
            uint8_t id;

            b2s_connected_add_node(param->bd_addr, &id);
        }
        break;

    case EVENT_SRC_DISCONN_LOST:
    case EVENT_SRC_DISCONN_NORMAL:
    case EVENT_SRC_DISCONN_ROLESWAP:
        {
            b2s_connected_del_node(param->bd_addr);
        }
        break;

    case EVENT_PROFILE_CONN_SUC:
        {
            b2s_connected_add_prof(param->bd_addr, param->prof);
        }
        break;

    case EVENT_PROFILE_DISCONN:
        {
            b2s_connected_del_prof(param->bd_addr, param->prof);
        }
        break;

    default:
        {
            is_done = false;
        }
        break;
    }

    return is_done;
}
#endif

static bool event_handle(T_EVENT event, void *param)
{
    bool ret = true;

    switch (event)
    {
    case EVENT_STARTUP:
        {
            startup_event_handle(param);
        }
        break;

    case EVENT_ENTER_DUT_TEST_MODE:
        {
            enter_dut_test_mode_event_handle();
        }
        break;

    case EVENT_SHUTDOWN:
        {
            shutdown_event_handle();
        }
        break;

    case EVENT_STOP:
        {
            stop_event_handle();
        }
        break;

    case EVENT_RESTORE:
        {
            restore_event_handle();
        }
        break;

    case EVENT_CONN_SNIFF:
        {
            conn_sniff_event_handle(param);
        }
        break;

    case EVENT_CONN_ACTIVE:
        {
            conn_active_event_handle(param);
        }
        break;


    case EVENT_ROLE_MASTER:
        {
            role_master_event_handle(param);
        }
        break;

    case EVENT_ROLE_SLAVE:
        {
            role_slave_event_handle(param);
        }
        break;

    case EVENT_ROLESWITCH_TIMEOUT:
        {
        }
        break;

    case EVENT_DEDICATED_ENTER_PAIRING_MODE:
        {
            dedicated_enter_pairing_mode_event_handle(param);
        }
        break;

    case EVENT_DEDICATED_EXIT_PAIRING_MODE:
        {
            dedicated_exit_pairing_mode_event_handle();
        }
        break;

    case EVENT_DEDICATED_CONNECT:
        {
            dedicated_connect_event_handle(param);
        }
        break;

    case EVENT_DEDICATED_DISCONNECT:
        {
            dedicated_disconnect_event_handle(param);
        }
        break;

    case EVENT_DISCONNECT_ALL:
        {
            disconnect_all_event_handle();
        }
        break;

    default:
        {
            ret = false;
        }
        break;
    }

    return ret;
}


void app_bt_policy_state_machine(T_EVENT event, void *param)
{
    cur_event = event;

    event_info(event);

    if (!event_handle(event, param))
    {
        if (bt_event_pre_handle(event, param))
        {
            connected();
        }

        if (bt_event_handle(event, param))
        {
            return;
        }

        switch (cur_state)
        {
        case STATE_SHUTDOWN_STEP:
            {
                state_shutdown_step_event_handle(event);
            }
            break;

        case STATE_AFE_LINKBACK:
            {
                state_afe_linkback_event_handle(event, param);
            }
            break;

        case STATE_AFE_CONNECTED:
        case STATE_AFE_PAIRING_MODE:
        case STATE_AFE_STANDBY:
            {
                state_afe_stable_event_handle(event);
            }
            break;

        case STATE_DUT_TEST_MODE:
            {
                state_dut_test_mode_event_handle(event);
            }
            break;

        default:
            break;
        }
    }
}

static void timer_cback(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_SHUTDOWN_STEP:
        {
            app_stop_timer(&timer_idx_shutdown_step);
            app_bt_policy_state_machine(EVENT_SHUTDOWN_STEP_TIMEOUT, NULL);
        }
        break;

    case APP_TIMER_PAIRING_MODE:
        {
            app_stop_timer(&timer_idx_pairing_mode);
            app_bt_policy_state_machine(EVENT_PAIRING_MODE_TIMEOUT, NULL);
        }
        break;

    case APP_TIMER_DISCOVERABLE:
        {
            app_stop_timer(&timer_idx_discoverable);
            app_bt_policy_state_machine(EVENT_DISCOVERABLE_TIMEOUT, NULL);
        }
        break;

    case APP_TIMER_ROLE_SWITCH:
        {
            T_APP_BR_LINK *p_link;

            p_link = &app_db.br_link[param];
            app_stop_timer(&timer_idx_role_switch[p_link->id]);
            app_bt_policy_state_machine(EVENT_ROLESWITCH_TIMEOUT, p_link);
        }
        break;

    case APP_TIMER_LINKBACK:
        {
            T_BT_PARAM bt_param;

            memset(&bt_param, 0, sizeof(T_BT_PARAM));
            bt_param.not_check_addr_flag = true;

            app_stop_timer(&timer_idx_linkback);
            app_bt_policy_state_machine(EVENT_SRC_CONN_TIMEOUT, &bt_param);
        }
        break;

    case APP_TIMER_LINKBACK_DELAY:
        {
            T_BT_PARAM bt_param;

            memset(&bt_param, 0, sizeof(T_BT_PARAM));
            bt_param.not_check_addr_flag = true;
            bt_param.cause = param;

            app_stop_timer(&timer_idx_linkback_delay);
            app_bt_policy_state_machine(EVENT_LINKBACK_DELAY_TIMEOUT, &bt_param);
        }
        break;

    case APP_TIMER_LATER_AVRCP:
        {
            T_APP_BR_LINK *p_link;
            T_BT_PARAM bt_param;

            p_link = &app_db.br_link[param];
            memset(&bt_param, 0, sizeof(T_BT_PARAM));
            bt_param.bd_addr = p_link->bd_addr;
            bt_param.prof = AVRCP_PROFILE_MASK;
            bt_param.is_special = false;
            bt_param.check_bond_flag = false;
            bt_param.is_later_avrcp = true;

            app_stop_timer(&timer_idx_later_avrcp[p_link->id]);
            app_bt_policy_state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
        }
        break;

    case APP_TIMER_LATER_HID:
        {
            T_APP_BR_LINK *p_link;
            T_BT_PARAM bt_param;

            p_link = &app_db.br_link[param];
            memset(&bt_param, 0, sizeof(T_BT_PARAM));
            bt_param.bd_addr = p_link->bd_addr;
            bt_param.prof = HID_PROFILE_MASK;
            bt_param.is_special = false;
            bt_param.check_bond_flag = true;
            bt_param.is_later_hid = true;

            app_stop_timer(&timer_idx_later_hid[p_link->id]);
            app_bt_policy_state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
        }
        break;

    case APP_TIMER_RECONNECT:
        {
            app_stop_timer(&timer_idx_reconnect);
        }
        break;

    case APP_TIMER_SRC_CONN_IND:
        {
            T_BT_PARAM bt_param;

            if (src_conn_ind[param].timer_idx != 0)
            {
                memset(&bt_param, 0, sizeof(T_BT_PARAM));
                bt_param.cause = param;

                app_stop_timer(&src_conn_ind[param].timer_idx);

                app_hfp_adjust_sco_window(src_conn_ind[param].bd_addr, APP_SCO_ADJUST_ACL_CONN_END_EVT);

                app_bt_policy_state_machine(EVENT_SRC_CONN_IND_TIMEOUT, &bt_param);
            }
        }
        break;

    case APP_TIMER_B2S_CONN:
        {
            app_stop_timer(&timer_idx_reconnect);
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {

                }
            }
        }
        break;

    case APP_TIMER_LATER_A2DP:
        {
            app_stop_timer(&timer_idx_later_a2dp);
            linkback_profile_connect_start(later_a2dp_addr, A2DP_SINK_PROFILE_MASK,
                                           &linkback_active_node.linkback_conn_param);
        }
        break;

    case APP_TIMER_LINKBACK_POWER_ON:
        {
            app_stop_timer(&timer_idx_linkback_power_on);
            linkback_sched();
        }
        break;

    default:
        break;
    }
}

void app_bt_update_pair_idx_mapping(void)
{
    uint8_t bond_num = app_bond_b2s_num_get();
    uint8_t pair_idx[8] = {0};
    uint8_t bd_addr[6] = {0};
    uint8_t pair_idx_temp = 0;
    uint8_t i = 0;
    uint8_t j = 0;
    bool mapping_flag = false;

    APP_PRINT_TRACE1("app_bt_update_pair_idx_mapping bond_num = %d", bond_num);

    // APP_PRINT_TRACE8("start app_bt_update_pair_idx_mapping: app_pair_idx_mapping %d,  %d, %d, %d, %d, %d, %d, %d",
    //                  app_cfg_nv.app_pair_idx_mapping[0], app_cfg_nv.app_pair_idx_mapping[1],
    //                  app_cfg_nv.app_pair_idx_mapping[2], app_cfg_nv.app_pair_idx_mapping[3],
    //                  app_cfg_nv.app_pair_idx_mapping[4], app_cfg_nv.app_pair_idx_mapping[5],
    //                  app_cfg_nv.app_pair_idx_mapping[6], app_cfg_nv.app_pair_idx_mapping[7]);

    if (bond_num > 8)
    {
        bond_num = 8;
    }

    for (i = 1; i <= bond_num; i++)
    {
        if (app_bond_b2s_addr_get(i, bd_addr))
        {
            if (bt_bond_index_get(bd_addr, &pair_idx_temp))
            {
                pair_idx[i - 1] = pair_idx_temp;
            }
        }
    }

    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < bond_num; i++)
        {
            if (app_cfg_nv.app_pair_idx_mapping[j] == pair_idx[i])
            {
                mapping_flag = true;
                break;
            }
        }

        if (!mapping_flag)
        {
            app_cfg_nv.audio_gain_level[j] = app_dsp_cfg_vol.playback_volume_default;
            app_cfg_nv.voice_gain_level[j] = app_dsp_cfg_vol.voice_out_volume_default;
            app_cfg_nv.app_pair_idx_mapping[j] = 0xFF;
        }

        mapping_flag = false;
    }

    for (i = 0; i < bond_num; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if (app_cfg_nv.app_pair_idx_mapping[j] == pair_idx[i])
            {
                mapping_flag = true;
                break;
            }
        }

        if (!mapping_flag)
        {
            for (j = 0; j < 8; j++)
            {
                if (app_cfg_nv.app_pair_idx_mapping[j] == 0xFF)
                {
                    app_cfg_nv.app_pair_idx_mapping[j] = pair_idx[i];
                    app_cfg_nv.audio_gain_level[j] = app_dsp_cfg_vol.playback_volume_default;
                    app_cfg_nv.voice_gain_level[j] = app_dsp_cfg_vol.voice_out_volume_default;
                    break;
                }
            }
        }
        mapping_flag = false;
    }

    // APP_PRINT_TRACE8("end app_bt_update_pair_idx_mapping: app_pair_idx_mapping %d,  %d, %d, %d, %d, %d, %d, %d",
    //                  app_cfg_nv.app_pair_idx_mapping[0], app_cfg_nv.app_pair_idx_mapping[1],
    //                  app_cfg_nv.app_pair_idx_mapping[2], app_cfg_nv.app_pair_idx_mapping[3],
    //                  app_cfg_nv.app_pair_idx_mapping[4], app_cfg_nv.app_pair_idx_mapping[5],
    //                  app_cfg_nv.app_pair_idx_mapping[6], app_cfg_nv.app_pair_idx_mapping[7]);

    // APP_PRINT_TRACE8("end app_bt_update_pair_idx_mapping: pair_idx %d,  %d, %d, %d, %d, %d, %d, %d",
    //                  pair_idx[0], pair_idx[1],
    //                  pair_idx[2], pair_idx[3],
    //                  pair_idx[4], pair_idx[5],
    //                  pair_idx[6], pair_idx[7]);
}

static void app_bt_policy_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_BT_PARAM bt_param;
    T_APP_BR_LINK *p_link = NULL;
    static uint8_t a2dp_link_count = 0;
    static uint8_t hfp_link_count = 0;
    static bool timer_active = false;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));
    APP_PRINT_TRACE1("app_bt_policy_cback event_type :0x%x", event_type);

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            bt_param.bd_addr = param->acl_conn_success.bd_addr;
            bool suc;
            uint8_t id;
            enum LINK_PARAM_POS
            {
                STATE_POS       = 0,
                ADDR_POS        = STATE_POS + 1,
                APP_LINK_ID_POS = ADDR_POS + 6,
                CONN_HANDLE_POS = APP_LINK_ID_POS + 1,
                END_POS         = CONN_HANDLE_POS + 2,
            };
            uint8_t temp_buff[END_POS];
            T_APP_BR_LINK *p_link = NULL;
            suc = b2s_connected_add_node(param->acl_conn_success.bd_addr, &id);
            if (suc)
            {
                if (b2s_connected_is_over())
                {
                    app_multi_disc_inactive_link(id);
                }
                app_bt_policy_sync_b2s_connected();
            }

            p_link = app_link_find_br_link(param->acl_conn_success.bd_addr);
            if (p_link != NULL)
            {
                p_link->acl.conn_handle = param->acl_conn_success.handle;

#if 0
                T_APP_LE_LINK *p_le_link = app_link_find_le_link_by_addr(param->acl_conn_success.bd_addr);
                if (p_le_link)
                {
                    le_disconnect(p_le_link->conn_id);
                }
#endif
                temp_buff[STATE_POS] = ACL_LINK_STATE_CONNECTED;
                memcpy(&temp_buff[ADDR_POS], p_link->bd_addr, 6);
                temp_buff[APP_LINK_ID_POS] = p_link->id;
                temp_buff[CONN_HANDLE_POS] = p_link->acl.conn_handle;
                temp_buff[CONN_HANDLE_POS + 1] = p_link->acl.conn_handle >> 8;
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
                app_tri_dongle_if_bt_evt(REDEV_MSG_BT_CONN_REPORT, &temp_buff,
                                         NULL);
#endif
#if F_APP_USB_HID_PC_TOOL
                app_report_event(CMD_PATH_USB_HID, EVENT_BR_LINK_STATUS, 0, temp_buff, sizeof(temp_buff));
#else
                app_report_event(CMD_PATH_UART, EVENT_BR_LINK_STATUS, 0, temp_buff, sizeof(temp_buff));
#endif
            }
        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {
            APP_PRINT_INFO1("BT_EVENT_ACL_CONN_FAIL acl_conn_fail.cause =%x ",
                            param->acl_conn_fail.cause);
            if (param->acl_conn_fail.cause == (HCI_ERR | HCI_ERR_PAGE_TIMEOUT))
            {

#if APP_DASHBOARD_DEMO_SUPPORT
                bt_param.bd_addr = param->acl_conn_fail.bd_addr;
                app_bt_policy_state_machine(EVENT_SRC_CONN_FAIL, &bt_param);
#else
                linkback_todo_queue_delete_all_node();
                linkback_todo_queue_insert_normal_node(param->acl_conn_fail.bd_addr, profiles_to_connect,
                                                       app_bt_policy_cfg.timer_linkback_timeout * 1000, 0);
                linkback_run();
#endif

            }
        }
        break;

    case BT_EVENT_ACL_AUTHEN_START:
        {
            authentic_fail = true;
        }
        break;

    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        {
            bt_param.bd_addr = param->acl_authen_success.bd_addr;
            connected_node_auth_suc(param->acl_authen_success.bd_addr);
            authentic_fail = false;
            bt_param.not_check_addr_flag = true;
            app_bt_policy_state_machine(EVENT_SRC_AUTH_SUC, &bt_param);
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            enum LINK_PARAM_POS
            {
                STATE_POS       = 0,
                ADDR_POS        = STATE_POS + 1,
                APP_LINK_ID_POS = ADDR_POS + 6,
                CONN_HANDLE_POS = APP_LINK_ID_POS + 1,
                END_POS         = CONN_HANDLE_POS + 2,
            };
            uint8_t temp_buff[END_POS];
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_link_find_br_link(param->acl_authen_success.bd_addr);
            if (p_link != NULL)
            {
                temp_buff[STATE_POS] = EVENT_BR_ACL_AUTHEN_SUCCESS;
                memcpy(&temp_buff[ADDR_POS], p_link->bd_addr, 6);
                temp_buff[APP_LINK_ID_POS] = p_link->id;
                temp_buff[CONN_HANDLE_POS] = p_link->acl.conn_handle;
                temp_buff[CONN_HANDLE_POS + 1] = p_link->acl.conn_handle >> 8;

                app_tri_dongle_if_bt_evt(REDEV_MSG_BT_ACL_AUTHEN_REPORT, &temp_buff,
                                         NULL);
                bool ret = false;
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
#if F_APP_BT_AUDIO_TRI_LEA_HIGH_PRIORITY
                ret = app_bond_mgr_le_find_for_lea(param->acl_authen_success.bd_addr);
                APP_PRINT_TRACE1("app_tri_dongle_mgr_target_profle p3 %d", ret);
#endif
#endif
                if (!ret)
                {
                    app_tri_dongle_set_pc_tool_inreport(temp_buff, sizeof(temp_buff));
                }
            }
#endif
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            APP_PRINT_INFO1("app_bt_policy_cback: acl_authen_fail.cause = %x ",
                            param->acl_authen_fail.cause);
            enum LINK_PARAM_POS
            {
                STATE_POS       = 0,
                ADDR_POS        = STATE_POS + 1,
                APP_LINK_ID_POS = ADDR_POS + 6,
                CONN_HANDLE_POS = APP_LINK_ID_POS + 1,
                END_POS         = CONN_HANDLE_POS + 2,
            };
            uint8_t temp_buff[END_POS];
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_link_find_br_link(param->acl_authen_fail.bd_addr);
            if ((p_link != NULL) && (authentic_fail ||
                                     param->acl_authen_fail.cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)))
            {
                APP_PRINT_TRACE2("BT_EVENT_ACL_AUTHEN_FAIL ACL_LINK_STATE_AUTHENTIC_FAIL: authentic_fail %d, cause 0x%x",
                                 authentic_fail, param->acl_authen_fail.cause);
                authentic_fail = false;
                temp_buff[STATE_POS] = ACL_LINK_STATE_AUTHENTIC_FAIL;
                memcpy(&temp_buff[ADDR_POS], p_link->bd_addr, 6);
                temp_buff[APP_LINK_ID_POS] = p_link->id;
                temp_buff[CONN_HANDLE_POS] = p_link->acl.conn_handle;
                temp_buff[CONN_HANDLE_POS + 1] = p_link->acl.conn_handle >> 8;
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
                app_tri_dongle_if_bt_evt(REDEV_MSG_BT_CONN_REPORT, &temp_buff,
                                         NULL);
#endif
#if F_APP_USB_HID_PC_TOOL
                app_report_event(CMD_PATH_USB_HID, EVENT_BR_LINK_STATUS, 0, temp_buff, sizeof(temp_buff));
#else
                app_report_event(CMD_PATH_UART, EVENT_BR_LINK_STATUS, 0, temp_buff, sizeof(temp_buff));
#endif
            }

            if ((param->acl_authen_fail.cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)) ||
                (param->acl_authen_fail.cause == (HCI_ERR | HCI_ERR_KEY_MISSING)))
            {
                bt_bond_delete(param->acl_authen_fail.bd_addr);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            enum LINK_PARAM_POS
            {
                STATE_POS       = 0,
                ADDR_POS        = STATE_POS + 1,
                APP_LINK_ID_POS = ADDR_POS + 6,
                CONN_HANDLE_POS = APP_LINK_ID_POS + 1,
                END_POS         = CONN_HANDLE_POS + 2,
            };
            T_APP_BR_LINK *p_link = NULL;
            uint8_t temp_buff[END_POS];
            bool send_event = false;

            p_link = app_link_find_br_link(param->acl_conn_disconn.bd_addr);
#endif
            bt_param.bd_addr = param->acl_conn_disconn.bd_addr;
            APP_PRINT_INFO1("app_bt_policy_cback: acl_conn_disconn.cause = %x ",
                            param->acl_conn_disconn.cause);
#if F_APP_GUI_SUPPORT
            app_panel_bredr_free_link(param->acl_conn_disconn.bd_addr);
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            if (p_link)
            {
                if ((param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                    (param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
                {
                    temp_buff[STATE_POS] = ACL_LINK_STATE_LOST;
                }
                else
                {
                    temp_buff[STATE_POS] = ACL_LINK_STATE_STANDBY;
                }
                memcpy(&temp_buff[ADDR_POS], p_link->bd_addr, 6);
                temp_buff[APP_LINK_ID_POS] = p_link->id;
                temp_buff[CONN_HANDLE_POS] = p_link->acl.conn_handle;
                temp_buff[CONN_HANDLE_POS + 1] = p_link->acl.conn_handle >> 8;
                send_event = true;
            }
#endif

#if APP_DASHBOARD_DEMO_SUPPORT
            if ((param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                (param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
            {
                bt_param.cause = param->acl_conn_disconn.cause;
                app_bt_policy_state_machine(EVENT_SRC_DISCONN_LOST, &bt_param);
            }
#else
            b2s_connected_del_node(param->acl_conn_disconn.bd_addr);
            app_bt_policy_qos_param_update(bt_param.bd_addr, BP_TPOLL_ACL_DISC_EVENT);
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            if (send_event)
            {
                app_tri_dongle_if_bt_evt(REDEV_MSG_BT_CONN_REPORT, &temp_buff,
                                         NULL);
#if F_APP_USB_HID_PC_TOOL
                app_report_event(CMD_PATH_USB_HID, EVENT_BR_LINK_STATUS, 0, temp_buff, sizeof(temp_buff));
#else
                app_report_event(CMD_PATH_UART, EVENT_BR_LINK_STATUS, 0, temp_buff, sizeof(temp_buff));
#endif
            }
#endif

        }
        break;

    case BT_EVENT_ACL_ROLE_MASTER:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->acl_role_master.bd_addr);
            if (p_link != NULL)
            {
                p_link->acl.role = BT_LINK_ROLE_MASTER;
#if F_APP_ACL_ROLE_FORCE_MASTER
                bt_role_switch_disable(param->acl_role_master.bd_addr);
#endif
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->acl_role_slave.bd_addr);
            if (p_link != NULL)
            {
                p_link->acl.role = BT_LINK_ROLE_SLAVE;
#if F_APP_ACL_ROLE_FORCE_MASTER
                bt_link_role_switch(param->acl_role_slave.bd_addr, true);
#endif
            }
        }
        break;

    case BT_EVENT_ACL_CONN_SNIFF:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->acl_conn_sniff.bd_addr);
            if (p_link != NULL)
            {
                p_link->acl.in_sniffmode = true;
            }
        }
        break;

    case BT_EVENT_ACL_CONN_ENCRYPTED:
        {
            app_bt_policy_qos_param_update(param->acl_conn_encrypted.bd_addr, BP_TPOLL_ACL_CONN_EVENT);
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {

#if F_APP_INTEGRATED_TRANSCEIVER
            if (!htpoll_set)
            {
                bt_link_periodic_traffic_qos_set(param->acl_conn_ready.bd_addr, 24, 12, 0xffff, false);
            }
            htpoll_set = true;
#else
            /* TODO not set pkt 2m in DUT Test Mode */
            bt_acl_pkt_type_set(param->acl_conn_ready.bd_addr, BT_ACL_PKT_TYPE_2M);
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            /* set pkt 2DH5 */
            uint16_t pkt_type = 0;
            pkt_type = GAP_PKT_TYPE_DM1 | GAP_PKT_TYPE_DH1 | \
                       GAP_PKT_TYPE_DM3 | GAP_PKT_TYPE_DH3 | \
                       GAP_PKT_TYPE_DM5 | GAP_PKT_TYPE_DH5 | \
                       GAP_PKT_TYPE_NO_3DH1 | GAP_PKT_TYPE_NO_3DH3 | \
                       GAP_PKT_TYPE_NO_3DH5 | GAP_PKT_TYPE_NO_2DH1 | GAP_PKT_TYPE_NO_2DH3;
            gap_br_cfg_acl_pkt_type(param->acl_conn_ready.bd_addr, pkt_type);
#endif
#endif

//            T_APP_BR_LINK *p_link;
//            p_link = app_link_find_br_link(param->acl_conn_ready.bd_addr);
//            if (p_link == NULL)
//            {
//                return;
//            }
//            if (app_bt_bond_get_cod_type(param->acl_conn_ready.bd_addr) == T_DEVICE_TYPE_PHONE
//                && p_link->acl_link_role != BT_LINK_ROLE_SLAVE)
//            {
//                bt_link_role_switch(param->acl_conn_ready.bd_addr, false);//add later
//            }

        }
        break;

    case BT_EVENT_ACL_CONN_IND:
        {
            APP_PRINT_TRACE1("app_bt_policy_cback: conn ind device cod 0x%08x", param->acl_conn_ind.cod);
            p_link = app_link_find_br_link(param->acl_conn_ind.bd_addr);
            if (p_link != NULL || b2s_connected_is_full())
            {
                bt_acl_conn_reject(param->acl_conn_ind.bd_addr, BT_ACL_REJECT_UNACCEPTABLE_ADDR);
            }
            else
            {
                if (((param->acl_conn_ind.cod & 0x1F00) >> 8) == 0x02)
                {
                    APP_PRINT_INFO1("connect to phone cod = 0x%x", param->acl_conn_ind.cod);
                    // add policy
#if F_APP_ACL_ROLE_FORCE_MASTER
                    bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_MASTER);
#else
                    bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
#endif

                }
                else if (((param->acl_conn_ind.cod & 0x1F00) >> 8) == 0x04)
                {
                    APP_PRINT_INFO1("connect to audio/video cod = 0x%x", param->acl_conn_ind.cod);
                    // add policy
                    bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_MASTER);
                }
                else if (((param->acl_conn_ind.cod & 0x1F00) >> 8) == 0x01)
                {
                    APP_PRINT_INFO1("connect to pc cod = 0x%x", param->acl_conn_ind.cod);
                    // add policy
#if F_APP_ACL_ROLE_FORCE_MASTER
                    bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_MASTER);
#else
                    bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
#endif
                }
                else
                {
                    APP_PRINT_INFO1("unknown device cod = 0x%x", param->acl_conn_ind.cod);
#if F_APP_ACL_ROLE_FORCE_MASTER
                    bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_MASTER);
#else
                    bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
#endif
                }

            }
        }
        break;

    case BT_EVENT_LINK_KEY_INFO:
        {
            authentic_fail = false;
            bt_bond_key_set(param->link_key_info.bd_addr, param->link_key_info.link_key,
                            param->link_key_info.key_type);
            app_bt_update_pair_idx_mapping();
        }
        break;

    case BT_EVENT_LINK_KEY_REQ:
        {
            uint8_t link_key[16];
            T_BT_LINK_KEY_TYPE type;
            authentic_fail = false;
            if (bt_bond_key_get(param->link_key_req.bd_addr, link_key, (uint8_t *)&type))
            {
                bt_link_key_cfm(param->link_key_req.bd_addr, true, type, link_key);
            }
            else
            {
                bt_link_key_cfm(param->link_key_req.bd_addr, false, type, link_key);
            }
        }
        break;

    case BT_EVENT_LINK_PIN_CODE_REQ:
        {
            bt_link_pin_code_cfm(param->link_pin_code_req.bd_addr, app_cfg_nv.pin_code,
                                 app_cfg_nv.pin_code_size, true);
        }
        break;

    case BT_EVENT_SCO_CONN_IND:
        {
            p_link = app_link_find_br_link(param->sco_conn_ind.bd_addr);
//            T_MUSIC_MODE play_mode = app_music_get_mode();

            if (p_link != NULL)
            {
//                if (app_music_get_state() == MUSIC_STATE_PLAY)
//                {
//                    //stop music
//                    if (play_mode == MUSIC_A2DP_SRC)
//                    {
//                        //save state app_db.sco_interrupt_a2dp = SCO_INTERRUPT_A2DP_SOURCE;
//#if F_APP_HFP_AG_SUPPORT
//                   bt_hfp_ag_audio_connect_cfm(param->sco_conn_ind.bd_addr, true);
//#elif F_APP_HFP_HF_SUPPORT
//                    bt_hfp_audio_connect_cfm(param->sco_conn_ind.bd_addr, true);
//#endif
//                    }
//                    else if (play_mode == MUSIC_LOCAL_PLAY)
//                    {
                //app_db.sco_interrupt_a2dp = SCO_INTERRUPT_LOCAL;
#if F_APP_HFP_AG_SUPPORT
                bt_hfp_ag_audio_connect_cfm(param->sco_conn_ind.bd_addr, true);
#endif

#if F_APP_HFP_HF_SUPPORT
                bt_hfp_audio_connect_cfm(param->sco_conn_ind.bd_addr, true);
#endif
//                    }
//                }
//                else
//                {
//#if F_APP_HFP_AG_SUPPORT
//                    bt_hfp_ag_audio_connect_cfm(param->sco_conn_ind.bd_addr, true);
//#elif F_APP_HFP_HF_SUPPORT
//                    bt_hfp_audio_connect_cfm(param->sco_conn_ind.bd_addr, true);
//#endif
//                }
            }
            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_SCO);
        }
        break;

    case BT_EVENT_SCO_CONN_RSP:
        {
            if (param->sco_conn_rsp.cause == 0)
            {
                app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_SCO);
            }
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_link_find_br_link(param->sco_conn_cmpl.bd_addr);

            if (p_link != NULL && param->sco_conn_cmpl.cause == 0)
            {
                p_link->sco.conn_handle = param->sco_conn_cmpl.handle;

//                if (app_bt_policy_get_radio_mode() == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)//wjl set radio mode later
//                {
//                    app_bt_policy_exit_pairing_mode();
//                }

#if !F_APP_INTEGRATED_TRANSCEIVER
                if (app_link_get_sco_conn_num() == 1)
                {
                    bt_sco_link_switch(param->sco_conn_cmpl.bd_addr);
                }
                else
                {
                    APP_PRINT_TRACE2("xm_sco_cback: active sco link %s, current link %s",
                                     TRACE_BDADDR(app_db.br_link[app_hfp_get_active_idx()].bd_addr),
                                     TRACE_BDADDR(param->sco_conn_cmpl.bd_addr));
                    bt_sco_link_switch(app_db.br_link[app_hfp_get_active_idx()].bd_addr);
                }

                app_bt_policy_qos_param_update(param->sco_conn_cmpl.bd_addr, BP_TPOLL_SCO_CONN_EVENT);
                app_hfp_adjust_sco_window(param->sco_conn_cmpl.bd_addr, APP_SCO_ADJUST_SCO_CONN_CMPL_EVT);
#endif

            }
            else
            {
                if (app_link_get_sco_conn_num() == 0)
                {
                    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_SCO);
                }
            }
        }
        break;

    case BT_EVENT_SCO_DISCONNECTED:
        {
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_link_find_br_link(param->sco_disconnected.bd_addr);
            if (app_link_get_sco_conn_num() <= 1)
            {
                app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_SCO);
            }

            if (p_link != NULL)
            {
                p_link->sco.conn_handle = 0;
            }
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {

            if (SPP_PROFILE_MASK == linkback_active_node.doing_prof)
            {
                if (bt_spp_registered_uuid_check((T_BT_SPP_UUID_TYPE)(
                                                     param->sdp_attr_info.info.srv_class_uuid_type),
                                                 (T_BT_SPP_UUID_DATA *) & (param->sdp_attr_info.info.srv_class_uuid_data),
                                                 &linkback_active_node.linkback_conn_param.local_server_chann))
                {
                    linkback_active_node.is_sdp_ok = true;
                    linkback_active_node.linkback_conn_param.server_channel = param->sdp_attr_info.info.server_channel;
                }
            }
            else if ((HFP_HF_PROFILE_MASK == linkback_active_node.doing_prof &&
                      param->sdp_attr_info.info.srv_class_uuid_data.uuid_16 == UUID_HANDSFREE) ||
                     (HSP_HF_PROFILE_MASK == linkback_active_node.doing_prof &&
                      param->sdp_attr_info.info.srv_class_uuid_data.uuid_16 == UUID_HEADSET))
            {
                linkback_active_node.linkback_conn_param.protocol_version =
                    param->sdp_attr_info.info.protocol_version;
                linkback_active_node.linkback_conn_param.server_channel =
                    param->sdp_attr_info.info.server_channel;
                linkback_active_node.linkback_conn_param.feature =
                    param->sdp_attr_info.info.supported_feat;
                linkback_active_node.is_sdp_ok = true;
            }
            else
            {
                linkback_active_node.linkback_conn_param.protocol_version =
                    param->sdp_attr_info.info.protocol_version;
                linkback_active_node.linkback_conn_param.server_channel =
                    param->sdp_attr_info.info.server_channel;
                linkback_active_node.linkback_conn_param.feature =
                    param->sdp_attr_info.info.supported_feat;
                linkback_active_node.is_sdp_ok = true;
            }

        }
        break;

    case  BT_EVENT_DID_ATTR_INFO:
        {

        }
        break;

    case BT_EVENT_SDP_DISCOV_CMPL:
        {
            if ((!b2s_connected_find_node(param->sdp_discov_cmpl.bd_addr, NULL)) ||
                (param->sdp_discov_cmpl.cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) ||
                (param->sdp_discov_cmpl.cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)) ||
                (param->sdp_discov_cmpl.cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                (param->sdp_discov_cmpl.cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
            {
                ENGAGE_PRINT_TRACE0("BT_EVENT_SDP_DISCOV_CMPL: wait EVENT_SRC_CONN_FAIL");
            }
            else
            {
                if (linkback_active_node.is_sdp_ok)
                {
                    if (DID_PROFILE_MASK == linkback_active_node.doing_prof)
                    {
                        linkback_active_node_step_suc_adjust_remain_profs();
                        linkback_run();
                    }
                    else
                    {
                        if (linkback_profile_connect_start(linkback_active_node.linkback_node.bd_addr,
                                                           linkback_active_node.doing_prof, &linkback_active_node.linkback_conn_param))
                        {
                            if (PBAP_PROFILE_MASK == linkback_active_node.doing_prof)
                            {
                                linkback_active_node_step_suc_adjust_remain_profs();
                                linkback_run();
                            }
                        }
                        else
                        {
                            linkback_active_node_step_fail_adjust_remain_profs();
                            linkback_run();
                        }
                    }
                }
                else
                {
                    linkback_active_node_step_fail_adjust_remain_profs();
                    linkback_run();
                }
            }
        }
        break;

    case BT_EVENT_SDP_DISCOV_STOP:
        {

        }
        break;

    case BT_EVENT_HFP_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;
            uint32_t prof;
            p_link = app_link_find_br_link(param->hfp_conn_cmpl.bd_addr);

            if (p_link)
            {
                if (param->hfp_conn_cmpl.is_hfp)
                {
                    prof = HFP_AG_PROFILE_MASK;
                }
                else
                {
                    prof = HSP_AG_PROFILE_MASK;
                }
                b2s_connected_add_prof(param->hfp_conn_cmpl.bd_addr, prof);
                if (linkback_active_node_judge_cur_conn_prof(param->hfp_conn_cmpl.bd_addr, prof))
                {
                    linkback_active_node_step_suc_adjust_remain_profs();
                    linkback_run();
                }
                hfp_link_count++;
            }
        }
        break;

    case BT_EVENT_HFP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_HFP_DISCONN_CMPL:
        {
            uint32_t prof;
            if (param->hfp_disconn_cmpl.is_hfp)
            {
                prof = HFP_AG_PROFILE_MASK;
            }
            else
            {
                prof = HSP_AG_PROFILE_MASK;
            }
            b2s_connected_del_prof(param->hfp_disconn_cmpl.bd_addr, prof);
            if (b2s_connected_no_profs(param->hfp_disconn_cmpl.bd_addr))
            {
                p_link = app_link_find_br_link(param->hfp_disconn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->acl.disconnected)
                    {
                        p_link->acl.disconnected = false;
                        gap_br_send_acl_disconn_req(param->hfp_disconn_cmpl.bd_addr);
                    }
                }
            }
            hfp_link_count--;
        }
        break;

    case BT_EVENT_HFP_CONN_FAIL:
        {

        }
        break;

    case BT_EVENT_HFP_AG_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;
            uint32_t prof;
            p_link = app_link_find_br_link(param->hfp_ag_conn_cmpl.bd_addr);

            if (p_link)
            {
                if (param->hfp_ag_conn_cmpl.is_hfp)
                {
                    prof = HFP_HF_PROFILE_MASK;
                }
                else
                {
                    prof = HSP_HF_PROFILE_MASK;
                }
                b2s_connected_add_prof(param->hfp_ag_conn_cmpl.bd_addr, prof);
                if (linkback_active_node_judge_cur_conn_prof(param->hfp_ag_conn_cmpl.bd_addr, prof))
                {
                    linkback_active_node_step_suc_adjust_remain_profs();
                    linkback_run();
                }

            }

            hfp_link_count++;
            if (hfp_link_count > a2dp_link_count && !is_later_a2dp_connected)
            {
                memcpy(later_a2dp_addr, param->hfp_ag_conn_cmpl.bd_addr, 6);
                app_start_timer(&timer_idx_later_a2dp, "later_a2dp",
                                bt_policy_timer_id, APP_TIMER_LATER_A2DP, 0, false,
                                A2DP_CONN_IND_DELAY_MS);
                timer_active = true;
            }
        }
        break;

    case BT_EVENT_HFP_AG_DISCONN_CMPL:
        {
            uint32_t prof;
            if (param->hfp_ag_disconn_cmpl.is_hfp)
            {
                prof = HFP_HF_PROFILE_MASK;
            }
            else
            {
                prof = HSP_HF_PROFILE_MASK;
            }
            b2s_connected_del_prof(param->hfp_ag_disconn_cmpl.bd_addr, prof);
            if (b2s_connected_no_profs(param->hfp_ag_disconn_cmpl.bd_addr))
            {
                p_link = app_link_find_br_link(param->hfp_ag_disconn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->acl.disconnected)
                    {
                        p_link->acl.disconnected = false;
                        gap_br_send_acl_disconn_req(param->hfp_ag_disconn_cmpl.bd_addr);
                    }
                }
            }
            hfp_link_count--;
        }
        break;

    case BT_EVENT_PBAP_CONN_CMPL:
        {
//            T_APP_BR_LINK *p_link;
//            uint32_t prof;
//            p_link = app_link_find_br_link(param->pbap_conn_cmpl.bd_addr);

//            if (p_link)
//            {
//                prof = PBAP_PROFILE_MASK;
//                b2s_connected_add_prof_ext(param->pbap_conn_cmpl.bd_addr, prof);
//                if (linkback_active_node_judge_cur_conn_prof(param->pbap_conn_cmpl.bd_addr, prof))
//                {
//                    linkback_active_node_step_suc_adjust_remain_profs();
//                    linkback_run_ext();
//                }
//            }
        }
        break;

    case BT_EVENT_PBAP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_PBAP_DISCONN_CMPL:
        {
//            uint32_t prof;
//            prof = PBAP_PROFILE_MASK;

//            b2s_connected_del_prof_ext(param->pbap_disconn_cmpl.bd_addr, prof);
//            if (b2s_connected_no_profs_ext(param->pbap_disconn_cmpl.bd_addr))
//            {
//                p_link = app_link_find_br_link(param->pbap_disconn_cmpl.bd_addr);
//                if (p_link != NULL)
//                {
//                    if (p_link->disconn_acl_flg)
//                    {
//                        p_link->disconn_acl_flg = false;
//                        legacy_send_acl_disconn_req(param->pbap_disconn_cmpl.bd_addr);
//                    }
//                }
//            }
        }
        break;

    case BT_EVENT_PBAP_CONN_FAIL:
        {

        }
        break;

#if (F_APP_BT_HID_HOST_SUPPORT | F_APP_MULTI_2EP_1_BR_1_BLE_HID)
    case BT_EVENT_HID_HOST_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;
            uint32_t prof;
            p_link = app_link_find_br_link(param->hid_host_conn_cmpl.bd_addr);

            if (p_link)
            {
                prof = HID_PROFILE_MASK;
                b2s_connected_add_prof(param->hid_host_conn_cmpl.bd_addr, prof);
                if (linkback_active_node_judge_cur_conn_prof(param->hid_host_conn_cmpl.bd_addr, prof))
                {
                    linkback_active_node_step_suc_adjust_remain_profs();
                    linkback_run();
                }

            }
        }
        break;

    case BT_EVENT_HID_HOST_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_HID_HOST_DISCONN_CMPL:
        {
            uint32_t prof;
            prof = HID_PROFILE_MASK;
            b2s_connected_del_prof(param->hid_host_disconn_cmpl.bd_addr, prof);
            if (b2s_connected_no_profs(param->hid_host_disconn_cmpl.bd_addr))
            {
                p_link = app_link_find_br_link(param->hid_host_disconn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->acl.disconnected)
                    {
                        p_link->acl.disconnected = false;
                        gap_br_send_acl_disconn_req(param->hid_host_disconn_cmpl.bd_addr);
                    }
                }
            }
        }
        break;

    case BT_EVENT_HID_HOST_CONN_FAIL:
        {

        }
        break;
#endif

#if F_APP_BT_HID_DEVICE_SUPPORT
    case BT_EVENT_HID_DEVICE_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;
            uint32_t prof;
            p_link = app_link_find_br_link(param->hid_device_conn_cmpl.bd_addr);

            if (p_link)
            {
                prof = HID_PROFILE_MASK;
                b2s_connected_add_prof(param->hid_device_conn_cmpl.bd_addr, prof);
                if (linkback_active_node_judge_cur_conn_prof(param->hid_device_conn_cmpl.bd_addr, prof))
                {
                    linkback_active_node_step_suc_adjust_remain_profs();
                    linkback_run();
                }

            }
        }
        break;

    case BT_EVENT_HID_DEVICE_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_HID_DEVICE_DISCONN_CMPL:
        {
            uint32_t prof;
            prof = HID_PROFILE_MASK;
            b2s_connected_del_prof(param->hid_device_disconn_cmpl.bd_addr, prof);
            if (b2s_connected_no_profs(param->hid_device_disconn_cmpl.bd_addr))
            {
                p_link = app_link_find_br_link(param->hid_device_disconn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->acl.disconnected)
                    {
                        p_link->acl.disconnected = false;
                        gap_br_send_acl_disconn_req(param->hid_device_disconn_cmpl.bd_addr);
                    }
                }
            }
        }
        break;

    case BT_EVENT_HID_DEVICE_CONN_FAIL:
        {

        }
        break;
#endif

    case BT_EVENT_A2DP_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->a2dp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                bt_param.bd_addr = param->a2dp_conn_cmpl.bd_addr;
                bt_param.prof = A2DP_PROFILE_MASK;

                b2s_connected_add_prof(bt_param.bd_addr, bt_param.prof);
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_CONNECTED);
            }
            if (linkback_active_node_judge_cur_conn_prof(bt_param.bd_addr, bt_param.prof))
            {
                linkback_active_node_step_suc_adjust_remain_profs();
                linkback_run();
            }

            a2dp_link_count++;
            if (a2dp_link_count == hfp_link_count && hfp_link_count && timer_active)
            {
                is_later_a2dp_connected = true;
                app_stop_timer(&timer_idx_later_a2dp);
            }
#if F_APP_INTEGRATED_TRANSCEIVER
            bt_link_qos_set(param->a2dp_conn_cmpl.bd_addr, BT_QOS_TYPE_GUARANTEED,
                            tpoll_table[BP_TPOLL_A2DP].tpoll_value);
#endif

        }
        break;

    case BT_EVENT_A2DP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            uint32_t prof = A2DP_PROFILE_MASK;
            b2s_connected_del_prof(param->a2dp_disconn_cmpl.bd_addr, prof);
            if (b2s_connected_no_profs(param->a2dp_disconn_cmpl.bd_addr))
            {
                p_link = app_link_find_br_link(param->a2dp_disconn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->acl.disconnected)
                    {
                        p_link->acl.disconnected = false;
                        gap_br_send_acl_disconn_req(param->a2dp_disconn_cmpl.bd_addr);
                    }
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_DISC);
                }
            }
            is_later_a2dp_connected = false;
            a2dp_link_count--;
        }
        break;

    case BT_EVENT_A2DP_CONN_FAIL:
        {

        }
        break;

    case BT_EVENT_AVRCP_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;
            uint32_t prof = AVRCP_PROFILE_MASK;;
            p_link = app_link_find_br_link(param->avrcp_conn_cmpl.bd_addr);

            if (p_link)
            {
                b2s_connected_add_prof(param->avrcp_conn_cmpl.bd_addr, prof);
                if (linkback_active_node_judge_cur_conn_prof(param->avrcp_conn_cmpl.bd_addr, prof))
                {
                    linkback_active_node_step_suc_adjust_remain_profs();
                    linkback_run();
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_AVRCP_DISCONN_CMPL:
        {
            uint32_t prof = AVRCP_PROFILE_MASK;
            b2s_connected_del_prof(param->avrcp_disconn_cmpl.bd_addr, prof);
            if (b2s_connected_no_profs(param->avrcp_disconn_cmpl.bd_addr))
            {
                p_link = app_link_find_br_link(param->avrcp_disconn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->acl.disconnected)
                    {
                        p_link->acl.disconnected = false;
                        gap_br_send_acl_disconn_req(param->avrcp_disconn_cmpl.bd_addr);
                    }
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_CONN_FAIL:
        {

        }
        break;

    case BT_EVENT_SPP_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;
            uint32_t prof;
            p_link = app_link_find_br_link(param->spp_conn_cmpl.bd_addr);

            if (p_link)
            {
                prof = SPP_PROFILE_MASK;
                b2s_connected_add_prof(param->spp_conn_cmpl.bd_addr, prof);
                if (linkback_active_node_judge_cur_conn_prof(param->spp_conn_cmpl.bd_addr, prof))
                {
                    linkback_active_node_step_suc_adjust_remain_profs();
                    linkback_run();
                }
            }
        }
        break;

    case BT_EVENT_SPP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            uint32_t prof;
            prof = SPP_PROFILE_MASK;

            b2s_connected_del_prof(param->spp_disconn_cmpl.bd_addr, prof);
            if (b2s_connected_no_profs(param->spp_disconn_cmpl.bd_addr))
            {
                p_link = app_link_find_br_link(param->spp_disconn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->acl.disconnected)
                    {
                        p_link->acl.disconnected = false;
                        gap_br_send_acl_disconn_req(param->spp_disconn_cmpl.bd_addr);
                    }
                }
            }
        }
        break;

    case BT_EVENT_SPP_CONN_FAIL:
        {

        }
        break;

    case BT_EVENT_IAP_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;
            uint32_t prof;
            p_link = app_link_find_br_link(param->iap_conn_cmpl.bd_addr);

            if (p_link)
            {
                prof = IAP_PROFILE_MASK;
                b2s_connected_add_prof(param->iap_conn_cmpl.bd_addr, prof);
                if (linkback_active_node_judge_cur_conn_prof(param->iap_conn_cmpl.bd_addr, prof))
                {
                    linkback_active_node_step_suc_adjust_remain_profs();
                    linkback_run();
                }
            }
        }
        break;

    case BT_EVENT_IAP_SNIFFING_DISCONN_CMPL:
    case BT_EVENT_IAP_DISCONN_CMPL:
        {
            uint32_t prof;
            prof = IAP_PROFILE_MASK;

            b2s_connected_del_prof(param->iap_disconn_cmpl.bd_addr, prof);
            if (b2s_connected_no_profs(param->iap_disconn_cmpl.bd_addr))
            {
                p_link = app_link_find_br_link(param->iap_disconn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->acl.disconnected)
                    {
                        p_link->acl.disconnected = false;
                        gap_br_send_acl_disconn_req(param->iap_disconn_cmpl.bd_addr);
                    }
                }
            }
        }
        break;

    case BT_EVENT_LINK_QOS_SET_CMPL:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->link_qos_set_cmpl.bd_addr);
            uint8_t tpoll_temp = 0;
            //reset need two condition:
            //1. The worked tpoll in controller is different value requested by host
            //2. Retry count is beyond limite number
            if (p_link)
            {
                if (param->link_qos_set_cmpl.cause)
                {
                    if (p_link->tpoll.status == BP_TPOLL_IDLE)
                    {
                        tpoll_temp = tpoll_table[BP_TPOLL_IDLE].tpoll_value + (p_link->id * 2);
                    }
                    else
                    {
                        tpoll_temp = tpoll_table[p_link->tpoll.status].tpoll_value;
                    }

                    APP_PRINT_ERROR4("set qoe failed: cause %d, real tpoll %d, req tpoll %d, reset num %d",
                                     param->link_qos_set_cmpl.cause, param->link_qos_set_cmpl.tpoll, tpoll_temp,
                                     p_link->tpoll.reset_num);

                    if (param->link_qos_set_cmpl.tpoll != tpoll_temp &&
                        (p_link->tpoll.reset_num < TPOLL_RESET_MAX_NUM))
                    {
                        bt_link_qos_set(param->link_qos_set_cmpl.bd_addr, BT_QOS_TYPE_GUARANTEED,
                                        tpoll_temp);
                        p_link->tpoll.reset_num++;
                    }
                    else
                    {
                        p_link->tpoll.reset_num = 0;
                    }
                }
                else
                {
                    p_link->tpoll.reset_num = 0;
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_bt_policy_device_event_cback(uint32_t event, void *msg)
{
    switch (event)
    {
    case APP_DEVICE_IPC_EVT_STACK_READY:
        {
            app_bt_update_pair_idx_mapping();
        }
        break;

    default:
        break;
    }
}

void app_bt_stop_a2dp_and_sco(void)
{
    T_APP_BR_LINK *p_link = NULL;

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_link_check_b2s_link_by_id(i))
        {
            p_link = &app_db.br_link[i];

            if (p_link)
            {
                if (p_link->a2dp.track_handle)
                {
                    audio_track_stop(p_link->a2dp.track_handle);
                }

                if (p_link->sco.track_handle)
                {
                    audio_track_stop(p_link->sco.track_handle);
                }
            }
        }
    }
}

void app_bt_policy_init(void)
{

    bt_mgr_cback_register(app_bt_policy_cback);
    app_timer_reg_cb(timer_cback, &bt_policy_timer_id);
    app_ipc_subscribe(APP_DEVICE_IPC_TOPIC, app_bt_policy_device_event_cback);

}
