/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "app_gui.h"
#include "board.h"
#include "section.h"
#include "io_dlps.h"
#include "rtl876x.h"
#include "os_sched.h"
#include "trace.h"
#include "trace.h"
#include "fmc_api.h"
#include "fmc_api_ext.h"
#include "feature_check.h"
#include "app_panel_init.h"
#include "gui_server.h"
#include "app_gui_bt_policy.h"
#include "fmc_api.h"
#include "pm.h"
#include "drv_lcd.h"
#if (LCD_INTERFACE == LCD_INTERFACE_QSPI)
#include "module_lcd_qspi.h"
#elif (LCD_INTERFACE == LCD_INTERFACE_8080)
#include "module_lcd_8080.h"
#elif (LCD_INTERFACE == LCD_INTERFACE_LCDC_QSPI)
#include "module_lcd_lcdc_qspi.h"
#endif
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
#include "lcd_st77916_320_385_qspi.h"
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#include "lcd_sh8601z_454_qspi.h"
#endif
#include "module_psram.h"
#include "drv_touch.h"
#if (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CS816T)
#include "module_touch_cst816t.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_GT9147)
#include "module_touch_gt9147.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CST816D)
#include "module_touch_cst816d.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CHSC5816)
#include "module_touch_chsc5816.h"
#endif
#include "app_io_resource_init.h"
#if (ENABLE_TE_FOR_LCD == 1)
#include "module_lcd_te.h"
#endif

#if F_GUI_BENCHMARK_SUPPORT
#include "benchmark_common.h"
#endif

uint32_t app_panel_get_cpu_freq(void)
{
#if TARGET_RTL8773DO
    return 160;
#else
    return 100;
#endif
}

void app_flash_resource_init(void)
{
    /*flash set*/
    bool ret = fmc_flash_try_high_speed_mode(FMC_SPIC_ID_0, FMC_FLASH_NOR_4_BIT_MODE);
    APP_PRINT_INFO1("app_flash_resource_init: set flash mode result %d", ret);

    uint32_t spic0_freq = 0;
    ret = fmc_flash_nor_clock_switch(FMC_SPIC_ID_0, 160, &spic0_freq);
    APP_PRINT_INFO1("app_flash_resource_init: set flash clock result %d", ret);
}

bool app_psram_check_support(void)
{
    uint8_t pkg_id = feature_check_get_pkg_id();
    APP_PRINT_INFO1("app_psram_check_support: pkg_id %d", pkg_id);
#if TARGET_RTL8773DO
    return false;
#elif CONFIG_SOC_SERIES_RTL8773E
    if (pkg_id == 0x02 || pkg_id == 0x04) // refer to platform_check.h   8773ewe-vp  8773ewp
    {
        return true;
    }
    else
    {
        return false;
    }
#else
    if (pkg_id == 0x11)  // refer to platform_check.h   8763ewe-vp
    {
        return true;
    }
    else
    {
        return false;
    }
#endif
}

void app_psram_resource_init(void)
{
    /*psram set*/
    if (app_psram_check_support() == true)
    {
        APP_PRINT_INFO0("app_psram_resource_init: IC support psram, enable init process");
        app_psram_init();
    }
}

bool app_components_init(void)
{
    bool gui_check_flag = false;
    touch_pin_config(TOUCH_I2C_SCL, TOUCH_I2C_SDA, TOUCH_INT, TOUCH_RST);
    gui_check_flag = drv_touch_init();
    if (gui_check_flag == false)
    {
        APP_PRINT_ERROR0("app_components_init: gui check failed");
        return false;
    }

#if (LCD_INTERFACE == LCD_INTERFACE_QSPI)
    lcd_pin_config(LCD_RST);
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
    lcd_bl_pin_config(LCD_BL);
#endif
#elif (LCD_INTERFACE == LCD_INTERFACE_8080)
    lcd_pin_config(LCD_RST, LCD_POWER_EN, LCD_8080_BL);
#elif (LCD_INTERFACE == LCD_INTERFACE_LCDC_QSPI)
    lcd_pin_config(LCD_RST);
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
    return true;
}

void app_gui_init(void)
{
    if (app_components_init() == false)
    {
        return;
    }

    bt_mgr_cback_register(app_gui_bt_policy_cback);

    app_flash_resource_init();
    app_psram_resource_init();

#ifdef ENABLE_RTK_GUI
    gui_server_init();
    app_io_resource_request_init();
    gui_set_keep_active_time(0xffffffff);
#endif
}

#endif


/*-----------------------------------------------------------*/
