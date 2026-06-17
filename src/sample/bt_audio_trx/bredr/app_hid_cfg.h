/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_HID_CFG_H_
#define _APP_HID_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_HID App Hid
  * @brief App Hid
  * @{
  */

/** @brief  Read only configurations for HID */
typedef struct
{
    uint8_t hid_link_number;
} T_APP_HID_CFG;


extern T_APP_HID_CFG app_hid_cfg;


/**
    * @brief  HID config module init
    * @param  void
    * @return void
    */
void app_hid_cfg_init(void);


/** End of APP_HID
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
