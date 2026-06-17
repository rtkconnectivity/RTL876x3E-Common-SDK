/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _MSC_H_
#define _MSC_H_
#include <stdint.h>
#include <stdbool.h>
#include "usb_ms_driver.h"

/** @defgroup USB_MSC USB MSC
  * @brief USB msc data pipe usage
  * @{
  */

#define     MSC_BULK_IN_EP          0x82
#define     MSC_BULK_OUT_EP         0x02

/**
 * usb_msc.h
 *
 * \brief   usb msc init
 *
 * \ingroup USB_MSC
 */
void usb_msc_init(void);

/**
 * usb_msc.h
 *
 * \brief   usb msc deinit
 *
 * \ingroup USB_MSC
 */
void usb_msc_deinit(void);

/** @}*/
/** End of USB_MSC
*/

#endif
