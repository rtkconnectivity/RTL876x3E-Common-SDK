/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _HID_H_
#define _HID_H_
#include <stdint.h>
#include <stdbool.h>
#include "app_flags.h"
#include "usb_hid_driver.h"

/** @defgroup USB_HID USB Hid
  * @brief USB hid data pipe usage
  * @{
  */
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#if F_APP_USB_HID_SEC_SUPPORT
#define USB_HID_SEC_SUPPORT 1
#else
#define USB_HID_SEC_SUPPORT 0
#endif
#else
#define USB_HID_SEC_SUPPORT 0
#endif

#define     HID_INT_IN_EP_1                 0x81
#define     HID_INT_IN_EP_2                 0x82
#define     HID_INT_IN_EP_4                 0x84

#define     HID_REPORT_TYPE_INPUT           0x01
#define     HID_REPORT_TYPE_OUTPUT          0x02
#define     HID_REPORT_TYPE_FEATURE         0x03

#define     HID_REPORT_ID_AUDIO_CONTROL     0x01
#define     HID_REPORT_ID_KEYBOARD          0x02
#define     HID_REPORT_ID_MOUSE             0x03

#define     HID_CONGESTION_CTRL_DROP_CUR    HID_DRIVER_CONGESTION_CTRL_DROP_CUR
#define     HID_CONGESTION_CTRL_DROP_FIRST  HID_DRIVER_CONGESTION_CTRL_DROP_FIRST

#define     USB_HID_MAX_PENDING_REQ_NUM     0x05

#define HID_REPORT_DESC_KEYBOARD  \
    0x05, 0x01,                         /* Usage Page (Generic Desktop)            */ \
          0x09, 0x06,                         /* Usage (keyboard)         */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
          0x85, HID_REPORT_ID_KEYBOARD,       /* Report ID (1)                    */ \
          0x05, 0x07,  /* Usage Page (key codes)                    */ \
          0x19, 0xE0,                         /* Usage Minimum (224)              */ \
          0x29, 0xE7,                         /* Usage Maximum (231)              */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x01,                         /* Logical Maximum (1)              */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x08,                         /* Report Count (8)                 */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x01,                         /* Report Count (1)                 */ \
          0x75, 0x08,                         /* Report Size (8)                  */ \
          0x81, 0x01,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x05,                         /* Report Count (5)                 */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x05, 0x08,  /* Usage Page (page# for LEDs)                    */ \
          0x19, 0x01,                         /* Usage Minimum (1)              */ \
          0x29, 0x05,                         /* Usage Maximum (5)              */ \
          0x91, 0x02,                         /* Output led report(Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x01,                         /* Report Count (1)                 */ \
          0x75, 0x03,                         /* Report Size (3)                  */ \
          0x91, 0x01,                         /* Output led report padding(Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x06,                         /* Report Count (6)                 */ \
          0x75, 0x08,                         /* Report Size (8)                  */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x65,                         /* Logical Maximum (101)              */ \
          0x05, 0x07,  /* Usage Page (key codes)                    */ \
          0x19, 0x00,                         /* Usage Minimum (0)              */ \
          0x29, 0x65,                         /* Usage Maximum (101)              */ \
          0x81, 0x00,                         /* Input key arrays(6bytes)(Data,Arr)*/ \
          0xC0                                /* END_COLLECTION                    */

#define HID_REPORT_DESC_KEYBOARD_DELL  \
    0x05, 0x01,                         /* Usage Page (Generic Desktop)            */ \
          0x09, 0x06,                         /* Usage (keyboard)         */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
          0x85, HID_REPORT_ID_KEYBOARD,       /* Report ID (1)                    */ \
          0x05, 0x07,                         /* Usage Page (key codes)                    */ \
          0x19, 0xE0,                         /* Usage Minimum (224)              */ \
          0x29, 0xE7,                         /* Usage Maximum (231)              */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x01,                         /* Logical Maximum (1)              */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x08,                         /* Report Count (8)                 */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x01,                         /* Report Count (1)                 */ \
          0x75, 0x08,                         /* Report Size (8)                  */ \
          0x81, 0x01,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x03,                         /* Report Count (3)                 */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x05, 0x08,                       /* Usage Page (page# for LEDs)                    */ \
          0x19, 0x01,                         /* Usage Minimum (1)              */ \
          0x29, 0x03,                         /* Usage Maximum (3)              */ \
          0x91, 0x02,                         /* Output led report(Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x01,                         /* Report Count (1)                 */ \
          0x75, 0x05,                         /* Report Size (5)                  */ \
          0x91, 0x01,                         /* Output led report padding(Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x06,                         /* Report Count (6)                 */ \
          0x75, 0x08,                         /* Report Size (8)                  */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x26, 0xff, 0x00,                   /* Logical Maximum (101)              */ \
          0x05, 0x07,                       /* Usage Page (key codes)                    */ \
          0x19, 0x00,                         /* Usage Minimum (0)              */ \
          0x2a, 0xff, 0x00,                   /* Usage Maximum (101)              */ \
          0x81, 0x00,                         /* Input key arrays(6bytes)(Data,Arr)*/ \
          0xC0                                /* END_COLLECTION                    */

#define HID_REPORT_DESC_MOUSE_LOGIC  \
    0x05, 0x01,                         /* Usage Page (Generic Desktop)            */ \
          0x09, 0x02,                         /* Usage (Mouse)         */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
          0x85, HID_REPORT_ID_MOUSE,       /* Report ID (1)                    */ \
          0x09, 0x01,  /* Usage (pointer)                    */ \
          0xA1, 0x00,                         /* Collection (Physical)         */ \
          0x05, 0x09,  /* Usage page(Button)                    */ \
          0x19, 0x01,                         /* Usage Minimum (1)              */ \
          0x29, 0x03,                         /* Usage Maximum (3)              */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x01,                         /* Logical Maximum (1)              */ \
          0x95, 0x08,                         /* Report Count (8)                 */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x81, 0x02,                         /* Input 3bits(Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x05, 0x01,  /* Usage page(Generic Desktop)                    */ \
          0x09, 0x30,                         /* Usage (X)              */ \
          0x09, 0x31,                         /* Usage (Y)              */ \
          0x09, 0x38,                         /* Usage (M)              */ \
          0x15, 0x81,                         /* Logical Minimum (127)              */ \
          0x25, 0x7F,                         /* Logical Maximum (-127)              */ \
          0x75, 0x08,                         /* Report Size (8)                  */ \
          0x95, 0x03,                         /* Report Count (2)                 */ \
          0x81, 0x06,                         /* Input 2position bytes(X&Y)(Data,Arr)*/ \
          0xC0,                               /* END_COLLECTION                    */ \
          0xC0                                /* END_COLLECTION                    */

extern void *inst;
extern void *inst1;

typedef uint32_t (*USB_HID_DATA_PIPE_CB)(void *handle, void *buf, uint32_t len, int status);

/**
 * usb_hid.h
 *
 * \brief   USB HID pipe attr
 *          \ref zlp: zero length packet
 *          \ref high_throughput: if it is set to 1, it can be be executed in interrupt, else it excute in task.
 *          \ref congestion_ctrl: if it is set to 0, drop current data, else drop the first data in the queue.
 *          \ref rsv: reserved
 *          \ref mtu: maximum transfer unit
 *
 * \ingroup USB_HID
 */
typedef struct _usb_hid_attr
{
    uint16_t zlp: 1;
    uint16_t high_throughput: 1;
    uint16_t congestion_ctrl: 2;
    uint16_t rsv: 12;
    uint32_t mtu;
} T_USB_HID_ATTR;

/**
 * usb_hid.h
 *
 * \brief   HID callback
 *          \ref get_report: this api will be called when receiving HID GET_REPORT request
 *          \ref set_report: this api will be called when receiving HID SET_REPORT request
 *
 */
typedef struct _hid_cbs
{
    INT_IN_FUNC    get_report;
    INT_OUT_FUNC   set_report;
} T_HID_CBS;

/**
 * usb_hid.h
 *
 * \brief   open hid data pipe
 *
 * \param[in]  ep_addr ep address
 * \param[in]  attr HID pipe attribute of \ref T_USB_HID_ATTR
 * \param[in]  pending_req_num supported pending request number
 * \param[in] cb callback of \ref USB_HID_DATA_PIPE_CB, which will be called after data is sent over
 *
 * \return handle
 *
 * \ingroup USB_HID
 */
void *usb_hid_data_pipe_open(uint8_t ep_addr, T_USB_HID_ATTR attr, uint8_t pending_req_num,
                             USB_HID_DATA_PIPE_CB cb);

/**
 * usb_hid.h
 *
 * \brief   hid pipe send data
 *
 * \details data is sent serially, which means data is not sent actually until previous data transmission is complete.
 *
 * \param[in]  handle return value of \ref usb_hid_data_pipe_open
 * \param[in]  buf data will be sent
 * \param[in]  len length of data
 *
 * \return true data will be sent now or after all previous transmissions are complete, false will never be sent
 *
 * \ingroup USB_HID
 */
int usb_hid_data_pipe_send(void *handle, void *buf, uint32_t len);

/**
 * usb_hid.h
 *
 * \brief   close hid data pipe
 *
 * \param[in]  handle return value of \ref usb_hid_data_pipe_open
 *
 * \return int result, refer to "errno.h"
 *
 * \ingroup USB_HID
 */
int usb_hid_data_pipe_close(void *handle);

/**
 * usb_hid.h
 *
 * \brief   update HID report descriptor register
 *
 * \param[in]  ep_addr ep address
 * \param[in]  desc HID report descriptor
 * \param[in]  desc length of HID report descriptor
 *
 */
void usb_hid_recfg_report_descs(uint8_t ep_addr, void *report_desc, uint16_t desc_len);

/**
 * usb_hid.h
 *
 * \brief   usb hid callback register
 *
 * \param[in]  cbs callback of \ref T_HID_CBS
 *
 * \ingroup USB_HID
 */
int usb_hid_ual_register(T_HID_CBS cbs);

/**
 * usb_hid.h
 *
 * \brief   usb hid callback unregister
 *
 * \ingroup USB_HID
 */
int usb_hid_ual_unregister(void);

/**
 * usb_hid.h
 *
 * \brief   usb hid init
 *
 * \ingroup USB_HID
 */
void usb_hid_init(void);

/**
 * usb_hid.h
 *
 * \brief   usb hid deinit
 *
 * \ingroup USB_HID
 */
void usb_hid_deinit(void);

/** @}*/
/** End of USB_HID
*/

#endif
