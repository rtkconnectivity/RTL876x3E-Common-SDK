/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include <string.h>
#include "app_lea_ini_cmd.h"
#include "app_lea_ini_bap_bsrc.h"
#include "app_a2dp_xmit_lea.h"
#include "app_lea_ini_scan.h"
#include "app_cmd.h"
#include "trace.h"
#include "app_main.h"
#include "app_lea_ini_cap.h"

#if BAP_BROADCAST_ASSISTANT
#include "app_lea_ini_bap.h"
#endif

#if (VCP_VOLUME_CONTROLLER || MICP_MIC_CONTROLLER)
#include "cap.h"
#include "ble_audio.h"
#endif

#if CCP_CALL_CONTROL_SERVER
#include "app_lea_ini_ccp.h"
#endif

#if MCP_MEDIA_CONTROL_SERVER
#include "app_lea_ini_mcp.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_uac.h"
#endif

void app_lea_ini_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));;

    APP_PRINT_TRACE3("app_lea_ini_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);

    switch (cmd_id)
    {
#if BAP_BROADCAST_SOURCE
    case CMD_LEA_BSRC_INIT:
        {
            if (app_db.bsrc_db.state != BROADCAST_SOURCE_STATE_IDLE)
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }
            else
            {
                struct
                {
                    uint16_t    cmd_id;
                    uint8_t     codec_cfg;
                    uint8_t     bis_num;
                    bool        encryption;
                    bool        ull_mode;
                    uint16_t    pd;
                } __attribute__((packed)) *bis_param = (typeof(bis_param))cmd_ptr;


                T_CODEC_CFG_ITEM codec_cfg_type = (T_CODEC_CFG_ITEM)bis_param->codec_cfg;
                uint8_t bis_num = bis_param->bis_num;
                bool encryption = bis_param->encryption;
                app_db.bis_ull_mode = bis_param->ull_mode;
                uint16_t presentation_delay = bis_param->pd;
                APP_PRINT_TRACE1("app_lea_ini_handle_cmd_set: bis ull mode %d", app_db.cis_ull_mode);
                T_QOS_CFG_TYPE qos_type = QOS_CFG_BIS_HIG_RELIABILITY;
                if (app_db.bis_ull_mode)
                {
                    qos_type = QOS_CFG_BIS_LOW_LATENCY;
                }
                app_lea_bsrc_init(codec_cfg_type,
                                  qos_type,
                                  bis_num,
                                  GAP_LOCAL_ADDR_LE_PUBLIC,
                                  encryption,
                                  presentation_delay);
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
    case CMD_LEA_BSRC_START:
        {
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            app_tri_dongle_uac_set_bis_src(true, true);
#else
            if (!app_lea_bsrc_start())
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }
#endif
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
    case CMD_LEA_BSRC_STOP:
        {
            bool release = cmd_ptr[2];
            if (!app_lea_bsrc_stop(release))
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            else
            {
                app_tri_dongle_uac_set_bis_src(false, true);
            }
#endif
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif
    case CMD_LEA_SCAN:
        {
            T_APP_LEA_SCAN_STATE type = (T_APP_LEA_SCAN_STATE)cmd_ptr[2];
            if (type == APP_LEA_SCAN_START)
            {
                app_lea_ini_scan_start();
            }
            else if (type == APP_LEA_SCAN_STOP)
            {
                app_lea_ini_scan_stop();
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#if BAP_UNICAST_CLIENT
    case CMD_LEA_CIS_ULL_MODE:
        {
            app_db.cis_ull_mode = cmd_ptr[2];
            APP_PRINT_TRACE1("app_lea_ini_handle_cmd_set: cis ull mode %d", app_db.cis_ull_mode);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
    case CMD_LEA_CIS_START_MEDIA:
        {
            uint8_t group_idx = cmd_ptr[2];
            if (app_lea_ini_cis_media_start(group_idx))
            {
                ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_CIS_START_CONVERSATION:
        {
            uint8_t group_idx = cmd_ptr[2];
            if (app_lea_ini_cis_conversation_start(group_idx))
            {
                ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_CIS_STOP_STREAM:
        {
            uint8_t group_idx = cmd_ptr[2];
            bool release = cmd_ptr[3];
            if (!app_lea_ini_cis_stream_stop(group_idx, release))
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#if CSIP_SET_COORDINATOR
    case CMD_LEA_CSIS_MEMBER_SCAN:
        {
            uint8_t group_idx = cmd_ptr[2];
            T_BLE_AUDIO_GROUP_HANDLE group_handle = app_lea_ini_find_group_handle_by_idx(group_idx);
            if (group_handle)
            {
                if (!app_lea_group_scan_start(group_handle))
                {
                    ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
    case CMD_LEA_CSIS_GROUP_SCAN_STOP:
        {
            if (!app_lea_group_scan_stop())
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif
#endif

#if BAP_BROADCAST_ASSISTANT
    case CMD_LEA_BST_START:
        {
            T_APP_LEA_GROUP_INFO *p_group = NULL;
            T_APP_LEA_SYNC_INFO *p_sync_info = NULL;
            uint8_t group_idx = cmd_ptr[2];
            uint8_t sync_idx = cmd_ptr[3];

            if (group_idx < app_db.group_handle_queue.count)
            {
                p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                group_idx);
            }

            if (sync_idx < app_db.sync_handle_queue.count)
            {
                p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                                   sync_idx);
            }
            if (p_group != NULL && p_sync_info != NULL)
            {
                app_lea_com_bst_reception_start(p_group->group_handle, p_sync_info->sync_handle);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_BST_STOP:
        {
            T_APP_LEA_GROUP_INFO *p_group = NULL;
            T_APP_LEA_SYNC_INFO *p_sync_info = NULL;
            uint8_t group_idx = cmd_ptr[2];
            uint8_t sync_idx = cmd_ptr[3];

            if (group_idx < app_db.group_handle_queue.count)
            {
                p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                group_idx);
            }

            if (sync_idx < app_db.sync_handle_queue.count)
            {
                p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                                   sync_idx);
            }
            if (p_group != NULL && p_sync_info != NULL)
            {
                app_lea_com_bst_reception_stop(p_group->group_handle, p_sync_info->sync_handle);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_BST_REMOVE:
        {
            T_APP_LEA_GROUP_INFO *p_group = NULL;
            T_APP_LEA_SYNC_INFO *p_sync_info = NULL;
            uint8_t group_idx = cmd_ptr[2];
            uint8_t sync_idx = cmd_ptr[3];

            if (group_idx < app_db.group_handle_queue.count)
            {
                p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                group_idx);
            }

            if (sync_idx < app_db.sync_handle_queue.count)
            {
                p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                                   sync_idx);
            }
            if (p_group != NULL && p_sync_info != NULL)
            {
                app_lea_com_bst_reception_remove(p_group->group_handle, p_sync_info->sync_handle);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_BAAS_SCAN:
        {
            app_lea_baas_scan_start();
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_PA_SYNC_DEV:
        {
            bool ret = false;
            uint8_t dev_idx = cmd_ptr[2];

            if (dev_idx < app_db.scan_baas_dev_queue.count)
            {
                T_APP_LEA_BAAS_SCAN_DEV *p_dev = (T_APP_LEA_BAAS_SCAN_DEV *)os_queue_peek(
                                                     &app_db.scan_baas_dev_queue,
                                                     dev_idx);
                if (p_dev)
                {
                    ret = app_lea_pa_sync_by_dev_info(&p_dev->dev_info);
                }
            }
            if (!ret)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if VCP_VOLUME_CONTROLLER
    case CMD_LEA_GVCS_VOLUME:
        {
            uint8_t group_idx = cmd_ptr[2];
            uint8_t volume_setting = cmd_ptr[3];
            T_BLE_AUDIO_GROUP_HANDLE group_handle = app_lea_ini_find_group_handle_by_idx(group_idx);
            if (group_handle)
            {
                if (cap_vcs_change_volume(group_handle, volume_setting) != LE_AUDIO_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_GVCS_MUTE:
        {
            uint8_t group_idx = cmd_ptr[2];
            uint8_t mute = cmd_ptr[3];
            T_BLE_AUDIO_GROUP_HANDLE group_handle = app_lea_ini_find_group_handle_by_idx(group_idx);
            if (group_handle)
            {
                if (cap_vcs_change_mute(group_handle, mute) != LE_AUDIO_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
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

#if MICP_MIC_CONTROLLER
    case CMD_LEA_GMIC_MUTE:
        {
            uint8_t group_idx = cmd_ptr[2];
            T_MICS_MUTE mute = (T_MICS_MUTE)cmd_ptr[3];
            T_BLE_AUDIO_GROUP_HANDLE group_handle = app_lea_ini_find_group_handle_by_idx(group_idx);
            if (group_handle)
            {
                if (cap_mics_change_mute(group_handle, mute) != LE_AUDIO_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
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

#if CCP_CALL_CONTROL_SERVER
    case CMD_LEA_CCP_ACTION:
        {
            T_APP_TBS_OUTPUT_MSG msg = (T_APP_TBS_OUTPUT_MSG)cmd_ptr[2];
            T_APP_TBS_OUTPUT_MSG_DATA msg_data = {0};
            uint8_t p_incoming_call_uri[] = "Incoming call";
            uint8_t p_outgoing_call_uri[] = "Outgoing call";

            if (msg == APP_TBS_OUTPUT_MSG_REMOTE_INCOMING_CALL)
            {
                msg_data.remote_incoming_call.call_uri_len =  strlen("Incoming call");
                msg_data.remote_incoming_call.p_call_uri =  p_incoming_call_uri;
            }
            else if (msg == APP_TBS_OUTPUT_MSG_LOCAL_ORIGINATE)
            {
                msg_data.local_originate.call_uri_len =  strlen("Outgoing call");
                msg_data.local_originate.p_call_uri =  p_outgoing_call_uri;
            }
            else if (msg == APP_TBS_OUTPUT_MSG_REMOTE_TERMINATE)
            {
                msg_data.remote_terminate.call_index =  cmd_ptr[3];
                msg_data.remote_terminate.reason_code = TBS_TERMINATION_REASON_CODES_REMOTE_END_CALL;
            }
            else if (msg == APP_TBS_OUTPUT_MSG_LOCAL_TERMINATE)
            {
                msg_data.local_terminate.call_index =  cmd_ptr[3];
                msg_data.local_terminate.reason_code = TBS_TERMINATION_REASON_CODES_SERVER_END_CALL;
            }
            else
            {
                msg_data.call_index =  cmd_ptr[3];
            }

            app_lea_ini_ccp_control(msg, &msg_data);

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if MCP_MEDIA_CONTROL_SERVER
    case CMD_LEA_MCP_STATE:
        {
            app_lea_ini_mcp_update_media_state((T_MCS_MEDIA_STATE)cmd_ptr[2]);

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEA_MCP_NOTIFY:
        {
            T_MCP_SERVER_SEND_DATA_PARAM data_param = {0};
            uint16_t mcs_uuid = (uint16_t)((cmd_ptr[2]) | (cmd_ptr[3] << 8));
            uint8_t param = cmd_ptr[4];

            if (mcs_uuid == MCS_UUID_CHAR_TRACK_CHANGED)
            {
                data_param.char_uuid = MCS_UUID_CHAR_TRACK_CHANGED;
            }
            else if (mcs_uuid == MCS_UUID_CHAR_TRACK_DURATION)
            {
                data_param.char_uuid = MCS_UUID_CHAR_TRACK_DURATION;
                data_param.param.track_duration = param;
                app_db.mcp_db.track_duration = param;
            }
            else if (mcs_uuid == MCS_UUID_CHAR_TRACK_POSITION)
            {
                data_param.char_uuid = MCS_UUID_CHAR_TRACK_POSITION;
                data_param.param.track_position = param;
                app_db.mcp_db.track_position = param;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            if (!mcp_server_send_data(app_db.mcp_db.gmcs_id, &data_param))
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
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

