/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __AW_CUSTOMER_DRIVER_INTERFACE__H__
#define __AW_CUSTOMER_DRIVER_INTERFACE__H__

#include <stdint.h>
#include "stdbool.h"

typedef enum
{
    aw_customer_mode_1 = 1,
    aw_customer_mode_2 = 2,
    aw_customer_mode_3 = 3,
    aw_customer_mode_4 = 4,
    aw_customer_mode_5 = 5,
    aw_customer_mode_6 = 6,
    aw_customer_mode_7 = 7,
    aw_customer_mode_8 = 8,
    aw_customer_mode_9 = 9,
    aw_customer_mode_10 = 10,
} AW_CUSTOMER_MODE_TYPE;

void set_customer_aw_mdoe_type(uint8_t type);
uint8_t get_customer_aw_mdoe_type(void);
void customer_aw_pin_init(void);
bool aw_customer_stop(void);
bool aw_customer_start(uint8_t sample_rate);
#endif
