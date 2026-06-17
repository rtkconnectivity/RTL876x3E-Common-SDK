/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_BT_PROFILE_MAP_MCE_SUPPORT
#include "app_map.h"
#include "bt_map.h"
#include "app_main.h"
#include "trace.h"
#include "gap_br.h"
#include "app_cmd.h"
#include "app_sdp.h"
#include "app_map_cfg.h"

void app_map_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                        uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE2("app_map_handle_cmd: cmd_id 0x%04x, cmd_len %d", cmd_id, cmd_len);

    switch (cmd_id)
    {
    case CMD_MAP_CONNECT:
        {
            T_GAP_UUID_DATA uuid_data;

            uuid_data.uuid_16 = UUID_MSG_ACCESS_SERVER;
            gap_br_start_sdp_discov(app_db.br_link[app_idx].bd_addr, GAP_UUID16, uuid_data);

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_MAP_SET_FOLDER:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t folder;
            } __attribute__((packed)) *params;

            params = (typeof(params)) cmd_ptr;

            bt_map_mas_folder_set(app_db.br_link[app_idx].bd_addr, (T_BT_MAP_FOLDER)params->folder);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_MAP_GET_FOLDER_LISTING:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t max_list_count;
                uint8_t start_offset;
            } __attribute__((packed)) *params = (typeof(params)) cmd_ptr;

            bt_map_mas_folder_listing_get(app_db.br_link[app_idx].bd_addr, params->max_list_count,
                                          params->start_offset);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_MAP_GET_MESSAGE_LISTING:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t max_list_count;
                uint8_t start_offset;
            } __attribute__((packed)) *params = (typeof(params)) cmd_ptr;

            uint8_t map_path_inbox[12] =
            {
                0x00, 0x69, 0x00, 0x6e, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x78, 0x00, 0x00
            };
            bt_map_mas_msg_listing_get(app_db.br_link[app_idx].bd_addr, map_path_inbox, sizeof(map_path_inbox),
                                       params->max_list_count, params->start_offset);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_MAP_GET_MESSAGE:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t handle_len;
                uint8_t msg_handle[];
            } __attribute__((packed)) *params = (typeof(params)) cmd_ptr;

            bt_map_mas_msg_get(app_db.br_link[app_idx].bd_addr, params->msg_handle, params->handle_len, false);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_MAP_REG_MSG_NOTIFICATION:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t enable;
            } __attribute__((packed)) *params = (typeof(params)) cmd_ptr;

            bt_map_mas_msg_notification_set(app_db.br_link[app_idx].bd_addr, params->enable);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_MAP_PUSH_MESSAGE:
        {
            struct
            {
                uint16_t cmd_id;
                uint16_t msg_len;
                uint8_t msg[];
            } __attribute__((packed)) *params = (typeof(params)) cmd_ptr;

            uint8_t map_path_outbox[14] =
            {
                0x00, 0x6f, 0x00, 0x75, 0x00, 0x74, 0x00, 0x62,
                0x00, 0x6f, 0x00, 0x78, 0x00, 0x00
            };
            bt_map_mas_msg_push(app_db.br_link[app_idx].bd_addr, map_path_outbox, sizeof(map_path_outbox),
                                false, false, params->msg, params->msg_len);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
    case CMD_MAP_DISCONNECT:
        {
            bt_map_mas_disconnect_req(app_db.br_link[app_idx].bd_addr);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        break;
    }
}

void app_map_init(void)
{
    bt_map_init(RFC_MAP_MNS_CHANN_NUM, L2CAP_MAP_MNS_PSM, 0x0000024F);
}

#endif

