/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CUSTOMER_AUDIO_POLICY_H_
#define _APP_CUSTOMER_AUDIO_POLICY_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "audio_type.h"
#include "app_cmd.h"

/*
from bt_a2dp.h
*/
#define APP_AUDIO_CODEC_TYPE_SBC                 0x00
#define APP_AUDIO_CODEC_TYPE_AAC                 0x02
#define APP_AUDIO_CODEC_TYPE_USAC                0x03
#define APP_AUDIO_CODEC_TYPE_LDAC                0x08
#define APP_AUDIO_CODEC_TYPE_LC3                 0x0f
#define APP_AUDIO_CODEC_TYPE_MP3                 0xF0
#define APP_AUDIO_CODEC_TYPE_PCM                 0xF1
#define APP_AUDIO_CODEC_TYPE_VENDOR              0xff

typedef enum
{
    APP_AUDIO_STATUS_STOP     = 0x00,
    APP_AUDIO_STATUS_PLAYING  = 0x01,
    APP_AUDIO_STATUS_PAUSED   = 0x02,
    APP_AUDIO_STATUS_FWD_SEED = 0x03,
    APP_AUDIO_STATUS_REV_SEED = 0x04,
} T_APP_AUDIO_PLAY_STATUS;

void app_customer_audio_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path,
                                   uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t *ack_pkt);

void app_customer_audio_handle_test_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                                        uint16_t cmd_len, uint8_t *ack_pkt);

void app_customer_audio_all_bypassed(bool bypassed);

void app_customer_audio_set_tts_path(T_CMD_PATH path);

void app_customer_report_play_status(uint8_t play_status, uint8_t codec_type);

void app_customer_report_play_time(uint32_t play_time);

void app_customer_audio_request_frame(void);

uint8_t app_customer_tts_ack_status(uint8_t type);

void app_customer_audio_init(void);

#endif
