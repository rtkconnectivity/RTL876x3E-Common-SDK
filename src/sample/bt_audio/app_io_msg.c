/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "os_sync.h"
#include "os_msg.h"
#include "trace.h"
#include "app_io_msg.h"
#include "app_console_msg.h"
#include "app_bt_audio_a2dp.h"

extern void *bt_audio_io_queue_handle;
extern void *bt_audio_evt_queue_handle;

bool app_io_send_msg(T_IO_MSG *io_msg)
{
    T_EVENT_TYPE  event;
    bool ret = false;
    event = EVENT_IO_TO_APP;

    if (os_msg_send(bt_audio_io_queue_handle, io_msg, 0) == true)
    {
        ret = os_msg_send(bt_audio_evt_queue_handle, &event, 0);
    }

    return ret;
}

void app_io_handle_msg(T_IO_MSG io_driver_msg_recv)
{
    uint16_t msgtype = io_driver_msg_recv.type;

    switch (msgtype)
    {
    case IO_MSG_TYPE_A2DP_SRC:
        {
            app_bt_audio_a2dp_handle_msg(io_driver_msg_recv);
        }
        break;

    case IO_MSG_TYPE_CONSOLE:
        {
            app_console_handle_msg(io_driver_msg_recv);
        }
        break;

    default:
        break;
    }
}
