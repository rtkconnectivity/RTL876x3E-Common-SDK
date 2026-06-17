/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_DEV_DRIVER_H__
#define __USB_DEV_DRIVER_H__
#include <stdint.h>
#include <stdbool.h>
#include "usb_spec20.h"

/** \addtogroup USB_DEV_Driver
  * \brief USB device driver.
  * @{
  */

/**
 * \defgroup USB_Dev_Driver_Usage_Chapter_Vendor How to Use Vendor Callback
 * @{
 *
 * \details The vendor callback will notify the information to the upper application.
 *
 * \par Example
 * \code
 *      #include "usb_dev_driver.h"
 *
 *      int demo_get_vendor(uint16_t request_cmd, void *buf, uint16_t len)
 *      {
 *          ...
 *      }
 *
 *      int demo_set_vendor(uint16_t request_cmd, void *buf, uint16_t len)
 *      {
 *          ...
 *      }
 *
 *      T_USB_DEV_VENDOR_CBS demo_cbs =
 *      {
 *          .get = demo_get_vendor,
 *          .set = demo_set_vendor,
 *      };
 *      usb_dev_driver_vendor_cbs_register(&demo_cbs);

 *\endcode
 * @}
 */

/** @defgroup USB_DEV_Driver_Exported_Types USB Device Driver Exported Types
  * @{
  */

/**
 * \brief   USB device-related strings.
 *
 * \param id The string ID defined in the device descriptor.
 * \param s  The actual string that matches with \ref id.
 *
 */
typedef struct _string
{
    uint8_t          id;
    const char      *s;
} T_STRING;

/**
 * \brief   USB device-related string table.
 *
 * \param language The language id used by strings.
 * \param strings  The strings refer to \ref T_STRING.
 *
 */
typedef struct _string_tab
{
    uint16_t language;
    T_STRING *strings;
} T_STRING_TAB;

/**
 *
 * \brief   USB device vendor callback.
 *
 */
typedef int (*INT_VENDOR_FUNC)(T_USB_DEVICE_REQUEST *ctrl_request, void *buf);

/**
 *
 * \brief   USB composite device callback to support CFU.
 * \param get: The api will be called when receiving a get_vendor request.
 * \param set: The api will be called when receiving a set_vendor request.
 *
 */
typedef struct _usb_dev_vendor_cbs
{
    INT_VENDOR_FUNC   get;
    INT_VENDOR_FUNC   set;
} T_USB_DEV_VENDOR_CBS;

/** End of group USB_DEV_Driver_Exported_Types
  * @}
  */

/** @defgroup USB_DEV_Driver_Exported_Functions USB Device Driver Exported Functions
  * @{
  */

/**
 * \brief  Register the USB device descriptor.
 *
 * \param desc The device descriptor.
 *
 */
void usb_dev_driver_dev_desc_register(T_USB_DEVICE_DESC *desc);

/**
 * \brief  Register the USB string descriptor.
 *
 * \param string_tbl The string descriptor table.
 *
 */
void usb_dev_driver_string_desc_register(T_STRING_TAB *string_tbl[]);

/**
 * \brief  Register the USB configuration descriptor.
 *
 * \param desc The configuration descriptor.
 *
 */
void usb_dev_driver_cfg_desc_register(T_USB_CONFIG_DESC *desc);


/**
 * \brief  Unregister the USB device descriptor.
 *
 */
void usb_dev_driver_dev_desc_unregister(void);

/**
 * \brief  Unregister the USB string descriptor.
 * \param string_tbl The string descriptor table.
 *
 */
void usb_dev_driver_string_desc_unregister(T_STRING_TAB *string_tbl[]);

/**
 * \brief  Unregister the USB configuration descriptor.
 * \param desc The configuration descriptor.
 *
 */
void usb_dev_driver_cfg_desc_unregister(T_USB_CONFIG_DESC *desc);

/**
 * \brief   Register USB device driver vendor callbacks.
 *
 * \param cbs USB device driver vendor callbacks, refer to \ref T_USB_DEV_VENDOR_CBS.
 * \par Example
 * Please refer to \ref USB_dev_driver_Usage_Chapter_Vendor.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 */

void usb_dev_driver_vendor_cbs_register(T_USB_DEV_VENDOR_CBS *cbs);

/**
 * \brief  Unregister USB device driver vendor callbacks.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 */

void usb_dev_driver_vendor_cbs_unregister(void);

/**
 * \brief  Initialize the buffer length of the control transfer.
 *
 * \param buf_len The buffer length of the control transfer.
 *
 * \note The default length of the control transfer buffer is 512. If a larger buffer is required,
 *          call this API during the initialization phase to increase the buffer size.
 *          This api must be called before \ref usb_dev_driver_dev_desc_register.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 */
void usb_dev_driver_ctrl_xfer_buf_init(uint16_t buf_len);

/** @} */ /* End of group USB_DEV_Driver_Exported_Functions */
/** @}*/
/** End of USB_DEV_Driver
*/
#endif
