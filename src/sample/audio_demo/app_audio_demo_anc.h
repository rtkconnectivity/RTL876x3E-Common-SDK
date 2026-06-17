/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AUDIO_DEMO_ANC_H_
#define _APP_AUDIO_DEMO_ANC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_anc_state
{
    ANC_STOPPED,
    ANC_STARTING,
    ANC_STARTED,
    ANC_STOPPING,
} T_ANC_SM_STATE;

/** @defgroup APP_AUDIO_DEMO_ANC Audio Anc
* @brief APP Audio Anc
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_AUDIO_DEMO_ANC_Exported_Macros Audio Anc Exported Macros
  * @brief
  * @{
  */



/** End of APP_AUDIO_DEMO_ANC_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_AUDIO_DEMO_ANC_Exported_Functions Audio Anc Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  Start/Stop Audio Anc
    * @param  void
    * @return bool
    */
bool app_audio_anc_enable(uint8_t);
bool app_audio_anc_disable(void);


/** @} End of APP_AUDIO_DEMO_ANC_Exported_Functions */

/** @} End of APP_AUDIO_DEMO_ANC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_AUDIO_DEMO_ANC_H_ */
