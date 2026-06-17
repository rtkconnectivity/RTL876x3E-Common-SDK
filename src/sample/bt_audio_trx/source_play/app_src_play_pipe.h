/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SRC_PLAY_PIPE_H_
#define _APP_SRC_PLAY_PIPE_H_

#include "rtl876x.h"
#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    SRC_PIPE_PLAY_STATE_START,
    SRC_PIPE_PLAY_STATE_IDLE,
} T_SRC_PIPE_PLAY_STATE;

typedef enum
{
    SRC_PIPE_DOWNSTREAM_PLAY_TYPE,
    SRC_PIPE_UPSTREAM_PLAY_TYPE,
} T_SRC_PIPE_TYPE;

typedef enum
{
    SRC_PIPE_MGR_SUCCESS            = 0x00,
    SRC_PIPE_MGR_READ_ERROR         = 0x01,
    SRC_PIPE_MGR_WRITE_ERROR        = 0x02,
    SRC_PIPE_MGR_MEM_ERROR          = 0x03,
    SRC_PIPE_MGR_PIPE_FILL_ERROR    = 0x04,
    SRC_PIPE_MGR_PIPE_DRAIN_ERROR   = 0x05,
    SRC_PIPE_MGR_PIPE_CREATE_ERROR  = 0x06,
    SRC_PIPE_MGR_DATA_SEND_ERROR    = 0x07,
    SRC_PIPE_MGR_SYS_ERROR          = 0x08,
} T_SRC_PIPE_MGR_ERROR;

bool app_src_play_pipe_handle_a2dp_stream_data(uint8_t *p_data, uint16_t data_len,
                                               uint8_t frame_number, uint16_t seq_num);
bool app_src_play_pipe_handle_sco_stream_data(uint8_t *p_data, uint16_t data_len,
                                              uint8_t bd_addr[6], bool is_hfp_ag_data);
bool app_src_play_downstream_pipe_start(uint8_t src_route, uint8_t play_route);
bool app_src_play_upstream_pipe_start(uint8_t src_route, uint8_t play_route);
void app_src_play_pipe_stop(void);
void app_src_play_pipe_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SRC_PLAY_SOURCE_H_ */
