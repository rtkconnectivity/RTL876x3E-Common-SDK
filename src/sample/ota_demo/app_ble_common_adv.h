/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BLE_COMMON_ADV_H_
#define _APP_BLE_COMMON_ADV_H_

#include <stdint.h>
#include <stdbool.h>
#include "ble_ext_adv.h"
#include "remote.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_BLE_COMMON_ADV App BLE Common Adv
  * @brief App BLE Common Adv
  * @{
  */


typedef void (*LE_COMMON_ADV_CB)(uint8_t cb_type, T_BLE_EXT_ADV_CB_DATA cb_data);


/*============================================================================*
 *                              Macros
 *============================================================================*/


/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
    * @brief  get ble common advertising conn id
    * @param  void
    * @return ble advertising conn id
    */
uint8_t app_ble_common_adv_get_conn_id(void);

/**
    * @brief  reset ble common advertising conn id
    * @param  void
    * @return void
    */
void app_ble_common_adv_reset_conn_id(void);

/**
    * @brief  start ble common advertising
    * @param  duration_ms advertising duration time
    * @return true  Command has been sent successfully.
    * @return false Command was fail to send.
    */
bool app_ble_common_adv_start(uint16_t duration_10ms);

/**
    * @brief  stop ble common advertising
    * @param  app_cause cause
    * @return true  Command has been sent successfully.
    * @return false Command was fail to send.
    */
bool app_ble_common_adv_stop(int8_t app_cause);

/**
    * @brief  init ble common advertising parameters
    * @param  void
    * @return void
    */
void app_ble_common_adv_init(void);


/** @} */ /* End of group APP_BLE_COMMON_ADV_Exported_Functions */

/** End of APP_BLE_COMMON_ADV
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BLE_COMMON_ADV_H_ */
