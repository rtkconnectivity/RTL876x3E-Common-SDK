/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_PANEL_MSG__
#define __APP_PANEL_MSG__



#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>
#include "stdbool.h"
#include "stdint.h"
typedef enum
{
    EVENT_GUI_TO_DEVICE,
    EVENT_GUI_TO_BREDR,
    EVENT_GUI_TO_BLE,
} T_GUI_EVENT_TYPE;

typedef enum
{
    GUI_DEVICE_SUBEVENT_STOP_SD_MUSIC,
    GUI_DEVICE_SUBEVENT_PLAY_SD_MUSIC,
    GUI_DEVICE_SUBEVENT_MAX,
} T_GUI_DEVICE_SUBEVENT_TYPE;

typedef enum
{
    GUI_BREDR_SUBEVENT_MAX,
} T_GUI_BREDR_SUBEVENT_TYPE;

typedef enum
{
    GUI_LE_SUBEVENT_ADJUST_VOLUME,
    GUI_LE_SUBEVENT_BASS_SCAN,
    GUI_LE_SUBEVENT_PA_SYNC,
    GUI_LE_SUBEVENT_BST_RECEPTION,
    GUI_LE_SUBEVENT_MAX,
} T_GUI_BLE_SUBEVENT_TYPE;

typedef struct
{
    uint16_t type;
    uint16_t subtype;
    union
    {
        uint32_t  param;
        void     *buf;
    } u;
} T_APP_GUI_MSG;

bool app_panel_msg_channel_register(void *evt_queue, void *io_queue, uint16_t msg_queue_elem_num);
void app_panel_msg_handle(uint8_t event);


#ifdef __cplusplus
}
#endif

#endif
