/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_COMPOSITE_DEV_H__
#define __USB_COMPOSITE_DEV_H__

#include <stdint.h>
#include "os_queue.h"
#include "hal_usb.h"
#include "usb_spec20.h"
#include "usb_utils.h"
/**
 * \addtogroup USB_Composite
 * \brief USB composite device consists of multiple configurations, and each configuration consists of multiple interfaces.
 * This module provides fundamental components for implementing USB composite devices, which include multiple USB
 * classes such as USB audio and HID.
 * @{
 */

/**
 * \defgroup USB_Composite_Usage_Chapter_Device How to Setup USB Device
 * @{
 *
 * \brief This group provides a comprehensive guide on setting up a USB device,
 *        complete with sample code for your reference.
 *
 * \section USB_Composite_Usage_Chapter_Device Initialize the USB Device and Configuration
 *
 * \par Example
 * \code
 *      T_USB_DEVICE_DESC demo_usb_dev_desc =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_CONFIG_DESC demo_usb_cfg_desc =
 *      {
 *          ...init...
 *      };
 *
 *      usb_composite_dev_init(&demo_usb_dev_desc);
 *      usb_composite_dev_cfg_add(&demo_usb_cfg_desc);
 *
 *      //if string index of demo_usb_dev_desc/demo_usb_cfg_desc is not zero
 *      usb_composite_dev_string_add(language, string index, string);
 * \endcode
 *
 * \section USB_Composite_Usage_Chapter_Interface Implement USB Interfaces
 *
 * \details The core interface information is defined in \ref T_USB_INTERFACE. It is mainly used to initialize
 *          \ref T_USB_INTERFACE to implement an interface.
 *
 * \par Example
 * \code
 *      #include "usb_spec20.h"
 *      T_USB_INTERFACE_DESC std_if_desc_hs =
 *      {
 *          ...init...
 *      };
 *
 *      (Class interface descriptor type) class_if_desc_hs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_ENDPOINT_DESC std_endpoint_desc_hs =
 *      {
 *          ...init...
 *      };
 *
 *      (Class endpoint descriptor type)  class_endpoint_desc_hs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_INTERFACE_DESC std_if_desc_fs =
 *      {
 *          ...init...
 *      };
 *
 *      (Class interface descriptor type) class_if_desc_fs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_ENDPOINT_DESC std_endpoint_desc_fs =
 *      {
 *          ...init...
 *      };
 *
 *      (Class endpoint descriptor type)  class_endpoint_desc_fs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_DESC_HDR* demo_if_descs_hs[] =
 *      {
 *          (T_USB_DESC_HDR*)&std_if_desc_hs,
 *          (T_USB_DESC_HDR*)&class_if_desc_hs,
 *          (T_USB_DESC_HDR*)&std_endpoint_desc_hs,
 *          (T_USB_DESC_HDR*)&class_endpoint_desc_hs,
 *          NULL,
 *      };
 *
 *      T_USB_DESC_HDR* demo_if_descs_fs[] =
 *      {
 *          (T_USB_DESC_HDR*)&std_if_desc_fs,
 *          (T_USB_DESC_HDR*)&class_if_desc_fs,
 *          (T_USB_DESC_HDR*)&std_endpoint_desc_fs,
 *          (T_USB_DESC_HDR*)&class_endpoint_desc_fs,
 *          NULL,
 *      };
 *
 *      int demo_if_ctrl_request_proc(T_USB_INTERFACE *interface, T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
 *      {
 *          //Process class-specific control requests
 *      }
 *
 *      int demo_if_alt_get(T_USB_INTERFACE *interface, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
 *      {
 *          //Process GET_ALT request
 *      }
 *
 *      int demo_if_alt_set(T_USB_INTERFACE *interface, T_HAL_USB_REQUEST_BLOCK *ctrl_urb, uint8_t alt)
 *      {
 *          //Process SET_ALT request
 *      }
 *
 *      int demo_if_create(T_USB_INTERFACE *interface)
 *      {
 *          //Initialize interface-related resources, such as endpoints
 *      }
 *
 *      int demo_if_release(T_USB_INTERFACE *interface)
 *      {
 *          //Deinit interface-related resources initialized in \ref demo_if_create
 *      }
 *
 *      int demo_if_suspend(T_USB_INTERFACE *interface)
 *      {
 *          //Process USB suspend
 *      }
 *
 *      int demo_if_resume(T_USB_INTERFACE *interface)
 *      {
 *          //Process USB resume
 *      }
 *
 *      T_USB_INTERFACE usb_if =
 *      {
 *          .if_num = 0,
 *          .descs_fs = demo_if_descs_fs,
 *          .descs_hs = demo_if_descs_hs,
 *          .ctrl_request_proc = demo_if_ctrl_request_proc,
 *          .alt_get = demo_if_alt_get,
 *          .alt_set = demo_if_alt_set,
 *          .suspend = demo_if_suspend,
 *          .resume = demo_if_resume,
 *          .create = demo_if_create,
 *          .release = demo_if_release,
 *      };
 *      usb_composite_dev_interface_add(&usb_if, cfg val);
 *\endcode
 * @}
 */

/** @defgroup USB_Composite_Device_Exported_Types USB Composite Device Exported Types
  * @{
  */

/**
 *
 * \brief The USB endpoint structure, which is an item of the \ref eps list in \ref T_USB_INTERFACE.
 * \param p_next Point to next endpoint of \ref eps list in \ref T_USB_INTERFACE.
 * \param addr The endpoint address.
 * \param desc The endpoint descriptor.
 * \param ep_handle The endpoint handle.
 *
 */
typedef struct _usb_ep
{
    struct _usb_ep *p_next;
    uint8_t addr;
    T_USB_ENDPOINT_DESC *desc;
    void *ep_handle;
    void *priv;
} T_USB_EP;

/**
 * \brief Core structure to realize interface.
 * \param if_num The interface number. NOTE: the value may be changed in \ref usb_composite_dev_interface_add.
 * \param descs_fs The interface descriptors of full speed.
 * \param descs_hs The interface descriptors of high speed.
 * \param eps The endpoint list belongs to the interface.
 * \param speed_enum_done The callback to process USB enumeration done.
 * \param ctrl_request_proc The callback to process class-specific requests.
 * \param alt_get The callback to process GET_ALT request.
 * \param alt_set The callback to process SET_ALT request.
 * \param create The callback to initialize the interface, and it will be called in \ref usb_composite_dev_interface_add.
 *               Also, \ref if_num in the input parameter of the interface may be changed, the interface MUST update
 *               bInterfaceNumber in the interface descriptor to the actual interface number.
 * \param release The callback to release interface-related resources.
 * \param priv Private data.
 *
 */
typedef struct _usb_interface
{
    uint8_t if_num;
    // uint8_t cur_alt;
    struct usb_descriptor_header **descs_fs;
    struct usb_descriptor_header **descs_hs;
    T_USB_UTILS_LIST eps;

    int (*speed_enum_done)(struct _usb_interface *interface, uint8_t speed);
    int (*ctrl_request_proc)(struct _usb_interface *interface, T_USB_DEVICE_REQUEST *ctrl_request,
                             T_HAL_USB_REQUEST_BLOCK *ctrl_urb);
    int (*alt_get)(struct _usb_interface *interface, T_HAL_USB_REQUEST_BLOCK *ctrl_urb);
    int (*alt_set)(struct _usb_interface *interface, T_HAL_USB_REQUEST_BLOCK *ctrl_urb, uint8_t alt);
    int (*suspend)(struct _usb_interface *interface);
    int (*resume)(struct _usb_interface *interface);
    int (*create)(struct _usb_interface *interface);
    int (*release)(struct _usb_interface *interface);

    void *priv;

} T_USB_INTERFACE;

/**
 *
 * \brief   The callback of USB composite device.
 *
 */
typedef int (*INT_VENDOR_FUNC)(T_USB_DEVICE_REQUEST *ctrl_request, void *buf);

/**
 *
 * \brief   USB composite device callback to complete interactions with other drivers, such as C-Media.
 * \param get: The api will be called when receiving a vendor get request.
 * \param set: The api will be called when receiving a vendor set request.
 *
 */
typedef struct _usb_composite_vendor_cbs
{
    INT_VENDOR_FUNC   get;
    INT_VENDOR_FUNC   set;
} T_USB_COMPOSITE_VENDOR_CBS;

/** End of group USB_Composite_Device_Exported_Types
  * @}
  */

/** @defgroup USB_Composite_Device_Exported_Functions USB Composite Device Exported Functions
  * @{
  */

/**
 *
 * \brief   Register composite device vendor callbacks to process vendor set/get requests.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  cbs Refer to \ref T_USB_COMPOSITE_VENDOR_CBS.
 *
 * \return Refer to `errno.h`.
 */
int usb_composite_dev_vendor_cbs_register(T_USB_COMPOSITE_VENDOR_CBS *cbs);

/**
 *
 * \brief   Unregister composite device vendor callbacks.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return Refer to `errno.h`.
 */
int usb_composite_dev_vendor_cbs_unregister(void);

/**
 * \brief Get the maximum packet size of Endpoint 0.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return The maximum packet size of Endpoint 0.
 *
 */
uint8_t usb_composite_dev_ep0_mps_get(void);

/**
 * \brief Add strings to USB composite device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param language The language of the target string.
 * \param id The ID of target string, which is the value of string index in the corresponding descriptor.
 * \param s The string.
 * \return Refer to `errno.h`.
 * \par Example
 * Please refer to \ref USB_Composite_Usage_Chapter_Device.
 *
 */
int usb_composite_dev_string_add(uint16_t language, uint8_t id, const char *s);

/**
 * \brief Remove strings from USB composite device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param language The language of the target string.
 * \param id The ID of target string, which is the value of string index in the corresponding descriptor.
 * \param s The string.
 * \return Refer to `errno.h`.
 */
int usb_composite_dev_string_remove(uint16_t language, uint8_t id, const char *s);

/**
 * \brief Add a configuration to USB composite device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param cfg_desc The configuration descriptor.
 * \return Refer to `errno.h`.
 * \par Example
 * Please refer to \ref USB_Composite_Usage_Chapter_Device.
 *
 */
int usb_composite_dev_cfg_add(T_USB_CONFIG_DESC *cfg_desc);

/**
 * \brief Remove a configuration from USB composite device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param cfg_desc The configuration descriptor.
 * \return Refer to `errno.h`.
 * \par Example
 * Please refer to \ref USB_Composite_Usage_Chapter_Device.
 *
 */
int usb_composite_dev_cfg_remove(T_USB_CONFIG_DESC *cfg_desc);

/**
 * \brief Add an interface to the target configuration.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param interface Refer to \ref USB_Composite_Usage_Chapter_Interface.
 * \param cfg_val The configuration value that the interface belongs to.
 * \return Refer to `errno.h`.
 * \par Example
 * Please refer to \ref USB_Composite_Usage_Chapter_Interface.
 */
int usb_composite_dev_interface_add(T_USB_INTERFACE *interface, uint8_t cfg_val);

/**
 * \brief Remove an interface from the target configuration.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param interface Refer to \ref USB_Composite_Usage_Chapter_Interface.
 * \param cfg_val The configuration value that the interface belongs to.
 * \return Refer to `errno.h`.
 *
 */
int usb_composite_dev_interface_remove(T_USB_INTERFACE *interface, uint8_t cfg_val);

/**
 * \brief Get enumeration speed, which is the actual operating speed after speed enumeration.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return Refer to \ref T_HAL_USB_SPEED.
 */
T_HAL_USB_SPEED usb_composite_dev_enum_speed_get(void);

/**
 * \brief Remote wakeup.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return Refer to `errno.h`.
 *
 */
int usb_composite_dev_remote_wakeup(void);

/**
 * \brief Initialize the length of the control transfer buffer.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param buf_len The length of the control transfer buffer.
 * \return Refer to `errno.h`.
 *
 */
int usb_composite_dev_ctrl_xfer_buf_init(uint16_t buf_len);

/**
 * \brief Initialize USB composite device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param dev_desc The device descriptor.
 * \return Refer to `errno.h`.
 * \par Example
 * Please refer to \ref USB_Composite_Usage_Chapter_Device.
 *
 */
int usb_composite_dev_init(T_USB_DEVICE_DESC *dev_desc);

/**
 * \brief Deinit USB composite device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 */
int usb_composite_dev_deinit(void);

/** @} */ /* End of group USB_Composite_Device_Exported_Functions */
/** \}*/
#endif
