/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "drv_lcd.h"
#include "trace.h"
#include "section.h"
#include "rtk_hal_lcd.h"

void drv_lcd_power_on(void)
{
    rtk_lcd_hal_init();
}

void drv_lcd_power_off(void)
{
    rtk_lcd_hal_set_reset(true);
}

void drv_lcd_update(uint8_t *framebuffer, uint16_t xStart, uint16_t yStart, uint16_t w,
                    uint16_t h)
{
    rtk_lcd_hal_set_window(xStart, yStart, w, h);
    rtk_lcd_hal_update_framebuffer(framebuffer, w * h);
}

void drv_lcd_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    rtk_lcd_hal_set_window(xStart, yStart, w, h);
}

void drv_lcd_start_transfer(uint8_t *buf, uint32_t len)
{
    //APP_PRINT_INFO2("drv_lcd_start_transfer: buffer 0x%p, len %u", buf, len);
    rtk_lcd_hal_start_transfer(buf, len);
}

void drv_lcd_transfer_done(void)
{
    rtk_lcd_hal_transfer_done();
}

uint32_t drv_lcd_get_screen_width(void)
{
    return rtk_lcd_hal_get_width();
}
uint32_t drv_lcd_get_screen_height(void)
{
    return rtk_lcd_hal_get_height();
}

uint32_t drv_lcd_get_fb_width(void)
{
    return ((rtk_lcd_hal_get_width() + 15) >> 4) << 4;
}
uint32_t drv_lcd_get_fb_height(void)
{
    return rtk_lcd_hal_get_height();
}

uint32_t drv_lcd_get_pixel_bits(void)
{
    return rtk_lcd_hal_get_pixel_bits();
}

ISR_TEXT_SECTION
void Display_Handler(void)
{
    APP_PRINT_TRACE0("Display_Handler");
}

void drv_lcd_init(void)
{
    rtk_lcd_hal_init();
}

void drv_lcd_set_TE_type(T_LCDC_TE_TYPE state)
{
    rtk_lcd_hal_set_TE_type(state);
}

T_LCDC_TE_TYPE drv_lcd_get_TE_type(void)
{
    return rtk_lcd_hal_get_TE_type();
}

void drv_lcd_set_te_trigger_state(bool state)
{
    rtk_lcd_hal_set_te_trigger_state(state);
}

bool drv_lcd_get_te_trigger_state(void)
{
    return rtk_lcd_hal_get_te_trigger_state();
}

void drv_lcd_te_enable(void)
{
    rtk_lcd_hal_te_enable();
}

void drv_lcd_te_disable(void)
{
    rtk_lcd_hal_te_disable();
}

/************** end of file ********************/
