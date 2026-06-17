/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_PAN_TEST_PING_H_
#define _APP_PAN_TEST_PING_H_

#include <stdbool.h>
#include <stdint.h>

bool app_bt_pan_ping(void);

void app_bt_pan_set_ping(char *server_name, uint32_t server_name_len);
#endif
