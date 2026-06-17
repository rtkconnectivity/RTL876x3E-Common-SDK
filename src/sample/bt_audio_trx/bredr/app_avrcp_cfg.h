/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AVRCP_CFG_H_
#define _APP_AVRCP_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_AVRCP App Avrcp
  * @brief App Avrcp
  * @{
  */
/** @brief  Read only configurations for avrcp */
typedef struct
{
    uint8_t avrcp_link_number;
    uint8_t enable_av_fwd_bwd_only_when_playing : 1;
    uint8_t disallow_adjust_volume_when_idle : 1;
} T_APP_AVRCP_CFG;


extern T_APP_AVRCP_CFG app_avrcp_cfg;


/**
    * @brief  AVRCP config module init
    * @param  void
    * @return void
    */
void app_avrcp_cfg_init(void);


/** End of APP_AVRCP
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
