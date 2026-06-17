/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "stdlib.h"
#include "os_msg.h"
#include "app_console.h"
#include "app_transfer_cfg.h"
#include "app_io_msg.h"
#include "console_uart.h"
#include "app_io_msg.h"
#include "console.h"
#include "app_cmd.h"
#include "app_uart.h"
#include "app_util.h"
#include "app_main.h"
#include "trace.h"


#if F_APP_ONE_WIRE_UART_SUPPORT
#include "app_one_wire_uart.h"
#endif

#include "bqb.h"

#define CONSOLE_TX_BUFFER_LARGE             1088
#define CONSOLE_RX_BUFFER_LARGE             1088
#define CONSOLE_TX_BUFFER_MEDIUM            512
#define CONSOLE_RX_BUFFER_MEDIUM            512
#define CONSOLE_TX_BUFFER_SMALL             256
#define CONSOLE_RX_BUFFER_SMALL             256

#define CONSOLE_TX_BUFFER_SIZE              CONSOLE_TX_BUFFER_SMALL
#define CONSOLE_RX_BUFFER_SIZE              CONSOLE_TX_BUFFER_SMALL
#define CONSOLE_PARSER_RX_BUFFER_SIZE       512

#if F_APP_BT_AUDIO_TRANSMITTER_MP3_DEMO_SUPPORT
#undef CONSOLE_TX_BUFFER_SIZE
#define CONSOLE_TX_BUFFER_SIZE              CONSOLE_TX_BUFFER_LARGE

#undef CONSOLE_RX_BUFFER_SIZE
#define CONSOLE_RX_BUFFER_SIZE              CONSOLE_RX_BUFFER_LARGE

#undef CONSOLE_PARSER_RX_BUFFER_SIZE
#define CONSOLE_PARSER_RX_BUFFER_SIZE       CONSOLE_RX_BUFFER_SIZE
#endif

#if F_APP_UART_DFU
#undef CONSOLE_TX_BUFFER_SIZE
#define CONSOLE_TX_BUFFER_SIZE              CONSOLE_RX_BUFFER_MEDIUM

#undef CONSOLE_RX_BUFFER_SIZE
#define CONSOLE_RX_BUFFER_SIZE              CONSOLE_RX_BUFFER_MEDIUM

#undef CONSOLE_PARSER_RX_BUFFER_SIZE
#define CONSOLE_PARSER_RX_BUFFER_SIZE       CONSOLE_RX_BUFFER_SIZE
#endif

static void app_console_handle_wake_up(T_CONSOLE_UART_EVENT event)
{
    T_IO_MSG dlps_msg;

    dlps_msg.type = IO_MSG_TYPE_GPIO;
    dlps_msg.subtype = IO_MSG_GPIO_UART_WAKE_UP;

    /* Send MSG to APP task */
    app_io_msg_send(&dlps_msg);
}

void app_console_uart_driver_init(uint8_t index)
{
    console_uart_driver_init(index);
}

static void app_console_uart_init(void)
{
#if F_APP_CONSOLE_SUPPORT
    T_CONSOLE_PARAM console_param;
    T_CONSOLE_OP console_op;
    T_CONSOLE_UART_CONFIG console_uart_config;

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
    console_uart_config.one_wire_uart_support = app_cfg_const.one_wire_uart_support;
    console_uart_config.uart_rx_pinmux = app_cfg_const.one_wire_uart_support ?
                                         app_cfg_const.one_wire_uart_data_pinmux : app_cfg_const.data_uart_rx_pinmux;
#else
    console_uart_config.one_wire_uart_support = 0;
    console_uart_config.uart_rx_pinmux = app_cfg_const.data_uart_rx_pinmux;
#endif
    console_uart_config.uart_tx_pinmux = app_cfg_const.data_uart_tx_pinmux;
    console_uart_config.rx_wake_up_pinmux = app_cfg_const.rx_wake_up_pinmux;
    console_uart_config.enable_rx_wake_up = app_cfg_const.enable_rx_wake_up;
    console_uart_config.data_uart_baud_rate = app_transfer_cfg.data_uart_baud_rate;
    console_uart_config.callback = app_console_handle_wake_up;

    console_uart_config.uart_rx_dma_enable = false;
    console_uart_config.uart_tx_dma_enable = false;

#if (F_APP_BT_AUDIO_TRANSMITTER_MP3_DEMO_SUPPORT || F_APP_UART_DFU)
    console_uart_config.uart_tx_dma_enable = app_cfg_const.one_wire_uart_support ? false : true;
    console_uart_config.uart_rx_dma_enable = app_cfg_const.one_wire_uart_support ? false : true;
#endif

#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0)
    console_param.tx_buf_size = CONSOLE_TX_BUFFER_SIZE;
    console_param.rx_buf_size = CONSOLE_RX_BUFFER_SIZE;
    app_db.external_mcu_mtu = console_param.tx_buf_size;
    console_param.tx_wakeup_pin = app_cfg_const.tx_wake_up_pinmux;
    console_param.rx_wakeup_pin = app_cfg_const.rx_wake_up_pinmux;

    console_op.init = console_uart_init;
    console_op.tx_wakeup_enable = NULL; //console_uart_tx_wakeup_enable;
    console_op.rx_wakeup_enable = NULL; //console_uart_rx_wakeup_enable;
    console_op.write = console_uart_write;
    console_op.wakeup = console_uart_wakeup;
    console_op.tx_empty = NULL;
#else
    app_db.external_mcu_mtu = 512;
#endif

    console_uart_config_init(&console_uart_config);

#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0)
    console_init(&console_param, &console_op);
    console_set_mode(CONSOLE_MODE_BINARY);
#endif
#endif
}

#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0)
bool app_console_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_msg_send(&msg);
}

typedef struct
{
    uint16_t            rx_count;
    uint16_t            rx_process_offset;
    uint8_t             *rx_buffer;
    uint32_t            rx_w_idx;
    uint32_t            rx_buf_size;
} T_CONSOLE_PARSER;

T_CONSOLE_PARSER console_parser;

bool app_console_cmd_set_parser(uint8_t *buf, uint32_t len)
{
    uint16_t cmd_len;
    uint16_t temp_index;
    //uint8_t rx_seqn;

    if (console_parser.rx_w_idx + len <= console_parser.rx_buf_size)
    {
        memcpy(&console_parser.rx_buffer[console_parser.rx_w_idx], buf, len);
        console_parser.rx_count += len;
        console_parser.rx_w_idx += len;
        if (console_parser.rx_w_idx == console_parser.rx_buf_size)
        {
            console_parser.rx_w_idx = 0;
        }
    }
    else
    {
        uint32_t temp;

        temp = console_parser.rx_buf_size - console_parser.rx_w_idx;
        memcpy(&console_parser.rx_buffer[console_parser.rx_w_idx], buf, temp);
        memcpy(&console_parser.rx_buffer[0], &buf[temp], len - temp);
        console_parser.rx_count += len;
        console_parser.rx_w_idx  = len - temp;
    }


    while (console_parser.rx_count)
    {
        if (console_parser.rx_buffer[console_parser.rx_process_offset] == CMD_SYNC_BYTE)
        {
            temp_index = console_parser.rx_process_offset;
            //sync_byte(1) and Seqn(1) and length(2) and cmd id(2) and checksum(1)
            if (console_parser.rx_count >= 7)
            {
                temp_index++;
                if (temp_index >= CONSOLE_PARSER_RX_BUFFER_SIZE)
                {
                    temp_index = 0;
                }
                //rx_seqn = console_parser.rx_buffer[temp_index];
                temp_index++;
                if (temp_index >= CONSOLE_PARSER_RX_BUFFER_SIZE)
                {
                    temp_index = 0;
                }
                cmd_len = console_parser.rx_buffer[temp_index];
                temp_index++;
                if (temp_index >= CONSOLE_PARSER_RX_BUFFER_SIZE)
                {
                    temp_index = 0;
                }
                cmd_len |= (console_parser.rx_buffer[temp_index] << 8);
                //only process when received whole command
                if (console_parser.rx_count >= (cmd_len + 5))
                {
                    uint8_t     *temp_ptr;
                    uint8_t     check_sum;

                    temp_ptr = malloc(cmd_len + 5);
                    if (temp_ptr != NULL)
                    {
                        if ((console_parser.rx_process_offset + cmd_len + 5) > CONSOLE_PARSER_RX_BUFFER_SIZE)
                        {
                            uint16_t    temp_len;

                            temp_len = CONSOLE_PARSER_RX_BUFFER_SIZE - console_parser.rx_process_offset;
                            memcpy(temp_ptr, &console_parser.rx_buffer[console_parser.rx_process_offset], temp_len);
                            temp_index = temp_len;
                            temp_len = (cmd_len + 5) - (CONSOLE_PARSER_RX_BUFFER_SIZE - console_parser.rx_process_offset);
                            memcpy(&temp_ptr[temp_index], &console_parser.rx_buffer[0], temp_len);
                        }
                        else
                        {
                            memcpy(temp_ptr, &console_parser.rx_buffer[console_parser.rx_process_offset],
                                   (cmd_len + 5));
                        }
                        check_sum = app_util_calc_checksum(&temp_ptr[1], cmd_len + 3);//from seqn
                        if (check_sum == temp_ptr[cmd_len + 4])
                        {
                            app_console_send_msg(IO_MSG_CONSOLE_BINARY_RX, temp_ptr);
                        }
                        else
                        {
                            APP_PRINT_ERROR2("app_console_cmd_set_parser: checksum error calc check_sum 0x%x, rx check_sum 0x%x",
                                             check_sum, temp_ptr[cmd_len + 4]);
                            free(temp_ptr);
                        }
                    }
                    console_parser.rx_count -= (cmd_len + 5);
                    console_parser.rx_process_offset += (cmd_len + 5);
                    if (console_parser.rx_process_offset >= CONSOLE_PARSER_RX_BUFFER_SIZE)
                    {
                        console_parser.rx_process_offset -= CONSOLE_PARSER_RX_BUFFER_SIZE;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            console_parser.rx_count--;
            console_parser.rx_process_offset++;
            if (console_parser.rx_process_offset >= CONSOLE_PARSER_RX_BUFFER_SIZE)
            {
                console_parser.rx_process_offset = 0;
            }
        }
    }
    return true;
}

bool app_console_parser_register(void)
{
    return console_register_parser(&app_console_cmd_set_parser);
}

void app_console_parser_init(void)
{
    console_parser.rx_buffer = malloc(CONSOLE_PARSER_RX_BUFFER_SIZE);
    console_parser.rx_buf_size = CONSOLE_PARSER_RX_BUFFER_SIZE;
    console_parser.rx_count = 0;
    console_parser.rx_process_offset = 0;
    console_parser.rx_w_idx = 0;
}
#endif

void app_console_init(void)
{
#if F_APP_CONSOLE_SUPPORT
    app_console_uart_init();

#if F_APP_ONE_WIRE_UART_SUPPORT
    if (app_cfg_const.one_wire_uart_support)
    {
        app_one_wire_uart_init();
    }
#endif
    bqb_cmd_register();

#if F_APP_ENABLE_TWO_ONE_WIRE_UART
    app_uart_init();
#else
    app_console_parser_init();
    app_console_parser_register();
#endif
#endif
}
