/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "trace.h"
#include "os_msg.h"
#include "app_panel_msg.h"
#include "app_panel_le_msg.h"
#include "app_panel_bredr_msg.h"
#include "app_panel_device_msg.h"

void *gui_msg_queue_handle = NULL;
void *gui_evt_queue_handle = NULL;

bool app_panel_msg_channel_register(void *evt_queue, void *io_queue, uint16_t msg_queue_elem_num)
{
    os_msg_queue_create(&gui_msg_queue_handle, "gui_event_queue", msg_queue_elem_num,
                        sizeof(T_APP_GUI_MSG));
    gui_evt_queue_handle = evt_queue;

    return true;
}

void app_panel_msg_handle(uint8_t event)
{
    T_APP_GUI_MSG gui_msg = {0};
    if (os_msg_recv(gui_msg_queue_handle, &gui_msg, 0) == true)
    {
        APP_PRINT_INFO1("app_panel_msg_handle: type %d", gui_msg.type);
        switch (gui_msg.type)
        {
        case EVENT_GUI_TO_DEVICE:
            app_panel_device_msg_handle(&gui_msg);
            break;
        case EVENT_GUI_TO_BREDR:
            app_panel_bredr_msg_handle(&gui_msg);
            break;
        case EVENT_GUI_TO_BLE:
            app_panel_le_msg_handle(&gui_msg);
            break;
        default:
            break;
        }
    }
}

#endif


/*-----------------------------------------------------------*/
