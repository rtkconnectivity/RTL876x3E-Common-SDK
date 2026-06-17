/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SRC_PLAY_H_
#define _APP_SRC_PLAY_H_

#include "stdbool.h"
#include <stdint.h>
#include "app_io_msg.h"

#if F_APP_USB_AUDIO_SUPPORT
#include "usb_audio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SOURCE_DIR_DS_BIT       0x01
#define SOURCE_DIR_US_BIT       0x02

typedef enum
{
    SOURCE_ROUTE_MIC         = 1,
    SOURCE_ROUTE_LINE_IN     = 2,
    SOURCE_ROUTE_USB         = 3,
    SOURCE_ROUTE_SD_CARD     = 4,
    SOURCE_ROUTE_SPDIF       = 5,
    SOURCE_ROUTE_I2S         = 6,
    SOURCE_ROUTE_A2DP        = 7,
    SOURCE_ROUTE_MULTI_DEV   = 8,
    SOURCE_ROUTE_HFP_HF      = 9,
    SOURCE_ROUTE_INVALID     = 0xFF
} T_SOURCE_ROUTE;

typedef enum
{
    PLAY_ROUTE_A2DP         = 1,
    PLAY_ROUTE_HFP_AG       = 2,
    PLAY_ROUTE_BIS          = 3,
    PLAY_ROUTE_CIS          = 4,
    PLAY_ROUTE_LOCAL        = 5,
    PLAY_ROUTE_MULTI_A2DP   = 6,
    PLAY_ROUTE_INVALID      = 0xFF
} T_PLAY_ROUTE;

typedef enum
{
    SRC_PLAY_FEATURE_NREC                = 0x0001,
    SRC_PLAY_FEATURE_APPEND_LOCALPLAY    = 0x0002
} T_ATTACH_FEATURE_BIT;

typedef enum
{
    PIPE_MSG_FILL       = 0,
    SD_PIPE_LC3_TIMER   = 1,
} T_SRC_PLAY_MSG;

bool app_src_play_is_nrec_mode(void);
bool app_src_play_is_local_play_attached(void);
void app_src_play_attach_local_play_handle_data(uint8_t *p_data, uint16_t len, uint16_t seq_num,
                                                uint8_t frame_num, uint32_t timestamp);

bool app_src_play_send_msg(T_SRC_PLAY_MSG sub_type, void *param_buf);

bool app_src_play_set_src_route(T_SOURCE_ROUTE src_route, uint8_t dir);
T_SOURCE_ROUTE app_src_play_get_src_route(void);

void app_src_play_set_play_route(T_PLAY_ROUTE play_route);
T_PLAY_ROUTE app_src_play_get_play_route(void);

bool app_src_play_route_in_start(void);
void app_src_play_route_in_stop(void);

bool app_src_play_route_out_start(void);
bool app_src_play_route_out_stop(void);

void app_src_play_attach_features(uint16_t feature_bits);
void app_src_play_detach_features(uint16_t feature_bits);
bool app_src_play_check_attach_feature_nrec(void);
bool app_src_play_check_attach_feature_append_localplay(void);

bool app_src_play_route_in_pipe_no_existed(bool encode);

void lea_tx_timer_start(void);
void lea_tx_timer_stop(void);

void lea_tx_timer_reload(uint32_t period_us);
void app_src_play_handle_msg(T_IO_MSG *msg);

#if F_APP_USB_AUDIO_SUPPORT
void app_src_play_handle_usb_active(T_USB_AUDIO_PIPE_ATTR attr);
void app_src_play_handle_usb_inactive(uint8_t dir);
#endif

bool app_audio_path_set_mic_vol_gain(uint16_t gain);

bool app_audio_path_set_spk_vol_gain(uint16_t gain);

bool app_src_play_bt_recv(uint8_t *buf, uint16_t len, uint8_t flag, uint32_t timestamp);

void app_src_play_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SRC_PLAY_H_ */
