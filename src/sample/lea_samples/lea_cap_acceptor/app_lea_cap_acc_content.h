/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_CONTENT_H_
#define _APP_LEA_CAP_ACC_CONTENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "app_lea_cap_acc_link.h"

#if (CCP_CALL_CONTROL_CLIENT || MCP_MEDIA_CONTROL_CLIENT)

typedef enum
{
    APP_LEA_CONTENT_TYPE_MCS        = 0x00,
    APP_LEA_CONTENT_TYPE_TBS        = 0x01,
} T_APP_LEA_CONTENT_TYPE;

#if CCP_CALL_CONTROL_CLIENT
typedef struct t_lea_call_index
{
    struct t_lea_call_index *p_next;
    uint8_t call_index;
    uint8_t call_state;
    uint8_t call_flags;
    uint16_t call_uri_len;
    uint8_t *call_uri;
} T_LEA_CALL_INDEX;

typedef struct
{
    T_OS_QUEUE call_idx_queue;
} T_APP_LEA_CONTENT_CCP;
#endif

#if MCP_MEDIA_CONTROL_CLIENT
typedef struct
{
    uint8_t media_state;
} T_APP_LEA_CONTENT_MCP;
#endif

typedef struct t_app_lea_content_db
{
    struct t_app_lea_content_db *p_next;
    uint8_t                ccid;
    bool                   general;
    bool                   active;
    uint8_t                srv_instance_id;
    T_APP_LEA_CONTENT_TYPE type;
    union
    {
#if MCP_MEDIA_CONTROL_CLIENT
        T_APP_LEA_CONTENT_MCP content_mcp;
#endif
#if CCP_CALL_CONTROL_CLIENT
        T_APP_LEA_CONTENT_CCP content_ccp;
#endif
    } param;
} T_APP_LEA_CONTENT_DB;

void app_lea_media_state_print(uint8_t media_state);
void app_lea_call_state_print(uint8_t call_state);
void app_lea_context_print(void);

T_APP_LEA_CONTENT_DB *app_lea_content_add(uint16_t conn_handle, T_APP_LEA_CONTENT_TYPE type,
                                          uint8_t srv_instance_id, bool general);
T_APP_LEA_CONTENT_DB *app_lea_content_find_by_ccid(T_APP_LE_LINK *p_link, uint8_t ccid);
T_APP_LEA_CONTENT_DB *app_lea_content_find(uint16_t conn_handle, T_APP_LEA_CONTENT_TYPE type,
                                           uint8_t srv_instance_id, bool general);
void app_lea_content_free(T_APP_LE_LINK *p_link);

#if CCP_CALL_CONTROL_CLIENT
T_LEA_CALL_INDEX *app_lea_call_idx_add(T_APP_LEA_CONTENT_DB *p_content, uint8_t call_index,
                                       uint8_t call_state, uint8_t call_flags);
T_LEA_CALL_INDEX *app_lea_call_idx_find(T_APP_LEA_CONTENT_DB *p_content, uint8_t call_index);
void app_lea_call_idx_del(T_APP_LEA_CONTENT_DB *p_content, uint8_t call_index);
#endif
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
