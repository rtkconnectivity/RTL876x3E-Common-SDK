/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_PANEL_DEVICE_DB__
#define __APP_PANEL_DEVICE_DB__



#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
#if F_GUI_CHARGEBOX_DEMO
#include "app_chargebox.h"
#endif

#define APP_GUI_DEVICE_NAME_MAX_LENGTH   (40)

typedef struct app_gui_device_data_t
{
    char device_name[APP_GUI_DEVICE_NAME_MAX_LENGTH];
    uint8_t battery_level;
    uint8_t res[3];
} T_APP_GUI_DEVICE_DATA;

void app_panel_device_db_set_device_name(uint8_t *device_name, uint8_t length);
char *app_panel_device_db_get_device_name(void);
void app_panel_device_db_set_bettery_level(uint8_t level);
uint8_t app_panel_device_db_get_bettery_level(void);
void app_panel_device_db_init(void);

#ifdef __cplusplus
}
#endif

#endif
