/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_AUDIO_A2DP_H_
#define _APP_BT_AUDIO_A2DP_H_

#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_AUDIO_A2DP BT Audio A2DP
* @brief APP BT Audio A2DP
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_BT_AUDIO_A2DP_Exported_Macros BT Audio A2DP Exported Macros
  * @brief
  * @{
  */



/** End of APP_BT_AUDIO_A2DP_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BT_AUDIO_A2DP_Exported_Functions BT Audio A2DP Exported Functions
  * @brief
  * @{
  */

void app_bt_audio_a2dp_init(uint8_t role);

void app_bt_audio_a2dp_handle_msg(T_IO_MSG msg);

bool app_bt_audio_a2dp_connect(uint8_t *bd_addr);

bool app_bt_audio_a2dp_disconnect(uint8_t *bd_addr);

bool app_bt_audio_a2dp_start(uint8_t *bd_addr);

bool app_bt_audio_a2dp_suspend(uint8_t *bd_addr);

bool app_bt_audio_a2dp_volume_up(uint8_t *bd_addr);

bool app_bt_audio_a2dp_volume_down(uint8_t *bd_addr);

/** @} End of APP_BT_AUDIO_A2DP_Exported_Functions */

/** @} End of APP_BT_AUDIO_A2DP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_AUDIO_A2DP_H_ */
