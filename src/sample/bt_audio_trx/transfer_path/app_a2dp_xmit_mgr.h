/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_A2DP_XMIT_MGR_H_
#define _APP_A2DP_XMIT_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "rtl876x.h"
#include "audio_type.h"

typedef enum
{
    XMIT_PLAY_ROUTE_A2DP_SRC     = 0,
    XMIT_PLAY_ROUTE_BIS          = 1,
    XMIT_PLAY_ROUTE_INVALID      = 0xFF,
} T_A2DP_XMIT_PLAY_ROUTE;

typedef enum
{
    XMIT_PLAY_STATE_START,
    XMIT_PLAY_STATE_IDLE,
} T_A2DP_XMIT_PLAY_STATE;

typedef enum
{
    A2DP_XMIT_MGR_SUCCESS            = 0x00,
    A2DP_XMIT_MGR_READ_ERROR         = 0x01,
    A2DP_XMIT_MGR_WRITE_ERROR        = 0x02,
    A2DP_XMIT_MGR_MEM_ERROR          = 0x03,
    A2DP_XMIT_MGR_PIPE_FILL_ERROR    = 0x04,
    A2DP_XMIT_MGR_PIPE_DRAIN_ERROR   = 0x05,
    A2DP_XMIT_MGR_PIPE_CREATE_ERROR  = 0x06,
    A2DP_XMIT_MGR_DATA_SEND_ERROR    = 0x07,
    A2DP_XMIT_MGR_SYS_ERROR          = 0x08,
} T_A2DP_XMIT_MGR_ERROR;

typedef struct
{
    uint8_t     frame_num;
    uint16_t    len;
} __attribute__((packed)) T_A2DP_XMIT_AUDIO_INFO;

void app_a2dp_xmit_mgr_init(void);

bool app_a2dp_xmit_mgr_get_a2dp_in_format(uint8_t *format_info);
void app_a2dp_xmit_mgr_print_format(const char *title, T_AUDIO_FORMAT_INFO format_info);

void app_a2dp_xmit_mgr_mgr_a2dp_raw_data_clear(void);
uint16_t app_a2dp_xmit_mgr_a2dp_raw_data_read(uint8_t *p_data, uint32_t len);
uint16_t app_a2dp_xmit_mgr_a2dp_raw_data_write(uint8_t *p_data, T_A2DP_XMIT_AUDIO_INFO audio_info);

void app_a2dp_xmit_mgr_save_a2dp_snk_param(uint8_t *a2dp_param, uint16_t len);
void app_a2dp_xmit_mgr_report_a2dp_param(void);
void app_a2dp_xmit_mgr_report_a2dp_data(uint8_t *p_data, uint16_t len);

void app_a2dp_xmit_mgr_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                      uint16_t cmd_len, uint8_t *ack_pkt);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_A2DP_XMIT_MGR_H_ */
