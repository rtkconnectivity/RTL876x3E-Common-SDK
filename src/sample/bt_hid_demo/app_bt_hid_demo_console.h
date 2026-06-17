/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _BT_HID_DEMO_CONSOLE_H_
#define _BT_HID_DEMO_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_HID_DEMO_CONSOLE BT HID Demo Console
* @brief BT HID Demo command
* @{
*/

typedef enum
{
    GAP_LEGACY_ID         = 0x0001,
    HID_ID                = 0x0002,
} T_BT_HID_DEMO_CMD_ID;

#define BT_HID_DEMO_ACTION_HID_CONNECT                    0x01
#define BT_HID_DEMO_ACTION_HID_DISCONNECT                 0x02
#define BT_HID_DEMO_ACTION_HID_SHIFT_LEFT                 0x03
#define BT_HID_DEMO_ACTION_HID_SHIFT_RIGHT                0x04
#define BT_HID_DEMO_ACTION_HID_SHIFT_UP                   0x05
#define BT_HID_DEMO_ACTION_HID_SHIFT_DOWN                 0x06
#define BT_HID_DEMO_ACTION_HID_CLICK                      0x07
#define BT_HID_DEMO_ACTION_HID_SNIFF                      0x08
#define BT_HID_DEMO_ACTION_HID_ENABLE_DLPS                0x09
#define BT_HID_DEMO_ACTION_HID_CLICK_KEYCODE              0x0a

#define BT_HID_DEMO_ACTION_HID_HOST_CONNECT               0x11
#define BT_HID_DEMO_ACTION_HID_HOST_DISCONNECT            0x12
#define BT_HID_DEMO_ACTION_HID_HOST_BOND_DELETE           0x13
#define BT_HID_DEMO_ACTION_HID_HOST_VIRTUAL_CABLE_UNPLUG  0x14
#define BT_HID_DEMO_ACTION_HID_HOST_GET_REPORT            0x15
#define BT_HID_DEMO_ACTION_HID_HOST_SET_REPORT            0x16
#define BT_HID_DEMO_ACTION_HID_HOST_SET_PROTOCOL          0x17
#define BT_HID_DEMO_ACTION_HID_HOST_INTERRUPT_DATA_SEND   0x18

void app_bt_hid_demo_cmd_register(void);

/** @} End of APP_BT_HID_DEMO_CONSOLE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_HID_DEMO_CONSOLE_H_ */
