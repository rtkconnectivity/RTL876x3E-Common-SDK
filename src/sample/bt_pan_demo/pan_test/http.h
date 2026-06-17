/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_PAN_TEST_HTTP_H_
#define _APP_PAN_TEST_HTTP_H_

#include <stdbool.h>
#include <stdint.h>

bool app_bt_pan_demo_http(void);


void app_bt_pan_set_http(char *ip_str, uint32_t ip_str_len, uint16_t port, char *uri,
                         uint32_t uri_len);

#endif


