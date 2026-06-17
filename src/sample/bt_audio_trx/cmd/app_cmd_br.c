/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "string.h"
#include "stdlib.h"
#include "bt_bond.h"
#include "gap_br.h"
#include "app_bond.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_cmd.h"
#include "app_test.h"
#include "app_cmd_br.h"
#include "app_bt_policy_cfg.h"
#include "app_linkback.h"
#include "app_main.h"
#include "app_multilink.h"
#include "app_timer.h"
#include "app_transfer.h"

#include "app_iap.h"
#include "app_iap_rtk.h"

#define CLEAR_BOND_INFO_SUCCESS     0x00
#define CLEAR_BOND_INFO_FAIL        0x01

uint32_t profiles_to_connect;

void br_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                   uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE1("app_cmd br_cmd_handle: cmd_id 0x%04x", cmd_id);

#if 0
    typedef struct
    {
        uint16_t cmd_id;
        uint8_t  status;
    } __attribute__((packed)) *ACK_PKT;
#endif

    switch (cmd_id)
    {
    case CMD_LEGACY_DATA_TRANSFER:
        {
            app_transfer_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_BT_READ_PAIRED_RECORD:
        {
            T_APP_LINK_RECORD *link_record;
            uint8_t bond_num = app_bond_b2s_num_get();
            uint8_t temp_buff[bond_num * 8 + 1];

            memset(temp_buff, 0, sizeof(temp_buff));
            link_record = malloc(sizeof(T_APP_LINK_RECORD) * bond_num);
            if (link_record != NULL)
            {
                bond_num = app_bond_get_b2s_link_record(link_record, bond_num);
                temp_buff[0] = bond_num;

                for (uint8_t i = 0; i < bond_num; i++)
                {
                    temp_buff[i * 8 + 1] = link_record[i].priority;
                    temp_buff[i * 8 + 2] = link_record[i].bond_flag;
                    memcpy(&temp_buff[i * 8 + 3], &(link_record[i].bd_addr), 6);
                }
                free(link_record);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
            {
                app_report_event(cmd_path, EVENT_REPLY_BR_PAIRED_RECORD, app_idx, temp_buff, sizeof(temp_buff));
            }
        }
        break;

    case CMD_BT_CREATE_CONNECTION:
        {
            struct
            {
                uint16_t cmd_id;
                uint32_t profile_mask;
                uint8_t  addr[6];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            APP_PRINT_INFO1("CMD_BT_CREATE_CONNECTION profile_mask = 0x%08x", cmd->profile_mask);
            profiles_to_connect = cmd->profile_mask;

#if F_APP_DEVICE_CMD_SUPPORT
            app_stop_timer(&app_cmd_mgr.tmr.indices.stop_periodic_inquiry);
#endif
            bt_periodic_inquiry_stop();
            linkback_todo_queue_delete_all_node();

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            linkback_todo_queue_insert_normal_node(cmd->addr, cmd->profile_mask,
                                                   app_bt_policy_cfg.timer_linkback_timeout * 1000, 0);
            linkback_run();
        }
        break;

    case CMD_BT_DISCONNECT:
        {
            T_APP_BR_LINK *p_link;
            struct
            {
                uint16_t cmd_id;
                uint32_t profile_mask;
                uint8_t  addr[6];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            p_link = app_link_find_br_link(cmd->addr);
            if (p_link != NULL)
            {
                app_bt_policy_disconnect(p_link->bd_addr, cmd->profile_mask);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_BT_READ_LINK_INFO:
        {
            uint8_t app_index = cmd_ptr[2];

            if ((app_index >= MAX_BR_LINK_NUM) || !app_link_check_b2s_link_by_id(app_index))
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            }
            else
            {
                uint8_t event_buff[9];

                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                event_buff[0] = app_index;
                event_buff[1] = app_db.br_link[app_index].connected_profile;
                event_buff[2] = 0;
                memcpy(&event_buff[3], app_db.br_link[app_index].bd_addr, 6);
                app_report_event(CMD_PATH_UART, EVENT_REPLY_LINK_INFO, 0, &event_buff[0], sizeof(event_buff));
            }
        }
        break;

    case CMD_BT_GET_REMOTE_NAME:
        {
            bt_remote_name_req(&cmd_ptr[2]);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_GET_BD_ADDR:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(cmd_path, EVENT_GET_BD_ADDR, app_idx, app_db.factory_addr,
                             sizeof(app_db.factory_addr));
        }
        break;

    case CMD_GET_LEGACY_RSSI:
        {
            if (cmd_ptr[2] == LEGACY_RSSI)
            {
                uint8_t *bd_addr = (uint8_t *)(&cmd_ptr[3]);

                if (gap_br_read_rssi(bd_addr) != GAP_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_BT_GET_LOCAL_ADDR:
        {
            uint8_t temp_buff[6];
            memcpy(&temp_buff[0], app_cfg_nv.bud_local_addr, 6);
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(CMD_PATH_UART, EVENT_LOCAL_ADDR, 0, temp_buff, sizeof(temp_buff));
        }
        break;

    case CMD_BT_SEND_AT_CMD:
        {
            uint8_t app_index = cmd_ptr[2];

            if (bt_hfp_send_vnd_at_cmd_req(app_db.br_link[app_index].bd_addr, (char *)&cmd_ptr[3]) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_BT_HFP_DIAL_WITH_NUMBER:
        {
            uint8_t app_index = app_hfp_get_active_idx();
            //char *number = (char *)&cmd_ptr[2];
            char number[cmd_len - 1];
            for (int i = 2; i < cmd_len; i++)
            {
                number[i - 2] = (char)cmd_ptr[i];
                APP_PRINT_INFO1("CMD_BT_HFP_DIAL_WITH_NUMBER number:%d", number[i - 2]);

            }
            number[cmd_len - 2] = '\0';
            APP_PRINT_INFO2("CMD_BT_HFP_DIAL_WITH_NUMBER number:%s,%d ", TRACE_STRING(number), strlen(number));
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            else
            {
                if ((app_db.br_link[app_index].hfp.state == APP_HF_STATE_CONNECTED) &&
                    (app_hfp_get_call_status() == BT_HFP_CALL_IDLE))
                {
                    if (bt_hfp_dial_with_number_req(app_db.br_link[app_index].bd_addr, (const char *)number) == false)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_BT_BOND_INFO_CLEAR:
        {
            T_APP_BR_LINK *p_link = NULL;
            uint8_t *bd_addr = (uint8_t *)(&cmd_ptr[3]);
            uint8_t temp_buff = CLEAR_BOND_INFO_FAIL;

            p_link = app_link_find_br_link(bd_addr);
            if (p_link == NULL)
            {
                if (cmd_ptr[2] == 0) //clear BR/EDR bond info
                {
                    if (bt_bond_delete(bd_addr) == true)
                    {
                        temp_buff = CLEAR_BOND_INFO_SUCCESS;
                    }
                }
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(CMD_PATH_UART, EVENT_BT_BOND_INFO_CLEAR, 0, &temp_buff, sizeof(temp_buff));
        }
        break;

    case CMD_GET_NUM_OF_CONNECTION:
        {
            uint8_t event_data = app_multi_get_acl_connect_num();

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(cmd_path, EVENT_REPORT_NUM_OF_CONNECTION, app_idx, &event_data,
                             sizeof(event_data));
        }
        break;

    case CMD_LINK_USER_PASSKEY_INPUT:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t  key[6];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            char passkey_str[7];
            for (int i = 0; i < 6; i++)
            {
                passkey_str[i] = cmd->key[i];
            }
            passkey_str[6] = '\0';
            uint32_t passkey = (uint32_t)atoi(passkey_str);
            uint8_t *bd_addr = app_test_get_user_passkey_req_bd_addr();

            gap_br_input_passkey(bd_addr, passkey, GAP_CFM_CAUSE_ACCEPT);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SUPPORT_MULTILINK:
        {
            {
                app_bt_policy_cfg_nv.enable_multi_link = 1;

                app_bt_policy_set_b2s_connected_num_max(app_bt_policy_cfg.max_legacy_multilink_devices);
                app_mmi_handle_action(MMI_DEV_ENTER_PAIRING_MODE);
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_BT_IAP_LAUNCH_APP:
        {
#if F_APP_IAP_RTK_SUPPORT && F_APP_IAP_SUPPORT
            uint8_t app_index = cmd_ptr[2];

            if (app_index < MAX_BR_LINK_NUM)
            {
                T_APP_IAP_HDL app_iap_hdl = NULL;
                app_iap_hdl = app_iap_search_by_addr(app_db.br_link[app_index].bd_addr);

                if ((app_db.br_link[app_index].connected_profile & IAP_PROFILE_MASK)
                    && (app_iap_is_authened(app_iap_hdl)))
                {
                    app_iap_rtk_launch(app_db.br_link[app_index].bd_addr, BT_IAP_APP_LAUNCH_WITH_USER_ALERT);
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

#endif
        }
        break;

    default:
        break;
    }
}
