/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_USB_H__
#define __APP_USB_H__
#include <stdbool.h>
#include "app_io_msg.h"
#include "usb_dm.h"

#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
#define     CPU_FREQ_MHZ_MAX    160
#else
#define     CPU_FREQ_MHZ_MAX    100
#endif

/** @defgroup APP_USB APP USB
  * @brief app usb module.
  * @{
  */

/**
 * app_usb.h
 *
 * \brief   handle USB-related message.
 *
 * \ingroup APP_USB
 */
void app_usb_msg_handle(T_IO_MSG *msg);

/**
 * app_usb.h
 *
 * \brief   Judge if USB is connected
 *
 * \details usb_dm_start will start USB task. USB connection state depends on if USB task is already started
 *
 * \ingroup APP_USB
 */
bool app_usb_connected(void);

/**
 * app_usb.h
 *
 * \brief   get power state
 * \return usb power state
 *
 * \ingroup APP_USB
 */
T_USB_POWER_STATE app_usb_power_state(void);

/**
 * app_usb.h
 *
 * \brief   app usb start
 *
 * \ingroup APP_USB
 */
void app_usb_start(void);

/**
 * app_usb.h
 *
 * \brief   app usb stop
 *
 * \ingroup APP_USB
 */
void app_usb_stop(void);

/**
 * app_usb.h
 *
 * \brief   app usb init
 *
 * \ingroup APP_USB
 */
void app_usb_init(void);

/** @}*/
/** End of APP_USB
*/

#endif
