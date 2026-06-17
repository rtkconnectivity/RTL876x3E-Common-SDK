/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _BT_PAN_DEMO_CONSOLE_H_
#define _BT_PAN_DEMO_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_PAN_DEMO_CONSOLE BT PAN Demo Console
* @brief BT PAN Demo command
* @{
*/

typedef enum
{
    GAP_ID = 0x0001,
    PAN_ID = 0x0002,
} T_BT_PAN_DEMO_CMD_ID;

#define BT_PAN_DEMO_ACTION_PAN_CONNECT                    0x01
#define BT_PAN_DEMO_ACTION_PAN_DISCONNECT                 0x02
#define BT_PAN_DEMO_ACTION_PAN_SETUP_CONN_RSP             0x03
#define BT_PAN_DEMO_ACTION_PAN_NET_TYPE_FILTER_SET        0x04
#define BT_PAN_DEMO_ACTION_PAN_MULTI_ADDR_FILTER_SET      0x05
#define BT_PAN_DEMO_ACTION_PAN_SNED_GENERAL               0x06
#define BT_PAN_DEMO_ACTION_PAN_SNED_COMPRESSED            0x07
#define BT_PAN_DEMO_ACTION_PAN_SNED_SRC_ONLY              0x08
#define BT_PAN_DEMO_ACTION_PAN_SNED_DST_ONLY              0x09
#define BT_PAN_DEMO_ACTION_PAN_ARP                        0x0a
#define BT_PAN_DEMO_ACTION_PAN_ARP_V6                     0x0b
#define BT_PAN_DEMO_ACTION_PAN_PING                       0x0c
#define BT_PAN_DEMO_ACTION_PAN_PING_V6                    0x0d
#define BT_PAN_DEMO_ACTION_PAN_PING_REPLY                 0x0e
#define BT_PAN_DEMO_ACTION_PAN_PING_REPLY_V6              0x0f
#define BT_PAN_DEMO_ACTION_HTTP                           0x10
#define BT_PAN_DEMO_ACTION_SOCKETS                        0x11
#define BT_PAN_DEMO_ACTION_SEM                            0x12
#define BT_PAN_DEMO_ACTION_HTTPS_CLIENT                   0x13


void app_bt_pan_demo_cmd_register(void);

/** @} End of APP_BT_PAN_DEMO_CONSOLE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_PAN_DEMO_CONSOLE_H_ */
