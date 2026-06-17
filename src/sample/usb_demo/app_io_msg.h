/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_IO_MSG_H_
#define _APP_IO_MSG_H_

#include <stdbool.h>
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup USB_APP_IO_MSG App IO Msg
  * @brief App IO Msg
  * @{
  */
bool app_io_msg_send(T_IO_MSG *io_msg);

/**
    * @brief  All the application events are pre-handled in this function.
    *         All the io messages are sent to this function.
    *         Then the event handling function shall be called according to the message type.
    * @param  io_driver_msg_recv The T_IO_MSG from peripherals or BT stack state machine.
    * @return void
    */
void app_io_handle_msg(T_IO_MSG io_driver_msg_recv);

/** End of USB_APP_IO_MSG
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_IO_MSG_H_ */
