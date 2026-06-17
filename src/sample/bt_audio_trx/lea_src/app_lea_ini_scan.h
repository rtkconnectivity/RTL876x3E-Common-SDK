/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_INI_SCAN_H_
#define _APP_LEA_INI_SCAN_H_

#include <stdbool.h>
#include "ble_audio_group.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define APP_LEA_ADV_DATA_ASCS_BIT   0x01
#define APP_LEA_ADV_DATA_BASS_BIT   0x02
#define APP_LEA_ADV_DATA_CAP_BIT    0x04
#define APP_LEA_ADV_DATA_RSI_BIT    0x08

typedef enum
{
    APP_LEA_SCAN_START,
    APP_LEA_SCAN_STOP,
} T_APP_LEA_SCAN_STATE;
typedef struct
{
    uint8_t  adv_data_flags;
    uint8_t  ascs_announcement_type;
    uint16_t ascs_sink_available_contexts;
    uint16_t ascs_source_available_contexts;
    uint8_t  cap_announcement_type;
    uint8_t  dev_name_len;
    uint8_t  dev_name[GAP_DEVICE_NAME_LEN];
} T_APP_LEA_ADV_DATA;

typedef struct t_lea_scan_dev
{
    struct t_lea_scan_dev *p_next;
    uint8_t bd_addr[GAP_BD_ADDR_LEN];
    uint8_t bd_type;
    T_APP_LEA_ADV_DATA adv_data;
} T_APP_LEA_SCAN_DEV;

#if BAP_BROADCAST_ASSISTANT
typedef struct
{
    uint8_t bd_addr[GAP_BD_ADDR_LEN];
    uint8_t bd_type;
    uint8_t adv_sid;
    uint8_t broadcast_id[3];
    uint8_t dev_name_len;
    uint8_t dev_name[GAP_DEVICE_NAME_LEN];
} T_APP_LEA_BAAS_DEV_INFO;

typedef struct t_lea_baas_scan_dev
{
    struct t_lea_baas_scan_dev *p_next;
    T_APP_LEA_BAAS_DEV_INFO dev_info;
} T_APP_LEA_BAAS_SCAN_DEV;
#endif

void app_lea_ini_scan_start(void);
void app_lea_ini_scan_stop(void);
bool app_lea_group_scan_start(T_BLE_AUDIO_GROUP_HANDLE group_handle);
bool app_lea_group_scan_stop(void);

#if BAP_BROADCAST_ASSISTANT
void app_lea_baas_scan_start(void);
void app_lea_baas_scan_stop(void);
void app_lea_clear_baas_scan_dev(void);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_LEA_INI_SCAN_H_ */
