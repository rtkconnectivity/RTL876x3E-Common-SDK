/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_PAN_TEST_SOCKETS_H_
#define _APP_PAN_TEST_SOCKETS_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum APP_BT_PAN_SOCKETS_MODE
{
    APP_BT_PAN_SOCKETS_CLIENT = 0,
    APP_BT_PAN_SOCKETS_SERVER = 1,
    APP_BT_PAN_SOCKETS_INVALID = 0xff,
} APP_BT_PAN_SOCKETS_MODE_T;

bool app_bt_pan_sockets(void);

void app_bt_pan_sockets_set_ip_str(char *ip_str, uint32_t len);

char *app_bt_pan_sockets_get_ip_str(void);

void app_bt_pan_sockets_set_port(uint16_t port);

uint16_t app_bt_pan_sockets_get_port(void);

void app_bt_pan_sockets_set_mode(char *mode_str);

APP_BT_PAN_SOCKETS_MODE_T app_bt_pan_sockets_get_mode(void);
#endif

