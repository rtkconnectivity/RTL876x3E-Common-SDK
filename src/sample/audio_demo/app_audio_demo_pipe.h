/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AUDIO_DEMO_PIPE_H_
#define _APP_AUDIO_DEMO_PIPE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_AUDIO_DEMO_PIPE Audio Pipe
* @brief APP Audio Pipe
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_AUDIO_DEMO_PIPE_Exported_Macros Audio Pipe Exported Macros
  * @brief
  * @{
  */



/** End of APP_AUDIO_DEMO_PIPE_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_AUDIO_DEMO_PIPE_Exported_Functions Audio Pipe Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  Create/Start/Stop/Release Audio Pipe
    * @param  void
    * @return bool
    */
bool app_audio_pipe_create(uint8_t src_type, uint8_t snk_type);
bool app_audio_pipe_start(void);
bool app_audio_pipe_stop(void);
bool app_audio_pipe_release(void);

/** @} End of APP_AUDIO_DEMO_PIPE_Exported_Functions */

/** @} End of APP_AUDIO_DEMO_PIPE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_AUDIO_DEMO_PIPE_H_ */
