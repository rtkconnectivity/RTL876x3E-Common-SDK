/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_IO_RESOURCE_INIT_H_
#define _APP_IO_RESOURCE_INIT_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <os_mem.h>
#include "trace.h"

#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

extern uint8_t lcd_dma_ch_num;

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
    * @brief  io resource request
    * @param  void
    * @return void
    */
void app_io_resource_request_init(void);


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif /*_APP_IO_RESOURCE_INIT_H_*/
