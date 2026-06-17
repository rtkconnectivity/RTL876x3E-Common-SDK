/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_AUDIO_AVRCP_H_
#define _APP_BT_AUDIO_AVRCP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_AUDIO_AVRCP BT Audio AVRCP
* @brief APP BT Audio AVRCP
* @{
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BT_AUDIO_AVRCP_Exported_Functions BT Audio AVRCP Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  AVRCP init
    * @param  void
    * @return void
    */
void app_bt_audio_avrcp_init(void);

bool app_bt_audio_avrcp_cover_art_connect(uint8_t *bd_addr);

bool app_bt_audio_avrcp_cover_art_disconnect(uint8_t *bd_addr);

bool app_bt_audio_avrcp_cover_art_get(uint8_t *bd_addr);

bool app_bt_audio_avrcp_element_attr_get(uint8_t *bd_addr, uint8_t attr_id);

/** @} End of APP_BT_AUDIO_AVRCP_Exported_Functions */

/** @} End of APP_BT_AUDIO_AVRCP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_AUDIO_AVRCP_H_ */
