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
#include "app_bt_iap_demo_app.h"
#include "app_bt_iap_demo_console.h"
#include "app_bt_iap_demo_link.h"

void app_console_handle_msg(T_IO_MSG console_msg)
{
    uint16_t  subtype;
    uint16_t  id;
    uint16_t  action;
    uint8_t  *p;
    uint8_t  addr[6];

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
        else if (id == IAP_ID)
        {
            switch (action)
            {
            case APP_ACTION_IAP_CONNECT:
                STREAM_TO_ARRAY(addr, p, 6);
                iap_demo_conn_start(addr);
                break;

            case APP_ACTION_IAP_DISCONNECT:
                iap_demo_disconnect_start();
                break;

            case APP_ACTION_IAP_DATA_SEND:
                iap_demo_app_tx_data_test();
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
