/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_CCP_H_
#define _APP_LEA_CAP_ACC_CCP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "cap.h"
#include "app_lea_cap_acc_link.h"

#if CCP_CALL_CONTROL_CLIENT
typedef struct
{
    uint16_t conn_handle;
    uint8_t  ccid;
    uint8_t  call_index;
} T_APP_LEA_CCP_CALL_DATA;

typedef enum
{
    APP_LEA_CCP_CALL_REJECT = 0x00, /*Incoming call action*/
    APP_LEA_CCP_CALL_ANSWER = 0x01, /*Incoming call action*/
    APP_LEA_CCP_CALL_TERMINATE = 0x02,
    APP_LEA_CCP_CALL_LOCAL_HOLD = 0x03,
    APP_LEA_CCP_CALL_LOCAL_RETRIEVE = 0x04,
} T_APP_LEA_CCP_CALL_ACTION;

void app_lea_acc_ccp_control(T_APP_LEA_CCP_CALL_ACTION action, T_APP_LEA_CCP_CALL_DATA *p_data);
void app_lea_acc_ccp_init_cap(T_CAP_INIT_PARAMS *p_param);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
