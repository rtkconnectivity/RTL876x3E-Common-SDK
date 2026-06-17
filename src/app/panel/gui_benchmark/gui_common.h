/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#ifndef __GUI_COMMON_H__
#define __GUI_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "gui_obj.h"
#include "gui_win.h"
#include "def_list.h"
#include "gui_tab.h"
#include "gui_img.h"
#include "gui_text.h"
#include "gui_scroll_text.h"
#include "gui_switch.h"
#include "gui_interface.h"

gui_img_t *app_gui_img_create(void       *parent,
                              const char *name,
                              void       *file,
                              int16_t     x,
                              int16_t     y,
                              int16_t     w,
                              int16_t     h);

gui_text_t *app_gui_text_create(void       *parent,
                                const char *name,
                                int16_t     x,
                                int16_t     y,
                                int16_t     w,
                                int16_t     h);

gui_scroll_text_t *app_gui_scroll_text_create(void       *parent,
                                              const char *name,
                                              int16_t     x,
                                              int16_t     y,
                                              int16_t     w,
                                              int16_t     h);


#ifdef __cplusplus
}
#endif
#endif
