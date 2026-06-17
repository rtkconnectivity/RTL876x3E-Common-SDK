/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include "app_lea_ini_conn_mgr.h"
#include "app_main.h"
#include "trace.h"
#include "app_link_util_cs.h"
#include "gap_conn_le.h"
#include "gap.h"
#include <stdlib.h>
#include "string.h"
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_bond_manager.h"
#include "app_tri_dongle_mgr.h"
#include "app_tri_dongle_uac.h"
#include "app_src_play_a2dp.h"
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
#include "app_bond.h"
#endif
#endif

#if F_APP_FIX_RAPOO_M300G_IOP_ISSUE
#define MAX_LE_CONN_UPDATE_COUNT 10
uint8_t rapoo_m300g_retry_count = 0;
uint8_t app_tri_dongle_get_rapoo_m300g_status(void)
{
    return rapoo_m300g_retry_count;
}

void app_tri_dongle_set_rapoo_m300g_status(bool evt)
{
    APP_PRINT_TRACE2("app_tri_dongle_set_rapoo_m300g_status %d,%d", rapoo_m300g_retry_count, evt);
    if (evt)
    {
        rapoo_m300g_retry_count = rapoo_m300g_retry_count + 1;
    }
    else
    {
        rapoo_m300g_retry_count = 0;
    }
}
#endif

T_GAP_CAUSE app_lea_ini_create_conn(uint8_t *p_bd_addr, uint8_t addr_type)
{
    T_GAP_CAUSE gap_cause;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    T_GAP_LOCAL_ADDR_TYPE own_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    uint8_t  init_phys = GAP_PHYS_CONN_INIT_2M_BIT | GAP_PHYS_CONN_INIT_1M_BIT;
    conn_req_param.scan_interval = 0x20;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = BLE_CONN_NORMAL_CI_DEF;
    conn_req_param.conn_interval_max = BLE_CONN_NORMAL_CI_DEF;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 500;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
    le_set_conn_param(GAP_CONN_PARAM_2M, &conn_req_param);

    gap_cause = le_connect(init_phys, p_bd_addr, (T_GAP_REMOTE_ADDR_TYPE)addr_type,
                           own_addr_type, 1000);
    return gap_cause;
}

void app_lea_ini_add_conn_dev(uint8_t addr_type, uint8_t *bd_addr)
{
    T_GAP_DEV_STATE dev_state;
    T_CONN_DEV_INFO *p_conn_dev;
    if (le_get_gap_param(GAP_PARAM_DEV_STATE, &dev_state) == GAP_CAUSE_SUCCESS)
    {
        if (dev_state.gap_conn_state == GAP_CONN_DEV_STATE_IDLE)
        {
            if (app_lea_ini_create_conn(bd_addr, addr_type) == GAP_CAUSE_SUCCESS)
            {
                return;
            }
        }
    }
    p_conn_dev = calloc(1, sizeof(T_CONN_DEV_INFO));
    if (p_conn_dev)
    {
        p_conn_dev->addr_type = addr_type;
        memcpy(p_conn_dev->bd_addr, bd_addr, 6);
        os_queue_in(&app_db.conn_dev_queue, p_conn_dev);
    }
}

void app_lea_ini_handle_connected(uint8_t conn_id)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(conn_id);
    T_GAP_CONN_INFO conn_info;
    if (p_link == NULL)
    {
        return;
    }
    if (!le_get_conn_info(conn_id, &conn_info))
    {
        APP_PRINT_ERROR1("app_lea_ini_handle_connected: can't find conn info %d", conn_id);
        return;
    }
    p_link->role = conn_info.role;
    uint16_t conn_interval;
    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, p_link->conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &p_link->conn_param.latency, p_link->conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &p_link->conn_param.timeout, p_link->conn_id);

    APP_PRINT_INFO2("app_lea_ini_handle_connected conn_id %d interval %x latency %x timeout %x",
                    conn_id,
                    conn_interval);
    p_link->conn_param.ci_min = conn_interval;
    p_link->conn_param.ci_max = conn_interval;
    if (p_link->conn_param.ce_2slot_used)
    {
        p_link->conn_param.ce_2slot_used = false;
        p_link->conn_param.ce_min = CONN_CE_MIN;
        p_link->conn_param.ce_max = CONN_CE_MIN;
    }
    else
    {
        p_link->conn_param.ce_min = 2 * (conn_interval - 1);
        p_link->conn_param.ce_max = 2 * (conn_interval - 1);
    }
}

void app_lea_ini_handle_disconnected(uint8_t conn_id)
{
    APP_PRINT_INFO1("app_lea_ini_handle_disconnected: conn_id %d",
                    conn_id);
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(conn_id);
    if (p_link == NULL)
    {
        return;
    }
    p_link->role = GAP_LINK_ROLE_UNDEFINED;
    memset((uint8_t *)&p_link->conn_param, 0, sizeof(T_LEA_CONN_PARAM));
}

static bool app_ini_conn_pend_params_need_update(T_APP_LE_LINK *p_link)
{
    if (!p_link)
    {
        return false;
    }

    T_LEA_CONN_PARAM *p_conn = &(p_link->conn_param);
    if (!p_conn)
    {
        return false;
    }
    if (!p_conn->param_update_pend)
    {
        return false;
    }

    APP_PRINT_INFO6("app_ini_conn_pend_params_need_update: CURRENT: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
                    p_conn->ci_min,
                    p_conn->ci_max,
                    p_conn->latency,
                    p_conn->timeout,
                    p_conn->ce_min,
                    p_conn->ce_max);

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    if ((p_conn->ci_min_pend == 0x06 && p_conn->ci_max_pend == 0x06) ||
        (p_conn->ci_min_pend == 0x08 && p_conn->ci_max_pend == 0x08) ||
        (p_conn->ci_min_pend == 0x0c && p_conn->ci_max_pend == 0x0c))
    {
        return true;
    }
#endif

    if ((p_conn->ci_min == p_conn->ci_min_pend) &&
        (p_conn->ci_max == p_conn->ci_max_pend) &&
        (p_conn->latency == p_conn->latency_pend) &&
        (p_conn->timeout == p_conn->timeout_pend) &&
        (p_conn->ce_min == p_conn->ce_min_pend) &&
        (p_conn->ce_max == p_conn->ce_max_pend))
    {
        return false;
    }

    return true;

}

static void app_ini_start_conn_update(T_APP_LE_LINK *p_link)
{
    bool ret = false;
    T_GAP_CAUSE cause;
    if (!p_link)
    {
        return;
    }

    T_LEA_CONN_PARAM *p_conn = &(p_link->conn_param);
    if (!p_conn)
    {
        return;
    }
    APP_PRINT_INFO2("app_ini_start_conn_update id %d, mask 0x%x", p_link->conn_id,
                    p_conn->conn_mask);

    if ((p_conn->conn_mask & BLE_UPDATE_WAIT_RSP) ||
        (p_conn->conn_mask & BLE_UPDATE_DISABLE))
    {
        return;
    }

    ret = app_ini_conn_pend_params_need_update(p_link);
    if (ret == false)
    {
        APP_PRINT_INFO0("app_ini_start_conn_update: no need update");
        return;
    }

    p_conn->param_update_pend = false;

    cause = le_update_conn_param(p_link->conn_id,
                                 p_conn->ci_min_pend, p_conn->ci_max_pend,
                                 p_conn->latency_pend, p_conn->timeout_pend,
                                 p_conn->ce_min_pend, p_conn->ce_max_pend);
    if (cause == GAP_CAUSE_SUCCESS)
    {
        if (p_link->role == GAP_LINK_ROLE_MASTER)
        {
            p_conn->conn_mask |= BLE_UPDATE_WAIT_RSP;
        }
        if ((p_conn->ce_min_pend == CONN_CE_MIN) && (p_conn->ce_max_pend == CONN_CE_MIN))
        {
            p_conn->ce_2slot_used = true;
        }
    }
    else
    {
        APP_PRINT_ERROR1("app_ini_start_conn_update update default param fail cause 0x%x", cause);
        return;
    }
}

static bool app_ini_check_conn_params(uint16_t ci_min, uint16_t ci_max,
                                      uint16_t latency, uint16_t timeout,
                                      uint16_t ce_min, uint16_t ce_max)
{
    if (ci_min < BLE_CONN_INT_MIN || ci_min > BLE_CONN_INT_MAX)
    {
        return false;
    }

    if (ci_max < BLE_CONN_INT_MIN || ci_max > BLE_CONN_INT_MAX)
    {
        return false;
    }

    if (ci_min > ci_max)
    {
        return false;
    }

    if (latency > BLE_CONN_LATENCY_MAX)
    {
        return false;
    }

    if (timeout < BLE_CONN_SUP_TOUT_MIN || timeout > BLE_CONN_SUP_TOUT_MAX)
    {
        return false;
    }

    if (ce_min > ce_max)
    {
        return false;
    }

    return true;
}

static bool app_ini_update_conn_params(uint8_t *bd_addr, uint16_t ci_min, uint16_t ci_max,
                                       uint16_t latency, uint16_t timeout, uint16_t ce_min, uint16_t ce_max)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }
    T_LEA_CONN_PARAM *p_conn = &(p_link->conn_param);
    if (!p_conn)
    {
        APP_PRINT_ERROR0("app_ini_update_conn_params: conn_param NULL");
        return false;
    }
    APP_PRINT_TRACE7("app_ini_update_conn_params conn_id %d ci_min 0x%x ci_max 0x%x latency 0x%x timeout 0x%x, ce_min 0x%x,ce_max 0x%x",
                     p_link->conn_id, ci_min, ci_max, latency, timeout, ce_min, ce_max);

    if (!app_ini_check_conn_params(ci_min, ci_max, latency, timeout, ce_min, ce_max))
    {
        APP_PRINT_ERROR0("app_ini_update_conn_params: check conn_param fail");
        return false;
    }

    p_conn->ci_min_pend = ci_min;
    p_conn->ci_max_pend = ci_max;
    p_conn->latency_pend = latency;
    p_conn->timeout_pend = timeout;
    p_conn->ce_min_pend = ce_min;
    p_conn->ce_max_pend = ce_max;

    p_conn->param_update_pend = true;

    app_ini_start_conn_update(p_link);
    return true;
}

bool app_ini_enable_conn_update(uint8_t *addr)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_addr(addr);
    if (p_link == NULL)
    {
        return false;
    }
    APP_PRINT_INFO2("app_ini_enable_conn_update conn_id %d conn_mask 0x%x",
                    p_link->conn_id, p_link->conn_param.conn_mask);
    if ((p_link->conn_param.conn_mask & BLE_UPDATE_DISABLE) == 0)
    {
        return true;
    }
    p_link->conn_param.conn_mask &= ~BLE_UPDATE_DISABLE;

    app_ini_start_conn_update(p_link);

    return true;
}

bool app_ini_disable_conn_update(uint8_t *addr)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_addr(addr);
    if (p_link == NULL)
    {
        return false;
    }
    APP_PRINT_INFO2("app_ini_disable_conn_update conn_id %d conn_mask 0x%x",
                    p_link->conn_id, p_link->conn_param.conn_mask);
    if (p_link->conn_param.conn_mask & BLE_UPDATE_DISABLE)
    {
        return true;
    }
    p_link->conn_param.conn_mask |= BLE_UPDATE_DISABLE;

    app_ini_start_conn_update(p_link);

    return true;
}

bool app_lea_ini_handle_dev_state_change_event(uint8_t *bd_addr, T_UPDATE_CONN_PARAM_EVENT event)
{
    uint16_t ci_min, ci_max, ce_min, ce_max;
    uint16_t timeout = BLE_CONN_TIMEOUT_DEF;
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_addr(bd_addr);

    if (p_link == NULL)
    {
        return false;
    }
    APP_PRINT_INFO2("app_lea_ini_handle_dev_state_change_event: conn_id %d, event 0x%x",
                    p_link->conn_id, event);
    switch (event)
    {
    case CP_EVENT_GATT_DISCOVERY:
        {
            ci_min = BLE_CONN_FAST_CI_DEF;
            ci_max = BLE_CONN_FAST_CI_DEF;
            ce_min = 2 * (ci_min - 1);
            ce_max = 2 * (ci_max - 1);
        }
        break;
    case CP_EVENT_GATT_DONE:
        {
            ci_min = BLE_CONN_NORMAL_CI_DEF;
            ci_max = BLE_CONN_NORMAL_CI_DEF;
            ce_min = 2 * (ci_min - 1);
            ce_max = 2 * (ci_max - 1);
        }
        break;
    case CP_EVENT_DISCV_ALL_DONE:
    case CP_EVENT_CIS_DISCONNECT:
        {
            ci_min = BLE_CONN_NORMAL_CI_DEF;
            ci_max = BLE_CONN_NORMAL_CI_DEF;
            ce_min = 2 * (ci_min - 1);
            ce_max = 2 * (ci_max - 1);
        }
        break;
    case CP_EVENT_CIS_ESTABLISH:
        {
            ci_min = BLE_CONN_NORMAL_CI_DEF;
            ci_max = BLE_CONN_NORMAL_CI_DEF;
            ce_min = CONN_CE_MIN;
            ce_max = CONN_CE_MIN;
        }
        break;

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    case CP_EVENT_SCO_ENABLE:
        {
            ci_min = 12;
            ci_max = 12;
            ce_min = 0x0002;
            ce_max = 0x0002;
            p_link->conn_param.conn_mask &= ~BLE_UPDATE_DISABLE;
        }
        break;

    case CP_EVENT_SCO_END_ENABLE:
        {
            ci_min = 6;
            ci_max = 6;
            ce_min = 0x0002;
            ce_max = 0x0002;
            timeout = BLE_CONN_TIMEOUT_SCO_END_DEF;
            p_link->conn_param.conn_mask &= ~BLE_UPDATE_DISABLE;
        }
        break;

    case CP_EVENT_BIS_ITV_10_ENABLE:
        {
            ci_min = 8;
            ci_max = 8;
            ce_min = 2 * (ci_min - 1);
            ce_max = 2 * (ci_max - 1);
            p_link->conn_param.conn_mask &= ~BLE_UPDATE_DISABLE;
        }
        break;
#endif

    default:
        return false;
    }

    return app_ini_update_conn_params(p_link->bd_addr, ci_min, ci_max,
                                      BLE_CONN_SLAVE_LATENCY_DEF, timeout,
                                      ce_min, ce_max);
}

T_APP_RESULT app_lea_ini_handle_conn_update_ind(T_LE_CONN_UPDATE_IND *p_conn_ind)
{
    uint16_t ci_min, ci_max, latency, timeout, ce_min, ce_max;
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(p_conn_ind->conn_id);
    if (p_link == NULL)
    {
        return APP_RESULT_REJECT;
    }

    T_LEA_CONN_PARAM *p_conn = &(p_link->conn_param);
    if (!p_conn)
    {
        APP_PRINT_ERROR0("app_lea_ini_handle_conn_update_ind: conn_param NULL");
        return APP_RESULT_REJECT;
    }

    APP_PRINT_TRACE5("app_lea_ini_handle_conn_update_ind: conn_id 0x%x, min 0x%x, max 0x%x latency 0x%x timeout 0x%x",
                     p_conn_ind->conn_id, p_conn_ind->conn_interval_min, p_conn_ind->conn_interval_max,
                     p_conn_ind->conn_latency, p_conn_ind->supervision_timeout);

    ci_min = p_conn_ind->conn_interval_min;
    ci_max = p_conn_ind->conn_interval_max;
    latency = p_conn_ind->conn_latency;
    timeout = p_conn_ind->supervision_timeout;
    ce_min = p_conn->ce_min;
    ce_max = p_conn->ce_max;

    APP_PRINT_TRACE2("app_lea_ini_handle_conn_update_ind, mask %x role %x",
                     p_link->conn_param.conn_mask,
                     p_link->role);
    if (p_link->role != GAP_LINK_ROLE_MASTER)
    {
        return APP_RESULT_REJECT;
    }
    if (!app_ini_check_conn_params(ci_min, ci_max, latency, timeout, ce_min, ce_max))
    {
        return APP_RESULT_REJECT;
    }

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    if (app_tri_dongle_get_check_rcu_status())
    {
        APP_PRINT_TRACE0("app_tri_dongle_rcu_skip_app_reject");
    }
    else
    {
#if F_APP_FIX_RAPOO_M300G_IOP_ISSUE
        if (app_tri_dongle_get_rapoo_m300g_status() > MAX_LE_CONN_UPDATE_COUNT)
        {
            p_link->conn_param.conn_mask &= ~BLE_UPDATE_DISABLE;
            app_tri_dongle_set_rapoo_m300g_status(false);
        }
        else
#endif
        {
            if ((app_tri_dongle_uac_bis_src_enable() && //or ble for lea exist
                 (p_conn_ind->conn_interval_min != 0x08 || p_conn_ind->conn_interval_max != 0x08)) ||
                ((!app_tri_dongle_uac_bis_src_enable()) &&
                 (p_conn_ind->conn_interval_min != 0x06 || p_conn_ind->conn_interval_max != 0x06)))
            {
#if F_APP_FIX_RAPOO_M300G_IOP_ISSUE
                app_tri_dongle_set_rapoo_m300g_status(true);
#endif
                return APP_RESULT_REJECT;
            }
            if ((app_tri_dongle_uac_get_sco_state() != APP_TRI_DONGLE_SCO_IDLE) ||
                (app_src_play_get_a2dp_state(0) == A2DP_STATE_STREAM_START) ||
                (app_src_play_get_a2dp_state(0) == A2DP_STATE_STREAM_START_RSP))
            {
                if (p_conn_ind->conn_interval_min != 0x0c || p_conn_ind->conn_interval_max != 0x0c)
                {
#if F_APP_FIX_RAPOO_M300G_IOP_ISSUE
                    app_tri_dongle_set_rapoo_m300g_status(true);
#endif
                    return APP_RESULT_REJECT;
                }
            }
            if (p_conn_ind->conn_latency != 0x00)
            {
#if F_APP_FIX_RAPOO_M300G_IOP_ISSUE
                app_tri_dongle_set_rapoo_m300g_status(true);
#endif
                return APP_RESULT_REJECT;
            }
        }
    }
#endif

    p_conn->ci_min_pend = ci_min;
    p_conn->ci_max_pend = ci_max;
    p_conn->latency_pend = latency;
    p_conn->timeout_pend = timeout;
    p_conn->ce_min_pend = ce_min;
    p_conn->ce_max_pend = ce_max;

    p_conn->param_update_pend = true;

    //FIX TODO reject here but still record param, update when next time enable update
    if ((p_conn->conn_mask & BLE_UPDATE_DISABLE) ||
        (p_conn->conn_mask & BLE_UPDATE_WAIT_RSP))
    {
        return APP_RESULT_REJECT;
    }
    else
    {
        app_ini_start_conn_update(p_link);
        return APP_RESULT_SUCCESS;
    }
}


void app_lea_ini_handle_conn_update_status(T_GAP_CONN_PARAM_UPDATE update_info)
{
    APP_PRINT_TRACE3("app_lea_ini_handle_conn_update_status: conn_id %d, status %d, cause 0x%04x",
                     update_info.conn_id, update_info.status, update_info.cause);
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(update_info.conn_id);
    if (p_link == NULL)
    {
        return;
    }

    T_LEA_CONN_PARAM *p_conn = &(p_link->conn_param);
    if (!p_conn)
    {
        APP_PRINT_ERROR0("app_lea_ini_handle_conn_update_status: conn_param NULL");
        return;
    }

    switch (update_info.status)
    {
    case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS:
        {
            /*Update conn param information*/
            uint16_t conn_interval;

            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, update_info.conn_id);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &p_conn->latency, update_info.conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &p_conn->timeout, update_info.conn_id);

            p_conn->ci_min = conn_interval;
            p_conn->ci_max = conn_interval;

            if (p_conn->ce_2slot_used)
            {
                p_conn->ce_2slot_used = false;
                p_conn->ce_min = CONN_CE_MIN;
                p_conn->ce_max = CONN_CE_MIN;
            }
            else
            {
                p_conn->ce_min = 2 * (conn_interval - 1);
                p_conn->ce_max = 2 * (conn_interval - 1);
            }

            /*FIXME: where to get ce param*/
            APP_PRINT_INFO4("app_lea_ini_handle_conn_update_status: update success:conn_id 0x%x, conn_interval 0x%04x, conn_slave_latency 0x%04x, conn_supervision_timeout 0x%04x",
                            update_info.conn_id, conn_interval, p_link->conn_param.latency,
                            p_link->conn_param.timeout);
            p_conn->conn_mask &= ~BLE_UPDATE_WAIT_RSP;
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            struct
            {
                uint16_t conn_handle;
                uint16_t conn_interval;

            } data = {0};

            data.conn_handle = p_link->conn_handle;
            data.conn_interval = conn_interval;
            app_tri_dongle_if_ble_evt(REDEV_MSG_BLE_CONN_INTERVAL_SUCCESS_REPORT, &data,
                                      NULL);
#if F_APP_FIX_RAPOO_M300G_IOP_ISSUE
            p_link->conn_param.conn_mask |= BLE_UPDATE_DISABLE;
#endif
            p_link->ci_update_cmpl = app_usb_uac_ci_update_check(conn_interval);
            app_tri_dongle_uac_ci_step_process();
#endif
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            p_conn->conn_mask &= ~BLE_UPDATE_WAIT_RSP;
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
        break;
    default:
        break;
    }

    app_ini_start_conn_update(p_link);
}
#endif
