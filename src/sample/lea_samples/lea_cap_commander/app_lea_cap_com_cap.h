/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_COM_CAP_H_
#define _APP_LEA_CAP_COM_CAP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "ble_audio_group.h"

typedef struct t_lea_group_info
{
    struct t_lea_group_info *p_next;
    T_BLE_AUDIO_GROUP_HANDLE group_handle;
    uint8_t set_mem_size;
} T_APP_LEA_GROUP_INFO;

void app_lea_com_cap_init(void);
void app_lea_com_group_cb(T_AUDIO_GROUP_MSG msg, T_BLE_AUDIO_GROUP_HANDLE handle,
                          void *buf);

T_APP_LEA_GROUP_INFO *app_lea_add_group(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                        uint8_t set_mem_size);
T_APP_LEA_GROUP_INFO *app_lea_find_group(T_BLE_AUDIO_GROUP_HANDLE group_handle);
T_APP_LEA_GROUP_INFO *app_lea_find_group_by_conn_handle(uint16_t conn_handle);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
