/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _A2DP_SRC_STREAM_H_
#define _A2DP_SRC_STREAM_H_

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include "rtl876x.h"


#define A2DP_SRC_STREAM_DBG             0

#define AUDIO_A2DP_SRC_EVENT_DATA_SEND  0xF0

typedef enum
{
    A2DP_SRC_PLAY_STATE_IDLE     = 0x00,
    A2DP_SRC_PLAY_STATE_PLAY,
    A2DP_SRC_PLAY_STATE_PAUSE,
    A2DP_SRC_PLAY_STATE_TOTAL,
} T_A2DP_SRC_PLAY_STATE;

typedef enum
{
    APP_A2DP_SRC_DISCONN      = 0x00,
    APP_A2DP_SRC_CONN         = 0x01,
    APP_A2DP_SRC_STREAM_STOP  = 0x02,
    APP_A2DP_SRC_STREAM_CONN  = 0x03,
    APP_A2DP_SRC_STREAM_START = 0x04,
} T_APP_A2DP_SRC_STATE;

typedef enum
{
    A2DP_SRC_SUCCESS            = 0x00,
    A2DP_SRC_READ_ERROR         = 0x01,
    A2DP_SRC_IDX_ERROR          = 0x02,
    A2DP_SRC_OPEN_FILE_ERROR    = 0x03,
    A2DP_SRC_CRC_ERROR          = 0x04,
    A2DP_SRC_CLOSE_FILE_ERROR   = 0x05,
    A2DP_SRC_PLAYLIST_ERROR     = 0x06,
    A2DP_SRC_WRITE_ERROR        = 0x07,
    A2DP_SRC_SYS_ERROR          = 0x08,
    A2DP_SRC_PIPE_DRAIN_ERROR   = 0x09,
    A2DP_SRC_ENC_RX_ERROR       = 0x0A,
    A2DP_SRC_DATA_SEND_ERROR    = 0x0B,
    A2DP_SRC_PIPE_FILL_ERROR    = 0x0C,
    A2DP_SRC_PIPE_CREATE_ERROR  = 0x0D,

} T_A2DP_SRC_ERROR;

typedef enum
{
    A2DP_SRC_BUF_NORMAL         = 0x00,
    A2DP_SRC_BUF_LOW            = 0x01,
    A2DP_SRC_BUF_HIGH           = 0x02,
} T_A2DP_SRC_BUF_STATE;

extern void (*a2dp_src_stream_ctrl_stop_complete_hook)(void);
extern void (*a2dp_src_stream_ctrl_delay_stop_hook)(uint16_t time_ms);

/**
  * @brief Initialize.
  * @param   No parameter.
  * @return  execution status
  */

T_A2DP_SRC_PLAY_STATE a2dp_src_stream_get_state(void);

void a2dp_src_stream_volume_up(void);
void a2dp_src_stream_volume_down(void);
bool a2dp_src_stream_volume_set(uint8_t volume);
uint8_t a2dp_src_stream_volume_get(void);

void a2dp_src_stream_handle_msg(T_IO_MSG msg);
void a2dp_src_stream_ctrl_start(void);
void a2dp_src_stream_ctrl_stop(void);

void a2dp_src_stream_ctrl_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _APP_PLAYBACK_H_ */

