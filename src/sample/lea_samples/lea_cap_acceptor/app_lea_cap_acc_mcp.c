/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "data_uart.h"
#include "app_lea_cap_acc_mcp.h"
#include "app_lea_cap_acc_link.h"
#include "app_lea_cap_acc_content.h"
#include "app_lea_cap_acc_main.h"

#if MCP_MEDIA_CONTROL_CLIENT
void app_lea_acc_mcp_cp(uint8_t ccid, T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM *p_param)
{
    T_APP_LE_LINK *p_link;
    T_APP_LEA_CONTENT_DB *p_content;

    if (p_param == NULL)
    {
        return;
    }

    for (uint8_t i = 0; i < APP_MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used)
        {
            p_link = &app_db.le_link[i];
            for (uint8_t j = 0; j < p_link->content_queue.count; j++)
            {
                p_content = (T_APP_LEA_CONTENT_DB *)os_queue_peek(&p_link->content_queue, j);
                if (p_content != NULL && p_content->type == APP_LEA_CONTENT_TYPE_MCS &&
                    p_content->active == true &&
                    p_content->ccid == ccid)
                {
                    mcp_client_write_media_cp(p_link->conn_handle, p_content->srv_instance_id, p_content->general,
                                              p_param, true);
                    APP_PRINT_INFO5("app_lea_acc_mcp_cp: ccid 0x%x, send cp 0x%x, conn_handle 0x%x, srv_instance_id %d, general %d",
                                    ccid, p_param->opcode, p_link->conn_handle,
                                    p_content->srv_instance_id, p_content->general);
                }
            }
        }
    }
}

static uint16_t app_lea_acc_mcp_update_state(uint16_t conn_handle, bool gmcs,
                                             uint8_t srv_instance_id, uint8_t media_state)
{
    T_APP_LEA_CONTENT_DB *p_content;

    p_content = app_lea_content_find(conn_handle, APP_LEA_CONTENT_TYPE_MCS,
                                     srv_instance_id, gmcs);
    if (p_content == NULL)
    {
        return BLE_AUDIO_CB_RESULT_APP_ERR;
    }
    if (p_content)
    {
        p_content->param.content_mcp.media_state = media_state;

        if (p_content->param.content_mcp.media_state != MCS_MEDIA_STATE_INACTIVE)
        {
            p_content->active = true;
        }
        else
        {
            p_content->active = false;
        }
    }
    return BLE_AUDIO_CB_RESULT_SUCCESS;
}

uint16_t app_lea_acc_mcp_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_MCP_CLIENT_DIS_DONE:
        {
            T_MCP_CLIENT_DIS_DONE *p_dis_done = (T_MCP_CLIENT_DIS_DONE *)buf;
            APP_PRINT_INFO4("app_lea_acc_mcp_handle_msg: LE_AUDIO_MSG_MCP_CLIENT_DIS_DONE, conn_handle 0x%x, gmcs %d, is_found %d, srv_num %d",
                            p_dis_done->conn_handle, p_dis_done->gmcs,
                            p_dis_done->is_found, p_dis_done->srv_num);

            if (p_dis_done->is_found)
            {
                if (p_dis_done->gmcs)
                {
                    mcp_client_read_char_value(p_dis_done->conn_handle, 0, MCS_UUID_CHAR_CONTENT_CONTROL_ID, true);
                    mcp_client_cfg_cccd(p_dis_done->conn_handle,
                                        (MCP_CLIENT_CFG_CCCD_FLAG_MEDIA_CONTROL_POINT | MCP_CLIENT_CFG_CCCD_FLAG_TRACK_CHANGED |
                                         MCP_CLIENT_CFG_CCCD_FLAG_MEDIA_STATE),
                                        true, 0, true);
                    mcp_client_read_char_value(p_dis_done->conn_handle, 0, MCS_UUID_CHAR_MEDIA_STATE, true);
                    mcp_client_read_char_value(p_dis_done->conn_handle, 0, MCS_UUID_CHAR_MEDIA_PLAYER_NAME, true);

                    app_lea_content_add(p_dis_done->conn_handle, APP_LEA_CONTENT_TYPE_MCS, 0, p_dis_done->gmcs);
                }
                else
                {
                    for (uint8_t i = 0; i < p_dis_done->srv_num; i++)
                    {
                        mcp_client_read_char_value(p_dis_done->conn_handle, i, MCS_UUID_CHAR_CONTENT_CONTROL_ID, false);
                        mcp_client_cfg_cccd(p_dis_done->conn_handle,
                                            (MCP_CLIENT_CFG_CCCD_FLAG_MEDIA_CONTROL_POINT | MCP_CLIENT_CFG_CCCD_FLAG_TRACK_CHANGED |
                                             MCP_CLIENT_CFG_CCCD_FLAG_MEDIA_STATE),
                                            false, i, true);
                        mcp_client_read_char_value(p_dis_done->conn_handle, i, MCS_UUID_CHAR_MEDIA_STATE, false);
                        mcp_client_read_char_value(p_dis_done->conn_handle, i, MCS_UUID_CHAR_MEDIA_PLAYER_NAME, false);
                        app_lea_content_add(p_dis_done->conn_handle, APP_LEA_CONTENT_TYPE_MCS, i, p_dis_done->gmcs);
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_READ_RESULT:
        {
            T_MCP_CLIENT_READ_RESULT *p_read_result = (T_MCP_CLIENT_READ_RESULT *)buf;

            APP_PRINT_INFO5("app_lea_acc_mcp_handle_msg: LE_AUDIO_MSG_MCP_CLIENT_READ_RESULT, conn_handle 0x%x, gmcs %d, srv_instance_id %d, char_uuid 0x%x, cause 0x%x",
                            p_read_result->conn_handle, p_read_result->gmcs,
                            p_read_result->srv_instance_id, p_read_result->char_uuid, p_read_result->cause);
            if (p_read_result->cause != GAP_SUCCESS)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            switch (p_read_result->char_uuid)
            {
            case MCS_UUID_CHAR_CONTENT_CONTROL_ID:
                {
                    T_APP_LEA_CONTENT_DB *p_content;

                    p_content = app_lea_content_find(p_read_result->conn_handle, APP_LEA_CONTENT_TYPE_MCS,
                                                     p_read_result->srv_instance_id, p_read_result->gmcs);
                    if (p_content == NULL)
                    {
                        return BLE_AUDIO_CB_RESULT_APP_ERR;
                    }
                    p_content->ccid = p_read_result->data.content_control_id;
                    data_uart_print("MCS: Read CCID 0x%2x\r\n", p_read_result->data.content_control_id);
                    APP_PRINT_INFO1("MCS_UUID_CHAR_CONTENT_CONTROL_ID: read result, content_control_id %d",
                                    p_read_result->data.content_control_id);
                }
                break;

            case MCS_UUID_CHAR_MEDIA_STATE:
                {
                    data_uart_print("MCS: Read Media State %d\r\n", p_read_result->data.media_state);
                    APP_PRINT_INFO1("MCS_UUID_CHAR_MEDIA_STATE: read result, media_state %d",
                                    p_read_result->data.media_state);
                    cb_result = app_lea_acc_mcp_update_state(p_read_result->conn_handle, p_read_result->gmcs,
                                                             p_read_result->srv_instance_id, p_read_result->data.media_state);
                }
                break;

            case MCS_UUID_CHAR_MEDIA_PLAYER_NAME:
                {
                    APP_PRINT_INFO1("MCS_UUID_CHAR_MEDIA_PLAYER_NAME: read result, media_player_name %s",
                                    TRACE_STRING(p_read_result->data.media_player_name.p_media_player_name));
                }
                break;

            case MCS_UUID_CHAR_TRACK_TITLE:
                {
                    APP_PRINT_INFO1("MCS_UUID_CHAR_TRACK_TITLE: read result, track_title %s",
                                    TRACE_STRING(p_read_result->data.track_title.p_track_title));
                }
                break;

            case MCS_UUID_CHAR_TRACK_DURATION:
                {
                    APP_PRINT_INFO1("MCS_UUID_CHAR_TRACK_DURATION: read result, track_duration 0x%x",
                                    p_read_result->data.track_duration);
                }
                break;

            case MCS_UUID_CHAR_TRACK_POSITION:
                {
                    APP_PRINT_INFO1("MCS_UUID_CHAR_TRACK_POSITION: read result, track_position 0x%x",
                                    p_read_result->data.track_position);
                }
                break;

            default:
                break;
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_NOTIFY:
        {
            T_MCP_CLIENT_NOTIFY *p_notify_result = (T_MCP_CLIENT_NOTIFY *)buf;
            APP_PRINT_INFO4("app_lea_acc_mcp_handle_msg: LE_AUDIO_MSG_MCP_CLIENT_NOTIFY, conn_handle 0x%x, gmcs %d, srv_instance_id %d, char_uuid 0x%x",
                            p_notify_result->conn_handle, p_notify_result->gmcs,
                            p_notify_result->srv_instance_id, p_notify_result->char_uuid);

            switch (p_notify_result->char_uuid)
            {
            case MCS_UUID_CHAR_MEDIA_STATE:
                {
                    APP_PRINT_INFO1("MCS_UUID_CHAR_MEDIA_STATE: notify, media_state %d",
                                    p_notify_result->data.media_state);
                    data_uart_print("MCS: Notify Media State %d\r\n", p_notify_result->data.media_state);
                    cb_result = app_lea_acc_mcp_update_state(p_notify_result->conn_handle, p_notify_result->gmcs,
                                                             p_notify_result->srv_instance_id, p_notify_result->data.media_state);
                }
                break;


            case MCS_UUID_CHAR_MEDIA_PLAYER_NAME:
                {
                    APP_PRINT_INFO1("MCS_UUID_CHAR_MEDIA_PLAYER_NAME: notify, media_player_name %s",
                                    TRACE_STRING(p_notify_result->data.media_player_name.p_media_player_name));
                }
                break;

            case MCS_UUID_CHAR_TRACK_CHANGED:
                {
                    APP_PRINT_INFO0("MCS_UUID_CHAR_TRACK_CHANGED: notify");
                }
                break;

            case MCS_UUID_CHAR_TRACK_TITLE:
                {
                    APP_PRINT_INFO1("MCS_UUID_CHAR_TRACK_TITLE: notify, track_title %s",
                                    TRACE_STRING(p_notify_result->data.track_title.p_track_title));
                }
                break;

            case MCS_UUID_CHAR_TRACK_DURATION:
                {
                    APP_PRINT_INFO1("MCS_UUID_CHAR_TRACK_DURATION: notify, track_duration 0x%x",
                                    p_notify_result->data.track_duration);
                }
                break;

            case MCS_UUID_CHAR_TRACK_POSITION:
                {
                    APP_PRINT_INFO1("MCS_UUID_CHAR_TRACK_POSITION: notify, track_position 0x%x",
                                    p_notify_result->data.track_position);
                }
                break;

            default:
                break;
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_MEDIA_CP_NOTIFY:
        {
            T_MCP_CLIENT_MEDIA_CP_NOTIFY *p_cp_notify = (T_MCP_CLIENT_MEDIA_CP_NOTIFY *)buf;
            APP_PRINT_INFO5("app_lea_acc_mcp_handle_msg: LE_AUDIO_MSG_MCP_CLIENT_MEDIA_CP_NOTIFY, conn_handle 0x%x, gmcs %d, srv_instance_id %d, requested_opcode 0x%x, result_code 0x%x",
                            p_cp_notify->conn_handle, p_cp_notify->gmcs, p_cp_notify->srv_instance_id,
                            p_cp_notify->requested_opcode, p_cp_notify->result_code);
        }
        break;

    default:
        break;
    }

    return cb_result;
}

void app_lea_acc_mcp_init_cap(T_CAP_INIT_PARAMS *p_param)
{
    p_param->mcp_media_control_client = true;
    ble_audio_cback_register(app_lea_acc_mcp_handle_msg);
}
#endif
