/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include "trace.h"
#include "bt_types.h"
#include "app_msg.h"
#include "app_console_msg.h"
#include "app_bt_spp_demo_console.h"
#include "app_bt_spp_demo_app.h"

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
        else if (id == SPP_ID)
        {
            switch (action)
            {
            case SPP_DEMO_ACTION_CONNECT:
                {
                    spp_demo_app_connect_req(p);
                }
                break;

            case SPP_DEMO_ACTION_DISCONNECT:
                {
                    uint8_t local_server_chann;

                    LE_STREAM_TO_UINT8(local_server_chann, p);
                    spp_demo_app_disconnect_req(p, local_server_chann);
                }
                break;

            case SPP_DEMO_ACTION_DISCONNECT_ALL:
                spp_demo_app_disconnect_all_req(p);
                break;

            case SPP_DEMO_ACTION_SEND_DATA:
                {
                    uint8_t  addr[6];
                    uint8_t  i;
                    uint8_t  data_len;

                    for (i = 0; i < 6; i++)
                    {
                        LE_STREAM_TO_UINT8(addr[i], p);
                    }

                    LE_STREAM_TO_UINT8(data_len, p);

                    spp_demo_app_tx_data(addr, p, data_len);
                }
                break;

            default:
                break;
            }
        }

        free(console_msg.u.buf);
        break;

    default:
        break;
    }
}
