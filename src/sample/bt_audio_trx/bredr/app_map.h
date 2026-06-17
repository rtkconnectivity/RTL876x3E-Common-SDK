/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_MAP_H_
#define _APP_MAP_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_cmd.h"


void app_map_init(void);

void app_map_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                        uint16_t cmd_len, uint8_t *ack_pkt);





#endif







