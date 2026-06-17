/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef _APP_AUDIO_POLICY_H_
#define _APP_AUDIO_POLICY_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_AUDIO_POLICY Audio Policy
* @brief APP Audio Policy
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_AUDIO_POLICY_Exported_Macros Audio Policy Exported Macros
  * @brief
  * @{
  */



/** End of APP_AUDIO_POLICY_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_AUDIO_POLICY_Exported_Functions Audio Policy Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  Register Audio policy cback
    * @param  void
    * @return void
    */
void app_audio_init(void);
bool app_audio_notification_play(uint8_t tone_index,  bool flush, bool relay);

/** @} End of APP_AUDIO_POLICY_Exported_Functions */

/** @} End of APP_AUDIO_POLICY */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_AUDIO_POLICY_H_ */
