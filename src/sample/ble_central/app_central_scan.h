/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CENTRAL_SCAN_H_
#define _APP_CENTRAL_SCAN_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"

#define APP_SCAN_INTERVAL     (0xA0)
#define APP_SCAN_WINDOW       (0x98)

/** @defgroup  CENTRAL_APP Central Application
    * @brief This file handles BLE central application routines.
    * @{
    */
/**
 * @brief app_scan_start
 * @param scan_interval The frequency of scan, in units of 0.625ms, range: 0x0004 to 0xFFFF.
 * @param scan_window   The length of scan, in units of 0.625ms, range: 0x0004 to 0xFFFF.
 */
void app_scan_start(uint16_t scan_interval, uint16_t scan_window);

/**
 * @brief app_scan_stop
 *
 */
void app_scan_stop(void);

/** End of CENTRAL_APP
* @}
*/
#ifdef __cplusplus
}
#endif
#endif
