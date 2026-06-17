/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _CMD_BR_H_
#define _CMD_BR_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void br_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                   uint8_t *ack_pkt);

extern uint32_t profiles_to_connect;

#ifdef __cplusplus
}
#endif /* __cplus */
#endif

