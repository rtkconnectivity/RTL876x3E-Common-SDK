/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SDP_H_
#define _APP_SDP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/** @defgroup APP_SDP App Sdp
  * @brief App Sdp
  * @{
  */

/**
* @brief Define SPP local server channel
* @note  SPP service num may be different in different SPP applications
*
*/

#define RFC_SPP_CHANN_NUM               6
#define RFC_RTK_VENDOR_CHANN_NUM        8

#define RTK_COMPANY_ID    (0x005D)
#define VENDOR_ID_SOURCE  (0x0001)
#define PRODUCT_ID        (0x223B)
#define PRODUCT_VERSION   (0x0100)


/**
    * @brief  sdp module init
    * @param  void
    * @return void
    */
void app_sdp_init(void);


/** End of APP_SDP
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SDP_H_ */
