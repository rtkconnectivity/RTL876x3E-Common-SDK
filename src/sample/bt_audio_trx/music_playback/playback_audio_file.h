/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _PLAYBACK_AUDIO_FILE_H_
#define _PLAYBACK_AUDIO_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include "rtl876x.h"
#include "audio_type.h"


#define STREAM_BUF_SIZE             (1024 * 20)
#define STREAM_BUF_CHECK_LEVEL      (1024 * 7)
#define STREAM_BUF_LOW_LEVEL        (1024 * 15)
#define STREAM_BUF_HIGH_LEVEL       (STREAM_BUF_SIZE - 2048)
#define STREAM_BUF_EMPTY_LEVEL      (1024 * 2)
#define STREAM_BUF_REMEDY_SIZE      (512 * 3)


extern uint8_t *g_p_ring_buf;
extern void (*playback_audio_report_playtime_hook)(void);

typedef void *T_PLAYBACK_AF_HANDLE; // playback audio file stream

typedef enum
{
    PLAYBACK_AUDIO_FILE_START       = 0x00,
    PLAYBACK_AUDIO_FILE_STARTING    = 0x01,
    PLAYBACK_AUDIO_FILE_STOPPING    = 0x02,
    PLAYBACK_AUDIO_FILE_STOPPED     = 0x03,
} T_PLAYBACK_AUDIO_FILE_STATE;

typedef enum
{
    AUDIO_STREAM_SUCCESS    = 0x00,
    AUDIO_STREAM_READ_ERROR,
    AUDIO_STREAM_WRITE_ERROR,
} T_AUDIO_STREAM_STATE;

typedef enum
{
    PLAYBACK_AF_ERR_HEADER          = 0x00,
    PLAYBACK_AF_NOT_HEADER          = 0x01,
    PLAYBACK_AF_IS_HEADER           = 0x02,
    PLAYBACK_AF_DATA_FRAME          = 0x03,
} T_PLAYBACK_AF_HEADER_TYPE;

typedef struct
{
    T_AUDIO_FORMAT_INFO format_info;
    uint16_t sample_counts;
    uint16_t frame_duration;
    uint16_t frame_size;
} T_PLAYBACK_AF_FORMAT_INFO;

typedef struct
{
    uint8_t                 frame_num;
    uint16_t                length;
    uint8_t                 *buf;
} T_PLAYBACK_FRAME_PKT;

typedef struct
{
    uint16_t                    file_idx;
    uint16_t                    name_len;
    uint32_t                    file_offset;
    uint16_t                    volume;
    uint8_t                     playlist_idx;

} T_PLAYBACK_AF_INFO_TYPE;



typedef enum
{
    PLAYBACK_AF_SUCCESS            = 0x00,
    PLAYBACK_AF_READ_ERROR         = 0x01,
    PLAYBACK_AF_END_ERROR          = 0x02,
    PLAYBACK_AF_OPEN_FILE_ERROR    = 0x03,
    PLAYBACK_AF_CRC_ERROR          = 0x04,
    PLAYBACK_AF_CLOSE_FILE_ERROR   = 0x05,
    PLAYBACK_AF_LENGTH_ERROR       = 0x06,
    PLAYBACK_AF_WRITE_ERROR        = 0x07,
    PLAYBACK_AF_MALLOC_ERROR       = 0x08,
    PLAYBACK_AF_TYPE_ERROR,
} T_PLAYBACK_AF_ERROR;

/** @defgroup APP_PLAYBACK App playback
  * @brief App playback
  * @{
  */


/**
  * @brief Initialize.
  * @param   No parameter.
  * @return  execution status
  */
uint8_t playback_audio_file_init(void);


uint8_t playback_audio_file_open(uint8_t *pFilename, uint16_t namelen);
uint8_t playback_audio_file_close(void);
uint8_t playback_audio_file_get_audio_info(T_PLAYBACK_AF_FORMAT_INFO *as_format_info);
uint8_t playback_audio_file_get_frame(T_PLAYBACK_FRAME_PKT *p_frame_pkt);
bool playback_audio_file_is_end(void);
void playback_audio_file_set_state(uint8_t file_state);
uint8_t playback_audio_file_get_state(void);
uint8_t playback_audio_file_get_header(uint32_t *p_length);

uint8_t playback_audio_write_stream(uint8_t *data, uint16_t len);
uint8_t stream_check_and_request_data(void);
uint32_t stream_get_data_size(void);
uint16_t stream_peek_data(uint8_t *rd_buf, uint32_t len, uint32_t *real_len);
uint16_t stream_peek_data_2(uint8_t *rd_buf, uint32_t offset, uint32_t len, uint32_t *real_len);
uint16_t stream_remove_data(uint32_t len, uint32_t *real_len);
uint16_t stream_read_data(uint8_t *rd_buf, uint32_t len, uint32_t *real_len);
void stream_clear_data(void);
void playback_audio_file_offset_reset(void);
void playback_audio_file_set_play_time(uint32_t play_time_ms);
uint32_t playback_audio_file_get_play_time(void);

#ifdef __cplusplus
}
#endif

#endif /* _PLAYBACK_AUDIO_FILE_H_ */


