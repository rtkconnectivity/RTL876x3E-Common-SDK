/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SCATTERNET_CLIENT__
#define _APP_SCATTERNET_CLIENT__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "profile_client.h"

/** @defgroup  SCATTERNET_APP Scatternet Application
    * @brief This file handles BLE Scatternet application routines.
    * @{
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
extern T_CLIENT_ID simple_ble_client_id;/**< Simple ble service client id*/
extern T_CLIENT_ID gaps_client_id;/**< gap service client id*/
extern T_CLIENT_ID bas_client_id;/**< battery service client id*/

/**
 * @brief app_client_init
          Add GATT clients and register callbacks
 * @return void
 */
void app_client_init(void);

/** End of SCATTERNET_APP
* @}
*/
#ifdef __cplusplus
}
#endif
#endif
