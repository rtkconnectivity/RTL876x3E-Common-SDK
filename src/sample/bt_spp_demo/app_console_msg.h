/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CONSOLE_MSG_H_
#define _APP_CONSOLE_MSG_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void app_console_handle_msg(T_IO_MSG console_msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_CONSOLE_MSG_H_ */
