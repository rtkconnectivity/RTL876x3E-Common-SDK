/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "console.h"
#include "os_mem.h"
#include "mem_types.h"
#include "pm.h"
#include "console.h"
#include "gap_br.h"
#include "btm.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "bt_bond.h"
#include "bt_hid_parser.h"
#include "bt_hid_device.h"
#include "bt_hid_host.h"
#include "app_bt_hid_demo_link.h"
#include "app_bt_hid_demo.h"

#define CHAR_ILLEGAL     0xff
#define CHAR_RETURN     '\n'
#define CHAR_ESCAPE      27
#define CHAR_TAB         '\t'
#define CHAR_BACKSPACE   0x7f

extern T_OS_QUEUE descriptor_list;

static const uint8_t keytable_us_none[] =
{
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /*   0-3 */
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',                   /*  4-13 */
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',                   /* 14-23 */
    'u', 'v', 'w', 'x', 'y', 'z',                                       /* 24-29 */
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',                   /* 30-39 */
    CHAR_RETURN, CHAR_ESCAPE, CHAR_BACKSPACE, CHAR_TAB, ' ',            /* 40-44 */
    '-', '=', '[', ']', '\\', CHAR_ILLEGAL, ';', '\'', 0x60, ',',       /* 45-54 */
    '.', '/', CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,   /* 55-60 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 61-64 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 65-68 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 69-72 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 73-76 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 77-80 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 81-84 */
    '*', '-', '+', '\n', '1', '2', '3', '4', '5',                       /* 85-97 */
    '6', '7', '8', '9', '0', '.', 0xa7,                                 /* 97-100 */
};

static const uint8_t keytable_us_shift[] =
{
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /*  0-3  */
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',                   /*  4-13 */
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',                   /* 14-23 */
    'U', 'V', 'W', 'X', 'Y', 'Z',                                       /* 24-29 */
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',                   /* 30-39 */
    CHAR_RETURN, CHAR_ESCAPE, CHAR_BACKSPACE, CHAR_TAB, ' ',            /* 40-44 */
    '_', '+', '{', '}', '|', CHAR_ILLEGAL, ':', '"', 0x7E, '<',         /* 45-54 */
    '>', '?', CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,   /* 55-60 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 61-64 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 65-68 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 69-72 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 73-76 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 77-80 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 81-84 */
    '*', '-', '+', '\n', '1', '2', '3', '4', '5',                       /* 85-97 */
    '6', '7', '8', '9', '0', '.', 0xb1,                                 /* 97-100 */
};

#if (BT_HID_DEMO_ROLE == 2)
#if F_APP_HID_MOUSE_SUPPORT
static const uint8_t hid_mouse_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR_2BYTE,
    0x01,
    0x06,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_HUMAN_INTERFACE_DEVICE_SERVICE >> 8),
    (uint8_t)(UUID_HUMAN_INTERFACE_DEVICE_SERVICE),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0D,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_HID_CONTROL >> 8),
    (uint8_t)(PSM_HID_CONTROL),
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_HIDP >> 8),
    (uint8_t)(UUID_HIDP),

    //Attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)(SDP_LANG_ENGLISH),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)(SDP_CHARACTER_UTF8),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)(SDP_BASE_LANG_OFFSET),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST),
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_HUMAN_INTERFACE_DEVICE_SERVICE >> 8),
    (uint8_t)(UUID_HUMAN_INTERFACE_DEVICE_SERVICE),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0101 >> 8),
    (uint8_t)(0x0101),

    //Attribute SDP_ATTR_ADD_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_ADD_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_ADD_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0F,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0D,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(0x0100 >> 8),
    (uint8_t)(0x0100),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_HID_INTERRUPT >> 8),
    (uint8_t)(PSM_HID_INTERRUPT),
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_HIDP >> 8),
    (uint8_t)(UUID_HIDP),

    //Attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0B,
    'R', 'e', 'a', 'l', 't', 'e', 'k', ' ', 'H', 'I', 'D',

    //Attribute SDP_ATTR_SRV_DESC
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x10,
    '(', '3', ')', ' ', 'B', 'u', 't', 't', 'o', 'n', ' ', 'M', 'o', 'u', 's', 'e',

    //Attribute SDP_ATTR_PROVIDER_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x07,
    'R', 'e', 'a', 'l', 't', 'e', 'k',

    //Attribute HIDParserVersion
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0201 >> 8),
    (uint8_t)(0x0201),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0111 >> 8),
    (uint8_t)(0x0111),

    //Attribute HIDDeviceSubclass
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0202 >> 8),
    (uint8_t)(0x0202),
    SDP_UNSIGNED_ONE_BYTE,
    0x80,

    //Attribute HIDCountryCode
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0203 >> 8),
    (uint8_t)(0x0203),
    SDP_UNSIGNED_ONE_BYTE,
    0x21,

    //Attribute HIDVirtualCable
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0204 >> 8),
    (uint8_t)(0x0204),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDReconnectInitiate
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0205 >> 8),
    (uint8_t)(0x0205),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDDescriptorList
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0206 >> 8),
    (uint8_t)(0x0206),
    SDP_DATA_ELEM_SEQ_HDR,
    //0x38,
    0x46,
    SDP_DATA_ELEM_SEQ_HDR,
    //0x36,
    0x44,
    SDP_UNSIGNED_ONE_BYTE,
    0x22,                   /* Type = Report Descriptor */
    SDP_STRING_HDR,
    //0x32,
    0x40,
    0x05, 0x01,             /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x02,             /* USAGE (Mouse) */
    0xA1, 0x01,             /* COLLECTION (Application) */
    0x85, 0x02,             /* REPORT_ID         (2)           */
    0x09, 0x01,             /*   USAGE (Pointer) */
    0xA1, 0x00,             /*   COLLECTION (Physical) */
    0x05, 0x09,             /*     USAGE_PAGE (Button) */
    0x19, 0x01,             /*     USAGE_MINIMUM (Button 1) */
    0x29, 0x03,             /*     USAGE_MAXIMUM (Button 3) */
    0x15, 0x00,             /*     LOGICAL_MINIMUM (0) */
    0x25, 0x01,             /*     LOGICAL_MAXIMUM (1) */
    0x95, 0x03,             /*     REPORT_COUNT (3) */
    0x75, 0x01,             /*     REPORT_SIZE (1) */
    0x81, 0x02,             /*     INPUT (Data,Var,Abs) */
    0x95, 0x01,             /*     REPORT_COUNT (1) */
    0x75, 0x05,             /*     REPORT_SIZE (5) */
    0x81, 0x03,             /*     INPUT (Cnst,Var,Abs) */
    0x05, 0x01,             /*     USAGE_PAGE (Generic Desktop) */
    0x09, 0x30,             /*     USAGE (X) */
    0x09, 0x31,             /*     USAGE (Y) */
    0x15, 0x81,             /*     LOGICAL_MINIMUM (-127) */
    0x25, 0x7f,             /*     LOGICAL_MAXIMUM (127) */
    0x75, 0x08,             /*     REPORT_SIZE (8) */
    0x95, 0x02,             /*     REPORT_COUNT (2) */
    0x81, 0x06,             /*     INPUT (Data,Var,Rel) */
    0x09, 0x38,             /*     USAGE (Wheel) */
    0x15, 0x81,             /*     LOGICAL_MINIMUM (-127) */
    0x25, 0x7f,             /*     LOGICAL_MAXIMUM (127) */
    0x75, 0x08,             /*     REPORT_SIZE (8) */
    0x95, 0x01,             /*     REPORT_COUNT (1) */
    0x81, 0x06,             /*     INPUT (Data,Var,Rel) */
    0xC0,                   /*   END_COLLECTION */
    0xC0,                   /* END_COLLECTION */

    //Attribute HIDLANGIDBaseList
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0207 >> 8),
    (uint8_t)(0x0207),
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0409 >> 8),
    (uint8_t)(0x0409),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0100 >> 8),
    (uint8_t)(0x0100),

    //Attribute HIDBatteryPower
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0209 >> 8),
    (uint8_t)(0x0209),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDRemoteWake
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x020A >> 8),
    (uint8_t)(0x020A),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDSupervisionTimeout
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x020C >> 8),
    (uint8_t)(0x020C),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x2580 >> 8),
    (uint8_t)(0x2580),

    //Attribute HIDNormallyConnectable
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x020D >> 8),
    (uint8_t)(0x020D),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDBootDevice
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x020E >> 8),
    (uint8_t)(0x020E),
    SDP_BOOL_ONE_BYTE,
    0x01,
};

const uint8_t hid_descriptor_mouse_boot_mode[] =
{
    0x05, 0x01,             /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x02,             /* USAGE (Mouse) */
    0xA1, 0x01,             /* COLLECTION (Application) */

    0x85, 0x02,             /* REPORT_ID         (2)           */
    0x09, 0x01,             /*   USAGE (Pointer) */
    0xA1, 0x00,             /*   COLLECTION (Physical) */

    0x05, 0x09,             /*     USAGE_PAGE (Button) */
    0x19, 0x01,             /*     USAGE_MINIMUM (Button 1) */
    0x29, 0x03,             /*     USAGE_MAXIMUM (Button 3) */
    0x15, 0x00,             /*     LOGICAL_MINIMUM (0) */
    0x25, 0x01,             /*     LOGICAL_MAXIMUM (1) */
    0x95, 0x03,             /*     REPORT_COUNT (3) */
    0x75, 0x01,             /*     REPORT_SIZE (1) */
    0x81, 0x02,             /*     INPUT (Data,Var,Abs) */
    0x95, 0x01,             /*     REPORT_COUNT (1) */
    0x75, 0x05,             /*     REPORT_SIZE (5) */
    0x81, 0x03,             /*     INPUT (Cnst,Var,Abs) */

    0x05, 0x01,             /*     USAGE_PAGE (Generic Desktop) */
    0x09, 0x30,             /*     USAGE (X) */
    0x09, 0x31,             /*     USAGE (Y) */
    0x15, 0x81,             /*     LOGICAL_MINIMUM (-127) */
    0x25, 0x7f,             /*     LOGICAL_MAXIMUM (127) */
    0x75, 0x08,             /*     REPORT_SIZE (8) */
    0x95, 0x02,             /*     REPORT_COUNT (2) */
    0x81, 0x06,             /*     INPUT (Data,Var,Rel) */
    0x09, 0x38,             /*     USAGE (Wheel) */
    0x15, 0x81,             /*     LOGICAL_MINIMUM (-127) */
    0x25, 0x7f,             /*     LOGICAL_MAXIMUM (127) */
    0x75, 0x08,             /*     REPORT_SIZE (8) */
    0x95, 0x01,             /*     REPORT_COUNT (1) */
    0x81, 0x06,             /*     INPUT (Data,Var,Rel) */

    0xC0,                   /*   END_COLLECTION */
    0xC0                    /* END_COLLECTION */
};
#endif

#if F_APP_HID_KEYBOARD_SUPPORT
static const uint8_t hid_keyboard_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR_2BYTE,
    0x01,
    0x59,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_HUMAN_INTERFACE_DEVICE_SERVICE >> 8),
    (uint8_t)(UUID_HUMAN_INTERFACE_DEVICE_SERVICE),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0D,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_HID_CONTROL >> 8),
    (uint8_t)(PSM_HID_CONTROL),
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_HIDP >> 8),
    (uint8_t)(UUID_HIDP),

    //Attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)(SDP_LANG_ENGLISH),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)(SDP_CHARACTER_UTF8),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)(SDP_BASE_LANG_OFFSET),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST),
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_HUMAN_INTERFACE_DEVICE_SERVICE >> 8),
    (uint8_t)(UUID_HUMAN_INTERFACE_DEVICE_SERVICE),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0101 >> 8),
    (uint8_t)(0x0101),

    //Attribute SDP_ATTR_ADD_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_ADD_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_ADD_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0F,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0D,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(0x0100 >> 8),
    (uint8_t)(0x0100),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_HID_INTERRUPT >> 8),
    (uint8_t)(PSM_HID_INTERRUPT),
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_HIDP >> 8),
    (uint8_t)(UUID_HIDP),

    //Attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0B,
    'R', 'e', 'a', 'l', 't', 'e', 'k', ' ', 'H', 'I', 'D',

    //Attribute SDP_ATTR_SRV_DESC
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x10,
    '(', '3', ')', ' ', 'B', 'u', 't', 't', 'o', 'n', ' ', 'M', 'o', 'u', 's', 'e',

    //Attribute SDP_ATTR_PROVIDER_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x07,
    'R', 'e', 'a', 'l', 't', 'e', 'k',

    //Attribute HIDParserVersion
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0201 >> 8),
    (uint8_t)(0x0201),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0111 >> 8),
    (uint8_t)(0x0111),

    //Attribute HIDDeviceSubclass
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0202 >> 8),
    (uint8_t)(0x0202),
    SDP_UNSIGNED_ONE_BYTE,
    0x40,

    //Attribute HIDCountryCode
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0203 >> 8),
    (uint8_t)(0x0203),
    SDP_UNSIGNED_ONE_BYTE,
    0x21,

    //Attribute HIDVirtualCable
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0204 >> 8),
    (uint8_t)(0x0204),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDReconnectInitiate
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0205 >> 8),
    (uint8_t)(0x0205),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDDescriptorList
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0206 >> 8),
    (uint8_t)(0x0206),
    SDP_DATA_ELEM_SEQ_HDR,
    //0x47,
    0x99,
    SDP_DATA_ELEM_SEQ_HDR,
    //0x45,
    0x97,
    SDP_UNSIGNED_ONE_BYTE,
    0x22,                   /* Type = Report Descriptor */
    SDP_STRING_HDR,
    0x93,
    0x05, 0x01,             /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x06,             /* USAGE (Keyboard) */
    0xA1, 0x01,             /* COLLECTION (Application) */
    0x85, 0x01,             /*     REPORT_ID         (1)           */
    0x95, 0x08,             /*     REPORT_COUNT (8) */
    0x75, 0x01,             /*     REPORT_SIZE (1) */
    0x05, 0x07,             /*     USAGE (Keyboard/Keypad) */
    0x19, 0xe0,             /*     USAGE_MINIMUM (Keyboard Left Control) */
    0x29, 0xe7,             /*     USAGE_MAXIMUM (Keyboard Right GUI) */
    0x15, 0x00,             /*     LOGICAL_MINIMUM (0) */
    0x25, 0x01,             /*     LOGICAL_MAXIMUM (1) */
    0x81, 0x02,             /*     INPUT (Data,Var,Abs) */
    0x95, 0x01,             /*     REPORT_COUNT (1) */
    0x75, 0x08,             /*     REPORT_SIZE (8) */
    0x81, 0x03,             /*     INPUT (Cnst,Var,Abs) */

    0x95, 0x05,             /*     REPORT_COUNT (5) */
    0x75, 0x01,             /*     REPORT_SIZE (1) */
    0x05, 0x08,             /*     USAGE_PAGE (LEDs) */
    0x19, 0x01,             /*     USAGE_MINIMUM (Num Lock) */
    0x29, 0x05,             /*     USAGE_MAXIMUM (Kana) */
    0x91, 0x02,             /*     OUTPUT (Data,Var,Abs) */
    0x95, 0x01,             /*     REPORT_COUNT (1) */
    0x75, 0x03,             /*     REPORT_SIZE (3) */
    0x91, 0x03,             /*     OUTPUT (Cnst,Var,Abs) */

    0x95, 0x06,             /*     REPORT_COUNT (6) */
    0x75, 0x08,             /*     REPORT_SIZE (8) */
    0x15, 0x00,             /*     LOGICAL_MINIMUM (0) */
    0x25, 0xff,             /*     LOGICAL_MAXIMUM (255) */
    0x05, 0x07,             /*     USAGE_PAGE (Keyboard/Keypad) */
    0x19, 0x00,             /*     USAGE_MINIMUM (Keyboard Return (ENTER)) */
    0x29, 0xff,             /*     USAGE_MAXIMUM (Keyboard ESCAPE) */
    0x81, 0x00,             /*     INPUT (Data,Arr,Abs) */
    0xC0,                   /* END_COLLECTION */

    0x05, 0x0c,             /* USAGE_PAGE (Consumer) */
    0x09, 0x01,             /* USAGE (Consumer Control) */
    0xA1, 0x01,             /* COLLECTION (Application) */
    0x85, 0x02,             /*     REPORT_ID         (2)           */
    0x95, 0x18,             /*     REPORT_COUNT (24) */
    0x75, 0x01,             /*     REPORT_SIZE (1) */
    0x15, 0x00,             /*     LOGICAL_MINIMUM (0) */
    0x25, 0x01,             /*     LOGICAL_MAXIMUM (1) */
    0x0a, 0x23, 0x02,       /*     USAGE (AL Home) */
    0x0a, 0x21, 0x02,       /*     USAGE (AC Search) */
    0x0a, 0x8a, 0x01,       /*     USAGE (AL Email Reader) */
    0x0a, 0x96, 0x01,       /*     USAGE (AL Internet Browser) */
    0x0a, 0x9e, 0x01,       /*     USAGE (AL Terminal Lock/Screensaver) */
    0x0a, 0x01, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x02, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x05, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x06, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x07, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x08, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x0a, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x0b, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0xae, 0x01,       /*     USAGE (AL Keyboard Layboard) */
    0x0a, 0xb1, 0x01,       /*     USAGE (AL Screen Saver) */
    0x09, 0x40,             /*     USAGE (Menu) */
    0x09, 0x30,             /*     USAGE (Power) */
    0x09, 0xb7,             /*     USAGE (Stop) */
    0x09, 0xb6,             /*     USAGE (Scan Previous Track) */
    0x09, 0xcd,             /*     USAGE (Play/Pause) */
    0x09, 0xb5,             /*     USAGE (Scan Next Track) */
    0x09, 0xe2,             /*     USAGE (Mute) */
    0x09, 0xea,             /*     USAGE (Volume Down) */
    0x09, 0xe9,             /*     USAGE (Volume Up) */
    0x81, 0x02,             /*     INPUT (Data,Var,Abs) */
    0xC0,                   /* END_COLLECTION */

    //Attribute HIDLANGIDBaseList
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0207 >> 8),
    (uint8_t)(0x0207),
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0409 >> 8),
    (uint8_t)(0x0409),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0100 >> 8),
    (uint8_t)(0x0100),

    //Attribute HIDBatteryPower
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0209 >> 8),
    (uint8_t)(0x0209),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDRemoteWake
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x020A >> 8),
    (uint8_t)(0x020A),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDSupervisionTimeout
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x020C >> 8),
    (uint8_t)(0x020C),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0C80 >> 8),
    (uint8_t)(0x0C80),

    //Attribute HIDNormallyConnectable
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x020D >> 8),
    (uint8_t)(0x020D),
    SDP_BOOL_ONE_BYTE,
    0x01,

    //Attribute HIDBootDevice
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x020E >> 8),
    (uint8_t)(0x020E),
    SDP_BOOL_ONE_BYTE,
    0x01,
};

const uint8_t hid_descriptor_keyboard_boot_mode[] =
{
    0x05, 0x01,             /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x06,             /* USAGE (Keyboard) */
    0xA1, 0x01,             /* COLLECTION (Application) */
    0x85, 0x01,             /*     REPORT_ID         (1)           */
    0x95, 0x08,             /*     REPORT_COUNT (8) */
    0x75, 0x01,             /*     REPORT_SIZE (1) */
    0x05, 0x07,             /*     USAGE (Keyboard/Keypad) */
    0x19, 0xe0,             /*     USAGE_MINIMUM (Keyboard Left Control) */
    0x29, 0xe7,             /*     USAGE_MAXIMUM (Keyboard Right GUI) */
    0x15, 0x00,             /*     LOGICAL_MINIMUM (0) */
    0x25, 0x01,             /*     LOGICAL_MAXIMUM (1) */
    0x81, 0x02,             /*     INPUT (Data,Var,Abs) */
    0x95, 0x01,             /*     REPORT_COUNT (1) */
    0x75, 0x08,             /*     REPORT_SIZE (8) */
    0x81, 0x03,             /*     INPUT (Cnst,Var,Abs) */

    0x95, 0x05,             /*     REPORT_COUNT (5) */
    0x75, 0x01,             /*     REPORT_SIZE (1) */
    0x05, 0x08,             /*     USAGE_PAGE (LEDs) */
    0x19, 0x01,             /*     USAGE_MINIMUM (Num Lock) */
    0x29, 0x05,             /*     USAGE_MAXIMUM (Kana) */
    0x91, 0x02,             /*     OUTPUT (Data,Var,Abs) */
    0x95, 0x01,             /*     REPORT_COUNT (1) */
    0x75, 0x03,             /*     REPORT_SIZE (3) */
    0x91, 0x03,             /*     OUTPUT (Cnst,Var,Abs) */

    0x95, 0x06,             /*     REPORT_COUNT (6) */
    0x75, 0x08,             /*     REPORT_SIZE (8) */
    0x15, 0x00,             /*     LOGICAL_MINIMUM (0) */
    0x25, 0xff,             /*     LOGICAL_MAXIMUM (255) */
    0x05, 0x07,             /*     USAGE_PAGE (Keyboard/Keypad) */
    0x19, 0x00,             /*     USAGE_MINIMUM (Keyboard Return (ENTER)) */
    0x29, 0xff,             /*     USAGE_MAXIMUM (Keyboard ESCAPE) */
    0x81, 0x00,             /*     INPUT (Data,Arr,Abs) */
    0xC0,                   /* END_COLLECTION */

    0x05, 0x0c,             /* USAGE_PAGE (Consumer) */
    0x09, 0x01,             /* USAGE (Consumer Control) */
    0xA1, 0x01,             /* COLLECTION (Application) */
    0x85, 0x02,             /*     REPORT_ID         (2)           */
    0x95, 0x18,             /*     REPORT_COUNT (24) */
    0x75, 0x01,             /*     REPORT_SIZE (1) */
    0x15, 0x00,             /*     LOGICAL_MINIMUM (0) */
    0x25, 0x01,             /*     LOGICAL_MAXIMUM (1) */
    0x0a, 0x23, 0x02,       /*     USAGE (AL Home) */
    0x0a, 0x21, 0x02,       /*     USAGE (AC Search) */
    0x0a, 0x8a, 0x01,       /*     USAGE (AL Email Reader) */
    0x0a, 0x96, 0x01,       /*     USAGE (AL Internet Browser) */
    0x0a, 0x9e, 0x01,       /*     USAGE (AL Terminal Lock/Screensaver) */
    0x0a, 0x01, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x02, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x05, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x06, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x07, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x08, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x0a, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0x0b, 0x03,       /*     USAGE (Reserved) */
    0x0a, 0xae, 0x01,       /*     USAGE (AL Keyboard Layboard) */
    0x0a, 0xb1, 0x01,       /*     USAGE (AL Screen Saver) */
    0x09, 0x40,             /*     USAGE (Menu) */
    0x09, 0x30,             /*     USAGE (Power) */
    0x09, 0xb7,             /*     USAGE (Stop) */
    0x09, 0xb6,             /*     USAGE (Scan Previous Track) */
    0x09, 0xcd,             /*     USAGE (Play/Pause) */
    0x09, 0xb5,             /*     USAGE (Scan Next Track) */
    0x09, 0xe2,             /*     USAGE (Mute) */
    0x09, 0xea,             /*     USAGE (Volume Down) */
    0x09, 0xe9,             /*     USAGE (Volume Up) */
    0x81, 0x02,             /*     INPUT (Data,Var,Abs) */
    0xC0,                   /* END_COLLECTION */
};
#endif

#endif

uint8_t *hid_data = NULL;
uint8_t  data[100] = {0};
uint16_t data_len = 0;
T_BT_HID_DEVICE_REPORT_TYPE report_type = BT_HID_DEVICE_REPORT_TYPE_RESERVED;
extern uint8_t *p_descriptor;
extern uint32_t descriptor_len;

static void app_bt_hid_demo_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BT_HID_DEMO_LINK *p_link;

    switch (event_type)
    {
    case BT_EVENT_HID_DEVICE_CONN_IND:
        p_link = app_bt_hid_demo_find_link(param->hid_device_conn_ind.bd_addr);
        if (p_link != NULL)
        {
            bt_hid_device_connect_cfm(p_link->bd_addr, true);
        }
        break;

    case BT_EVENT_HID_DEVICE_CONN_CMPL:
        {
            p_link = app_bt_hid_demo_find_link(param->hid_device_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                char *temp_buff = "HID Connected!\r\n";

                p_link->hid_conn_cmpl = true;
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    case BT_EVENT_HID_DEVICE_DISCONN_CMPL:
        {
            p_link = app_bt_hid_demo_find_link(param->hid_device_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                char *temp_buff = "HID Disconnected!\r\n";

                p_link->hid_conn_cmpl = false;
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    case BT_EVENT_HID_DEVICE_CONTROL_DATA_IND:
        break;

    case BT_EVENT_HID_DEVICE_GET_REPORT_IND:
        {
            data_len = 1 + param->hid_device_get_report_ind.report_size;
            hid_data = os_mem_zalloc(RAM_TYPE_DATA_ON, data_len);
            hid_data[0] = param->hid_device_get_report_ind.report_id;
            report_type = (T_BT_HID_DEVICE_REPORT_TYPE)param->hid_device_get_report_ind.report_type;
            memcpy(&hid_data[1], data, param->hid_device_get_report_ind.report_size);
        }
        break;

    case BT_EVENT_HID_DEVICE_SET_REPORT_IND:
        break;

    case BT_EVENT_HID_DEVICE_GET_PROTOCOL_IND:
        break;

    case BT_EVENT_HID_DEVICE_SET_PROTOCOL_IND:
        break;

    case BT_EVENT_HID_DEVICE_SET_IDLE_IND:
        break;

    case BT_EVENT_HID_DEVICE_INTERRUPT_DATA_IND:
        break;

    case BT_EVENT_HID_HOST_CONN_IND:
        p_link = app_bt_hid_demo_find_link(param->hid_host_conn_ind.bd_addr);
        if (p_link != NULL)
        {
            bt_hid_host_connect_cfm(p_link->bd_addr, true);
        }
        break;

    case BT_EVENT_HID_HOST_CONN_CMPL:
        {
            p_link = app_bt_hid_demo_find_link(param->hid_host_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                char *temp_buff = "HID HOST Connected!\r\n";
                T_HID_HOST_DESCRIPTOR *hid_descriptor;

                p_link->hid_conn_cmpl = true;
                hid_descriptor = app_hid_host_descriptor_find(param->hid_host_conn_cmpl.bd_addr);
                if (hid_descriptor != NULL)
                {
                    bt_hid_host_descriptor_set(param->hid_host_conn_cmpl.bd_addr,
                                               hid_descriptor->descriptor,
                                               hid_descriptor->descriptor_len);
                }
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    case BT_EVENT_HID_HOST_CONN_FAIL:
        {
            T_HID_HOST_DESCRIPTOR *hid_descriptor;

            hid_descriptor = app_hid_host_descriptor_find(param->hid_host_conn_fail.bd_addr);
            if (hid_descriptor != NULL)
            {
                app_hid_host_free_descriptor(hid_descriptor);
            }
        }
        break;

    case BT_EVENT_HID_HOST_INTERRUPT_DATA_IND:
        {
            uint16_t               report_size;
            uint8_t                report_type;
            uint16_t               report_len;
            uint8_t               *report;
            T_HID_HOST_DESCRIPTOR *hid_descriptor;

            report_size = param->hid_host_interrupt_data_ind.report_size;
            report_type = (T_BT_HID_HOST_REPORT_TYPE)param->hid_host_interrupt_data_ind.report_type;
            hid_descriptor = app_hid_host_descriptor_find(param->hid_host_interrupt_data_ind.bd_addr);
            if (hid_descriptor != NULL)
            {
                if (param->hid_host_interrupt_data_ind.report_id != 0)
                {
                    report_len = 1 + report_size;
                    report = calloc(1, report_len);
                    report[0] = param->hid_host_interrupt_data_ind.report_id;
                    memcpy(&report[1], param->hid_host_interrupt_data_ind.p_data, report_size);
                }
                else
                {
                    report_len = report_size;
                    report = calloc(1, report_len);
                    memcpy(report, param->hid_host_interrupt_data_ind.p_data, report_size);
                }

                if (report_type == BT_HID_HOST_REPORT_TYPE_INPUT)
                {
                    app_bt_hid_demo_host_handle_interrupt_report(hid_descriptor, report, report_len);
                }

                free(report);
            }
        }
        break;

    case BT_EVENT_HID_HOST_DISCONN_CMPL:
        {
            p_link = app_bt_hid_demo_find_link(param->hid_host_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                char *temp_buff = "HID HOST Disconnected!\r\n";

                p_link->hid_conn_cmpl = false;
                if (param->hid_host_disconn_cmpl.virtual_unplug)
                {
                    T_HID_HOST_DESCRIPTOR *hid_descriptor;

                    hid_descriptor = app_hid_host_descriptor_find(param->hid_host_disconn_cmpl.bd_addr);
                    if (hid_descriptor != NULL)
                    {
                        app_hid_host_free_descriptor(hid_descriptor);
                    }
                }
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    default:
        break;
    }
}

void app_bt_hid_demo_init(void)
{
    app_bt_hid_demo_db_init();

#if (BT_HID_DEMO_ROLE == 1)
    if (bt_hid_host_init(true) == false)
    {
        APP_PRINT_ERROR0("app_bt_hid_demo_init: failed");
    }

    os_queue_init(&descriptor_list);
    bt_mgr_cback_register(app_bt_hid_demo_cback);

    return;
#endif
#if (BT_HID_DEMO_ROLE == 2)
    int32_t ret = 0;
#if F_APP_HID_MOUSE_SUPPORT
    if (bt_sdp_record_add((void *)hid_mouse_sdp_record) == false)
    {
        ret = -1;
        goto fail_sdp_add;
    }
#endif

#if F_APP_HID_KEYBOARD_SUPPORT
    if (bt_sdp_record_add((void *)hid_keyboard_sdp_record) == false)
    {
        ret = -1;
        goto fail_sdp_add;
    }
#endif

    if (bt_hid_device_init(true) == false)
    {
        ret = -2;
        goto fail_init;
    }
#if F_APP_HID_MOUSE_SUPPORT
    bt_hid_device_descriptor_set(hid_descriptor_mouse_boot_mode,
                                 sizeof(hid_descriptor_mouse_boot_mode));
#endif

#if F_APP_HID_KEYBOARD_SUPPORT
    bt_hid_device_descriptor_set(hid_descriptor_keyboard_boot_mode,
                                 sizeof(hid_descriptor_keyboard_boot_mode));
#endif
    bt_mgr_cback_register(app_bt_hid_demo_cback);

    return;

fail_init:
fail_sdp_add:
    APP_PRINT_ERROR1("app_bt_hid_demo_init: failed %d", ret);
#endif
}

bool app_bt_hid_demo_connect(uint8_t *bd_addr)
{
    return bt_hid_device_connect_req(bd_addr);
}

bool app_bt_hid_demo_disconnect(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;

    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_hid_device_disconnect_req(bd_addr);
    }

    return false;
}

bool app_bt_hid_demo_get_report_rsp(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;

    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_hid_device_get_report_rsp(bd_addr,
                                            report_type,
                                            hid_data,
                                            data_len);
    }

    return false;
}

bool app_bt_hid_demo_mouse_shift_left(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;
    static int dx;
    static int dy;
    static uint8_t buttons;
    static const int MOUSE_SPEED = 10;
    dx -= MOUSE_SPEED;
    uint8_t report[] = {0x02, buttons, (uint8_t) dx, (uint8_t) dy, 0};
    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        char *temp_buff = "HID mouse shift left!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));

        return bt_hid_device_interrupt_data_send(bd_addr, BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                                 &report[0], sizeof(report));
    }

    return false;
}

bool app_bt_hid_demo_mouse_shift_right(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;
    static int dx;
    static int dy;
    static uint8_t buttons;
    static const int MOUSE_SPEED = 10;
    dx += MOUSE_SPEED;
    uint8_t report[] = {0x02, buttons, (uint8_t) dx, (uint8_t) dy, 0};
    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        char *temp_buff = "HID mouse shift right!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));

        return bt_hid_device_interrupt_data_send(bd_addr, BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                                 &report[0], sizeof(report));
    }

    return false;
}

bool app_bt_hid_demo_mouse_shift_up(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;
    static int dx;
    static int dy;
    static uint8_t buttons;
    static const int MOUSE_SPEED = 10;
    dy -= MOUSE_SPEED;
    uint8_t report[] = {0x02, buttons, (uint8_t) dx, (uint8_t) dy, 0};
    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        char *temp_buff = "HID mouse shift up!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));

        return bt_hid_device_interrupt_data_send(bd_addr, BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                                 &report[0], sizeof(report));
    }

    return false;
}

bool app_bt_hid_demo_mouse_shift_down(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;
    static int dx;
    static int dy;
    static uint8_t buttons;
    static const int MOUSE_SPEED = 10;
    dy += MOUSE_SPEED;
    uint8_t report[] = {0x02, buttons, (uint8_t) dx, (uint8_t) dy, 0};
    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        char *temp_buff = "HID mouse shift down!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));

        return bt_hid_device_interrupt_data_send(bd_addr, BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                                 &report[0], sizeof(report));
    }

    return false;
}

bool app_bt_hid_demo_mouse_click(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;
    static int dx;
    static int dy;
    static uint8_t buttons;
    buttons |= 1;
    uint8_t report1[] = {0x02, buttons, (uint8_t) dx, (uint8_t) dy, 0};
    buttons &= 0;
    uint8_t report2[] = {0x02, buttons, (uint8_t) dx, (uint8_t) dy, 0};
    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        char *temp_buff = "HID mouse click!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));

        if (bt_hid_device_interrupt_data_send(bd_addr, BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                              &report1[0], sizeof(report1)) ==  false)
        {
            return false;
        }
        if (bt_hid_device_interrupt_data_send(bd_addr, BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                              &report2[0], sizeof(report2)) ==  false)
        {
            return false;
        }
        return true;
    }

    return false;
}

bool app_bt_hid_demo_sniff(uint8_t *bd_addr, uint16_t interval)
{
    T_APP_BT_HID_DEMO_LINK *p_link;
    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        if (bt_sniff_mode_enable(bd_addr, interval, interval, 0, 0) ==  false)
        {
            return false;
        }
        return true;
    }

    return false;
}

bool app_bt_hid_demo_enable_dlps(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;
    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        power_mode_set(POWER_DLPS_MODE);
        return true;
    }

    return false;
}

bool app_bt_hid_demo_keyboard_click_keycode(uint8_t *bd_addr, uint8_t keycode)
{
    int i;
    T_APP_BT_HID_DEMO_LINK *p_link;
    p_link = app_bt_hid_demo_find_link(bd_addr);
    uint8_t key_press[] = {0x01, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t key_release[] = {0x01, 0, 0, 0, 0, 0, 0, 0, 0};

    if (p_link == NULL)
    {
        APP_PRINT_INFO0("Wendy app_bt_hid_demo_keyboard_click_keycode link is NULL");
    }

    for (i = 0; i < sizeof(keytable_us_none); i++)
    {
        if (keytable_us_none[i] == keycode)
        {
            key_press[3] = i;
            break;
        }
    }
    if (i == sizeof(keytable_us_none))
    {
        return false;
    }

    if (p_link != NULL)
    {
        char *temp_buff = "HID keyboard click keycode!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));

        if (bt_hid_device_interrupt_data_send(bd_addr, BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                              &key_press[0], sizeof(key_press)) ==  false)
        {
            return false;
        }
        if (bt_hid_device_interrupt_data_send(bd_addr, BT_HID_DEVICE_REPORT_TYPE_INPUT,
                                              &key_release[0], sizeof(key_release)) ==  false)
        {
            return false;
        }
        return true;
    }

    return false;
}


T_HID_HOST_DESCRIPTOR *app_hid_host_alloc_descriptor(uint8_t   bd_addr[6],
                                                     uint8_t  *descriptor,
                                                     uint16_t  descriptor_len)
{
    T_HID_HOST_DESCRIPTOR *hid_descriptor;

    hid_descriptor = malloc(sizeof(T_HID_HOST_DESCRIPTOR) + descriptor_len);
    if (hid_descriptor != NULL)
    {
        hid_descriptor->descriptor_len = descriptor_len;
        memcpy(hid_descriptor->descriptor, descriptor, descriptor_len);
        memcpy(hid_descriptor->bd_addr, bd_addr, 6);
        os_queue_in(&descriptor_list, hid_descriptor);
    }

    return hid_descriptor;
}

T_HID_HOST_DESCRIPTOR *app_hid_host_descriptor_find(uint8_t *bd_addr)
{
    T_HID_HOST_DESCRIPTOR *hid_descriptor;

    hid_descriptor = os_queue_peek(&descriptor_list, 0);
    while (hid_descriptor != NULL)
    {
        if (!memcmp(hid_descriptor->bd_addr, bd_addr, 6))
        {
            break;
        }

        hid_descriptor = hid_descriptor->next;
    }

    return hid_descriptor;
}

void app_hid_host_free_descriptor(T_HID_HOST_DESCRIPTOR *hid_descriptor)
{
    if (os_queue_delete(&descriptor_list, hid_descriptor) == true)
    {
        free(hid_descriptor);
    }
}

bool app_bt_hid_demo_host_connect(uint8_t *bd_addr)
{
    T_BT_SDP_UUID_DATA uuid;

    uuid.uuid_16 = UUID_HUMAN_INTERFACE_DEVICE_SERVICE;

    return bt_sdp_discov_start(bd_addr, BT_SDP_UUID16, uuid);
}

bool app_bt_hid_demo_host_disconnect(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;

    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_hid_host_disconnect_req(bd_addr);
    }

    return false;
}

bool app_bt_hid_demo_host_bond_delete(uint8_t *bd_addr)
{
    return bt_bond_delete(bd_addr);
}

bool app_bt_hid_demo_host_virtual_cable_unplug(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *p_link;

    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_hid_host_control_req(bd_addr, BT_HID_HOST_CONTROL_OPERATION_VIRTUAL_CABLE_UNPLUG);
    }

    return false;
}

bool app_bt_hid_demo_host_get_report(uint8_t *bd_addr,
                                     uint8_t  report_id)
{
    T_APP_BT_HID_DEMO_LINK *p_link;

    T_BT_HID_HOST_REPORT_TYPE report_type = BT_HID_HOST_REPORT_TYPE_RESERVED;
    if (report_id == 2)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_INPUT;
    }
    else if (report_id == 3)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_OUTPUT;
    }
    else if (report_id == 5)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_FEATURE;
    }

    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_hid_host_get_report_req(bd_addr,
                                          report_type,
                                          report_id,
                                          0);
    }

    return false;
}

bool app_bt_hid_demo_host_set_report(uint8_t *bd_addr,
                                     uint8_t  report_id)
{
    T_APP_BT_HID_DEMO_LINK *p_link;

    uint8_t report[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    T_BT_HID_HOST_REPORT_TYPE report_type = BT_HID_HOST_REPORT_TYPE_RESERVED;
    uint8_t report_len = 0;
    report[0] = report_id;

    if (report_id == 1)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_OUTPUT;
        //report[1] = 1;
        report_len = 2;
    }
    if (report_id == 2)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_INPUT;
        report_len = 5;
    }
    else if (report_id == 3)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_OUTPUT;
        report_len = 3;
    }
    else if (report_id == 5)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_FEATURE;
        report_len = 4;
    }


    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_hid_host_set_report_req(bd_addr,
                                          report_type,
                                          report,
                                          report_len);
    }

    return false;
}

bool app_bt_hid_demo_host_set_protocol(uint8_t *bd_addr,
                                       uint8_t  proto_mode)
{
    T_APP_BT_HID_DEMO_LINK *p_link;

    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_hid_host_set_protocol_req(bd_addr,
                                            (T_BT_HID_HOST_PROTOCOL_MODE)proto_mode);
    }

    return false;
}

bool app_bt_hid_demo_host_interrupt_data_send(uint8_t *bd_addr,
                                              uint8_t  report_id)
{
    T_APP_BT_HID_DEMO_LINK *p_link;

    uint8_t report[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    T_BT_HID_HOST_REPORT_TYPE report_type = BT_HID_HOST_REPORT_TYPE_RESERVED;
    uint8_t report_len = 0;
    report[0] = report_id;

    if (report_id == 1)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_OUTPUT;
        //report[1] = 1;
        report_len = 2;
    }
    if (report_id == 2)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_INPUT;
        report_len = 5;
    }
    else if (report_id == 3)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_OUTPUT;
        report_len = 3;
    }
    else if (report_id == 5)
    {
        report_type = BT_HID_HOST_REPORT_TYPE_FEATURE;
        report_len = 4;
    }

    p_link = app_bt_hid_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_hid_host_interrupt_data_send(bd_addr,
                                               report_type,
                                               report,
                                               report_len);
    }

    return false;
}

#define NUM_KEYS 6
static uint8_t last_keys[NUM_KEYS];
void app_bt_hid_demo_host_handle_interrupt_report(T_HID_HOST_DESCRIPTOR *hid_descriptor,
                                                  const uint8_t         *report,
                                                  uint16_t               report_len)
{
    T_BT_HID_PARSER parser;
    int             shift = 0;
    uint8_t         new_keys[NUM_KEYS];
    int             new_keys_count = 0;

    memset(new_keys, 0, sizeof(new_keys));
    bt_hid_parser_init(&parser,
                       hid_descriptor->descriptor,
                       hid_descriptor->descriptor_len,
                       BT_HID_REPORT_TYPE_INPUT,
                       report,
                       report_len);
    while (bt_hid_parser_has_more(&parser))
    {
        uint16_t usage_page;
        uint16_t usage;
        int32_t  value;

        bt_hid_parser_access_report_field(&parser,
                                          &usage_page,
                                          &usage,
                                          &value);
        if (usage_page != 0x07) { continue; }
        switch (usage)
        {
        case 0xe1:
        case 0xe6:
            if (value)
            {
                shift = 1;
            }
            continue;
        case 0x00:
            continue;
        default:
            break;
        }
        if (usage >= sizeof(keytable_us_none)) { continue; }

        // store new keys
        new_keys[new_keys_count++] = (uint8_t) usage;

        // check if usage was used last time (and ignore in that case)
        int i;
        for (i = 0; i < NUM_KEYS; i++)
        {
            if (usage == last_keys[i])
            {
                usage = 0;
            }
        }
        if (usage == 0) { continue; }

        uint8_t key;
        if (shift)
        {
            key = keytable_us_shift[usage];
        }
        else
        {
            key = keytable_us_none[usage];
        }
        if (key == CHAR_ILLEGAL) { continue; }
        if (key == CHAR_BACKSPACE)
        {
            // go back one char, print space, go back one char again
            APP_PRINT_INFO0("app_bt_hid_demo_host_handle_interrupt_report \b \b");
            continue;
        }
        APP_PRINT_INFO1("app_bt_hid_demo_host_handle_interrupt_report %c", key);
        char temp_buff[] = " \r\n";
        temp_buff[0] = (char)key;
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }
    memcpy(last_keys, new_keys, NUM_KEYS);
}
