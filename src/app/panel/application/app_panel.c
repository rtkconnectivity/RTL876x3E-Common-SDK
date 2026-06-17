/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "draw_font.h"
#include "resource.h"
#include "gui_win.h"
#include <gui_img.h>
#include "app_gui.h"
#include "gui_components_init.h"
#include "gui_view_instance.h"

#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
#define IMAGE_0_BIN_FILE_ADDR   IMAGE_368_448_0_BIN
#define IMAGE_1_BIN_FILE_ADDR   IMAGE_368_448_1_BIN
#define IMAGE_2_BIN_FILE_ADDR   IMAGE_368_448_2_BIN
#define IMAGE_3_BIN_FILE_ADDR   IMAGE_368_448_3_BIN
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
#define IMAGE_0_BIN_FILE_ADDR   IMAGE_385_320_0_BIN
#define IMAGE_1_BIN_FILE_ADDR   IMAGE_385_320_1_BIN
#define IMAGE_2_BIN_FILE_ADDR   IMAGE_385_320_2_BIN
#define IMAGE_3_BIN_FILE_ADDR   IMAGE_385_320_3_BIN
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#define IMAGE_0_BIN_FILE_ADDR   IMAGE_410_502_0_BIN
#define IMAGE_1_BIN_FILE_ADDR   IMAGE_410_502_1_BIN
#define IMAGE_2_BIN_FILE_ADDR   IMAGE_410_502_2_BIN
#define IMAGE_3_BIN_FILE_ADDR   IMAGE_410_502_3_BIN
#else
#define IMAGE_0_BIN_FILE_ADDR   IMAGE_454_454_0_BIN
#define IMAGE_1_BIN_FILE_ADDR   IMAGE_454_454_1_BIN
#define IMAGE_2_BIN_FILE_ADDR   IMAGE_454_454_2_BIN
#define IMAGE_3_BIN_FILE_ADDR   IMAGE_454_454_3_BIN
#endif



#define VIEW_NAME_0             "panel_view_0"
#define VIEW_NAME_1             "panel_view_1"
#define VIEW_NAME_2             "panel_view_2"
#define VIEW_NAME_3             "panel_view_3"

/* view 0 */
static void switch_in_cb_0(gui_view_t *view);
static void switch_out_cb_0(gui_view_t *view);
GUI_VIEW_INSTANCE(VIEW_NAME_0, false, switch_in_cb_0, switch_out_cb_0);

static void switch_out_cb_0(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void switch_in_cb_0(gui_view_t *view)
{
    gui_img_create_from_mem(view, "bg0", IMAGE_0_BIN_FILE_ADDR, 0, 0, 0, 0);
    gui_view_switch_on_event(view, VIEW_NAME_1, SWITCH_OUT_TO_LEFT_USE_TRANSLATION,
                             SWITCH_IN_FROM_RIGHT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_LEFT);
    gui_view_switch_on_event(view, VIEW_NAME_3, SWITCH_OUT_TO_RIGHT_USE_TRANSLATION,
                             SWITCH_IN_FROM_LEFT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_RIGHT);
}

/* view 1 */
static void switch_in_cb_1(gui_view_t *view);
static void switch_out_cb_1(gui_view_t *view);
GUI_VIEW_INSTANCE(VIEW_NAME_1, false, switch_in_cb_1, switch_out_cb_1);

static void switch_out_cb_1(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void switch_in_cb_1(gui_view_t *view)
{
    gui_img_create_from_mem(view, "bg1", IMAGE_1_BIN_FILE_ADDR, 0, 0, 0, 0);
    gui_view_switch_on_event(view, VIEW_NAME_0, SWITCH_OUT_TO_RIGHT_USE_TRANSLATION,
                             SWITCH_IN_FROM_LEFT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_RIGHT);
    gui_view_switch_on_event(view, VIEW_NAME_2, SWITCH_OUT_TO_LEFT_USE_TRANSLATION,
                             SWITCH_IN_FROM_RIGHT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_LEFT);
}

/* view 2 */
static void switch_in_cb_2(gui_view_t *view);
static void switch_out_cb_2(gui_view_t *view);
GUI_VIEW_INSTANCE("panel_view_2", false, switch_in_cb_2, switch_out_cb_2);

static void switch_out_cb_2(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void switch_in_cb_2(gui_view_t *view)
{
    gui_img_create_from_mem(view, "bg2", IMAGE_2_BIN_FILE_ADDR, 0, 0, 0, 0);
    gui_view_switch_on_event(view, VIEW_NAME_1, SWITCH_OUT_TO_RIGHT_USE_TRANSLATION,
                             SWITCH_IN_FROM_LEFT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_RIGHT);
    gui_view_switch_on_event(view, VIEW_NAME_3, SWITCH_OUT_TO_LEFT_USE_TRANSLATION,
                             SWITCH_IN_FROM_RIGHT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_LEFT);
}

/* view 3 */
static void switch_in_cb_3(gui_view_t *view);
static void switch_out_cb_3(gui_view_t *view);
GUI_VIEW_INSTANCE("panel_view_3", false, switch_in_cb_3, switch_out_cb_3);

static void switch_out_cb_3(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void switch_in_cb_3(gui_view_t *view)
{
    gui_img_create_from_mem(view, "bg3", IMAGE_3_BIN_FILE_ADDR, 0, 0, 0, 0);
    gui_view_switch_on_event(view, VIEW_NAME_2, SWITCH_OUT_TO_RIGHT_USE_TRANSLATION,
                             SWITCH_IN_FROM_LEFT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_RIGHT);
    gui_view_switch_on_event(view, VIEW_NAME_0, SWITCH_OUT_TO_LEFT_USE_TRANSLATION,
                             SWITCH_IN_FROM_RIGHT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_LEFT);
}

static int app_init(void)
{
    gui_view_create(gui_obj_get_root(), VIEW_NAME_0, 0, 0, 0, 0);
    return 0;
}

GUI_INIT_APP_EXPORT(app_init);
