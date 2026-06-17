/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_PP_BUFFER_H_
#define _APP_PP_BUFFER_H_

#include "stdint.h"

typedef struct
{
    void *buffer[2];
    volatile uint8_t write_index;
    volatile uint8_t read_index;
    volatile uint8_t read_avaliable[2];
} T_PP_BUFFER;

void app_pp_buffer_init(T_PP_BUFFER *ppbuf, void *buf0, void *buf1);

bool app_pp_buffer_get_read_buf(T_PP_BUFFER *ppbuf, void **p_read_buf);

void app_pp_buffer_set_read_done(T_PP_BUFFER *ppbuf);

void app_pp_buffer_get_write_buf(T_PP_BUFFER *ppbuf, void **p_write_buf);

void app_pp_buffer_set_write_done(T_PP_BUFFER *ppbuf);

#endif
