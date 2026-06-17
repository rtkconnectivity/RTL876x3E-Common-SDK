/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_PAN_DEMO_H_
#define _APP_BT_PAN_DEMO_H_

#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_PAN_DEMO BT PAN Demo
* @brief APP BT PAN Demo
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_BT_PAN_DEMO_Exported_Macros BT PAN Demo Exported Macros
  * @brief
  * @{
  */



/** End of APP_BT_PAN_DEMO_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BT_PAN_DEMO_Exported_Functions BT PAN Demo Exported Functions
  * @brief
  * @{
  */

void app_bt_pan_demo_init(void);

bool app_bt_pan_demo_connect(uint8_t *bd_addr);

bool app_bt_pan_demo_disconnect(uint8_t *bd_addr);

bool app_bt_pan_demo_setup_onn_rsp(uint8_t *bd_addr);

bool app_bt_pan_demo_filter_net_type_set(uint8_t *bd_addr);

bool app_bt_pan_demo_filter_multi_addr_set(uint8_t *bd_addr);

bool app_bt_pan_demo_send(uint8_t *bd_addr, uint8_t type);

bool app_bt_pan_demo_arp(uint8_t *bd_addr);

bool app_bt_pan_demo_arp_v6(uint8_t *bd_addr);

bool app_bt_pan_demo_ping(uint8_t *bd_addr);

bool app_bt_pan_demo_ping_v6(uint8_t *bd_addr);

bool app_bt_pan_demo_ping_reply(uint8_t *bd_addr);

bool app_bt_pan_demo_ping_reply_v6(uint8_t *bd_addr);

/** @} End of APP_BT_PAN_DEMO_Exported_Functions */

/** @} End of APP_BT_PAN_DEMO */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_PAN_DEMO_H_ */
