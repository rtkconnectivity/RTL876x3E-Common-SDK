/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include <stdlib.h>
#include "data_uart.h"
#include "mcp_def.h"
#include "ccp_def.h"
#include "app_lea_cap_acc_content.h"
#include "app_lea_cap_acc_main.h"

#if CCP_CALL_CONTROL_CLIENT
T_LEA_CALL_INDEX *app_lea_call_idx_find(T_APP_LEA_CONTENT_DB *p_content,
                                        uint8_t call_index)
{
    T_LEA_CALL_INDEX *p_call;
    if (p_content == NULL)
    {
        return NULL;
    }
    if (p_content->type == APP_LEA_CONTENT_TYPE_TBS)
    {
        for (uint8_t i = 0; i < p_content->param.content_ccp.call_idx_queue.count; i++)
        {
            p_call = (T_LEA_CALL_INDEX *)os_queue_peek(&p_content->param.content_ccp.call_idx_queue, i);
            if (p_call != NULL && p_call->call_index == call_index)
            {
                return p_call;
            }
        }
    }
    return NULL;
}

T_LEA_CALL_INDEX *app_lea_call_idx_add(T_APP_LEA_CONTENT_DB *p_content,
                                       uint8_t call_index, uint8_t call_state, uint8_t call_flags)
{
    T_LEA_CALL_INDEX *p_call;

    if (p_content->type != APP_LEA_CONTENT_TYPE_TBS)
    {
        return NULL;
    }

    p_call = app_lea_call_idx_find(p_content, call_index);
    if (p_call)
    {
        return p_call;
    }
    p_call = calloc(1, sizeof(T_LEA_CALL_INDEX));
    if (p_call)
    {
        p_call->call_index = call_index;
        p_call->call_state = call_state;
        p_call->call_flags = call_flags;
        APP_PRINT_INFO3("app_lea_call_idx_add: call_index 0x%x, call_state %d, call_flags 0x%x",
                        call_index, call_state, call_flags);
        os_queue_in(&p_content->param.content_ccp.call_idx_queue, p_call);
    }
    return p_call;
}

void app_lea_call_idx_del(T_APP_LEA_CONTENT_DB *p_content, uint8_t call_index)
{
    T_LEA_CALL_INDEX *p_call = app_lea_call_idx_find(p_content, call_index);
    if (p_call)
    {
        if (p_call->call_uri != NULL)
        {
            free(p_call->call_uri);
        }
        os_queue_delete(&p_content->param.content_ccp.call_idx_queue, p_call);
        free(p_call);
    }
}
#endif

#if (CCP_CALL_CONTROL_CLIENT || MCP_MEDIA_CONTROL_CLIENT)
#if MCP_MEDIA_CONTROL_CLIENT
void app_lea_media_state_print(uint8_t media_state)
{
    if (media_state == MCS_MEDIA_STATE_INACTIVE)
    {
        data_uart_print("(MCS_MEDIA_STATE_INACTIVE)\r\n");
    }
    else if (media_state == MCS_MEDIA_STATE_PLAYING)
    {
        data_uart_print("(MCS_MEDIA_STATE_PLAYING)\r\n");
    }
    else if (media_state == MCS_MEDIA_STATE_PAUSED)
    {
        data_uart_print("(MCS_MEDIA_STATE_PAUSED)\r\n");
    }
    else if (media_state == MCS_MEDIA_STATE_SEEKING)
    {
        data_uart_print("(MCS_MEDIA_STATE_SEEKING)\r\n");
    }
}
#endif

#if CCP_CALL_CONTROL_CLIENT
void app_lea_call_state_print(uint8_t call_state)
{
    if (call_state == TBS_CALL_STATE_INCOMING)
    {
        data_uart_print("(TBS_CALL_STATE_INCOMING)\r\n");
    }
    else if (call_state == TBS_CALL_STATE_DIALING)
    {
        data_uart_print("(TBS_CALL_STATE_DIALING)\r\n");
    }
    else if (call_state == TBS_CALL_STATE_ALERTING)
    {
        data_uart_print("(TBS_CALL_STATE_ALERTING)\r\n");
    }
    else if (call_state == TBS_CALL_STATE_ACTIVE)
    {
        data_uart_print("(TBS_CALL_STATE_ACTIVE)\r\n");
    }
    else if (call_state == TBS_CALL_STATE_LOCALLY_HELD)
    {
        data_uart_print("(TBS_CALL_STATE_LOCALLY_HELD)\r\n");
    }
    else if (call_state == TBS_CALL_STATE_REMOTELY_HELD)
    {
        data_uart_print("(TBS_CALL_STATE_REMOTELY_HELD)\r\n");
    }
    else if (call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)
    {
        data_uart_print("(TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD)\r\n");
    }
}

#endif

void app_lea_context_print(void)
{
    T_APP_LE_LINK *p_link;
    T_APP_LEA_CONTENT_DB *p_content;

    for (uint8_t i = 0; i < APP_MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true)
        {
            p_link = &app_db.le_link[i];

            data_uart_print("LEA Context: conn handle 0x%4x \r\n", p_link->conn_handle);

            for (uint8_t j = 0; j < p_link->content_queue.count; j++)
            {
                p_content = (T_APP_LEA_CONTENT_DB *)os_queue_peek(&p_link->content_queue, j);
                if (p_content)
                {
#if MCP_MEDIA_CONTROL_CLIENT
                    if (p_content->type == APP_LEA_CONTENT_TYPE_MCS)
                    {
                        data_uart_print("MCS[%d]: general %d, srv_instance_id %d, ccid %d, media_state %d ",
                                        i,  p_content->general, p_content->srv_instance_id,
                                        p_content->ccid, p_content->param.content_mcp.media_state);
                        app_lea_media_state_print(p_content->param.content_mcp.media_state);
                    }
#endif
#if CCP_CALL_CONTROL_CLIENT
                    if (p_content->type == APP_LEA_CONTENT_TYPE_TBS)
                    {
                        data_uart_print("TBS[%d]: general %d, srv_instance_id %d, ccid %d \r\n",
                                        i,  p_content->general, p_content->srv_instance_id, p_content->ccid);
                        for (uint8_t k = 0; k < p_content->param.content_ccp.call_idx_queue.count; k++)
                        {
                            T_LEA_CALL_INDEX *p_call = (T_LEA_CALL_INDEX *)os_queue_peek(
                                                           &p_content->param.content_ccp.call_idx_queue, k);
                            if (p_call != NULL)
                            {
                                data_uart_print("    Call[%d]: call_index %d, call_flags %d, call_uri_len %d, call_state %d ",
                                                k,  p_call->call_index, p_call->call_flags, p_call->call_uri_len, p_call->call_state);
                                app_lea_call_state_print(p_call->call_state);
                            }
                        }
                    }
#endif
                }
            }
        }
    }
}

T_APP_LEA_CONTENT_DB *app_lea_content_find_by_ccid(T_APP_LE_LINK *p_link, uint8_t ccid)
{
    T_APP_LEA_CONTENT_DB *p_content;
    for (uint8_t i = 0; i < p_link->content_queue.count; i++)
    {
        p_content = (T_APP_LEA_CONTENT_DB *)os_queue_peek(&p_link->content_queue, i);
        if (p_content != NULL && p_content->ccid == ccid)
        {
            return p_content;
        }
    }
    return NULL;
}

T_APP_LEA_CONTENT_DB *app_lea_content_find(uint16_t conn_handle, T_APP_LEA_CONTENT_TYPE type,
                                           uint8_t srv_instance_id, bool general)
{
    T_APP_LE_LINK *p_link;
    T_APP_LEA_CONTENT_DB *p_content;

    p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    if (p_link == NULL)
    {
        return NULL;
    }

    for (uint8_t i = 0; i < p_link->content_queue.count; i++)
    {
        p_content = (T_APP_LEA_CONTENT_DB *)os_queue_peek(&p_link->content_queue, i);
        if (p_content != NULL && p_content->type == type &&
            p_content->srv_instance_id == srv_instance_id && p_content->general == general)
        {
            return p_content;
        }
    }
    return NULL;
}

T_APP_LEA_CONTENT_DB *app_lea_content_add(uint16_t conn_handle, T_APP_LEA_CONTENT_TYPE type,
                                          uint8_t srv_instance_id, bool general)
{
    T_APP_LE_LINK *p_link;
    T_APP_LEA_CONTENT_DB *p_content;

    p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    if (p_link == NULL)
    {
        return NULL;
    }

    p_content = app_lea_content_find(conn_handle, type, srv_instance_id, general);
    if (p_content)
    {
        return p_content;
    }
    p_content = calloc(1, sizeof(T_APP_LEA_CONTENT_DB));
    if (p_content)
    {
        p_content->type = type;
        p_content->srv_instance_id = srv_instance_id;
        p_content->general = general;
        APP_PRINT_INFO4("app_lea_content_add: conn_handle 0x%x, type %d, srv_instance_id %d, general %d",
                        p_link->conn_handle, type, srv_instance_id, general);
        os_queue_in(&p_link->content_queue, p_content);
    }
    return p_content;
}

void app_lea_content_free(T_APP_LE_LINK *p_link)
{
    T_APP_LEA_CONTENT_DB *p_content;
    while ((p_content = (T_APP_LEA_CONTENT_DB *)os_queue_out(&p_link->content_queue)) != NULL)
    {
#if CCP_CALL_CONTROL_CLIENT
        if (p_content->type == APP_LEA_CONTENT_TYPE_TBS)
        {
            T_LEA_CALL_INDEX *p_call;
            while ((p_call = (T_LEA_CALL_INDEX *)os_queue_out(&p_content->param.content_ccp.call_idx_queue)) !=
                   NULL)
            {
                if (p_call->call_uri != NULL)
                {
                    free(p_call->call_uri);
                }
                os_queue_delete(&p_content->param.content_ccp.call_idx_queue, p_call);
                free(p_call);
            }
        }
#endif
        free(p_content);
    }
}
#endif
