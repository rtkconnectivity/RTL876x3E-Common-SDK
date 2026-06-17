/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "guidef.h"
#include "gui_port.h"
#include "drv_lcd.h"
#include "drv_touch.h"
#include "trace.h"
#include "gui_api.h"
#include "gui_server.h"

static struct gui_touch_port_data raw_data = {0};
static gui_wheel_port_data_t wheel_port_data = {0};
extern bool gui_port_dc_lcd_is_work(void);

/***touch device***/

void port_button_set_indicate(void (*callback)(void))
{
    rtk_touch_hal_set_indicate((void (*)(void *))callback);
}

void port_button_indicate_cback(void)
{
    if (gui_port_dc_lcd_is_work() == false)
    {
        gui_msg_t msg = {0};
        msg.event = GUI_EVENT_DISPLAY_ON;
        gui_send_msg_to_server(&msg);
    }
}

gui_touch_port_data_t *port_touchpad_get_data()
{
    uint16_t x = 0;
    uint16_t y = 0;
    bool pressing = 0;

    if (drv_touch_read(&x, &y, &pressing) == false)
    {
        return NULL;
    }
    if (pressing == true)
    {
        raw_data.event = 2;
    }
    else
    {
        raw_data.event = 1;
    }


    raw_data.timestamp_ms = sys_timestamp_get();

    raw_data.width = 0;
    raw_data.x_coordinate = x;
    raw_data.y_coordinate = y;
    //gui_log("event = %d, x = %d, y = %d, ms:%d\n", raw_data.event, raw_data.x_coordinate, raw_data.y_coordinate,raw_data.timestamp_ms);

    return &raw_data;
}

gui_wheel_port_data_t *port_wheel_get_data(void)
{
    return &wheel_port_data;
}

static struct gui_indev indev =
{
    .tp_get_data = port_touchpad_get_data,
    .wheel_get_port_data = port_wheel_get_data,
    .touch_timeout_ms = 110,
    .long_button_time_ms = 800,
    .short_button_time_ms = 300,
    .quick_slide_time_ms = 50,
};


extern void gui_indev_info_register(struct gui_indev *info);
void gui_port_indev_init(void)
{
    indev.tp_height = drv_lcd_get_screen_height();
    indev.tp_witdh = drv_lcd_get_screen_width();
    gui_indev_info_register(&indev);
    port_button_set_indicate(port_button_indicate_cback);
}

