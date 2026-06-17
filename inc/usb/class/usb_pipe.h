/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_PIPE_H__
#define __USB_PIPE_H__
#include <stdint.h>
#include "usb_spec20.h"

/**
 * \addtogroup USB_Pipe
 * \brief The module mainly provides common pipe operations for interrupt and bulk endpoint. \n
 * This driver supports opening, sending, and receiving data pipes.
 * @{
  */

/**
 * \defgroup USB_PIPE_USAGE How to Use USB Pipes
 * @{
 *
 * \brief This section provides a detailed explanation on utilizing USB pipes,
 *        accompanied by sample code for better understanding.
 *
 * \section USB_PIPE_OPEN_PIPE Open Pipe
 * Open data pipe by \ref usb_pipe_open.
 *
 * \par Example
 * \code
 *      // open in endpoint
 *      T_USB_HID_ATTR in_attr =
 *      {
 *          .zlp = 1,
 *          .high_throughput = 0,
 *          .congestion_ctrl = USB_PIPE_CONGESTION_CTRL_DROP_CUR,
 *          .rsv = 0,
 *          .mtu = HID_MAX_TRANSMISSION_UNIT
 *      };
 *      void *demo_in_handle = usb_pipe_open(in_desc, in_ep_addr, in_attr, in_pending_req_num, in_cb);
 *      // open out endpoint. If you intend to use an output endpoint, you must open that output endpoint
 *      // during the initialization phase, prior to USB enumeration, because the alternate setting (alt_set)
 *      // function will call \ref usb_pipe_recv.
 *      T_USB_HID_ATTR out_attr =
 *      {
 *          .zlp = 0,
 *          .high_throughput = 0,
 *          .congestion_ctrl = USB_PIPE_CONGESTION_CTRL_DROP_CUR,
 *          .rsv = 0,
 *          .mtu = HID_MAX_TRANSMISSION_UNIT
 *      };
 *      void *demo_out_handle = usb_pipe_open(out_desc, in_ep_addr, out_attr, out_pending_req_num, out_cb);
 * \endcode
 *
 * \section USB_PIPE_SEND_DATA Send Data
 * Send data by \ref usb_pipe_send.
 *
 * \par Example
 * \code
 *      usb_hid_data_pipe_send(demo_in_handle, data, length);
 * \endcode
 *
 * \section USB_PIPE_RECEIVE_DATA Receive Data
 * Receive data by \ref usb_pipe_recv.
 *
 * \par Example
 * \code
 *      usb_pipe_recv(demo_out_handle);
 * \endcode
 */
/** @}*/

/** @defgroup USB_PIPE_Exported_Constants USB PIPE Exported Constants
  * @{
  */

/**
 * \brief   Congestion control.
 *
 * \details If the macro is set, the current data to be sent will be dropped;
 *          otherwise, the first data in the queue will be dropped.
 *
 * \note    Only effective for in endpoint.
 *
 */
#define USB_PIPE_CONGESTION_CTRL_DROP_CUR     (0)

/**
 * \brief   Congestion control.
 *
 * \details If the macro is set, the first data in the queue will be dropped.
 *
 * \note    Only effective for in endpoint.
 *
 */
#define USB_PIPE_CONGESTION_CTRL_DROP_FIRST   (1)

/** End of group USB_PIPE_Exported_Constants
  * @}
  */

/** @defgroup USB_PIPE_Exported_Types USB PIPE Exported Types
  * @{
  */

/**
 * \brief   USB pipe attribute.
 *
 * \details
 * \param zlp: Zero length packet.
 * \param high_throughput: If it is set to 1, it can be be executed in the interrupt; otherwise it is excuted in a task.
 * \param congestion_ctrl: If it is set to 0, drop the current data; otherwise drop the first data in the queue.
 * \param direct: If it is set to 1, the \ref buf in \ref usb_pipe_send will be directly used for data transmission. \n
 *                If it is set to 0, the \ref buf in \ref usb_pipe_send will be copied to the buf in urb before data transmission. \n
 *                The parameter is only valid for in pipe and when \ref pending_req_num is 0 in \ref usb_pipe_open.
 * \param rsv: Reserved.
 * \param mtu: The maximum transfer unit.
 *
 *  \note   The congestion control is only effective for input endpoints.
 */
typedef struct _usb_pipe_attr
{
    uint16_t zlp: 1;
    uint16_t high_throughput: 1;
    uint16_t congestion_ctrl: 2;
    uint16_t direct: 1;
    uint16_t rsv: 11;
    uint32_t mtu;
} T_USB_PIPE_ATTR;

/**
 * \brief   USB pipe callback.
 *
 */
typedef uint32_t (*USB_PIPE_CB)(void *handle, void *buf, uint32_t len, int status);

/** End of group USB_PIPE_Exported_Types
  * @}
  */

/** @defgroup USB_PIPE_Exported_Functions USB PIPE Exported Functions
  * @{
  */

/**
 * \brief   Open data pipe.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  desc The descriptor of endpoint.
 * \param  ep_addr The address of endpoint.
 * \param  attr The pipe attribute of \ref T_USB_PIPE_ATTR.
 * \param  pending_req_num The number of supported pending requests.
 * \param  cb The application callbacks of \ref USB_PIPE_CB, which will be called after data transmission is completed.
 *
 * \return The pipe handle.
 */
void *usb_pipe_open(T_USB_ENDPOINT_DESC **desc, uint8_t ep_addr, T_USB_PIPE_ATTR attr,
                    uint8_t pending_req_num, USB_PIPE_CB cb);

/**
 * \brief   Close data pipe.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  handle The return value of \ref usb_pipe_open.
 *
 * \return Refer to `errno.h`.
 */
int usb_pipe_close(void *handle);

/**
 * \brief   Get data pipe pending number.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  handle The return value of \ref usb_pipe_open.
 *
 * \return The pending number.
 */
int usb_pipe_pending_num_get(void *handle);

/**
 * \brief   Send data pipe.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \details The data is sent serially, which means that data will not be actually sent until the previous data transmission is complete.
 *
 * \param  handle The return value of \ref usb_pipe_open.
 * \param  buf The data to be sent.
 * \param  len The length of data.
 *
 * \return Refer to `errno.h`.
 */
int usb_pipe_send(void *handle, void *buf, uint32_t len);

/**
 * \brief   Receive data pipe.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  handle The return value of \ref usb_pipe_open.
 * \param  buf The data to be received. If \ref direct is set to 1 in \ref usb_pipe_open, the buf is used to store \n
 *             the received data and the buf should be 4-byte aligned. Otherwise, the buf will be ignored.
 * \param  len The length of data.
 *
 * \return Refer to `errno.h`.
 */
int usb_pipe_recv(void *handle, uint8_t *buf, uint32_t len);

/**
 * \brief Stall USB pipe, then all data transfer through the pipe will be \b stalled.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param handle The return value of \ref usb_pipe_open.
 * \param err The error number that leads to pipe stalling.
 * \return Refer to `errno.h`.
 *
 */
int usb_pipe_stall_set(void *handle, int err);

/**
 * \brief Clear the pipe stall status set by \ref usb_pipe_stall_set.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param handle The return value of \ref usb_pipe_open.
 * \return Refer to `errno.h`.
 */
int usb_pipe_stall_clear(void *handle);

/**
 * \brief Get the attributes of USB pipe.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param handle The return value of \ref usb_pipe_open.
 * \return T_USB_PIPE_ATTR* pipe attribute.
 */
T_USB_PIPE_ATTR *usb_pipe_attr_get(void *handle);

/**
 * \brief   Flush data pipe.
 *
 * \param  handle The return value of \ref usb_pipe_open.
 *
 * \return Refer to `errno.h`.
 */
int usb_pipe_flush(void *handle);

/**
 * \brief Nuke all pending requests in the pipe with the specified error code.
 */
int usb_pipes_nuke(int err);

/** @} */ /* End of group USB_PIPE_Exported_Functions */
/** @}*/
/** End of USB_PIPE
*/

#endif
