/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "bt_types.h"
#include "os_mem.h"
#include "app_msg.h"
#include "app_console_msg.h"
#include "app_bt_hid_demo_console.h"
#include "app_bt_hid_demo.h"

void app_console_handle_msg(T_IO_MSG console_msg)
{
    uint16_t  subtype;
    uint16_t  id;
    uint16_t  action;
    uint8_t  *p;

    p       = console_msg.u.buf;
    subtype = console_msg.subtype;

    switch (subtype)
    {
    case IO_MSG_CONSOLE_STRING_RX:
        LE_STREAM_TO_UINT16(id, p);
        LE_STREAM_TO_UINT8(action, p);

        if (id == GAP_LEGACY_ID)
        {

        }
        else if (id == HID_ID)
        {
            switch (action)
            {
            case BT_HID_DEMO_ACTION_HID_CONNECT:
                app_bt_hid_demo_connect(p);
                break;
            case BT_HID_DEMO_ACTION_HID_DISCONNECT:
                app_bt_hid_demo_disconnect(p);
                break;
            case BT_HID_DEMO_ACTION_HID_SHIFT_LEFT:
                app_bt_hid_demo_mouse_shift_left(p);
                break;
            case BT_HID_DEMO_ACTION_HID_SHIFT_RIGHT:
                app_bt_hid_demo_mouse_shift_right(p);
                break;
            case BT_HID_DEMO_ACTION_HID_SHIFT_UP:
                app_bt_hid_demo_mouse_shift_up(p);
                break;
            case BT_HID_DEMO_ACTION_HID_SHIFT_DOWN:
                app_bt_hid_demo_mouse_shift_down(p);
                break;
            case BT_HID_DEMO_ACTION_HID_CLICK:
                app_bt_hid_demo_mouse_click(p);
                break;
            case BT_HID_DEMO_ACTION_HID_SNIFF:
                {
                    uint16_t interval;

                    LE_STREAM_TO_UINT16(interval, p);

                    app_bt_hid_demo_sniff(p, interval);
                }
                break;

            case BT_HID_DEMO_ACTION_HID_ENABLE_DLPS:
                {
                    app_bt_hid_demo_enable_dlps(p);
                }
                break;

            case BT_HID_DEMO_ACTION_HID_CLICK_KEYCODE:
                {
                    uint8_t keycode;

                    LE_STREAM_TO_UINT8(keycode, p);

                    app_bt_hid_demo_keyboard_click_keycode(p, keycode);
                }
                break;

            case BT_HID_DEMO_ACTION_HID_HOST_CONNECT:
                app_bt_hid_demo_host_connect(p);
                break;

            case BT_HID_DEMO_ACTION_HID_HOST_DISCONNECT:
                app_bt_hid_demo_host_disconnect(p);
                break;

            case BT_HID_DEMO_ACTION_HID_HOST_BOND_DELETE:
                app_bt_hid_demo_host_bond_delete(p);
                break;

            case BT_HID_DEMO_ACTION_HID_HOST_VIRTUAL_CABLE_UNPLUG:
                app_bt_hid_demo_host_virtual_cable_unplug(p);
                break;
            case BT_HID_DEMO_ACTION_HID_HOST_GET_REPORT:
                {
                    uint8_t report_id;

                    LE_STREAM_TO_UINT8(report_id, p);

                    app_bt_hid_demo_host_get_report(p, report_id);
                }
                break;
            case BT_HID_DEMO_ACTION_HID_HOST_SET_REPORT:
                {
                    uint8_t  report_id;

                    LE_STREAM_TO_UINT8(report_id, p);

                    app_bt_hid_demo_host_set_report(p, report_id);
                }
                break;
            case BT_HID_DEMO_ACTION_HID_HOST_SET_PROTOCOL:
                {
                    uint8_t  proto_mode;

                    LE_STREAM_TO_UINT8(proto_mode, p);

                    app_bt_hid_demo_host_set_protocol(p, proto_mode);
                }
                break;
            case BT_HID_DEMO_ACTION_HID_HOST_INTERRUPT_DATA_SEND:
                {
                    uint8_t  report_id;

                    LE_STREAM_TO_UINT8(report_id, p);

                    app_bt_hid_demo_host_interrupt_data_send(p, report_id);
                }
                break;

            default:
                break;
            }
        }

        os_mem_free(console_msg.u.buf);
        break;

    default:
        break;
    }
}
