/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <os_sched.h>
#include <trace.h>
#include "app_gui.h"
#include "board.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "app_task.h"
#include "pm.h"
#include "app_io_resource_init.h"
#include "rtk_hal_lcd.h"
#if (LCD_INTERFACE == LCD_INTERFACE_QSPI)
#include "module_lcd_qspi.h"
#elif (LCD_INTERFACE == LCD_INTERFACE_QSPI_1_BIT)
#include "module_lcd_qspi_1b.h"
#elif (LCD_INTERFACE == LCD_INTERFACE_8080)
#include "module_lcd_8080.h"
#elif (LCD_INTERFACE == LCD_INTERFACE_SPI)
#include "module_lcd_spi.h"
#endif
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
#include "lcd_st77916_320_385_qspi.h"
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
#include "lcd_st7801_368_448_qspi.h"
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#include "lcd_sh8601z_454_qspi.h"
#endif
#if (ENABLE_TE_FOR_LCD == 1)
#include "module_lcd_te.h"
#endif

#define   LCD_QSPI_CPU_FREQ_MHZ                   (100)
#define   TEST_IMAGE_ADDR                         (void *)0x027fa448

static uint8_t lcd_qspi_cpu_freq_handle = 0;

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Peripheral APP, thus only one APP task is init here
 * @return   void
 */
void task_init(void)
{
    app_task_init();
}

static void display_clock_config(void)
{
    uint32_t cpu_freq = 0;
    pm_cpu_freq_init();
    pm_cpu_freq_req(&lcd_qspi_cpu_freq_handle, LCD_QSPI_CPU_FREQ_MHZ, &cpu_freq);
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int main(void)
{
    DBG_DIRECT("APP MAIN, Demo for LCD");

    app_io_resource_request_init();
    display_clock_config();
#if (LCD_INTERFACE == LCD_INTERFACE_QSPI)
    lcd_pin_config(LCD_RST);
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
    lcd_bl_pin_config(LCD_BL);
#endif
#elif (LCD_INTERFACE == LCD_INTERFACE_QSPI_1_BIT)
    lcd_pin_config(LCD_RST, LCD_DCX);
#elif (LCD_INTERFACE == LCD_INTERFACE_8080)
    lcd_pin_config(LCD_RST, LCD_POWER_EN, LCD_8080_BL);
#elif (LCD_INTERFACE == LCD_INTERFACE_SPI)
    lcd_pin_config(LCD_RST, LCD_DCX, LCD_SPI_CS, LCD_SPI_CLK, LCD_SPI_MOSI);
#endif
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
    lcd_vci_en_pin_config(VCI_EN);
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
    lcd_avdd_en_pin_config(LCD_AVDD_EN);
#endif
#if (ENABLE_TE_FOR_LCD == 1)
    lcd_te_pin_config(LCD_TE);
#endif
    rtk_lcd_hal_init();
    rtk_lcd_hal_rect_fill(0, 0, rtk_gui_config.lcd_width, rtk_gui_config.lcd_hight, BLUE);
    task_init();
    os_sched_start();

    return 0;
}
/** @} */ /* End of group LCDC_QSPI_DEMO */


