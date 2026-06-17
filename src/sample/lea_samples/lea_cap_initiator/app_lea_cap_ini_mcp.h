/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_INI_MCP_H_
#define _APP_LEA_CAP_INI_MCP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "cap.h"
#include "mcp_mgr.h"
#include "mcp_def.h"

#if MCP_MEDIA_CONTROL_SERVER
#define APP_LEA_GMCS_CCID  0x20

typedef struct
{
    uint8_t       gmcs_id;
    uint8_t       media_state;
    uint16_t      mcp_enabled_cccd;
    int32_t       track_duration;
    int32_t       track_position;
} T_APP_LEA_MCP_DB;

void app_lea_ini_mcp_update_media_state(T_MCS_MEDIA_STATE state);
void app_lea_ini_mcp_init_cap(T_CAP_INIT_PARAMS *p_param);
void app_lea_ini_mcp_init(void);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
