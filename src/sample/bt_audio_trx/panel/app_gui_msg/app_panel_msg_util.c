/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include <stdlib.h>
#include "trace.h"
#include "os_msg.h"
#include "app_panel_msg_util.h"

extern void *gui_msg_queue_handle;
extern void *gui_evt_queue_handle;

bool app_panel_msg_send_to_app(T_APP_GUI_MSG   *data)
{
    int8_t error_code = 0;
    uint8_t event = 0;
    if (os_msg_send(gui_msg_queue_handle, data, 0) != true)
    {
        error_code = -1;
        goto FAIL;
    }

    event = (uint8_t)(EVENT_GROUP_GUI << 4);
    if (os_msg_send(audio_evt_queue_handle, &event, 0) != true)
    {
        error_code = -2;
        goto FAIL;
    }
    return true;
FAIL:
    APP_PRINT_ERROR1("app_panel_msg_send_to_app: send msg to gui queue failed, error code %d",
                     error_code);
    return false;
}

#endif


/*-----------------------------------------------------------*/
