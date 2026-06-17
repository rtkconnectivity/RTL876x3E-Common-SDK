/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_MAP_CFG_H_
#define _APP_MAP_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_MAP App map
  * @brief App map
  * @{
  */
/** @brief  Read only configurations for map */
typedef struct
{
    uint8_t map_link_number;
} T_APP_MAP_CFG;


extern T_APP_MAP_CFG app_map_cfg;


/**
    * @brief  MAP config module init
    * @param  void
    * @return void
    */
void app_map_cfg_init(void);


/** End of APP_MAP
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
