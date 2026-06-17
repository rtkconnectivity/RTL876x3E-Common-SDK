/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#ifndef _BOARD_H_
#define _BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup IO Driver Config
  * @note user must config it firstly!! Do not change macro names!!
  * @{
  */

#if (LCD_INTERFACE == LCD_INTERFACE_QSPI)
#define LCD_RST                                   P4_4
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
#define LCD_BL                                    P2_3
#endif
#elif (LCD_INTERFACE == LCD_INTERFACE_QSPI_1_BIT)
#define LCD_RST                                   P4_4
#define LCD_DCX                                   P4_3
#elif (LCD_INTERFACE == LCD_INTERFACE_SPI)
#define LCD_RST                                   P4_4
#define LCD_DCX                                   P4_3
#define LCD_SPI_CS                                P9_2
#define LCD_SPI_CLK                               P9_4
#define LCD_SPI_MOSI                              P9_3
#elif (LCD_INTERFACE == LCD_INTERFACE_8080)
#define LCD_RST                                   P9_5
#define LCD_POWER_EN                              P0_1
#define LCD_8080_BL                               P9_4
#endif

#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916) || (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801) || (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#define LCD_TE                                    P2_2
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z_QSPI_1_BIT) || (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z_SPI)
#define LCD_TE                                    P4_5
#endif

#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
#define VCI_EN                                    P4_3
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#define LCD_AVDD_EN                               P2_3
#endif

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif
