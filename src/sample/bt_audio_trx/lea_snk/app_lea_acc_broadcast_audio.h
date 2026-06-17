/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BROADCAST_SYNC_H_
#define _APP_BROADCAST_SYNC_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */
#include "bt_direct_msg.h"
#include "app_lea_acc_scan.h"
#include "audio.h"
#include "audio_type.h"
#include "ble_audio.h"
#include "codec_def.h"
#include "app_types.h"
#include "app_lea_acc_scan.h"
#include "app_lea_acc_broadcast_audio.h"
#include "audio_track.h"

#define APP_LEA_BCA_GET                     0xFF
#define APP_LEA_BCA_CLR                     0x00
#define APP_BIS_BS_FIND_RECORD              0x01
#define APP_BIS_BS_FIND_RESET_RECORD        0xFE
#define APP_BIS_BS_FIND_ALIGN               0x04
#define APP_BIS_BS_FIND_RESET_ALIGN         0xFB
#define APP_BIS_BS_FIND_HIT                 0x10
#define APP_BIS_BS_FIND_RESET_HIT           0xEF
#define APP_BIS_BS_FIND_RECORD_ALIGN        (APP_BIS_BS_FIND_RECORD|APP_BIS_BS_FIND_ALIGN)
#define APP_BIS_BIS_CTRL_ACTIVE             0x01
#define APP_BIS_BIS_CTRL_SD_ACTIVE          0x02

#define APP_BIS_BIS_CTRL_RESET_ACTIVE       0xFE
#define APP_BIS_BIS_CTRL_RESET_SD_ACTIVE    0xFD

//#define APP_BIS_BIS_CTRL_INACTIVE         0x10
//#define APP_BIS_BIS_CTRL_RESET_INACTIVE   0xEF
#define APP_BIS_BIS_MAX_COUNT               3
#define APP_BIS_SEC_NUM                     2

#define AUDIO_CHANNEL_L  (AUDIO_CHANNEL_LOCATION_FL|AUDIO_CHANNEL_LOCATION_BL|AUDIO_CHANNEL_LOCATION_FLC|AUDIO_CHANNEL_LOCATION_SL \
                          |AUDIO_CHANNEL_LOCATION_TFL|AUDIO_CHANNEL_LOCATION_TBL|AUDIO_CHANNEL_LOCATION_TSL|AUDIO_CHANNEL_LOCATION_BFL \
                          |AUDIO_CHANNEL_LOCATION_FLW|AUDIO_CHANNEL_LOCATION_LS) \

#define AUDIO_CHANNEL_R  (AUDIO_CHANNEL_LOCATION_FR|AUDIO_CHANNEL_LOCATION_BR|AUDIO_CHANNEL_LOCATION_FRC|AUDIO_CHANNEL_LOCATION_SR \
                          |AUDIO_CHANNEL_LOCATION_TFR|AUDIO_CHANNEL_LOCATION_TBR|AUDIO_CHANNEL_LOCATION_TSR|AUDIO_CHANNEL_LOCATION_BFR \
                          |AUDIO_CHANNEL_LOCATION_FRW|AUDIO_CHANNEL_LOCATION_RS) \


typedef enum
{
    LEA_BCA_SPK_OUT_MUTE          = 0x01,
    LEA_BCA_SPK_OUT_UNMUTE        = 0x02,
    LEA_BCA_SPK_OUT_MAX,
} T_APP_LEA_BCA_SPK_OUT;

typedef enum
{
    LEA_BCA_STATE_IDLE              = 0x00,
    LEA_BCA_STATE_BASS_ADD_SOURCE   = 0x01,
    LEA_BCA_STATE_PRE_SCAN          = 0x02,
    LEA_BCA_STATE_SCAN              = 0x03,
    LEA_BCA_STATE_STREAMING         = 0x04,
    LEA_BCA_STATE_WAIT_TERM         = 0x05,
    LEA_BCA_STATE_WAIT_RETRY        = 0x06,
} T_APP_LEA_BCA_STATE;

typedef enum
{
    LEA_BIS_POLICY_RANDOM             = 0x00,
    LEA_BIS_POLICY_SPECIFIC           = 0x01,
    LEA_BIS_POLICY_RANDOM_AND_LAST    = 0x02,
    LEA_BIS_POLICY_SPECIFIC_AND_LAST  = 0x03,
} T_LEA_BIS_POLICY;

typedef enum
{
    LEA_BROADCAST_NONE                = 0x00,
    LEA_BROADCAST_DELEGATOR           = 0x01,
    LEA_BROADCAST_SINK                = 0x02,
} T_LEA_BROADCAST_ROLE;


typedef struct
{
    uint8_t lea_adv: 3;
    uint8_t lea_bca: 5;
} T_LEA_SM;


typedef enum
{
    LEA_MMI_BCA                  = 0x01,
    LEA_BIG_SYNC                 = 0x02,
    LEA_BIG_TERMINATE            = 0x03,
    LEA_SCAN_TIMEOUT             = 0x04,
    LEA_SCAN_STOP                = 0x05,
    LEA_SCAN_DELEGATOR           = 0x06,
} T_LEA_BCA_EVENT;

typedef enum
{
    LEA_REMOTE_MMI_SWITCH_SYNC        = 0x01,
    LEA_REMOTE_PA_INFO_SYNC           = 0x02,
    LEA_REMOTE_SEC_SYNC_CIS_STREAMING = 0x03,
    LEA_REMOTE_MEDIA_SUSPEND_SYNC     = 0x04,
    LEA_REMOTE_SYNC_SIRK              = 0x05,
    LEA_REMOTE_ISO_INFO_SYNC          = 0x06,
    LEA_REMOTE_MSG_TOTAL
} T_LEA_REMOTE_MSG;


void app_lea_mgr_tri_mmi_handle_action(uint8_t action, bool inter);

bool app_lea_mgr_is_streaming(void);

bool app_lea_bca_bs_update_device_info(T_LEA_BRS_INFO *src_info);
bool app_lea_bca_pa_sync(uint8_t dev_idx);
bool app_lea_bca_get_pa_info(uint8_t *sets, uint8_t *active_index, uint8_t *para);
bool app_lea_bca_bs_set_active(uint8_t dev_idx);
void app_lea_bca_bs_handle_pa_info_sync(T_LEA_BRS_INFO *src_info);
bool app_lea_bca_bs_pa_terminate(uint8_t dev_idx);
bool app_lea_bca_bs_big_establish_dev_idex(uint8_t dev_idx);
bool app_lea_bca_big_terminate(uint8_t dev_idx);
bool app_lea_bca_terminate(void);
bool app_lea_bca_sink_pa_terminate(uint8_t source_id);
bool app_lea_bca_reset_active(uint16_t *conn_handle);
bool app_lea_bca_sm(uint8_t event, void *p_data);
void app_lea_bca_tgt_addr(bool get, uint8_t *para);
void app_lea_bca_handle_iso_data(T_BT_DIRECT_CB_DATA *p_data);
void app_lea_bca_used(void);
void app_lea_bca_bmr_terminate(void);
void app_lea_bca_set_spk_out(T_APP_LEA_BCA_SPK_OUT set);
T_APP_LEA_BCA_SPK_OUT app_lea_bca_get_spk_out(void);
void *app_lea_bca_get_track_handle(void);
uint8_t app_lea_bca_tgt(uint8_t flag);
void app_lea_bca_state_change(T_APP_LEA_BCA_STATE state);
T_APP_LEA_BCA_STATE app_lea_bca_state(void);
void app_lea_bca_scan_info(T_LEA_BRS_INFO *src_info);
void app_lea_bca_init(void);
void app_lea_bca_sync_init(uint8_t mode, uint8_t policy, uint8_t *addr);
uint8_t app_lea_mgr_bis_spout_gain(uint8_t *para);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif

