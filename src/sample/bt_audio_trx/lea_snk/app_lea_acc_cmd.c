/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
#include <string.h>
#include "trace.h"
#include "ccp_client.h"
#include "mcp_client.h"
#include "app_audio_policy.h"
#include "app_audio_tone_cfg.h"
#include "app_cmd.h"
#include "app_lea_acc_adv.h"
#include "app_lea_acc_ascs.h"
#include "app_lea_acc_broadcast_audio.h"
#include "app_lea_acc_ccp.h"
#include "app_lea_acc_cmd.h"
#include "app_lea_acc_scan.h"
#include "app_lea_acc_vol_def.h"
#include "app_lea_acc_vol_policy.h"
#include "app_main.h"


void app_lea_acc_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));;

    APP_PRINT_TRACE3("app_lea_acc_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);

    switch (cmd_id)
    {
#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
    case CMD_LEA_ADV_START:
        {
            if (app_lea_adv_get_state() == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            app_lea_adv_start((uint8_t)LEA_ADV_MODE_PAIRING);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_ADV_STOP:
        {
            if (app_lea_adv_get_state() == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            app_lea_adv_stop();
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    case CMD_LEA_MCP_MEDIA_CP:
        {
            uint8_t conn_id = cmd_ptr[2];
            uint8_t media_cp = cmd_ptr[3];
            T_APP_LE_LINK *p_link;

            p_link = app_link_find_le_link_by_conn_id(conn_id);
            if (p_link)
            {
                if (((media_cp >= MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PLAY) &&
                     (media_cp <= MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_STOP)) ||
                    (media_cp == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PREVIOUS_TRACK) ||
                    (media_cp == MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_NEXT_TRACK))
                {
                    T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM param;

                    param.opcode = media_cp;
                    mcp_client_write_media_cp(p_link->conn_handle, 0, p_link->gmcs, &param, true);
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_CCP_CALL_CP:
        {
            uint8_t conn_id = cmd_ptr[2];
            uint8_t call_cp = cmd_ptr[3];
            T_APP_LE_LINK *p_link;

            p_link = app_link_find_le_link_by_conn_id(conn_id);
            if (p_link)
            {
                if (call_cp > TBS_CALL_CONTROL_POINT_CHAR_OPCODE_JOIN)
                {
                    ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
                }
                else
                {
                    if (call_cp == TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ORIGINATE)
                    {
                        if (p_link->active_call_uri != NULL)
                        {
                            T_CCP_CLIENT_WRITE_CALL_CP_PARAM write_call_cp_param = {0};

                            write_call_cp_param.opcode = call_cp;
                            write_call_cp_param.param.originate_opcode_param.p_uri = p_link->active_call_uri;
                            write_call_cp_param.param.originate_opcode_param.uri_len = strlen((char *)p_link->active_call_uri);

                            ccp_client_write_call_cp(p_link->conn_handle, 0, p_link->gtbs, false, &write_call_cp_param);
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        }
                    }
                    else
                    {
                        T_LEA_CALL_ENTRY *p_call_entry;

                        p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, p_link->active_call_index);
                        if (p_call_entry)
                        {
                            T_CCP_CLIENT_WRITE_CALL_CP_PARAM write_call_cp_param = {0};

                            write_call_cp_param.opcode = call_cp;
                            write_call_cp_param.param.accept_opcode_call_index = p_call_entry->call_index;
                            ccp_client_write_call_cp(p_link->conn_handle, 0, p_link->gtbs, false, &write_call_cp_param);
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        }
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif
#if F_APP_TMAP_BMR_SUPPORT
    case CMD_LEA_SYNC_INIT:
        {
            struct
            {
                uint8_t     bis_mode;
                uint8_t     bis_policy;
                uint8_t     addr[6];
            } __attribute__((packed)) *bis_init = (typeof(bis_init))(cmd_ptr + 2);

            app_lea_bca_sync_init(bis_init->bis_mode, bis_init->bis_policy, bis_init->addr);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_SYNC:
        {
            bool param = (bool)cmd_ptr[2];
            if (param == false)
            {
                app_lea_mgr_tri_mmi_handle_action(MMI_BIG_START, false);
            }
            else if (param == true)
            {
                app_lea_mgr_tri_mmi_handle_action(MMI_BIG_STOP, false);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif
#if F_APP_VCS_SUPPORT
    case CMD_LEA_VCS_SET:
        {
            uint8_t conn_id = cmd_ptr[2];
            T_LEA_VOL_CHG_TYPE vol_type = (T_LEA_VOL_CHG_TYPE)cmd_ptr[3];
            T_APP_LE_LINK *p_link;

            p_link = app_link_find_le_link_by_conn_id(conn_id);
            if (p_link)
            {
                if (vol_type > LEA_SPK_UNMUTE)
                {
                    ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
                }
                else
                {
                    T_LEA_VOL_CHG_RESULT change_result;

                    change_result = app_lea_vol_local_volume_change(vol_type);
                    if ((change_result == LEA_VOL_LEVEL_CHANGE) ||
                        (change_result == LEA_VOL_MUTE_CHANGE))
                    {
                        app_lea_vol_update_track_volume();
                    }

                    if ((change_result == LEA_VOL_LEVEL_LIMIT) ||
                        (change_result == LEA_VOL_LEVEL_CHANGE))
                    {
                        if (app_cfg_nv.lea_vol_setting == MAX_VOLUME_SETTING)
                        {
                            app_audio_tone_type_play(TONE_VOL_MAX, false, false);
                        }
                        else if (app_cfg_nv.lea_vol_setting == 0)
                        {
                            app_audio_tone_type_play(TONE_VOL_MIN, false, false);
                        }
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif
    default:
        break;
    }
}
#endif
