/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "bt_types.h"
#include "os_mem.h"
#include "app_msg.h"
#include "app_console_msg.h"
#include "app_bt_map_demo_link.h"
#include "app_bt_map_demo_console.h"
#include "app_bt_map_demo.h"

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
        else if (id == MAP_ID)
        {
            switch (action)
            {
            case MAP_DEMO_ACTION_CONNECT:
                {
                    memcpy(app_db.remote_addr, p, 6);
                    app_map_demo_connect(app_db.remote_addr);
                }
                break;

            case MAP_DEMO_ACTION_DISCONNECT:
                app_map_demo_disconnect(app_db.remote_addr);
                break;

            case MAP_DEMO_ACTION_MNS_ON:
                app_map_demo_msg_notification_set(app_db.remote_addr, true);
                break;

            case MAP_DEMO_ACTION_MNS_OFF:
                app_map_demo_msg_notification_set(app_db.remote_addr, false);
                break;

            case MAP_DEMO_ACTION_SET_FOLDER:
                {
                    uint8_t folder;

                    LE_STREAM_TO_UINT8(folder, p);
                    app_map_demo_folder_set(app_db.remote_addr, folder);
                }
                break;

            case MAP_DEMO_ACTION_GET_FOLDER_LISTING:
                app_map_demo_folder_listing_get(app_db.remote_addr);
                break;

            case MAP_DEMO_ACTION_GET_MSG_LISTING:
                app_map_demo_msg_listing_get(app_db.remote_addr);
                break;

            case MAP_DEMO_ACTION_GET_MSG:
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
