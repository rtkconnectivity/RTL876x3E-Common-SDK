/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BLE_CLIENT_H_
#define _APP_BLE_CLIENT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_BLE_CLIENT App Ble Client
  * @brief App Ble Client
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_BLE_CLIENT_Exported_Macros App Ble Client Macros
   * @{
   */

/** End of APP_BLE_CLIENT_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_BLE_CLIENT_Exported_Types App Ble Client Types
    * @{
    */




/**  @brief  Define notification source data for record ANCS notification parameters */
typedef struct
{
    /**This field informs the accessory whether the given iOS notification was added,
           modified, or removed*/
    uint8_t event_id;
    /**A bitmask whose set bits inform an NC of specificities with the iOS notification.*/
    uint8_t event_flags;
    /**A numerical value providing a category in which the iOS notification can be classified.*/
    uint8_t category_id;
    /**The current number of active iOS notifications in the given category.*/
    uint8_t category_count;
    /**The unique identifier (UID) for the iOS notification.*/
    uint32_t notification_uid;
} T_NS_DATA, *P_NS_DATA;



/** End of APP_BLE_CLIENT_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_BLE_CLIENT_Exported_Functions App Ble Client Functions
    * @{
    */
/**
    * @brief  Ble Client module init
    * @param  void
    * @return void
    */
void app_ble_client_init(void);
/** @} */ /* End of group APP_BLE_CLIENT_Exported_Functions */

#if BLE_HID_CLIENT_SUPPORT
uint16_t app_ble_hid_get_conn_handle(void);
#endif

/** End of APP_BLE_CLIENT
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BLE_CLIENT_H_ */
