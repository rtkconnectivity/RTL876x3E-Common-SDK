/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_MAP_H_
#define _APP_BT_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_MAP BT MAP
* @brief APP BT MAP
* @{
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BT_MAP_Exported_Functions BT MAP Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  MAP init
    * @param  void
    * @return void
    */
void app_map_demo_init(void);

bool app_map_demo_connect(uint8_t *bd_addr);

bool app_map_demo_disconnect(uint8_t *bd_addr);

bool app_map_demo_msg_notification_set(uint8_t *bd_addr, bool enable);

bool app_map_demo_folder_set(uint8_t *bd_addr, uint8_t folder);

bool app_map_demo_folder_listing_get(uint8_t *bd_addr);

bool app_map_demo_msg_listing_get(uint8_t *bd_addr);

/** @} End of APP_BT_MAP_Exported_Functions */

/** @} End of APP_BT_MAP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_MAP_H_ */
