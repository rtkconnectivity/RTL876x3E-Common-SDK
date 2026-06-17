/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BLE_CFG_H_
#define _APP_BLE_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_BLE_DEVICE App BLE Device
  * @brief App BLE Device
  * @{
  */

/** @brief  Read only configurations for ble */
typedef struct
{
    uint8_t rtk_app_adv_support : 1;
    uint16_t timer_ota_adv_timeout;
} T_APP_BLE_CFG;


extern T_APP_BLE_CFG app_ble_cfg;


/**
    * @brief  BLE config module init
    * @param  void
    * @return void
    */
void app_ble_cfg_init(void);


/** End of APP_BLE_DEVICE
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
