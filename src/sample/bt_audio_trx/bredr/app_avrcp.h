/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AVRCP_H_
#define _APP_AVRCP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>


/** @defgroup APP_AVRCP App Avrcp
  * @brief App Avrcp
  * @{
  */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_AVRCP_Exported_Types App avrcp Types
    * @{
    */

/**  @brief  App define bud stream state */
typedef enum
{
    ABS_VOL_NOT_SUPPORTED,
    ABS_VOL_SUPPORTED,
} T_APP_ABS_VOL_STATE;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_AVRCP_Exported_Functions App Avrcp Functions
    * @{
    */

/* @brief  abs vol state sync
*
* @param  void
* @return void
*/
bool app_avrcp_sync_abs_vol_state(uint8_t *bd_addr, T_APP_ABS_VOL_STATE abs_vol_state);

/**
    * @brief  avrcp module init
    * @param  void
    * @return void
    */
void app_avrcp_init(void);

/* @brief check abs vol check timer exist
*
* @param  void
* @return none
*/
bool app_avrcp_abs_vol_check_timer_exist(void);

/* @brief stop abs vol check timer
*
* @param  void
* @return none
*/
void app_avrcp_stop_abs_vol_check_timer(void);

void app_avrcp_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                          uint8_t *ack_pkt);

void app_avcrp_play_pause(void);

void app_avrcp_stop(void);

void app_avcrp_forward(void);

void app_avcrp_backward(void);

void app_avrcp_fastforward(void);

void app_avrcp_rewind(void);

void app_avrcp_fast_forward_stop(void);

void app_avrcp_rewind_stop(void);

/** @} */ /* End of group APP_AVRCP_Exported_Functions */

/** End of APP_AVRCP
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_AVRCP_H_ */
