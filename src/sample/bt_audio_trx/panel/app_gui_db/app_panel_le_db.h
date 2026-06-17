/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_PANEL_LE_DB__
#define __APP_PANEL_LE_DB__



#ifdef __cplusplus
extern "C" {
#endif
#include "string.h"
#include "app_main.h"
#include "remote.h"
#include "app_panel_db_common.h"

typedef enum
{
    BLE_LINK_DISCONNECT,
    BLE_LINK_CONNECT,
} T_APP_GUI_LE_LINK_STATUS;

typedef enum
{
    BUD_LEFT_SIDE,
    BUD_RIGHT_SIDE,
} T_APP_GUI_LE_BUD_SIDE;

typedef struct app_gui_le_link_data_t
{
    bool active;
    uint8_t bd_addr[6];
    T_APP_GUI_LE_BUD_SIDE bud_side;
    uint8_t battery_level[2]; // first byte: left side battery   second byte:  right side battery
    uint8_t volume;
    T_APP_GUI_LISTENING_MODE    anc_status;
    uint8_t remote_device_name[40];
    T_APP_GUI_LE_LINK_STATUS remote_device_link_status;
    uint8_t res;
} T_APP_GUI_LE_LINK_DATA;

typedef struct app_gui_le_data_t
{
    T_APP_GUI_LE_LINK_DATA gui_le_link_data[MAX_BLE_LINK_NUM];
    uint8_t dev_list_num;
} T_APP_GUI_LE_DATA;

typedef struct app_gui_le_bass_scan_rsp_t
{
    uint8_t index;
    uint8_t *device_name;
} T_APP_GUI_LE_BASS_SCAN_RSP_DATA;

T_APP_GUI_LE_LINK_DATA *app_panel_le_find_active_le_link(void);
T_APP_GUI_LE_LINK_DATA *app_panel_le_find_le_link(uint8_t *bd_addr);
void app_panel_le_alloc_link(uint8_t *bd_addr);
void app_panel_le_free_link(uint8_t *bd_addr);
void app_panel_le_check_awake_gui(void);
void app_panel_le_db_set_bettery_level(uint8_t *bd_addr, uint8_t *level);
bool app_panel_le_db_get_bettery_level(uint8_t *level);
void app_panel_le_db_set_volume(uint8_t *bd_addr, uint8_t volume_value);
void app_panel_le_db_set_anc_status(uint8_t *bd_addr, uint8_t anc_status);
void app_panel_le_db_set_bud_side(uint8_t *bd_addr, T_APP_GUI_LE_BUD_SIDE side);
void app_panel_le_db_set_remote_name(uint8_t *bd_addr, uint8_t *name, uint8_t length);
void app_panel_le_db_set_remote_link_status(uint8_t *bd_addr, uint8_t status);
void app_panel_le_db_add_bass_scan_dev(T_APP_GUI_LE_BASS_SCAN_RSP_DATA *scan_info);
#ifdef __cplusplus
}
#endif

#endif
