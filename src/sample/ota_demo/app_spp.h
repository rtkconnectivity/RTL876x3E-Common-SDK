/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SPP_H_
#define _APP_SPP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_SPP App Spp
  * @brief App Sdp
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup OTA_DEMO_APP_Exported_Macros OTA Demo APP Exported Macros
  * @brief
  * @{
  */

/**
* @brief Define SPP maximum link numbers.
* @note  only one SPP server channel exists in the unique link based on RFCOMM.
*/
#define OTA_DEMO_LINK_NUM 0x2

/**
* @brief Define SPP maximum service numbers.
* @note  SPP service num includes different server channels
*/
#define OTA_DEMO_RFC_SPP_SERVICE_NUM 0x3 //
/** End of OTA_DEMO_APP_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup OTA_DEMO_APP_Exported_Functions OTA Demo APP Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  spp module init
    * @param  void
    * @return void
    */
void app_spp_init(void);


/** End of APP_SPP
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SPP_H_ */
