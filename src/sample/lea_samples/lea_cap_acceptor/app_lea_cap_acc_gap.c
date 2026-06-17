/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "trace.h"
#include "gap_msg.h"
#include "gap_bond_le.h"
#include "ble_audio.h"
#include "user_cmd_parse.h"
#include "app_lea_cap_acc_gap.h"
#include "app_lea_cap_acc_link.h"
#include "app_lea_cap_acc_user_cmd.h"
#include "app_lea_cap_acc_adv.h"
#include "ble_mgr.h"
#include "gattc_tbl_storage.h"
#include "gap_vendor.h"
#include "bt_gatt_client.h"
#if APP_LEA_EATT_SUPPORT
#include "app_lea_cap_acc_ecfc.h"
#endif

/** @defgroup  APP_LEA_CAP_ACC CAP Acceptor APP
    * @brief This file handles BLE Audio CAP Acceptor application routines.
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/

/** @defgroup  APP_LEA_CAP_ACC_GAP_MSG GAP Message Handler
    * @brief Handle GAP Message
    * @{
    */
T_GAP_DEV_STATE gap_dev_state = {0, 0, 0, 0};/**< GAP device state */
void app_handle_gap_msg(T_IO_MSG *p_gap_msg);
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief app_ble_mgr_lib_init
 *        Initialize ble manager lib.
 */
void app_ble_mgr_lib_init(void)
{
    BLE_MGR_PARAMS param = {0};

    param.ble_ext_adv.enable = true;
    param.ble_ext_adv.adv_num = 1;
#if (BAP_BROADCAST_SINK || BAP_SCAN_DELEGATOR)
    param.ble_scan.enable = true;
    gap_vendor_le_set_host_feature(0, 1);
#endif
    param.ble_conn.enable = true;
    param.ble_conn.link_num = APP_MAX_BLE_LINK_NUM;
    ble_mgr_init(&param);
}

/**
 * @brief app_handle_io_msg
 *        All the application messages are pre-handled in this function
 * @note  All the IO MSGs are sent to this function, then the event handling function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return    void
 */
void app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;
    uint8_t rx_char;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        {
            app_handle_gap_msg(&io_msg);
        }
        break;

    case IO_MSG_TYPE_LE_AUDIO:
        {
            ble_audio_handle_msg(&io_msg);
        }
        break;

    case IO_MSG_TYPE_UART:
        /* We handle user command informations from Data UART in this branch. */
        rx_char = (uint8_t)io_msg.subtype;
        user_cmd_collect(&user_cmd_if, &rx_char, sizeof(rx_char), user_cmd_table);
        break;

    default:
        break;
    }
}

/**
 * @brief app_handle_dev_state_evt
 *        Handle msg GAP_MSG_LE_DEV_STATE_CHANGE
 * @note  All the gap device state events are pre-handled in this function.Then the event handling function shall be called according to the new_state
 * @param[in] new_state  New gap device state
 * @param[in] cause GAP  device state change cause
 * @return    void
 */
void app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO3("app_handle_dev_state_evt: init state %d, scan state %d, cause 0x%x",
                    new_state.gap_init_state,
                    new_state.gap_scan_state, cause);
    if (gap_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            uint8_t bt_addr[6];
            APP_PRINT_INFO0("app_handle_dev_state_evt: GAP stack ready");
            /*stack ready*/
            gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
            data_uart_print("local bd addr: 0x%2x:%2x:%2x:%2x:%2x:%2x\r\n",
                            bt_addr[5],
                            bt_addr[4],
                            bt_addr[3],
                            bt_addr[2],
                            bt_addr[1],
                            bt_addr[0]);
            app_ext_adv_params_init();
            app_ext_adv_start();
        }
    }
    gap_dev_state = new_state;
}

/**
 * @brief app_handle_conn_state_evt
 *        Handle msg GAP_MSG_LE_CONN_STATE_CHANGE
 * @note  All the gap conn state events are pre-handled in this function.
 *        Then the event handling function shall be called according to the new_state
 * @param[in] conn_id    Connection ID
 * @param[in] new_state  New gap connection state
 * @param[in] disc_cause Use this cause when new_state is GAP_CONN_STATE_DISCONNECTED
 * @return   void
 */
void app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
    T_APP_LE_LINK *p_link;

    APP_PRINT_INFO3("app_handle_conn_state_evt: conn_id %d, new_state %d, disc_cause 0x%x",
                    conn_id, new_state, disc_cause);

    p_link = app_link_find_le_link_by_conn_id(conn_id);

    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTING:
        if (p_link != NULL)
        {
            p_link->state = GAP_CONN_STATE_DISCONNECTING;
        }
        break;

    case GAP_CONN_STATE_DISCONNECTED:
        if (p_link != NULL)
        {
            if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
                && (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
            {
                APP_PRINT_ERROR2("app_handle_conn_state_evt: connection lost, conn_id %d, cause 0x%x", conn_id,
                                 disc_cause);
            }

            data_uart_print("Disconnect conn_id %d\r\n", conn_id);
            app_link_free_le_link(p_link);
            app_ext_adv_start();
        }
        break;

    case GAP_CONN_STATE_CONNECTING:
        if (p_link == NULL)
        {
            p_link = app_link_alloc_le_link_by_conn_id(conn_id);
            if (p_link != NULL)
            {
                p_link->state = GAP_CONN_STATE_CONNECTING;
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        if (p_link != NULL)
        {
            p_link->conn_handle = le_get_conn_handle(conn_id);
            p_link->state = GAP_CONN_STATE_CONNECTED;
            le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &p_link->mtu_size, conn_id);
            le_get_conn_addr(conn_id, p_link->bd_addr,
                             &p_link->bd_type);
            data_uart_print("Connected success conn_id %d\r\n", conn_id);
        }
        break;

    default:
        break;

    }
}


#if (MCP_MEDIA_CONTROL_CLIENT || CCP_CALL_CONTROL_CLIENT)
T_APP_RESULT app_bt_gatt_client_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                                   void *p_data)
{
    if (type == GATT_CLIENT_EVENT_DIS_ALL_STATE)
    {
        T_GATT_CLIENT_DIS_ALL_DONE *p_disc = (T_GATT_CLIENT_DIS_ALL_DONE *)p_data;
        APP_PRINT_INFO2("app_bt_gatt_client_cb: GATT_CLIENT_EVENT_DIS_ALL_STATE, state %d, load_from_ftl %d",
                        p_disc->state, p_disc->load_from_ftl);
    }
#if APP_LEA_EATT_SUPPORT
    else if (type == GATT_CLIENT_EVENT_GATT_SERVICE_INFO)
    {
        T_GATT_CLIENT_GATT_SERVICE_INFO *p_gatt_svc = (T_GATT_CLIENT_GATT_SERVICE_INFO *)p_data;
        APP_PRINT_INFO2("app_bt_gatt_client_cb: GATT_CLIENT_EVENT_GATT_SERVICE_INFO, is_found %d, load_from_ftl %d",
                        p_gatt_svc->is_found, p_gatt_svc->load_from_ftl);
        if (p_gatt_svc->p_gatt_data)
        {
            APP_PRINT_INFO6("app_bt_gatt_client_cb:  char_exist 0x%x, data_exist 0x%x, client supported feature[%d] 0x%x,  server supported feature[%d] 0x%x",
                            p_gatt_svc->p_gatt_data->char_exist,
                            p_gatt_svc->p_gatt_data->data_exist,
                            p_gatt_svc->p_gatt_data->client_supported_features_len,
                            p_gatt_svc->p_gatt_data->client_supported_features[0],
                            p_gatt_svc->p_gatt_data->server_supported_features_len,
                            p_gatt_svc->p_gatt_data->server_supported_features[0]);
        }
    }
#endif
    return APP_RESULT_SUCCESS;
}
#endif

/**
 * @brief app_handle_authen_state_evt
 *        Handle msg GAP_MSG_LE_AUTHEN_STATE_CHANGE
 * @note  All the gap authentication state events are pre-handled in this function.
 *        Then the event handling function shall be called according to the new_state
 * @param[in] conn_id    Connection ID
 * @param[in] new_state  New authentication state
 * @param[in] cause      Use this cause when new_state is GAP_AUTHEN_STATE_COMPLETE
 * @return   void
 */
void app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    APP_PRINT_INFO2("app_handle_authen_state_evt:conn_id %d, cause 0x%x", conn_id, cause);

    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
        {
            APP_PRINT_INFO0("app_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
        }
        break;

    case GAP_AUTHEN_STATE_COMPLETE:
        {
            if (cause == GAP_SUCCESS)
            {
                data_uart_print("Pair success\r\n");
                APP_PRINT_INFO0("app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair success");
#if (MCP_MEDIA_CONTROL_CLIENT || CCP_CALL_CONTROL_CLIENT)
                gatt_client_start_discovery_all(le_get_conn_handle(conn_id),
                                                app_bt_gatt_client_cb);
#endif
            }
            else
            {
                data_uart_print("Pair failed: cause 0x%x\r\n", cause);
                APP_PRINT_INFO0("app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair failed");
            }
        }
        break;

    default:
        {
            APP_PRINT_ERROR1("app_handle_authen_state_evt: unknown new_state %d", new_state);
        }
        break;
    }
}

/**
 * @brief app_handle_conn_mtu_info_evt
 *        Handle msg GAP_MSG_LE_CONN_MTU_INFO
 * @note  This msg is used to inform APP that exchange mtu procedure is completed.
 * @param[in] conn_id   Connection ID
 * @param[in] mtu_size  New mtu size
 * @return   void
 */
void app_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
    APP_PRINT_INFO2("app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
}

/**
 * @brief app_handle_conn_param_update_evt
 *        Handle msg GAP_MSG_LE_CONN_PARAM_UPDATE
 * @note  All the connection parameter update change  events are pre-handled in this function.
 * @param[in] conn_id Connection ID
 * @param[in] status  New update state
 * @param[in] cause   Use this cause when status is GAP_CONN_PARAM_UPDATE_STATUS_FAIL
 * @return   void
 */
void app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status, uint16_t cause)
{
    switch (status)
    {
    case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS:
        {
            uint16_t conn_interval;
            uint16_t conn_slave_latency;
            uint16_t conn_supervision_timeout;

            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
            APP_PRINT_INFO4("app_handle_conn_param_update_evt update success:conn_id %d, conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x",
                            conn_id, conn_interval, conn_slave_latency, conn_supervision_timeout);
            data_uart_print("connection parameters update success \r\n");
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            APP_PRINT_ERROR2("app_handle_conn_param_update_evt update failed: conn_id %d, cause 0x%x",
                             conn_id, cause);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
        {
            APP_PRINT_INFO1("app_handle_conn_param_update_evt update pending: conn_id %d", conn_id);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief app_handle_gap_msg
 *        All the BT GAP MSG are pre-handled in this function.
 * @note  Then the event handling function shall be called according to the subtype of T_IO_MSG
 * @param[in] p_gap_msg Pointer to GAP msg
 * @return   void
 */
void app_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
    T_LE_GAP_MSG gap_msg;

    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));
    ble_mgr_handle_gap_msg(p_gap_msg->subtype, &gap_msg);
    APP_PRINT_TRACE1("app_handle_gap_msg: subtype %d", p_gap_msg->subtype);
    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            app_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
                                     gap_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_handle_conn_state_evt(gap_msg.msg_data.gap_conn_state_change.conn_id,
                                      (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
                                      gap_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            app_handle_conn_mtu_info_evt(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
                                         gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            app_handle_conn_param_update_evt(gap_msg.msg_data.gap_conn_param_update.conn_id,
                                             gap_msg.msg_data.gap_conn_param_update.status,
                                             gap_msg.msg_data.gap_conn_param_update.cause);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            app_handle_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
                                        gap_msg.msg_data.gap_authen_state.new_state,
                                        gap_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            le_bond_just_work_confirm(gap_msg.msg_data.gap_bond_just_work_conf.conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_JUST_WORK");
        }
        break;

    default:
        break;
    }
    ble_audio_handle_gap_msg(p_gap_msg->subtype, gap_msg);
}
/** @} */ /* End of group APP_LEA_CAP_ACC_GAP_MSG */

/** @defgroup  APP_LEA_CAP_ACC_GAP_CALLBACK GAP Callback Event Handler
    * @brief Handle GAP callback event
    * @{
    */
/**
  * @brief app_gap_callback
  *        Callback for gap le to notify app
  * @param[in] cb_type   callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result       @ref T_APP_RESULT
  */
T_APP_RESULT app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;

    ble_mgr_handle_gap_cb(cb_type, p_cb_data);
    ble_audio_handle_gap_cb(cb_type, p_cb_data);

    //APP_PRINT_TRACE1("app_gap_callback: cb_type 0x%x", cb_type);

    switch (cb_type)
    {
    case GAP_MSG_LE_CONN_UPDATE_IND:
        APP_PRINT_INFO5("GAP_MSG_LE_CONN_UPDATE_IND: conn_id %d, conn_interval_max 0x%x, conn_interval_min 0x%x, conn_latency 0x%x,supervision_timeout 0x%x",
                        p_data->p_le_conn_update_ind->conn_id,
                        p_data->p_le_conn_update_ind->conn_interval_max,
                        p_data->p_le_conn_update_ind->conn_interval_min,
                        p_data->p_le_conn_update_ind->conn_latency,
                        p_data->p_le_conn_update_ind->supervision_timeout);
        /* if reject the proposed connection parameter from peer device, use APP_RESULT_REJECT. */
        result = APP_RESULT_ACCEPT;
        break;

    case GAP_MSG_LE_BOND_MODIFY_INFO:
        APP_PRINT_INFO1("GAP_MSG_LE_BOND_MODIFY_INFO: type 0x%x",
                        p_data->p_le_bond_modify_info->type);
#if GATTC_TBL_STORAGE_SUPPORT
        gattc_tbl_storage_handle_bond_modify(p_data->p_le_bond_modify_info);
#endif
        break;

    default:
        //APP_PRINT_ERROR1("app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}

/**
  * @brief app_gap_common_callback
  *        Callback for gap to notify app
  * @param[in] cb_type   callback msy type @ref GAP_COMMON_MSG_TYPE.
  * @param[in] p_cb_data point to callback data @ref T_GAP_CB_DATA.
  * @retval result void
  */
void app_gap_common_callback(uint8_t cb_type, void *p_cb_data)
{
    ble_mgr_handle_gap_common_cb(cb_type, p_cb_data);
}

/**
 * @brief app_gap_init
 *        register gap message callback and initialize ble manager module.
 */
void app_gap_init(void)
{
    le_gap_init(APP_MAX_BLE_LINK_NUM);
    gap_lib_init();

    /* Device name and device appearance */
    uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "CAP Acceptor";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

    /* Use LE Advertising Extensions */
    uint8_t use_extended = true;
    le_set_gap_param(GAP_PARAM_USE_EXTENDED_ADV, sizeof(use_extended), &use_extended);

    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint8_t  irk_auto = true;

    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);
    le_bond_set_param(GAP_PARAM_BOND_GEN_LOCAL_IRK_AUTO, sizeof(uint8_t), &irk_auto);

    /* register gap message callback */
    le_register_app_cb(app_gap_callback);
    /* register gap common message callback */
    gap_register_app_cb(app_gap_common_callback);

    /* ble manager module initialize*/
    app_ble_mgr_lib_init();
#if APP_LEA_EATT_SUPPORT
    app_lea_ecfc_init();
#endif
}

/** @} */ /* End of group APP_LEA_CAP_ACC_GAP_CALLBACK */
/** @} */ /* End of group APP_LEA_CAP_ACC */
