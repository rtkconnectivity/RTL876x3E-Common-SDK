/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SRC_PLAY_HFP_H_
#define _APP_SRC_PLAY_HFP_H_

#include "rtl876x.h"
#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    HFP_STATE_DISCONN      = 0x00,
    HFP_STATE_CONN         = 0x01,
    HFP_STATE_IND          = 0x02,
} T_SRC_PLAY_HFP_STATE;

typedef enum
{
    SRC_PLAY_HFP_SUCCESS           = 0x00,
    SRC_PLAY_HFP_ERR_DATA_LEN      = 0x01,
    SRC_PLAY_HFP_ERR_RAM           = 0x02,
    SRC_PLAY_HFP_ERR_RINGBUF       = 0x03,
} T_SRC_PLAY_HFP_ERR;

typedef struct
{
    T_AUDIO_FORMAT_INFO             hfp_hf_format;
    bool                            hfp_hf_format_ready;
    uint8_t                         hf_addr[6];
    T_AUDIO_FORMAT_INFO             hfp_ag_format;
    bool                            hfp_ag_format_ready;
    uint8_t                         ag_addr[6];
} T_SRC_PLAY_HFP;

extern T_SRC_PLAY_HFP hfp_play;

void app_src_play_print_hfp_format(const char *title, T_AUDIO_FORMAT_INFO *format_info);
void app_src_play_save_hfp_format(T_AUDIO_FORMAT_INFO *format_info, uint8_t *bd_addr);
bool app_src_play_get_hfp_format(T_AUDIO_FORMAT_INFO *format_info);
void app_src_play_hfp_send_sco(uint8_t *p_data, uint16_t len, uint8_t bd_addr[6]);
bool app_src_play_hfp_start_req(void);
bool app_src_play_hfp_stop(void);
void app_src_play_hfp_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SRC_PLAY_SOURCE_H_ */
