/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __AW87390_DRIVER_INTERFACE__H__
#define __AW87390_DRIVER_INTERFACE__H__

#include <stdint.h>
#include "stdbool.h"

typedef enum
{
    AW87390_MODE_MUSIC     = 0x00,
    AW87390_MODE_BYPASS    = 0x01,
} T_AW87390_MODE;

void aw87390_set_bypass(bool bypassed);
void aw87390_i2c_enter_dlps(void);
void aw87390_i2c_exit_dlps(void);
bool aw87390_get_chip_id(uint8_t *p_chip_id);
bool aw87390_start(uint8_t sample_rate);
bool aw87390_stop(void);
void aw87390_driver_init(void);

#endif
