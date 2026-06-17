/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_INI_SCAN_H_
#define _APP_LEA_CAP_INI_SCAN_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio_group.h"

typedef struct
{
    uint8_t  adv_data_flags;
    uint8_t  ascs_announcement_type;
    uint16_t ascs_sink_available_contexts;
    uint16_t ascs_source_available_contexts;
    uint8_t  cap_announcement_type;
} T_APP_LEA_ADV_DATA;

typedef struct t_lea_scan_dev
{
    struct t_lea_scan_dev *p_next;
    uint8_t bd_addr[GAP_BD_ADDR_LEN];
    uint8_t bd_type;
    T_APP_LEA_ADV_DATA adv_data;
} T_APP_LEA_SCAN_DEV;


void app_lea_scan_start(void);
void app_lea_scan_stop(void);
void app_lea_group_scan_start(T_BLE_AUDIO_GROUP_HANDLE group_handle);
void app_lea_group_scan_stop(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
