/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_MCP_H_
#define _APP_LEA_CAP_ACC_MCP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "cap.h"
#include "mcp_client.h"
#include "app_lea_cap_acc_content.h"

#if MCP_MEDIA_CONTROL_CLIENT
void app_lea_acc_mcp_cp(uint8_t ccid, T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM *p_param);
void app_lea_acc_mcp_init_cap(T_CAP_INIT_PARAMS *p_param);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
