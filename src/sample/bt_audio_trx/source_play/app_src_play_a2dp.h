/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SRC_PLAY_A2DP_H_
#define _APP_SRC_PLAY_A2DP_H_

#include "rtl876x.h"
#include "app_flags.h"
#include "audio_type.h"
#include "ring_buffer.h"
#include "app_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    A2DP_STATE_DISCONN          = 0x00,
    A2DP_STATE_CONN             = 0x01,
    A2DP_STATE_STREAM_STOP      = 0x02,
    A2DP_STATE_STREAM_OPEN      = 0x03,
    A2DP_STATE_STREAM_START     = 0x04,
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    A2DP_STATE_STREAM_START_RSP = 0x05,
#endif
} T_SRC_PLAY_A2DP_STATE;

typedef enum
{
    SRC_PLAY_A2DP_SUCCESS           = 0x00,
    SRC_PLAY_A2DP_ERR_DATA_LEN      = 0x01,
    SRC_PLAY_A2DP_CREDIT_LOW        = 0x02,
    SRC_PLAY_A2DP_ERR_LOWERSTACK    = 0x03,
    SRC_PLAY_A2DP_ERR_RAM           = 0x04,
    SRC_PLAY_A2DP_ERR_RINGBUF       = 0x05,
} T_SRC_PLAY_A2DP_ERR;

typedef struct
{
    uint8_t multi_a2dp_credits;
    uint16_t multi_a2dp_seq_num;
    uint32_t multi_a2dp_timestamp;
    bool a2dp_is_used;
} T_MULTI_A2DP_PARAM;

typedef struct
{
    T_AUDIO_FORMAT_INFO             src_a2dp_format;
    bool                            src_a2dp_format_ready;
    T_AUDIO_FORMAT_INFO             sink_a2dp_format[MAX_BR_LINK_NUM];
    bool                            sink_a2dp_format_ready[MAX_BR_LINK_NUM];
    T_SRC_PLAY_A2DP_STATE           sink_a2dp_state[MAX_BR_LINK_NUM];
    T_MULTI_A2DP_PARAM              sink_a2dp_param[MAX_BR_LINK_NUM];
    uint8_t                         num_frame_buf;
    uint8_t                         *p_buf;
    T_RING_BUFFER                   ring_buf;
    uint8_t                         sink_addr[MAX_BR_LINK_NUM][6];
    uint8_t                         src_addr[6];
} T_SRC_PLAY_A2DP;

extern T_SRC_PLAY_A2DP a2dp_play;

uint8_t app_src_play_a2dp_get_connected_idx(uint8_t *bd_addr);
bool app_src_play_a2dp_get_idle_idx(uint8_t *idle_idx);
void app_src_play_save_a2dp_format(T_AUDIO_FORMAT_INFO *format_info, uint8_t *bd_addr,
                                   uint8_t role);
bool app_src_play_get_a2dp_format(T_AUDIO_FORMAT_INFO *format_info);
bool app_src_play_a2dp_start_req(void);
bool app_src_play_a2dp_stop(void);
void app_src_play_print_a2dp_format(const char *title, T_AUDIO_FORMAT_INFO *format_info);
T_SRC_PLAY_A2DP_STATE app_src_play_get_a2dp_state(uint8_t link_idx);
void app_src_play_a2dp_init(void);
uint16_t app_src_play_a2dp_handle_data(uint8_t *p_data, uint16_t data_len, uint8_t frame_number);
uint16_t app_src_play_multi_a2dp_handle_data(uint8_t *p_data, uint16_t data_len, uint8_t active_idx,
                                             uint8_t frame_number);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SRC_PLAY_SOURCE_H_ */
