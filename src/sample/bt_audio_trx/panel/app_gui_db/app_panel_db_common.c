/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "trace.h"
#include "guidef.h"
#include "gui_server.h"
#include "app_dlps.h"

extern bool gui_port_dc_lcd_is_work(void);

void app_panel_db_common_awake_gui_task(void)
{
    if (gui_port_dc_lcd_is_work() == false)
    {
        gui_msg_t msg = {0};
        msg.event = GUI_EVENT_DISPLAY_ON;
        gui_send_msg_to_server(&msg);
    }
}

#endif


/*-----------------------------------------------------------*/
