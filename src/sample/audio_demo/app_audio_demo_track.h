/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AUDIO_DEMO_TRACK_H_
#define _APP_AUDIO_DEMO_TRACK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_AUDIO_DEMO_TRACK Audio Track
* @brief APP Audio Track
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_AUDIO_DEMO_TRACK_Exported_Macros Audio Track Exported Macros
  * @brief
  * @{
  */



/** End of APP_AUDIO_DEMO_TRACK_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_AUDIO_DEMO_TRACK_Exported_Functions Audio Track Exported Functions
  * @brief
  * @{
  */

void app_audio_track_write(void);

bool app_audio_track_create(uint8_t stream_type,
                            uint8_t format_type,
                            uint8_t volume_out,
                            uint8_t volume_in);
bool app_audio_track_start(void);
bool app_audio_track_stop(void);
bool app_audio_track_pause(void);
bool app_audio_track_restart(void);
bool app_audio_track_release(void);
bool app_audio_track_volume_out_set(uint8_t volume_out);
bool app_audio_track_volume_out_mute(void);
bool app_audio_track_volume_out_unmute(void);

/** @} End of APP_AUDIO_DEMO_TRACK_Exported_Functions */

/** @} End of APP_AUDIO_DEMO_TRACK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_AUDIO_DEMO_TRACK_H_ */
