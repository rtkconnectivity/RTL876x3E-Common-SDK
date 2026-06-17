/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include "trace.h"
#include "data_uart.h"
#include "ccp_client.h"
#include "app_lea_cap_acc_ccp.h"
#include "app_lea_cap_acc_link.h"
#include "app_lea_cap_acc_content.h"

#if CCP_CALL_CONTROL_CLIENT

#define CCP_CHAR_CALL_STATE_MEM_LEN   3

void app_lea_acc_ccp_control(T_APP_LEA_CCP_CALL_ACTION action, T_APP_LEA_CCP_CALL_DATA *p_data)
{
    T_APP_LE_LINK *p_link;
    T_LEA_CALL_INDEX *p_call = NULL;
    T_CCP_CLIENT_WRITE_CALL_CP_PARAM write_call_cp_param = {0};
    T_APP_LEA_CONTENT_DB *p_ccp_content = NULL;

    p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);
    if (p_link == NULL)
    {
        return;
    }
    p_ccp_content = app_lea_content_find_by_ccid(p_link, p_data->ccid);

    if (p_ccp_content == NULL)
    {
        return;
    }

    p_call = app_lea_call_idx_find(p_ccp_content, p_data->call_index);

    if (p_call == NULL)
    {
        return;
    }
    switch (action)
    {
    case APP_LEA_CCP_CALL_ANSWER:
        {
            if (p_call->call_state == TBS_CALL_STATE_INCOMING)
            {
                write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ACCEPT;
                write_call_cp_param.param.accept_opcode_call_index = p_call->call_index;

                ccp_client_write_call_cp(p_link->conn_handle, p_ccp_content->srv_instance_id,
                                         p_ccp_content->general, false, &write_call_cp_param);
            }
        }
        break;

    case APP_LEA_CCP_CALL_REJECT:
        {
            if (p_call->call_state == TBS_CALL_STATE_INCOMING)
            {
                write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE;
                write_call_cp_param.param.terminate_opcode_call_index = p_call->call_index;

                ccp_client_write_call_cp(p_link->conn_handle, p_ccp_content->srv_instance_id,
                                         p_ccp_content->general, false, &write_call_cp_param);
            }
        }
        break;

    case APP_LEA_CCP_CALL_TERMINATE:
        {
            write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE;
            write_call_cp_param.param.terminate_opcode_call_index = p_call->call_index;

            ccp_client_write_call_cp(p_link->conn_handle, p_ccp_content->srv_instance_id,
                                     p_ccp_content->general, false, &write_call_cp_param);

        }
        break;

    case APP_LEA_CCP_CALL_LOCAL_HOLD:
        {
            if (p_call->call_state == TBS_CALL_STATE_ACTIVE ||
                p_call->call_state == TBS_CALL_STATE_REMOTELY_HELD)
            {
                write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_HOLD;
                write_call_cp_param.param.local_hold_opcode_call_index = p_call->call_index;

                ccp_client_write_call_cp(p_link->conn_handle, p_ccp_content->srv_instance_id,
                                         p_ccp_content->general, false,
                                         &write_call_cp_param);
            }
        }
        break;

    case APP_LEA_CCP_CALL_LOCAL_RETRIEVE:
        {
            if ((p_call->call_state == TBS_CALL_STATE_LOCALLY_HELD) ||
                (p_call->call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD))
            {
                write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_RETRIEVE;
                write_call_cp_param.param.local_retrieve_opcode_call_index = p_call->call_index;

                ccp_client_write_call_cp(p_link->conn_handle, p_ccp_content->srv_instance_id,
                                         p_ccp_content->general, false,
                                         &write_call_cp_param);
            }
        }
        break;

    default:
        break;
    }
}

static void app_lea_acc_ccp_handle_call_state(T_APP_LEA_CONTENT_DB *p_content,
                                              T_CCP_CLIENT_CALL_STATE call_state_info)
{
    uint8_t call_index;
    uint8_t call_state;
    uint8_t call_flags;
    T_LEA_CALL_INDEX *p_call;
    uint8_t call_list_num = 0;

    if (call_state_info.call_state_len < CCP_CHAR_CALL_STATE_MEM_LEN)
    {
        return;
    }

    call_list_num = call_state_info.call_state_len / CCP_CHAR_CALL_STATE_MEM_LEN;

    for (uint8_t i = 0; i < call_list_num; i++)
    {
        call_index = call_state_info.p_call_state[i * CCP_CHAR_CALL_STATE_MEM_LEN];
        call_state = call_state_info.p_call_state[i * CCP_CHAR_CALL_STATE_MEM_LEN + 1];
        call_flags = call_state_info.p_call_state[i * CCP_CHAR_CALL_STATE_MEM_LEN + 2];

        data_uart_print("Call State[%d]: call_index %d, call_flags %d, call_state %d ",
                        i,  call_index, call_flags, call_state);
        app_lea_call_state_print(call_state);

        p_call = app_lea_call_idx_find(p_content, call_index);
        if (p_call != NULL)
        {
            p_call->call_state = call_state;
            p_call->call_flags = call_flags;
        }
        else
        {
            app_lea_call_idx_add(p_content, call_index, call_state, call_flags);
        }
    }
}

static void app_lea_acc_ccp_handle_call_info(T_APP_LEA_CONTENT_DB *p_content,
                                             T_CCP_CLIENT_NOTIFY *p_notify_data)
{
    uint8_t call_index;
    uint8_t call_state;
    uint8_t call_flags;
    T_LEA_CALL_INDEX *p_call;

    if (p_notify_data->char_uuid == TBS_UUID_CHAR_CALL_STATE)
    {
        app_lea_acc_ccp_handle_call_state(p_content, p_notify_data->data.call_state);
    }
    else if (p_notify_data->char_uuid == TBS_UUID_CHAR_BEARER_LIST_CURRENT_CALLS)
    {
        uint8_t item_len = 0;
        if (p_notify_data->data.bearer_list_current_calls.bearer_list_current_calls_len == 0)
        {
            return;
        }
        for (uint8_t i = 0; i < p_notify_data->data.bearer_list_current_calls.bearer_list_current_calls_len;
            )
        {
            item_len = p_notify_data->data.bearer_list_current_calls.p_bearer_list_current_calls[i];
            call_index = p_notify_data->data.bearer_list_current_calls.p_bearer_list_current_calls[i + 1];
            call_state = p_notify_data->data.bearer_list_current_calls.p_bearer_list_current_calls[i + 2];
            call_flags = p_notify_data->data.bearer_list_current_calls.p_bearer_list_current_calls[i + 3];
            i += item_len + 1;

            data_uart_print("Bearer List Current call: call_index %d, call_flags %d, call_state %d ",
                            call_index, call_flags, call_state);
            app_lea_call_state_print(call_state);

            p_call = app_lea_call_idx_find(p_content, call_index);
            if (p_call != NULL)
            {
                p_call->call_state = call_state;
                p_call->call_flags = call_flags;
            }
            else
            {
                app_lea_call_idx_add(p_content, call_index, call_state, call_flags);
            }
        }
    }
    else if (p_notify_data->char_uuid == TBS_UUID_CHAR_INCOMING_CALL)
    {
        uint8_t  *p_buf;
        uint8_t  *p_uri;
        uint16_t uri_len;

        if (p_notify_data->data.incoming_call.uri_len == 0)
        {
            return;
        }
        call_index = p_notify_data->data.incoming_call.call_index;
        p_uri = p_notify_data->data.incoming_call.p_uri;
        uri_len = p_notify_data->data.incoming_call.uri_len;

        data_uart_print("Incoming call : call_index %d, call_uri_len %d \r\n", call_index, uri_len);

        p_buf = calloc(1, uri_len);
        if (p_buf != NULL)
        {
            p_call = app_lea_call_idx_find(p_content, call_index);
            if (p_call == NULL)
            {
                p_call = app_lea_call_idx_add(p_content, call_index, TBS_CALL_STATE_INCOMING, 0);
            }
            if (p_call)
            {
                if (p_call->call_uri != NULL)
                {
                    free(p_call->call_uri);
                    p_call->call_uri = NULL;
                    p_call->call_uri_len = 0;
                }
                p_call->call_uri = p_buf;
                p_call->call_uri_len = uri_len;
                memcpy(p_call->call_uri, p_uri, uri_len);

                APP_PRINT_INFO1("app_lea_acc_ccp_handle_call_info: incoming call, call_index %d",
                                call_index);
            }
            else
            {
                free(p_buf);
            }
        }
    }
}

static void app_lea_acc_ccp_handle_termination(T_APP_LEA_CONTENT_DB *p_content, uint8_t call_index)
{
    app_lea_call_idx_del(p_content, call_index);
}

uint16_t app_lea_acc_ccp_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_LEA_CONTENT_DB *p_content;
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_CCP_CLIENT_DIS_DONE:
        {
            T_CCP_CLIENT_DIS_DONE *p_dis_done = (T_CCP_CLIENT_DIS_DONE *)buf;

            APP_PRINT_INFO5("LE_AUDIO_MSG_CCP_CLIENT_DIS_DONE: conn_handle 0x%x, gtbs %d, is_found %d, load_from_ftl %d, srv_num %d",
                            p_dis_done->conn_handle, p_dis_done->gtbs, p_dis_done->is_found,
                            p_dis_done->load_from_ftl, p_dis_done->srv_num);

            if (p_dis_done->is_found)
            {
                uint16_t cfg_flags = CCP_CLIENT_CFG_CCCD_FLAG_BEARER_LIST_CURRENT_CALLS |
                                     CCP_CLIENT_CFG_CCCD_FLAG_STATUS_FLAGS | CCP_CLIENT_CFG_CCCD_FLAG_CALL_STATE |
                                     CCP_CLIENT_CFG_CCCD_FLAG_CALL_CONTROL_POINT | CCP_CLIENT_CFG_CCCD_FLAG_TERMINATION_REASON |
                                     CCP_CLIENT_CFG_CCCD_FLAG_INCOMING_CALL;
                if (p_dis_done->gtbs)
                {
                    ccp_client_cfg_cccd(p_dis_done->conn_handle, 0, cfg_flags, true, p_dis_done->gtbs);
                    ccp_client_read_char_value(p_dis_done->conn_handle, 0, TBS_UUID_CHAR_CONTENT_CONTROL_ID,
                                               p_dis_done->gtbs);
                    ccp_client_read_char_value(p_dis_done->conn_handle, 0, TBS_UUID_CHAR_CALL_STATE, p_dis_done->gtbs);
                    app_lea_content_add(p_dis_done->conn_handle, APP_LEA_CONTENT_TYPE_TBS, 0, p_dis_done->gtbs);
                }
#if !CCP_CALL_CONTROL_CLIENT_GTBS_ONLY
                else
                {
                    for (uint8_t i = 0; i < p_dis_done->srv_num; i++)
                    {
                        ccp_client_cfg_cccd(p_dis_done->conn_handle, i, cfg_flags, true, p_dis_done->gtbs);
                        ccp_client_read_char_value(p_dis_done->conn_handle, i, TBS_UUID_CHAR_CONTENT_CONTROL_ID,
                                                   p_dis_done->gtbs);
                        ccp_client_read_char_value(p_dis_done->conn_handle, i, TBS_UUID_CHAR_CALL_STATE, p_dis_done->gtbs);
                        app_lea_content_add(p_dis_done->conn_handle, APP_LEA_CONTENT_TYPE_TBS, i, p_dis_done->gtbs);
                    }
                }
#endif
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_READ_RESULT:
        {
            T_CCP_CLIENT_READ_RESULT *p_read_result = (T_CCP_CLIENT_READ_RESULT *)buf;
            APP_PRINT_INFO5("LE_AUDIO_MSG_CCP_CLIENT_READ_RESULT: conn_handle 0x%x, gtbs %d, srv_instance_id %d, char_uuid 0x%x, cause 0x%x",
                            p_read_result->conn_handle, p_read_result->gtbs, p_read_result->srv_instance_id,
                            p_read_result->char_uuid, p_read_result->cause);
            if (p_read_result->cause == GAP_SUCCESS)
            {
                p_content = app_lea_content_find(p_read_result->conn_handle, APP_LEA_CONTENT_TYPE_TBS,
                                                 p_read_result->srv_instance_id, p_read_result->gtbs);
                if (p_content)
                {
                    if (p_read_result->char_uuid == TBS_UUID_CHAR_CONTENT_CONTROL_ID)
                    {
                        p_content->ccid = p_read_result->data.content_control_id;
                        data_uart_print("TBS: Read CCID 0x%2x\r\n", p_read_result->data.content_control_id);
                    }
                    else if (p_read_result->char_uuid == TBS_UUID_CHAR_CALL_STATE)
                    {
                        app_lea_acc_ccp_handle_call_state(p_content, p_read_result->data.call_state);
                    }
                }

                if (p_read_result->char_uuid == TBS_UUID_CHAR_BEARER_PROVIDER_NAME)
                {
                    APP_PRINT_INFO2("TBS_UUID_CHAR_BEARER_PROVIDER_NAME: len %d, data %b",
                                    p_read_result->data.bearer_provider_name.bearer_provider_name_len,
                                    TRACE_BINARY(p_read_result->data.bearer_provider_name.bearer_provider_name_len,
                                                 p_read_result->data.bearer_provider_name.p_bearer_provider_name));
                }
                else if (p_read_result->char_uuid == TBS_UUID_CHAR_BEARER_UCI)
                {
                    APP_PRINT_INFO2("TBS_UUID_CHAR_BEARER_UCI: len %d, data %b",
                                    p_read_result->data.bearer_uci.bearer_uci_len,
                                    TRACE_BINARY(p_read_result->data.bearer_uci.bearer_uci_len,
                                                 p_read_result->data.bearer_uci.p_bearer_uci));
                }
                else if (p_read_result->char_uuid == TBS_UUID_CHAR_BEARER_TECHNOLOGY)
                {
                    APP_PRINT_INFO2("TBS_UUID_CHAR_BEARER_TECHNOLOGY: len %d, data %b",
                                    p_read_result->data.bearer_technology.bearer_technology_len,
                                    TRACE_BINARY(p_read_result->data.bearer_technology.bearer_technology_len,
                                                 p_read_result->data.bearer_technology.p_bearer_technology));
                }
                else if (p_read_result->char_uuid == TBS_UUID_CHAR_BEARER_URI_SCHEMES_SUPPORTED_LIST)
                {
                    APP_PRINT_INFO2("TBS_UUID_CHAR_BEARER_URI_SCHEMES_SUPPORTED_LIST: len %d, data %b",
                                    p_read_result->data.bearer_uri_schemes_supported_list.bearer_uri_schemes_supported_list_len,
                                    TRACE_BINARY(
                                        p_read_result->data.bearer_uri_schemes_supported_list.bearer_uri_schemes_supported_list_len,
                                        p_read_result->data.bearer_uri_schemes_supported_list.p_bearer_uri_schemes_supported_list));
                }
                else if (p_read_result->char_uuid == TBS_UUID_CHAR_STATUS_FLAGS)
                {
                    APP_PRINT_INFO1("TBS_UUID_CHAR_STATUS_FLAGS: status_flags 0x%x",
                                    p_read_result->data.status_flags);
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_NOTIFY:
        {
            T_CCP_CLIENT_NOTIFY *p_notify_data = (T_CCP_CLIENT_NOTIFY *)buf;

            APP_PRINT_INFO4("LE_AUDIO_MSG_CCP_CLIENT_NOTIFY: conn_handle 0x%x, gtbs %d, srv_instance_id %d, char_uuid 0x%x",
                            p_notify_data->conn_handle, p_notify_data->gtbs, p_notify_data->srv_instance_id,
                            p_notify_data->char_uuid);

            p_content = app_lea_content_find(p_notify_data->conn_handle, APP_LEA_CONTENT_TYPE_TBS,
                                             p_notify_data->srv_instance_id, p_notify_data->gtbs);
            if (p_content)
            {
                if (p_notify_data->char_uuid == TBS_UUID_CHAR_TERMINATION_REASON)
                {
                    data_uart_print("Termination reason : call_index %d, reason_code %d\r\n",
                                    p_notify_data->data.termination_reason.call_index,
                                    p_notify_data->data.termination_reason.reason_code);
                    app_lea_acc_ccp_handle_termination(p_content, p_notify_data->data.termination_reason.call_index);
                }
                else if (p_notify_data->char_uuid == TBS_UUID_CHAR_CALL_STATE ||
                         p_notify_data->char_uuid == TBS_UUID_CHAR_BEARER_LIST_CURRENT_CALLS ||
                         p_notify_data->char_uuid == TBS_UUID_CHAR_INCOMING_CALL)
                {
                    app_lea_acc_ccp_handle_call_info(p_content, p_notify_data);
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_CALL_CP_NOTIFY:
        {
            T_CCP_CLIENT_CALL_CP_NOTIFY *p_cp_notify = (T_CCP_CLIENT_CALL_CP_NOTIFY *)buf;
            APP_PRINT_INFO6("LE_AUDIO_MSG_CCP_CLIENT_CALL_CP_NOTIFY: conn_handle 0x%x, gtbs %d, srv_instance_id %d, requested_opcode 0x%x, call_index 0x%x, result_code 0x%x",
                            p_cp_notify->conn_handle, p_cp_notify->gtbs, p_cp_notify->srv_instance_id,
                            p_cp_notify->requested_opcode, p_cp_notify->call_index, p_cp_notify->result_code);
        }
        break;

    default:
        break;
    }
    return cb_result;
}

void app_lea_acc_ccp_init_cap(T_CAP_INIT_PARAMS *p_param)
{
    p_param->ccp_call_control_client = true;
    ble_audio_cback_register(app_lea_acc_ccp_handle_msg);
}
#endif
