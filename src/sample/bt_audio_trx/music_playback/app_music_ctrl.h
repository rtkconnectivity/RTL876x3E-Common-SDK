/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _APP_MUSIC_H_
#define _APP_MUSIC_H_

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include "rtl876x.h"
#include "app_report.h"

typedef enum
{
    MUSIC_IDLE           = 0x00,
    MUSIC_LOCAL_PLAY     = 0x01,
    MUSIC_A2DP_SRC       = 0x02,
} T_MUSIC_MODE;

typedef enum
{
    MUSIC_STATE_IDLE     = 0x00,
    MUSIC_STATE_PLAY,
    MUSIC_STATE_PAUSE,
    MUSIC_STATE_TOTAL,
} T_MUSIC_STATE;

/**
 * app_music_ctrl.h
 *
 * \brief  music play status.
 *
 */
typedef enum
{
    MUSIC_PLAYER_STOPPED          = 0x00,
    MUSIC_PLAYER_PLAYING          = 0x01,
    MUSIC_PLAYER_PAUSED           = 0x02,
    MUSIC_PLAYER_FWD_SEEK         = 0x03,
    MUSIC_PLAYER_REV_SEEK         = 0x04,
    MUSIC_PLAYER_FAST_FWD         = 0x05,
    MUSIC_PLAYER_REWIND           = 0x06,
    MUSIC_PLAYER_ERROR            = 0xff,
} T_MUSIC_PLAYER_STATE;

typedef enum
{
    MUSIC_FLOW_IDLE           = 0x00,
    MUSIC_FLOW_JUMP_HEAD      = 0x01,
    MUSIC_FLOW_CREAT_TRACK    = 0x02,
    MUSIC_FLOW_START          = 0x03,
    MUSIC_FLOW_STARTED        = 0x04
} T_MUSIC_FLOW_STATE;

typedef enum
{
    MUSIC_SUCCESS             = 0x00,
    MUSIC_READ_ERROR          = 0x01,
    MUSIC_STATE_ERROR         = 0x02,
    MUSIC_SYS_ERROR           = 0x03,
    MUSIC_CREATE_ERROR        = 0x04,
    MUSIC_NEXT_STATE_ERROR    = 0x05,

} T_MUSIC_ERROR;

/**
  * @brief Initialize.
  * @param   No parameter.
  * @return  execution status
  */
void app_music_handle_xmmi_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                               uint16_t cmd_len, uint8_t *ack_pkt);

uint8_t app_music_set_mode(uint8_t mode);
T_MUSIC_MODE app_music_get_mode(void);
T_MUSIC_STATE app_music_get_state(void);
uint8_t app_music_send_player_status(uint8_t status);
void app_music_report_play_time(void);
void app_music_set_flow_state(uint8_t flow_state);
void app_music_is_short_audio_set(bool value);

uint8_t app_music_create(void);
uint8_t app_music_start(void);
uint8_t app_music_pause(void);
uint8_t app_music_resume(void);
uint8_t app_music_close(void);

void app_music_volume_up(void);
void app_music_volume_down(void);
bool app_music_volume_set(uint8_t volume);
uint8_t app_music_volume_get(void);

void app_music_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _APP_PLAYBACK_H_ */

