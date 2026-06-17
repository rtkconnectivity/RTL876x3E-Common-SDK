/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_SCAN_H_
#define _APP_LEA_CAP_ACC_SCAN_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

typedef struct
{
    uint8_t bd_addr[GAP_BD_ADDR_LEN];
    uint8_t bd_type;
    uint8_t adv_sid;
    uint8_t broadcast_id[3];
} T_APP_LEA_BAAS_DEV_INFO;

typedef struct t_lea_baas_scan_dev
{
    struct t_lea_baas_scan_dev *p_next;
    T_APP_LEA_BAAS_DEV_INFO dev_info;
} T_APP_LEA_BAAS_SCAN_DEV;


void app_lea_baas_scan_start(void);
void app_lea_baas_scan_stop(void);


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
