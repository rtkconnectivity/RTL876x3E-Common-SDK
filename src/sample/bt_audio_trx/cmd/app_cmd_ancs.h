/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CMD_ANCS_H__
#define _APP_CMD_ANCS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <app_report.h>



#define ANCS_ATTR_IDS_MAX_LEN   14


void ancs_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr, uint8_t *ack_pkt);

#ifdef __cplusplus
}
#endif

#endif

