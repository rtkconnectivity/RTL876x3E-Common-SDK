/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __DRV_LCD_H__
#define __DRV_LCD_H__

#include <stdint.h>
#include "rtk_hal_lcd.h"

/**
    * @brief  Set TE type.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  TE type, please refer to T_LCDC_TE_TYPE.
    * @return none.
    */
void drv_lcd_set_TE_type(T_LCDC_TE_TYPE state);

/**
    * @brief  Get TE type.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void.
    * @return The TE type.
    */
T_LCDC_TE_TYPE drv_lcd_get_TE_type(void);

/**
    * @brief  Set TE signal status.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  TE signal status.
    *     This parameter can be following values:
    *     @arg true: TE signal is triggered.
    *     @arg false: TE signal is not triggered.
    * @return none.
    */
void drv_lcd_set_te_trigger_state(bool state);

/**
    * @brief  Get TE signal status.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void.
    * @return TE signal status.
    *     @retval true: TE signal is triggered.
    *     @retval false: TE signal is not triggered.
    */
bool drv_lcd_get_te_trigger_state(void);

/**
    * @brief  Enable GPIO interrupt for TE pin.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void.
    * @return none.
    */
void drv_lcd_te_enable(void);

/**
    * @brief  Disable GPIO interrupt for TE pin.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void.
    * @return none.
    */
void drv_lcd_te_disable(void);

void drv_lcd_init(void);
void drv_lcd_power_on(void);
void drv_lcd_power_off(void);

void drv_lcd_update(uint8_t *framebuffer, uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void drv_lcd_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void drv_lcd_start_transfer(uint8_t *buf, uint32_t len);
void drv_lcd_transfer_done(void);

uint32_t drv_lcd_get_screen_width(void);
uint32_t drv_lcd_get_screen_height(void);
uint32_t drv_lcd_get_fb_width(void);
uint32_t drv_lcd_get_fb_height(void);
uint32_t drv_lcd_get_pixel_bits(void);

#endif  /* __DRV_LCD_H__ */

