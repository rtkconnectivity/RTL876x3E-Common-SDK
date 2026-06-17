/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SPP_CFG_H_
#define _APP_SPP_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_SPP App Spp
  * @brief App Spp
  * @{
  */

/** @brief  Read only configurations for spp */
typedef struct
{
    uint8_t spp_link_number;

} T_APP_SPP_CFG;


extern T_APP_SPP_CFG app_spp_cfg;


/**
    * @brief  SPP config module init
    * @param  void
    * @return void
    */
void app_spp_cfg_init(void);


/** End of APP_SPP
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
