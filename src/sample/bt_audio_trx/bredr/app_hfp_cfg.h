/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_HFP_CFG_H_
#define _APP_HFP_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_HFP App HFP
  * @brief App HFP
  * @{
  */
/** @brief  Read only configurations for hfp */
typedef struct
{
    uint8_t hfp_link_number;
    uint16_t hfp_hf_brsf_capability;
    uint16_t hfp_ag_brsf_capability;

    uint8_t timer_no_service_warning;

    uint8_t enable_auto_answer_incoming_call : 1;
    uint8_t enable_last_num_redial_always_by_first_phone: 1;
    uint8_t enable_last_num_redial_always_by_last_phone: 1;
    uint8_t always_play_hf_incoming_tone_when_incoming_call : 1;

    uint8_t fixed_inband_tone_gain : 1;
    uint8_t inband_tone_gain_lv : 4;

    uint8_t timer_hfp_auto_answer_call;
    uint8_t timer_mic_mute_alarm;

} T_APP_HFP_CFG;


extern T_APP_HFP_CFG app_hfp_cfg;


/**
    * @brief  HFP config module init
    * @param  void
    * @return void
    */
void app_hfp_cfg_init(void);



/** End of APP_HFP
* @}
*/

#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
