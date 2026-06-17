/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_GUI_SIMPLE_SPEED_DEMO
#include "draw_font.h"
#include "resource.h"
#include "gui_win.h"
#include "gui_img.h"
#include "gui_text.h"
#include "gui_obj.h"
#include "gui_fb.h"
#include "font_mem.h"
#include "gui_view_instance.h"
#include "gui_message.h"


#define CURRENT_VIEW_NAME "simple_speed_view"
gui_text_t *text = NULL;
uint8_t num = 100;
char str[4] = {0};

void app_simple_speed_ui_design(gui_view_t *view)
{
#ifdef ENABLE_RTK_GUI_SCRIPT_AS_A_APP
#else
    gui_log("app_simple_speed_ui_design");
    gui_win_t *win = gui_win_create(gui_obj_get_root(), "win", 0, 0, 320, 320);
    gui_img_t *bg1 = gui_img_create_from_mem(win, "bg1", BG_410_502_BIN, 0, 0, 0, 0);
    gui_img_set_mode(bg1, IMG_BYPASS_MODE);

    gui_set_keep_active_time(8000);
#endif
}

GUI_VIEW_INSTANCE(CURRENT_VIEW_NAME, false, app_simple_speed_ui_design,
                  NULL);
#endif
