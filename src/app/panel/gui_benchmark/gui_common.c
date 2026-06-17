/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#include "gui_common.h"
#include "gui_interface.h"
#include "gui_server.h"
#include "def_list.h"

#define MAX_USER_DEFINE_MSG_EVENT  16

static gui_event_dsc_t user_define_msg_event[MAX_USER_DEFINE_MSG_EVENT];
static uint32_t user_define_msg_event_cnt = 0;
static void *user_define_msg_event_obj[MAX_USER_DEFINE_MSG_EVENT];

static void gui_user_event_handler(gui_obj_t *obj, gui_msg_t *msg)
{
    gui_list_t *node = NULL;
    gui_list_for_each(node, &obj->child_list)
    {
        gui_obj_t *obj = gui_list_entry(node, gui_obj_t, brother_list);
        if (msg->sub_event > GUI_EVENT_USER_DEFINE)
        {
            for (uint32_t i = 0; i < obj->event_dsc_cnt; i++)
            {
                gui_event_dsc_t *event_dsc = obj->event_dsc + i;
                if (event_dsc->filter == msg->sub_event)
                {
                    if (user_define_msg_event_cnt >= MAX_USER_DEFINE_MSG_EVENT)
                    {
                        GUI_ASSERT(NULL != NULL);
                        return;
                    }
                    user_define_msg_event[user_define_msg_event_cnt].event_cb = event_dsc->event_cb;
                    user_define_msg_event[user_define_msg_event_cnt].event_code = (gui_event_t)msg->sub_event;
                    user_define_msg_event[user_define_msg_event_cnt].user_data = msg->payload;
                    user_define_msg_event_obj[user_define_msg_event_cnt] = obj;
                    user_define_msg_event_cnt++;
                }
            }
        }
        gui_user_event_handler(obj, msg);
    }
}

static void gui_update_event_cb(gui_msg_t *msg)
{
    gui_app_t *app = gui_current_app();
    gui_obj_t *screen = &app->screen;

    if (msg->sub_event > GUI_EVENT_USER_INIT)
    {
        gui_user_event_handler(screen, msg);
        for (uint32_t i = 0; i < user_define_msg_event_cnt; i++)
        {
            user_define_msg_event[i].event_cb(user_define_msg_event_obj[i], user_define_msg_event[i].event_code,
                                              user_define_msg_event[i].user_data);
            user_define_msg_event[i].event_code = GUI_EVENT_INVALIDE;
        }
        user_define_msg_event_cnt = 0;
    }
}

void gui_update_by_event(gui_event_user_t event, void *data, bool force_update)
{
    extern bool gui_send_msg_to_server(gui_msg_t *msg);

    gui_msg_t p_msg;
    if (event < GUI_EVENT_USER_INIT)
    {
        p_msg.event = event;
        gui_send_msg_to_server(&p_msg);
    }
    else if (event > GUI_EVENT_USER_INIT)
    {
        p_msg.event = GUI_EVENT_USER_INIT;
        p_msg.sub_event = event;
        p_msg.payload = data;
        p_msg.cb = (gui_msg_cb)gui_update_event_cb;
        gui_send_msg_to_server(&p_msg);
    }
    else
    {
        gui_log("do nothing when send GUI_EVENT_USER_INIT");
        //do nothing
    }

    if (force_update)
    {
        gui_msg_t p_msg_on;
        p_msg_on.event = GUI_EVENT_DISPLAY_ON;
        gui_send_msg_to_server(&p_msg_on);
    }
}

gui_img_t *app_gui_img_create(void       *parent,
                              const char *name,
                              void       *file,
                              int16_t     x,
                              int16_t     y,
                              int16_t     w,
                              int16_t     h)
{
    gui_img_t *img = NULL;

    img = gui_img_create_from_mem(parent, name, file, x, y, w, h);

    return img;
}

gui_text_t *app_gui_text_create(void       *parent,
                                const char *name,
                                int16_t     x,
                                int16_t     y,
                                int16_t     w,
                                int16_t     h)
{
    gui_text_t *text = NULL;

    text = gui_text_create(parent, name, x, y, w, h);
    gui_text_font_mode_set(text, FONT_SRC_MEMADDR);

    return text;
}

gui_scroll_text_t *app_gui_scroll_text_create(void       *parent,
                                              const char *name,
                                              int16_t     x,
                                              int16_t     y,
                                              int16_t     w,
                                              int16_t     h)
{
    gui_scroll_text_t *scroll_text = NULL;

    scroll_text = gui_scroll_text_create(parent, name, x, y, w, h);

    return scroll_text;
}
