/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_INI_BAP_H_
#define _APP_LEA_CAP_INI_BAP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "ble_audio_group.h"

#define APP_ISOC_CIG_MAX_NUM                      2
#define APP_ISOC_CIS_MAX_NUM                      4
#define APP_MAX_PA_ADV_SET_NUM                    1
#define APP_ISOC_BROADCASTER_MAX_BIG_HANDLE_NUM   1
#define APP_ISOC_BROADCASTER_MAX_BIS_NUM          4

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

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
