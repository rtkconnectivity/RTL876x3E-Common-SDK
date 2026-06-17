/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _PLAYBACK_STREAM_CTRL_H_
#define _PLAYBACK_STREAM_CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include "rtl876x.h"
#include "playback_audio_file.h"


#define PLAYBACK_TRACK_STATE_CLEAR                  0xFF


typedef enum
{
    PLAYBACK_STATE_IDLE     = 0x00,
    PLAYBACK_STATE_PLAY,
    PLAYBACK_STATE_PAUSE,
    PLAYBACK_STATE_TOTAL,
} T_PLAYBACK_STATE;

typedef enum
{
    PLAYBACK_SUCCESS            = 0x00,
    PLAYBACK_READ_ERROR         = 0x01,
    PLAYBACK_IDX_ERROR          = 0x02,
    PLAYBACK_OPEN_FILE_ERROR    = 0x03,
    PLAYBACK_CRC_ERROR          = 0x04,
    PLAYBACK_CLOSE_FILE_ERROR   = 0x05,
    PLAYBACK_PLAYLIST_ERROR     = 0x06,
    PLAYBACK_WRITE_ERROR        = 0x07,
    PLAYBACK_SYS_ERROR          = 0x08,
    PLAYBACK_PIPE_DRAIN_ERROR   = 0x09,
    PLAYBACK_ENC_RX_ERROR       = 0x0A,
    PLAYBACK_DATA_SEND_ERROR    = 0x0B,
    PLAYBACK_PIPE_FILL_ERROR    = 0x0C,
    PLAYBACK_PIPE_CREATE_ERROR  = 0x0D,

} T_PLAYBACK_ERROR;

typedef enum
{
    PLAYBACK_BUF_NORMAL         = 0x00,
    PLAYBACK_BUF_LOW            = 0x01,
    PLAYBACK_BUF_HIGH           = 0x02,
} T_PLAYBACK_BUF_STATE;


typedef struct
{
    T_AUDIO_FORMAT_INFO         format_info;
    uint16_t                    frame_duration_ms;
    uint16_t                    sample_counts;
} T_PLAYBACK_FORMAT_INFO;

typedef struct
{
    uint16_t latency;
    uint16_t low_level;
    uint16_t upper_level;
    uint16_t play_duration;
    uint16_t preq_pkts;
    uint8_t frame_size;
} T_PLAY_SET_INFO;

extern void (*playback_stream_ctrl_stop_complete_hook)(void);
extern void (*playback_stream_ctrl_delay_stop_hook)(uint16_t time_ms);

/**
  * @brief Initialize.
  * @param   No parameter.
  * @return  execution status
  */

T_PLAYBACK_STATE playback_stream_get_state(void);
uint8_t playback_stream_get_music_info(T_PLAYBACK_AF_FORMAT_INFO fmt_info,
                                       T_PLAY_SET_INFO *set_info);

bool playback_stream_get_sample_rate(uint32_t *p_sample_rate);
uint8_t playback_stream_ctrl_start(void);
uint8_t playback_stream_ctrl_stop(void);

void playback_stream_volume_up(void);
void playback_stream_volume_down(void);
bool playback_stream_volume_set(uint8_t volume);
uint8_t playback_stream_volume_get(void);

void playback_stream_ctrl_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _APP_PLAYBACK_H_ */


