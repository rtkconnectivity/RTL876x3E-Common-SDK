/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "rtl876x.h"
#include "app_io_resource_init.h"
#include "dma_channel.h"
#include "app_gui.h"
#if (LCD_INTERFACE == LCD_INTERFACE_QSPI)
#include "module_lcd_qspi.h"
#elif (LCD_INTERFACE == LCD_INTERFACE_8080)
#include "module_lcd_8080.h"
#elif (LCD_INTERFACE == LCD_INTERFACE_LCDC_QSPI)
#include "module_lcd_lcdc_qspi.h"
#endif
#include "gui_port.h"

uint8_t lcd_dma_ch_num  = 0xA5;


void app_io_resource_request_init(void)
{
#if (LCD_INTERFACE == LCD_INTERFACE_LCDC_QSPI)
    lcd_set_dma_ch_num(0);
#if (ENABLE_PSRAM_FOR_LCD == 1)
    if (!GDMA_channel_request(&lcd_dma_ch_num, NULL, true))
    {
        return;
    }
    else
    {
        APP_PRINT_INFO1("app_io_resource_request_init: lcd_dma_ch_num = %d", lcd_dma_ch_num);
        gui_port_dc_set_dma(lcd_dma_ch_num);
    }
#endif
#else
    if (!GDMA_channel_request(&lcd_dma_ch_num, NULL, true))
    {
        return;
    }
    else
    {
        APP_PRINT_INFO1("lcd dma channel number = %d", lcd_dma_ch_num);
        lcd_set_dma_ch_num(lcd_dma_ch_num);
        gui_port_dc_set_dma(lcd_dma_ch_num);
    }
#endif
}
#endif

