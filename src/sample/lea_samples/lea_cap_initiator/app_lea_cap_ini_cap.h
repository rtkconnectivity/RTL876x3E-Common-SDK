/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_INI_CAP_H_
#define _APP_LEA_CAP_INI_CAP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "ble_audio_group.h"
#include "bap_unicast_client.h"
#include "codec_qos.h"
#include "app_lea_cap_ini_link.h"

#define LEA_MEDIA_AUDIO_CFG_SUPPORT (UNICAST_AUDIO_CFG_1_BIT |UNICAST_AUDIO_CFG_6_I_BIT | \
                                     UNICAST_AUDIO_CFG_6_II_BIT)
#define LEA_MEDIA_AUDIO_CFG_PREFER  (UNICAST_AUDIO_CFG_1_BIT |UNICAST_AUDIO_CFG_6_I_BIT | \
                                     UNICAST_AUDIO_CFG_6_II_BIT)
#define LEA_MEDIA_CODEC_CFG_SUPPORT (CODEC_CFG_ITEM_16_2_BIT | \
                                     CODEC_CFG_ITEM_48_1_BIT | CODEC_CFG_ITEM_48_2_BIT | \
                                     CODEC_CFG_ITEM_48_3_BIT | CODEC_CFG_ITEM_48_4_BIT | \
                                     CODEC_CFG_ITEM_48_5_BIT| CODEC_CFG_ITEM_48_6_BIT )

#define LEA_CONVERSATIONAL_AUDIO_CFG_SUPPORT (UNICAST_AUDIO_CFG_3_BIT |UNICAST_AUDIO_CFG_7_II_BIT| \
                                              UNICAST_AUDIO_CFG_8_I_BIT|UNICAST_AUDIO_CFG_8_II_BIT| \
                                              UNICAST_AUDIO_CFG_11_I_BIT | UNICAST_AUDIO_CFG_11_II_BIT)
#define LEA_CONVERSATIONAL_AUDIO_CFG_PREFER  (UNICAST_AUDIO_CFG_3_BIT | \
                                              UNICAST_AUDIO_CFG_11_I_BIT | UNICAST_AUDIO_CFG_11_II_BIT)
#define LEA_CONVERSATIONAL_CODEC_CFG_SUPPORT (CODEC_CFG_ITEM_16_1_BIT | CODEC_CFG_ITEM_16_2_BIT |  \
                                              CODEC_CFG_ITEM_32_1_BIT | CODEC_CFG_ITEM_32_2_BIT)


#if BAP_UNICAST_CLIENT
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE session_handle;
    uint16_t                      ready_conn_handle;
    T_AUDIO_STREAM_STATE          bap_state;
    T_UNICAST_AUDIO_CFG_TYPE      cfg_type;
    T_CODEC_CFG_ITEM              codec_cfg_item;
    T_ASE_TARGET_LATENCY          target_latency;
    bool                          release_req;
    uint16_t                      contexts_type;
    uint8_t                       ccid_num;
    uint8_t                       ccid[APP_LEA_CCID_MAX_NUM];
    uint8_t                       dev_num;
    T_BLE_AUDIO_DEV_HANDLE        dev_tbl[2];
    uint32_t                      sink_used_location;
    uint32_t                      source_used_location;
} T_APP_LEA_UNICAST;
#endif

typedef struct t_lea_group_info
{
    struct t_lea_group_info      *p_next;
    T_BLE_AUDIO_GROUP_HANDLE      group_handle;
    uint8_t                       set_mem_size;
#if BAP_UNICAST_CLIENT
    T_APP_LEA_UNICAST             lea_unicast;
#endif
} T_APP_LEA_GROUP_INFO;

void app_lea_ini_cap_init(void);
void app_lea_ini_group_cb(T_AUDIO_GROUP_MSG msg, T_BLE_AUDIO_GROUP_HANDLE handle,
                          void *buf);

T_APP_LEA_GROUP_INFO *app_lea_add_group(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                        uint8_t set_mem_size);
T_APP_LEA_GROUP_INFO *app_lea_find_group(T_BLE_AUDIO_GROUP_HANDLE group_handle);
T_APP_LEA_GROUP_INFO *app_lea_find_group_by_conn_handle(uint16_t conn_handle);

#if BAP_UNICAST_CLIENT
bool app_lea_ini_start_media(T_BLE_AUDIO_GROUP_HANDLE group_handle);
bool app_lea_ini_start_conversation(T_BLE_AUDIO_GROUP_HANDLE group_handle);
bool app_lea_ini_unicast_audio_stop(T_BLE_AUDIO_GROUP_HANDLE group_handle, uint32_t cis_timeout_ms);
bool app_lea_ini_unicast_audio_release(T_BLE_AUDIO_GROUP_HANDLE group_handle);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
