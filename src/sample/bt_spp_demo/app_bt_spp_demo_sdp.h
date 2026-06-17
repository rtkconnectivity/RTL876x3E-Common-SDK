/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _SPP_DEMO_SDP_H_
#define _SPP_DEMO_SDP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup SPP_DEMO_SDP SPP Demo SDP
* @brief SPP Demo SDP
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup SPP_DEMO_SDP_Exported_Macros SPP Demo SDP Exported Macros
  * @brief
  * @{
  */

/**
* @brief Define SPP default l2cap mtu size
* @note  SPP application may send tx data lower than SPP mtu size at a time
*/
#define SPP_DEMO_SPP_DEFAULT_MTU_SIZE 1012
/**
* @brief Define SPP default local tx window size
* @note  SPP application should send tx data when SPP link credit is not zero
*
*/
#define SPP_DEMO_SPP_DEFAULT_CREDITS 7
/**
* @brief Define SPP local server channel
* @note  SPP service num may be different in different SPP applications
*
*/
#define SPP_DEMO_RFC_SPP_CHANN_NUM 0x3

/**
* @brief Define vendor SPP local server channel
* @note  SPP service num may be different in different SPP applications
*
*/
#define SPP_DEMO_RFC_VENDOR_SPP_CHANN_NUM 0x4

/** End of SPP_DEMO_SDP_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup SPP_DEMO_SDP_Exported_Functions SPP Demo SDP Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  SPP SDP init
    * @param  void
    * @return void
    */
void spp_demo_sdp_init(void);

/** @} End of SPP_DEMO_SDP_Exported_Functions */

/** @} End of SPP_DEMO_SDP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SPP_DEMO_SDP_H_ */
