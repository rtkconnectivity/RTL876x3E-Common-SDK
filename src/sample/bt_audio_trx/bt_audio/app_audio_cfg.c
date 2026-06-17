/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "voice_prompt.h"
#include "app_audio_cfg.h"

T_APP_AUDIO_CFG app_audio_cfg;

void app_audio_cfg_init(void)
{
    app_audio_cfg.solo_speaker_channel = CHANNEL_LR_HELF;
    app_audio_cfg.voice_prompt_language = VOICE_PROMPT_LANGUAGE_ENGLISH;
    app_audio_cfg.maximum_packet_lost_compensation_count = 4;

    app_audio_cfg.normal_apt_support = false;
    app_audio_cfg.enable_circular_vol_up = false;
    app_audio_cfg.rws_circular_vol_up_when_solo_mode  = false;
    app_audio_cfg.bud_to_phone_sco_tx_power_control = 0;
    app_audio_cfg.play_max_min_tone_when_adjust_vol_from_phone = false;
}


