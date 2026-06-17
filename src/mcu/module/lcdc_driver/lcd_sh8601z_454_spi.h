/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef LCD_SH8601Z_454_SPI_H
#define LCD_SH8601Z_454_SPI_H

#include <stdint.h>

void sh8601a_init(void);
void SH8601A_spi_power_off(void);
void SH8601A_spi_power_on(void);
void lcd_SH8601A_spi_454_set_window(uint16_t xStart, uint16_t yStart,
                                    uint16_t xEnd, uint16_t yEnd);

#endif // LCD_SH8601Z_454_QSPI_H
