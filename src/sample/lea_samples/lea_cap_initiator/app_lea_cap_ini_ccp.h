/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_INI_CCP_H_
#define _APP_LEA_CAP_INI_CCP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "cap.h"
#include "ccp_mgr.h"

#if CCP_CALL_CONTROL_SERVER

#define APP_LEA_GTBS_CCID                            0x01

#define APP_LEA_TBS_LOCAL_HOLD_SUPPORT               1
#define APP_LEA_TBS_CALL_LIST_MAX                    2

typedef struct
{
    uint16_t    call_uri_len;
    uint8_t    *p_call_uri;
} T_APP_TBS_CALL_URI;

typedef struct
{
    uint8_t             call_index;
    uint8_t             call_flags;
    T_TBS_CALL_STATE    call_state;
} T_APP_TBS_CALL_INFO;

typedef union
{
    T_APP_TBS_CALL_INFO call_info;
    T_CCP_SERVER_TERMINATION_REASON call_terminate;
} T_APP_TBS_INPUT_MSG_DATA;

typedef enum
{
    APP_TBS_INPUT_MSG_CALL_INFO,
    APP_TBS_INPUT_MSG_CALL_TERMINATE,
} T_APP_TBS_INPUT_MSG;

/*Client action*/
typedef union
{
    uint8_t call_index;
    T_CCP_SERVER_TERMINATION_REASON remote_terminate;
    T_CCP_SERVER_TERMINATION_REASON local_terminate;
    T_APP_TBS_CALL_URI remote_incoming_call;
    T_APP_TBS_CALL_URI local_originate;
} T_APP_TBS_OUTPUT_MSG_DATA;

typedef enum
{
    /*Remote party action*/
    APP_TBS_OUTPUT_MSG_REMOTE_HOLD = 0x00,
    APP_TBS_OUTPUT_MSG_REMOTE_RETRIEVE = 0x01,
    APP_TBS_OUTPUT_MSG_REMOTE_TERMINATE = 0x02,
    APP_TBS_OUTPUT_MSG_REMOTE_ALERT_START = 0x03,   /*Outing call action*/
    APP_TBS_OUTPUT_MSG_REMOTE_ANSWER = 0x04,        /*Outing call action*/
    APP_TBS_OUTPUT_MSG_REMOTE_INCOMING_CALL = 0x05, /*Incoming call action*/

    /*Server action*/
    APP_TBS_OUTPUT_MSG_LOCAL_HOLD = 0x10,
    APP_TBS_OUTPUT_MSG_LOCAL_RETRIEVE = 0x11,
    APP_TBS_OUTPUT_MSG_LOCAL_TERMINATE = 0x12,
    APP_TBS_OUTPUT_MSG_LOCAL_ORIGINATE = 0x13,     /*Outing call action*/
    APP_TBS_OUTPUT_MSG_LOCAL_ACCEPT = 0x14,        /*Incoming call action*/
} T_APP_TBS_OUTPUT_MSG;

typedef struct
{
    uint8_t             call_index;
    uint8_t             call_flags;
    T_TBS_CALL_STATE    call_state;
} T_TBS_CALL_DB;

typedef struct
{
    uint8_t             gtbs_id;
    uint16_t            ccp_enabled_cccd;

    uint8_t             call_num;
    T_TBS_CALL_DB       call_list[APP_LEA_TBS_CALL_LIST_MAX];
} T_APP_LEA_CCP_DB;

void app_lea_ini_ccp_control(T_APP_TBS_OUTPUT_MSG msg, T_APP_TBS_OUTPUT_MSG_DATA *p_data);
void app_lea_ini_ccp_init_cap(T_CAP_INIT_PARAMS *p_param);
void app_lea_ini_ccp_init(void);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
