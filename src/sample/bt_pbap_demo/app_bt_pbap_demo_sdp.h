/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _PBAP_DEMO_SDP_H_
#define _PBAP_DEMO_SDP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup PBAP_DEMO_SDP PBAP Demo SDP
* @brief PBAP Demo SDP
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup PBAP_DEMO_SDP_Exported_Macros PBAP Demo SDP Exported Macros
  * @brief
  * @{
  */

/**
* @brief Define PBAP default l2cap mtu size
* @note  PBAP application may send tx data lower than PBAP mtu size at a time
*/
#define PBAP_DEMO_PBAP_DEFAULT_MTU_SIZE 1012
/**
* @brief Define PBAP default local tx window size
* @note  PBAP application should send tx data when PBAP link credit is not zero
*
*/
#define PBAP_DEMO_PBAP_DEFAULT_CREDITS 7
/**
* @brief Define PBAP local server channel
* @note  PBAP service num may be different in different PBAP applications
*
*/
#define PBAP_DEMO_RFC_PBAP_CHANN_NUM 0x3

/** End of PBAP_DEMO_SDP_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup PBAP_DEMO_SDP_Exported_Functions PBAP Demo SDP Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  PBAP SDP init
    * @param  void
    * @return void
    */
void pbap_demo_sdp_init(void);

/** @} End of PBAP_DEMO_SDP_Exported_Functions */

/** @} End of PBAP_DEMO_SDP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PBAP_DEMO_SDP_H_ */
