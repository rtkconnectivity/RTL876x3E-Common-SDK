/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_MCP_SUPPORT
#include "trace.h"
#include "ble_audio.h"
#include "mcp_client.h"
#include "app_link_util_cs.h"
#if F_APP_LEA_CMD_SUPPORT
#include "app_lea_acc_def.h"
#include "app_main.h"

#endif

static uint16_t app_lea_mcp_active_conn_handle = 0;

#if F_APP_LEA_CMD_SUPPORT
static void app_lea_mcp_report_media_state(T_MCS_MEDIA_STATE media_state)
{
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].lea_link_state != LEA_LINK_IDLE)
        {
            struct
            {
                uint8_t conn_id;
                uint8_t media_state;
            } __attribute__((packed)) rpt = {};

            rpt.conn_id = app_db.le_link[i].id;
            rpt.media_state = media_state;

            app_report_event(CMD_PATH_UART, EVENT_MCP_MEDIA_STATE, 0, (uint8_t *)&rpt,
                             sizeof(rpt));
        }
    }
}
#endif

static uint16_t app_lea_mcp_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    if ((msg & 0xFF00) == LE_AUDIO_MSG_GROUP_MCP_CLIENT)
    {
        APP_PRINT_INFO1("app_lea_mcp_ble_audio_cback: msg %x", msg);
    }

    switch (msg)
    {
    case LE_AUDIO_MSG_MCP_CLIENT_DIS_DONE:
        {
            T_APP_LE_LINK *p_link;
            T_MCP_CLIENT_DIS_DONE *p_dis_done = (T_MCP_CLIENT_DIS_DONE *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_dis_done->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_dis_done->is_found)
            {
                if (p_dis_done->gmcs)
                {
                    mcp_client_cfg_cccd(p_dis_done->conn_handle,
                                        MCP_CLIENT_CFG_CCCD_FLAG_MEDIA_CONTROL_POINT | MCP_CLIENT_CFG_CCCD_FLAG_TRACK_CHANGED |
                                        MCP_CLIENT_CFG_CCCD_FLAG_MEDIA_STATE,
                                        true, 0, true);
                    mcp_client_read_char_value(p_dis_done->conn_handle, 0, MCS_UUID_CHAR_MEDIA_STATE, true);
                    mcp_client_read_char_value(p_dis_done->conn_handle, 0, MCS_UUID_CHAR_MEDIA_PLAYER_NAME, true);
                    p_link->gmcs = p_dis_done->gmcs;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_READ_RESULT:
        {
            T_APP_LE_LINK *p_link;
            T_MCP_CLIENT_READ_RESULT *p_read_result = (T_MCP_CLIENT_READ_RESULT *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_read_result->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_read_result->cause == GAP_SUCCESS)
            {
                if (p_read_result->char_uuid == MCS_UUID_CHAR_MEDIA_STATE)
                {
                    if (p_read_result->data.media_state != MCS_MEDIA_STATE_INACTIVE)
                    {
                        p_link->pre_media_state = p_link->media_state;
                        p_link->media_state = p_read_result->data.media_state;
                    }
                    APP_PRINT_INFO1("app_lea_mcp_ble_audio_cback: media_state %x", p_link->media_state);
                }
                else if (p_read_result->char_uuid == MCS_UUID_CHAR_MEDIA_PLAYER_NAME)
                {
                    if (p_read_result->data.media_player_name.media_player_name_len != 0)
                    {
                        uint8_t *p_name = p_read_result->data.media_player_name.p_media_player_name;
                        APP_PRINT_INFO1("app_lea_mcp_ble_audio_cback: player_name %s", TRACE_STRING(p_name));
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_CLIENT_NOTIFY:
        {
            T_APP_LE_LINK *p_link;
            T_MCP_CLIENT_NOTIFY *p_notify_result = (T_MCP_CLIENT_NOTIFY *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_notify_result->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            switch (p_notify_result->char_uuid)
            {
            case MCS_UUID_CHAR_MEDIA_STATE:
                {
                    // Because dongle set MCP states is pause forever, not change state.
                    // here is workaround, when headset is streaming, local state will be change.
                    if (p_notify_result->data.media_state != MCS_MEDIA_STATE_INACTIVE)
                    {
                        p_link->pre_media_state = p_link->media_state;
                        p_link->media_state = p_notify_result->data.media_state;
#if F_APP_LEA_CMD_SUPPORT
                        if (p_link->pre_media_state != p_link->media_state)
                        {
                            app_lea_mcp_report_media_state((T_MCS_MEDIA_STATE)p_link->media_state);
                        }
#endif
                    }
                    APP_PRINT_INFO1("app_lea_mcp_ble_audio_cback: media_state %x", p_notify_result->data.media_state);
                }
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

uint16_t app_lea_mcp_get_active_conn_handle(void)
{
    return app_lea_mcp_active_conn_handle;
}

bool app_lea_mcp_set_active_conn_handle(uint16_t conn_handle)
{
    bool ret = false;
    T_APP_LE_LINK *p_link;

    p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        app_lea_mcp_active_conn_handle = conn_handle;
        ret = true;
    }
    APP_PRINT_INFO2("app_lea_mcp_set_active_conn_handle: active_mcp_conn_handle 0x%x, ret %x",
                    app_lea_mcp_active_conn_handle, ret);
    return ret;
}

void app_lea_mcp_init(void)
{
    ble_audio_cback_register(app_lea_mcp_ble_audio_cback);
}
#endif
