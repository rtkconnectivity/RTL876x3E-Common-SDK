/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 1)
#include <string.h>
#include <stdlib.h>
#include "os_mem.h"
#include "os_sync.h"
#include "rtl876x.h"
#include "section.h"
#include "trace.h"
#include "console_uart.h"
#include "app_cfg.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_io_msg.h"
#include "app_uart.h"
#include "app_util.h"

#define UART_BLOCK_SIZE                 512
#define UART_NUM_ENABLE                 3

static void app_uart_buffer_alloc(uint8_t index);

T_UART_CMD cmd_block[UART_NUM_ENABLE];//Currently, only uart0 & uart2 are valid in this case

static void app_uart_buffer_alloc(uint8_t index)
{
    cmd_block[index].id = index;
    cmd_block[index].rx_buffer = malloc(UART_BLOCK_SIZE);
    cmd_block[index].rx_buf_size = UART_BLOCK_SIZE;
    cmd_block[index].rx_count = 0;
    cmd_block[index].rx_process_offset = 0;
    cmd_block[index].rx_w_idx = 0;
}

void app_uart_handle_rx_msg(T_IO_MSG *msg)
{
    uint8_t idx = (uint8_t)(msg->u.param);

    app_uart_parser(idx);
}

void app_uart_parser(uint8_t index)
{
    APP_PRINT_TRACE1("app_uart_parser %d", index);

    uint32_t s;
    T_UART_CMD *p_cmd_block = &(cmd_block[index]);

    if (p_cmd_block->id != index)
    {
        return;
    }

    while (p_cmd_block->rx_count)
    {
        if (p_cmd_block->rx_buffer[p_cmd_block->rx_process_offset] == CMD_SYNC_BYTE)
        {
            uint16_t temp_index = p_cmd_block->rx_process_offset;

            if (p_cmd_block->rx_count >= 7)
            {
                //sync_byte(1) and Seqn(1) and length(2) and cmd id(2) and checksum(1)
                temp_index ++;
                if (temp_index >= UART_BLOCK_SIZE)
                {
                    temp_index = 0;
                }
                uint8_t seqn = p_cmd_block->rx_buffer[temp_index];

                temp_index++;
                if (temp_index >= UART_BLOCK_SIZE)
                {
                    temp_index = 0;
                }

                uint16_t cmd_len = p_cmd_block->rx_buffer[temp_index];

                temp_index++;
                if (temp_index >= UART_BLOCK_SIZE)
                {
                    temp_index = 0;
                }
                cmd_len |= (p_cmd_block->rx_buffer[temp_index] << 8);

                if (p_cmd_block->rx_count >= (cmd_len + 5))
                {
                    uint8_t     *temp_ptr;
                    uint8_t     check_sum;

                    temp_ptr = os_mem_alloc(RAM_TYPE_DATA_ON, (cmd_len + 5));
                    if (temp_ptr != NULL)
                    {
                        if ((p_cmd_block->rx_process_offset + cmd_len + 5) > UART_BLOCK_SIZE)
                        {
                            uint16_t temp_len;

                            temp_len = UART_BLOCK_SIZE - p_cmd_block->rx_process_offset;
                            memcpy(temp_ptr, &(p_cmd_block->rx_buffer[p_cmd_block->rx_process_offset]), temp_len);
                            temp_index = temp_len;
                            temp_len = (cmd_len + 5) - (UART_BLOCK_SIZE - p_cmd_block->rx_process_offset);
                            memcpy(&temp_ptr[temp_index], &(p_cmd_block->rx_buffer[0]), temp_len);
                        }
                        else
                        {
                            memcpy(temp_ptr, &(p_cmd_block->rx_buffer[p_cmd_block->rx_process_offset]),
                                   (cmd_len + 5));
                        }
                        check_sum = app_util_calc_checksum(&temp_ptr[1], cmd_len + 3);//from seqn
                        if (check_sum == temp_ptr[cmd_len + 4])
                        {
                            uint16_t cmd_id = temp_ptr[4] | (temp_ptr[5] << 8);
                            app_handle_cmd_set(&temp_ptr[4], cmd_len, CMD_PATH_UART, seqn, index);
                        }
                        else
                        {
                            APP_PRINT_ERROR2("app_uart_parser: checksum error calc check_sum 0x%x, rx check_sum 0x%x",
                                             check_sum, temp_ptr[cmd_len + 4]);
                        }
                        os_mem_free(temp_ptr);
                    }

                    s = os_lock();
                    p_cmd_block->rx_count -= (cmd_len + 5);
                    os_unlock(s);
                    p_cmd_block->rx_process_offset += cmd_len + 5;

                    if (p_cmd_block->rx_process_offset >= UART_BLOCK_SIZE)
                    {
                        p_cmd_block->rx_process_offset = p_cmd_block->rx_process_offset - UART_BLOCK_SIZE;
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
            s = os_lock();
            p_cmd_block->rx_count--;
            os_unlock(s);
            p_cmd_block->rx_process_offset++;

            if (p_cmd_block->rx_process_offset >= UART_BLOCK_SIZE)
            {
                p_cmd_block->rx_process_offset = 0;
            }
        }
    }
}

RAM_TEXT_SECTION void app_uart_callback(T_CONSOLE_UART_EVENT evt, void *buf, uint16_t len,
                                        uint8_t index)
{
    APP_PRINT_TRACE4("app_uart_callback len %x, index %x, evt %x, data %b", len, index, evt,
                     TRACE_BINARY(len, buf));

    uint32_t s;

    switch (evt)
    {
    case CONSOLE_UART_EVENT_DATA_IND:
        {
            T_IO_MSG  msg;

            if (index == cmd_block[index].id)
            {
                uint8_t w_idx = cmd_block[index].rx_w_idx;

                if (cmd_block[index].rx_w_idx + len <= cmd_block[index].rx_buf_size)
                {
                    s = os_lock();
                    memcpy(&(cmd_block[index].rx_buffer[w_idx]), buf, len);
                    cmd_block[index].rx_count += len;
                    os_unlock(s);
                    cmd_block[index].rx_w_idx += len;

                    if (cmd_block[index].rx_w_idx == cmd_block[index].rx_buf_size)
                    {
                        cmd_block[index].rx_w_idx = 0;
                    }
                }
                else
                {
                    uint32_t temp;

                    temp = cmd_block[index].rx_buf_size - cmd_block[index].rx_w_idx;
                    s = os_lock();
                    memcpy(&(cmd_block[index].rx_buffer[w_idx]), buf, temp);
                    memcpy((uint8_t *)(cmd_block[index].rx_buffer), (uint8_t *)buf + temp, len - temp);
                    cmd_block[index].rx_count += len;
                    os_unlock(s);
                    cmd_block[index].rx_w_idx = len - temp;
                }

                msg.type    = IO_MSG_TYPE_UART;
                msg.subtype = 0;
                msg.u.param = index;

                app_io_msg_send(&msg);
            }
        }
        break;

    default:
        break;
    }
}

void app_uart_init(void)
{
    app_uart_buffer_alloc(0);
    app_uart_buffer_alloc(2);
    console_uart_init(app_uart_callback);
}
#endif
