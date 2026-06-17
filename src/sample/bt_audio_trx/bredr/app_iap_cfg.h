/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_IAP_CFG_H_
#define _APP_IAP_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_IAP App Iap
  * @brief This file provide api for iap function.
  * @{
  */

/** @brief  Read only configurations for IAP */
typedef struct
{
    uint8_t iap_link_number;
} T_APP_IAP_CFG;


extern T_APP_IAP_CFG app_iap_cfg;


/**
    * @brief  IAP config module init
    * @param  void
    * @return void
    */
void app_iap_cfg_init(void);


/** End of APP_IAP
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
