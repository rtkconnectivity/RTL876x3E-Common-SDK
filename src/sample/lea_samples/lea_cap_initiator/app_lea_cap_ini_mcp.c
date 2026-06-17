/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "data_uart.h"
#include "mcp_mgr.h"
#include "ble_audio_group.h"
#include "app_lea_cap_ini_mcp.h"
#include "app_lea_cap_ini_cap.h"
#include "app_lea_cap_ini_main.h"

#if MCP_MEDIA_CONTROL_SERVER
void app_lea_ini_mcp_update_media_state(T_MCS_MEDIA_STATE state)
{
    T_MCP_SERVER_SET_PARAM set_param = {0};

    APP_PRINT_INFO1("app_lea_ini_mcp_update_media_state: state %x", state);
    if (state > MCS_MEDIA_STATE_SEEKING ||
        app_db.mcp_db.media_state == state)
    {
        return;
    }
    app_db.mcp_db.media_state = state;

    set_param.char_uuid = MCS_UUID_CHAR_MEDIA_STATE;
    set_param.param.media_state = state;

    mcp_server_set_param(app_db.mcp_db.gmcs_id, &set_param);
}

bool app_lea_ini_mcp_handle_operation(T_MCP_SERVER_WRITE_MEDIA_CP_IND *p_mcp_req)
{
    uint8_t opcode = p_mcp_req->opcode;

    data_uart_print("MCP CP: opcode 0x%x, curr media state %d\r\n", opcode,
                    app_db.mcp_db.media_state);

    if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PLAY)
    {
        if (app_db.mcp_db.media_state == MCS_MEDIA_STATE_SEEKING ||
            app_db.mcp_db.media_state == MCS_MEDIA_STATE_PAUSED)
        {
            /*If the Media State characteristic value is Paused or Seeking, the media player shall start playing the current track
            and the Media State characteristic value shall be set to Playing. */
            app_lea_ini_mcp_update_media_state(MCS_MEDIA_STATE_PLAYING);
        }
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PAUSE)
    {
        if (app_db.mcp_db.media_state == MCS_MEDIA_STATE_SEEKING)
        {
            /*If the Media State characteristic value is Seeking, the media player shall stop seeking, set the current
            track and track position as a result of seeking, and the Media State characteristic value shall be set to Paused.*/
            app_lea_ini_mcp_update_media_state(MCS_MEDIA_STATE_PAUSED);
        }
        else if (app_db.mcp_db.media_state == MCS_MEDIA_STATE_PLAYING)
        {
            /*If the Media State characteristic value is Playing, the media player
              shall pause playing the current track and the Media State characteristic value shall be set to Paused.*/
            app_lea_ini_mcp_update_media_state(MCS_MEDIA_STATE_PAUSED);
        }
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_STOP)
    {
        /*If the Media State characteristic value is Playing, Paused, or Seeking, the media player
        shall stop any activity and the Media State characteristic shall be set to Paused.
        The track position shall be set to the beginning of the current track.*/
        if (app_db.mcp_db.media_state != MCS_MEDIA_STATE_INACTIVE)
        {
            app_lea_ini_mcp_update_media_state(MCS_MEDIA_STATE_PAUSED);
        }
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_FAST_FORWARD)
    {
        /*If the Media State characteristic value is Paused or Playing, the media player shall move the current track position forward.
        The media player should stop playing the current track, set the Media State characteristic to Seeking,
        and move the current track position forward at a speed determined by the implementation.*/
        app_lea_ini_mcp_update_media_state(MCS_MEDIA_STATE_SEEKING);
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_FAST_REWIND)
    {
        /*If the Media State characteristic is Paused or Playing, the media player shall move the current track position backwards.
        The media player should stop playing the current track, set the Media State characteristic to Seeking,
        and move the current track position backwards at a speed determined by the implementation.*/
        app_lea_ini_mcp_update_media_state(MCS_MEDIA_STATE_SEEKING);
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PREVIOUS_TRACK)
    {
        /*If the Media State characteristic value is Playing, Paused, or Seeking, the current track
        shall be set to either the previous track based on the playing order or remain on the current track,
        as determined by the implementation. The Track Position characteristic shall be set to 0.
        The resulting Media State characteristic value is left up to the implementation.*/
        T_MCP_SERVER_SEND_DATA_PARAM send_data_param = {0};

        send_data_param.char_uuid = MCS_UUID_CHAR_TRACK_CHANGED;

        mcp_server_send_data(app_db.mcp_db.gmcs_id, &send_data_param);
    }
    else if (opcode == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_NEXT_TRACK)
    {
        /*If the Media State characteristic value is Playing, Paused, or Seeking, the current track
        shall be set to the next track based on the playing order and the Track Position shall be set to 0.
        The resulting Media State characteristic value is left up to the implementation.*/
        T_MCP_SERVER_SEND_DATA_PARAM send_data_param = {0};

        send_data_param.char_uuid = MCS_UUID_CHAR_TRACK_CHANGED;

        mcp_server_send_data(app_db.mcp_db.gmcs_id, &send_data_param);
    }
    return true;
}

uint16_t app_lea_ini_mcp_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result  = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_MCP_SERVER_WRITE_MEDIA_CP_IND:
        {
            T_MCP_SERVER_WRITE_MEDIA_CP_IND *p_mcp_req = (T_MCP_SERVER_WRITE_MEDIA_CP_IND *)buf;

            APP_PRINT_INFO4("app_lea_ini_mcp_handle_msg: LE_AUDIO_MSG_MCP_SERVER_WRITE_MEDIA_CP_IND, conn_handle 0x%x, cid 0x%x, service_id %d, opcode 0x%x",
                            p_mcp_req->conn_handle, p_mcp_req->cid,
                            p_mcp_req->service_id, p_mcp_req->opcode);
            if (!app_lea_ini_mcp_handle_operation(p_mcp_req))
            {
                cb_result = BLE_AUDIO_CB_RESULT_REJECT;
            }
        }
        break;

    case LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO:
        {
            T_SERVER_ATTR_CCCD_INFO *p_cccd = (T_SERVER_ATTR_CCCD_INFO *)buf;
            if (app_db.mcp_db.gmcs_id == p_cccd->service_id)
            {
                app_db.mcp_db.mcp_enabled_cccd = p_cccd->ccc_bits;
                APP_PRINT_INFO4("LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO: MCS, conn_handle 0x%x, char_uuid 0x%x, char_attrib_index %d, ccc_bits 0x%x",
                                p_cccd->conn_handle,
                                p_cccd->char_uuid,
                                p_cccd->char_attrib_index,
                                p_cccd->ccc_bits);
            }
        }
        break;

    case LE_AUDIO_MSG_MCP_SERVER_READ_IND:
        {
            T_MCP_SERVER_READ_IND *p_read_ind = (T_MCP_SERVER_READ_IND *)buf;

            if (p_read_ind)
            {
                T_MCP_SERVER_READ_CFM read_cfm = {0};
                APP_PRINT_INFO5("app_lea_ini_mcp_handle_msg: LE_AUDIO_MSG_MCP_SERVER_READ_IND, conn_handle 0x%x, cid 0x%x, service_id %d, char_uuid 0x%x, offset %d",
                                p_read_ind->conn_handle, p_read_ind->cid,
                                p_read_ind->service_id, p_read_ind->char_uuid, p_read_ind->offset);
                read_cfm.cause = BLE_AUDIO_CB_RESULT_SUCCESS;
                read_cfm.conn_handle = p_read_ind->conn_handle;
                read_cfm.cid = p_read_ind->cid;
                read_cfm.service_id = p_read_ind->service_id;
                read_cfm.char_uuid = p_read_ind->char_uuid;
                read_cfm.offset = p_read_ind->offset;

                switch (p_read_ind->char_uuid)
                {
                case MCS_UUID_CHAR_MEDIA_PLAYER_NAME:
                    {
                        read_cfm.param.media_player_name.p_media_player_name = "RTK player";
                        read_cfm.param.media_player_name.media_player_name_len = strlen("RTK player");

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                case MCS_UUID_CHAR_TRACK_TITLE:
                    {
                        read_cfm.param.track_title.p_track_title = "RTK media trace title";
                        read_cfm.param.track_title.track_title_len = strlen("RTK media trace title");

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                case MCS_UUID_CHAR_TRACK_DURATION:
                    {
                        read_cfm.param.track_duration = app_db.mcp_db.track_duration;

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                case MCS_UUID_CHAR_TRACK_POSITION:
                    {
                        read_cfm.param.track_position = app_db.mcp_db.track_position;

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                case MCS_UUID_CHAR_CONTENT_CONTROL_ID:
                    {
                        read_cfm.param.content_control_id = APP_LEA_GMCS_CCID;

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        mcp_server_read_cfm(&read_cfm);
                    }
                    break;

                default:
                    break;
                }
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

void app_lea_ini_mcp_init_cap(T_CAP_INIT_PARAMS *p_param)
{
    p_param->mcs.mcp_media_control_server = true;
    p_param->mcs.mcs_num = 1;
    p_param->mcs.ots_num = 0;
    ble_audio_cback_register(app_lea_ini_mcp_handle_msg);
}

void app_lea_ini_mcp_init(void)
{
    T_MCP_SERVER_REG_SRV_PARAM reg_srv_param = {0};

    reg_srv_param.gmcs = true;
    reg_srv_param.char_media_control_point.support = true;
    reg_srv_param.char_track_duration.optional_property_notify = true;
    reg_srv_param.char_track_position.optional_property_notify = true;
    app_db.mcp_db.gmcs_id = mcp_server_reg_srv(&reg_srv_param);
    if (app_db.mcp_db.gmcs_id != 0xFF)
    {
        T_MCP_SERVER_SET_PARAM set_param = {0};

        set_param.char_uuid = MCS_UUID_CHAR_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED;
        set_param.param.media_control_point_opcodes_supported =
            (MCS_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED_CHAR_BIT_VALUE_PLAY |
             MCS_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED_CHAR_BIT_VALUE_PAUSE |
             MCS_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED_CHAR_BIT_VALUE_FAST_REWIND |
             MCS_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED_CHAR_BIT_VALUE_FAST_FORWARD |
             MCS_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED_CHAR_BIT_VALUE_STOP |
             MCS_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED_CHAR_BIT_VALUE_PREVIOUS_TRACK |
             MCS_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED_CHAR_BIT_VALUE_NEXT_TRACK);
        mcp_server_set_param(app_db.mcp_db.gmcs_id, &set_param);

        set_param.char_uuid = MCS_UUID_CHAR_MEDIA_STATE;
        set_param.param.media_state = MCS_MEDIA_STATE_PAUSED;

        mcp_server_set_param(app_db.mcp_db.gmcs_id, &set_param);

        app_db.mcp_db.media_state = MCS_MEDIA_STATE_PAUSED;
        app_db.mcp_db.track_duration = MCS_TRACK_DURATION_CHAR_VALUE_UNKNOWN;
        app_db.mcp_db.track_position = MCS_TRACK_POSITION_CHAR_VALUE_UNAVAILABLE;
    }
}
#endif
