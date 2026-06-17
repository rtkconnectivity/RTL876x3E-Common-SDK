/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_A2DP_XMIT_SRC_H_
#define _APP_A2DP_XMIT_SRC_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "rtl876x.h"
#include "app_a2dp_xmit_mgr.h"

typedef enum
{
    A2DP_XMIT_SRC_DISCONN      = 0x00,
    A2DP_XMIT_SRC_CONN         = 0x01,
    A2DP_XMIT_SRC_STREAM_STOP  = 0x02,
    A2DP_XMIT_SRC_STREAM_CONN  = 0x03,
    A2DP_XMIT_SRC_STREAM_START = 0x04,
} T_A2DP_XMIT_SRC_STREAM_STATE;

void app_a2dp_xmit_src_init(void);
void app_a2dp_xmit_src_start_stop(T_A2DP_XMIT_PLAY_STATE type);
void app_a2dp_xmit_src_handle_a2dp_data_ind(uint8_t *data, T_A2DP_XMIT_AUDIO_INFO audio_info);
void app_a2dp_xmit_src_save_a2dp_out_param(uint8_t *format_info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_A2DP_XMIT_SRC_H_ */
