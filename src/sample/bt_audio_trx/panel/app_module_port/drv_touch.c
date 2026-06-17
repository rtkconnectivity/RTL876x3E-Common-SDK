/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_gui.h"
#include "trace.h"
#include "drv_touch.h"
#include "board.h"
#if (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CS816T)
#include "module_touch_cst816t.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_GT9147)
#include "module_touch_gt9147.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CST816D)
#include "module_touch_cst816d.h"
#elif (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CHSC5816)
#include "module_touch_chsc5816.h"
#endif

uint16_t x_old = 0;
uint16_t y_old = 0;

bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    TOUCH_DATA touch_data = {0};
    touch_read_key_value(&touch_data);
    if (touch_data.is_press)
    {
        *x = touch_data.x;
        *y = touch_data.y;
        x_old = touch_data.x;
        y_old = touch_data.y;
    }
    else
    {
        *x = x_old;
        *y = y_old;
    }
    *pressing = touch_data.is_press;
    return true;
}

void rtk_touch_hal_set_indicate(void (*indicate)(void *))
{
    touch_register_irq_callback(indicate, NULL);
}

void rtk_touch_hal_int_config(bool enable)
{
    rtk_touch_hal_int_config(enable);
}

bool rtk_touch_hal_init(void)
{
    return touch_driver_init();
}

bool drv_touch_read(uint16_t *x, uint16_t *y, bool *pressing)
{
    return rtk_touch_hal_read_all(x, y, pressing);
}

void drv_touch_set_indicate(void (*indicate)(void *))
{
    rtk_touch_hal_set_indicate(indicate);
}

void drv_touch_int_config(bool enable)
{
    rtk_touch_hal_int_config(enable);
}

bool drv_touch_init(void)
{
    return rtk_touch_hal_init();
}


/************** end of file ********************/
