/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if CCP_CALL_CONTROL_SERVER
#include <string.h>
#include "trace.h"
#include "app_lea_ini_ccp.h"
#include "app_main.h"

static T_TBS_CALL_DB *app_lea_ini_ccp_call_find(uint8_t call_index)
{
    T_TBS_CALL_DB *p_call = NULL;

    APP_PRINT_INFO1("app_lea_ini_ccp_call_find: call_index 0x%x", call_index);
    for (uint8_t i = 0; i < APP_LEA_TBS_CALL_LIST_MAX; i++)
    {
        p_call = (T_TBS_CALL_DB *)&app_db.ccp_db.call_list[i];
        if (call_index == p_call->call_index)
        {
            return p_call;
        }
    }

    APP_PRINT_INFO0("app_lea_ini_ccp_call_find: NULL");
    return NULL;
}

static T_TBS_CALL_DB *app_lea_ini_ccp_call_allocate(uint8_t call_index)
{
    T_TBS_CALL_DB *p_call = NULL;

    APP_PRINT_INFO1("app_lea_ini_ccp_call_allocate: call_index 0x%x", call_index);

    for (uint8_t i = 0; i < APP_LEA_TBS_CALL_LIST_MAX; i++)
    {
        p_call = (T_TBS_CALL_DB *)&app_db.ccp_db.call_list[i];
        if (p_call->call_index == 0)
        {
            p_call->call_index = call_index;
            p_call->call_state = TBS_CALL_STATE_RFU;
            return p_call;
        }
    }
    APP_PRINT_WARN1("app_lea_ini_ccp_call_allocate: call_index 0x%x failed", call_index);
    return NULL;
}

static void app_lea_ini_ccp_call_release(uint8_t call_index)
{
    T_TBS_CALL_DB *p_call = NULL;

    p_call = app_lea_ini_ccp_call_find(call_index);
    if (!p_call)
    {
        return;
    }

    APP_PRINT_INFO1("app_lea_ini_ccp_call_release: call_index 0x%x", call_index);

    p_call->call_index = 0;
    p_call->call_state = TBS_CALL_STATE_RFU;
    p_call->call_flags = 0;
}

void app_lea_ini_ccp_send_input_msg(T_APP_TBS_INPUT_MSG msg, T_APP_TBS_INPUT_MSG_DATA *p_data)
{
    switch (msg)
    {
    case APP_TBS_INPUT_MSG_CALL_INFO:
        {
            APP_PRINT_INFO3("APP_TBS_INPUT_MSG_CALL_INFO: call_index %d, call_flags 0x%2x\r\n, call_state %d",
                            p_data->call_info.call_index, p_data->call_info.call_flags, p_data->call_info.call_state);
            // switch (p_data->call_info.call_state)
            // {
            // case TBS_CALL_STATE_INCOMING:
            //     {
            //         data_uart_print("TBS_CALL_STATE_INCOMING\r\n");
            //     }
            //     break;
            // case TBS_CALL_STATE_DIALING:
            //     {
            //         data_uart_print("TBS_CALL_STATE_DIALING\r\n");
            //     }
            //     break;
            // case TBS_CALL_STATE_ALERTING:
            //     {
            //         data_uart_print("TBS_CALL_STATE_ALERTING\r\n");
            //     }
            //     break;
            // case TBS_CALL_STATE_ACTIVE:
            //     {
            //         data_uart_print("TBS_CALL_STATE_ACTIVE\r\n");
            //     }
            //     break;
            // case TBS_CALL_STATE_LOCALLY_HELD:
            //     {
            //         data_uart_print("TBS_CALL_STATE_LOCALLY_HELD\r\n");
            //     }
            //     break;
            // case TBS_CALL_STATE_REMOTELY_HELD:
            //     {
            //         data_uart_print("TBS_CALL_STATE_REMOTELY_HELD\r\n");
            //     }
            //     break;
            // case TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD:
            //     {
            //         data_uart_print("TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD\r\n");
            //     }
            //     break;
            // default:
            //     break;
            // }
        }
        break;

    case APP_TBS_INPUT_MSG_CALL_TERMINATE:
        {
            APP_PRINT_INFO2("APP_TBS_INPUT_MSG_CALL_TERMINATE: call_index %d, reason_code %d\r\n",
                            p_data->call_terminate.call_index,
                            p_data->call_terminate.reason_code);
        }
        break;

    default:
        break;
    }
}

void app_lea_ini_ccp_update_call_state(T_TBS_CALL_DB *p_call, T_TBS_CALL_STATE state)
{
    T_APP_TBS_INPUT_MSG_DATA input_data;

    APP_PRINT_TRACE2("app_lea_ini_ccp_update_call_state: call_index 0x%x, call_state 0x%x",
                     p_call->call_index, state);

    p_call->call_state = state;
    ccp_server_update_call_state_by_call_index(app_db.ccp_db.gtbs_id, p_call->call_index,
                                               state, p_call->call_flags, true);


    input_data.call_info.call_index = p_call->call_index;
    input_data.call_info.call_flags = p_call->call_flags;
    input_data.call_info.call_state = p_call->call_state;
    app_lea_ini_ccp_send_input_msg(APP_TBS_INPUT_MSG_CALL_INFO, &input_data);
}

static uint8_t app_lea_ini_ccp_create_call(bool outing_call, T_APP_TBS_CALL_URI call_uri)
{
    uint8_t call_index = 0;
    T_TBS_CALL_DB *p_call = NULL;

    call_index = ccp_server_create_call(app_db.ccp_db.gtbs_id, call_uri.p_call_uri,
                                        call_uri.call_uri_len);

    if (call_index != 0)
    {
        p_call = app_lea_ini_ccp_call_allocate(call_index);

        if (p_call)
        {
            if (outing_call)
            {
                p_call->call_flags = (1 << TBS_CALL_FLAGS_BIT_INCOMING_OUTGOING);
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_DIALING);
            }
            else
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_INCOMING);
            }
        }
    }
    if (call_index == 0)
    {
        APP_PRINT_ERROR0("app_lea_ini_ccp_create_call: fail");
    }

    app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_CCP_CREATE_CALL, 0,
                     (uint8_t *)&call_index, sizeof(call_index));
    return call_index;
}

static bool app_lea_ini_ccp_terminate_call(T_CCP_SERVER_TERMINATION_REASON call_termination)
{
    T_TBS_CALL_DB *p_call = NULL;

    p_call = app_lea_ini_ccp_call_find(call_termination.call_index);
    if (!p_call)
    {
        return false;
    }
    APP_PRINT_INFO1("app_lea_ini_ccp_terminate_call: call_index 0x%x", call_termination.call_index);

    if (ccp_server_terminate_call(app_db.ccp_db.gtbs_id, &call_termination))
    {
        app_lea_ini_ccp_call_release(call_termination.call_index);
        T_APP_TBS_INPUT_MSG_DATA input_data;
        input_data.call_terminate.call_index = call_termination.call_index;
        input_data.call_terminate.reason_code = call_termination.reason_code;
        app_lea_ini_ccp_send_input_msg(APP_TBS_INPUT_MSG_CALL_TERMINATE, &input_data);
    }

    return true;
}

void app_lea_ini_ccp_control(T_APP_TBS_OUTPUT_MSG msg, T_APP_TBS_OUTPUT_MSG_DATA *p_data)
{
    T_TBS_CALL_DB *p_call = NULL;
    if (p_data == NULL)
    {
        return;
    }

    switch (msg)
    {
    case APP_TBS_OUTPUT_MSG_REMOTE_TERMINATE:
        {
            p_call = app_lea_ini_ccp_call_find(p_data->remote_terminate.call_index);
            if (!p_call)
            {
                return;
            }
            app_lea_ini_ccp_terminate_call(p_data->remote_terminate);
        }
        break;

    case APP_TBS_OUTPUT_MSG_LOCAL_TERMINATE:
        {
            p_call = app_lea_ini_ccp_call_find(p_data->local_terminate.call_index);
            if (!p_call)
            {
                return;
            }
            app_lea_ini_ccp_terminate_call(p_data->local_terminate);
        }
        break;

    case APP_TBS_OUTPUT_MSG_REMOTE_INCOMING_CALL:
        {
            app_lea_ini_ccp_create_call(false, p_data->remote_incoming_call);
        }
        break;

    case APP_TBS_OUTPUT_MSG_LOCAL_ORIGINATE:
        {
            app_lea_ini_ccp_create_call(true, p_data->local_originate);
        }
        break;

    case APP_TBS_OUTPUT_MSG_REMOTE_ALERT_START:
        {
            p_call = app_lea_ini_ccp_call_find(p_data->call_index);
            if (!p_call)
            {
                return;
            }

            if (p_call->call_state == TBS_CALL_STATE_DIALING)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_ALERTING);
            }
        }
        break;

    case APP_TBS_OUTPUT_MSG_REMOTE_ANSWER:
        {
            p_call = app_lea_ini_ccp_call_find(p_data->call_index);
            if (!p_call)
            {
                return;
            }

            if (p_call->call_state == TBS_CALL_STATE_ALERTING)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_ACTIVE);
            }
        }
        break;

    case APP_TBS_OUTPUT_MSG_LOCAL_ACCEPT:
        {
            p_call = app_lea_ini_ccp_call_find(p_data->call_index);
            if (!p_call)
            {
                return;
            }

            if (p_call->call_state == TBS_CALL_STATE_INCOMING)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_ACTIVE);
            }
        }
        break;

    case APP_TBS_OUTPUT_MSG_LOCAL_HOLD:
        {
            p_call = app_lea_ini_ccp_call_find(p_data->call_index);
            if (!p_call)
            {
                return;
            }

            if (p_call->call_state == TBS_CALL_STATE_ACTIVE ||
                p_call->call_state == TBS_CALL_STATE_INCOMING)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_LOCALLY_HELD);
            }
            else if (p_call->call_state == TBS_CALL_STATE_REMOTELY_HELD)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
            }
        }
        break;

    case APP_TBS_OUTPUT_MSG_REMOTE_HOLD:
        {
            p_call = app_lea_ini_ccp_call_find(p_data->call_index);
            if (!p_call)
            {
                return;
            }

            if (p_call->call_state == TBS_CALL_STATE_ACTIVE)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_REMOTELY_HELD);
            }
            else if (p_call->call_state == TBS_CALL_STATE_LOCALLY_HELD)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
            }
        }
        break;

    case APP_TBS_OUTPUT_MSG_REMOTE_RETRIEVE:
        {
            p_call = app_lea_ini_ccp_call_find(p_data->call_index);
            if (!p_call)
            {
                return;
            }

            if (p_call->call_state == TBS_CALL_STATE_REMOTELY_HELD)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_ACTIVE);
            }
            else if (p_call->call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_LOCALLY_HELD);
            }
        }
        break;

    case APP_TBS_OUTPUT_MSG_LOCAL_RETRIEVE:
        {
            p_call = app_lea_ini_ccp_call_find(p_data->call_index);
            if (!p_call)
            {
                return;
            }

            if (p_call->call_state == TBS_CALL_STATE_LOCALLY_HELD)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_ACTIVE);
            }
            else if (p_call->call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
            {
                app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_REMOTELY_HELD);
            }
        }
        break;

    default:
        break;
    }
}

static uint16_t app_lea_ini_ccp_handle_op_accept(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    uint8_t call_index = 0;
    T_TBS_CALL_DB *p_call = NULL;

    call_index = p_ccp_val->param.accept_opcode_call_index;

    p_call = app_lea_ini_ccp_call_find(call_index);
    if (!p_call)
    {
        return BLE_AUDIO_CB_RESULT_APP_ERR;
    }

    APP_PRINT_INFO2("app_lea_ini_ccp_handle_op_accept: call_index 0x%x, call_state 0x%x",
                    call_index, p_call->call_state);

    if (p_call->call_state == TBS_CALL_STATE_INCOMING)
    {
        app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_ACTIVE);
        return BLE_AUDIO_CB_RESULT_SUCCESS;
    }

    return BLE_AUDIO_CB_RESULT_APP_ERR;
}

static uint16_t app_lea_ini_ccp_handle_op_terminate(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    uint8_t call_index = 0;
    T_TBS_CALL_DB *p_call = NULL;

    call_index = p_ccp_val->param.terminate_opcode_call_index;

    p_call = app_lea_ini_ccp_call_find(call_index);
    if (!p_call)
    {
        return BLE_AUDIO_CB_RESULT_APP_ERR;
    }

    APP_PRINT_INFO2("app_lea_ini_ccp_handle_op_terminate: call_index %x, call_state %x",
                    call_index, p_call->call_state);
    T_CCP_SERVER_TERMINATION_REASON terminate_reason;
    terminate_reason.call_index = p_ccp_val->param.terminate_opcode_call_index;
    terminate_reason.reason_code = TBS_TERMINATION_REASON_CODES_CLIENT_TERMINATE_CALL;
    app_lea_ini_ccp_terminate_call(terminate_reason);
    return BLE_AUDIO_CB_RESULT_SUCCESS;
}

#if APP_LEA_TBS_LOCAL_HOLD_SUPPORT
static uint16_t app_lea_ini_ccp_handle_op_local_hold(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    uint8_t call_index = 0;
    T_TBS_CALL_DB *p_call = NULL;

    call_index = p_ccp_val->param.local_hold_opcode_call_index;

    p_call = app_lea_ini_ccp_call_find(call_index);
    if (!p_call)
    {
        return BLE_AUDIO_CB_RESULT_APP_ERR;
    }

    APP_PRINT_INFO2("app_lea_ini_ccp_handle_op_local_hold: call_index 0x%x, call_state 0x%x",
                    call_index, p_call->call_state);


    if (p_call->call_state == TBS_CALL_STATE_ACTIVE ||
        p_call->call_state == TBS_CALL_STATE_INCOMING)
    {
        app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_LOCALLY_HELD);
    }
    else if (p_call->call_state == TBS_CALL_STATE_REMOTELY_HELD)
    {
        app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD);
    }

    return BLE_AUDIO_CB_RESULT_APP_ERR;
}

static uint16_t app_lea_ini_ccp_handle_op_local_retrieve(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    uint8_t call_index = 0;
    T_TBS_CALL_DB *p_call = NULL;

    call_index = p_ccp_val->param.local_retrieve_opcode_call_index;

    p_call = app_lea_ini_ccp_call_find(call_index);
    if (!p_call)
    {
        return BLE_AUDIO_CB_RESULT_APP_ERR;
    }

    APP_PRINT_INFO2("app_lea_ini_ccp_handle_op_local_retrieve: call_index 0x%x, call_state 0x%x",
                    call_index, p_call->call_state);

    if (p_call->call_state == TBS_CALL_STATE_LOCALLY_HELD)
    {
        app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_ACTIVE);
    }
    else if (p_call->call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
    {
        app_lea_ini_ccp_update_call_state(p_call, TBS_CALL_STATE_REMOTELY_HELD);
    }

    return BLE_AUDIO_CB_RESULT_APP_ERR;
}
#endif

static uint16_t app_lea_ini_ccp_handle_op_originate(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    return BLE_AUDIO_CB_RESULT_REJECT;
}

static uint16_t app_lea_ini_ccp_handle_op_join(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    return BLE_AUDIO_CB_RESULT_REJECT;
}

uint16_t app_lea_ini_ccp_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_CCP_SERVER_WRITE_CALL_CP_IND:
        {
            T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_cp_ind = (T_CCP_SERVER_WRITE_CALL_CP_IND *)buf;

            if (p_ccp_cp_ind == NULL)
            {
                return cb_result;
            }

            APP_PRINT_INFO2("LE_AUDIO_MSG_CCP_HANDLE_CCP: conn_handle 0x%x, opcode %d",
                            p_ccp_cp_ind->conn_handle,
                            p_ccp_cp_ind->opcode);
            // data_uart_print("LE_AUDIO_MSG_CCP_HANDLE_CCP: conn_handle 0x%2x, opcode %d\r\n",
            //                 p_ccp_cp_ind->conn_handle,
            //                 p_ccp_cp_ind->opcode);
            cb_result = BLE_AUDIO_CB_RESULT_APP_ERR;
#if !F_APP_TRI_DONGLE_LEAUDIO_CCP
            switch (p_ccp_cp_ind->opcode)
            {
            case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ACCEPT:
                {
                    cb_result = app_lea_ini_ccp_handle_op_accept(p_ccp_cp_ind);
                }
                break;
            case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE:
                {
                    cb_result = app_lea_ini_ccp_handle_op_terminate(p_ccp_cp_ind);
                }
                break;
#if APP_LEA_TBS_LOCAL_HOLD_SUPPORT
            case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_HOLD:
                {
                    cb_result = app_lea_ini_ccp_handle_op_local_hold(p_ccp_cp_ind);
                }
                break;
            case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_RETRIEVE:
                {
                    cb_result = app_lea_ini_ccp_handle_op_local_retrieve(p_ccp_cp_ind);
                }
                break;
#endif
            case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ORIGINATE:
                {
                    cb_result = app_lea_ini_ccp_handle_op_originate(p_ccp_cp_ind);
                }
                break;
            case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_JOIN:
                {
                    cb_result = app_lea_ini_ccp_handle_op_join(p_ccp_cp_ind);
                }
                break;

            default:
                break;
            }
#endif
        }
        break;

    case LE_AUDIO_MSG_CCP_SERVER_READ_IND:
        {
            T_CCP_SERVER_READ_IND *p_read_ind = (T_CCP_SERVER_READ_IND *)buf;

            if (p_read_ind)
            {
                T_CCP_SERVER_READ_CFM read_cfm = {0};

                read_cfm.cause = BLE_AUDIO_CB_RESULT_SUCCESS;
                read_cfm.conn_handle = p_read_ind->conn_handle;
                read_cfm.cid = p_read_ind->cid;
                read_cfm.service_id = p_read_ind->service_id;
                read_cfm.char_uuid = p_read_ind->char_uuid;
                read_cfm.offset = p_read_ind->offset;


                switch (p_read_ind->char_uuid)
                {
                case TBS_UUID_CHAR_BEARER_PROVIDER_NAME:
                    {
                        read_cfm.param.bearer_provider_name.p_bearer_provider_name = (uint8_t *)"Provider name";
                        read_cfm.param.bearer_provider_name.bearer_provider_name_len = strlen("Provider name");
                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_BEARER_UCI:
                    {
                        read_cfm.param.bearer_uci.p_bearer_uci = (uint8_t *)"un001";
                        read_cfm.param.bearer_uci.bearer_uci_len = strlen("un001");
                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_BEARER_TECHNOLOGY:
                    {
                        uint8_t tech = TBS_BEARER_TECHNOLOGY_CHAR_VALUE_WIFI;

                        read_cfm.param.bearer_technology.p_bearer_technology = &tech;
                        read_cfm.param.bearer_technology.bearer_technology_len = 1;
                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_BEARER_URI_SCHEMES_SUPPORTED_LIST:
                    {
                        read_cfm.param.bearer_uri_schemes_supported_list.p_bearer_uri_schemes_supported_list =
                            (uint8_t *)"tel";
                        read_cfm.param.bearer_uri_schemes_supported_list.bearer_uri_schemes_supported_list_len =
                            strlen("tel");
                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_STATUS_FLAGS:
                    {
                        /* inband_enable|not silent mode */
                        read_cfm.param.status_flags = (1 << TBS_STATUS_FLAGS_CHAR_BIT_INBAND_RINGTONE);
                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_CONTENT_CONTROL_ID:
                    {
                        read_cfm.param.content_control_id = APP_LEA_GTBS_CCID;
                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                default:
                    return BLE_AUDIO_CB_RESULT_SUCCESS;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO:
        {
            T_SERVER_ATTR_CCCD_INFO *p_cccd = (T_SERVER_ATTR_CCCD_INFO *)buf;
            if (app_db.ccp_db.gtbs_id == p_cccd->service_id)
            {
                app_db.ccp_db.ccp_enabled_cccd = p_cccd->ccc_bits;
                APP_PRINT_INFO4("LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO: TBS, conn_handle 0x%x, char_uuid 0x%x, char_attrib_index %d, ccc_bits 0x%x",
                                p_cccd->conn_handle,
                                p_cccd->char_uuid,
                                p_cccd->char_attrib_index,
                                p_cccd->ccc_bits);
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

void app_lea_ini_ccp_init_cap(T_CAP_INIT_PARAMS *p_param)
{
    p_param->tbs.ccp_call_control_server = true;
    p_param->tbs.tbs_num = 1;
    ble_audio_cback_register(app_lea_ini_ccp_handle_msg);
}

void app_lea_ini_ccp_init(void)
{
    T_CCP_SERVER_REG_SRV_PARAM p_gtbs_param = {0};

    p_gtbs_param.gtbs = true;
    p_gtbs_param.char_incoming_call_target_bearer_uri.support = true;

    app_db.ccp_db.gtbs_id = ccp_server_reg_srv(&p_gtbs_param);
    if (app_db.ccp_db.gtbs_id != 0xFF)
    {
        T_CCP_SERVER_SET_PARAM param = {0};

        param.char_uuid = TBS_UUID_CHAR_CALL_CONTROL_POINT_OPTIONAL_OPCODES;
#if APP_LEA_TBS_LOCAL_HOLD_SUPPORT
        param.param.call_control_point_optional_opcodes =
            (1 << TBS_CALL_CONTROL_POINT_OPTIONAL_OPCODES_CHAR_BIT_LOCAL_HOLD);
#endif
        ccp_server_set_param(app_db.ccp_db.gtbs_id, &param);
    }
}
#endif
