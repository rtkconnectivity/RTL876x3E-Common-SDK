/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "board.h"
#include "trace.h"
#include "pm.h"
#include "io_dlps.h"
#include "section.h"
#include "rtl876x_uart.h"

static uint32_t dlps_bitmap;

/**
* @brief Console uart use cpu mode tx
*
* @param data Pointer to the buffer to be tx
* @param len length of the buffer
* @retval none
*/
void console_uart_direct(uint8_t *data, uint32_t len)
{
    uint32_t blkcount  = len / UART_TX_FIFO_SIZE;
    uint32_t remainder = len % UART_TX_FIFO_SIZE;
    uint32_t i = 0;

    while (UART_GetFlagState(UART0, UART_FLAG_THR_TSR_EMPTY) != SET);
    for (i = 0; i < blkcount; ++i)
    {
        UART_SendData(UART0, data + UART_TX_FIFO_SIZE * i, UART_TX_FIFO_SIZE);
        /* wait tx fifo empty */
        while (UART_GetFlagState(UART0, UART_FLAG_THR_TSR_EMPTY) != SET);
    }

    while (UART_GetFlagState(UART0, UART_FLAG_THR_TSR_EMPTY) != SET);
    /* send left bytes */
    if (remainder)
    {
        UART_SendData(UART0, data + UART_TX_FIFO_SIZE * i, remainder);
        /* wait tx fifo empty */
        while (UART_GetFlagState(UART0, UART_FLAG_THR_TSR_EMPTY) != SET);
    }
}

RAM_TEXT_SECTION void app_dlps_enable(uint32_t bit)
{
    if (dlps_bitmap & bit)
    {
        APP_PRINT_TRACE3("app_dlps_enable: %08x %08x -> %08x", bit, dlps_bitmap,
                         (dlps_bitmap & ~bit));
    }
    dlps_bitmap &= ~bit;
}

RAM_TEXT_SECTION void app_dlps_disable(uint32_t bit)
{
    if ((dlps_bitmap & bit) == 0)
    {
        APP_PRINT_TRACE3("app_dlps_disable: %08x %08x -> %08x", bit, dlps_bitmap,
                         (dlps_bitmap | bit));
    }

    dlps_bitmap |= bit;
}

RAM_TEXT_SECTION bool app_dlps_check_callback(void)
{

    static uint16_t dlps_bitmap_pre;
    bool dlps_enter_en = false;
    if (dlps_bitmap == 0)
    {
        dlps_enter_en = true;
    }

    if (dlps_bitmap_pre != dlps_bitmap)
    {
        APP_PRINT_WARN2("app_dlps_check_callback: dlps_bitmap_pre 0x%x dlps_bitmap 0x%x", dlps_bitmap_pre,
                        dlps_bitmap);
    }
    dlps_bitmap_pre = dlps_bitmap;

    return dlps_enter_en;
}

/**
    * @brief   Need to handle message in this callback function,when App enter dlps mode
    * @param  void
    * @return void
    */
RAM_TEXT_SECTION void app_dlps_enter_callback(void)
{
    POWERMode lps_mode = power_mode_get();

    DBG_DIRECT("app_dlps_enter_callback: lps_mode %d", lps_mode);
}

RAM_TEXT_SECTION void app_dlps_exit_callback(void)
{
    APP_PRINT_INFO4("dump aon reg. 0x126 = 0x%x, 0x128 = 0x%x, 0x130 = 0x%x, 0x132 = 0x%x",
                    btaon_fast_read_safe(0x126), btaon_fast_read_safe(0x128), btaon_fast_read_safe(0x130),
                    btaon_fast_read_safe(0x132));
}

void app_dlps_init(void)
{
    io_dlps_register();

    /* register of call back function */
    power_check_cb_register(app_dlps_check_callback);

    power_stage_cb_register(app_dlps_enter_callback, POWER_STAGE_STORE);
    power_stage_cb_register(app_dlps_exit_callback, POWER_STAGE_RESTORE);

    SYSBLKCTRL->u_208.BITS_208.r_DSP_CLK_SRC_EN = 0;
    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
}
