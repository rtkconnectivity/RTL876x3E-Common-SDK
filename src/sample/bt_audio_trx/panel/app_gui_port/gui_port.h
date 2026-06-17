/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __GUI_PORT_H__
#define __GUI_PORT_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    GUI_PORT_SKIP_FB                   = 0x00,
} T_GUI_PORT_TIMER;

void gui_port_dc_init(void);
void gui_port_indev_init(void);
void gui_port_os_init(void);
void gui_port_dc_set_dma(uint8_t num);
void app_gui_set_bond_flag(void);
#ifdef __cplusplus
}
#endif

#endif
