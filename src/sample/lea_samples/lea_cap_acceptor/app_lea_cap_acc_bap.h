/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_BAP_H_
#define _APP_LEA_CAP_ACC_BAP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio_sync.h"
#include "ble_audio.h"
#include "app_lea_cap_acc_scan.h"

#if (BAP_BROADCAST_SINK || BAP_SCAN_DELEGATOR)
#define APP_LEA_BIG_PROC_BIG_SYNC_REQ         0x01
#define APP_LEA_BIG_PROC_BIG_INFO_RECEIVED    0x02
#define APP_LEA_BIG_PROC_BROADCAST_CODE_EXIST 0x04

#define APP_BIG_SYNC_MAX_BIS_NUM              2

typedef struct t_lea_sync_info
{
    struct t_lea_sync_info            *p_next;
    T_BLE_AUDIO_SYNC_HANDLE            sync_handle;

    uint8_t                            source_id;
    T_GAP_PA_SYNC_STATE                pa_state;
    T_GAP_BIG_SYNC_RECEIVER_SYNC_STATE big_state;
#if BAP_BROADCAST_SINK
    uint32_t                           supported_bis_array;
    uint8_t                            big_proc_flags;
    uint8_t                            encryption;
    uint8_t                            broadcast_code[16];
    uint8_t                            bis_num;
    uint8_t                            bis_idx[APP_BIG_SYNC_MAX_BIS_NUM];
#endif
} T_APP_LEA_SYNC_INFO;
#endif

void app_lea_acc_bap_init(void);

void app_lea_acc_bap_update_available_contexts(uint16_t lea_sink_available_contexts,
                                               uint16_t lea_source_available_contexts);

#if (BAP_BROADCAST_SINK || BAP_SCAN_DELEGATOR)
T_APP_LEA_SYNC_INFO *app_lea_add_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle);
T_APP_LEA_SYNC_INFO *app_lea_find_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle);
void app_lea_remove_sync(T_BLE_AUDIO_SYNC_HANDLE sync_handle);
bool app_lea_big_terminate(T_APP_LEA_SYNC_INFO *p_sync_info);
bool app_lea_pa_terminate(T_APP_LEA_SYNC_INFO *p_sync_info);
bool app_lea_pa_sync(T_APP_LEA_SYNC_INFO *p_sync_info);
bool app_lea_pa_sync_by_dev_info(T_APP_LEA_BAAS_DEV_INFO *p_dev_info);
void app_lea_big_sync(T_APP_LEA_SYNC_INFO *p_sync_info);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
