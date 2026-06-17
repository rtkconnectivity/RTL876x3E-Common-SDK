/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_AUDIO_DATA_H_
#define _APP_LEA_CAP_ACC_AUDIO_DATA_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "codec_def.h"
#include "ble_isoch_def.h"

#define CIG_ISO_MODE        0x01
#define BIG_ISO_MODE        0x02

typedef struct t_lea_iso_chann
{
    struct t_lea_iso_chann     *p_next;
    uint8_t                     iso_mode;
    uint8_t                     frame_num;
    uint8_t                     path_direction;   //DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t                    iso_conn_handle;
    uint16_t                    pkt_seq_num;
    uint32_t                    presentation_delay;
    T_CODEC_CFG                 codec_data;
#if APP_LEA_INPUT_AUDIO_DATA_TEST
    uint16_t                    iso_sdu_len;
    uint32_t                    sdu_interval;
    const uint8_t              *p_encoded_data;
#endif
} T_APP_LEA_ISO_CHANN;

typedef struct
{
    uint8_t iso_mode;
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t iso_conn_handle;
    uint32_t presentation_delay;
    T_CODEC_CFG codec_parsed_data;
} T_LEA_SETUP_DATA_PATH;

typedef struct
{
    uint8_t iso_mode;
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t iso_conn_handle;
} T_LEA_REMOVE_DATA_PATH;

void app_lea_handle_data_path_setup(T_LEA_SETUP_DATA_PATH *p_data);
void app_lea_handle_data_path_remove(T_LEA_REMOVE_DATA_PATH *p_data);
T_APP_LEA_ISO_CHANN *app_lea_find_iso_chann(uint16_t iso_conn_handle, uint8_t direction);
void app_lea_audio_data_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
