/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __HID_DESC_H__
#define __HID_DESC_H__
#include <stdint.h>
#include "app_flags.h"

#define USB_CLASS_CODE_HID                  0x03

#define DESC_TYPE_HID                       0x21
#define DESC_TYPE_REPORT                    0x22

#define HID_REPORT_ID_AUDIO_CONTROL         0x01
#define HID_REPORT_DESC_CONSUMER_CONTROL  \
    0x05, 0x0C,                         /* Usage Page (Consumer)            */ \
          0x09, 0x01,                         /* Usage (Consumer Control)         */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
          0x85, HID_REPORT_ID_AUDIO_CONTROL,  /* Report ID (1)                    */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x01,                         /* Logical Maximum (1)              */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x01,                         /* Report Count (1)                 */ \
          0x09, 0xE9,                         /* Usage (Volume Increment)         */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x09, 0xEA,                         /* Usage (Volume Decrement)         */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x09, 0xCD,                         /* Usage (Play/Pause)               */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x09, 0xB5,                         /* Usage (Scan Next Track)          */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x09, 0xB6,                         /* Usage (Scan Previous Track)      */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x03,                         /* Report Count (3)                 */ \
          0x81, 0x01,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0xC0,                               /* END_COLLECTION                    */

#define HID_REPORT_DESC_KEYBOARD  \
    0x05, 0x01,                         /* Usage Page (Generic Desktop)            */ \
          0x09, 0x06,                         /* Usage (keyboard)         */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
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
          0xC0,                               /* END_COLLECTION                    */

#define HID_REPORT_DESC_KEYBOARD_DELL  \
    0x05, 0x01,                         /* Usage Page (Generic Desktop)            */ \
          0x09, 0x06,                         /* Usage (keyboard)         */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
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
          0xC0,                               /* END_COLLECTION                    */

#define HID_REPORT_DESC_MOUSE_LOGIC  \
    0x05, 0x01,                         /* Usage Page (Generic Desktop)            */ \
          0x09, 0x02,                         /* Usage (Mouse)         */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
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
          0xC0,                               /* END_COLLECTION                    */

#define USB_HID_VOLUME_UP {HID_REPORT_ID_AUDIO_CONTROL, 0x01}
#define USB_HID_VOLUME_DOWN {HID_REPORT_ID_AUDIO_CONTROL, 0x02}
#define USB_HID_VOLUME_RELEASE {HID_REPORT_ID_AUDIO_CONTROL, 0x00}

#define CONSUMER_CTRL_MAX_TRANSMISSION_UNIT                   0x02
#define CONSUMER_CTRL_MAX_PENDING_REQ_NUM                     0x0A

#if F_APP_USB_AUDIO_SUPPORT
#define HID_REPORT_DESCS        HID_REPORT_DESC_CONSUMER_CONTROL
#else
#if USB_HID_MOUSE_EN
#define HID_REPORT_DESCS        HID_REPORT_DESC_MOUSE_LOGIC
#endif
#if USB_HID_KEYBOARD_EN
#define HID_REPORT_DESCS        HID_REPORT_DESC_KEYBOARD //HID_REPORT_DESC_KEYBOARD_DELL
#endif
#endif

#endif
