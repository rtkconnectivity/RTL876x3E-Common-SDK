/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_ACC_ASCS_DEF_H_
#define _APP_LEA_ACC_ASCS_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "codec_def.h"
#include "audio_track.h"

#define ASCS_ASE_ENTRY_NUM 6 /** max ASE entry num */

typedef struct t_lea_ase_entry
{
    bool used;
    bool control_point_enable;
    uint8_t ase_id;
    uint8_t cig_id;
    uint8_t cis_id;
    uint8_t prequeue_pkt_cnt;
    uint16_t pkt_seq;
    uint16_t conn_handle;
    uint16_t cis_conn_handle;
    uint16_t audio_context;
    uint8_t frame_num;
    uint8_t handshake_fg;
    uint8_t path_direction;
    uint8_t bit_depth;
    T_AUDIO_TRACK_STATE track_state;
    T_AUDIO_TRACK_HANDLE track_handle;
    T_AUDIO_STREAM_TYPE stream_type;
    uint16_t track_write_len;
    T_AUDIO_FORMAT_TYPE codec_type;
    T_CODEC_CFG codec_cfg;
    uint32_t presentation_delay;
    T_AUDIO_EFFECT_INSTANCE sidetone_instance;
} T_LEA_ASE_ENTRY;

/** @defgroup APP_LEA_ASCS App LE Audio ASCS
  * @brief this file handle ASCS profile related process
  * @{
  */
typedef enum
{
    LEA_ASE_DEVICE_INOUT        = 0x01,
    LEA_ASE_UP_DIRECT           = 0x02,
    LEA_ASE_DOWN_DIRECT         = 0x03,
    LEA_ASE_ASE_ID              = 0x04,
    LEA_ASE_CONN                = 0x05,
    LEA_ASE_CIS_CONN            = 0x06,
    LEA_ASE_TRACK               = 0x07,
    LEA_ASE_CONTEXT             = 0x08,
} T_LEA_ASE_TYPE;

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
