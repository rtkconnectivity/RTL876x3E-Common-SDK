/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_HID_DRIVER_H__
#define __USB_HID_DRIVER_H__
#include <stdbool.h>
#include <stdint.h>
#include "usb_pipe.h"
/**
 * \addtogroup USB_HID_Driver
 * \brief The module primarily provides components for implementing USB HID Class.
 * This driver support multiple interfaces and endpoints, along with a variety of functions
 * such as consumer control, mouse, keyboard and so on.
 * @{
  */

/**
 * \defgroup USB_HID_DRIVER_USAGE How to Implement USB HID Interface
 * @{
 *
 * \brief This section provides a comprehensive guide on implementing a USB HID interface,
 *        complete with sample code for your reference.
 *
 * \section USB_HID_DRIVER_ALLOCATE_INSTANCE Allocate Instance
 * Allocate a function instance by \ref usb_hid_driver_inst_alloc.
 *
 * \par Example
 * \code
 *      void *demo_instance = usb_hid_driver_inst_alloc();
 * \endcode
 *
 * \section USB_HID_DRIVER_HID_INTERFACES HID Interfaces
 * Initialize HID interfaces as follows:
 *    - Initialize a repot descriptor.
 *    - Initialize descriptor arrays.
 *    - Implement pipe operations.
 *
 * \par Example
 * \code
 *      const char demo_report_descs[] =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_INTERFACE_DESC demo_std_if_desc =
 *      {
 *          ...init...
 *      };
 *      T_HID_CS_IF_DESC demo_cs_if_desc =
 *      {
 *          ...init...
 *      };
 *      T_USB_ENDPOINT_DESC demo_int_in_ep_desc_fs =
 *      {
 *          ...init...
 *      };
 *      T_USB_ENDPOINT_DESC demo_int_in_ep_desc_hs =
 *      {
 *          ...init...
 *      };
 *      void *demo_if_descs_fs[] =
 *      {
 *          (void *) &demo_std_if_desc,
 *          (void *) &demo_cs_if_desc,
 *          (void *) &demo_int_in_ep_desc_fs,
 *          NULL,
 *      };
 *      void *demo_if_descs_hs[] =
 *      {
 *          ...
 *          NULL,
 *      };
 *
 *      void *demo_data_pipe_open(uint8_t ep_addr, T_USB_HID_DRIVER_ATTR attr, uint8_t pending_req_num,
 *                                  USB_HID_DATA_PIPE_CB cb)
 *      {
 *          ...
 *          return usb_hid_driver_data_pipe_open(ep_addr, attr, pending_req_num, cb);
 *      }
 *      bool demo_data_pipe_send(void *handle, void *buf, uint32_t len)
 *      {
 *          usb_hid_driver_data_pipe_send(handle, buf, len);
 *          return true;
 *      }
 *      int demo_data_pipe_close(void *handle)
 *      {
 *          return usb_hid_driver_data_pipe_close(handle);
 *      }
 *      int demo_data_pipe_flush(void *handle)
 *      {
 *          return usb_hid_driver_data_pipe_flush(handle);
 *      }
 *      usb_hid_driver_if_desc_register(demo_instance, (void *)demo_if_descs_hs, (void *)demo_if_descs_fs, (void *)demo_report_descs);
 *      T_USB_HID_DRIVER_CB demo_cbs =
 *      {
 *          .get_report = demo_get_report,
 *          .set_report = demo_set_report,
 *      }
 *      usb_hid_driver_cbs_register(demo_instance, &demo_cbs);
 * \endcode
 *
 * \section USB_HID_DRIVER_INITIALIZE_HID_DRIVER Initialize HID Driver
 * Call \ref usb_hid_driver_init to initialize USB HID driver.
 */
/** @}*/

/** @defgroup USB_HID_Driver_Exported_Constants USB HID Driver Exported Constants
  * @{
  */

/**
 * \brief   The zero-length packet attribute of an HID pipe.
 *
 * \details If the ZLP attribute is set, HID will send a zero-length packet when the data length
 *          equals to the maximum packet size of an endpoint.
 *
 */
#define HID_PIPE_ATTR_ZLP   0x0001

/**
 * \brief   Congestion control.
 *
 * \details If HID_DRIVER_CONGESTION_CTRL_DROP_CUR is set, current data to send
 *          will be dropped, else the first data in the queue will be dropped.
 *
 * \note    Only effective for in endpoint.
 *
 */
#define HID_DRIVER_CONGESTION_CTRL_DROP_CUR     USB_PIPE_CONGESTION_CTRL_DROP_CUR
/**
 * \brief   Congestion control.
 *
 * \details If HID_DRIVER_CONGESTION_CTRL_DROP_FIRST is set, the first data in the queue will be dropped.
 *
 * \note    Only effective for in endpoint.
 *
 */
#define HID_DRIVER_CONGESTION_CTRL_DROP_FIRST   USB_PIPE_CONGESTION_CTRL_DROP_FIRST

/**
 * \brief   The number of HID report id defined in \ref T_USB_HID_DRIVER_REPORT_IDS.
 *
 */
#define HID_DRIVER_REPORT_ID_NUM    10

/** End of group USB_HID_Driver_Exported_Constants
  * @}
  */

/** @defgroup USB_HID_Driver_Exported_Types USB HID Driver Exported Types
  * @{
  */

/**
 * \brief   USB HID pipe attribute.
 *
 */
typedef T_USB_PIPE_ATTR T_USB_HID_DRIVER_ATTR;

/**
 * \brief   HID report value.
 *
 */
typedef struct _hid_driver_request_val
{
    uint8_t     id;
    uint8_t     type;
} T_HID_DRIVER_REPORT_REQ_VAL;

/**
 * \brief USB HID driver callback.
 */
typedef int32_t (*INT_IN_FUNC)(T_HID_DRIVER_REPORT_REQ_VAL, void *, uint16_t *);
/**
 * \brief USB HID driver callback.
 */
typedef int32_t (*INT_OUT_FUNC)(T_HID_DRIVER_REPORT_REQ_VAL, void *, uint16_t);
/**
 * \brief USB HID driver callback.
 */
typedef int32_t (*PROTOCOL_CHANGE_FUNC)(uint8_t protocol);
/**
 * \brief USB HID pipe callback.
 */
typedef USB_PIPE_CB USB_HID_DRIVER_CB;

/**
 * \brief HID driver callbacks
 *
 * \param get_report: This API will be called when a HID get_report request is received.
 * \param set_report: This API will be called when a HID set_report request is received.
 */
typedef struct _usb_hid_driver_cbs
{
    INT_IN_FUNC    get_report;           /**< Callback for GET_REPORT request */
    INT_OUT_FUNC   set_report;           /**< Callback for SET_REPORT request */
} T_USB_HID_DRIVER_CBS;

/**
 * \brief The report IDs used in \ref T_USB_HID_DRIVER_REPORT_DESC_PARSER.
 *
 * \param cnt: The report ID count.
 * \param info: The address and size of report IDs.
 */
typedef struct _usb_hid_driver_report_ids
{
    uint8_t cnt;
    struct
    {
        uint16_t addr;
        uint8_t size;
    } info[HID_DRIVER_REPORT_ID_NUM];
} T_USB_HID_DRIVER_REPORT_IDS;

/**
 * \brief Report descriptor parser used in \ref USB_HID_DRIVER_REPORT_DESC_PARSE_CB.
 *
 * \param type: The usage type.
 * \param start: The start of a usage.
 * \param end: The end of a usage.
 * \param tlc_start: The next byte of TLC, report ID can be inserted into this byte.
 * \param include_physical: Indicate if the report map includes a physical field.
 * \param report_ids: Refer to \ref T_USB_HID_DRIVER_REPORT_IDS.
 */
typedef struct _usb_hid_driver_report_desc_parser
{
    uint32_t type;
    uint16_t start;
    uint16_t end;
    uint16_t tlc_start;

    uint16_t include_physical: 1;
    uint16_t rsv: 15;

    T_USB_HID_DRIVER_REPORT_IDS report_ids;
} T_USB_HID_DRIVER_REPORT_DESC_PARSER;

/**
 * \brief   USB HID report descriptor parse callback.
 *
 * \param T_USB_HID_DRIVER_REPORT_DESC_PARSER USB HID report descriptor parser defined in
 *            \ref T_USB_HID_DRIVER_REPORT_DESC_PARSER.
 *
 */
typedef int (*USB_HID_DRIVER_REPORT_DESC_PARSE_CB)(T_USB_HID_DRIVER_REPORT_DESC_PARSER *parser);

/** End of group USB_HID_Driver_Exported_Types
  * @}
  */

/** @defgroup USB_HID_Driver_Exported_Functions USB HID Driver Exported Functions
  * @{
  */

/**
 * \brief Allocate the HID function instance, which possesses independent function.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return  HID function instance.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 */
void *usb_hid_driver_inst_alloc(void);

/**
 * \brief Free HID function instance alloacted by \ref usb_hid_driver_inst_alloc.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param inst The instance alloacted by \ref usb_hid_driver_inst_alloc.
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 */
int usb_hid_driver_inst_free(void *inst);

/**
 * \brief  Register HID interface descriptors to the driver.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  inst HID instance returned by \ref usb_hid_driver_inst_alloc.
 * \param  hs_desc HID interface descriptor of high speed.
 * \param  fs_desc HID interface descriptor of full speed.
 * \param  report_desc HID report descriptor.
 *
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 */
int usb_hid_driver_if_desc_register(void *inst, void *hs_desc, void *fs_desc, void *report_desc);

/**
 * \brief  Unregister HID interface descriptors.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param inst HID instance returned in \ref usb_hid_driver_inst_alloc.
 *
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 */
int usb_hid_driver_if_desc_unregister(void *inst);

/**
 * \brief   Parse report infomation to the application.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param report_desc The report descriptor.
 *
 * \param report_len The length of report descriptor.
 *
 * \param cb Report parser callback defined in \ref USB_HID_DRIVER_REPORT_DESC_PARSE_CB.
 *
 * \return Refer to `errno.h`.
 *
 * \par Example
 * \code
 *      const char demo_report_descs[] =
 *      {
 *          ...init...
 *      };
 *      demo_report_len = sizeof(demo_report_descs);
 *      int demo_cb(T_USB_HID_DRIVER_REPORT_DESC_PARSER *demo_parser)
 *      {
 *          // the application receives report descriptor parser and then processes it
 *          ...
 *      }
 *      usb_hid_driver_report_desc_parse(demo_report_descs, demo_report_len, demo_cb);
 * \endcode
 */
int usb_hid_driver_report_desc_parse(uint8_t *report_desc, uint16_t len,
                                     USB_HID_DRIVER_REPORT_DESC_PARSE_CB cb);

/**
 * \brief   Open HID data pipe.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  ep_addr The endpoint address.
 * \param  attr The attribute of \ref T_USB_HID_DRIVER_ATTR.
 * \param  pending_req_num The number of supported pending requests.
 * \param  cb The application callbacks of \ref USB_HID_DRIVER_CB, which will be called after
 *            data transmission is completed.
 *
 * \return HID handle.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 */
void *usb_hid_driver_data_pipe_open(uint8_t ep_addr, T_USB_HID_DRIVER_ATTR attr,
                                    uint8_t pending_req_num, USB_HID_DRIVER_CB cb);

/**
 * \brief   Close HID data pipe.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  handle The return value of \ref usb_hid_driver_data_pipe_open.
 *
 * \return Refer to `errno.h`.
 */
int usb_hid_driver_data_pipe_close(void *handle);

/**
 * \brief   HID pipe sends data.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  handle The return value of \ref usb_hid_driver_data_pipe_open.
 * \param  buf The data to be sent.
 * \param  len The length of data.
 *
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 */
int usb_hid_driver_data_pipe_send(void *handle, void *buf, uint32_t len);

/**
 * \brief   Flush HID pipe.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  handle The return value of \ref usb_hid_driver_data_pipe_open.
 *
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 *
 */
int usb_hid_driver_data_pipe_flush(void *handle);
/**
 * \brief   Get data pipe pending number
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  handle The return value of \ref usb_hid_driver_data_pipe_open.
 *
 * \return The pending number.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 *
 */
int usb_hid_driver_data_pipe_pending_num_get(void *handle);

/**
 * \brief   Register HID driver callbacks to process set_reoprt/get_report request.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  inst HID instance returned by \ref usb_hid_driver_inst_alloc.
 * \param  cbs Refer to \ref T_USB_HID_DRIVER_CBS.
 *
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 */
int usb_hid_driver_cbs_register(void *inst, T_USB_HID_DRIVER_CBS *cbs);

/**
 * \brief   Unregister HID driver callbacks.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  inst HID instance returned by \ref usb_hid_driver_inst_alloc.
 *
 * \return Refer to `errno.h`.
 */
int usb_hid_driver_cbs_unregister(void *inst);

/**
 * \brief   USB remote wakeup.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return Refer to `errno.h`.
 */
int usb_hid_driver_remote_wakeup(void);

/**
 * \brief   Initalize USB HID interfaces.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_HID_DRIVER_USAGE.
 */
int usb_hid_driver_init(void);

/**
 * \brief   Deinit USB HID interfaces.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return Refer to `errno.h`.
 */
int usb_hid_driver_deinit(void);

/** @} */ /* End of group USB_HID_Driver_Exported_Functions */
/** @}*/
#endif
