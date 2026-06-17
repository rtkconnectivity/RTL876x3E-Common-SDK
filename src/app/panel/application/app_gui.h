/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_GUI_H_
#define _APP_GUI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "flash_map.h"

#define LCD_INTERFACE_QSPI                   0
#define LCD_INTERFACE_8080                   1
#define LCD_INTERFACE_QSPI_1_BIT             2
#define LCD_INTERFACE_SPI                    3

#define LCD_DEVICE_ST7789                    0
#define LCD_DEVICE_NT35110                   1
#define LCD_DEVICE_RM69330                   2
#define LCD_DEVICE_SH8601Z                   3
#define LCD_DEVICE_SH8601Z_QSPI_1_BIT        4
#define LCD_DEVICE_ST77916                   5
#define LCD_DEVICE_SH8601Z_SPI               6
#define LCD_DEVICE_ST7801                    7

#define TOUCH_DEVICE_CS816T                  0
#define TOUCH_DEVICE_GT9147                  1
#define TOUCH_DEVICE_CST816D                 2
#define TOUCH_DEVICE_CHSC6417                3
#define TOUCH_DEVICE_CHSC5816                4

#define TARGET_LCD_DEVICE                    LCD_DEVICE_SH8601Z
#define TARGET_TOUCH_DEVICE                  TOUCH_DEVICE_CHSC5816

#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST7789)
#define LCD_INTERFACE                        LCD_INTERFACE_QSPI
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_NT35110)
#define LCD_INTERFACE                        LCD_INTERFACE_8080
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_RM69330)
#define LCD_INTERFACE                        LCD_INTERFACE_QSPI
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#define LCD_INTERFACE                        LCD_INTERFACE_QSPI
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z_QSPI_1_BIT)
#define LCD_INTERFACE                        LCD_INTERFACE_QSPI_1_BIT
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
#define LCD_INTERFACE                        LCD_INTERFACE_QSPI
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z_SPI)
#define LCD_INTERFACE                        LCD_INTERFACE_SPI
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
#define LCD_INTERFACE                        LCD_INTERFACE_QSPI
#endif


#if (TARGET_LCD_DEVICE == LCD_DEVICE_NT35110)
#define LCD_WIDTH                           480
#define LCD_HIGHT                           800
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
#define LCD_WIDTH                           385
#define LCD_HIGHT                           320
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
#define LCD_WIDTH                           368
#define LCD_HIGHT                           448
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#define LCD_WIDTH                           410
#define LCD_HIGHT                           502
#else
#define LCD_WIDTH                           454
#define LCD_HIGHT                           454
#endif
#define PAGE_SWITCH_TIMER_INTERVAL          40
#define LCD_SECTION_HEIGHT                  20
#define FEATURE_PSRAM                       1

#define RGB16BIT_565                        16
#define RGB24BIT_888                        24
#define PIXEL_FORMAT                        RGB16BIT_565
#define PIXEL_BYTES                         (2)
#define LCD_SECTION_BYTE_LEN                (LCD_WIDTH * LCD_SECTION_HEIGHT * PIXEL_BYTES)
#define MAX_SECTION_COUNT                   (0x1FFFF/LCD_SECTION_BYTE_LEN)
#define TOTAL_SECTION_COUNT                 (LCD_HIGHT / LCD_SECTION_HEIGHT + ((LCD_HIGHT % LCD_SECTION_HEIGHT)?1:0))

#define ENABLE_PSRAM_FOR_LCD                (1)

#define ENABLE_TE_FOR_LCD                   (1)

#define FB_DIRECTION_ROTATE                 (0)

#if (PIXEL_FORMAT == RGB24BIT_888)&&(LCD_WIDTH % 4 != 0)
#error "DMA TRANS LIMIT!"
#endif

#define RED                                 (0xf800)
#define BLUE                                (0x001f)

typedef struct
{
    uint16_t lcd_width;
    uint16_t lcd_hight;
    uint16_t lcd_section_height;
    uint16_t pixel_format;
    uint16_t pixel_bytes;
    uint16_t lcd_section_byte_len;
    uint16_t max_section_count;
    uint16_t total_section_count;
} T_RTK_GENERAL_GUI_CONFIG;

extern const T_RTK_GENERAL_GUI_CONFIG rtk_gui_config;

#define FONT_DATA_ADDR USER_DATA1_ADDR

#define ENABLE_BENCHMARK    0

#ifdef __cplusplus
}
#endif

#endif /* _APP_GUI_H_ */
