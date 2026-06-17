/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT

#include "trace.h"
#include "gui_view_instance.h"
#include "gui_rect.h"

#define CURRENT_VIEW_NAME "blue_view"


static void switch_in_cb(gui_view_t *view);
static void switch_out_cb(gui_view_t *view);

GUI_VIEW_INSTANCE(CURRENT_VIEW_NAME, true, switch_in_cb, switch_out_cb);

static void switch_out_cb(gui_view_t *view)
{
    GUI_UNUSED(view);
    APP_PRINT_INFO0("blue view clean\n");
}

static void switch_in_cb(gui_view_t *view)
{
    gui_view_set_animate_step(view, 20);

    gui_rect_create(view, "hg_rect_notification_1_bg", 0, 0, 400, 490, 16, gui_rgb(0, 0, 255));

}


static int app_init(void)
{
    gui_view_create(gui_obj_get_root(), CURRENT_VIEW_NAME, 0, 0, 0, 0);
    return 0;
}

//GUI_INIT_APP_EXPORT(app_init);
#endif
