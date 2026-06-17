/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __DRV_TOUCH_H__
#define __DRV_TOUCH_H__

#include <stdbool.h>
#include <stdint.h>

bool rtk_touch_hal_init(void);
bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing);
void rtk_touch_hal_set_indicate(void (*indicate)(void *));
void rtk_touch_hal_int_config(bool enable);
bool drv_touch_read(uint16_t *x, uint16_t *y, bool *pressing);
bool drv_touch_init(void);

#endif  /* __DRV_TOUCH_H__ */

/******************* end of file ***************/
