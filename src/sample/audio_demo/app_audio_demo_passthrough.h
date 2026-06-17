/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AUDIO_DEMO_PASSTHROUGH_H_
#define _APP_AUDIO_DEMO_PASSTHROUGH_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_apt_state
{
    APT_STOPPED,
    APT_STARTING,
    APT_STARTED,
    APT_STOPPING,
} T_APT_SM_STATE;

/** @defgroup APP_AUDIO_DEMO_PASSTHROUGH Audio Passthrough
* @brief APP Audio Passthrough
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_AUDIO_DEMO_PASSTHROUGH_Exported_Macros Audio Passthrough Exported Macros
  * @brief
  * @{
  */



/** End of APP_AUDIO_DEMO_PASSTHROUGH_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_AUDIO_DEMO_PASSTHROUGH_Exported_Functions Audio Passthrough Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  Init Audio Passthrough
    * @param  void
    * @return void
    */
void app_audio_passthrough_init(void);

/**
    * @brief  Enable/Disable Audio Passthrough
    * @param  void
    * @return bool
    */
bool app_audio_normal_apt_enable(void);
bool app_audio_llapt_enable(uint8_t);
bool app_audio_normal_apt_disable(void);
bool app_audio_llapt_disable(void);

/** @} End of APP_AUDIO_DEMO_PASSTHROUGH_Exported_Functions */

/** @} End of APP_AUDIO_DEMO_PASSTHROUGH */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_AUDIO_DEMO_PASSTHROUGH_H_ */
