/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_BT_HID_DEVICE_SUPPORT
#include "trace.h"
#include "btm.h"
#include "bt_hid_device.h"
#include "app_hid_cfg.h"
#include "stdlib.h"
#include "string.h"
#include "app_link_util_cs.h"
#include "app_hid.h"

#define REPORT_ID_USB_UTILITY_1 (42)
#define REPORT_ID_USB_UTILITY_3 (44)
#define REPORT_ID_USB_UTILITY_4 (45)

#define REPORT_ID_USB_UTILITY_5 (0x24)
#define REPORT_ID_USB_UTILITY_6 (0x27)

#define REPORT_ID_USB_UTILITY_7 (0x39)


#if F_APP_HID_MOUSE_SUPPORT
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


static void app_hid_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_HID_DEVICE_CONN_IND:
        {
            p_link = app_link_find_br_link(param->hid_device_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_hid_device_connect_cfm(p_link->bd_addr, true);
            }
        }
        break;

    case BT_EVENT_HID_DEVICE_CONN_CMPL:
        {

        }
        break;

    case BT_EVENT_HID_DEVICE_CONTROL_DATA_IND:
        {

        }
        break;

    case BT_EVENT_HID_DEVICE_GET_REPORT_IND:
        {
            p_link = app_link_find_br_link(param->hid_device_get_report_ind.bd_addr);

            if (p_link != NULL)
            {
                uint8_t *temp_buf;
                uint16_t len;
                len = param->hid_device_get_report_ind.report_size + 1;
                temp_buf = malloc(len);
                memset(temp_buf, 0, len);
                temp_buf[0] = (uint8_t)(param->hid_device_get_report_ind.report_id);
                bt_hid_device_get_report_rsp(param->hid_device_get_report_ind.bd_addr,
                                             (T_BT_HID_DEVICE_REPORT_TYPE)param->hid_device_get_report_ind.report_type,
                                             temp_buf, len);
            }
        }
        break;

    case BT_EVENT_HID_DEVICE_SET_REPORT_IND:
        {
        }
        break;

    case BT_EVENT_HID_DEVICE_GET_PROTOCOL_IND:
        {

        }
        break;

    case BT_EVENT_HID_DEVICE_SET_PROTOCOL_IND:
        {

        }
        break;

    case BT_EVENT_HID_DEVICE_SET_IDLE_IND:
        {
        }
        break;

    case BT_EVENT_HID_DEVICE_INTERRUPT_DATA_IND:
        {

        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_hid_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_hid_init(void)
{
    if (app_cfg_const.supported_profile_mask & HID_PROFILE_MASK)
    {
        bt_hid_device_init(true);
#if F_APP_HID_MOUSE_SUPPORT
        bt_hid_device_descriptor_set(hid_descriptor_mouse_boot_mode,
                                     sizeof(hid_descriptor_mouse_boot_mode));
#endif
#if F_APP_HID_KEYBOARD_SUPPORT
        bt_hid_device_descriptor_set(hid_descriptor_keyboard_boot_mode,
                                     sizeof(hid_descriptor_keyboard_boot_mode));
#endif

        bt_mgr_cback_register(app_hid_bt_cback);
    }
}
#endif
