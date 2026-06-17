/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_PERIPHERAL_SERVICE__
#define _APP_PERIPHERAL_SERVICE__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <profile_server.h>
#include <simple_ble_service.h>
#include <bas.h>

/** @defgroup PERIPH_APP Peripheral Application
  * @brief Peripheral Application
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief app_peripheral_service_init
 * Init simple service, battery service and register service callback
 *
 */
void app_peripheral_service_init(void);

/** End of PERIPH_APP
* @}
*/

#ifdef __cplusplus
}
#endif
#endif
