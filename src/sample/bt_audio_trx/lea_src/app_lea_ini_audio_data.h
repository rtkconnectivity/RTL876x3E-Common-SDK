/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_INI_AUDIO_DATA_H_
#define _APP_LEA_INI_AUDIO_DATA_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "rtl876x.h"
#include "audio_type.h"
#include "codec_def.h"
#include "ble_isoch_def.h"
#include "audio_type.h"

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
    uint16_t                    iso_sdu_len;
    uint32_t                    sdu_interval;
    uint32_t                    time_stamp;
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

#if LE_AUDIO_ISO_REF_CLK
#define REF_GUARD_TIME_US       (3000)
#define LEA_SYNC_CLK_REF        SYNCCLK_ID4
typedef enum
{
    UPLINK_SYNCREF_STAMP        = 0x5E,
} T_LEA_DSP_CMD;
#endif

typedef enum
{
    LEA_CODEC_DIR_ENCODE    = 0,
    LEA_CODEC_DIR_DECODE    = 1,
} T_LEA_CODEC_DIR;

void app_lea_print_lc3_format(const char *title, T_AUDIO_FORMAT_INFO *format_info);
bool app_lea_get_data_format(T_LEA_CODEC_DIR direction, T_AUDIO_FORMAT_INFO *format_info);
void app_lea_iso_data_send(uint8_t *p_data, uint16_t len, bool ext_flag, uint32_t ts, uint16_t seq);
bool le_audio_get_ap_delta(uint32_t *ap_delta, uint32_t *iso_interval);

void app_lea_handle_cis_data_path_setup(T_LEA_SETUP_DATA_PATH *p_data);
void app_lea_handle_cis_data_path_remove(T_LEA_REMOVE_DATA_PATH *p_data);

void app_lea_handle_bis_data_path_setup(T_LEA_SETUP_DATA_PATH *p_data);
void app_lea_handle_bis_data_path_remove(T_LEA_REMOVE_DATA_PATH *p_data);

void app_lea_audio_data_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
