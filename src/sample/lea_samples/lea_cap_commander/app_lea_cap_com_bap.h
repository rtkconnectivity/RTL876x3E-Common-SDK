/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_COM_BAP_H_
#define _APP_LEA_CAP_COM_BAP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio_sync.h"
#include "ble_audio.h"
#include "app_lea_cap_com_scan.h"

#if BAP_BROADCAST_ASSISTANT

typedef struct t_lea_sync_info
{
    struct t_lea_sync_info            *p_next;
    T_BLE_AUDIO_SYNC_HANDLE            sync_handle;
    T_APP_LEA_BAAS_DEV_INFO            dev_info;
    T_GAP_PA_SYNC_STATE                pa_state;
} T_APP_LEA_SYNC_INFO;
#endif

void app_lea_com_bap_init(void);

#if BAP_BROADCAST_ASSISTANT
T_APP_LEA_SYNC_INFO *app_lea_add_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle,
                                             T_APP_LEA_BAAS_DEV_INFO *p_dev_info);
T_APP_LEA_SYNC_INFO *app_lea_find_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle);
T_APP_LEA_SYNC_INFO *app_lea_find_sync_handle_by_addr(uint8_t *bd_addr, uint8_t bd_type,
                                                      uint8_t adv_sid, uint8_t broadcast_id[3]);
void app_lea_remove_sync(T_BLE_AUDIO_SYNC_HANDLE sync_handle);
bool app_lea_pa_terminate(T_APP_LEA_SYNC_INFO *p_sync_info);
bool app_lea_pa_sync(T_APP_LEA_SYNC_INFO *p_sync_info);
bool app_lea_pa_sync_by_dev_info(T_APP_LEA_BAAS_DEV_INFO *p_dev_info);

void app_lea_com_bst_reception_start(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                     T_BLE_AUDIO_SYNC_HANDLE sync_handle);
void app_lea_com_bst_reception_remove(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                      T_BLE_AUDIO_SYNC_HANDLE sync_handle);
void app_lea_com_bst_reception_stop(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                    T_BLE_AUDIO_SYNC_HANDLE sync_handle);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
