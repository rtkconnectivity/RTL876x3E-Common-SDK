/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SCP_XMIT_MGR_H_
#define _APP_SCP_XMIT_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "rtl876x.h"

typedef enum
{
    SCO_XMIT_SUCCESS            = 0x00,
    SCO_XMIT_READ_ERROR         = 0x01,
    SCO_XMIT_IDX_ERROR          = 0x02,
    SCO_XMIT_OPEN_FILE_ERROR    = 0x03,
    SCO_XMIT_CRC_ERROR          = 0x04,
    SCO_XMIT_CLOSE_FILE_ERROR   = 0x05,
    SCO_XMIT_PLAYLIST_ERROR     = 0x06,
    SCO_XMIT_WRITE_ERROR        = 0x07,
    SCO_XMIT_SYS_ERROR          = 0x08,
    SCO_XMIT_PIPE_DRAIN_ERROR   = 0x09,
    SCO_XMIT_ENC_RX_ERROR       = 0x0A,
    SCO_XMIT_DATA_SEND_ERROR    = 0x0B,
    SCO_XMIT_PIPE_FILL_ERROR    = 0x0C,
    SCO_XMIT_PIPE_CREATE_ERROR  = 0x0D,
    SCO_XMIT_MEM_ERROR          = 0x0E,
} T_SCO_XMIT_ERROR;

typedef enum
{
    SCO_STATE_DISCONNECTED           = 0x00,
    SCO_STATE_CONNECTED              = 0x01,
    SCO_STATE_IND                    = 0x02,
} T_SCO_STATE;

void app_sco_xmit_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                 uint16_t cmd_len, uint8_t *ack_pkt);

void app_sco_xmit_send_sco(uint8_t *data, uint16_t len);

void app_sco_xmit_save_output_param(T_AUDIO_FORMAT_INFO *format_info);

uint8_t app_sco_xmit_param_recfg(void);

void app_sco_xmit_handle_sco_param(uint8_t *param, uint16_t param_len);

void app_sco_xmit_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SCP_XMIT_MGR_H_ */
