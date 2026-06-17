/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _IAP_DEMO_APP_H_
#define _IAP_DEMO_APP_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup IAP_DEMO_APP iAP Demo APP
  * @brief iAP Demo APP
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup IAP_DEMO_APP_Exported_Macros iAP Demo APP Exported Macros
  * @brief
  * @{
  */

/**
* @brief Define iAP ExternalAccessoryProtocol identifuer.
* @note  All ExternalAccessoryProtocol identifuer must be unique.
*/
#define EA_PROTOCOL_ID_RTK      10

/**
* @brief Define iAP maximum link numbers.
* @note  only one iAP server channel exists in the unique link based on RFCOMM.
*/
#define IAP_DEMO_LINK_NUM 0x2

/** End of IAP_DEMO_APP_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup IAP_DEMO_APP_Exported_Functions iAP Demo APP Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  iAP cp init
    * @param  void
    * @return void
    */
void app_iap_cp_hw_init(void);

/**
    * @brief  iAP profile init
    * @param  void
    * @return void
    */
void iap_demo_app_init(void);

/**
    * @brief  Start iAP connect procedure
    * \param[in] bd_addr    Remote BT address.
    * @return void
    */
bool iap_demo_conn_start(uint8_t *bd_addr);

/**
    * @brief  Start iAP disconnect procedure
    * @param  void
    * @return void
    */
bool iap_demo_disconnect_start(void);

/**
    * @brief  Send iAP data
    * @param  void
    * @return void
    */
bool iap_demo_app_tx_data_test(void);

/** @} End of IAP_DEMO_APP_Exported_Functions */

/** @} End of IAP_DEMO_APP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IAP_DEMO_APP_H_ */
