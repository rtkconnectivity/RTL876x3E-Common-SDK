/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_USB_H__
#define __APP_USB_H__
#include <stdbool.h>
#include "app_io_msg.h"

/** @defgroup USB_DEMO_CTRL USB Control Demo
  * @brief Main entry function for audio sample application.
  * @{
  */

void app_usb_msg_handle(T_IO_MSG *msg);
bool app_usb_connected(void);
void app_usb_init(void);

/** @} */ /* End of group USB_DEMO_CTRL */

#endif
