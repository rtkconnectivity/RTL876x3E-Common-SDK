/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_INI_BAP_H_
#define _APP_LEA_INI_BAP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "ble_audio_group.h"
#include "broadcast_source_sm.h"
#if BAP_BROADCAST_ASSISTANT
#include "app_lea_ini_scan.h"
#include "ble_audio_sync.h"
#endif

#define APP_ISOC_CIG_MAX_NUM                      2
#define APP_ISOC_CIS_MAX_NUM                      4
#define APP_MAX_PA_ADV_SET_NUM                    1
#define APP_ISOC_BROADCASTER_MAX_BIG_HANDLE_NUM   1
#define APP_ISOC_BROADCASTER_MAX_BIS_NUM          4

#if BAP_BROADCAST_ASSISTANT
typedef struct t_lea_sync_info
{
    struct t_lea_sync_info            *p_next;
    T_BLE_AUDIO_SYNC_HANDLE            sync_handle;
    T_APP_LEA_BAAS_DEV_INFO            dev_info;
    T_GAP_PA_SYNC_STATE                pa_state;
} T_APP_LEA_SYNC_INFO;
#endif

void app_lea_ini_bap_init(void);

#if (BAP_BROADCAST_ASSISTANT || BAP_BROADCAST_SOURCE)
void app_lea_ini_bst_reception_start(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                     T_BROADCAST_SOURCE_HANDLE source_handle);
void app_lea_ini_bst_reception_remove(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                      T_BROADCAST_SOURCE_HANDLE source_handle);
void app_lea_ini_bst_reception_stop(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                    T_BROADCAST_SOURCE_HANDLE source_handle);
void app_lea_link_brs_char_add_source_handle(void);
#endif

#if BAP_BROADCAST_ASSISTANT
T_APP_LEA_SYNC_INFO *app_lea_add_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle,
                                             T_APP_LEA_BAAS_DEV_INFO *p_dev_info);
T_APP_LEA_SYNC_INFO *app_lea_find_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle);
T_APP_LEA_SYNC_INFO *app_lea_find_sync_handle_by_addr(uint8_t *bd_addr, uint8_t bd_type,
                                                      uint8_t adv_sid, uint8_t broadcast_id[3]);
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
