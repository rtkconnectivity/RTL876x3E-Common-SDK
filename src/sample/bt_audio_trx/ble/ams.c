/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_BLE_AMS_CLIENT_SUPPORT

#include <stdint.h>

#include "trace.h"
#include "app_main.h"
#include "ams.h"
#include "app_cmd.h"

#include <string.h>
#include <stdlib.h>
#include "ams_client.h"

#define AMS_MSG_QUEUE_NUM   5  //!< AMS message queue size


typedef enum
{
    PARAM_START_POS = 2,
} CMD_POS;


static struct
{
    struct
    {
        uint8_t app_idx;
        T_CMD_PATH cur_path;
    } app_cmd;
} ams =
{
    .app_cmd.app_idx = 0xff,
    .app_cmd.cur_path = CMD_PATH_UART,
};

void ams_media_control(uint16_t conn_handle, T_AMS_REMOTE_CMD_ID cmd_id)
{
    T_APP_LE_LINK *p_le_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_le_link == NULL)
    {
        APP_PRINT_ERROR0("ams_media_control: no le link exsit.");
        return;
    }

    if (!ams_write_remote_cmd(conn_handle, cmd_id))
    {
        APP_PRINT_ERROR0("ams_media_control: ams_write_remote_cmd fail.");
    }

}

static void set_trans_info(uint8_t app_idx, T_CMD_PATH cur_path)
{
    ams.app_cmd.app_idx = app_idx;
    ams.app_cmd.cur_path = cur_path;
}


static void send_cmd_evt(uint16_t event_id, uint8_t *params, uint16_t len)
{
    app_report_event(ams.app_cmd.cur_path, event_id, ams.app_cmd.app_idx, params, len);
}


static void noti_evt_report(T_EVENT_ID evt, uint16_t conn_handle, uint8_t *data, uint32_t len)
{
    enum PARAM_POS
    {
        APP_LINK_ID_POS     = 0,
        PKT_TYPE_POS        = APP_LINK_ID_POS + 1,
        TOTAL_LEN_POS       = PKT_TYPE_POS + 1,
        PKT_LEN_POS         = TOTAL_LEN_POS + 2,
        PAYLOAD_POS         = PKT_LEN_POS + 2,
    };

    uint8_t *params = calloc(1, len + PAYLOAD_POS);
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    params[APP_LINK_ID_POS] = p_link->id;
    params[PKT_TYPE_POS] = PKT_TYPE_SINGLE;
    params[TOTAL_LEN_POS] = (uint8_t)len;
    params[TOTAL_LEN_POS + 1] = len >> 8;
    params[PKT_LEN_POS] = params[TOTAL_LEN_POS];
    params[PKT_LEN_POS + 1] = params[TOTAL_LEN_POS + 1];

    memcpy(&params[PAYLOAD_POS], data, len);

    APP_PRINT_INFO2("ams noti_evt_report: data[%d]: %b",
                    len, TRACE_BINARY(len, data));
    send_cmd_evt(evt, params, len + PAYLOAD_POS);
    free(params);
}


static void cb(uint16_t conn_handle, T_AMS_CB_DATA *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_AMS_CB_DATA *p_cb_data = (T_AMS_CB_DATA *)p_data;

    APP_PRINT_TRACE2("ams cb: conn_handle 0x%04x, type %d", conn_handle, p_cb_data->cb_type);

    switch (p_cb_data->cb_type)
    {
    case AMS_CLIENT_CB_TYPE_DISC_STATE:
        if (p_cb_data->cb_content.disc_state.is_found)
        {
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
            uint8_t params[2] = {p_link->id, 0};
            send_cmd_evt(EVENT_AMS_REGISTER_COMPLETE, params, sizeof(params));
            ams_subscribe_remote_cmd(conn_handle, true);
        }
        break;

    case AMS_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
        {
            switch (p_cb_data->cb_content.notify_data.type)
            {
            case AMS_NOTIFY_FROM_REMOTE_CMD:
                {
                    noti_evt_report(EVENT_AMS_REMOTE_CMD_NOTIFY, conn_handle,
                                    p_cb_data->cb_content.notify_data.p_value, p_cb_data->cb_content.notify_data.value_size);
                }

                break;
            case AMS_NOTIFY_FROM_ENTITY_UPD:
                {
                    noti_evt_report(EVENT_AMS_ENTITY_UPD_NOTIFY, conn_handle,
                                    p_cb_data->cb_content.notify_data.p_value, p_cb_data->cb_content.notify_data.value_size);
                }
                break;
            default:
                break;
            }
        }
        break;

    case AMS_CLIENT_CB_TYPE_READ_RESULT:
        APP_PRINT_TRACE1("ams cb: AMS_CLIENT_CB_TYPE_READ_RESULT type %d",
                         p_cb_data->cb_content.read_data.type);
        switch (p_cb_data->cb_content.read_data.type)
        {
        case AMS_READ_FROM_ENTITY_UPD:
            APP_PRINT_INFO3("ams cb: AMS_READ_FROM_ENTITY_UPD: cause 0x%x, data[%d]: %b",
                            p_cb_data->cb_content.read_data.cause,
                            p_cb_data->cb_content.read_data.value_size,
                            TRACE_BINARY(p_cb_data->cb_content.read_data.value_size, p_cb_data->cb_content.read_data.p_value));
            break;

        case AMS_READ_FROM_ENTITY_ATTR:
            {
                struct
                {
                    uint8_t app_link_id;
                    uint16_t data_len;
                    uint8_t data[];
                } __attribute__((packed)) *rpt = NULL;

                uint16_t total_len = sizeof(*rpt) + p_cb_data->cb_content.read_data.value_size;

                rpt = calloc(1, total_len);
                if (rpt)
                {
                    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
                    rpt->app_link_id = p_link->id;
                    rpt->data_len = p_cb_data->cb_content.read_data.value_size;
                    memcpy(rpt->data, p_cb_data->cb_content.read_data.p_value, rpt->data_len);
                    send_cmd_evt(EVENT_AMS_READ_ENTITY_ATTR, (uint8_t *)rpt, total_len);
                    APP_PRINT_TRACE2("ams cb: app_link_id %d, data_len %d", rpt->app_link_id, rpt->data_len);
                    free(rpt);
                }
            }
            break;

        default:
            break;
        }
        break;

    case AMS_CLIENT_CB_TYPE_WRITE_RESULT:
        {
            if (p_cb_data->cb_content.write_result.cause != ATT_SUCCESS)
            {
                APP_PRINT_ERROR1("ams cb: AMS_CLIENT_CB_TYPE_WRITE_RESULT: Failed, cause 0x%x",
                                 p_cb_data->cb_content.write_result.cause);
            }
            switch (p_cb_data->cb_content.write_result.type)
            {
            case AMS_WRITE_REMOTE_CMD_NOTIFY_ENABLE:
                {
                    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
                    send_cmd_evt(EVENT_AMS_REMOTE_CMD_NOTIFY_EN, &p_link->id, sizeof(p_link->id));
                    ams_subscribe_entity_upd(conn_handle, true);
                }
                break;

            case AMS_WRITE_REMOTE_CMD_NOTIFY_DISABLE:
                break;

            case AMS_WRITE_ENTITY_UPD_NOTIFY_ENABLE:
                {
                    T_APP_LE_LINK   *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
                    send_cmd_evt(EVENT_AMS_ENTITY_UPD_NOTIFY_EN, &p_link->id, sizeof(p_link->id));
                }
                break;
            case AMS_WRITE_ENTITY_UPD_NOTIFY_DISABLE:
                break;

            case AMS_WRITE_ENTITY_ATTR_VALUE:
                break;

            case AMS_WRITE_REMOTE_CMD_VALUE:
                break;

            default:
                break;
            }
            APP_PRINT_INFO1("AMS_CLIENT_CB_TYPE_WRITE_RESULT: type = 0x%x",
                            p_cb_data->cb_content.write_result.type);
        }
        break;

    case AMS_CLIENT_CB_TYPE_DISCONNECT_INFO:
        {
            APP_PRINT_INFO1("ams cb: AMS_CLIENT_CB_TYPE_DISCONNECT_INFO: conn_handle = 0x%04x", conn_handle);
        }
        break;

    default:
        break;
    }
}


void ams_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr, uint8_t *ack_pkt)
{
    set_trans_info(app_idx, cmd_path);
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE2("ams_handle_cmd: cmd_id 0x%04x, app_link_id %d", cmd_id, cmd_ptr[PARAM_START_POS]);

    switch (cmd_id)
    {
    case CMD_AMS_REGISTER:
        {
            uint8_t app_link_id = cmd_ptr[PARAM_START_POS];
        }
        break;

    case CMD_AMS_WRITE_REMOTE_CMD:
        {
            uint8_t app_link_id = cmd_ptr[PARAM_START_POS];
            uint8_t rmt_cmd_id = cmd_ptr[PARAM_START_POS + 1];
            ams_write_remote_cmd(app_db.le_link[app_link_id].conn_handle, (T_AMS_REMOTE_CMD_ID)rmt_cmd_id);
        }
        break;

    case CMD_AMS_WRITE_ENTITY_UPD_CMD:
        {
            uint8_t app_link_id = cmd_ptr[PARAM_START_POS];
            uint8_t len = cmd_ptr[PARAM_START_POS + 1];
            ams_write_entity_upd_cmd(app_db.le_link[app_link_id].conn_handle, &cmd_ptr[PARAM_START_POS + 1 + 1],
                                     len);
        }
        break;
    case CMD_AMS_READ_ENTITY_ATTR:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t app_link_id;
                uint8_t entity_id;
                uint8_t attr_id;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            T_AMS_ENTITY_ATTR entity_attr = {};
            entity_attr.entity_id = (T_AMS_ENTITY_ID)cmd->entity_id;

            switch (entity_attr.entity_id)
            {
            case ENTITY_ID_PLAYER:
                entity_attr.attr_id.player_atrr_id = (T_AMS_ENTITY_PLAYER_ATTR_ID)cmd->attr_id;
                break;
            case ENTITY_ID_QUEUE:
                entity_attr.attr_id.queue_attr_id = (T_AMS_ENTITY_QUEUE_ATTR_ID)cmd->attr_id;
                break;
            case ENTITY_ID_TRACK:
                entity_attr.attr_id.track_attr_id = (T_AMS_ENTITY_TRACK_ATTR_ID)cmd->attr_id;
                break;

            default:
                break;
            }

            APP_PRINT_TRACE3("ams_handle_cmd: app_link_id %d, entity_id 0x%x, attr_id 0x%x", cmd->app_link_id,
                             cmd->entity_id, cmd->attr_id);

            ams_read_entity_attr(app_db.le_link[cmd->app_link_id].conn_handle, entity_attr);
        }
        break;

    default:
        break;
    }

    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
}


void ams_init(void)
{
    ams_client_init(cb);
}

#endif
