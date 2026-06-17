/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_MULTILINK_H_
#define _APP_MULTILINK_H_

#include <stdbool.h>
#include <stdint.h>



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_MULTILINK App Multilink
  * @brief App Multilink
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_MULTILINK_Exported_Macros App Multilink Macros
   * @{
   */
#define UPDATE_ACTIVE_A2DP_INDEX_TIMER  2000
#define MULTILINK_SRC_CONNECTED         2
#define TIMER_TO_DISCONN_ACL            800
/** End of APP_MULTILINK_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_MULTILINK_Exported_Types App Multilink Types
    * @{
    */
/** @brief Multilink handle connect, disconnect, related player status event */
typedef enum
{
    JUDGE_EVENT_A2DP_CONNECTED,
    JUDGE_EVENT_MEDIAPLAYER_PLAYING,
    JUDGE_EVENT_MEDIAPLAYER_PAUSED,
    JUDGE_EVENT_A2DP_START,
    JUDGE_EVENT_A2DP_DISC,
    JUDGE_EVENT_DSP_SILENT,
    JUDGE_EVENT_A2DP_STOP,
    JUDGE_EVENT_SNIFFING_STOP,

    JUDGE_EVENT_TOTAL
} T_APP_JUDGE_A2DP_EVENT;

typedef enum
{
    APP_REMOTE_MSG_PROFILE_CONNECTED,
    APP_REMOTE_MSG_PHONE_CONNECTED,
    APP_REMOTE_MSG_MUTE_AUDIO_WHEN_MULTI_CALL_NOT_IDLE,
    APP_REMOTE_MSG_UNMUTE_AUDIO_WHEN_MULTI_CALL_IDLE,
    APP_REMOTE_MSG_RESUME_BT_ADDRESS,
    APP_REMOTE_MSG_ACTIVE_BD_ADDR_SYNC,
    APP_REMOTE_MSG_CONNECTED_TONE_NEED,

    APP_REMOTE_MSG_MULTILINK_TOTAL
} T_MULTILINK_REMOTE_MSG;

/** @brief Multilink update active a2dp link, delay role switch timer */

/** End of APP_MULTILINK_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_MULTILINK_Exported_Functions App Multilink Functions
    * @{
    */
/**
    * @brief  multilink module init
    * @param  void
    * @return void
    */
void app_multilink_init(void);


void app_multi_disc_inactive_link(uint8_t app_idx);


/**
    * @brief  judge one device a2dp link as active link and avrcp can control it.
    *         config BT qos when multilink connect or disconnect.
    * @param  app_idx BT link index
    * @param  event judge a2dp active link event
    * @return void
    */
void app_judge_active_a2dp_idx_and_qos(uint8_t app_idx, T_APP_JUDGE_A2DP_EVENT event);
bool app_pause_other_a2dp_avrcp(uint8_t keep_active_a2dp_idx, uint8_t resume_fg);
void app_resume_a2dp_avrcp(uint8_t app_idx);


void app_multi_active_hfp_transfer(uint8_t idx);
void app_multi_preempt_hfp(uint8_t inactive_idx);


void app_multi_enable(bool is_on_by_mmi);
bool app_multi_is_enabled(void);


uint8_t app_multi_get_acl_connect_num(void);


/** @} */ /* End of group APP_MULTILINK_Exported_Functions */

/** End of APP_MULTILINK
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_MULTILINK_H_ */
