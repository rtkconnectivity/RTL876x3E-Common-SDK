/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_PANEL_DB_COMMON__
#define __APP_PANEL_DB_COMMON__



#ifdef __cplusplus
extern "C" {
#endif
#define BATTERY_INVALIE_VALUE      (0x7F)

typedef enum
{
    GUI_LISTENING_OFF,
    GUI_LISTENING_NORMAL_APT,
    GUI_LISTENING_ANC,
    GUI_LISTENING_LLAPT,
} T_APP_GUI_LISTENING_MODE;

void app_panel_db_common_awake_gui_task(void);

#ifdef __cplusplus
}
#endif

#endif
