/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "stdlib.h"
#include "console.h"
#include "app_util.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_link_util_cs.h"
#include "app_transfer.h"
#include "app_report.h"
#include "app_transfer.h"
#include "app_audio_cfg.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "eq_utils.h"
#include "app_eq.h"

#if (F_APP_SPI_ROLE_MASTER || F_APP_SPI_ROLE_SLAVE)
#include "app_spi_api.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_hid_report_desc.h"
#include "app_tri_dongle_usb_hid_ctrl.h"
#include "app_usb.h"
#include "app_usb_hid.h"
uint8_t usb_hid_tx_seqn = 0;
#endif

#if F_APP_NXP_UWB_CALIBRATION_DATA_DUMP
#include "stdarg.h"
#include "stdio.h"
#include "console_uart.h"
char tx_buffer[256] = {0};
#endif

static uint8_t uart_tx_seqn[3] = {0, 0, 0};

static void app_report_spi_event(uint16_t event_id, uint8_t *data, uint16_t len)
{
    uint8_t result = 0xff;

#if F_APP_SPI_ROLE_MASTER
    result = app_spi_master_send_data(event_id, data, len);
#elif F_APP_SPI_ROLE_SLAVE
    result = app_spi_slave_send_data(event_id, data, len);
#endif

    if (result != 0)
    {
        APP_PRINT_ERROR1("app_report_spi_event: err 0x%x", result);
    }
}

static void app_report_uart_event(uint16_t event_id, uint8_t *data, uint16_t len, uint8_t index)
{
#if ((F_APP_CONSOLE_SUPPORT == 1) && (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0))
    if (console_get_mode() != CONSOLE_MODE_BINARY)
    {
        return;
    }
#endif

    if (app_cfg_const.enable_data_uart || app_cfg_const.one_wire_uart_support)
    {
        uint16_t total_len;
        uint8_t check_sum;
        uint8_t *p_buf;
        uint16_t checksum_data_len;

        typedef struct
        {
            uint8_t sync_byte;
            uint8_t seq;
            uint16_t pkt_len;
            uint16_t event_id;
            uint8_t data[0];
        }   __attribute__((packed)) T_SEND_BUF;

        total_len = sizeof(T_SEND_BUF) + len + sizeof(check_sum);
        if (total_len > app_db.external_mcu_mtu)
        {
            APP_PRINT_ERROR2("app_report_uart_event: data_len %d is larger than external_mcu_mtu %d",
                             total_len, app_db.external_mcu_mtu);
            return;
        }

        T_SEND_BUF *send_buf = malloc(total_len);
        if (send_buf == NULL)
        {
            return;
        }
        p_buf = (uint8_t *)send_buf + sizeof(send_buf->sync_byte);
        checksum_data_len = total_len - sizeof(send_buf->sync_byte) - sizeof(check_sum);

        uart_tx_seqn[index]++;
        if (uart_tx_seqn[index] == 0)
        {
            uart_tx_seqn[index] = 1;
        }

        send_buf->sync_byte = CMD_SYNC_BYTE;
        send_buf->seq = uart_tx_seqn[index];
        send_buf->pkt_len = len + sizeof(send_buf->event_id);
        send_buf->event_id = event_id;
        if (len)
        {
            memcpy(send_buf->data, data, len);
        }
        check_sum = app_util_calc_checksum(p_buf, checksum_data_len);
        send_buf->data[len] = check_sum;
        if (app_push_data_transfer_queue(CMD_PATH_UART, (uint8_t *)send_buf, total_len, index) == false)
        {
            free(send_buf);
        }
    }
}

#if F_APP_NXP_UWB_CALIBRATION_DATA_DUMP
static void app_report_usb_uwb_event(uint8_t *data, uint16_t len, uint8_t index)
{
    if (app_cfg_const.enable_data_uart)
    {
        if (len == 0)
        {
            return;
        }

        app_push_data_transfer_queue(CMD_PATH_UART, (uint8_t *)data, len, index);
    }
}

int uart_log_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsnprintf((char *)tx_buffer, 256, format, args);
    va_end(args);
    console_write((uint8_t *)tx_buffer, n);
    return n;
}
#endif

#if F_APP_USB_HID_PC_TOOL
static void app_report_usb_hid_event(uint16_t event_id, uint8_t *data, uint16_t len, uint8_t index)
{
    if (app_usb_power_state() != USB_CONFIGURED)
    {
        return;
    }

    uint16_t total_len;
    uint8_t check_sum;

    typedef struct
    {
        uint8_t hid_report_id;
        uint16_t pkt_len;
        uint16_t hid_event;
        uint8_t seq;
        uint8_t data[0];
    }   __attribute__((packed)) T_SEND_HID_ACK_BUF;

    total_len = sizeof(T_SEND_HID_ACK_BUF) + len;

    T_SEND_HID_ACK_BUF *send_hid_ack_buf = malloc(APP_TRI_DONGLE_PC_TOOL_USB_HID_SIZE);
    if (send_hid_ack_buf == NULL)
    {
        return;
    }
    memset(send_hid_ack_buf, 0, APP_TRI_DONGLE_PC_TOOL_USB_HID_SIZE);

    usb_hid_tx_seqn++;
    if (usb_hid_tx_seqn == 0)
    {
        usb_hid_tx_seqn = 1;
    }

    send_hid_ack_buf->hid_report_id = REPORT_ID_CTRL_DATA_IN_REQUEST;
    send_hid_ack_buf->pkt_len = len + sizeof(send_hid_ack_buf->hid_event) + sizeof(
                                    send_hid_ack_buf->seq);
    if (event_id == EVENT_ACK)
    {
        send_hid_ack_buf->hid_event = APP_TRI_DONGLE_HID_ACK_OPCODE;
    }
    else
    {
        send_hid_ack_buf->hid_event = event_id;
    }
    send_hid_ack_buf->seq = usb_hid_tx_seqn;

    if (len)
    {
        memcpy(send_hid_ack_buf->data, data, len);
    }

    APP_PRINT_TRACE2("app_report_usb_hid_event: total_len %d, %b", total_len, TRACE_BINARY(total_len,
                     send_hid_ack_buf));
#if USB_HID_CFU_USE_SINGLE_EP
    app_usb_hid_interrupt_pipe_send_ep4(send_hid_ack_buf, APP_TRI_DONGLE_PC_TOOL_USB_HID_SIZE);
#else
    app_usb_hid_interrupt_pipe_send(send_hid_ack_buf, APP_TRI_DONGLE_PC_TOOL_USB_HID_SIZE);
#endif
    free(send_hid_ack_buf);
}
#endif

static void app_report_le_event(T_APP_LE_LINK *p_link, uint16_t event_id, uint8_t *data,
                                uint16_t len)
{
    if (p_link->state == LE_LINK_STATE_CONNECTED)
    {
        uint8_t *buf;

        buf = malloc(len + 6);
        if (buf == NULL)
        {
            return;
        }

        buf[0] = CMD_SYNC_BYTE;
        p_link->cmd.tx_seqn++;
        if (p_link->cmd.tx_seqn == 0)
        {
            p_link->cmd.tx_seqn = 1;
        }
        buf[1] = p_link->cmd.tx_seqn;
        buf[2] = (uint8_t)(len + 2);
        buf[3] = (uint8_t)((len + 2) >> 8);
        buf[4] = (uint8_t)event_id;
        buf[5] = (uint8_t)(event_id >> 8);

        if (len)
        {
            memcpy(&buf[6], data, len);
        }

        if (app_push_data_transfer_queue(CMD_PATH_LE, buf, len + 6, p_link->id) == false)
        {
            free(buf);
        }
    }
}

static void app_report_gatt_over_bredr_event(T_APP_BR_LINK *p_link, uint16_t event_id,
                                             uint8_t *data,
                                             uint16_t len)
{
    uint8_t *buf;

    buf = malloc(len + 6);
    if (buf == NULL)
    {
        return;
    }

    buf[0] = CMD_SYNC_BYTE;
    p_link->cmd.tx_seqn++;
    if (p_link->cmd.tx_seqn == 0)
    {
        p_link->cmd.tx_seqn = 1;
    }
    buf[1] = p_link->cmd.tx_seqn;
    buf[2] = (uint8_t)(len + 2);
    buf[3] = (uint8_t)((len + 2) >> 8);
    buf[4] = (uint8_t)event_id;
    buf[5] = (uint8_t)(event_id >> 8);

    if (len)
    {
        memcpy(&buf[6], data, len);
    }

    if (app_push_data_transfer_queue(CMD_PATH_GATT_OVER_BREDR, buf, len + 6, p_link->id) == false)
    {
        free(buf);
    }
}

static void app_report_spp_iap_event(T_APP_BR_LINK *p_link, uint16_t event_id, uint8_t *data,
                                     uint16_t len, uint8_t cmd_path)
{
    uint32_t check_prof = 0;

    if (cmd_path == CMD_PATH_SPP)
    {
        check_prof = SPP_PROFILE_MASK;
    }
    else if (cmd_path == CMD_PATH_IAP)
    {
        check_prof = IAP_PROFILE_MASK;
    }

    if (p_link->connected_profile & check_prof)
    {
        uint8_t *buf;

        buf = malloc(len + 6);
        if (buf == NULL)
        {
            return;
        }

        buf[0] = CMD_SYNC_BYTE;
        p_link->cmd.tx_seqn++;
        if (p_link->cmd.tx_seqn == 0)
        {
            p_link->cmd.tx_seqn = 1;
        }
        buf[1] = p_link->cmd.tx_seqn;
        buf[2] = (uint8_t)(len + 2);
        buf[3] = (uint8_t)((len + 2) >> 8);
        buf[4] = (uint8_t)event_id;
        buf[5] = (uint8_t)(event_id >> 8);

        if (len)
        {
            memcpy(&buf[6], data, len);
        }

        if (app_push_data_transfer_queue(cmd_path, buf, len + 6, p_link->id) == false)
        {
            free(buf);
        }
    }
}

static void app_report_spp_iap_raw_data(T_APP_BR_LINK *p_link, uint8_t *data, uint16_t len,
                                        uint8_t cmd_path)
{
    uint32_t check_prof = 0;

    if (cmd_path == CMD_PATH_SPP)
    {
        check_prof = SPP_PROFILE_MASK;
    }
    else if (cmd_path == CMD_PATH_IAP)
    {
        check_prof = IAP_PROFILE_MASK;
    }

    if (p_link->connected_profile & check_prof)
    {
        uint8_t *buf;

        buf = malloc(len);
        if (buf == NULL)
        {
            return;
        }

        memcpy(buf, data, len);

        if (app_push_data_transfer_queue(cmd_path, buf, len, p_link->id) == false)
        {
            free(buf);
        }
    }
}

static void app_report_le_raw_data(T_APP_LE_LINK *p_link, uint8_t *data, uint16_t len)
{
    uint8_t *buf;

    buf = malloc(len);
    if (buf == NULL)
    {
        return;
    }

    memcpy(buf, data, len);

    if (app_push_data_transfer_queue(CMD_PATH_LE, buf, len, p_link->id) == false)
    {
        free(buf);
    }
}

void app_report_event(uint8_t cmd_path, uint16_t event_id, uint8_t app_index, uint8_t *data,
                      uint16_t len)
{
    APP_PRINT_TRACE4("app_report_event: cmd_path %d, event_id 0x%04x, app_index %d, data_len %d",
                     cmd_path, event_id, app_index, len);

    switch (cmd_path)
    {
    case CMD_PATH_UART:
        app_report_uart_event(event_id, data, len, app_index);
        break;

#if F_APP_USB_HID_PC_TOOL
    case CMD_PATH_USB_HID:
        app_report_usb_hid_event(event_id, data, len, app_index);
        break;
#endif

    case CMD_PATH_LE:
        if (app_index < MAX_BLE_LINK_NUM)
        {
            app_report_le_event(&app_db.le_link[app_index], event_id, data, len);
        }
        break;

    case CMD_PATH_GATT_OVER_BREDR:
        if (app_index < MAX_BR_LINK_NUM)
        {
            app_report_gatt_over_bredr_event(&app_db.br_link[app_index], event_id, data, len);
        }
        break;

    case CMD_PATH_SPP:
    case CMD_PATH_IAP:
        if (app_index < MAX_BR_LINK_NUM)
        {
            app_report_spp_iap_event(&app_db.br_link[app_index], event_id, data, len, cmd_path);
        }
        break;

    case CMD_PATH_SPI:
        {
            app_report_spi_event(event_id, data, len);
        }

    default:
        break;
    }
}

void app_report_event_broadcast(uint16_t event_id, uint8_t *buf, uint16_t len)
{
    T_APP_BR_LINK *br_link;
    T_APP_LE_LINK *le_link;
    uint8_t        i;

    app_report_event(CMD_PATH_UART, event_id, 0, buf, len);

    for (i = 0; i < MAX_BR_LINK_NUM; i ++)
    {
        br_link = &app_db.br_link[i];

        if (br_link->cmd.enable == true)
        {
            if (br_link->connected_profile & SPP_PROFILE_MASK)
            {
                app_report_event(CMD_PATH_SPP, event_id, i, buf, len);
            }

            if (br_link->connected_profile & IAP_PROFILE_MASK)
            {
                app_report_event(CMD_PATH_IAP, event_id, i, buf, len);
            }

            if (br_link->connected_profile & GATT_PROFILE_MASK)
            {
                app_report_event(CMD_PATH_GATT_OVER_BREDR, event_id, i, buf, len);
            }
        }
    }

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        le_link = &app_db.le_link[i];

        if (le_link->state == LE_LINK_STATE_CONNECTED)
        {
            if (le_link->cmd.enable == true)
            {
                app_report_event(CMD_PATH_LE, event_id, i, buf, len);
            }
        }
    }
}

void app_report_raw_data(uint8_t cmd_path, uint8_t app_index, uint8_t *data,
                         uint16_t len)
{
    APP_PRINT_TRACE3("app_report_raw_data: cmd_path %d, app_index %d, data_len %d", cmd_path, app_index,
                     len);

    switch (cmd_path)
    {
    case CMD_PATH_SPP:
    case CMD_PATH_IAP:
    case CMD_PATH_GATT_OVER_BREDR:
        if ((app_index < MAX_BR_LINK_NUM) && data)
        {
            app_report_spp_iap_raw_data(&app_db.br_link[app_index], data, len, cmd_path);
        }
        break;

    case CMD_PATH_LE:
        if ((app_index < MAX_BLE_LINK_NUM) && data)
        {
            app_report_le_raw_data(&app_db.le_link[app_index], data, len);
        }
        break;

    default:
        break;
    }
}

void app_report_eq_idx(T_EQ_DATA_UPDATE_EVENT eq_data_update_event)
{
    if (!app_db.eq_ctrl_by_src)
    {
        return;
    }

    uint8_t buf[3];

    if (app_db.spk_eq_mode == GAMING_MODE)
    {
        buf[0] = app_cfg_nv.eq_idx_gaming_mode_record;
    }
    else if (app_db.spk_eq_mode == ANC_MODE)
    {
        buf[0] = app_cfg_nv.eq_idx_anc_mode_record;
    }
#if F_APP_VOICE_SPK_EQ_SUPPORT
    else if (app_db.spk_eq_mode == VOICE_SPK_MODE)
    {
        buf[0] = 0;
    }
#endif
    else
    {
        buf[0] = app_cfg_nv.eq_idx_normal_mode_record;
    }

    buf[1]  = app_db.spk_eq_mode;
    buf[2]  = eq_data_update_event;
    app_report_event_broadcast(EVENT_AUDIO_EQ_INDEX_REPORT, buf, 3);
}

void app_report_rws_state(void)
{
    uint8_t buf[2];
    buf[0] = GET_STATUS_RWS_STATE;
    buf[1] = app_db.remote_session_state;
    app_report_event_broadcast(EVENT_REPORT_STATUS, buf, 2);
}

void app_report_rws_bud_side(void)
{
    uint8_t buf[2];
    buf[0] = GET_STATUS_RWS_BUD_SIDE;
    buf[1] = app_cfg_const.bud_side;
    app_report_event_broadcast(EVENT_REPORT_STATUS, buf, 2);
}


void app_report_get_bud_info(uint8_t *data)
{
    uint8_t buf[6];
    uint8_t spk_channel = app_cfg_nv.spk_channel;

    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        buf[0] = BUD_TYPE_SINGLE;           // bud type
        buf[1] = INVALID_ROLE_CH;             // primary bud side
        buf[2] = INVALID_ROLE_CH;             // RWS channel
        buf[3] = app_db.local_batt_level;   // battery status
        buf[4] = 0;                         // invalid value
    }
    else
    {
        // bud type
        buf[0] = BUD_TYPE_RWS;

        // primary bud side
        if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
            (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
        {
            // should not go here (report normally by primary bud)
            buf[1] = CHECK_IS_LCH ? R_CH_PRIMARY : L_CH_PRIMARY;
        }
        else
        {
            buf[1] = CHECK_IS_LCH ? L_CH_PRIMARY : R_CH_PRIMARY;
        }

        if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
        {
            spk_channel = app_audio_cfg.solo_speaker_channel;
        }

        // RWS channel
        if (CHECK_IS_LCH)
        {
            buf[2] = (spk_channel << 4) | app_db.remote_spk_channel;
        }
        else
        {
            buf[2] = (app_db.remote_spk_channel << 4) | spk_channel;
        }

        // battery status
        buf[3] = L_CH_BATT_LEVEL;
        buf[4] = R_CH_BATT_LEVEL;
    }

    // case battery volume equals to (case_battery & 0x7F)
    buf[5] = 0xFF; // invalid value

    memcpy(data, &buf[0], sizeof(buf));
}

void app_report_rws_bud_info(void)
{
    uint8_t buf[6];

    app_report_get_bud_info(&buf[0]);
    app_report_event_broadcast(EVENT_REPORT_BUD_INFO, buf, sizeof(buf));
}

#if F_APP_CHARGING_CASE_CMD_SUPPORT
void app_report_charging_case_cmd(uint8_t cmd_path, uint16_t cmd_id, uint8_t app_index,
                                  uint8_t *data, uint16_t len)
{
    APP_PRINT_TRACE3("app_report_charging_case_cmd: cmd_path %d, app_index %d, data %b",
                     cmd_path, app_index, TRACE_BINARY(len, data));

    switch (cmd_path)
    {
    case CMD_PATH_UART:
        app_report_uart_event(cmd_id, data, len, app_index);
        break;

    default:
        break;
    }
}
#endif
