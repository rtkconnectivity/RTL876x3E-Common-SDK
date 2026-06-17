/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AUDIO_DEMO_LINE_IN_H_
#define _APP_AUDIO_DEMO_LINE_IN_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_AUDIO_DEMO_LINE_IN Audio Line_in
* @brief APP Audio Line_in
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_AUDIO_DEMO_LINE_IN_Exported_Macros Audio Line_in Exported Macros
  * @brief
  * @{
  */



/** End of APP_AUDIO_DEMO_LINE_IN_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_AUDIO_DEMO_LINE_IN_Exported_Functions Audio Line_in Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  Init Audio Line_in
    * @param  void
    * @return void
    */
void app_audio_line_in_init(void);

/**
    * @brief  Start/Stop Audio Line_in
    * @param  void
    * @return bool
    */
bool app_audio_line_in_start(void);
bool app_audio_line_in_stop(void);

/** @} End of APP_AUDIO_DEMO_LINE_IN_Exported_Functions */

/** @} End of APP_AUDIO_DEMO_LINE_IN */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_AUDIO_DEMO_LINE_IN_H_ */
