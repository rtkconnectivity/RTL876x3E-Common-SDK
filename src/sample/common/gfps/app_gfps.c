/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_GFPS_COMMON_BASIC_FEATURE_SUPPORT
#include "stdlib.h"
#include "trace.h"
#include "gap_br.h"
#include "gap_bond_le.h"
#include "bt_gfps.h"
#include "gfps.h"
#include "ecc_enhanced.h"
#include "app_ecc.h"
#include "app_gfps_timer.h"
#include "ble_ext_adv.h"
#include "ble_conn.h"
#include "app_gfps_cfg.h"
#include "app_gfps.h"
#include "app_gfps_account_key.h"
#include "app_gfps_personalized_name.h"

#include "app_ble_gap.h"
#include "app_main.h"
#include "app_bt_policy_api.h"
#include "app_hfp.h"
#include "dis.h"
#if F_GFPS_COMMON_BATTERY_INFO_REPORT
#include "app_gfps_battery.h"
#endif
#include "app_gfps_msg.h"
#include "app_multilink.h"
#include "app_gfps_device.h"
#include "app_bond.h"
#include "app_gfps_sync.h"
#include "app_gfps_link.h"
#include "app_audio_policy.h"
#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
#include "bt_bond_le.h"
#if F_GFPS_COMMON_LE_L2CAP_CHANNEL
#include "app_gfps_psm.h"
#endif
#endif
#if F_GFPS_COMMON_FINDER_SUPPORT
#include "app_dult.h"
#include "app_dult_device.h"
#include "gfps_find_my_device.h"
#include "app_gfps_finder.h"
#endif
#if CONFIG_REALTEK_GFPS_SASS_SUPPORT
#include "app_sass_policy.h"
#include "gfps_sass_conn_status.h"
#endif
#if F_GFPS_COMMON_PERIODIC_WAKEUP
#include "pm.h"
#include "app_cfg.h"
#include "app_dlps.h"
#include "rtl876x_rtc.h"
#include "hal_adp.h"
#endif
#include "gap_bond_le.h"


T_GFPS_DB gfps_db;
uint8_t gfps_adv_len;
uint8_t gfps_adv_data[GAP_MAX_LEGACY_ADV_LEN];

void app_gfps_ble_conn_info_init(uint8_t conn_id);
static T_APP_RESULT app_gfps_cb(T_SERVER_ID service_id, void *p_data);
static void app_gfps_le_disconnect_cb(uint8_t conn_id, uint8_t local_disc_cause,
                                      uint16_t disc_cause);

#define GATT_UUID_CAS        0x1853
static uint8_t gfps_scan_rsp_data[] =
{
    0x04,/* length */
    GAP_ADTYPE_SERVICE_DATA,/* type="service uuid" */
    LO_WORD(GATT_UUID_CAS),
    HI_WORD(GATT_UUID_CAS),
    0x00,
};
static uint8_t gfps_get_scan_rsp_data_len(void)
{
    uint8_t gfps_scan_rsp_data_len = 0;

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        T_GFPS_LE_DEVICE_MODE gfps_le_device_mode = gfps_le_get_device_mode();

        if ((gfps_le_device_mode == GFPS_LE_DEVICE_MODE_LE_MODE_WITH_LEA ||
             gfps_le_device_mode == GFPS_LE_DEVICE_MODE_DUAL_MODE_WITH_LEA))
        {
            gfps_scan_rsp_data_len = sizeof(gfps_scan_rsp_data);
        }
    }
#endif
    return gfps_scan_rsp_data_len;
}

void app_gfps_update_announcement(uint8_t announcement)
{
    if (!extend_app_cfg_const.gfps_le_device_support)
    {
        return;
    }

    APP_PRINT_TRACE1("app_gfps_update_announcement: %d", announcement);
    gfps_scan_rsp_data[4] = announcement;
    ble_ext_adv_mgr_set_scan_response_data(gfps_db.gfps_adv_handle, sizeof(gfps_scan_rsp_data),
                                           gfps_scan_rsp_data);
}

/*google Fast pair initialize*/
void app_gfps_init(void)
{
    APP_PRINT_TRACE0("app_gfps_init: start");

    uint8_t sec_req_enable = false;
    bool is_tag = extend_app_cfg_const.gfps_device_type == GFPS_LOCATOR_TRACKER ? true : false;

    if (app_gfps_account_key_init(extend_app_cfg_const.gfps_account_key_num) == false)
    {
        goto error;
    }
#if CONFIG_REALTEK_GFPS_SASS_SUPPORT
    gfps_sass_conn_status_init();
#endif

#if GFPS_PERSONALIZED_NAME_SUPPORT
    app_gfps_personalized_name_init();
#endif

    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &sec_req_enable);
    if (gfps_init(extend_app_cfg_const.gfps_model_id, extend_app_cfg_const.gfps_public_key,
                  extend_app_cfg_const.gfps_private_key) == false)
    {
        goto error;
    }

#if F_GFPS_COMMON_FINDER_SUPPORT
    gfps_set_finder_enable(extend_app_cfg_const.gfps_finder_support);
#endif

#if CONFIG_REALTEK_GFPS_SASS_SUPPORT
    gfps_set_sass_enable(extend_app_cfg_const.gfps_sass_support);
#endif

    gfps_set_tx_power(extend_app_cfg_const.gfps_enable_tx_power, extend_app_cfg_const.gfps_tx_power);
#if F_GFPS_COMMON_BATTERY_INFO_REPORT
    app_gfps_battery_info_init();
#endif
    gfps_db.current_conn_id = 0xFF;

    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        T_APP_GFPS_LE_LINK *le_link = &app_gfps_link_db.le_link[i];

        le_link->gfps_link.gfps_conn_id = 0xFF;
        le_link->gfps_link.gfps_conn_state = GAP_CONN_STATE_DISCONNECTED;
    }

    gfps_db.gfps_id = gfps_add_service(app_gfps_cb);
#if F_APP_ERWS_SUPPORT
    app_gfps_sync_relay_init();
#endif
    gfps_ecc_manager_malloc();
    app_gfps_link_priority_queue_init();

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
    gfps_le_device_init(extend_app_cfg_const.gfps_le_device_support,
                        extend_app_cfg_const.gfps_le_device_mode, is_tag); // TODO: work around
    if (extend_app_cfg_const.gfps_le_device_support)
    {
#if F_GFPS_COMMON_LE_L2CAP_CHANNEL
        app_gfps_psm_init();
#endif
    }
#endif

    gap_get_param(GAP_PARAM_BOND_IO_CAPABILITIES, &(gfps_db.additional_default_io_cap));
    gap_get_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, &(gfps_db.additional_default_auth_flags));

    app_gfps_timer_init();
    app_gfps_set_allow_write_account_key(true);

    /*Google's specifications recommend setting it to 7.5ms,
     but Realtek's le audio doesn't seem to support it(from wade_li)*/
#if 0
    uint16_t interval_min = 6;//6*1.25ms
    uint16_t interval_max = 6;
    uint16_t latency = 0;
    uint16_t supervision_timeout = 500;

    gaps_set_peripheral_preferred_conn_param(interval_min, interval_max,
                                             latency, supervision_timeout);
#endif
    return;

error:
    APP_PRINT_ERROR0("app_gfps_init: failed");
}

void app_gfps_get_random_addr(uint8_t *random_bd)
{
    memcpy(random_bd, gfps_db.random_address, GAP_BD_ADDR_LEN);
}

bool app_gfps_get_application_stop_adv_state(void)
{
    return gfps_db.app_stop_gfps_adv;
}

/**
 * @brief Stop gfps adv by application
 *  Note: Using this method is not recommended as it may affect the Gfps code logic.
 *  If you think that the existence of gfps adv consumes bandwidth or power,
 *  you can use app_gfps_adv_update_adv_interval() to increase the gfps advertising interval.
 *  Note that you shall remember to change the interval back to its default.
 *
 * If the app strongly insists on stopping broadcasting,
 * remember to call app_gfps_adv_start_by_application() to re-enable broadcasting.
 *
 * @param stop_cause please add cause in  app_adv_stop_cause.h
 * @return T_GAP_CAUSE Result
 */
T_GAP_CAUSE app_gfps_adv_stop_by_application(uint8_t stop_cause)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    gfps_db.app_stop_gfps_adv = true;

    if (gfps_db.gfps_adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
    {
        ret = ble_ext_adv_mgr_disable(gfps_db.gfps_adv_handle, stop_cause);
    }
    APP_PRINT_TRACE2("app_gfps_adv_stop: stop_cause 0x%x, ret 0x%x", stop_cause, ret);
    return ret;
}

/**
 * @brief Start gfps adv by application
 *
 * @return
 */
void app_gfps_adv_start_by_application(void)
{
    gfps_db.app_stop_gfps_adv = false;

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    }
    else
#endif
    {
        app_gfps_device_check_state((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    }
    APP_PRINT_TRACE0("app_gfps_adv_start_by_application");
}

bool app_gfps_adv_start(T_GFPS_ADV_MODE mode, bool show_ui)
{
    app_gfps_timer_stop_update_rpa();
    app_gfps_timer_start_update_rpa();

    uint16_t interval = extend_app_cfg_const.gfps_discov_adv_interval;

    if (mode == NOT_DISCOVERABLE_MODE)
    {
        ble_ext_adv_mgr_change_own_address_type(gfps_db.gfps_adv_handle, GAP_LOCAL_ADDR_LE_RANDOM);
        interval = extend_app_cfg_const.gfps_not_discov_adv_interval;
    }
    else if (mode == DISCOVERABLE_MODE_WITH_MODEL_ID)
    {
        ble_ext_adv_mgr_change_own_address_type(gfps_db.gfps_adv_handle, GAP_LOCAL_ADDR_LE_PUBLIC);
    }

    gfps_gen_adv_data(mode, gfps_adv_data, &gfps_adv_len, show_ui);

    ble_ext_adv_mgr_set_multi_param(gfps_db.gfps_adv_handle, NULL,
                                    interval, gfps_adv_len, gfps_adv_data, 0, NULL);

    if (gfps_db.gfps_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        if (ble_ext_adv_mgr_enable(gfps_db.gfps_adv_handle, 0) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return true;
}

T_GAP_CAUSE app_gfps_adv_update_adv_interval(uint32_t adv_interval)
{
    T_GAP_CAUSE result = GAP_CAUSE_SUCCESS;

    result = ble_ext_adv_mgr_change_adv_interval(gfps_db.gfps_adv_handle, adv_interval);
    APP_PRINT_TRACE2("app_gfps_adv_update_adv_interval: result %d, adv_interval %d",
                     result, adv_interval);
    return result;
}

void app_gfps_get_ble_addr(uint8_t *ble_addr)
{
    if (gfps_db.force_enter_pair_mode)
    {
        gap_get_param(GAP_PARAM_BD_ADDR, ble_addr);
    }
    else
    {
        app_gfps_get_random_addr(ble_addr);
    }

    APP_PRINT_TRACE2("app_gfps_get_ble_addr: %s, force_enter_pair_mode %d",
                     TRACE_BDADDR(ble_addr), gfps_db.force_enter_pair_mode);
}

void app_gfps_update_rpa(bool gfps_generate_rpa)
{
    if (gfps_generate_rpa)
    {
        //generate a valid rpa
        le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, gfps_db.random_address);
        //set this rpa into stack
        ble_ext_adv_mgr_set_random(gfps_db.gfps_adv_handle, gfps_db.random_address);
    }
    else
    {
        //directly get rpa from stack
        ble_ext_adv_get_rpa_by_adv_handle(gfps_db.gfps_adv_handle, gfps_db.random_address);
    }

    //update rpa in gfps through RFCOMM or LE L2CAP channel
    app_gfps_msg_update_rpa_addr();
    APP_PRINT_TRACE1("app_gfps_update_rpa: RPA %s", TRACE_BDADDR(gfps_db.random_address));
}

bool app_gfps_sec_adv_start(T_GFPS_ADV_MODE mode, bool show_ui, uint16_t duration_10ms)
{
    APP_PRINT_TRACE1("app_gfps_sec_adv_start: duration_10ms %d", duration_10ms);
    uint8_t random_address[6] = {0};
    le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, random_address);

    if (gfps_gen_adv_data(mode, gfps_adv_data, &gfps_adv_len, show_ui))
    {
        uint16_t interval = extend_app_cfg_const.gfps_discov_adv_interval;

        memcpy(gfps_db.random_address, random_address, GAP_BD_ADDR_LEN);
        if (mode == DISCOVERABLE_MODE_WITH_MODEL_ID)
        {
            interval = extend_app_cfg_const.gfps_discov_adv_interval;
        }
        else if (mode == NOT_DISCOVERABLE_MODE)
        {
            interval = extend_app_cfg_const.gfps_not_discov_adv_interval;
        }
        ble_ext_adv_mgr_set_multi_param(gfps_db.gfps_adv_handle, random_address,
                                        interval, gfps_adv_len, gfps_adv_data, 0, NULL);
    }
    else
    {
        APP_PRINT_ERROR0("app_gfps_sec_adv_start: gfps_gen_adv_data failed");
        return false;
    }

    if (gfps_db.gfps_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        if (ble_ext_adv_mgr_enable(gfps_db.gfps_adv_handle, duration_10ms) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }
    return true;
}

bool app_gfps_next_action(T_GFPS_ACTION gfps_next_action)
{
    uint8_t link_num = app_gfps_link_find_gfps_link();

    APP_PRINT_TRACE4("app_gfps_next_action: gfps_curr_action %d, gfps_next_action %d, gfps_adv_state %d, link_num %d",
                     gfps_db.gfps_curr_action, gfps_next_action,
                     gfps_db.gfps_adv_state, link_num);

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
#else
    if ((gfps_next_action != GFPS_ACTION_IDLE) && (link_num != 0))
    {
        return false;
    }
#endif

    gfps_db.gfps_curr_action = gfps_next_action;

    switch (gfps_db.gfps_curr_action)
    {
    case GFPS_ACTION_IDLE:
        {
            if (gfps_db.gfps_adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                ble_ext_adv_mgr_disable(gfps_db.gfps_adv_handle, APP_STOP_ADV_CAUSE_GFPS_ACTION_IDLE);
            }
            else if (link_num != 0)
            {
                app_gfps_link_disconnect_all_gfps_link();
            }
        }
        break;

    case GFPS_ACTION_ADV_DISCOVERABLE_MODE_WITH_MODEL_ID:
        {
            app_gfps_adv_start(DISCOVERABLE_MODE_WITH_MODEL_ID, true);
        }
        break;

    case GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE:
        {
            app_gfps_adv_start(NOT_DISCOVERABLE_MODE, true);
#if F_GFPS_COMMON_BATTERY_INFO_REPORT
            app_gfps_battery_info_report(GFPS_BATTERY_REPORT_ADV_START);
#endif
        }
        break;

    case GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                //app_gfps_sec_adv_start(NOT_DISCOVERABLE_MODE, false, 18000);
            }
            else
            {
                app_gfps_adv_start(NOT_DISCOVERABLE_MODE, false);
            }
        }
        break;

    default:
        break;
    }

    return true;
}

bool app_gfps_ble_conn_info_handle(uint8_t *remote_bd_addr, uint8_t remote_addr_type)
{
    bool receive_ble_link = true;

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        uint8_t link_num = app_gfps_link_find_gfps_link();
        bool addr_reslove_ret = false;
        uint8_t resolved_addr[6] = {0};
        uint8_t resolved_bd_type = 0xFF;

        if (link_num >= GFPS_LE_MAX_LINK_NUMBER)
        {
            receive_ble_link = false;
            return receive_ble_link;
        }

        if (remote_addr_type == GAP_REMOTE_ADDR_LE_RANDOM)
        {
            addr_reslove_ret = bt_le_resolve_random_address(remote_bd_addr, resolved_addr, &resolved_bd_type);
        }

        if ((gfps_db.force_enter_pair_mode == false) && (addr_reslove_ret == true))
        {
            T_ACCOUNT_KEY *p_key_info = app_gfps_account_key_get_table();

            for (uint8_t i = 0; i < p_key_info->num; i++)
            {
                if (memcmp(resolved_addr, p_key_info->account_info[i].addr, 6) == 0)
                {
                    gfps_db.gfps_linkback_init = true;
                }
            }
        }
    }
#endif

    APP_PRINT_TRACE1("app_gfps_ble_conn_info_handle: receive_ble_link %d", receive_ble_link);
    return receive_ble_link;
}

static void app_gfps_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));

    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            gfps_db.gfps_adv_state = cb_data.p_ble_state_change->state;

            if (cb_data.p_ble_state_change->state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                gfps_db.gfps_curr_action = GFPS_ACTION_IDLE;
            }

            APP_PRINT_TRACE4("app_gfps_adv_callback: adv_state %d, adv_handle %d, stop_cause %d, app_cause 0x%02x",
                             cb_data.p_ble_state_change->state,
                             cb_data.p_ble_state_change->adv_handle,
                             cb_data.p_ble_state_change->stop_cause,
                             cb_data.p_ble_state_change->app_cause);
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        {
            APP_PRINT_TRACE4("app_gfps_adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x, adv_handle %d, local_addr_type %d, local_bd %s",
                             cb_data.p_ble_conn_info->conn_id,
                             cb_data.p_ble_conn_info->adv_handle,
                             cb_data.p_ble_conn_info->local_addr_type,
                             TRACE_BDADDR(cb_data.p_ble_conn_info->local_addr));

            app_link_reg_le_link_disc_cb(cb_data.p_ble_conn_info->conn_id, app_gfps_le_disconnect_cb);

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
            if (extend_app_cfg_const.gfps_le_device_support)
            {
                if (cb_data.p_ble_conn_info->adv_handle == gfps_db.gfps_adv_handle)
                {
                    app_gfps_ble_conn_info_init(cb_data.p_ble_conn_info->conn_id);
                }
            }
            else
#endif
            {
                gfps_db.current_conn_id = cb_data.p_ble_conn_info->conn_id;
            }

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
            if (extend_app_cfg_const.gfps_le_device_support)
            {
                app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
            }
            else
#endif
            {
                app_gfps_device_check_state((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
            }
        }
        break;

    default:
        break;
    }

    return;
}

void app_gfps_ble_conn_info_init(uint8_t conn_id)
{
    T_APP_GFPS_LE_LINK *p_link;
    p_link = app_gfps_link_find_le_link_by_conn_id(conn_id);

    if (p_link == NULL)
    {
        return;
    }

    p_link->gfps_link.gfps_conn_state = GAP_CONN_STATE_CONNECTED;
    p_link->gfps_link.gfps_conn_id = conn_id;
    app_gfps_link_add_link_into_priority_queue(p_link->gfps_link.gfps_conn_id);
    gfps_db.current_conn_id = conn_id;
}

void app_gfps_linkback_info_init(uint8_t conn_id)
{
    if (gfps_db.gfps_linkback_init == true)
    {
        app_gfps_ble_conn_info_init(conn_id);
        gfps_db.gfps_linkback_init = false;

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
        //need to start adv for LE audio multilink reconnect
        if (extend_app_cfg_const.gfps_le_device_support)
        {
            T_ACCOUNT_KEY *p_key_info = app_gfps_account_key_get_table();

            if (p_key_info->num != 0)
            {
                app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
            }
            app_gfps_set_ble_conn_param(conn_id);
        }
#endif
    }
}

void app_gfps_adv_init(void)
{
    bool ret = false;
    uint8_t random_address[6] = {0};
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    uint16_t adv_interval_min = extend_app_cfg_const.gfps_discov_adv_interval;
    uint16_t adv_interval_max = extend_app_cfg_const.gfps_discov_adv_interval;
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;

    T_GAP_CAUSE cause = le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, random_address);
    memcpy(gfps_db.random_address, random_address, GAP_BD_ADDR_LEN);

    ble_ext_adv_mgr_init_adv_params(&gfps_db.gfps_adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type,
                                    peer_address, filter_policy, 0, NULL,
                                    gfps_get_scan_rsp_data_len(), gfps_scan_rsp_data, random_address);

    ret = ble_ext_adv_mgr_register_callback(app_gfps_adv_callback, gfps_db.gfps_adv_handle);

    APP_PRINT_TRACE4("app_gfps_adv_init: gfps_adv_handle %d, cause %d, ret %d, gfps version %b",
                     gfps_db.gfps_adv_handle, cause, ret, TRACE_STRING(extend_app_cfg_const.gfps_version));
}

static void app_gfps_le_disconnect_cb(uint8_t conn_id, uint8_t local_disc_cause,
                                      uint16_t disc_cause)
{
    APP_PRINT_TRACE3("app_gfps_le_disconnect_cb: conn_id %d, local_disc_cause %d, disc_cause 0x%x",
                     conn_id, local_disc_cause, disc_cause);

    T_APP_GFPS_LE_LINK *p_link;
    p_link = app_gfps_link_find_le_link_by_conn_id(conn_id);

    if (p_link == NULL)
    {
        return;
    }

#if F_GFPS_COMMON_FINDER_SUPPORT
    /*gfps finder maybe connected by gfps ble link to do provising or provisioned actions,
    so when gfps ble link disconencted, we shall reset gfps finder link here*/
    if (extend_app_cfg_const.gfps_finder_support)
    {
        app_gfps_finder_reset_conn();
    }
#endif

    APP_PRINT_TRACE3("app_gfps_le_disconnect_cb: plink connid %d, gfps link connid %d, gfps curr conn_id %d",
                     p_link->conn_id, p_link->gfps_link.gfps_conn_id, gfps_db.current_conn_id);

    if (p_link->conn_id != p_link->gfps_link.gfps_conn_id && p_link->conn_id != gfps_db.current_conn_id)
    {
        return;
    }

    if ((p_link->conn_id == gfps_db.current_conn_id) &&
        (disc_cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)))
    {
        //reset current link aes key and pair status
        gfps_db.current_conn_id = 0xFF;
        gfps_reset_aeskey_and_pairing_status();

        //provider recover to original io capability
        if (p_link->gfps_link.auth_param_change)
        {
            APP_PRINT_TRACE2("app_gfps_le_disconnect_cb: gap_set_pairable_mode, io_cap %d, authen_flag 0x%x",
                             p_link->gfps_link.io_cap, p_link->gfps_link.auth_flags);

            gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &(p_link->gfps_link.io_cap));
            gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t),
                          &(p_link->gfps_link.auth_flags));
            gap_set_pairable_mode();
        }
    }
    else
    {
        APP_PRINT_ERROR1("app_gfps_le_disconnect_cb: abnormal disconnection cause 0x%x", disc_cause);
    }

    app_gfps_link_delete_link_from_priority_queue(p_link->gfps_link.gfps_conn_id);
    p_link->gfps_link.gfps_conn_state = GAP_CONN_STATE_DISCONNECTED;
    p_link->gfps_link.gfps_conn_id = 0xff;

    memset(&p_link->gfps_link, 0, sizeof(T_GFPS_LINK));
    app_gfps_device_handle_ble_link_disconnected(local_disc_cause);
}

void app_gfps_send_le_bond_confirm(uint8_t conn_id)
{
    T_APP_GFPS_LE_LINK *p_link;
    p_link = app_gfps_link_find_le_link_by_conn_id(conn_id);
    if (p_link == NULL)
    {
        return;
    }

    if (p_link->gfps_link.le_bond_passkey == p_link->gfps_link.gfps_raw_passkey)
    {
        le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        gfps_send_passkey(conn_id, gfps_db.gfps_id, p_link->gfps_link.le_bond_passkey);
    }
    else
    {
        le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_REJECT);
    }
    p_link->gfps_link.le_bond_confirm_pending = false;
}

void app_gfps_send_legacy_bond_confirm(uint8_t conn_id)
{
    T_APP_GFPS_LE_LINK *p_link;
    p_link = app_gfps_link_find_le_link_by_conn_id(conn_id);
    if (p_link == NULL)
    {
        return;
    }

    if (p_link->gfps_link.edr_bond_passkey == p_link->gfps_link.gfps_raw_passkey)
    {
        gap_br_user_cfm_req_cfm(p_link->gfps_link.edr_bond_bd_addr, GAP_CFM_CAUSE_ACCEPT);
        gfps_send_passkey(conn_id, gfps_db.gfps_id, p_link->gfps_link.edr_bond_passkey);
    }
    else
    {
        gap_br_user_cfm_req_cfm(p_link->gfps_link.edr_bond_bd_addr, GAP_CFM_CAUSE_REJECT);
    }
    p_link->gfps_link.edr_bond_confirm_pending = false;
}

static void app_gfps_set_receive_pair_req(bool received)
{
    gfps_db.receive_pair_req = received;
}

/**
 * @brief Check if the Fast Pair Provider should reject the normal pair request.
 * When the Fast Pair Provider receives a KBP write request,
 * it shall attempt to decrypt the value using the AES key .
 * if the value is not decrypted successfully,
 * reject the normal pair for security reasons.
 * @return true  reject normal pairing for MITM attack protection
 * @return false allow normal pairing
 */
bool app_gfps_get_reject_pair_flag(void)
{
    APP_PRINT_INFO1("app_gfps_get_reject_pair_flag: %d", gfps_db.reject_pair);
    return gfps_db.reject_pair;
}

void app_gfps_clear_reject_pair_flag(void)
{
    gfps_db.reject_pair = false;
    APP_PRINT_INFO0("app_gfps_clear_reject_pair_flag");
}

void app_gfps_handle_le_justwork_bond_cfm(uint8_t conn_id)
{
    T_GAP_CFM_CAUSE cause;
    if (extend_app_cfg_const.gfps_support)
    {
        if (app_gfps_get_reject_pair_flag())
        {
            cause = GAP_CFM_CAUSE_REJECT;
        }
        else
        {
            cause = GAP_CFM_CAUSE_ACCEPT;
        }
    }
    else
    {
        cause = GAP_CFM_CAUSE_ACCEPT;
    }
    le_bond_just_work_confirm(conn_id, cause);
}

bool app_gfps_handle_bt_user_confirm(T_BT_EVENT_PARAM_LINK_USER_CONFIRMATION_REQ confirm_req)
{
    T_APP_GFPS_LE_LINK *p_link;
    p_link = app_gfps_link_find_le_link_by_conn_id(gfps_db.current_conn_id);

    if (!extend_app_cfg_const.gfps_support || p_link == NULL)
    {
        return false;
    }

    if (app_gfps_get_reject_pair_flag())
    {
        gap_br_user_cfm_req_cfm(confirm_req.bd_addr, GAP_CFM_CAUSE_REJECT);
        return true;
    }

    if (p_link->gfps_link.is_gfps_pairing)
    {
        app_gfps_set_receive_pair_req(true);
        APP_PRINT_TRACE3("app_gfps_handle_bt_user_confirm: CB_EVENT_USER_CONFIRM_REQ is_gfps_pairing %d, passkey %d, gfps_raw_passkey %d",
                         p_link->gfps_link.is_gfps_pairing, confirm_req.display_value, p_link->gfps_link.gfps_raw_passkey);

        memcpy(p_link->gfps_link.edr_bond_bd_addr, confirm_req.bd_addr, 6);
        p_link->gfps_link.edr_bond_passkey = confirm_req.display_value;

        if (p_link->gfps_link.gfps_raw_passkey_received)
        {
            app_gfps_send_legacy_bond_confirm(gfps_db.current_conn_id);
            p_link->gfps_link.gfps_raw_passkey_received = false;
        }
        else
        {
            p_link->gfps_link.edr_bond_confirm_pending = true;
            //wait up to 10 seconds for a write to the Passkey characteristic.
            gfps_db.receive_passkey = false;
            app_gfps_timer_start_check_receive_passkey();
        }

        return true;
    }
    else
    {
        APP_PRINT_TRACE0("app_gfps_handle_bt_user_confirm: not gfps pair, app maybe need call gap_br_user_cfm_req_cfm");
        return false;
    }
}

bool app_gfps_handle_ble_user_confirm(uint8_t conn_id)
{
    APP_PRINT_TRACE1("app_gfps_handle_ble_user_confirm: gfps_support %d",
                     extend_app_cfg_const.gfps_support);

    if (extend_app_cfg_const.gfps_support)
    {
        T_APP_GFPS_LE_LINK *p_link;
        p_link = app_gfps_link_find_le_link_by_conn_id(conn_id);

        if (p_link == NULL)
        {
            return false;
        }

        uint32_t passkey = 0;
        le_bond_get_display_key(conn_id, &passkey);
        p_link->gfps_link.le_bond_passkey = passkey;

        APP_PRINT_TRACE3("app_gfps_handle_ble_user_confirm: passkey %d, gfps_raw_passkey %d, is_gfps_pairing %d",
                         passkey, p_link->gfps_link.gfps_raw_passkey, p_link->gfps_link.is_gfps_pairing);

        if (p_link->gfps_link.is_gfps_pairing)
        {
            app_gfps_set_receive_pair_req(true);
            if (p_link->gfps_link.gfps_raw_passkey_received)
            {
                app_gfps_send_le_bond_confirm(conn_id);
                p_link->gfps_link.gfps_raw_passkey_received = false;
            }
            else
            {
                p_link->gfps_link.le_bond_confirm_pending = true;
                //wait up to 10 seconds for a write to the Passkey characteristic.
                gfps_db.receive_passkey = false;
                app_gfps_timer_start_check_receive_passkey();
            }

            return true;
        }
    }

    return false;
}

#if F_GFPS_COMMON_TWS_MODE
bool app_gfps_handle_additional_ble_user_confirm(uint8_t conn_id)
{
    uint32_t passkey = 0;
    le_bond_get_display_key(conn_id, &passkey);
    gfps_db.gfps_additional_passkey_from_stack = passkey;

    APP_PRINT_TRACE1("app_gfps_handle_additional_ble_user_confirm: additional passkey %d", passkey);

    if (gfps_db.is_gfps_additional_pairing)
    {
        gfps_db.gfps_additional_conn_id = conn_id;

        if (gfps_db.gfps_additional_passkey_from_gfps_received)
        {
            if (gfps_db.gfps_additional_passkey_from_gfps == gfps_db.gfps_additional_passkey_from_stack)
            {
                le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            }
            else
            {
                le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_REJECT);
            }

            gfps_db.gfps_additional_passkey_from_gfps_received = false;
            app_gfps_sec_handle_additional_io_cap(false);
        }
        else
        {
            gfps_db.gfps_additional_passkey_from_gfps_pending = true;
            //TODO: start 10s timer
        }

        return true;
    }

    return false;
}

bool app_gfps_handle_le_numerical_comparison_bond_cfm(uint8_t conn_id)
{
    bool ret = false;

    if (extend_app_cfg_const.gfps_support)
    {
        if (app_gfps_get_reject_pair_flag())
        {
            le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_REJECT);
            return true;
        }

        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            ret = app_gfps_handle_ble_user_confirm(conn_id);
        }
        else
        {
#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
            if (extend_app_cfg_const.gfps_le_device_support)
            {
                ret = app_gfps_handle_additional_ble_user_confirm(conn_id);
            }
#endif
        }
    }

    return ret;
}

void app_gfps_sec_handle_addition_bond_info(uint8_t *buf)
{
    APP_PRINT_TRACE0("app_gfps_sec_handle_addition_bond_info");
    T_GFPS_ADDITIONAL_BOND_DATA gfps_additional_bond_data = {0};
    memcpy(&gfps_additional_bond_data, buf, sizeof(T_GFPS_ADDITIONAL_BOND_DATA));

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        gfps_db.gfps_additional_passkey_from_gfps = gfps_additional_bond_data.bond_passkey;
        gfps_db.gfps_additional_passkey_from_gfps_received = true;

        if (gfps_db.gfps_additional_passkey_from_gfps_pending == true)
        {
            if (gfps_db.gfps_additional_passkey_from_gfps == gfps_db.gfps_additional_passkey_from_stack)
            {
                le_bond_user_confirm(gfps_db.gfps_additional_conn_id, GAP_CFM_CAUSE_ACCEPT);
                gfps_additional_bond_data.status_code = GFPS_PASSKEY_STATUS_CODE_SUCCESS;
            }
            else
            {
                le_bond_user_confirm(gfps_db.gfps_additional_conn_id, GAP_CFM_CAUSE_REJECT);
                gfps_additional_bond_data.status_code = GFPS_PASSKEY_STATUS_CODE_FAIL;
            }

            /*secondary bud send additional bond result to primary bud*/
            gfps_additional_bond_data.bond_passkey = gfps_db.gfps_additional_passkey_from_stack;

            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS,
                                                APP_REMOTE_MSG_GFPS_ADDITIONAL_BOND_INFO,
                                                (uint8_t *)&gfps_additional_bond_data, sizeof(T_GFPS_ADDITIONAL_BOND_DATA));

            gfps_db.gfps_additional_passkey_from_gfps_received = false;
            gfps_db.gfps_additional_passkey_from_gfps_pending = false;

            app_gfps_sec_handle_additional_io_cap(false);
        }
    }

    APP_PRINT_TRACE2("app_gfps_sec_handle_addition_bond_info: gfps passkey %d, stack passkey %d",
                     gfps_db.gfps_additional_passkey_from_gfps,
                     gfps_db.gfps_additional_passkey_from_stack);
}

void app_gfps_pri_handle_additional_bond_info(uint8_t *buf)
{
    APP_PRINT_TRACE0("app_gfps_pri_handle_additional_bond_info");
    T_GFPS_ADDITIONAL_BOND_DATA gfps_additional_bond_data = {0};
    memcpy(&gfps_additional_bond_data, buf, sizeof(T_GFPS_ADDITIONAL_BOND_DATA));

    T_GFPS_ADDITIONAL_PASSKEY passkey_info;
    passkey_info.passkey = gfps_additional_bond_data.bond_passkey;
    passkey_info.status_code = (T_GFPS_PASSKEY_STATUS_CODE)gfps_additional_bond_data.status_code;

    memcpy(passkey_info.target_bonding_addr, gfps_additional_bond_data.bond_bd_addr, 6);

    gfps_set_additional_passkey_info(passkey_info);

    gfps_send_additional_passkey_response(gfps_db.current_conn_id, gfps_db.gfps_id, passkey_info);
}

void app_gfps_sec_handle_additional_io_cap(bool change_io_cap)
{
    APP_PRINT_TRACE1("app_gfps_sec_handle_additional_io_cap: change_io_cap %d", change_io_cap);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        if (change_io_cap)
        {
            uint8_t io_cap = GAP_IO_CAP_DISPLAY_YES_NO;
            uint16_t auth_flags;

            gfps_db.is_gfps_additional_pairing = true;
            auth_flags = (gfps_db.additional_default_auth_flags | GAP_AUTHEN_BIT_MITM_FLAG);
            gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
            gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
            gap_set_pairable_mode();
            APP_PRINT_TRACE2("app_gfps_sec_handle_additional_io_cap: gap_set_pairable_mode, io_cap %d, authen_flag 0x%x",
                             io_cap, auth_flags);
#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
            app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
#endif
        }
        else
        {
            gfps_db.is_gfps_additional_pairing = false;
            //change to original io capability
            gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t),
                          &(gfps_db.additional_default_io_cap));
            gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t),
                          &(gfps_db.additional_default_auth_flags));
            gap_set_pairable_mode();
        }
    }
}

T_APP_RESULT app_gfps_cb_handle_additional_passkey(T_GFPS_CALLBACK_DATA *p_callback)
{
    APP_PRINT_TRACE0("app_gfps_cb_handle_additional_passkey");
    T_GFPS_ADDITIONAL_BOND_DATA gfps_additional_bond_data = {0};

    memcpy(gfps_additional_bond_data.bond_bd_addr,
           p_callback->msg_data.additional_passkey.target_bonding_addr, 6);

    gfps_additional_bond_data.bond_passkey = p_callback->msg_data.additional_passkey.passkey;
    gfps_db.status_code = GFPS_PASSKEY_STATUS_CODE_PENDING;

    /*primary bud send passkey get from gfps additional passkey characteristic to secondary bud*/
    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS,
                                        APP_REMOTE_MSG_GFPS_ADDITIONAL_BOND_INFO,
                                        (uint8_t *)&gfps_additional_bond_data, sizeof(T_GFPS_ADDITIONAL_BOND_DATA));

    return APP_RESULT_SUCCESS;
}
#endif

void app_gfps_set_allow_write_account_key(bool allow)
{
    gfps_db.allow_write_account_key = allow;
}

void app_gfps_check_receive_passkey(void)
{
    T_APP_GFPS_LE_LINK *p_le_link = app_gfps_link_find_le_link_by_conn_id(gfps_db.current_conn_id);
    if (!gfps_db.receive_passkey)
    {
        if (p_le_link)
        {
            app_gfps_ble_disconnect(p_le_link, GFPS_LE_LOCAL_DISC_CAUSE_GFPS_PAIR_NOT_RECEIVE_PASSKEY);
        }
    }
    APP_PRINT_TRACE2("app_gfps_check_receive_passkey: receive_passkey %d, p_le_link 0x%x",
                     gfps_db.receive_passkey, p_le_link);
}

void app_gfps_check_receive_pair_req(void)
{
    T_APP_GFPS_LE_LINK *p_le_link = app_gfps_link_find_le_link_by_conn_id(gfps_db.current_conn_id);

    if (!gfps_db.receive_pair_req)
    {
        if (p_le_link)
        {
            app_gfps_ble_disconnect(p_le_link, GFPS_LE_LOCAL_DISC_CAUSE_GFPS_PAIR_NOT_STARTED);
        }
    }
    APP_PRINT_TRACE2("app_gfps_check_receive_pair_req: receive_pair_req %d, p_le_link 0x%x",
                     gfps_db.receive_pair_req, p_le_link);
}

void app_gfps_reset_account_key_write_counts(void)
{
    APP_PRINT_TRACE0("app_gfps_reset_account_key_write_counts");
    gfps_db.account_key_write_counts = 0;
}

void app_gfps_reset_failure_count(void)
{
    APP_PRINT_TRACE0("app_gfps_reset_failure_count");
    gfps_db.failure_count = 0;
}

static bool app_gfps_check_provider_addr(uint8_t *received_provider_addr)
{
    /*The value is decrypted successfully if the output matches the format in Table 1.2.1 or Table 1.2.2
     (that is, if it contains either the Fast Pair Provider's current BLE address,
     or the Fast Pair Provider's public address).*/
    uint8_t local_public_addr[6] = {0x00};
    gap_get_param(GAP_PARAM_BD_ADDR, local_public_addr);

    if (memcmp(received_provider_addr, gfps_db.random_address, 6) != 0 &&
        memcmp(received_provider_addr, local_public_addr, 6) != 0)
    {
        APP_PRINT_ERROR3("app_gfps_check_provider_addr: provider addr %s not right, random %s, public %s",
                         TRACE_BDADDR(received_provider_addr),
                         TRACE_BDADDR(gfps_db.random_address),
                         TRACE_BDADDR(local_public_addr));
        return false;
    }
    return true;
}

static bool app_gfps_check_failure_count(T_GFPS_KBP_WRITE_RESULT result)
{
    if (gfps_db.failure_count >= 10)
    {
        APP_PRINT_ERROR1("app_gfps_security_condition_check: failure_count %d", gfps_db.failure_count);
        return false;
    }

    /*Keep a count of these failures. When the failure count hits 10, fail all new requests immediately.
    Reset the failure count after 5 minutes, after power on, or after a success.*/
    if (result != GFPS_WRITE_RESULT_SUCCESS)
    {
        gfps_db.failure_count++;
        APP_PRINT_ERROR1("app_gfps_security_condition_check, failure_count %d", gfps_db.failure_count);
        if (gfps_db.failure_count == 10)
        {
            APP_PRINT_ERROR0("app_gfps_timer_start_reset_failure_count");
            app_gfps_timer_start_reset_failure_count();
        }
        return false;
    }
    else
    {
        gfps_db.failure_count = 0;
    }
    return true;
}

static T_APP_RESULT app_gfps_security_condition_check(T_APP_GFPS_LE_LINK *p_le_link,
                                                      T_GFPS_CALLBACK_DATA *p_callback)
{
#if !F_GFPS_COMMON_LE_DEVICE_SUPPORT
    if (gfps_db.current_conn_id != p_callback->conn_id)
    {
        app_gfps_ble_disconnect(p_le_link, GFPS_LE_LOCAL_DISC_CAUSE_GFPS_FAILED);
        return APP_RESULT_VALUE_NOT_ALLOWED;
    }
#endif

    /*Keep a count of these failures. When the failure count hits 10, fail all new requests immediately.
    Reset the failure count after 5 minutes, after power on, or after a success.*/
    if (!app_gfps_check_failure_count(p_callback->msg_data.kbp.result))
    {
        return APP_RESULT_REJECT;
    };

    /*The value is decrypted successfully if the output matches the format in Table 1.2.1 or Table 1.2.2
    (that is, if it contains either the Fast Pair Provider's current BLE address,
     or the Fast Pair Provider's public address).*/
    if (!app_gfps_check_provider_addr(p_callback->msg_data.kbp.provider_addr))
    {
        return APP_RESULT_REJECT;
    };

    /*If the optional Public Key field is present.*/
    if (p_callback->msg_data.kbp.pk_field_exist == 1)
    {
        //If the device is not in pairing mode, ignore the write and exit.
        if (!p_callback->msg_data.kbp.retroactively_account_key && !gfps_db.force_enter_pair_mode)
        {
            APP_PRINT_ERROR0("app_gfps_security_condition_check: not in pairing mode");
            return APP_RESULT_APP_ERR;
        }
    }
    return APP_RESULT_SUCCESS;
}

static void app_gfps_set_io_cap_display_yes_no(T_APP_GFPS_LE_LINK *p_le_link)
{
    uint8_t io_cap = GAP_IO_CAP_DISPLAY_YES_NO;
    uint16_t auth_flags;

    gap_get_param(GAP_PARAM_BOND_IO_CAPABILITIES, &(p_le_link->gfps_link.io_cap));
    gap_get_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, &(p_le_link->gfps_link.auth_flags));
    p_le_link->gfps_link.auth_param_change = true;
    auth_flags = (p_le_link->gfps_link.auth_flags | GAP_AUTHEN_BIT_MITM_FLAG);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);

    gap_set_pairable_mode();
    APP_PRINT_TRACE2("app_gfps_set_io_cap_display_yes_no: gap_set_pairable_mode, io_cap %d, authen_flag 0x%x",
                     io_cap, auth_flags);

}

void app_gfps_set_multi_identity_address(T_GFPS_CALLBACK_DATA *p_callback)
{
    uint8_t ret = 0;
#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        T_GFPS_LE_DEVICE_MODE gfps_le_device_mode = gfps_le_get_device_mode();

        if (gfps_le_device_mode == GFPS_LE_DEVICE_MODE_LE_MODE_WITH_LEA ||
            gfps_le_device_mode == GFPS_LE_DEVICE_MODE_DUAL_MODE_WITH_LEA)
        {
            if (p_callback->msg_data.kbp.seeker_ble_audio_support)
            {
#if F_GFPS_COMMON_TWS_MODE
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
                    ret = 1;//b2b connected, provider support lea, seeker support lea
                    uint8_t change_io_cap = true;
                    /*primary bud send change_io_cap to secondary bud*/
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS,
                                                        APP_REMOTE_MSG_GFPS_ADDITIONAL_IO_CAP,
                                                        &change_io_cap, 1);
                }
                else
                {
                    ret = 2;//b2b disconnect
                }
#endif
            }
            else
            {
                ret = 3;//seeker not support lea
            }
        }
        else
        {
            ret = 4;////provider not support lea
        }
    }
    else
#endif
    {
        ret = 5;//provider not support le
    }

    if (ret == 1)
    {
#if F_GFPS_COMMON_TWS_MODE
        gfps_set_identity_address(app_cfg_nv.bud_local_addr, app_cfg_nv.bud_peer_addr, true);
#endif
    }
    else
    {
        gfps_set_identity_address(app_cfg_nv.bud_local_addr, NULL, false);
    }

    APP_PRINT_TRACE1("app_gfps_set_multi_identity_address: ret %d", ret);
}

T_APP_RESULT app_gfps_cb_handle_kbp_write_req(T_SERVER_ID service_id,
                                              T_GFPS_CALLBACK_DATA *p_callback)
{
    T_APP_RESULT  app_result = APP_RESULT_SUCCESS;
    T_APP_GFPS_LE_LINK *p_le_link = app_gfps_link_find_le_link_by_conn_id(p_callback->conn_id);

    if (!p_le_link)
    {
        return APP_RESULT_APP_ERR;
    }
    else
    {
        app_gfps_ble_conn_info_init(p_callback->conn_id);
        p_le_link->gfps_link.is_gfps_pairing = true;

        APP_PRINT_TRACE7("app_gfps_cb_handle_kbp_write_req: provider bond %d, notify name %d, "
                         "retroactively %d, pk exist %d, accountkey idx %d, provider addr %s, seek addr %s",
                         p_callback->msg_data.kbp.provider_init_bond,
                         p_callback->msg_data.kbp.notify_existing_name,
                         p_callback->msg_data.kbp.retroactively_account_key,
                         p_callback->msg_data.kbp.pk_field_exist,
                         p_callback->msg_data.kbp.account_key_idx,
                         TRACE_BDADDR(p_callback->msg_data.kbp.provider_addr),
                         TRACE_BDADDR(p_callback->msg_data.kbp.seek_br_edr_addr));

        T_APP_RESULT gfps_check_result = app_gfps_security_condition_check(p_le_link, p_callback);
        if (gfps_check_result != APP_RESULT_SUCCESS)
        {
#if F_GFPS_COMMON_FINDER_SUPPORT
            if (extend_app_cfg_const.gfps_finder_support &&
                (extend_app_cfg_const.gfps_device_type == GFPS_LOCATOR_TRACKER))
            {
                APP_PRINT_TRACE0("app_gfps_cb_handle_kbp_write_req: location tags ignore gfps_check_result");
            }
            else
#endif
            {
                APP_PRINT_ERROR1("app_gfps_security_condition_check: err result 0x%x", gfps_check_result);
                return gfps_check_result;
            }
        }
    }

    app_gfps_set_allow_write_account_key(true);

    if (p_callback->msg_data.kbp.retroactively_account_key == 1)
    {
        /*If the Provider is bonded without going through the Fast Pair flow,
        allow a new account key to be written through the Key-based Pairing method for up to one minute.
        Only accept one account key to be written during this time.*/
        gfps_db.is_gfps_retroactive_pairing = true;
        app_gfps_timer_retroactively_start_check_accountkey();

        /*If an Raw Request with Flags bit 3 set is received,
        the Provider should verify the bonded device's BR/EDR address is the same as what is included in the request.
        If not, reject the request.*/
        T_APP_BR_LINK *p_curr_br_link = app_link_find_br_link(p_callback->msg_data.kbp.seek_br_edr_addr);
        T_APP_GFPS_LE_LINK *p_curr_le_link = app_gfps_link_find_le_link_by_addr(
                                                 p_callback->msg_data.kbp.seek_br_edr_addr);

        APP_PRINT_TRACE2("app_gfps_cb_handle_kbp_write_req: p_curr_br_link 0x%x, p_curr_le_link 0x%x",
                         p_curr_br_link, p_curr_le_link);

        if (!p_curr_br_link && !p_curr_le_link)
        {
            gfps_db.allow_write_account_key = false;
            APP_PRINT_ERROR1("app_gfps_cb_handle_kbp_write_req: err addr %s, allow_write_account_key false",
                             TRACE_BDADDR(p_callback->msg_data.kbp.seek_br_edr_addr));
            return APP_RESULT_REJECT;
        }
        else
        {
            memcpy(p_le_link->gfps_link.edr_bond_bd_addr, p_callback->msg_data.kbp.seek_br_edr_addr, 6);
        }
    }
    else if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
#if F_GFPS_COMMON_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support &&
            (extend_app_cfg_const.gfps_device_type == GFPS_LOCATOR_TRACKER))
        {
            APP_PRINT_TRACE0("app_gfps_cb_handle_kbp_write_req: location tags ignore pair procedure");
            return app_result;
        }
#endif

        app_gfps_set_io_cap_display_yes_no(p_le_link);
        app_gfps_set_multi_identity_address(p_callback);

        if (p_callback->msg_data.kbp.provider_init_bond)
        {
            gap_br_pairing_req(p_callback->msg_data.kbp.seek_br_edr_addr);
        }
        else
        {
            /*Wait up to 10 seconds for a pairing request. If none is received, exit.*/
            app_gfps_set_receive_pair_req(false);
            app_gfps_timer_start_check_receive_pair_req();
        }

#if GFPS_PERSONALIZED_NAME_SUPPORT
        if (p_callback->msg_data.kbp.notify_existing_name)
        {
            app_gfps_personalized_name_send(p_callback->conn_id, service_id);
        }
#endif

#if !F_GFPS_COMMON_LOCATION_TRACKER
        if (p_callback->msg_data.kbp.pk_field_exist == 0)
        {
            bool need_enter_pair_mode =
                (app_cfg_const.enable_multi_link &&
                 (app_link_get_b2s_link_num() == app_cfg_const.max_legacy_multilink_devices)) ||
                (!app_cfg_const.enable_multi_link && (app_link_get_b2s_link_num() > 0));

            if (need_enter_pair_mode && app_hfp_get_call_status() == APP_CALL_IDLE)
            {
                APP_PRINT_TRACE1("app_gfps_cb_handle_kbp_write_req: disconnect b2s for subsequent pair, multilink enable %d",
                                 app_cfg_const.enable_multi_link);
                app_bt_policy_enter_pairing_mode(true, false);
            }
        }
#endif
    }
    return app_result;
}

T_APP_RESULT app_gfps_cb_handle_action_req(T_GFPS_CALLBACK_DATA *p_callback)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    if (p_callback->msg_data.action_req.result == GFPS_WRITE_RESULT_SUCCESS)
    {
        APP_PRINT_TRACE5("app_gfps_cb_handle_action_req: pk_field_exist %d, account_key_idx %d, "
                         "device_action %d, additional_data %d, provider_addr %s",
                         p_callback->msg_data.action_req.pk_field_exist,
                         p_callback->msg_data.action_req.account_key_idx,
                         p_callback->msg_data.action_req.device_action,
                         p_callback->msg_data.action_req.additional_data,
                         TRACE_BDADDR(p_callback->msg_data.action_req.provider_addr)
                        );
    }
    else
    {
        APP_PRINT_TRACE1("app_gfps_cb_handle_action_req: failed, result %d",
                         p_callback->msg_data.action_req.result);
    }

    return app_result;
}

T_APP_RESULT app_gfps_cb_handle_passkey(T_GFPS_CALLBACK_DATA *p_callback)
{
    T_APP_GFPS_LE_LINK *p_link;
    p_link = app_gfps_link_find_le_link_by_conn_id(p_callback->conn_id);
    if (p_link == NULL)
    {
        return APP_RESULT_APP_ERR;
    }

    gfps_db.receive_passkey = true;
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    p_link->gfps_link.gfps_raw_passkey = p_callback->msg_data.passkey;
    p_link->gfps_link.gfps_raw_passkey_received = true;

    APP_PRINT_TRACE3("app_gfps_cb_handle_passkey: gfps_raw_passkey %d ,le_bond_confirm_pending %d, edr_bond_confirm_pending %d",
                     p_link->gfps_link.gfps_raw_passkey,
                     p_link->gfps_link.le_bond_confirm_pending,
                     p_link->gfps_link.edr_bond_confirm_pending);

    if (p_link->gfps_link.le_bond_confirm_pending)
    {
        app_gfps_send_le_bond_confirm(p_callback->conn_id);
        p_link->gfps_link.gfps_raw_passkey_received = false;
    }
    else if (p_link->gfps_link.edr_bond_confirm_pending)
    {
        app_gfps_send_legacy_bond_confirm(p_callback->conn_id);
        p_link->gfps_link.gfps_raw_passkey_received = false;
    }

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
    if (gfps_get_pairing_status() == GFPS_PAIRING_STATUS_SUBSEQUENT &&
        extend_app_cfg_const.gfps_le_device_support)
    {
        T_GFPS_LE_DEVICE_MODE gfps_le_device_mode = gfps_le_get_device_mode();

        if ((gfps_le_device_mode == GFPS_LE_DEVICE_MODE_LE_MODE_WITH_LEA  ||
             gfps_le_device_mode == GFPS_LE_DEVICE_MODE_DUAL_MODE_WITH_LEA)
            && app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            APP_PRINT_TRACE0("app_gfps_cb_handle_passkey: start gfps and le audio adv for subsequent pairing");
            app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
        }

        if (p_link->gfps_link.auth_param_change)
        {
            APP_PRINT_TRACE2("app_gfps_cb_handle_passkey: gap_set_pairable_mode, io_cap %d, authen_flag 0x%x",
                             p_link->gfps_link.io_cap, p_link->gfps_link.auth_flags);
            //provider recover to original io capability
            gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &(p_link->gfps_link.io_cap));
            gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t),
                          &(p_link->gfps_link.auth_flags));
            gap_set_pairable_mode();
            p_link->gfps_link.auth_param_change = false;
        }
#if F_GFPS_COMMON_TWS_MODE
        /*The current procedure was a subsequent pair, seeker will not write account key,
        so primary need relay APP_REMOTE_MSG_GFPS_SUBPAIR_SEC_ADV_ENABLE to let secondary enable gfps/le_audio adv */
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS,
                                            APP_REMOTE_MSG_GFPS_SUBPAIR_SEC_ADV_ENABLE,
                                            NULL, 0);
#endif
    }
#endif

    return app_result;
}

T_APP_RESULT app_gfps_cb_handle_accountkey(T_GFPS_CALLBACK_DATA *p_callback)
{
    T_APP_GFPS_LE_LINK *p_link;
    p_link = app_gfps_link_find_le_link_by_conn_id(p_callback->conn_id);

    if (p_link == NULL)
    {
        return APP_RESULT_APP_ERR;
    }

    /*If the Provider is bonded without going through the Fast Pair flow,
     allow a new account key to be written through the Key-based Pairing method for up to one minute.
     Only accept one account key to be written during this time.*/
    if (gfps_db.is_gfps_retroactive_pairing)
    {
        gfps_db.account_key_write_counts++;
        gfps_db.is_gfps_retroactive_pairing = false;

        if (gfps_db.account_key_write_counts > 1)
        {
            APP_PRINT_ERROR1("app_gfps_cb_handle_accountkey: account_key_write_counts %d",
                             gfps_db.account_key_write_counts);
            return APP_RESULT_VALUE_NOT_ALLOWED;
        }

        if (!gfps_db.allow_write_account_key)
        {
            APP_PRINT_ERROR1("app_gfps_cb_handle_accountkey: allow_write_account_key %d",
                             gfps_db.allow_write_account_key);
            return APP_RESULT_VALUE_NOT_ALLOWED;
        }
    }
    else
    {
        /*
        If phone and the earphones have been paired before:
        When the earphones enable the discoverable adv (earphones re-enter pairing mode),
        phone will reconnect to the earphones.Both the BR/EDR link and LE link will be established,
        but there will be no exchange of passkey information. The BR/EDR will not re-enter the pairing process,
        The LE link will exchange key-based pairing characteristic, and account key characteristic information.
        However, because phone and the earphones have been paired before, the LE link will not exchange passkey information.
        so we need call app_gfps_set_receive_pair_req(true) here.
        */
        app_gfps_set_receive_pair_req(true);

#if F_GFPS_COMMON_LOCATION_TRACKER
        app_gfps_force_enter_pairing_mode(GFPS_EXIT_PAIR_MODE);
#endif
    }

    /*for initial pairing or retroactive write account key receive account key from seeker*/
    if (app_gfps_account_key_store(p_callback->msg_data.account_key,
                                   p_link->gfps_link.edr_bond_bd_addr))
    {
#if F_GFPS_COMMON_TWS_MODE
        /*The current procedure was a initial pair or retroactive write accountkey, seeker will write account key,
        so relay account key info to secondary, secondary need enable gfps/le_audio adv when receive account key sync info */
        app_gfps_account_key_remote_add(p_callback->msg_data.account_key,
                                        p_link->gfps_link.edr_bond_bd_addr);
#endif
#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
        if (extend_app_cfg_const.gfps_le_device_support)
        {
            app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);

            T_GFPS_LE_DEVICE_MODE gfps_le_device_mode = gfps_le_get_device_mode();

            if ((gfps_le_device_mode == GFPS_LE_DEVICE_MODE_LE_MODE_WITH_LEA  ||
                 gfps_le_device_mode == GFPS_LE_DEVICE_MODE_DUAL_MODE_WITH_LEA)
                && app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                APP_PRINT_TRACE0("app_gfps_cb_handle_accountkey: start le audio adv for subsequent pairing");
                app_gfps_lea_adv_start();
            }
        }
        else
#endif
        {
            app_gfps_device_check_state((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
        }
    }

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        if (p_link->gfps_link.auth_param_change)
        {
            APP_PRINT_TRACE2("app_gfps_cb_handle_accountkey: gap_set_pairable_mode, io_cap %d, authen_flag 0x%x",
                             p_link->gfps_link.io_cap, p_link->gfps_link.auth_flags);
            //provider recover to original io capability;
            gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &(p_link->gfps_link.io_cap));
            gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t),
                          &(p_link->gfps_link.auth_flags));
            gap_set_pairable_mode();
            p_link->gfps_link.auth_param_change = false;
        }
    }
#endif

    return APP_RESULT_SUCCESS;
}

T_APP_RESULT app_gfps_cb_handle_additional_data(T_GFPS_CALLBACK_DATA *p_callback)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    //personalized name need to be stored in ftl
#if GFPS_PERSONALIZED_NAME_SUPPORT
    if (p_callback->msg_data.additional_data.data_id == GFPS_DATA_ID_PERSONALIZED_NAME)
    {
        T_GFPS_PERSON_NAME_RST result = app_gfps_personalized_name_store(
                                            p_callback->msg_data.additional_data.p_data,
                                            p_callback->msg_data.additional_data.data_len);
        if (result == APP_GFPS_PERSONALIZED_NAME_SUCCESS)
        {
#if F_GFPS_COMMON_TWS_MODE
            app_gfps_personalized_name_remote_sync();
#endif
        }

        APP_PRINT_TRACE4("app_gfps_cb_handle_additional_data: personalized name %s, result %d, data_id 0x%x, data_len %d",
                         TRACE_STRING(p_callback->msg_data.additional_data.p_data), result,
                         p_callback->msg_data.additional_data.data_id,
                         p_callback->msg_data.additional_data.data_len);
    }
#endif

    return app_result;
}

void app_gfps_set_ble_conn_param(uint8_t conn_id)
{
    /*Google's specifications recommend setting it to 7.5ms,
    but Realtek's le audio doesn't seem to support it(from wade_li)*/
#if 0
    uint16_t interval_min = 6;//6*1.25ms
    uint16_t interval_max = 6;
    uint16_t latency = 0;
    uint16_t supervision_timeout = 500;

    ble_set_prefer_conn_param(conn_id, interval_min, interval_max,
                              latency, supervision_timeout);
#endif
}

/*Fast pair service callback*/
static T_APP_RESULT app_gfps_cb(T_SERVER_ID service_id, void *p_data)
{
    uint8_t ret = 0;
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_GFPS_CALLBACK_DATA *p_callback = (T_GFPS_CALLBACK_DATA *)p_data;

    if (p_callback->ret != GFPS_WRITE_RESULT_SUCCESS)
    {
        APP_PRINT_ERROR0("app_gfps_cb: KBP write failed reject pair");
        gfps_db.reject_pair = true;
        app_gfps_timer_start_check_reject_pair_flag();
    }

    switch (p_callback->msg_type)
    {
    case GFPS_CALLBACK_TYPE_NOTIFICATION_ENABLE:
        {
            APP_PRINT_TRACE1("app_gfps_cb: GFPS_CALLBACK_TYPE_NOTIFICATION_ENABLE, T_GFPS_NOTIFICATION_TYPE %d",
                             p_callback->msg_data.notify_type);
        }
        break;

    case GFPS_CALLBACK_TYPE_KBP_WRITE_REQ:
        {
            app_result = app_gfps_cb_handle_kbp_write_req(service_id, p_callback);
        }
        break;

    case GFPS_CALLBACK_TYPE_ACTION_REQ:
        {
            app_result = app_gfps_cb_handle_action_req(p_callback);
        }
        break;

    case GFPS_CALLBACK_TYPE_PASSKEY:
        {
            app_result = app_gfps_cb_handle_passkey(p_callback);
        }
        break;

    case GFPS_CALLBACK_TYPE_ACCOUNT_KEY:
        {
            app_result = app_gfps_cb_handle_accountkey(p_callback);

#if F_GFPS_COMMON_FINDER_SUPPORT
            if (extend_app_cfg_const.gfps_finder_support &&
                (extend_app_cfg_const.gfps_device_type == GFPS_LOCATOR_TRACKER))
            {
                app_gfps_timer_start_tag_auto_reset();
            }
#endif
        }
        break;

#if GFPS_ADDTIONAL_DATA_SUPPORT
    case GFPS_CALLBACK_TYPE_ADDITIONAL_DATA:
        {
            app_result = app_gfps_cb_handle_additional_data(p_callback);
        }
        break;
#endif

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
    case GFPS_CALLBACK_TYPE_ADDITIONAL_PASSKEY:
        {
            if (extend_app_cfg_const.gfps_le_device_support)
            {
#if F_GFPS_COMMON_TWS_MODE
                app_result = app_gfps_cb_handle_additional_passkey(p_callback);
#endif
            }
        }
        break;
#endif

    default:
        break;
    }

    return app_result;
}

/**
 * @brief non discoverable mode has two submode:show pop up ui or hide pop up ui
 * hide pop up ui: we want to stop showing the subsequent pairing notification since that pairing could be rejected
 *
 */
void app_gfps_enter_nondiscoverable_mode()
{
    APP_PRINT_TRACE0("app_gfps_enter_nondiscoverable_mode");
    app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE);
}

void app_gfps_force_enter_pairing_mode(uint8_t status)
{
    if (!extend_app_cfg_const.gfps_support)
    {
        return;
    }

    switch (status)
    {
    case GFPS_KEY_FORCE_ENTER_PAIR_MODE:
    case GFPS_LE_DISCONN_FORCE_ENTER_PAIR_MODE:
    case GFPS_BT_POLICY_FORCE_ENTER_PAIR_MODE:
        {
            gfps_db.force_enter_pair_mode = true;
        }
        break;

    case GFPS_EXIT_PAIR_MODE:
    case GFPS_BT_POLICY_FORCE_EXIT_PAIR_MODE:
        {
            gfps_db.force_enter_pair_mode = false;
        }
        break;

    default:
        break;
    }

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        APP_PRINT_TRACE2("app_gfps_force_enter_pairing_mode: gfps le enable, status %d, force_enter_pair_mode %d",
                         status, gfps_db.force_enter_pair_mode);
        app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    }
    else
#endif
    {
        if (gfps_db.force_enter_pair_mode)
        {
            APP_PRINT_TRACE2("app_gfps_force_enter_pairing_mode: gfps legacy enable, status %d, force_enter_pair_mode %d",
                             status, gfps_db.force_enter_pair_mode);
            app_gfps_next_action(GFPS_ACTION_ADV_DISCOVERABLE_MODE_WITH_MODEL_ID);
        }
    }
}

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
void app_gfps_le_device_adv_start(T_REMOTE_SESSION_ROLE device_role)
{
    APP_PRINT_TRACE3("app_gfps_le_device_adv_start: bud_role %d, force enter pairing %d, le support %d",
                     device_role, gfps_db.force_enter_pair_mode,
                     extend_app_cfg_const.gfps_le_device_support);

    if (!extend_app_cfg_const.gfps_le_device_support)
    {
        return;
    }

    if (!gfps_db.force_enter_pair_mode && (app_gfps_get_application_stop_adv_state() == true))
    {
        /*The app calls app_gfps_adv_stop_by_application() to stop gfps advertising,
        but does not call app_gfps_adv_start_by_application to restart gfps advertising.*/
        APP_PRINT_TRACE0("app_gfps_le_device_adv_start: gfps adv is not allowed by application"
                         "Please check if it meets expectations.");
        return;
    }

    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        uint8_t link_num = app_gfps_link_find_gfps_link();
        T_GFPS_LE_DEVICE_MODE gfps_le_device_mode = gfps_le_get_device_mode();

        if (link_num >= GFPS_LE_MAX_LINK_NUMBER)
        {
            if (device_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI);
            }

            return;
        }

        if (device_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            T_ACCOUNT_KEY *p_key_info = app_gfps_account_key_get_table();

            if (gfps_db.force_enter_pair_mode)
            {
#if F_GFPS_COMMON_FINDER_SUPPORT
                if (extend_app_cfg_const.gfps_finder_support &&
                    (extend_app_cfg_const.gfps_device_type == GFPS_LOCATOR_TRACKER) &&
                    app_gfps_finder_provisoned())
                {
                    //Discoverable Fast Pair frames shouldn't be advertised while the Provider is provisioned for FHN.
                    APP_PRINT_ERROR0("app_gfps_le_device_adv_start: tag already provisioned, please do factory reset firstly and then enter pairing mode");
                    return;
                }
                else
#endif
                {
                    app_gfps_next_action(GFPS_ACTION_ADV_DISCOVERABLE_MODE_WITH_MODEL_ID);
                }
            }
            else
            {
#if F_GFPS_COMMON_FINDER_SUPPORT
                if (extend_app_cfg_const.gfps_finder_support &&
                    (extend_app_cfg_const.gfps_device_type == GFPS_LOCATOR_TRACKER))
                {
                    //When advertising non-discoverable Fast Pair frames, UI indications shouldn't be enabled.
                    app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI);
                }
                else
#endif
                {
                    app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE);
                }
            }

#if F_APP_LEA_SUPPORT
            if (gfps_le_device_mode == GFPS_LE_DEVICE_MODE_LE_MODE_WITH_LEA ||
                gfps_le_device_mode == GFPS_LE_DEVICE_MODE_DUAL_MODE_WITH_LEA)
            {
                app_gfps_lea_adv_start();
            }
#endif
        }
        else
        {
            if (gfps_le_device_mode == GFPS_LE_DEVICE_MODE_LE_MODE_WITH_LEA ||
                gfps_le_device_mode == GFPS_LE_DEVICE_MODE_DUAL_MODE_WITH_LEA)
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
#if F_APP_LEA_SUPPORT
                    /*b2b must connected and pri relay SIRK to sec (LEA_REMOTE_SYNC_SIRK)
                    and then secondary start LE audio adv*/
                    app_gfps_lea_adv_start();
#endif
                }
            }
            else
            {
                app_gfps_next_action(GFPS_ACTION_IDLE);
                app_gfps_link_disconnect_all_gfps_link();
            }
        }
    }
}
#endif

/**
 * @brief Include the battery information in the advertisement with hide battery notification (0b0100) for at least 5 seconds
 * when case is closed and both buds are docked.
 * Include the battery information in the advertisement with show battery notification(0b0011) when the case has opened.
 * @param[in] open  if open = true means case open, if open = false means case close.
 */
void app_gfps_handle_case_status(bool open)
{
    APP_PRINT_TRACE1("app_gfps_handle_case_status: open %d", open);
    if (!extend_app_cfg_const.gfps_support)
    {
        return;
    }

    T_BT_DEVICE_MODE radio_mode = app_bt_policy_get_radio_mode();

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        if (open)
        {
#if F_GFPS_COMMON_BATTERY_INFO_REPORT
            /*show battery popup*/
            app_gfps_battery_show_ui(true);
#endif
            /*show pair popup*/
            app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE);

#if F_GFPS_COMMON_FINDER_SUPPORT
            if (extend_app_cfg_const.gfps_finder_support)
            {
                T_GFPS_FINDER_BEACON_STATE beacon_state = GFPS_FINDER_BEACON_STATE_ON;
                app_gfps_finder_enter_beacon_state(beacon_state);
            }
#endif
        }
        else
        {
#if F_GFPS_COMMON_BATTERY_INFO_REPORT
            /*hid battery popup*/
            app_gfps_battery_show_ui(false);
#endif
            /*hid pair popup*/
            app_gfps_next_action(GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI);

#if F_GFPS_COMMON_FINDER_SUPPORT
            if (extend_app_cfg_const.gfps_finder_support)
            {
                T_GFPS_FINDER_BEACON_STATE beacon_state = GFPS_FINDER_BEACON_STATE_OFF;
                app_gfps_finder_enter_beacon_state(beacon_state);
            }
#endif
        }

        //app_gfps_enter_nondiscoverable_mode();
    }
}

void app_gfps_handle_b2s_connected(uint8_t *bd_addr)
{
    uint8_t account_key[16] = {0};
    T_APP_GFPS_LE_LINK *p_link;

    p_link = app_gfps_link_find_le_link_by_conn_id(gfps_db.current_conn_id);

    if (!extend_app_cfg_const.gfps_support || p_link == NULL)
    {
        return;
    }

    if (p_link->gfps_link.is_gfps_pairing)
    {
        /*(memcmp(bd_addr, gfps_link.edr_bond_bd_addr, 6) == 0): b2s is connected through gfps pair.
        gfps_get_subsequent_pairing_account_key(account_key) == true: is gfps subsequent pairing.
        */
        if ((memcmp(bd_addr, p_link->gfps_link.edr_bond_bd_addr, 6) == 0) &&
            (gfps_get_subsequent_pairing_account_key(account_key) == true))
        {
            /*store subsequent pairing account key*/
            if (app_gfps_account_key_store(account_key, p_link->gfps_link.edr_bond_bd_addr))
            {
#if F_GFPS_COMMON_TWS_MODE
                app_gfps_account_key_remote_add(account_key, p_link->gfps_link.edr_bond_bd_addr);
#endif
            }
            APP_PRINT_TRACE0("app_gfps_handle_b2s_connected: subsequent pair account key store");
        }

        APP_PRINT_TRACE2("app_gfps_handle_b2s_connected:bd_addr: %b, gfps_link->edr_bond_bd_addr: %b",
                         TRACE_BINARY(6, bd_addr), TRACE_BINARY(6, p_link->gfps_link.edr_bond_bd_addr));
        app_gfps_force_enter_pairing_mode(GFPS_EXIT_PAIR_MODE);
    }
    else
    {
#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
        if (extend_app_cfg_const.gfps_le_device_support)
        {
            app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
        }
        else
#endif
        {
            app_gfps_device_check_state((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
        }
    }
}

void app_gfps_handle_b2s_ble_bonded(uint8_t conn_id, uint8_t *p_remote_identity_addr)
{
    T_APP_GFPS_LE_LINK *p_link;
    p_link = app_gfps_link_find_le_link_by_conn_id(conn_id);

    if (!extend_app_cfg_const.gfps_le_device_support || p_link == NULL)
    {
        return;
    }

    if (p_link->gfps_link.is_gfps_pairing)
    {
        APP_PRINT_TRACE2("app_gfps_handle_b2s_ble_bonded: is_gfps_pairing %d, remote identity address %b",
                         p_link->gfps_link.is_gfps_pairing, TRACE_BDADDR(p_remote_identity_addr));

        memcpy(p_link->gfps_link.edr_bond_bd_addr, p_remote_identity_addr, 6);

        T_GFPS_LE_DEVICE_MODE gfps_le_device_mode = gfps_le_get_device_mode();

        if ((gfps_le_device_mode == GFPS_LE_DEVICE_MODE_LE_MODE_WITH_LEA ||
             gfps_le_device_mode == GFPS_LE_DEVICE_MODE_LE_MODE_WITHOUT_LEA))
        {
            T_GFPS_PAIRING_STATUS gfps_pair_status = GFPS_PAIRING_STATUS_IDLE;
            gfps_set_pairing_status(gfps_pair_status);
            app_gfps_force_enter_pairing_mode(GFPS_EXIT_PAIR_MODE);
        }
    }
    else
    {
#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
        if (extend_app_cfg_const.gfps_le_device_support)
        {
            app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
        }
        else
#endif
        {
            app_gfps_device_check_state((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
        }
    }
}

T_APP_RESULT app_gfps_read_firmware_version(void)
{
    if (extend_app_cfg_const.gfps_support)
    {
#if F_GFPS_COMMON_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support &&
            (extend_app_cfg_const.gfps_device_type == GFPS_LOCATOR_TRACKER) &&
            app_gfps_finder_provisoned())
        {
            APP_PRINT_TRACE0("gfps finder do not support read firmware version");
            return APP_RESULT_REJECT;
        }
        else
#endif
        {
            dis_set_parameter(DIS_PARAM_FIRMWARE_REVISION,
                              sizeof(extend_app_cfg_const.gfps_version),
                              (void *)extend_app_cfg_const.gfps_version);
        }
    }
    return APP_RESULT_SUCCESS;
}

void app_gfps_msg_queue_init(void)
{
    if (extend_app_cfg_const.gfps_support)
    {
        gfps_ecc_init_msg_queue(audio_evt_queue_handle, audio_io_queue_handle);
    }

    APP_PRINT_TRACE1("app_gfps_msg_queue_init: gfps_support %d", extend_app_cfg_const.gfps_support);
}

void app_gfps_module_init(void)
{
    app_gfps_cfg_init();
    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_msg_audio_and_rfc_init();
        app_gfps_init();
    }

#if F_GFPS_COMMON_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support)
    {
        app_dult_device_init();
        app_dult_svc_init();
    }
#endif
}

void app_gfps_module_adv_init(void)
{
    /*Resolvable private address can only be successfully generate after BLE stack ready,
    app_gfps_adv_init() and app_gfps_finder_init() need to generate RPA, so we call them here*/
    APP_PRINT_TRACE2("app_gfps_module_adv_init: gfps_support %d, finder_support %d",
                     extend_app_cfg_const.gfps_support,
                     extend_app_cfg_const.gfps_finder_support);

    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_adv_init();

#if F_GFPS_COMMON_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            app_gfps_finder_init();

#if F_GFPS_COMMON_PERIODIC_WAKEUP
            if (!extend_app_cfg_const.disable_finder_adv_when_power_off
                && app_gfps_finder_provisioned())
            {
                uint32_t clock_value_delayed = 0;
                uint32_t rtc_counter = 0;

                rtc_counter = RTC_GetCounter();
                clock_value_delayed = RTC_COUNTER_TO_SECOND(rtc_counter);

                APP_PRINT_TRACE2("app_gfps_module_adv_init: Wakeup, rtc_counter %d, clock_value_delayed %d",
                                 rtc_counter, clock_value_delayed);

                RTC_RunCmd(DISABLE);
                power_mode_set(POWER_POWERDOWN_MODE);
                app_gfps_finder_update_clock_value(clock_value_delayed);

                if (app_db.wake_up_reason & WAKE_UP_RTC)
                {
                    power_mode_set(POWER_DLPS_MODE);
                    app_gfps_finder_handle_event_wakeup_by_rtc();
                }
            }
#endif
        }
#endif
    }
}

void app_gfps_in_case_out_case_report_battery(uint8_t remote_loc, uint8_t local_loc)
{
    if (extend_app_cfg_const.gfps_support)
    {
#if F_GFPS_COMMON_BATTERY_INFO_REPORT
        if (remote_loc == BUD_LOC_IN_CASE)
        {
            app_gfps_battery_info_report(GFPS_BATTERY_REPORT_BUD_IN_CASE);
        }
        else if ((local_loc != BUD_LOC_IN_CASE) && (remote_loc != BUD_LOC_IN_CASE))
        {
            app_gfps_battery_info_report(GFPS_BATTERY_REPORT_BUDS_OUT_CASE);
        }
#endif
    }
}

void app_gfps_set_profile_mask(void)
{
    app_cfg_const.supported_profile_mask |= GFPS_PROFILE_MASK;
}

void app_gfps_set_lea_support(bool enable)
{
    APP_PRINT_TRACE1("app_gfps_set_lea_support: %d", enable);
    extend_app_cfg_const.gfps_le_device_support = enable;
}

void app_gfps_set_key_convert_flag(uint8_t *p_key_convert_flag)
{
#if !F_GFPS_COMMON_LOCATION_TRACKER
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        *p_key_convert_flag |= GAP_SC_KEY_CONVERT_LE_TO_BREDR_FLAG;
    }
#endif

    APP_PRINT_TRACE1("app_gfps_set_key_convert_flag: 0x%x", *p_key_convert_flag);
}

void app_gfps_handle_mmi_lea_enter_pair_mode(void)
{
    APP_PRINT_TRACE0("app_gfps_handle_mmi_lea_enter_pair_mode");
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        app_gfps_force_enter_pairing_mode(GFPS_KEY_FORCE_ENTER_PAIR_MODE);

#if !F_GFPS_COMMON_LOCATION_TRACKER
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)TONE_GFPS_ENTER_PAIRING_MODE, false, false);
        }
#endif
    }
}

T_APP_RESULT app_gfps_ble_connection_accept_or_reject(uint8_t *remote_bd_addr,
                                                      uint8_t remote_addr_type)
{
    T_APP_RESULT result;

    if (app_gfps_ble_conn_info_handle(remote_bd_addr, remote_addr_type) == true)
    {
        result = APP_RESULT_ACCEPT;
    }
    else
    {
        result = APP_RESULT_REJECT;
    }

    APP_PRINT_TRACE1("app_gfps_ble_connection_accept_or_reject: result %d", result);

    return result;
}

#if CONFIG_REALTEK_GFPS_SASS_SUPPORT
/**
 * @brief Check if sass connection status changed.
 *        If changed, update the sass connection status
 *
 */
void app_gfps_adv_check_sass_conn_status(void)
{
    T_SASS_CONN_STATUS_FIELD cur_conn_status;
    //get full conn status
    gfps_sass_get_conn_status(&cur_conn_status);
    //generate new conn status
    app_gfps_gen_conn_status(&cur_conn_status);

    if (gfps_sass_set_conn_status(&cur_conn_status) == true)
    {
        //if new conn status different with old conn status, update conn status in adv data
        if (gfps_db.gfps_curr_action == GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE)
        {
            if (gfps_gen_adv_data(NOT_DISCOVERABLE_MODE, gfps_adv_data, &gfps_adv_len, true))
            {
                ble_ext_adv_mgr_set_adv_data(gfps_db.gfps_adv_handle, gfps_adv_len, gfps_adv_data);
            }
        }
        else if (gfps_db.gfps_curr_action == GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI)
        {
            if (gfps_gen_adv_data(NOT_DISCOVERABLE_MODE, gfps_adv_data, &gfps_adv_len, false))
            {
                ble_ext_adv_mgr_set_adv_data(gfps_db.gfps_adv_handle, gfps_adv_len, gfps_adv_data);
            }
        }
        else
        {
            APP_PRINT_TRACE1("app_gfps_adv_check_sass_conn_status: invalid action %d",
                             gfps_db.gfps_curr_action);
        }
    }
    else
    {
        APP_PRINT_TRACE0("app_gfps_adv_check_sass_conn_status: no change, do not need update");
    }
}

/**
 * @brief update sass conn status
 *
 */
void app_gfps_adv_update_conn_status(void)
{
    app_gfps_adv_check_sass_conn_status();

    if (gfps_db.gfps_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
        if (extend_app_cfg_const.gfps_le_device_support)
        {
            app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
        }
        else
#endif
        {
            app_gfps_device_check_state((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
        }
    }
}

void app_gfps_check_inuse_account_key(void)
{
    uint8_t active_idx = app_multi_get_active_idx() < MAX_BR_LINK_NUM ?
                         app_multi_get_active_idx() : MAX_BR_LINK_NUM;
    T_APP_BR_LINK *p_link = NULL;
    uint8_t inuse_account_key = gfps_get_inuse_account_key_index();
    uint8_t recently_inuse_account_key = gfps_get_recently_inuse_account_key_index();
    uint8_t b2s_link_num = app_link_get_b2s_link_num();
    APP_PRINT_TRACE5("app_gfps_check_inuse_account_key active %d ,multicfg %d, b2s %d, inused %d, rececnt %d",
                     active_idx, app_cfg_nv.sass_multipoint_state, b2s_link_num, inuse_account_key,
                     recently_inuse_account_key);

    if (b2s_link_num != 0)
    {
        if (active_idx != MAX_BR_LINK_NUM)
        {
            p_link = &app_db.br_link[active_idx];
        }
        if (app_cfg_nv.sass_multipoint_state)
        {
            if (b2s_link_num == 1)
            {
                if ((active_idx != MAX_BR_LINK_NUM) &&
                    (p_link->used))
                {
                    if (inuse_account_key != p_link->gfps_inuse_account_key)
                    {
                        if (p_link->gfps_inuse_account_key != 0xff)//sass device
                        {
                            gfps_set_most_recently_inuse_account_key_index(0xff);
                        }
                        else //non-sass device
                        {
                            if (inuse_account_key != 0xff)
                            {
                                gfps_set_most_recently_inuse_account_key_index(inuse_account_key);
                                app_cfg_nv.sass_recently_used_account_key_index = inuse_account_key;
                            }
                        }
                        gfps_set_inuse_account_key_index(p_link->gfps_inuse_account_key);
                    }
                }
            }
            else if (b2s_link_num == 2)
            {
                uint8_t inactive_idx = MAX_BR_LINK_NUM;
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_link_check_b2s_link_by_id(i) && (i != active_idx))
                    {
                        inactive_idx = i;
                        break;
                    }
                }
                //sass device
                if ((active_idx != MAX_BR_LINK_NUM) && (inactive_idx != MAX_BR_LINK_NUM))
                {
                    if (p_link->gfps_inuse_account_key != 0xff)
                    {
                        gfps_set_most_recently_inuse_account_key_index(0xff);
                        gfps_set_inuse_account_key_index(p_link->gfps_inuse_account_key);
                    }
                    else//inused non sass device
                    {
                        if (app_db.br_link[inactive_idx].gfps_inuse_account_key != 0xff)
                        {
                            gfps_set_most_recently_inuse_account_key_index(app_db.br_link[inactive_idx].gfps_inuse_account_key);
                            app_cfg_nv.sass_recently_used_account_key_index =
                                app_db.br_link[inactive_idx].gfps_inuse_account_key;
                        }
                        else
                        {
                            gfps_set_most_recently_inuse_account_key_index(app_cfg_nv.sass_recently_used_account_key_index);
                        }
                        gfps_set_inuse_account_key_index(0xff);
                    }
                }
            }
        }
        else
        {
            if ((active_idx != MAX_BR_LINK_NUM) &&
                (p_link->used))
            {
                if (inuse_account_key != p_link->gfps_inuse_account_key)
                {
                    gfps_set_most_recently_inuse_account_key_index(inuse_account_key);
                    if (inuse_account_key != 0xff)
                    {
                        app_cfg_nv.sass_recently_used_account_key_index = inuse_account_key;
                    }
                    gfps_set_inuse_account_key_index(p_link->gfps_inuse_account_key);
                }
            }
        }
    }
    else
    {
        if (inuse_account_key != 0xff)
        {
            gfps_set_most_recently_inuse_account_key_index(inuse_account_key);
            app_cfg_nv.sass_recently_used_account_key_index = inuse_account_key;
            gfps_set_inuse_account_key_index(0xff);
        }
    }
    inuse_account_key = gfps_get_inuse_account_key_index();
    recently_inuse_account_key = gfps_get_recently_inuse_account_key_index();
    if ((inuse_account_key != 0xff) && (recently_inuse_account_key != 0xff))
    {
        APP_PRINT_TRACE3("app_gfps_check_inuse_account_key abnormal key setting 0x%,0x%x,0x%x",
                         inuse_account_key, recently_inuse_account_key, p_link->gfps_inuse_account_key);
        gfps_set_most_recently_inuse_account_key_index(0xff);

    }
}

void app_gfps_notify_conn_status(void)
{
    if (extend_app_cfg_const.gfps_sass_support)
    {
        app_gfps_check_inuse_account_key();

        app_gfps_adv_update_conn_status();
        app_gfps_msg_notify_connection_status();
    }
}

bool app_gfps_gen_conn_status(T_SASS_CONN_STATUS_FIELD *conn_status)
{
    uint8_t length = 3;
    T_SASS_FOCUS_MODE focus_mode = SASS_NOT_IN_FOCUS;
    T_SASS_ON_HEAD_DETECTION on_head = SASS_NOT_ON_HEAD;
    T_SASS_AUTO_RECONN auto_reconnect = SASS_AUTO_RECONN_DISABLE;
    T_SASS_CONN_STATE conn_state = SASS_DISABLE_CONN_SWITCH;
    T_SASS_CONN_AVAILABLE conn_available = SASS_CONN_NOT_FULL;
    uint8_t active_index = 0;
    if (conn_status == NULL)
    {
        return false;
    }
    active_index = app_multi_get_active_idx();
#if 0
    if (app_sass_policy_get_focus_mode())
    {
        focus_mode = SASS_IN_FOCUS;
    }
#endif
    if ((app_db.local_loc == BUD_LOC_IN_EAR) || (app_db.remote_loc == BUD_LOC_IN_EAR))
    {
        on_head = SASS_ON_HEAD;
    }
    if (app_db.br_link[active_index].connected_by_linkback)
    {
        auto_reconnect = SASS_AUTO_RECONN_ENABLE;
    }
    conn_state = (T_SASS_CONN_STATE) app_sass_policy_get_connection_state();
    conn_available = app_sass_get_available_connection_num() <= 0 ? SASS_CONN_FULL :
                     SASS_CONN_NOT_FULL;
    conn_status->conn_status_info.auto_reconn = auto_reconnect;
    conn_status->conn_status_info.conn_availability = conn_available;
    conn_status->conn_status_info.conn_state = (uint8_t)conn_state;
    conn_status->conn_status_info.focus_mode = focus_mode;
    conn_status->conn_status_info.on_head_detection = on_head;
    conn_status->conn_dev_bitmap = app_sass_policy_get_conn_bit_map();
    return true;
}
#endif

#endif
