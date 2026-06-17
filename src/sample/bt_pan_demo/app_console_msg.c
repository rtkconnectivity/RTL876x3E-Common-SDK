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
#include "app_bt_pan_demo.h"
#include "app_bt_pan_demo_link.h"
#include "app_bt_pan_demo_console.h"
#include "pan_test/ping.h"
#include "pan_test/http.h"
#include "pan_test/sockets.h"
#include "pan_test/sem.h"
#include "pan_test/https/https_demo.h"


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

        if (id == GAP_ID)
        {

        }
        else if (id == PAN_ID)
        {
            switch (action)
            {
            case BT_PAN_DEMO_ACTION_PAN_CONNECT:
                app_bt_pan_demo_connect(p);
                break;

            case BT_PAN_DEMO_ACTION_PAN_DISCONNECT:
                app_bt_pan_demo_disconnect(app_db.remote_addr);
                break;

            case BT_PAN_DEMO_ACTION_PAN_SETUP_CONN_RSP:
                app_bt_pan_demo_setup_onn_rsp(app_db.remote_addr);
                break;

            case BT_PAN_DEMO_ACTION_PAN_NET_TYPE_FILTER_SET:
                app_bt_pan_demo_filter_net_type_set(app_db.remote_addr);
                break;

            case BT_PAN_DEMO_ACTION_PAN_MULTI_ADDR_FILTER_SET:
                app_bt_pan_demo_filter_multi_addr_set(app_db.remote_addr);
                break;

            case BT_PAN_DEMO_ACTION_PAN_SNED_GENERAL:
                app_bt_pan_demo_send(app_db.remote_addr, 0);
                break;

            case BT_PAN_DEMO_ACTION_PAN_SNED_COMPRESSED:
                app_bt_pan_demo_send(app_db.remote_addr, 2);
                break;

            case BT_PAN_DEMO_ACTION_PAN_SNED_SRC_ONLY:
                app_bt_pan_demo_send(app_db.remote_addr, 3);
                break;

            case BT_PAN_DEMO_ACTION_PAN_SNED_DST_ONLY:
                app_bt_pan_demo_send(app_db.remote_addr, 4);
                break;

            case BT_PAN_DEMO_ACTION_PAN_ARP:
                app_bt_pan_demo_arp(app_db.remote_addr);
                break;

            case BT_PAN_DEMO_ACTION_PAN_ARP_V6:
                app_bt_pan_demo_arp_v6(app_db.remote_addr);
                break;

            case BT_PAN_DEMO_ACTION_PAN_PING:
                //app_bt_pan_demo_ping(app_db.remote_addr);
                app_bt_pan_ping();
                break;

            case BT_PAN_DEMO_ACTION_PAN_PING_V6:
                app_bt_pan_demo_ping_v6(app_db.remote_addr);
                break;

            case BT_PAN_DEMO_ACTION_PAN_PING_REPLY:
                app_bt_pan_demo_ping_reply(app_db.remote_addr);
                break;

            case BT_PAN_DEMO_ACTION_PAN_PING_REPLY_V6:
                app_bt_pan_demo_ping_reply_v6(app_db.remote_addr);
                break;

            case BT_PAN_DEMO_ACTION_HTTP:
                app_bt_pan_demo_http();
                break;

            case BT_PAN_DEMO_ACTION_SOCKETS:
                app_bt_pan_sockets();
                break;

            case BT_PAN_DEMO_ACTION_SEM:
                app_bt_pan_sem();
                break;

            case BT_PAN_DEMO_ACTION_HTTPS_CLIENT:
                app_bt_pan_https_client();
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
