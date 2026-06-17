/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_INI_BAP_BSRC_H_
#define _APP_LEA_CAP_INI_BAP_BSRC_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "broadcast_source_sm.h"
#include "codec_qos.h"

#if BAP_BROADCAST_SOURCE
#define PRIMARY_ADV_INTERVAL_MIN          48
#define PRIMARY_ADV_INTERVAL_MAX          48
#define PERIODIC_ADV_INTERVAL_MIN         160
#define PERIODIC_ADV_INTERVAL_MAX         160

typedef struct
{
    T_BROADCAST_SOURCE_HANDLE source_handle;
    T_BROADCAST_SOURCE_STATE state;
    T_BROADCAST_SOURCE_STATE prefer_state;
    uint8_t                  encryption;
    uint8_t                  group1_bis_num;
    uint8_t                  group1_idx;
    uint8_t                  group1_subgroup1_idx;
    uint8_t                  group1_bis1_idx;
    uint8_t                  group1_bis2_idx;
    uint8_t                  group1_bis3_idx;

    uint16_t                  max_sdu;
    uint32_t                  sdu_interval;
    T_CODEC_CFG               codec_cfg;
    T_QOS_CFG_PREFERRED       prefer_qos;

    uint8_t                   source_adv_sid;
    uint8_t                   broadcast_id[BROADCAST_ID_LEN];
    uint8_t                   source_address[GAP_BD_ADDR_LEN];

//Configure parameters
    uint8_t                   cfg_bis_num;
    T_CODEC_CFG_ITEM          cfg_codec_index;
    T_QOS_CFG_TYPE            cfg_qos_type;
    T_GAP_LOCAL_ADDR_TYPE     cfg_local_addr_type;
    bool                      cfg_encryption;
} T_BROADCAST_SOURCE_CB;

extern uint8_t brs_broadcast_code[16];//Just for test, app can change the value

void app_lea_bsrc_init(T_CODEC_CFG_ITEM index, T_QOS_CFG_TYPE qos_type, uint8_t bis_num,
                       T_GAP_LOCAL_ADDR_TYPE local_addr_type, bool encryption);
T_BROADCAST_SOURCE_HANDLE app_lea_bsrc_get_handle(uint8_t *bd_addr, uint8_t bd_type,
                                                  uint8_t adv_sid, uint8_t broadcast_id[3]);
bool app_lea_bsrc_start(void);
bool app_lea_bsrc_stop(bool release);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
