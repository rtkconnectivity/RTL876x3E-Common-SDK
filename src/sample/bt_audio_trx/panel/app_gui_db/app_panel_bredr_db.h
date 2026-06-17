/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_PANEL_BREDR_DB__
#define __APP_PANEL_BREDR_DB__



#ifdef __cplusplus
extern "C" {
#endif
#include "string.h"
#include "app_main.h"

typedef struct app_gui_bredr_link_data_t
{
    bool active;
    uint8_t bd_addr[6];
} T_APP_GUI_BREDR_LINK_DATA;

typedef struct app_gui_bredr_data_t
{
    T_APP_GUI_BREDR_LINK_DATA gui_bredr_link_data[MAX_BR_LINK_NUM];
} T_APP_GUI_BREDR_DATA;

T_APP_GUI_BREDR_LINK_DATA *app_panel_bredr_find_active_br_link(void);
T_APP_GUI_BREDR_LINK_DATA *app_panel_bredr_find_br_link(uint8_t *bd_addr);
void app_panel_bredr_alloc_link(uint8_t *bd_addr);
void app_panel_bredr_free_link(uint8_t *bd_addr);
void app_panel_bredr_check_awake_gui(void);


#ifdef __cplusplus
}
#endif

#endif
