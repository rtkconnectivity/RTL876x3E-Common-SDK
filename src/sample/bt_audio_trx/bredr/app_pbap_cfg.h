/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_PBAP_CFG_H_
#define _APP_PBAP_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_PBAP App Pbap
  * @brief App Pbap
  * @{
  */

/** @brief  Read only configurations for pbap */
typedef struct
{
    uint8_t pbap_link_number;
} T_APP_PBAP_CFG;


extern T_APP_PBAP_CFG app_pbap_cfg;


/**
    * @brief  PBAP config module init
    * @param  void
    * @return void
    */
void app_pbap_cfg_init(void);


/** End of APP_PBAP
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
