/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if TRANSMIT_CLIENT_SUPPORT
#include <string.h>
#include "trace.h"
#include "mem_types.h"
#include "stdlib_corecrt.h"
#include "os_mem.h"
#include "app_cmd.h"
#include "app_flags.h"
#include "transmit_gatt_client.h"
#include "app_le_transfer.h"
#include "app_link_util_cs.h"
#include "ble_audio_def.h"
#include "app_cfg_nv.h"
#include "app_charging_case_cmd.h"
#include "app_report.h"
#if F_APP_CHARGING_CASE_CMD_SUPPORT
#include "app_main.h"
#endif

extern uint8_t GATT_UUID128_TRANSMIT_SRV[16];

static void app_le_transfer_send_packet(uint8_t *bd_addr, uint16_t data_len, uint8_t *p_data)
{
    uint8_t idx = 0;
    uint8_t transmit_data_len = 4;
    uint8_t *p_transmit_data = NULL;
    T_APP_LE_LINK *p_le_link = NULL;

    p_le_link = app_link_find_le_link_by_addr(bd_addr);

    if (p_le_link == NULL)
    {
        return;
    }

    p_le_link->tx_event_seqn++;
    p_transmit_data = calloc(1, data_len + 4);
    p_transmit_data[idx++] = CMD_SYNC_BYTE;
    p_transmit_data[idx++] = p_le_link->tx_event_seqn;
    p_transmit_data[idx++] = data_len;
    p_transmit_data[idx++] = data_len >> 8;
    //p_transmit_data[idx++] = opcode;
    //p_transmit_data[idx++] = opcode >> 8;

    for (int i = 0; i < data_len; i++)
    {
        p_transmit_data[idx++] = p_data[i];
    }

    APP_PRINT_INFO1("app_le_transfer_send_packet: p_transmit_data %s",
                    TRACE_BINARY(data_len + transmit_data_len, p_transmit_data));

    transmit_data_write(p_le_link->conn_handle, p_transmit_data, data_len + transmit_data_len);
    free(p_transmit_data);
}

static bool app_le_transfer_start(uint16_t data_len, uint8_t *p_data)
{
    uint8_t link_num;

    if (p_data == NULL)
    {
        return false;
    }

    link_num = le_get_active_link_num();

    if (link_num == 0)
    {
        APP_PRINT_ERROR0("app_transfer_start: no le link exsit.");
        return false;
    }

    app_le_transfer_send_packet(app_cfg_nv.chargecase_remote_bd, data_len, p_data);

    return true;
}

void app_le_transfer_send_ack(uint16_t event_id, uint8_t result)
{
    uint8_t cmd_ptr[5] = {0};
    APP_PRINT_TRACE2("app_le_transfer_send_ack: event_id %d, result %d", event_id, result);

    cmd_ptr[2] = (uint8_t)event_id;
    cmd_ptr[3] = (uint8_t)(event_id >> 8);
    cmd_ptr[4] = result;

    app_le_transfer_start(5, cmd_ptr);
}

void app_le_transfer_send_battery(uint8_t level)
{
    uint8_t cmd_ptr[3] = {0};
    APP_PRINT_TRACE1("app_le_transfer_send_battery: level %d", level);

    cmd_ptr[0] = 0x07;
    cmd_ptr[1] = 0x81;
    cmd_ptr[2] = level;

    app_le_transfer_start(3, cmd_ptr);
}

void app_le_transfer_volume_control(uint8_t action)
{
    uint8_t cmd_ptr[4] = {0};
    APP_PRINT_TRACE1("app_le_transfer_volume_control: action %d", action);

    switch (action)
    {
    case LE_TRANSFER_CONTROL_VOLUME_UP:
        {
            cmd_ptr[0] = 0x04;
            cmd_ptr[3] = 0x30;

            app_le_transfer_start(4, cmd_ptr);
        }
        break;

    case LE_TRANSFER_CONTROL_VOLUME_DOWN:
        {
            cmd_ptr[0] = 0x04;
            cmd_ptr[3] = 0x31;

            app_le_transfer_start(4, cmd_ptr);
        }
        break;

    default:
        break;
    }
}

#if F_APP_CHARGING_CASE_CMD_SUPPORT
void app_le_transfer_send_chargecase_init_cmd(uint16_t conn_handle)
{
    uint8_t cmd_ptr[8] = {0};
    uint8_t temp_buff[2] = {0};
    T_APP_LE_LINK *p_link;

    cmd_ptr[0] = (uint8_t)EVENT_CHARGER_CASE_CONN_STATUS;
    cmd_ptr[1] = (uint8_t)(EVENT_CHARGER_CASE_CONN_STATUS >> 8);

    p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        memcpy(&cmd_ptr[2], app_cfg_nv.bud_local_addr, 6);
        memcpy(app_cfg_nv.chargecase_remote_bd, p_link->bd_addr, 6);
    }

    app_le_transfer_start(sizeof(cmd_ptr), cmd_ptr);

    temp_buff[0] = (uint8_t)CMD_LANGUAGE_GET;
    temp_buff[1] = (uint8_t)(CMD_LANGUAGE_GET >> 8);
    //confirm common link when charging case connected
    app_le_transfer_start(sizeof(temp_buff), temp_buff);

    app_le_transfer_send_battery(app_db.local_batt_level);
}
#endif

void app_le_transfer_control_cmd_handle(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                        uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[2] | (cmd_ptr[3] << 8));

    APP_PRINT_TRACE3("app_le_transfer_control_cmd_handle: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);

    switch (cmd_id)
    {
    case CMD_SET_VOLUME:
        {
            uint8_t volume_level = cmd_ptr[4];
            uint8_t max_volume = 0x0F;
            uint8_t min_volume = 0;
            if (volume_level < min_volume || volume_level > max_volume)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            else
            {
                app_le_transfer_start(cmd_len - 2, &cmd_ptr[2]);
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_MMI:
    case CMD_SET_VP_VOLUME:
    case CMD_AUDIO_EQ_INDEX_SET:
    case CMD_SET_KEY_MMI_MAP:
    case CMD_LISTENING_MODE_CYCLE_SET:
    case CMD_LISTENING_STATE_SET:
        {
            app_le_transfer_start(cmd_len - 2, &cmd_ptr[2]);

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        break;
    }
}

static T_APP_RESULT app_le_transfer_callback(uint16_t conn_handle, uint8_t *p_srv_uuid,
                                             void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    if (!memcmp(p_srv_uuid, GATT_UUID128_TRANSMIT_SRV, 16))
    {
        T_TRANSMIT_CLT_CB_DATA *p_cb_data = (T_TRANSMIT_CLT_CB_DATA *)p_data;

        switch (p_cb_data->cb_type)
        {
        case TRANSMIT_CLIENT_CB_TYPE_DISC_DONE:
            {
                APP_PRINT_INFO1("app_le_transfer_callback: TRANSMIT_CLIENT_CB_TYPE_DISC_DONE is_found 0x%x",
                                p_cb_data->cb_content.disc_done.is_found);
#if F_APP_CHARGING_CASE_CMD_SUPPORT
                if (p_cb_data->cb_content.disc_done.load_from_ftl)
                {
                    app_le_transfer_send_chargecase_init_cmd(conn_handle);
                }
#endif
            }
            break;

        case TRANSMIT_CLIENT_CB_TYPE_WRITE_RESULT:
            {
                APP_PRINT_INFO3("app_le_transfer_callback: TRANSMIT_CLIENT_CB_TYPE_WRITE_RESULT: conn_handle 0x%x, write type 0x%x, cause 0x%x",
                                conn_handle, p_cb_data->cb_content.write_result.type,
                                p_cb_data->cb_content.write_result.cause);
            }
            break;

        case TRANSMIT_CLIENT_CB_TYPE_CCCD_CFG:
            {
                APP_PRINT_INFO2("app_le_transfer_callback: conn_handle 0x%x, cause 0x%x",
                                p_cb_data->cb_content.cccd_cfg.conn_handle,
                                p_cb_data->cb_content.cccd_cfg.cause);
#if F_APP_CHARGING_CASE_CMD_SUPPORT
                app_le_transfer_send_chargecase_init_cmd(conn_handle);
#endif
            }
            break;

        case TRANSMIT_CLIENT_CB_TYPE_NOTIFY_IND:
            {
                APP_PRINT_INFO2("app_le_transfer_callback: TRANSMIT_CLIENT_CB_TYPE_NOTIFY_IND conn_handle 0x%x, size 0x%x",
                                conn_handle, p_cb_data->cb_content.notify_ind.value_size);
#if F_APP_CHARGING_CASE_CMD_SUPPORT
                app_charging_case_headset_event_handler(conn_handle, p_cb_data->cb_content.notify_ind.p_value,
                                                        p_cb_data->cb_content.notify_ind.value_size);
#endif
            }
            break;

        default:
            break;
        }
    }

    return app_result;
}

void app_le_transfer_init(void)
{
    transmit_client_init(app_le_transfer_callback);
}
#endif
