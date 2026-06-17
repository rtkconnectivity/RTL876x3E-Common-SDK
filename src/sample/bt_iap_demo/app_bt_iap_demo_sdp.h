/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _IAP_DEMO_SDP_H_
#define _IAP_DEMO_SDP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup IAP_DEMO_SDP iAP Demo SDP
* @brief iAP Demo SDP
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup IAP_DEMO_SDP_Exported_Macros iAP Demo SDP Exported Macros
  * @brief
  * @{
  */

/**
* @brief Define iAP default l2cap mtu size
* @note  iAP application may send tx data lower than iAP mtu size at a time
*/
#define IAP_DEMO_IAP_DEFAULT_MTU_SIZE 1012
/**
* @brief Define iAP default local tx window size
* @note  iAP application should send tx data when iAP link credit is not zero
*
*/
#define IAP_DEMO_IAP_DEFAULT_CREDITS 7
/**
* @brief Define iAP local server channel
* @note  iAP service num may be different in different iAP applications
*
*/
#define IAP_DEMO_RFC_IAP_CHANN_NUM 0x3

/** End of IAP_DEMO_SDP_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup IAP_DEMO_SDP_Exported_Functions iAP Demo SDP Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  iAP SDP init
    * @param  void
    * @return void
    */
void iap_demo_sdp_init(void);

/** @} End of IAP_DEMO_SDP_Exported_Functions */

/** @} End of IAP_DEMO_SDP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IAP_DEMO_SDP_H_ */
