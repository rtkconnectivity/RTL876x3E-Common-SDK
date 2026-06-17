/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_IO_MSG_H_
#define _APP_IO_MSG_H_

#include <stdint.h>
#include <stdbool.h>

#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


bool app_io_send_msg(T_IO_MSG *io_msg);

void app_io_handle_msg(T_IO_MSG io_driver_msg_recv);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_IO_MSG_H_ */
