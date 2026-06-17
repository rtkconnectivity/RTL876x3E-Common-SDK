/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BLE_DEVICE_H_
#define _APP_BLE_DEVICE_H_



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_BLE_DEVICE App BLE Device
  * @brief App BLE Device
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_BLE_DEVICE_Exported_Functions App Ble Device Functions
    * @{
    */
/**
    * @brief  Ble Device module init
    * @param  void
    * @return void
    */
void app_ble_device_init(void);
void app_ble_device_handle_factory_reset(void);
void app_ble_device_handle_power_on(void);
void app_ble_device_handle_power_off(void);
void app_ble_device_handle_enter_pair_mode(void);
void app_ble_device_handle_b2s_bt_connected(uint8_t *bd_addr);
void app_ble_device_handle_radio_mode(T_BT_DEVICE_MODE radio_mode);

bool app_ble_device_is_active(void);
/** @} */ /* End of group APP_BLE_DEVICE_Exported_Functions */

/** End of APP_BLE_DEVICE
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BLE_DEVICE_H_ */
