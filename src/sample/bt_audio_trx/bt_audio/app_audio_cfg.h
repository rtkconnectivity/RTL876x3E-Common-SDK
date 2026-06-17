/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AUDIO_CFG_H_
#define _APP_AUDIO_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_AUDIO App Audio
  * @brief App Audio
  * @{
  */

#define VOICE_PROMPT_INDEX              0x80
#define TONE_INVALID_INDEX              0xFF

#define ULTRA_LOW_LATENCY_MS                20
#define ULTRA_NONE_LATENCY_MS               0
#define A2DP_LATENCY_MS                     280
#define GAMING_MODE_DYNAMIC_LATENCY_FIX     false
#define RTK_GAMING_MODE_DYNAMIC_LATENCY_FIX true
#define NORMAL_MODE_DYNAMIC_LATENCY_FIX     false

typedef enum
{
    CHANNEL_L = 0x01,
    CHANNEL_R = 0x02,
    CHANNEL_LR_HELF = 0x03,
} T_CHANNEL_TYPE;

/** @brief  Read only configurations for audio */
typedef struct
{
    uint8_t solo_speaker_channel;
    uint8_t voice_prompt_language;
    uint8_t maximum_packet_lost_compensation_count : 3;
    uint8_t normal_apt_support : 1;
    uint8_t enable_circular_vol_up : 1;
    uint8_t rws_circular_vol_up_when_solo_mode : 1;
    uint8_t enable_aux_in_supress_a2dp: 1;
    uint8_t bud_to_phone_sco_tx_power_control : 4;
    uint8_t play_max_min_tone_when_adjust_vol_from_phone : 1;
} T_APP_AUDIO_CFG;

extern T_APP_AUDIO_CFG app_audio_cfg;

/**
    * @brief  Audio config module init
    * @param  void
    * @return void
    */
void app_audio_cfg_init(void);


/** End of APP_AUDIO
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
