/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_ACC_DEF_H_
#define _APP_LEA_ACC_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#if F_APP_LE_AUDIO_REF_CLK
#include "syncclk_driver.h"
#define LEA_SYNC_CLK_REF SYNCCLK_ID4
#endif

//Setting 1 due to DSP doesn't support LC3 multi-frame*/
#define FIXED_FRAME_NUM           1

typedef enum
{
    LEA_CONNECT                  = 0x20,
    LEA_DISCONNECT               = 0x21,
    LEA_CODEC_CONFIGURED         = 0x22,
    LEA_PREFER_QOS               = 0x23,
    LEA_ENABLE                   = 0x25,
    LEA_ENABLEING                = 0x26,
    LEA_SETUP_DATA_PATH          = 0x27,
    LEA_REMOVE_DATA_PATH         = 0x28,
    LEA_STREAMING                = 0x29,
    LEA_PAUSE                    = 0x2A,
    LEA_A2DP_START               = 0x2B,
    LEA_A2DP_SUSPEND             = 0x2C,
    LEA_AVRCP_PLAYING            = 0x2D,
    LEA_AVRCP_PAUSE              = 0x2E,
    LEA_MCP_STATE                = 0x2F,
    LEA_HFP_CALL_STATE           = 0x30,
    LEA_CCP_CALL_STATE           = 0x31,
    LEA_CCP_READ_RESULT          = 0x32,
    LEA_CCP_TERM_NOTIFY          = 0x33,
    LEA_SNIFFING_STOP            = 0x34,
    LEA_PAUSE_LOCAL              = 0x35,
    LEA_DISCON_LOCAL             = 0x36,
    LEA_CONTEXTS_CHECK           = 0x37,
    LEA_METADATA_UPDATE          = 0x38,
    LEA_RELEASING                = 0x39,
} T_LEA_LINK_EVENT;

typedef enum
{
    LEA_LINK_IDLE                = 0x00,
    LEA_LINK_CONNECTED           = 0x01,
    LEA_LINK_STREAMING           = 0x02,
} T_LEA_LINK_STATE;


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
