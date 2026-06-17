/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_A2DP_XMIT_LEA_H_
#define _APP_A2DP_XMIT_LEA_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */
#include "app_a2dp_xmit_mgr.h"
#include "app_msg.h"

typedef enum
{
    IO_LEA_SRC_SYNC_TIMER,
} IO_LEA_SRC_MSG_TYPE;

void app_a2dp_xmit_lea_msg_handle(T_IO_MSG io_msg);
void app_a2dp_xmit_lea_handle_a2dp_data_ind(uint8_t *p_audio, T_A2DP_XMIT_AUDIO_INFO audio_info);
void app_a2dp_xmit_lea_src_start_stop(T_A2DP_XMIT_PLAY_STATE type);
void app_a2dp_xmit_lea_pipe_rcfg(void);
void app_a2dp_xmit_lea_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
