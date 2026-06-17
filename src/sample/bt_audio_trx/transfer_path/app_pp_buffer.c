/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if (F_APP_SPI_ROLE_MASTER || F_APP_SPI_ROLE_SLAVE)

#include "trace.h"
#include "app_pp_buffer.h"
#include "string.h"

void app_pp_buffer_init(T_PP_BUFFER *ppbuf, void *buf0, void *buf1)
{
    memset(ppbuf, 0, sizeof(T_PP_BUFFER));
    ppbuf->buffer[0] = buf0;
    ppbuf->buffer[1] = buf1;
}

bool app_pp_buffer_get_read_buf(T_PP_BUFFER *ppbuf, void **p_read_buf)
{
    if (ppbuf->read_avaliable[0])
    {
        ppbuf->read_index = 0;
    }
    else if (ppbuf->read_avaliable[1])
    {
        ppbuf->read_index = 1;
    }
    else
    {
        return false;
    }

    *p_read_buf = ppbuf->buffer[ppbuf->read_index];

    return true;
}

void app_pp_buffer_set_read_done(T_PP_BUFFER *ppbuf)
{
    ppbuf->read_avaliable[ppbuf->read_index] = false;
}

void app_pp_buffer_get_write_buf(T_PP_BUFFER *ppbuf, void **p_write_buf)
{
    if (ppbuf->write_index == ppbuf->read_index)
    {
        ppbuf->write_index = !ppbuf->read_index;
    }
    *p_write_buf = ppbuf->buffer[ppbuf->write_index];
}

void app_pp_buffer_set_write_done(T_PP_BUFFER *ppbuf)
{
    ppbuf->read_avaliable[ppbuf->write_index] = true;
    ppbuf->write_index = !ppbuf->write_index;
}

#endif
