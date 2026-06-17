/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_PANEL_INIT__
#define __APP_PANEL_INIT__



#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t lcd_dma_ch_num;
void app_gui_init(void);
uint32_t app_panel_get_cpu_freq(void);

#ifdef __cplusplus
}
#endif

#endif
