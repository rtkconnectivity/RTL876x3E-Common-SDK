/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "app_gui.h"

const T_RTK_GENERAL_GUI_CONFIG rtk_gui_config =
{
    .lcd_width = LCD_WIDTH,
    .lcd_hight = LCD_HIGHT,
    .lcd_section_height = LCD_SECTION_HEIGHT,
    .pixel_format = PIXEL_FORMAT,
    .pixel_bytes = PIXEL_BYTES,
    .lcd_section_byte_len = LCD_SECTION_BYTE_LEN,
    .max_section_count = MAX_SECTION_COUNT,
    .total_section_count = TOTAL_SECTION_COUNT,
};
