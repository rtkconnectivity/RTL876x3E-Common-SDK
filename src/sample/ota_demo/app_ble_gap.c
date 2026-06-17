/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "sysm.h"
#include "gap_conn_le.h"
#include "gap_bond_le.h"
#include "gap_ext_scan.h"
#include "dfu_api.h"
#include "ota_service.h"
#include "app_ble_gap.h"
#include "app_main.h"
#include "app_ble_common_adv.h"
#include "ble_stream.h"
#include "ble_scan.h"
#include "ble_mgr.h"
#include "ble_conn.h"

uint8_t scan_rsp_data_len;
uint8_t scan_rsp_data[GAP_MAX_LEGACY_ADV_LEN];

bool app_ble_gap_gen_scan_rsp_data(uint8_t *p_scan_len, uint8_t *p_scan_data)
{
    uint8_t device_name_len;

    if (p_scan_len == NULL || p_scan_data == NULL)
    {
        return false;
    }

    uint8_t ble_device_name[40] = "BLE ota demo";
    device_name_len = strlen((const char *)ble_device_name);

    if (device_name_len > GAP_MAX_LEGACY_ADV_LEN - 2)
    {
        device_name_len = GAP_MAX_LEGACY_ADV_LEN - 2;
    }

    p_scan_data[0] = device_name_len + 1;
    p_scan_data[1] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;

    memcpy(&p_scan_data[2], ble_device_name, device_name_len);

    *p_scan_len = device_name_len + 2;
    return true;
}

static T_APP_RESULT app_ble_gap_cb(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA cb_data;

    memcpy(&cb_data, p_cb_data, sizeof(T_LE_CB_DATA));

    ble_mgr_handle_gap_cb(cb_type, p_cb_data);

    APP_PRINT_TRACE1("app_ble_gap_cb %d", cb_type);

    switch (cb_type)
    {
    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE_INFO:
        {
            APP_PRINT_TRACE0("GAP_MSG_LE_EXT_ADV_STATE_CHANGE_INFO");
        }
        break;
    case GAP_MSG_LE_EXT_ADV_REPORT_INFO:
        APP_PRINT_INFO6("GAP_MSG_LE_EXT_ADV_REPORT_INFO:event_type 0x%x, bd_addr %s, addr_type %d, rssi %d, data_len %d,data %b",
                        cb_data.p_le_ext_adv_report_info->event_type,
                        TRACE_BDADDR(cb_data.p_le_ext_adv_report_info->bd_addr),
                        cb_data.p_le_ext_adv_report_info->addr_type,
                        cb_data.p_le_ext_adv_report_info->rssi,
                        cb_data.p_le_ext_adv_report_info->data_len,
                        TRACE_BINARY(cb_data.p_le_ext_adv_report_info->data_len, cb_data.p_le_ext_adv_report_info->p_data));
        break;

    case GAP_MSG_LE_CONN_UPDATE_IND:
        {
            APP_PRINT_TRACE0("GAP_MSG_LE_CONN_UPDATE_IND");
            result = APP_RESULT_ACCEPT;
        }
        break;

    case GAP_MSG_LE_EXT_ADV_START_SETTING:
        {
            APP_PRINT_TRACE0("GAP_MSG_LE_EXT_ADV_START_SETTING");
        }
        break;

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        {
            APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation = 0x%x cause=0x%x",
                            cb_data.p_le_modify_white_list_rsp->operation,
                            cb_data.p_le_modify_white_list_rsp->cause);
        }
        break;
    default:
        break;
    }

    return result;
}

bool app_ble_gap_disconnect(T_APP_LE_LINK *p_link, T_LE_LOCAL_DISC_CAUSE disc_cause)
{
    if (p_link != NULL)
    {
        APP_PRINT_TRACE2("app_ble_gap_disconnect: conn_id %d, disc_cause %d",
                         p_link->conn_id, disc_cause);
        if (le_disconnect(p_link->conn_id) == GAP_CAUSE_SUCCESS)
        {
            p_link->local_disc_cause = disc_cause;
            return true;
        }
    }
    return false;
}

void app_ble_gap_handle_mtu_info(uint8_t conn_id, uint16_t mtu)
{
    T_APP_LE_LINK *p_link;
    p_link = app_link_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        p_link->mtu_size = mtu;
    }
}

static void app_ble_gap_handle_new_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                                  uint16_t disc_cause)
{
    T_APP_LE_LINK *p_link;

    APP_PRINT_TRACE3("app_ble_gap_handle_new_conn_state_evt: conn_id %d, new_state %d, cause 0x%04x",
                     conn_id, new_state, disc_cause);

    p_link = app_link_find_le_link_by_conn_id(conn_id);

    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTING:
        if (p_link != NULL)
        {
            p_link->state = LE_LINK_STATE_DISCONNECTING;
        }
        break;

    case GAP_CONN_STATE_DISCONNECTED:
        if (p_link != NULL)
        {
            // uint8_t local_disc_cause = p_link->local_disc_cause;
            for (uint8_t i = 0; i < p_link->disc_cb_list.count; i++)
            {
                T_LE_DISC_CB_ENTRY *p_entry;
                p_entry = os_queue_peek(&p_link->disc_cb_list, i);
                if (p_entry != NULL && p_entry->disc_callback != NULL)
                {
                    p_entry->disc_callback(conn_id, p_link->local_disc_cause, disc_cause);
                }
            }

            //app_transfer_queue_reset(CMD_PATH_LE);
            if (conn_id == app_ble_common_adv_get_conn_id())
            {
                app_ble_common_adv_reset_conn_id();
            }

            app_link_free_le_link(p_link);
            //if (local_disc_cause != LE_LOCAL_DISC_CAUSE_POWER_OFF &&
            //      local_disc_cause != LE_LOCAL_DISC_CAUSE_ROLESWAP)
            //{
            //      app_ble_device_handle_power_on_rtk_adv();
            //}
        }
        break;

    case GAP_CONN_STATE_CONNECTING:
        if (p_link == NULL)
        {
            p_link = app_link_alloc_le_link_by_conn_id(conn_id);
            if (p_link != NULL)
            {
                p_link->state = LE_LINK_STATE_CONNECTING;
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        if (p_link != NULL)
        {
            p_link->conn_handle = le_get_conn_handle(conn_id);


            if (p_link->state == LE_LINK_STATE_CONNECTING)
            {
                //app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_LINK_EXIST);
                p_link->state = LE_LINK_STATE_CONNECTED;
                le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &p_link->mtu_size, conn_id);
            }


            if (conn_id == app_ble_common_adv_get_conn_id())
            {
                uint16_t conn_interval_min = RWS_LE_DEFAULT_MIN_CONN_INTERVAL;
                uint16_t conn_interval_max = RWS_LE_DEFAULT_MAX_CONN_INTERVAL;
                uint16_t conn_latency = RWS_LE_DEFAULT_SLAVE_LATENCY;
                uint16_t conn_supervision_timeout = RWS_LE_DEFAULT_SUPERVISION_TIMEOUT;

                ble_set_prefer_conn_param(conn_id, conn_interval_min, conn_interval_max, conn_latency,
                                          conn_supervision_timeout);
            }
        }
        break;

    default:
        break;
    }
}

void app_ble_gap_handle_gap_msg(T_IO_MSG *p_io_msg)
{
    T_LE_GAP_MSG stack_msg;
    memcpy(&stack_msg, &p_io_msg->u.param, sizeof(p_io_msg->u.param));
    APP_PRINT_TRACE1("app_ble_gap_handle_gap_msg: subtype %d", p_io_msg->subtype);

    ble_mgr_handle_gap_msg(p_io_msg->subtype, &stack_msg);

    switch (p_io_msg->subtype)
    {
    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_ble_gap_handle_new_conn_state_evt(stack_msg.msg_data.gap_conn_state_change.conn_id,
                                                  (T_GAP_CONN_STATE)stack_msg.msg_data.gap_conn_state_change.new_state,
                                                  stack_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            app_ble_gap_handle_mtu_info(stack_msg.msg_data.gap_conn_mtu_info.conn_id,
                                        stack_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;

    default:
        break;
    }
}

static void app_ble_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            APP_PRINT_INFO0("BT_EVENT_READY2");
            memcpy(app_db.factory_addr, param->ready.bd_addr, 6);
            /* init here to avoid app_cfg_nv.bud_local_addr no mac info (due to factory reset) */
            app_ble_common_adv_init();
            app_ble_common_adv_start(0);//START ADV
        }
        break;

    default:
        break;
    }
}

void app_ble_gap_ble_mgr_init(void)
{
    BLE_MGR_PARAMS param = {0};

    param.ble_ext_adv.enable = true;
    param.ble_ext_adv.adv_num = 1;//todo future
    param.ble_conn.enable = true;
    uint8_t supported_max_le_link_num = le_get_max_link_num();
    param.ble_conn.link_num = ((MAX_BLE_LINK_NUM <= supported_max_le_link_num) ? MAX_BLE_LINK_NUM :
                               supported_max_le_link_num);

    param.ble_scan.enable = true;

    ble_mgr_init(&param);
}

void app_ble_gap_init(void)
{
    APP_PRINT_INFO0("app_ble_gap_init");
    bt_mgr_cback_register(app_ble_gap_bt_cback);
    le_register_app_cb(app_ble_gap_cb);
    app_ble_gap_ble_mgr_init();
}

void app_ble_gap_param_init(void)
{
    /*set device name and device appearance*/
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, "BLE ota demo");

    /*set GAP Bond Manager parameters*/
    uint32_t passkey             = 0;
    uint8_t use_fixed_passkey    = false;
    uint8_t sec_req_enable       = false;
    uint16_t sec_req_requirement = GAP_AUTHEN_BIT_SC_FLAG;

    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(uint32_t), &passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(uint8_t), &use_fixed_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &sec_req_requirement);

    /*Initialize ble link*/
    uint8_t supported_max_le_link_num = le_get_max_link_num();
    uint8_t link_num = ((MAX_BLE_LINK_NUM <= supported_max_le_link_num) ? MAX_BLE_LINK_NUM :
                        supported_max_le_link_num);
    le_gap_init(link_num);

    /* Initialize scan parameters */
    T_GAP_SCAN_MODE  scan_mode              = GAP_SCAN_MODE_PASSIVE;
    uint8_t scan_filter_policy              = GAP_SCAN_FILTER_ANY;
    uint8_t scan_filter_duplicate           = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
    T_GAP_LOCAL_ADDR_TYPE  own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;

    T_GAP_LE_EXT_SCAN_PARAM extended_scan_param[GAP_EXT_SCAN_MAX_PHYS_NUM];
    extended_scan_param[0].scan_type     = scan_mode;
    extended_scan_param[0].scan_interval = 0x10;
    extended_scan_param[0].scan_window   = 0x10;

    uint8_t  scan_phys         = GAP_EXT_SCAN_PHYS_1M_BIT;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period   = 0;

    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_LOCAL_ADDR_TYPE, sizeof(own_address_type),
                          &own_address_type);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                          &scan_filter_policy);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                          &scan_filter_duplicate);
    le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_1M, &extended_scan_param[0]);
}

