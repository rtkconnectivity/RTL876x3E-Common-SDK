/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/

#include "app_gui.h"
#include "board.h"
#include "section.h"
#include "trace.h"
#include "fmc_api.h"
#include "fmc_api_ext.h"
#include "pm.h"
#include "app_panel_init.h"

#if (RTK_BT_TASK == 1)
#include "bt_task.h"
#endif
#include "gui_server.h"
#include "drv_lcd.h"
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
#include "drv_touch.h"
#if (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CS816T)
#include "module_touch_cst816t.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_GT9147)
#include "module_touch_gt9147.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CST816D)
#include "module_touch_cst816d.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CHSC6417)
#include "module_touch_chsc6417.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CHSC5816)
#include "module_touch_chsc5816.h"
#endif
#if ENABLE_PSRAM_FOR_LCD
#include "module_psram.h"
#endif
#if (ENABLE_TE_FOR_LCD == 1)
#include "module_lcd_te.h"
#endif
#if (ENABLE_BENCHMARK == 1)
#include "benchmark_common.h"
#endif

#define DASHBOARD_CPU_FREQ_MHZ             (100)
static uint8_t dashboard_cpu_handle = 0;

void gui_main(void)
{
    gui_server_init();
#if (ENABLE_BENCHMARK == 1)
    app_benchmark_start();
#else
    gui_set_keep_active_time(5000);
#endif
}

void app_task_init(void)
{
#if (RTK_BT_TASK == 1) || (RTK_BLE_TASK == 1)
    bt_task_init();
#endif
    gui_main();
#ifdef MODULE_USING_PORT_FOR_LVGL
    lv_8762g_demo();
#endif
}

void app_system_clock_init(void)
{
    uint32_t cpu_freq = 0;
    pm_cpu_freq_init();
    pm_cpu_freq_req(&dashboard_cpu_handle, DASHBOARD_CPU_FREQ_MHZ, &cpu_freq);
    APP_PRINT_INFO1("app_system_clock_init: cpu_freq = %d", cpu_freq);
    bool ret = fmc_flash_try_high_speed_mode(FMC_SPIC_ID_0, FMC_FLASH_NOR_4_BIT_MODE);
    if (ret == true)
    {
        APP_PRINT_INFO0("app_system_clock_init: flash set 4 bits success");
    }
    else
    {
        APP_PRINT_ERROR1("app_system_clock_init: flash set to 4 bits fail, ret = %d", ret);
    }

    uint32_t spic0_freq = 0;
    fmc_flash_set_seq_trans(FMC_SPIC_ID_0, true);
    ret = fmc_flash_nor_clock_switch(FMC_SPIC_ID_0, 160, &spic0_freq);

    if (ret == true)
    {
        APP_PRINT_INFO0("app_system_clock_init: set flash clock 160M success");
    }
    else
    {
        APP_PRINT_ERROR0("app_system_clock_init: set flash clock 160M fail");
    }

#if ENABLE_PSRAM_FOR_LCD
    app_wb_opi_psram_init();
#endif
}

void app_components_init(void)
{
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
    drv_lcd_init();
    touch_pin_config(TOUCH_I2C_SCL, TOUCH_I2C_SDA, TOUCH_INT, TOUCH_RST);
    drv_touch_init();
}

/*-----------------------------------------------------------*/
