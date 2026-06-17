/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BROADCASTER_GAP_H__
#define _APP_BROADCASTER_GAP_H__

#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include <gap_le.h>

/** @defgroup BROADCASTER_APP Broadcaster Application
  * @brief Broadcaster Application
  * @{
  */


/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief app_broadcaster_gap_handle_io_msg
          All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void app_broadcaster_gap_handle_io_msg(T_IO_MSG io_msg);

/**
  * @brief app_broadcaster_gap_init
           Initialize broadcaster related parameters
  * @return void
  */
void app_broadcaster_gap_init(void);

/** End of BROADCASTER_APP
* @}
*/


#ifdef __cplusplus
}
#endif

#endif

