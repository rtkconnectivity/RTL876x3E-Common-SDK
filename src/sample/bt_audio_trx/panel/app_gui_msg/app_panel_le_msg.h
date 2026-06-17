/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_PANEL_LE_MSG__
#define __APP_PANEL_LE_MSG__



#ifdef __cplusplus
extern "C" {
#endif
#include "app_panel_msg.h"

typedef enum
{
    GUI_ADJUST_VOLUME_UP,
    GUI_ADJUST_VOLUME_DOWN,
} T_GUI_ADJUST_VOLUME_TYPE;

typedef enum
{
    GUI_BASS_SCAN_STOP,
    GUI_BASS_SCAN_START,
} T_GUI_BASS_SCAN_ACTION_TYPE;

typedef enum
{
    GUI_LE_BST_RECEPTION_STOP,
    GUI_LE_BST_RECEPTION_START,
} T_GUI_BST_RECEPTION_TYPE;

void app_panel_le_msg_handle(T_APP_GUI_MSG *data);

#ifdef __cplusplus
}
#endif

#endif
