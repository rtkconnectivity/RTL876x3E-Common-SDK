/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_AUDIO_HFP_H_
#define _APP_BT_AUDIO_HFP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_AUDIO_HFP BT Audio HFP
* @brief APP BT Audio HFP
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_BT_AUDIO_HFP_Exported_Macros BT Audio HFP Exported Macros
  * @brief
  * @{
  */

#define BT_AUDIO_HFP_ROLE_HF  0
#define BT_AUDIO_HFP_ROLE_AG  1

/** End of APP_BT_AUDIO_HFP_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BT_AUDIO_HFP_Exported_Functions BT Audio HFP Exported Functions
  * @brief
  * @{
  */

void app_bt_audio_hfp_init(uint8_t role);

bool app_bt_audio_hfp_sco_connect(uint8_t *bd_addr);

bool app_bt_audio_hfp_sco_disconnect(uint8_t *bd_addr);

bool app_bt_audio_hfp_connect(uint8_t *bd_addr);

bool app_bt_audio_hfp_disconnect(uint8_t *bd_addr);

bool app_bt_audio_hfp_call_incoming(uint8_t *bd_addr);

bool app_bt_audio_hfp_call_answer(uint8_t *bd_addr);

bool app_bt_audio_hfp_call_terminate(uint8_t *bd_addr);

bool app_bt_audio_hfp_vendor_at_cmd(uint8_t *bd_addr);

/** @} End of APP_BT_AUDIO_HFP_Exported_Functions */

/** @} End of APP_BT_AUDIO_HFP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_AUDIO_HFP_H_ */
