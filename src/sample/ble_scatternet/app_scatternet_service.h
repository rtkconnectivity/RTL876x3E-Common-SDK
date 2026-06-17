/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SCATTERNET_SERVICE__
#define _APP_SCATTERNET_SERVICE__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "profile_server.h"
#include "simple_ble_service.h"
#include "bas.h"

/** @defgroup SCATTERNET_APP Scatternet Application
  * @brief Scatternet Application
  * @{
  */
extern T_SERVER_ID simp_srv_id;/**< Simple ble service id*/
extern T_SERVER_ID bas_srv_id;/**< Battery service id */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief app_service_init
 * Init simple service, battery service and register service callback
 *
 */
void app_service_init(void);

/** End of SCATTERNET_APP
* @}
*/

#ifdef __cplusplus
}
#endif
#endif
