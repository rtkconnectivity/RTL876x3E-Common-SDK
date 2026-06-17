/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#if F_APP_TMAP_BMR_SUPPORT
#include <string.h>
#include "trace.h"
#include "ble_conn.h"
#include "bt_types.h"
#include "audio_probe.h"
#include "base_data_parse.h"
#include "bass_mgr.h"
#include "ble_audio.h"
#include "ble_audio_sync.h"
#include "bt_direct_msg.h"
#include "bt_gatt_client.h"
#include "gap_ext_scan.h"
#include "gap_iso_data.h"
#include "pacs_def.h"
#include "app_audio_policy.h"
#include "app_auto_power_off.h"
#include "app_cfg.h"
#include "app_dsp_cfg.h"
#include "app_lea_acc_adv.h"
#include "app_lea_acc_mgr.h"
#include "app_lea_acc_broadcast_audio.h"
#include "app_lea_acc_unicast_audio.h"
#include "app_lea_acc_def.h"
#include "app_lea_acc_profile.h"
#include "app_lea_acc_scan.h"
#include "app_link_util_cs.h"
#include "app_main.h"
#include "app_mmi.h"

#if F_APP_LC3_PLUS_SUPPORT
#include "app_lea_acc_pacs.h"
#endif

/*============================================================================*
 *                              Variables
 *============================================================================*/

#define APP_LEA_BCA_DB_NUM           4

//BCA State
#define APP_LEA_BCA_SD               0x01
#define APP_LEA_BCA_BASE_DATA        0x02
#define APP_LEA_BCA_CODEC_CFG        0x04
#define APP_LEA_BCA_BIG_INFO         0x08
#define APP_LEA_BCA_BS               0x10

//BCA State Reset(Inverse of BCA State)
#define APP_LEA_BCA_SD_RESET         0xFE
#define APP_LEA_BCA_BASE_DATA_RESET  0xFD
#define APP_LEA_BCA_CODEC_CFG_RESET  0xFB
#define APP_LEA_BCA_BIG_INFO_RESET   0xF7
#define APP_LEA_BCA_BS_RESET         0xEF

#define APP_LEA_BCA_BS_MSE           0
#define APP_LEA_BCA_BS_SYNC_TO       100

#define APP_LEA_BCA_PA_OPTION        0
#define APP_LEA_BCA_PA_TYPE          0
#define APP_LEA_BCA_PA_SKIP          0
#define APP_LEA_BCA_PA_SYNC_TO       1000


typedef enum
{
    BS_CH_L         = 0x00,
    BS_CH_R         = 0x01,
    BS_CH_LR        = 0x02,
    BS_CH_MAX,
} T_APP_LEA_BCA_CH;

typedef enum
{
    BS_TYPE_PA      = 0x00,
    BS_TYPE_BIG     = 0x01,
} T_APP_LEA_BCA_CHECK_TYPE;


typedef struct
{
    uint8_t target_srcaddr[6];
    uint8_t target_lea_srctype;
    uint8_t target_lea_srcsid;
    uint8_t target_lea_bsrid[3];
    uint8_t find;
    uint8_t channel;
    uint8_t ctrl;
    uint8_t lea_encryp;
    uint8_t lea_code[16];
} T_APP_LEA_BCA_BS_TGT;

typedef struct
{
    uint8_t used;
    uint8_t bis_id;
    uint8_t ch_id;
    uint8_t frame_num;
    uint8_t bit_depth;
    T_AUDIO_FORMAT_TYPE codec_type;
    uint16_t bis_conn_handle;
    void *audio_track_handle;
    T_GAP_BIG_SYNC_RECEIVER_SYNC_STATE big_state;
    T_CODEC_CFG     bis_codec_cfg;
    uint32_t  presentation_delay;
} T_LEA_BCA_SECTION;

typedef struct
{
    uint8_t id;
    uint8_t used;
    uint8_t source_id;
    uint8_t bis_idx;
    uint8_t bis_msk;
    uint8_t num_bis;
    uint8_t act_num_bis;
    uint8_t frame_num;
    uint8_t advertiser_address[GAP_BD_ADDR_LEN];
    uint8_t advertiser_address_type;
    uint8_t advertiser_sid;
    uint8_t src_id[3];
    uint8_t ch_l_idx;
    uint8_t ch_r_idx;
    uint8_t ch_lr_idx;
    bool is_past;
    bool disallow_pa;
    bool is_encryp;
    uint16_t conn_handle;
    uint16_t pa_interval;
    T_BLE_AUDIO_SYNC_HANDLE sync_handle;
    T_GAP_PA_SYNC_STATE sync_state;
    T_GAP_BIG_SYNC_RECEIVER_SYNC_STATE big_state;
    T_LEA_BCA_SECTION bis_sec[GAP_BIG_MGR_MAX_BIS_NUM];
    uint8_t broadcast_Code[16];
    uint32_t stream_allocation;
    uint16_t left_conn_handle;
    uint16_t right_conn_handle;
    uint8_t  left_ch_iso_state;
    uint8_t  right_ch_iso_state;
} T_APP_LEA_BCA_DB;

typedef struct
{
    T_APP_LEA_BCA_CHECK_TYPE type;
    T_BLE_AUDIO_SYNC_CB_DATA *p_sync_cb;
    T_APP_LEA_BCA_DB *p_bc_source;
    T_BLE_AUDIO_SYNC_HANDLE handle;
} T_APP_LEA_BCA_SYNC_CHECK;

static const uint32_t sample_rate_freq[13] =
{
    8000, 11000, 16000, 22000, 24000, 32000, 44100, 48000, 88000, 96000, 176000, 192000, 384000
};

static T_APP_LEA_BCA_DB  app_lea_bca_db[APP_LEA_BCA_DB_NUM] = {0};
static T_APP_LEA_BCA_BS_TGT app_lea_bca_bs_tgt = {0};
static uint8_t app_lea_bca_bs_idx = APP_LEA_BCA_DB_NUM;
static uint8_t app_lea_bca_sd_idx = APP_LEA_BCA_DB_NUM;
static uint32_t app_lea_bca_bs_ch_L_db;
static uint32_t app_lea_bca_bs_ch_R_db;
static uint32_t app_lea_bca_bs_ch_LR_db;
static T_APP_LEA_BCA_STATE app_lea_bca_state_machine = LEA_BCA_STATE_IDLE;
static T_APP_LEA_BCA_SPK_OUT app_lea_bca_spk_out = LEA_BCA_SPK_OUT_UNMUTE;
static T_OS_QUEUE app_lea_bca_left_ch_queue;
static T_OS_QUEUE app_lea_bca_right_ch_queue;

static uint8_t bis_mode = LEA_BROADCAST_SINK;
static uint8_t bis_policy = LEA_BIS_POLICY_RANDOM;
static uint8_t subgroup = 1;
static uint8_t bstsrc[6] = {0};
static uint8_t focus_bis_ch = 0;

bool app_lea_bca_sink_add_device(T_LEA_BRS_INFO *src_info);
uint8_t app_lea_bca_bs_get_active(void);
bool app_lea_bca_sd_get_active(uint8_t *dev_info);
void app_lea_bca_bs_reset(void);
void app_lea_bca_state_change(T_APP_LEA_BCA_STATE state);
T_APP_LEA_BCA_DB *app_lea_bca_find_device_by_bs_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle);
T_APP_LEA_BCA_DB *app_lea_bca_find_sd_db(void);
T_BLE_AUDIO_SYNC_HANDLE app_lea_bca_get_sd_sync_handle(uint8_t src_id);
static void app_lea_mgr_set_bis_spk_out(uint8_t action);

/*============================================================================*
 *                              Functions
 *============================================================================*/
void app_lea_bca_set_spk_out(T_APP_LEA_BCA_SPK_OUT state)
{
    APP_PRINT_INFO1("app_lea_bca_set_spk_out: %d", state);
    app_lea_bca_spk_out = state;
}

T_APP_LEA_BCA_SPK_OUT app_lea_bca_get_spk_out(void)
{
    return app_lea_bca_spk_out;
}

uint8_t app_lea_bca_tgt(uint8_t flag)
{
    APP_PRINT_ERROR2("app_lea_bca_tgt: used 0x%x, 0x%x", app_lea_bca_bs_tgt.find, flag);
    if (flag == APP_LEA_BCA_CLR)
    {
        app_lea_bca_bs_tgt.find = 0;
        goto RESULT;
    }
    if (flag != APP_LEA_BCA_GET)
    {
        app_lea_bca_bs_tgt.find |= flag;
    }
RESULT:
    return app_lea_bca_bs_tgt.find;
}

void app_lea_bca_tgt_addr(bool get, uint8_t *para)
{
    if (get)
    {
        memcpy(para, app_lea_bca_bs_tgt.target_srcaddr, GAP_BD_ADDR_LEN);
    }
    else
    {
        memcpy(app_lea_bca_bs_tgt.target_srcaddr, para, GAP_BD_ADDR_LEN);
    }
}

void app_lea_bca_tgt_active(bool set, uint8_t para)
{
    if (set)
    {
        app_lea_bca_bs_tgt.ctrl |= para;
    }
    else
    {
        app_lea_bca_bs_tgt.ctrl &= para;
    }
    APP_PRINT_ERROR3("app_lea_bca_tgt_active: ctrl 0x%x, set 0x%x, para 0x%x",
                     app_lea_bca_bs_tgt.ctrl,
                     app_lea_bca_bs_tgt.find, para);
}

bool app_lea_bca_default_bis(void)
{
    if (focus_bis_ch)
    {
        return true;
    }
    return false;
}

static void app_lea_bca_db_init(void)
{
    uint8_t i;
    app_lea_bca_bs_ch_L_db = AUDIO_CHANNEL_L;
    app_lea_bca_bs_ch_R_db = AUDIO_CHANNEL_R;
    app_lea_bca_bs_ch_LR_db = AUDIO_CHANNEL_L | AUDIO_CHANNEL_R;

    memset(&app_lea_bca_bs_tgt, 0x00, sizeof(T_APP_LEA_BCA_BS_TGT));

    app_lea_bca_bs_tgt.channel = GAP_BIG_MGR_MAX_BIS_NUM + 1;
    if (bis_policy == LEA_BIS_POLICY_SPECIFIC)
    {
        memcpy(app_lea_bca_bs_tgt.target_srcaddr, bstsrc, GAP_BD_ADDR_LEN);
        app_lea_bca_bs_tgt.find |= (APP_BIS_BS_FIND_RECORD | APP_BIS_BS_FIND_ALIGN);

    }
    else if (bis_policy == LEA_BIS_POLICY_SPECIFIC_AND_LAST)
    {
        if (app_cfg_nv.lea_valid &&
            (bis_mode & LEA_BROADCAST_DELEGATOR))
        {
            memcpy(app_lea_bca_bs_tgt.target_srcaddr, app_cfg_nv.lea_srcaddr, GAP_BD_ADDR_LEN);
            app_lea_bca_bs_tgt.target_lea_srctype = app_cfg_nv.lea_srctype;
            app_lea_bca_bs_tgt.target_lea_srcsid  = app_cfg_nv.lea_srcsid;
            app_lea_bca_bs_tgt.channel  = app_cfg_nv.lea_srcch;
            app_lea_bca_bs_tgt.lea_encryp = app_cfg_nv.lea_encryp;
            memcpy(app_lea_bca_bs_tgt.lea_code, app_cfg_nv.lea_code, BROADCAST_CODE_LEN);
            app_lea_bca_bs_tgt.find |= APP_BIS_BS_FIND_RECORD;
        }
    }


    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        app_lea_bca_db[i].id = i;
        app_lea_bca_db[i].sync_handle = NULL;
        app_lea_bca_db[i].is_past = 1;
        app_lea_bca_db[i].disallow_pa = false;
        app_lea_bca_db[i].act_num_bis = 0;
        app_lea_bca_db[i].ch_l_idx  = BS_CH_MAX;
        app_lea_bca_db[i].ch_r_idx  = BS_CH_MAX;
        app_lea_bca_db[i].ch_lr_idx = BS_CH_MAX;
    }
}

void app_lea_bca_sync_init(uint8_t mode, uint8_t policy, uint8_t *addr)
{
    bis_mode = mode;
    bis_policy = policy;
    if (addr)
    {
        memcpy(bstsrc, addr, 6);
    }
    app_lea_bca_db_init();
}

T_LEA_BCA_SECTION *app_lea_bca_find_sec(T_APP_LEA_BCA_DB *p_bc_source, uint8_t *bis_idx)
{
    uint8_t i;

    if (p_bc_source == NULL)
    {
        return NULL;
    }

    for (i = 0; i < GAP_BIG_MGR_MAX_BIS_NUM; i++)
    {
        if ((p_bc_source->bis_sec[i].used & APP_LEA_BCA_CODEC_CFG) &&
            (p_bc_source->bis_sec[i].bis_id == *bis_idx))
        {
            return &p_bc_source->bis_sec[i];
        }
    }

    return NULL;
}


bool app_lea_bca_ch_exist(T_APP_LEA_BCA_DB *p_bc_source, uint8_t *bis_idx)
{
    uint8_t num_bis = p_bc_source->num_bis;
    uint8_t offset = 0;
    bool result = false;

    while (num_bis > 0)
    {
        APP_PRINT_INFO4("app_lea_bca_ch_exist: %d,%d,%d,%d", num_bis, offset, (1 << offset),
                        *bis_idx);
        if (p_bc_source->bis_msk & (1 << offset))
        {
            APP_PRINT_INFO1("app_lea_bca_ch_exist: check offset %d", offset);
            if (*bis_idx == offset)
            {
                result = true;
                break;
            }
            num_bis--;

        }
        offset++;
        if (offset > 7)
        {
            num_bis = 0;
        }
    }
    return result;
}

uint8_t  app_lea_bca_ch_policy(T_APP_LEA_BCA_DB **p_bc_source)
{
    uint8_t result = 0;
    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        if (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_lr_idx != BS_CH_MAX)
        {
            ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                T_APP_LEA_BCA_DB *)(
                                                                *p_bc_source))->ch_lr_idx].bis_id;
        }
        else if ((((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_l_idx != BS_CH_MAX) &&
                 (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_r_idx != BS_CH_MAX))
        {
            ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                T_APP_LEA_BCA_DB *)(
                                                                *p_bc_source))->ch_l_idx].bis_id |
                                                            ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                        T_APP_LEA_BCA_DB *)(
                                                                        *p_bc_source))->ch_r_idx].bis_id;
        }
        else if (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_l_idx != BS_CH_MAX)
        {
            ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                T_APP_LEA_BCA_DB *)(
                                                                *p_bc_source))->ch_l_idx].bis_id;
        }
        else if (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_r_idx != BS_CH_MAX)
        {
            ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                T_APP_LEA_BCA_DB *)(
                                                                *p_bc_source))->ch_r_idx].bis_id;
        }
        else
        {
            result = 1;
        }
    }
    else
    {
        if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
        {
            if (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_l_idx != BS_CH_MAX)
            {
                ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                    T_APP_LEA_BCA_DB *)(
                                                                    *p_bc_source))->ch_l_idx].bis_id;
            }
            else if (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_r_idx != BS_CH_MAX)
            {
                ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                    T_APP_LEA_BCA_DB *)(
                                                                    *p_bc_source))->ch_r_idx].bis_id;
            }
            else if (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_lr_idx != BS_CH_MAX)
            {
                ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                    T_APP_LEA_BCA_DB *)(
                                                                    *p_bc_source))->ch_lr_idx].bis_id;
            }
            else
            {
                result = 2;
            }
        }
        else
        {
            if (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_r_idx != BS_CH_MAX)
            {
                ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                    T_APP_LEA_BCA_DB *)(
                                                                    *p_bc_source))->ch_r_idx].bis_id;
            }
            else if (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_l_idx != BS_CH_MAX)
            {
                ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                    T_APP_LEA_BCA_DB *)(
                                                                    *p_bc_source))->ch_l_idx].bis_id;
            }
            else if (((T_APP_LEA_BCA_DB *)(*p_bc_source))->ch_lr_idx != BS_CH_MAX)
            {
                ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_idx = ((T_APP_LEA_BCA_DB *)(*p_bc_source))->bis_sec[((
                                                                    T_APP_LEA_BCA_DB *)(
                                                                    *p_bc_source))->ch_lr_idx].bis_id;
            }
            else
            {
                result = 3;
            }
        }

    }
    return result;
}

void app_lea_bca_audio_path_reset(T_AUDIO_TRACK_HANDLE audio_track_handle, bool release_track_only)
{
    if (audio_track_handle != NULL)
    {
        app_lea_mgr_set_media_state((uint8_t)ISOCH_DATA_PKT_STATUS_LOST_DATA);
#if F_APP_LE_AUDIO_REF_CLK
        syncclk_drv_timer_stop(LEA_SYNC_CLK_REF);
#endif
        audio_track_release(audio_track_handle);
    }

    if (release_track_only)
    {
        return;
    }

    app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BLE_AUDIO, app_cfg_const.timer_auto_power_off);

}

void app_lea_bca_apply_sync_pera(T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM *sync_param,
                                 T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t encryption)
{
    T_APP_LEA_BCA_DB *p_bc_source = app_lea_bca_find_device_by_bs_handle(handle);
    sync_param->encryption = encryption;
    sync_param->mse = APP_LEA_BCA_BS_MSE;
    sync_param->big_sync_timeout = APP_LEA_BCA_BS_SYNC_TO;

    sync_param->num_bis = 1;
    if (app_lea_bca_default_bis())
    {
        uint8_t channel = subgroup;
        if (app_lea_bca_find_sec(p_bc_source, &channel) != NULL)
        {
            p_bc_source->bis_idx = channel;
        }
        else
        {
            uint8_t result;

            result = app_lea_bca_ch_policy(&p_bc_source);
            APP_PRINT_TRACE1("app_lea_bca_apply_sync_pera: set channel %d", result);
        }
        sync_param->bis[0] = p_bc_source->bis_idx;

    }
    else
    {
        if (app_lea_bca_bs_tgt.channel < (GAP_BIG_MGR_MAX_BIS_NUM + 1) &&
            (app_lea_bca_find_sec(p_bc_source, &app_lea_bca_bs_tgt.channel) != NULL))
        {
            APP_PRINT_INFO1("app_lea_bca_apply_sync_pera: %d", app_lea_bca_bs_tgt.channel);
            p_bc_source->bis_idx = app_lea_bca_bs_tgt.channel;
            sync_param->bis[0] = p_bc_source->bis_idx;
        }
        else
        {
            uint8_t result;
            result = app_lea_bca_ch_policy(&p_bc_source);
            if (__builtin_popcount(p_bc_source->bis_idx) > 1 && p_bc_source->ch_lr_idx == BS_CH_MAX)
            {
                sync_param->num_bis = 2;
                p_bc_source->stream_allocation |=
                    p_bc_source->bis_sec[p_bc_source->ch_l_idx].bis_codec_cfg.audio_channel_allocation;
                p_bc_source->stream_allocation |=
                    p_bc_source->bis_sec[p_bc_source->ch_r_idx].bis_codec_cfg.audio_channel_allocation;
                sync_param->bis[0] = p_bc_source->bis_sec[p_bc_source->ch_l_idx].bis_id;
                sync_param->bis[1] = p_bc_source->bis_sec[p_bc_source->ch_r_idx].bis_id;
            }
            else
            {
                sync_param->bis[0] = p_bc_source->bis_idx;
            }
            APP_PRINT_TRACE1("app_lea_bca_apply_sync_pera: According side set channel %d", result);
        }
    }
    APP_PRINT_TRACE1("app_lea_bca_apply_sync_pera: bis_idx %d", p_bc_source->bis_idx);

    if (encryption)
    {
        memcpy(sync_param->broadcast_code, p_bc_source->broadcast_Code, BROADCAST_CODE_LEN);
    }
}

bool app_lea_bca_big_establish(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t encryption)
{

    T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM sync_param;

    app_lea_bca_apply_sync_pera(&sync_param, handle, encryption);
    APP_PRINT_INFO5("app_lea_bca_big_establish: %d,%d,%d,%d,%d",
                    sync_param.encryption,
                    sync_param.mse,
                    sync_param.num_bis,
                    sync_param.bis[0],
                    sync_param.bis[1]);


    if (!ble_audio_big_sync_establish(handle, &sync_param))
    {
        APP_PRINT_ERROR1("app_lea_bca_big_establish: sync failed, num_bis: 0x%x", sync_param.num_bis);
        return false;
    }
    return true;
}


bool app_lea_bca_sink_pa_terminate(uint8_t source_id)
{
    if (ble_audio_pa_terminate(app_lea_bca_get_sd_sync_handle(source_id)) == false)
    {
        goto error;
    }
    return true;
error:
    return false;
}

void *app_lea_bca_get_track_handle(void)//feature multi trace process
{
    uint8_t dev_idx =  app_lea_bca_bs_get_active();
    T_APP_LEA_BCA_DB *p_bc_source = NULL;
    void *db = NULL;
    if (dev_idx == APP_LEA_BCA_DB_NUM)
    {
        return db;
    }
    p_bc_source = &app_lea_bca_db[dev_idx];
    //APP_PRINT_ERROR5("app_lea_bca_get_track_handle: used: 0x%x, %d,%d,%d, bis_idx=%d",
    //                 p_bc_source->used,
    //                 p_bc_source->ch_lr_idx, p_bc_source->ch_l_idx, p_bc_source->ch_r_idx,
    //                p_bc_source->bis_idx);

    if (p_bc_source->used & APP_LEA_BCA_BS)
    {
        uint8_t i;
        for (i = 0; i < GAP_BIG_MGR_MAX_BIS_NUM; i++)
        {
            if (p_bc_source->bis_sec[i].audio_track_handle != NULL)
            {
                return (void *) p_bc_source->bis_sec[i].audio_track_handle;
            }
        }
    }
    return NULL;
}

uint8_t app_lea_bca_get_sd_source(void)
{
    if ((app_lea_bca_sd_idx < APP_LEA_BCA_DB_NUM) &&
        (app_lea_bca_db[app_lea_bca_sd_idx].used & APP_LEA_BCA_SD))
    {
        return app_lea_bca_db[app_lea_bca_sd_idx].source_id;
    }
    else
    {
        return 0xff;
    }
}

T_BLE_AUDIO_SYNC_HANDLE app_lea_bca_get_sd_sync_handle(uint8_t src_id)
{
    uint8_t i;
    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        if ((app_lea_bca_db[i].used & APP_LEA_BCA_SD) &&
            (app_lea_bca_db[i].source_id == src_id))
        {
            APP_PRINT_ERROR2("app_lea_bca_get_sd_sync_handle: src_id %d, g_sd_idx %d", src_id,
                             app_lea_bca_sd_idx);
            return app_lea_bca_db[i].sync_handle;
        }
    }
    return NULL;
}

T_APP_LEA_BCA_DB *app_lea_bca_find_device_by_bs_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    uint8_t i;
    if (sync_handle == NULL)
    {
        return NULL;
    }
    /* Check if device is already in device list*/
    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        if ((app_lea_bca_db[i].used & APP_LEA_BCA_BS) &&
            (app_lea_bca_db[i].sync_handle == sync_handle))
        {
            return &app_lea_bca_db[i];
        }
    }
    return NULL;
}

T_APP_LEA_BCA_DB *app_lea_bca_sd_find_bs(T_LEA_BRS_INFO *src_info)
{
    T_APP_LEA_BCA_DB *p_src = app_lea_bca_find_sd_db();

    if (p_src)
    {
        if ((p_src->advertiser_address_type == src_info->adv_addr_type) &&
            (p_src->advertiser_sid == src_info->advertiser_sid) &&
            (!memcmp(src_info->adv_addr, p_src->advertiser_address, GAP_BD_ADDR_LEN)) &&
            (!memcmp(src_info->broadcast_id, p_src->src_id, 3)))
        {
            p_src->used  |= APP_LEA_BCA_BS;
            app_lea_bca_bs_set_active(p_src->id);
            if (app_lea_bca_pa_sync(p_src->id) == false)
            {
                APP_PRINT_ERROR3("app_lea_bca_sd_find_bs: fail %d, %d, %d", p_src->id, app_lea_bca_sd_idx,
                                 app_lea_bca_bs_idx);
            }
            else
            {
                APP_PRINT_INFO8("app_lea_bca_sd_find_bs: Add sink RemoteBd [%02x:%02x:%02x:%02x:%02x:%02x] , sid %d type %d \r\n",

                                p_src->advertiser_address[5], p_src->advertiser_address[4],
                                p_src->advertiser_address[3], p_src->advertiser_address[2],
                                p_src->advertiser_address[1], p_src->advertiser_address[0],
                                p_src->advertiser_sid,
                                p_src->advertiser_address_type);
            }
            return p_src;
        }
        else
        {
            return NULL;
        }
    }

    return p_src;
}

T_APP_LEA_BCA_DB *app_lea_bca_bs_find_device_by_index(uint8_t index)
{
    uint8_t i;
    if (index < APP_LEA_BCA_DB_NUM)
    {
        if (app_lea_bca_db[index].used & APP_LEA_BCA_BS)
        {
            return &app_lea_bca_db[index];
        }
    }
    return NULL;
}

bool app_lea_bca_bs_find_index_by_bdaddr(uint8_t *bd_addr, uint8_t *index)
{
    uint8_t i;
    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        if ((app_lea_bca_db[i].used & APP_LEA_BCA_BS) &&
            (!memcmp(app_lea_bca_db[i].advertiser_address, bd_addr, 6)))
        {
            *index = i;
            return true;
        }
    }
    return false;
}

bool app_lea_bca_get_pa_info(uint8_t *sets, uint8_t *active_index, uint8_t *para)
{
    T_APP_LEA_BCA_DB *pa_info = NULL;
    bool ret = false;
    uint8_t offset = 0;
    *active_index = app_lea_bca_bs_get_active();
    *sets = 0;
    for (uint8_t i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        pa_info = app_lea_bca_bs_find_device_by_index(i);
        if (pa_info)
        {
            if (pa_info->used & APP_LEA_BCA_BS)
            {
                (*sets)++;
                memcpy(&para[offset], pa_info->advertiser_address, 6);
                para[offset + 6] = pa_info->advertiser_address_type;
                para[offset + 7] = pa_info->advertiser_sid;
                memcpy(&para[offset + 8], pa_info->src_id, 3);
                offset += 11;
                ret = true;
            }
        }
    }
    return ret;
}

bool app_lea_bca_bs_update_device_info(T_LEA_BRS_INFO *src_info)
{
    memcpy(app_lea_bca_bs_tgt.target_srcaddr, src_info->adv_addr, GAP_BD_ADDR_LEN);
    app_lea_bca_bs_tgt.target_lea_srctype = src_info->adv_addr_type;
    app_lea_bca_bs_tgt.target_lea_srcsid  = src_info->advertiser_sid;
    memcpy(app_lea_bca_bs_tgt.target_lea_bsrid, src_info->broadcast_id, 3);
    app_lea_bca_bs_tgt.find = APP_BIS_BS_FIND_RECORD_ALIGN;
    return true;
}

void app_lea_bca_bs_handle_pa_info_sync(T_LEA_BRS_INFO *src_info)
{
    uint8_t dev_idx = APP_LEA_BCA_DB_NUM;
    APP_PRINT_TRACE4("app_lea_bca_bs_handle_pa_info_sync: bdaddr %s,addr_type %d, adv_sid %d, %d",
                     TRACE_BDADDR(src_info->adv_addr), src_info->adv_addr_type, src_info->advertiser_sid,
                     app_lea_bca_state());

    if (app_lea_bca_bs_find_index_by_bdaddr(src_info->adv_addr, &dev_idx) &&//must align
        ((app_lea_bca_state() == LEA_BCA_STATE_PRE_SCAN) ||
         (app_lea_bca_state() == LEA_BCA_STATE_SCAN)))
    {
        if (app_lea_bca_pa_sync(dev_idx))
        {
            app_lea_bca_bs_set_active(dev_idx);
        }
    }
    else
    {
        app_lea_bca_sink_add_device(src_info);
    }
}

bool app_lea_bca_db_add_sink(T_LEA_BRS_INFO *src_info)
{
    uint8_t i;
    /* Check if device is already in device list*/
    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        APP_PRINT_INFO3("app_lea_bca_db_add_sink: idx %d, %d, %d\r\n",
                        i, app_lea_bca_db[i].used, app_lea_bca_db[i].advertiser_sid);
        if (!(app_lea_bca_db[i].used & APP_LEA_BCA_BS) &&
            !(app_lea_bca_db[i].used & APP_LEA_BCA_SD))
        {
            /*Add addr to device list list*/
            app_lea_bca_db[i].used  |=  APP_LEA_BCA_BS;
            memcpy(app_lea_bca_db[i].advertiser_address, src_info->adv_addr, GAP_BD_ADDR_LEN);
            app_lea_bca_db[i].advertiser_address_type = src_info->adv_addr_type;
            app_lea_bca_db[i].advertiser_sid = src_info->advertiser_sid;
            memcpy(app_lea_bca_db[i].src_id, src_info->broadcast_id, 3);

            if (app_lea_bca_pa_sync(i))
            {
                app_lea_bca_bs_set_active(i);
                memcpy(app_cfg_nv.lea_srcaddr, app_lea_bca_db[i].advertiser_address, GAP_BD_ADDR_LEN);
                app_cfg_nv.lea_srctype = app_lea_bca_db[i].advertiser_address_type;
                app_cfg_nv.lea_srcsid  = app_lea_bca_db[i].advertiser_sid;
                app_cfg_nv.lea_valid = 1;
                app_cfg_nv.lea_encryp = 0;
                app_cfg_store(app_cfg_nv.lea_srcaddr, 12);
            }
            else
            {
                APP_PRINT_ERROR1("app_lea_bca_db_add_sink: pa_sync failed dev_idx 0x%x", i);
            }
            return true;
        }
    }
    return false;
}

bool app_lea_bca_sink_add_device(T_LEA_BRS_INFO *src_info)
{
    uint8_t                 adv_address[GAP_BD_ADDR_LEN];

    if (app_lea_bca_sd_find_bs(src_info) != NULL)
    {
        return true;
    }


    APP_PRINT_INFO8("app_bis_mgr_sync_add_device_record = [%02x:%02x:%02x:%02x:%02x:%02x] , type %d , %d\r\n",
                    app_lea_bca_bs_tgt.target_srcaddr[5], app_lea_bca_bs_tgt.target_srcaddr[4],
                    app_lea_bca_bs_tgt.target_srcaddr[3], app_lea_bca_bs_tgt.target_srcaddr[2],
                    app_lea_bca_bs_tgt.target_srcaddr[1], app_lea_bca_bs_tgt.target_srcaddr[0],
                    app_lea_bca_bs_tgt.target_lea_srctype,
                    app_lea_bca_bs_tgt.target_lea_srcsid);
    APP_PRINT_INFO8("app_lea_bca_sink_add_device = [%02x:%02x:%02x:%02x:%02x:%02x] , type %d , %d\r\n",
                    src_info->adv_addr[5], src_info->adv_addr[4],
                    src_info->adv_addr[3], src_info->adv_addr[2],
                    src_info->adv_addr[1], src_info->adv_addr[0],
                    src_info->adv_addr_type,
                    src_info->advertiser_sid);
    if (!(app_lea_bca_tgt(APP_LEA_BCA_GET)&APP_BIS_BS_FIND_HIT))
    {
        if (app_lea_bca_tgt(APP_LEA_BCA_GET)&APP_BIS_BS_FIND_RECORD_ALIGN &&
            (!memcmp(src_info->adv_addr, app_lea_bca_bs_tgt.target_srcaddr, GAP_BD_ADDR_LEN)))
        {
            app_lea_bca_bs_tgt.find |= APP_BIS_BS_FIND_HIT;
            return app_lea_bca_db_add_sink(src_info);
        }
        else
        {
            return false;
        }
    }
    else
    {
        return true;
    }
}

bool app_lea_bca_bs_clear_device(T_BLE_AUDIO_SYNC_HANDLE handle)
{
    uint8_t i;
    if (handle == NULL)
    {
        APP_PRINT_ERROR0("app_lea_bca_bs_clear_device: handle is NULL");
        return false;
    }

    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        APP_PRINT_ERROR2("app_lea_bca_bs_clear_device: i %d, used %d", i, app_lea_bca_db[i].used);
        if ((app_lea_bca_db[i].used & APP_LEA_BCA_BS) &&
            (app_lea_bca_db[i].sync_handle == handle))
        {
            app_lea_bca_db[i].used &= APP_LEA_BCA_BS_RESET;
            app_lea_bca_db[i].sync_handle = NULL;
            app_lea_bca_db[i].disallow_pa = false;
            app_lea_bca_db[i].act_num_bis = 0;
            app_lea_bca_db[i].is_encryp = 0;
            app_lea_bca_db[i].ch_l_idx  = BS_CH_MAX;
            app_lea_bca_db[i].ch_r_idx  = BS_CH_MAX;
            app_lea_bca_db[i].ch_lr_idx = BS_CH_MAX;
            app_lea_bca_db[i].stream_allocation = 0;
            memset(app_lea_bca_db[i].broadcast_Code, 0x00, BROADCAST_CODE_LEN);
            memset(app_lea_bca_db[i].bis_sec, 0x00, sizeof(T_LEA_BCA_SECTION) * GAP_BIG_MGR_MAX_BIS_NUM);
            return true;
        }
    }

    return false;

}
bool app_lea_bca_set_cap(T_LEA_BCA_SECTION  *p_bc_source,
                         uint8_t codec_id[CODEC_ID_LEN],
                         T_CODEC_CFG *bis_codec_cfg,
                         uint32_t *presentation_delay)
{
    bool ret = false;
    if (p_bc_source == NULL || bis_codec_cfg == NULL || codec_id == NULL)
    {
        APP_PRINT_INFO0("app_lea_bca_set_cap: NULL");
        return ret;
    }

    if (p_bc_source->used & APP_LEA_BCA_CODEC_CFG)
    {
        APP_PRINT_INFO1("app_lea_bca_set_cap: is used 0x%x  error", p_bc_source->used);
        return ret;
    }

    p_bc_source->used |= APP_LEA_BCA_CODEC_CFG;
    p_bc_source->codec_type = AUDIO_FORMAT_TYPE_LC3;
    p_bc_source->bis_codec_cfg.frame_duration = bis_codec_cfg->frame_duration;
    p_bc_source->bis_codec_cfg.sample_frequency = bis_codec_cfg->sample_frequency;
    p_bc_source->bis_codec_cfg.codec_frame_blocks_per_sdu = bis_codec_cfg->codec_frame_blocks_per_sdu;
    p_bc_source->bis_codec_cfg.octets_per_codec_frame = bis_codec_cfg->octets_per_codec_frame;
    p_bc_source->bis_codec_cfg.audio_channel_allocation = bis_codec_cfg->audio_channel_allocation;
    p_bc_source->presentation_delay = *presentation_delay;
#if F_APP_LC3_PLUS_SUPPORT
    p_bc_source->codec_type = app_lea_pacs_check_codec_type(codec_id);
#endif
    APP_PRINT_INFO7("app_lea_bca_set_cap: bis_idx [%d], %d, frame_duration: 0x%x, sample_frequency: 0x%x, codec_frame_blocks_per_sdu: 0x%x, octets_per_codec_frame: 0x%x, audio_channel_allocation: 0x%x",
                    p_bc_source->bis_id,
                    p_bc_source->ch_id,
                    p_bc_source->bis_codec_cfg.frame_duration, p_bc_source->bis_codec_cfg.sample_frequency,
                    p_bc_source->bis_codec_cfg.codec_frame_blocks_per_sdu,
                    p_bc_source->bis_codec_cfg.octets_per_codec_frame,
                    p_bc_source->bis_codec_cfg.audio_channel_allocation);
    ret = true;
    return ret;
}

T_APP_LEA_BCA_CH app_lea_bca_audio_cap(T_CODEC_CFG *bis_codec_cfg, uint8_t  bis_id)
{
    T_APP_LEA_BCA_CH ret = BS_CH_MAX;
    uint32_t check_L = 0;
    uint32_t check_R = 0;
    APP_PRINT_INFO8("app_lea_bca_audio_cap:  L_db 0x%x, R_db 0x%x, LR_db 0x%x, frame_duration 0x%x, sample_frequency 0x%x, codec_frame_blocks_per_sdu 0x%x, octets_per_codec_frame 0x%x, audio_channel_allocation 0x%x",
                    app_lea_bca_bs_ch_L_db,
                    app_lea_bca_bs_ch_R_db,
                    app_lea_bca_bs_ch_LR_db,
                    bis_codec_cfg->frame_duration,
                    bis_codec_cfg->sample_frequency,
                    bis_codec_cfg->codec_frame_blocks_per_sdu,
                    bis_codec_cfg->octets_per_codec_frame,
                    bis_codec_cfg->audio_channel_allocation);

    check_L = app_lea_bca_bs_ch_L_db &  bis_codec_cfg->audio_channel_allocation;
    check_R = app_lea_bca_bs_ch_R_db &  bis_codec_cfg->audio_channel_allocation;
    if (check_L && check_R)
    {
        ret = BS_CH_LR;
    }
    else if (check_L)
    {
        ret = BS_CH_L;
    }
    else if (check_R)
    {
        ret = BS_CH_R;
    }
    return ret;


}

void app_lea_bca_set_ch_idx(T_APP_LEA_BCA_CH *ch_check, T_APP_LEA_BCA_DB **p_bc_source,
                            uint8_t *idx)
{
    if (*ch_check == BS_CH_L)
    {
        ((T_APP_LEA_BCA_DB *)*p_bc_source)->ch_l_idx = *idx;
    }
    else if (*ch_check == BS_CH_R)
    {
        ((T_APP_LEA_BCA_DB *)*p_bc_source)->ch_r_idx = *idx;
    }
    else if (*ch_check == BS_CH_LR)
    {
        ((T_APP_LEA_BCA_DB *)*p_bc_source)->ch_lr_idx = *idx;
    }

}

void app_lea_bca_remap_bis(T_BASE_DATA_MAPPING *p_base_mapping,
                           T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    uint8_t i, j;
    uint8_t sec_idx = 0;
    T_APP_LEA_BCA_DB *p_bc_source = app_lea_bca_find_device_by_bs_handle(sync_handle);

    APP_PRINT_INFO5("app_lea_bca_remap_bis: %d, %d, 0x%x, %d, %d",
                    p_base_mapping->num_bis,
                    p_base_mapping->num_subgroups,
                    p_base_mapping->p_subgroup->p_bis_param->bis_index,
                    subgroup,
                    bis_mode);

    p_bc_source->num_bis = p_base_mapping->num_bis;
    for (i = 0; i < p_base_mapping->num_subgroups; i++)
    {
        for (j = 0; j < p_base_mapping->p_subgroup[i].num_bis; j ++)
        {
            T_APP_LEA_BCA_CH temp_ch = BS_CH_MAX;

            temp_ch = app_lea_bca_audio_cap(&p_base_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg,
                                            p_base_mapping->p_subgroup[i].p_bis_param[j].bis_index);
            if (temp_ch != BS_CH_MAX)
            {
                uint8_t ch_mask = 1 << p_base_mapping->p_subgroup[i].p_bis_param[j].bis_index ;
                p_bc_source->bis_msk |= ch_mask;

                APP_PRINT_INFO6("app_lea_bca_remap_bis: tgt.channel 0x%x , bis_msk 0x%x , ch_mask 0x%x ,  app_cfg_nv.bud_role %d, app_cfg_const.bud_side %d, bis_index %d",
                                app_lea_bca_bs_tgt.channel,
                                p_bc_source->bis_msk,
                                ch_mask,
                                app_cfg_nv.bud_role,
                                app_cfg_const.bud_side,
                                p_base_mapping->p_subgroup[i].p_bis_param[j].bis_index);

                if (app_lea_bca_set_cap(&p_bc_source->bis_sec[sec_idx],
                                        &p_base_mapping->p_subgroup[i].p_bis_param->codec_id[0],
                                        &p_base_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg,
                                        &p_base_mapping->presentation_delay))
                {
                    p_bc_source->bis_sec[sec_idx].bis_id = p_base_mapping->p_subgroup[i].p_bis_param[j].bis_index;
                    p_bc_source->bis_sec[sec_idx].ch_id = temp_ch;
#if F_APP_LC3_PLUS_SUPPORT
                    p_bc_source->bis_sec[sec_idx].bit_depth = app_lea_pacs_check_bit_depth(
                                                                  p_bc_source->bis_sec[sec_idx].codec_type,
                                                                  p_base_mapping->p_subgroup[i].metadata_len,
                                                                  p_base_mapping->p_subgroup[i].p_metadata);
#endif
                    app_lea_bca_set_ch_idx(&temp_ch, &p_bc_source, &sec_idx);
                    sec_idx++;
                    APP_PRINT_INFO1("app_lea_bca_remap_bis: sec_idx=  %d",
                                    sec_idx);
                }
            }
            else
            {
                APP_PRINT_INFO5("app_lea_bca_remap_bis: bis_codec_cfg 0x%x, 0x%x, 0x%x, 0x%x,0x%x",
                                p_base_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.frame_duration,
                                p_base_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.sample_frequency,
                                p_base_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.codec_frame_blocks_per_sdu,
                                p_base_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.octets_per_codec_frame,
                                p_base_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.audio_channel_allocation);
            }
        }
    }
}

static bool app_lea_bca_check_track_exist(T_APP_LEA_BCA_DB *p_bc_source, T_LEA_BCA_SECTION *src_sec)
{
    if (p_bc_source)
    {
        for (uint8_t i = 0; i < GAP_BIG_MGR_MAX_BIS_NUM; i++)
        {
            if ((p_bc_source->bis_sec[i].used & APP_LEA_BCA_CODEC_CFG) &&
                (&p_bc_source->bis_sec[i] != src_sec) &&
                p_bc_source->bis_sec[i].audio_track_handle)
            {
                src_sec->audio_track_handle = p_bc_source->bis_sec[i].audio_track_handle;
                src_sec->frame_num = p_bc_source->bis_sec[i].frame_num;
                return true;
            }
        }
    }

    return false;

}

void app_lea_bca_track(T_APP_LEA_BCA_DB *p_bc_source, T_LEA_BCA_SECTION *src_sec)
{
    uint8_t ret = 0;
    T_AUDIO_STREAM_TYPE type = AUDIO_STREAM_TYPE_PLAYBACK;
    uint8_t volume_out = app_cfg_nv.bis_audio_gain_level;
    uint8_t volume_in = 0;
    uint8_t channel_count = 0;
    uint32_t device = app_db.playback_device;
    T_AUDIO_FORMAT_INFO format_info;
    T_CODEC_CFG *codec = &src_sec->bis_codec_cfg;

    if (app_lea_bca_check_track_exist(p_bc_source, src_sec))
    {
        ret = 1;
        goto fail_track_create;
    }

    format_info.type = src_sec->codec_type;
    format_info.frame_num = FIXED_FRAME_NUM;
#if F_APP_LC3_PLUS_SUPPORT
    if (format_info.type == AUDIO_FORMAT_TYPE_LC3PLUS)
    {
        format_info.attr.lc3plus.sample_rate = sample_rate_freq[codec->sample_frequency - 1];
        if (__builtin_popcount(p_bc_source->stream_allocation) > 1)
        {
            format_info.attr.lc3plus.chann_location = p_bc_source->stream_allocation;
        }
        else
        {
            format_info.attr.lc3plus.chann_location = codec->audio_channel_allocation;
        }
        format_info.attr.lc3plus.frame_length = codec->octets_per_codec_frame;
        format_info.attr.lc3plus.mode = AUDIO_LC3PLUS_MODE_NOMRAL;
        format_info.attr.lc3plus.bit_width = src_sec->bit_depth;
        format_info.attr.lc3plus.presentation_delay = src_sec->presentation_delay;

        if (codec->frame_duration == FRAME_DURATION_CFG_10_MS)
        {
            format_info.attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_10_MS;
        }
        else if (codec->frame_duration == FRAME_DURATION_CFG_5_MS)
        {
            format_info.attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_5_MS;
        }
        else
        {
            format_info.attr.lc3plus.frame_duration = AUDIO_LC3PLUS_FRAME_DURATION_2_5_MS;
        }

        channel_count = (format_info.attr.lc3plus.chann_location == AUDIO_CHANNEL_LOCATION_MONO) ?
                        1 : __builtin_popcount(format_info.attr.lc3plus.chann_location);
    }
    else
#endif
    {
        format_info.attr.lc3.sample_rate = sample_rate_freq[codec->sample_frequency - 1];
        if (__builtin_popcount(p_bc_source->stream_allocation) > 1)
        {
            format_info.attr.lc3.chann_location = p_bc_source->stream_allocation;
        }
        else
        {
            format_info.attr.lc3.chann_location = codec->audio_channel_allocation;
        }
        format_info.attr.lc3.frame_length = codec->octets_per_codec_frame;
        format_info.attr.lc3.frame_duration = (T_AUDIO_LC3_FRAME_DURATION)codec->frame_duration;
        format_info.attr.lc3.presentation_delay = src_sec->presentation_delay;

        channel_count = (format_info.attr.lc3.chann_location == AUDIO_CHANNEL_LOCATION_MONO) ?
                        1 : __builtin_popcount(format_info.attr.lc3.chann_location);
    }

    APP_PRINT_TRACE8("app_lea_bca_track: chann_location 0x%02X, codec_type 0x%02X, sample_frequency 0x%02X, \
frame_duration 0x%02X, frame_length 0x%02X, codec_frame_blocks_per_sdu 0x%02X, presentation_delay 0x%02X, volume_out 0x%02X",
                     codec->audio_channel_allocation,
                     src_sec->codec_type,
                     codec->sample_frequency,
                     codec->frame_duration,
                     codec->octets_per_codec_frame,
                     codec->codec_frame_blocks_per_sdu,
                     src_sec->presentation_delay,
                     (app_cfg_nv.lea_vol_out_mute << 8 | volume_out));

    app_lea_mgr_set_media_state((uint8_t)ISOCH_DATA_PKT_STATUS_LOST_DATA);
    src_sec->frame_num = codec->codec_frame_blocks_per_sdu * channel_count;
    src_sec->audio_track_handle = audio_track_create(type,
                                                     AUDIO_STREAM_MODE_DIRECT,
                                                     AUDIO_STREAM_USAGE_LOCAL,
                                                     format_info,
                                                     volume_out,
                                                     volume_in,
                                                     device,
                                                     NULL,
                                                     NULL);
    if (src_sec->audio_track_handle == NULL)
    {
        ret = 2;
        goto fail_track_create;
    }
#if F_APP_LE_AUDIO_REF_CLK
    else
    {
        syncclk_drv_timer_start(LEA_SYNC_CLK_REF, CONN_HANDLE_TYPE_FREERUN_CLOCK, 0xFF, 0);
    }
#endif

#if F_APP_MALLEUS_SUPPORT
    lea_malleus_instance = malleus_create(src_sec->audio_track_handle);
#endif
    audio_track_start(src_sec->audio_track_handle);

fail_track_create:
    if (ret != 0)
    {
        APP_PRINT_ERROR1("app_lea_bca_track: failed %d", -ret);
    }
}

static void app_lea_bca_remove_data_path(T_APP_LEA_BCA_DB *p_bc_source, void *p_data)
{
    T_BLE_AUDIO_BIG_REMOVE_DATA_PATH *p_rmv_path = (T_BLE_AUDIO_BIG_REMOVE_DATA_PATH *)p_data;

    if (p_bc_source)
    {
        if (p_bc_source->left_conn_handle == p_rmv_path->bis_conn_handle)
        {
            app_lea_mgr_clear_iso_queue(&app_lea_bca_left_ch_queue);
            p_bc_source->left_conn_handle = 0;
            p_bc_source->left_ch_iso_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
        }
        else if (p_bc_source->right_conn_handle == p_rmv_path->bis_conn_handle)
        {
            app_lea_mgr_clear_iso_queue(&app_lea_bca_right_ch_queue);
            p_bc_source->right_conn_handle = 0;
            p_bc_source->right_ch_iso_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
        }
    }
    APP_PRINT_INFO2("app_lea_bca_remove_data_path: bis_id 0x%x, bis_handle 0x%x",  p_rmv_path->bis_idx,
                    p_rmv_path->bis_conn_handle);
}

bool app_lea_bca_release_all(T_APP_LEA_BCA_SYNC_CHECK para)
{
    if ((para.type == BS_TYPE_PA &&
         (para.p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED) &&
         (para.p_bc_source->big_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED)) ||
        ((para.type == BS_TYPE_BIG) &&
         (para.p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED) &&
         (para.p_bc_source->sync_state == GAP_PA_SYNC_STATE_TERMINATED)))
    {
        app_lea_bca_bs_clear_device(para.handle);
        app_lea_bca_bs_reset();
        ble_audio_sync_release(&para.handle);

        app_lea_bca_state_change(LEA_BCA_STATE_IDLE);
        if (app_lea_bca_state() == LEA_BCA_STATE_WAIT_RETRY)
        {
            app_lea_acc_scan_stop();
            app_lea_mgr_tri_mmi_handle_action(MMI_BIG_START, true);
        }

        return true;
    }
    else
    {
        if (para.type == BS_TYPE_PA)
        {
            if (para.p_bc_source->big_state != BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED &&
                para.p_bc_source->big_state != BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATING)
            {
                ble_audio_big_terminate(para.p_bc_source->sync_handle);
            }
        }
        else if (para.type == BS_TYPE_BIG)
        {
            if (para.p_bc_source->sync_state != GAP_PA_SYNC_STATE_TERMINATED &&
                para.p_bc_source->sync_state != GAP_PA_SYNC_STATE_TERMINATING)
            {
                ble_audio_pa_terminate(para.p_bc_source->sync_handle);
            }
        }

        return false;
    }
}

void app_lea_bca_sync_cb(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t cb_type, void *p_cb_data)
{

    T_BLE_AUDIO_SYNC_CB_DATA *p_sync_cb = (T_BLE_AUDIO_SYNC_CB_DATA *)p_cb_data;
    T_APP_LEA_BCA_DB *p_bc_source = app_lea_bca_find_device_by_bs_handle(handle);
    if (p_bc_source == NULL)
    {
        return;
    }
    APP_PRINT_INFO1("app_lea_bca_sync_cb: cb_type %d,\r\n",
                    cb_type);

    switch (cb_type)
    {

    case MSG_BLE_AUDIO_SYNC_HANDLE_RELEASED:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_SYNC_HANDLE_RELEASED: action_role %d",
                             p_sync_cb->p_sync_handle_released->action_role);
        }
        break;

    case MSG_BLE_AUDIO_ADDR_UPDATE:
        {
            APP_PRINT_TRACE1("MSG_BLE_AUDIO_ADDR_UPDATE: advertiser_address %s",
                             TRACE_BDADDR(p_sync_cb->p_addr_update->advertiser_address));
        }
        break;

    case MSG_BLE_AUDIO_PA_SYNC_STATE:
        {
            APP_PRINT_INFO3("MSG_BLE_AUDIO_PA_SYNC_STATE: sync_state %d, action %d, cause 0x%x\r\n",
                            p_sync_cb->p_pa_sync_state->sync_state,
                            p_sync_cb->p_pa_sync_state->action,
                            p_sync_cb->p_pa_sync_state->cause);
            T_APP_LEA_BCA_SYNC_CHECK para;

            para.type = BS_TYPE_PA;
            para.p_sync_cb = p_sync_cb;
            para.p_bc_source = p_bc_source;
            para.handle = handle;

            //PA sync lost
            if (((p_sync_cb->p_pa_sync_state->action == BLE_AUDIO_PA_LOST) ||
                 (p_sync_cb->p_pa_sync_state->action == BLE_AUDIO_PA_SYNC)) &&
                (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED) &&
                (p_sync_cb->p_pa_sync_state->cause == 0x13e))
            {
                if (app_lea_bca_release_all(para))
                {
                    app_lea_bca_tgt_active(false, (APP_BIS_BIS_CTRL_RESET_ACTIVE & APP_BIS_BIS_CTRL_RESET_SD_ACTIVE));
                    app_lea_mgr_tri_mmi_handle_action(MMI_BIG_START, true);
                }
            }
            else if ((p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_WAIT_SCANNING)
                     &&
                     !p_bc_source->is_past)
            {
                p_bc_source->is_past = 1;
                app_lea_acc_scan_start();
            }
            else if (((p_sync_cb->p_pa_sync_state->action == BLE_AUDIO_PA_TERMINATE) &&
                      (p_sync_cb->p_pa_sync_state->cause == 0x144) &&
                      (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)))
            {
                if (app_lea_bca_release_all(para))
                {
                    app_lea_bca_tgt_active(false, (APP_BIS_BIS_CTRL_RESET_ACTIVE & APP_BIS_BIS_CTRL_RESET_SD_ACTIVE));
                }
            }
            else if (((p_sync_cb->p_pa_sync_state->action == BLE_AUDIO_PA_TERMINATE) &&
                      (p_sync_cb->p_pa_sync_state->cause == 0x116) &&
                      (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)))
            {
                if (app_lea_bca_state() == LEA_BCA_STATE_WAIT_TERM ||
                    app_lea_bca_state() == LEA_BCA_STATE_WAIT_RETRY)
                {
                    if (app_lea_bca_release_all(para))
                    {
                        app_lea_bca_tgt_active(false, (APP_BIS_BIS_CTRL_RESET_ACTIVE & APP_BIS_BIS_CTRL_RESET_SD_ACTIVE));
                    }
                }
            }

            p_bc_source->sync_state = p_sync_cb->p_pa_sync_state->sync_state;
        }
        break;

    case MSG_BLE_AUDIO_BASE_DATA_MODIFY_INFO:
        {
            APP_PRINT_TRACE3("MSG_BLE_AUDIO_BASE_DATA_MODIFY_INFO: p_base_mapping %p, used %d, sd_source %d\r\n",
                             p_sync_cb->p_base_data_modify_info->p_base_mapping, p_bc_source->used, app_lea_bca_get_sd_source());

            if (p_sync_cb->p_base_data_modify_info->p_base_mapping)
            {
                T_BASE_DATA_MAPPING *p_mapping = p_sync_cb->p_base_data_modify_info->p_base_mapping;

                app_lea_bca_remap_bis(p_mapping, handle);
                p_bc_source->used |= APP_LEA_BCA_BASE_DATA;

                if (p_bc_source->used & APP_LEA_BCA_BIG_INFO &&
                    ((app_lea_bca_bs_tgt.ctrl & APP_BIS_BIS_CTRL_SD_ACTIVE) == 0))
                {
                    if (app_lea_bca_get_sd_source() != 0xff && p_bc_source->is_encryp)
                    {
                        bass_send_broadcast_code_required(p_bc_source->source_id);
                    }
                    else
                    {
                        app_lea_bca_big_establish(handle, p_bc_source->is_encryp);
                    }
                }
            }
        }
        break;


    case MSG_BLE_AUDIO_PA_REPORT_INFO:
        {
            if (p_bc_source->big_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
            {
                ble_audio_pa_terminate(p_bc_source->sync_handle);
            }
        }
        break;

    case MSG_BLE_AUDIO_PA_BIGINFO:
        {
            T_LE_BIGINFO_ADV_REPORT_INFO *p_adv_report_info =  p_sync_cb->p_le_biginfo_adv_report_info;

            APP_PRINT_INFO7("MSG_BLE_AUDIO_PA_BIGINFO: num_bis %d, %d, %d, %d, 0x%x ,%d, %d",
                            p_adv_report_info->num_bis,
                            p_adv_report_info->encryption,
                            app_lea_bca_state_machine,
                            p_bc_source->used,
                            app_lea_bca_get_sd_source(),
                            p_bc_source->is_encryp,
                            p_adv_report_info->encryption);

            if (app_lea_bca_state_machine == LEA_BCA_STATE_WAIT_TERM)
            {
                break;
            }

            p_bc_source->used |= APP_LEA_BCA_BIG_INFO;
            p_bc_source->is_encryp = p_adv_report_info->encryption;

            if ((p_bc_source->used & APP_LEA_BCA_BASE_DATA) == 0)
            {
                break;
            }

            if (app_lea_bca_bs_tgt.ctrl & APP_BIS_BIS_CTRL_SD_ACTIVE)
            {
                T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM sync_param;

                app_lea_bca_apply_sync_pera(&sync_param, handle, p_adv_report_info->encryption);
            }
            else
            {
                if (app_lea_bca_get_sd_source() != 0xff &&
                    !p_bc_source->is_encryp && p_adv_report_info->encryption)
                {
                    bass_send_broadcast_code_required(p_bc_source->source_id);
                }
                else
                {
                    app_lea_bca_big_establish(handle, p_adv_report_info->encryption);
                }
            }
        }
        break;

    case MSG_BLE_AUDIO_BIG_SYNC_STATE:
        {
            T_APP_LEA_BCA_SYNC_CHECK para;
            para.type = BS_TYPE_BIG;
            para.p_sync_cb = p_sync_cb;
            para.p_bc_source = p_bc_source;
            para.handle = handle;

            APP_PRINT_INFO4("MSG_BLE_AUDIO_BIG_SYNC_STATE: sync_state %d, action %d,action role %d, cause 0x%x\r\n",
                            p_sync_cb->p_big_sync_state->sync_state,
                            p_sync_cb->p_big_sync_state->action,
                            p_sync_cb->p_big_sync_state->action_role,
                            p_sync_cb->p_big_sync_state->cause);


            if ((p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED) &&
                (p_sync_cb->p_big_sync_state->action == BLE_AUDIO_BIG_TERMINATE ||
                 p_sync_cb->p_big_sync_state->action == BLE_AUDIO_BIG_IDLE) &&
                (p_sync_cb->p_big_sync_state->cause == 0))
            {
                uint8_t dev_info = 0;

                APP_PRINT_INFO1("app_lea_bca_sync_cb: BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED, sync_handle: 0x%x",
                                p_bc_source->sync_handle);

                if (app_lea_bca_release_all(para))
                {
                    app_lea_bca_tgt_active(false, (APP_BIS_BIS_CTRL_RESET_ACTIVE & APP_BIS_BIS_CTRL_RESET_SD_ACTIVE));
                }
            }
            else if ((p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED) &&
                     ((p_sync_cb->p_big_sync_state->action == BLE_AUDIO_BIG_SYNC) ||
                      (p_sync_cb->p_big_sync_state->action == BLE_AUDIO_BIG_IDLE)) &&
                     ((p_sync_cb->p_big_sync_state->cause == 0x113) || (p_sync_cb->p_big_sync_state->cause == 0x108) ||
                      (p_sync_cb->p_big_sync_state->cause == 0x13e)))
            {
                uint8_t dev_info = 0;

                if (!app_lea_bca_release_all(para))
                {
                    app_lea_bca_state_change(LEA_BCA_STATE_WAIT_RETRY);
                }
                else
                {
                    app_lea_bca_tgt_active(false, (APP_BIS_BIS_CTRL_RESET_ACTIVE & APP_BIS_BIS_CTRL_RESET_SD_ACTIVE));
                }

                if (p_bc_source->big_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
                {
                    app_audio_tone_type_play(TONE_BIS_LOSST, false, false);
                }

                app_lea_mgr_tri_mmi_handle_action(MMI_BIG_START, true);


            }
            else if (p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
            {
                if (p_sync_cb->p_big_sync_state->action == BLE_AUDIO_BIG_SYNC)
                {
                    T_BLE_AUDIO_BIS_INFO bis_sync_info;

                    bool result = ble_audio_get_bis_sync_info(p_bc_source->sync_handle,
                                                              &bis_sync_info);
                    if (result == false)
                    {
                        // goto failed;
                    }
                    APP_PRINT_INFO5("BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED %d,%d,%d, source_id %d, stream_allocation 0x%x",
                                    result,
                                    bis_sync_info.bis_num, bis_sync_info.bis_info[0].bis_idx, p_bc_source->source_id,
                                    p_bc_source->stream_allocation);

                    uint8_t codec_id[5] = {1, 0, 0, 0, 0};
                    uint32_t controller_delay = 0x1122;
                    codec_id[0] = LC3_CODEC_ID;

                    if (__builtin_popcount(p_bc_source->stream_allocation) > 1)
                    {
                        ble_audio_bis_setup_data_path(handle, p_bc_source->bis_sec[p_bc_source->ch_l_idx].bis_id, codec_id,
                                                      controller_delay, 0, NULL);
                        ble_audio_bis_setup_data_path(handle, p_bc_source->bis_sec[p_bc_source->ch_r_idx].bis_id, codec_id,
                                                      controller_delay, 0, NULL);
                    }
                    else
                    {
                        ble_audio_bis_setup_data_path(handle, p_bc_source->bis_idx, codec_id, controller_delay, 0, NULL);
                    }
                    APP_PRINT_INFO1("app_lea_bca_sync_cb: MSG_BLE_AUDIO_BIG_SYNC_STATE: bis_idx: %d",
                                    p_bc_source->bis_idx);

                    app_lea_acc_scan_stop();
                }
            }

            if (p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED ||
                p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATING)
            {
                p_bc_source->used &= (APP_LEA_BCA_BASE_DATA_RESET & APP_LEA_BCA_BIG_INFO_RESET);
            }

            p_bc_source->big_state = p_sync_cb->p_big_sync_state->sync_state;
        }
        break;

    case MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH:
        {
            T_LEA_BCA_SECTION *src_sec = app_lea_bca_find_sec(p_bc_source,
                                                              &p_sync_cb->p_setup_data_path->bis_idx);
            T_APP_LE_LINK *p_link = NULL;

#if F_APP_AUTO_POWER_TEST_LOG
            TEST_PRINT_INFO0("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH");
#else
            APP_PRINT_INFO3("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH: bis_idx %d, cause 0x%x, %d\r\n",
                            p_sync_cb->p_setup_data_path->bis_idx,
                            p_sync_cb->p_setup_data_path->cause,
                            p_bc_source->bis_idx);
#endif
            if (src_sec == NULL)
            {
                break;
            }

            if (src_sec->bis_codec_cfg.audio_channel_allocation & AUDIO_CHANNEL_R)
            {
                p_bc_source->right_conn_handle = p_sync_cb->p_setup_data_path->bis_conn_handle;
            }
            else if (src_sec->bis_codec_cfg.audio_channel_allocation & AUDIO_CHANNEL_L)
            {
                p_bc_source->left_conn_handle = p_sync_cb->p_setup_data_path->bis_conn_handle;
            }

            p_bc_source->act_num_bis ++;
            src_sec->bis_conn_handle = p_sync_cb->p_setup_data_path->bis_conn_handle;
            app_lea_bca_track(p_bc_source, src_sec);

            app_lea_bca_sm(LEA_STREAMING, NULL);
        }
        break;

    case MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH:
        {
            T_LEA_BCA_SECTION *src_sec = app_lea_bca_find_sec(p_bc_source,
                                                              &p_sync_cb->p_setup_data_path->bis_idx);
            T_APP_LE_LINK *p_link = NULL;
#if F_APP_AUTO_POWER_TEST_LOG
            TEST_PRINT_INFO0("MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH");
#else
            APP_PRINT_INFO4("MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH: bis_idx %d, cause 0x%x, %d,p_bc_source->act_num_bis=%d\r\n",
                            p_sync_cb->p_remove_data_path->bis_idx,
                            p_sync_cb->p_remove_data_path->cause,
                            p_bc_source->bis_idx,
                            p_bc_source->act_num_bis);
#endif
            if (src_sec == NULL)
            {
                break;
            }

            src_sec->used &= APP_LEA_BCA_CODEC_CFG_RESET;
            app_lea_bca_remove_data_path(p_bc_source, (void *)p_sync_cb->p_remove_data_path);

            p_bc_source->act_num_bis --;
            if (p_bc_source->act_num_bis)
            {
                app_lea_bca_audio_path_reset(src_sec->audio_track_handle, true);
            }
            else
            {
                app_lea_bca_audio_path_reset(src_sec->audio_track_handle, false);
            }
        }
        break;

    default:
        break;
    }
    return;
}

T_APP_LEA_BCA_DB *app_lea_bca_find_sd_db(void)
{
    APP_PRINT_INFO2("app_lea_bca_find_sd_db, used: 0x%x, 0x%x", app_lea_bca_sd_idx,
                    app_lea_bca_db[app_lea_bca_sd_idx].used);
    if (app_lea_bca_sd_idx < APP_LEA_BCA_DB_NUM &&
        (app_lea_bca_db[app_lea_bca_sd_idx].used & APP_LEA_BCA_SD))
    {
        return &app_lea_bca_db[app_lea_bca_sd_idx];
    }
    else
    {
        return NULL;
    }
}


T_APP_LEA_BCA_DB *app_lea_bca_find_db_by_handle(uint16_t *conn_handle)
{
    uint8_t i;
    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        APP_PRINT_INFO2("app_lea_bca_find_db_by_handle, used 0x%x, 0x%x", app_lea_bca_db[i].used,
                        app_lea_bca_db[i].conn_handle);
        if ((app_lea_bca_db[i].used & APP_LEA_BCA_SD) && app_lea_bca_db[i].conn_handle == *conn_handle)
        {
            return &app_lea_bca_db[i];
        }
    }
    return NULL;
}

T_APP_LEA_BCA_DB *app_lea_bca_find_db_by_sync_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    uint8_t i;
    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        APP_PRINT_INFO2("app_lea_bca_find_db_by_sync_handle, used 0x%x, 0x%x", app_lea_bca_db[i].used,
                        app_lea_bca_db[i].conn_handle);
        if ((app_lea_bca_db[i].used & APP_LEA_BCA_SD) && app_lea_bca_db[i].sync_handle == sync_handle)
        {
            return &app_lea_bca_db[i];
        }
    }
    return NULL;
}

bool app_lea_bca_sd_set_active(uint16_t *conn_handle)
{
    uint8_t i;
    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        //APP_PRINT_INFO4("app_lea_bca_sd_set_active, app_lea_bca_sd_idx %d, used 0x%x, 0x%x, 0x%x",
        //                app_lea_bca_sd_idx, app_lea_bca_db[i].used,
        //                app_lea_bca_db[i].conn_handle, *conn_handle);
        if ((!(app_lea_bca_db[i].used & APP_LEA_BCA_SD) && !(app_lea_bca_db[i].used & APP_LEA_BCA_BS)) ||
            (app_lea_bca_db[i].conn_handle == *conn_handle))
        {
            app_lea_bca_db[i].used |= APP_LEA_BCA_SD;
            app_lea_bca_db[i].conn_handle = *conn_handle;
            app_lea_bca_sd_idx = i;
            return true;
        }
    }
    APP_PRINT_INFO0("app_lea_bca_sd_set_active: fail");
    return false;
}

bool app_lea_bca_bs_set_active(uint8_t dev_idx)
{
    APP_PRINT_INFO1("app_lea_bca_bs_set_active, dev_idx: 0x%x", dev_idx);
    if (dev_idx < APP_LEA_BCA_DB_NUM)
    {
        app_lea_bca_bs_idx = dev_idx;
        return true;
    }
    return false;
}

uint8_t app_lea_bca_bs_get_active(void)
{
    APP_PRINT_INFO1("app_lea_bca_bs_get_active, _idx: 0x%x", app_lea_bca_bs_idx);
    return app_lea_bca_bs_idx;
}

bool app_lea_bca_sd_get_active(uint8_t *dev_info)
{
    if (dev_info != NULL)
    {
        *dev_info = app_lea_bca_sd_idx;
    }
    APP_PRINT_INFO1("app_lea_bca_sd_get_active: idx 0x%x", app_lea_bca_sd_idx);

    if (app_lea_bca_sd_idx < APP_LEA_BCA_DB_NUM)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool app_lea_bca_reset_active(uint16_t *conn_handle)
{
    uint8_t i;
    for (i = 0; i < APP_LEA_BCA_DB_NUM; i++)
    {
        APP_PRINT_INFO4("app_lea_bca_reset_active: app_lea_bca_sd_idx %d, used 0x%x, 0x%x, 0x%x",
                        app_lea_bca_sd_idx, app_lea_bca_db[i].used,
                        app_lea_bca_db[i].conn_handle, *conn_handle);
        if ((app_lea_bca_db[i].used & APP_LEA_BCA_SD) &&
            (app_lea_bca_db[i].conn_handle == *conn_handle))
        {
            app_lea_bca_db[i].used &= APP_LEA_BCA_SD_RESET;
            if (app_lea_bca_sd_idx == i)
            {
                app_lea_bca_sd_idx = APP_LEA_BCA_DB_NUM;
                APP_PRINT_INFO0("app_lea_bca_reset_active :find g_sd again");///modify
            }
            return true;
        }
    }
    APP_PRINT_INFO0("app_lea_bca_reset_active: fail");
    return false;
}


T_LEA_BCA_SECTION *app_lea_bca_find_con_handle(uint16_t conn_handle)
{
    uint8_t i;

    for (i = 0; i < GAP_BIG_MGR_MAX_BIS_NUM; i++)
    {
        if ((app_lea_bca_db[app_lea_bca_bs_idx].bis_sec[i].used & APP_LEA_BCA_CODEC_CFG) &&
            (app_lea_bca_db[app_lea_bca_bs_idx].bis_sec[i].bis_conn_handle == conn_handle))
        {
            return &app_lea_bca_db[app_lea_bca_bs_idx].bis_sec[i];
        }
    }

    return NULL;
}

void app_lea_bca_used(void)
{
    APP_PRINT_INFO2("app_lea_bca_used, idx=%d, used: 0x%x", app_lea_bca_bs_idx,
                    app_lea_bca_db[app_lea_bca_bs_idx].used);
}

void app_lea_bca_bs_reset(void)
{
    APP_PRINT_INFO2("app_lea_bca_bs_reset, dev_idx: 0x%x, 0x%x", app_lea_bca_bs_idx,
                    app_lea_bca_db[app_lea_bca_bs_idx].used);
    app_lea_bca_bs_idx = APP_LEA_BCA_DB_NUM;
    app_lea_bca_bs_tgt.find &= APP_BIS_BS_FIND_RESET_HIT;

}

bool app_lea_bca_pa_sync(uint8_t dev_idx)
{
    APP_PRINT_INFO3("app_lea_bca_pa_sync: dev_idx 0x%x, 0x%x, %d",
                    dev_idx, app_lea_bca_db[dev_idx].used, app_lea_bca_db[dev_idx].disallow_pa);
    uint8_t options = APP_LEA_BCA_PA_OPTION;
    uint8_t sync_cte_type = APP_LEA_BCA_PA_TYPE;
    uint16_t skip = APP_LEA_BCA_PA_SKIP;
    uint16_t sync_timeout = APP_LEA_BCA_PA_SYNC_TO;
    T_APP_LEA_BCA_DB *p_bc_source = NULL;
    uint8_t error_code = 0;

    if (dev_idx < APP_LEA_BCA_DB_NUM &&
        (app_lea_bca_db[dev_idx].used & APP_LEA_BCA_BS) &&
        (app_lea_bca_db[dev_idx].sync_handle == NULL) &&
        !app_lea_bca_db[dev_idx].disallow_pa)
    {
        p_bc_source = &app_lea_bca_db[dev_idx];
        if (p_bc_source->sync_handle == NULL)
        {
            p_bc_source->sync_handle = ble_audio_sync_allocate(app_lea_bca_sync_cb,
                                                               p_bc_source->advertiser_address_type,
                                                               p_bc_source->advertiser_address, p_bc_source->advertiser_sid,
                                                               p_bc_source->src_id);
        }
        if (p_bc_source->sync_handle == NULL)
        {
            error_code = 1;
            goto error;
        }
        else
        {
            app_lea_bca_db[dev_idx].disallow_pa = true;
        }
    }

    if (p_bc_source == NULL)
    {
        error_code = 2;
        goto error;
    }
    if (ble_audio_pa_sync_establish(p_bc_source->sync_handle,
                                    options, sync_cte_type, skip, sync_timeout) == false)
    {
        error_code = 3;
        goto error;
    }
    return true;
error:
    APP_PRINT_INFO2("app_lea_bca_pa_sync: dev_idx 0x%x, error_code=%d", dev_idx, error_code);
    return false;
}

bool app_lea_bca_bs_pa_terminate(uint8_t dev_idx)
{
    T_APP_LEA_BCA_DB *p_bc_source = NULL;
    if (dev_idx < APP_LEA_BCA_DB_NUM && (app_lea_bca_db[dev_idx].used & APP_LEA_BCA_BS))
    {
        p_bc_source = &app_lea_bca_db[dev_idx];
        if (p_bc_source->sync_handle == NULL)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return ble_audio_pa_terminate(p_bc_source->sync_handle);
}

bool app_lea_bca_bs_big_establish_dev_idex(uint8_t dev_idx)
{
    T_APP_LEA_BCA_DB *p_bc_source = NULL;
    T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM sync_param;

    if (dev_idx < APP_LEA_BCA_DB_NUM && (app_lea_bca_db[dev_idx].used & APP_LEA_BCA_BS))
    {
        p_bc_source = &app_lea_bca_db[dev_idx];
        if (p_bc_source->sync_handle == NULL)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    app_lea_bca_apply_sync_pera(&sync_param, p_bc_source, 0);
    APP_PRINT_INFO5("app_lea_bca_bs_big_establish_dev_idex: %d,%d,%d,%d,%d",
                    sync_param.encryption,
                    sync_param.mse,
                    sync_param.num_bis,
                    sync_param.bis[0],
                    sync_param.bis[1]);

    if (!ble_audio_big_sync_establish(p_bc_source->sync_handle, &sync_param))
    {
        APP_PRINT_ERROR1("app_lea_bca_bs_big_establish_dev_idex: sync failed, num_bis 0x%x",
                         sync_param.num_bis);
        return false;
    }
    return true;
}

bool app_lea_bca_terminate(void)
{
    T_APP_LEA_BCA_DB *p_bc_source = NULL;
    bool result = false;

    if (app_lea_bca_bs_idx < APP_LEA_BCA_DB_NUM &&
        (app_lea_bca_db[app_lea_bca_bs_idx].used & APP_LEA_BCA_BS))
    {
        p_bc_source = &app_lea_bca_db[app_lea_bca_bs_idx];
    }
    if ((p_bc_source == NULL) || (p_bc_source->sync_handle == NULL))
    {
        APP_PRINT_ERROR0("app_lea_bca_terminate: invaild idx");
        result = true;
    }
    else
    {
        APP_PRINT_ERROR2("app_lea_bca_terminate:  %d, %d",
                         p_bc_source->sync_state, p_bc_source->big_state);
        ble_audio_pa_terminate(p_bc_source->sync_handle);
        ble_audio_big_terminate(p_bc_source->sync_handle);
        if ((p_bc_source->sync_state == GAP_PA_SYNC_STATE_TERMINATED) &&
            (p_bc_source->big_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED))
        {
            app_lea_bca_bs_clear_device(p_bc_source->sync_handle);
            app_lea_bca_bs_reset();

            if (p_bc_source->sync_handle == 0)
            {
                result = true;
                goto END_PROCESS;
            }

            result = ble_audio_sync_release(&p_bc_source->sync_handle);
            APP_PRINT_ERROR1("app_lea_bca_terminate: sync release result %d, ", result);
        }
    }

END_PROCESS:
    return result;
}

bool app_lea_bca_big_terminate(uint8_t dev_idx)
{
    T_APP_LEA_BCA_DB *p_bc_source = NULL;
    APP_PRINT_ERROR2("app_lea_bca_big_terminate: dev_idx=%d, used: %d", dev_idx,
                     app_lea_bca_db[dev_idx].used);
    if (dev_idx < APP_LEA_BCA_DB_NUM && (app_lea_bca_db[dev_idx].used & APP_LEA_BCA_BS))
    {
        p_bc_source = &app_lea_bca_db[dev_idx];
        if (p_bc_source->sync_handle == NULL)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    APP_PRINT_ERROR0("app_lea_bca_big_terminate: OK");
    return ble_audio_big_terminate(p_bc_source->sync_handle);
}

static void app_lea_bca_le_disconnect_cb(uint8_t conn_id, uint8_t local_disc_cause,
                                         uint16_t disc_cause)
{
    APP_PRINT_TRACE2("app_lea_bca_le_disconnect_cb: disc_cause 0x%x, cause 0x%x", local_disc_cause,
                     disc_cause);
    T_APP_LE_LINK *p_link;
    uint8_t lea_link_state = LEA_LINK_IDLE;

    p_link = app_link_find_le_link_by_conn_id(conn_id);
    if (p_link)
    {
        lea_link_state = p_link->lea_link_state;
        app_lea_bca_state_change(LEA_BCA_STATE_IDLE);
    }

    if ((disc_cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE) &&
         (lea_link_state != LEA_LINK_IDLE)) ||
        (disc_cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT))  ||
        (disc_cause == (HCI_ERR | HCI_ERR_FAIL_TO_ESTABLISH_CONN))  ||
        (disc_cause == (HCI_ERR | HCI_ERR_INSTANT_PASSED))  ||
        (disc_cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT))  ||
        ((disc_cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)) &&
         (local_disc_cause == LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED)))
    {
        app_lea_mgr_adv_sm(LEA_CIG_START, &disc_cause);
    }
}

static uint16_t app_lea_bca_handle_cccd_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    uint8_t i = 0;

    APP_PRINT_INFO1("app_lea_bca_handle_cccd_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO:
        {
            T_SERVER_ATTR_CCCD_INFO *p_cccd = (T_SERVER_ATTR_CCCD_INFO *)buf;

            if (p_cccd->char_uuid >= PACS_UUID_CHAR_SINK_PAC &&
                p_cccd->char_uuid <= PACS_UUID_CHAR_SUPPORTED_AUDIO_CONTEXTS)
            {
                uint8_t conn_id;
                T_APP_LE_LINK *p_link;

                le_get_conn_id_by_handle(p_cccd->conn_handle, &conn_id);
                p_link = app_link_find_le_link_by_conn_id(conn_id);
                if (p_link && ((p_link->lea_device & APP_LEA_BASS_CCCD_ENABLED) == 0))
                {
                    p_link->lea_device |= APP_LEA_BASS_CCCD_ENABLED;
                    if (p_link->lea_link_state == LEA_LINK_IDLE)
                    {
                        app_lea_uca_link_sm(p_link->conn_handle, LEA_CONNECT, NULL);
                    }
                    app_lea_mgr_adv_sm(LEA_ADV_STOP, NULL);
                    app_link_reg_le_link_disc_cb(conn_id, app_lea_bca_le_disconnect_cb);
                }
            }
            APP_PRINT_INFO5("LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO: conn_handle 0x%x, service_id %d, char_uuid 0x%x, ccc_bits 0x%x, param %d",
                            p_cccd->conn_handle,
                            p_cccd->service_id,
                            p_cccd->char_uuid,
                            p_cccd->ccc_bits,
                            p_cccd->param);
        }
        break;

    default:
        break;
    }
    return cb_result;
}

static uint16_t app_lea_bca_handle_bass_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_APP_LEA_BCA_DB *p_src_active = app_lea_bca_find_sd_db();
    uint16_t msg_group = (msg & 0xFF00);
    uint8_t i = 0;
    uint8_t j;

    APP_PRINT_INFO1("app_lea_bca_handle_bass_msg: msg 0x%x", msg);

    switch (msg)
    {
    case LE_AUDIO_MSG_BASS_CP_IND:
        {
            T_BASS_CP_IND *p_data = (T_BASS_CP_IND *)buf;

            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_CP_IND: conn_handle 0x%x, cp opcode 0x%x",
                            p_data->conn_handle, p_data->p_cp_data->cp_op);

            switch (p_data->p_cp_data->cp_op)
            {
            case BASS_CP_OP_ADD_SOURCE:
                {

                    if (app_lea_bca_state() != LEA_BCA_STATE_IDLE)
                    {
                        app_lea_bca_sm(LEA_BIG_TERMINATE, NULL);
                    }

                    if (!app_lea_bca_sd_set_active(&p_data->conn_handle))
                    {
                        break;
                    }
                    p_src_active = app_lea_bca_find_db_by_handle(&p_data->conn_handle);

                    memcpy(p_src_active->advertiser_address, p_data->p_cp_data->param.add_source.advertiser_address,
                           GAP_BD_ADDR_LEN);
                    p_src_active->advertiser_address_type = p_data->p_cp_data->param.add_source.advertiser_address_type;
                    p_src_active->advertiser_sid = p_data->p_cp_data->param.add_source.advertiser_sid;
                    memcpy(p_src_active->src_id, p_data->p_cp_data->param.add_source.broadcast_id,
                           BROADCAST_ID_LEN);
                    p_src_active->is_past = p_data->p_cp_data->param.add_source.pa_sync;
                    p_src_active->pa_interval = p_data->p_cp_data->param.add_source.pa_interval;

                    if (!memcmp(app_cfg_nv.lea_srcaddr, p_src_active->advertiser_address, 6))
                    {
                        p_src_active->is_encryp = app_cfg_nv.lea_encryp;
                        memcpy(p_src_active->broadcast_Code, app_cfg_nv.lea_code, BROADCAST_CODE_LEN);
                        app_lea_bca_bs_tgt.lea_encryp = app_cfg_nv.lea_encryp;
                        memcpy(app_lea_bca_bs_tgt.lea_code, app_cfg_nv.lea_code, BROADCAST_CODE_LEN);
                    }
                    memcpy(app_cfg_nv.lea_srcaddr, p_src_active->advertiser_address, GAP_BD_ADDR_LEN);
                    app_cfg_nv.lea_srctype = p_src_active->advertiser_address_type;
                    app_cfg_nv.lea_srcsid  = p_src_active->advertiser_sid;
                    app_cfg_nv.lea_srcch = p_data->p_cp_data->param.add_source.p_cp_bis_info->bis_sync;
                    app_cfg_nv.lea_valid = 1;

                    memcpy(app_lea_bca_bs_tgt.target_srcaddr, p_src_active->advertiser_address, GAP_BD_ADDR_LEN);
                    app_lea_bca_bs_tgt.target_lea_srctype = p_src_active->advertiser_address_type;
                    app_lea_bca_bs_tgt.target_lea_srcsid  = p_src_active->advertiser_sid;
                    memcpy(app_lea_bca_bs_tgt.target_lea_bsrid, p_src_active->src_id, BROADCAST_ID_LEN);
                    app_lea_bca_bs_tgt.channel = p_data->p_cp_data->param.add_source.p_cp_bis_info->bis_sync;

                    app_lea_bca_bs_tgt.find |= APP_BIS_BS_FIND_RECORD;
                    app_cfg_store(app_cfg_nv.lea_srcaddr, 12);
                    if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_lea_bca_bs_tgt.find |= APP_BIS_BS_FIND_ALIGN;
                    }
                    if (!p_data->p_cp_data->param.add_source.p_cp_bis_info->bis_sync)
                    {
                        app_lea_bca_tgt_active(true, APP_BIS_BIS_CTRL_ACTIVE);
                    }
                    else
                    {
                        app_lea_bca_tgt_active(true, APP_BIS_BIS_CTRL_SD_ACTIVE);
                    }
                    app_lea_bca_state_change(LEA_BCA_STATE_BASS_ADD_SOURCE);
                    //mtc_topology_dm(MTC_TOPO_EVENT_BIS_START);
                }
                break;

            case BASS_CP_OP_MODIFY_SOURCE:
                /* to synchronize to, or to stop synchronization to, a PA and/or a BIS.
                 Add or update Metadata*/

                break;

            case BASS_CP_OP_REMOVE_SOURCE:
                //app_lea_bca_state_change(LEA_BCA_STATE_IDLE);
                //clear pa info
                break;

            case BASS_CP_OP_SET_BROADCAST_CODE:
                {
                    T_BASS_CP_IND *p_data = (T_BASS_CP_IND *)buf;
                    APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_CP_IND: conn_handle 0x%x, p_src_active->sync_handle 0x%x",
                                    p_data->conn_handle, p_src_active->sync_handle);

                    if (app_lea_bca_bs_tgt.ctrl & APP_BIS_BIS_CTRL_ACTIVE)
                    {

                        T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM sync_param;

                        app_lea_bca_apply_sync_pera(&sync_param, p_src_active->sync_handle, 1);
                        APP_PRINT_INFO5("app_lea_bca_big_establish: %d,%d,%d,%d,%d",
                                        sync_param.encryption,
                                        sync_param.mse,
                                        sync_param.num_bis,
                                        sync_param.bis[0],
                                        sync_param.bis[1]);

                        p_src_active->is_encryp = 1;
                        memcpy(sync_param.broadcast_code, p_data->p_cp_data->param.set_broadcast_code.broadcast_code,
                               BROADCAST_CODE_LEN);
                        memcpy(p_src_active->broadcast_Code, p_data->p_cp_data->param.set_broadcast_code.broadcast_code,
                               BROADCAST_CODE_LEN);

                        if (ble_audio_big_sync_establish(p_src_active->sync_handle, &sync_param))
                        {
                            app_cfg_nv.lea_encryp = 1;
                            memcpy(app_cfg_nv.lea_code, p_src_active->broadcast_Code, BROADCAST_CODE_LEN);
                        }


                    }
                    else
                    {
                        p_src_active->is_encryp = 1;
                        memcpy(p_src_active->broadcast_Code, p_data->p_cp_data->param.set_broadcast_code.broadcast_code,
                               BROADCAST_CODE_LEN);
                        app_cfg_nv.lea_encryp = 1;
                        memcpy(app_cfg_nv.lea_code, p_src_active->broadcast_Code, BROADCAST_CODE_LEN);
                    }

                } break;

            default:
                break;
            }


        }
        break;

    case LE_AUDIO_MSG_BASS_BRS_MODIFY:
        {
            T_BASS_BRS_MODIFY *p_data = (T_BASS_BRS_MODIFY *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_BRS_MODIFY: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
            if (p_data->p_brs_data)
            {
                if (p_data->p_brs_data->brs_is_used)
                {
                    APP_PRINT_INFO2("source_address_type 0x%02x, source_adv_sid 0x%02x",
                                    p_data->p_brs_data->source_address_type, p_data->p_brs_data->source_adv_sid);
                    APP_PRINT_INFO4("pa_sync_state %d, bis_sync_state 0x%x, big_encryption %d, num_subgroups %d",
                                    p_data->p_brs_data->pa_sync_state, p_data->p_brs_data->bis_sync_state,
                                    p_data->p_brs_data->big_encryption, p_data->p_brs_data->num_subgroups);
                    for (uint8_t i = 0; i < p_data->p_brs_data->num_subgroups; i++)
                    {
                        if (p_data->p_brs_data->p_cp_bis_info[i].metadata_len == 0)
                        {
                            APP_PRINT_INFO2("Subgroup[%d], BIS Sync: [0x%x], Metadata Length: [0]", i,
                                            p_data->p_brs_data->p_cp_bis_info[i].bis_sync);
                        }
                        else
                        {
                            APP_PRINT_INFO4("Subgroup[%d], BIS Sync: [0x%x] Metadata data[%d]: %b", i,
                                            p_data->p_brs_data->p_cp_bis_info[i].bis_sync,
                                            p_data->p_brs_data->p_cp_bis_info[i].metadata_len,
                                            TRACE_BINARY(p_data->p_brs_data->p_cp_bis_info[i].metadata_len,
                                                         p_data->p_brs_data->p_cp_bis_info[i].p_metadata));
                        }
                    }
                }
                else
                {
                    APP_PRINT_INFO0("LE_AUDIO_MSG_BASS_BRS_MODIFY: brs char not used");
                }
            }
        }
        break;

    case LE_AUDIO_MSG_BASS_BA_ADD_SOURCE:
        {
            T_BASS_BA_ADD_SOURCE *p_data = (T_BASS_BA_ADD_SOURCE *)buf;

            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_BA_ADD_SOURCE: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
            if (p_src_active != NULL)
            {
                app_lea_bca_bs_set_active(app_lea_bca_sd_idx);
                p_src_active->used |= APP_LEA_BCA_BS;
                p_src_active->sync_handle = (T_BLE_AUDIO_SYNC_HANDLE)p_data->handle;
                p_src_active->source_id = p_data->source_id;
                p_src_active->disallow_pa = true;
                ble_audio_sync_update_cb(p_data->handle, app_lea_bca_sync_cb);
            }
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_PA_SYNC_PARAM:
        {

            T_BASS_GET_PA_SYNC_PARAM *p_data = (T_BASS_GET_PA_SYNC_PARAM *)buf;
            T_APP_LEA_BCA_DB *p_src_active = app_lea_bca_find_db_by_sync_handle(p_data->handle);
            APP_PRINT_INFO4("LE_AUDIO_MSG_BASS_GET_PA_SYNC_PARAM: sync handle %p, source_id %d, is_past %d, pa_interval 0x%x",
                            p_data->handle, p_data->source_id,
                            p_data->is_past, p_data->pa_interval);
            if (p_src_active != NULL)
            {
                p_src_active->is_past = p_data->is_past;
                p_src_active->pa_interval = p_data->pa_interval;
            }
#if 0
            uint16_t pa_sync_timeout_10ms = 200;
            bass_cfg_pa_sync_param(p_data->source_id, 0, 0, pa_sync_timeout_10ms, 300);
#endif
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_BIG_SYNC_PARAM:
        {
            T_BASS_GET_BIG_SYNC_PARAM *p_data = (T_BASS_GET_BIG_SYNC_PARAM *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_BASS_GET_BIG_SYNC_PARAM: sync handle %p, source_id %d",
                            p_data->handle, p_data->source_id);
#if 0
            T_BLE_AUDIO_SYNC_INFO sync_info;
            uint16_t big_sync_timeout_10ms = 200;
            if (ble_audio_sync_get_info(p_data->handle, &sync_info))
            {
                if (sync_info.big_info_received)
                {
                    big_sync_timeout_10ms = ((sync_info.big_info.iso_interval * 1.25) / 10) * 20;
                }

            }
            bass_cfg_big_sync_param(p_data->source_id, 0, big_sync_timeout_10ms);
#endif
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_BROADCAST_CODE:
        {
            T_BASS_GET_BROADCAST_CODE *p_data = (T_BASS_GET_BROADCAST_CODE *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_BASS_GET_BROADCAST_CODE: sync handle %p, source_id %d, 0x%x",
                            p_data->handle, p_data->source_id, app_lea_bca_bs_tgt.ctrl);
        }
        break;

    case LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC:
        {
            T_BASS_SET_PREFER_BIS_SYNC *p_data = (T_BASS_SET_PREFER_BIS_SYNC *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC: sync handle %p, source_id %d, num_subgroups %d",
                            p_data->handle, p_data->source_id,
                            p_data->num_subgroups);

            //According to TWS or headset
            T_BLE_AUDIO_SYNC_INFO sync_info;
            for (uint8_t i = 0; i < p_data->num_subgroups; i++)
            {
                if ((p_data->p_cp_bis_info->bis_sync == BASS_CP_BIS_SYNC_NO_PREFER) &&
                    ble_audio_sync_get_info(p_data->handle, &sync_info))
                {
                    if (sync_info.p_base_mapping)
                    {
                        T_BASE_DATA_MAPPING *p_mapping = sync_info.p_base_mapping;
                        T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM sync_param;
                        uint8_t check_idx = 0;
                        uint8_t  listen_index = 0;
                        memset(&sync_param, 0x00, sizeof(T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM));

                        app_lea_bca_remap_bis(p_mapping, p_data->handle);
                        app_lea_bca_apply_sync_pera(&sync_param, p_data->handle, 0);
                        APP_PRINT_INFO3("app_bis_mgr_bas prefer: %d,%d,%d",
                                        sync_param.num_bis,
                                        sync_param.bis[0],
                                        sync_param.bis[1]);
                        sync_info.p_base_mapping->p_subgroup[i].bis_array = 0;
                        while (sync_param.num_bis > 0)
                        {
                            sync_param.num_bis--;
                            listen_index  |= 1 << (sync_param.bis[check_idx] - 1);
                            check_idx++;
                        }
                        if (!bass_cfg_prefer_bis_sync(p_data->source_id, i, listen_index))
                        {
                            APP_PRINT_INFO0("app_bis_mgr_bas set prefer fail");
                        }
                        else
                        {
                            app_lea_bca_bs_tgt.channel =  GAP_BIG_MGR_MAX_BIS_NUM + 1;
                        }

                    }
                }
            }

        }
        break;

    case LE_AUDIO_MSG_BASS_BRS_CHAR_NO_EMPTY:
        {
            T_BASS_BRS_CHAR_NO_EMPTY *p_data = (T_BASS_BRS_CHAR_NO_EMPTY *)buf;
            APP_PRINT_INFO1("LE_AUDIO_MSG_BASS_BRS_CHAR_NO_EMPTY:conn_handle 0x%x",
                            p_data->conn_handle);
            //ble_audio_sync_release(&bqb_bap_sde_sink.sync_handle);
        }
        break;
    default:
        break;
    }
    return cb_result;
}

static uint16_t app_lea_bca_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    uint16_t msg_group;

    msg_group = msg & 0xFF00;
    switch (msg_group)
    {
    case LE_AUDIO_MSG_GROUP_SERVER:
        cb_result = app_lea_bca_handle_cccd_msg(msg, buf);
        break;

    case LE_AUDIO_MSG_GROUP_BASS:
        cb_result = app_lea_bca_handle_bass_msg(msg, buf);
        break;

    default:
        break;
    }
    return cb_result;
}

void app_lea_bca_state_change(T_APP_LEA_BCA_STATE state)
{
    APP_PRINT_INFO2("app_lea_bca_state_change: change from %d to %d", app_lea_bca_state_machine,
                    state);
    app_lea_bca_state_machine = state;
}

T_APP_LEA_BCA_STATE app_lea_bca_state(void)
{
    APP_PRINT_INFO1("app_lea_bca_state: app_lea_bca_state_machine %d", app_lea_bca_state_machine);
    return app_lea_bca_state_machine;
}


bool app_lea_bca_state_idle(uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_lea_bca_state_idle: event %x, state %x",
                    event, app_lea_bca_state_machine);
    bool accept = true;
    switch (event)
    {
    case LEA_MMI_BCA:
        {
            // if ((app_link_get_b2s_link_num() == 0) &&
            //     app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
            // {
            //     app_audio_speaker_channel_set(AUDIO_SINGLE_SET, 0);
            // }
            app_audio_tone_type_play(TONE_BIS_START, false, false);

            app_lea_bca_state_change(LEA_BCA_STATE_PRE_SCAN);
        }
        break;

    default:
        accept = false;
        break;
    }
    return accept;
}

bool app_lea_bca_state_starting(uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_lea_bca_state_starting: event %x, state %x",
                    event, app_lea_bca_state_machine);
    bool accept = true;

    switch (event)
    {
    case LEA_BIG_SYNC:
        {
            uint8_t dev_idx = app_lea_bca_bs_get_active();

            app_lea_bca_tgt_active(true, APP_BIS_BIS_CTRL_ACTIVE);

            if (app_lea_bca_pa_sync(dev_idx))
            {
                APP_PRINT_INFO1("app_lea_bca_state_starting: pa_sync active_idx: 0x%x", dev_idx);
            }
            else
            {
                app_lea_bca_used();
                app_lea_acc_scan_start();
                app_lea_bca_state_change(LEA_BCA_STATE_SCAN);
            }
        }
        break;

    default:
        accept = false;
        break;
    }
    return accept;
}

bool app_lea_bca_state_bass_add_source(uint8_t event, void *p_data)
{
    APP_PRINT_TRACE2("app_lea_bca_state_bass_add_source: event 0x%02X, state 0x%02X",
                     event, app_lea_bca_state_machine);
    bool accept = true;

    switch (event)
    {
    case LEA_STREAMING:
        {
            app_lea_bca_state_change(LEA_BCA_STATE_STREAMING);
        }
        break;

    default:
        accept = false;
        break;
    }
    return accept;
}

bool app_lea_bca_state_scan(uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_lea_bca_state_scan: event 0x%x, state 0x%x",
                    event, app_lea_bca_state_machine);
    bool accept = true;
    switch (event)
    {
    case LEA_BIG_TERMINATE:
    case LEA_SCAN_STOP:
    case LEA_SCAN_TIMEOUT:
        {
            uint8_t dev_idx = app_lea_bca_bs_get_active();

            app_lea_acc_scan_stop();
            app_lea_bca_tgt_active(false, (APP_BIS_BIS_CTRL_RESET_ACTIVE & APP_BIS_BIS_CTRL_RESET_SD_ACTIVE));

            if (dev_idx != APP_LEA_BCA_DB_NUM)
            {
                if (app_lea_bca_terminate())
                {
                    app_lea_bca_state_change(LEA_BCA_STATE_IDLE);
                }
                else
                {
                    app_lea_bca_state_change(LEA_BCA_STATE_WAIT_TERM);
                }
            }
            else
            {
                app_lea_bca_bs_reset();
                app_lea_bca_state_change(LEA_BCA_STATE_IDLE);
            }

        }
        break;

    case LEA_STREAMING:
        {
            app_lea_bca_state_change(LEA_BCA_STATE_STREAMING);
        }
        break;

    default:
        accept = false;
        break;
    }
    return accept;
}

bool app_lea_bca_state_streaming(uint8_t event, void *p_data)
{
    APP_PRINT_INFO3("app_lea_bca_state_streaming: event 0x%x, state 0x%x, 0x%x",
                    event, app_lea_bca_state_machine, app_lea_bca_get_sd_source());
    bool accept = true;

    switch (event)
    {
    case LEA_A2DP_START:
    case LEA_BIG_TERMINATE:
        {
            app_lea_bca_tgt_active(false, (APP_BIS_BIS_CTRL_RESET_ACTIVE & APP_BIS_BIS_CTRL_RESET_SD_ACTIVE));
            app_lea_bca_terminate();

        }
        break;

    default:
        accept = false;
        break;
    }

    return accept;
}

bool app_lea_bca_sm(uint8_t event, void *p_data)
{
    APP_PRINT_INFO2("app_lea_bca_sm: event 0x%x, state 0x%x", event,
                    app_lea_bca_state_machine);
    bool accept = true;

    switch (app_lea_bca_state_machine)
    {
    case LEA_BCA_STATE_IDLE:
        {
            accept = app_lea_bca_state_idle(event, p_data);
        }
        break;
    case LEA_BCA_STATE_PRE_SCAN:
        {
            accept = app_lea_bca_state_starting(event, p_data);
        }
        break;

    case LEA_BCA_STATE_BASS_ADD_SOURCE:
        {
            accept = app_lea_bca_state_bass_add_source(event, p_data);
        }
        break;

    case LEA_BCA_STATE_SCAN:
        {
            accept = app_lea_bca_state_scan(event, p_data);
        }
        break;

    case LEA_BCA_STATE_STREAMING:
        {
            accept = app_lea_bca_state_streaming(event, p_data);
        }
        break;

    default:
        accept = false;
        break;
    }
    return accept;
}
///////////////////////////////////////////////////////////////

void app_lea_bca_bmr_terminate(void)
{
    app_lea_bca_sm(LEA_BIG_TERMINATE, NULL);

    if (bis_policy == LEA_BIS_POLICY_RANDOM &&
        ((bis_mode == LEA_BROADCAST_SINK) ||
         ((app_link_get_le_link_num() == 0) &&
          (bis_mode & LEA_BROADCAST_DELEGATOR)))
       )
    {
        uint8_t non_addr[6] = { 0};
        app_lea_bca_tgt_addr(false, non_addr);
        app_lea_bca_tgt(APP_LEA_BCA_CLR);
    }
}

bool app_lea_mgr_is_streaming(void)
{
    for (uint8_t idx = 0; idx < MAX_BLE_LINK_NUM; idx++)
    {
        if (app_db.le_link[idx].used &&
            app_db.le_link[idx].lea_link_state == LEA_LINK_STREAMING)
        {
            return true;
        }
    }

    if (app_lea_bca_state() == LEA_BCA_STATE_STREAMING)
    {
        return true;
    }

    return false;
}

uint8_t app_lea_mgr_bis_spout_gain(uint8_t *para)
{
    if (para != NULL)
    {
        app_cfg_nv.bis_audio_gain_level = *para;
    }
    return app_cfg_nv.bis_audio_gain_level;
}

static void app_lea_mgr_set_bis_spk_out(uint8_t action)
{
    uint8_t result = 0;
    bool track_spk_state = false;
    T_APP_LEA_BCA_SPK_OUT spk_state = app_lea_bca_get_spk_out();
    T_AUDIO_TRACK_HANDLE bis_audio_track_handle = app_lea_bca_get_track_handle();

    if (bis_audio_track_handle == NULL)
    {
        result = 1;
        goto RESULT_FAIL;
    }

    if ((spk_state == LEA_BCA_SPK_OUT_MUTE && action == MMI_DEV_SPK_MUTE) ||
        (spk_state == LEA_BCA_SPK_OUT_UNMUTE && action == MMI_DEV_SPK_UNMUTE))
    {
        result = 2;
        goto RESULT_FAIL;
    }

    audio_track_volume_out_is_muted(bis_audio_track_handle, &track_spk_state);

    if (track_spk_state && audio_track_volume_out_unmute(bis_audio_track_handle))
    {
        app_lea_bca_set_spk_out(LEA_BCA_SPK_OUT_UNMUTE);
    }
    else  if (!track_spk_state && audio_track_volume_out_mute(bis_audio_track_handle))
    {
        app_lea_bca_set_spk_out(LEA_BCA_SPK_OUT_MUTE);
    }
    else
    {
        result = 3;
        goto RESULT_FAIL;
    }

    return;

RESULT_FAIL:
    APP_PRINT_INFO3("app_lea_mgr_set_bis_spk_out: fail cause %d, spk_state %d, action %d", -result,
                    spk_state, action);
}

static bool app_lea_bca_check_combination_iso_data(T_ISO_DATA_IND **p_iso_elem,
                                                   uint8_t *output_channel,
                                                   T_BT_DIRECT_CB_DATA *p_data)
{
    T_OS_QUEUE *p_mirror_queue = NULL;
    T_OS_QUEUE *p_insert_queue = NULL;
    uint16_t output_handle = 0;
    uint8_t ret = 0;
    T_APP_LEA_BCA_DB *p_bc_source = NULL;

    *output_channel = AUDIO_LOCATION_MONO;

    p_bc_source = &app_lea_bca_db[app_lea_bca_bs_get_active()];
    if (p_bc_source == NULL)
    {
        ret = 1;
        goto fail_get_iso;
    }

    if (p_bc_source->left_conn_handle == p_data->p_bt_direct_iso->conn_handle)
    {
        if (p_data->p_bt_direct_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
        {
            p_bc_source->left_ch_iso_state  = p_data->p_bt_direct_iso->pkt_status_flag;
        }

        p_mirror_queue = &app_lea_bca_right_ch_queue;
        p_insert_queue = &app_lea_bca_left_ch_queue;
        output_handle = p_bc_source->left_conn_handle;
        *output_channel = AUDIO_LOCATION_FL;
    }
    else if (p_bc_source->right_conn_handle ==
             p_data->p_bt_direct_iso->conn_handle)
    {
        if (p_data->p_bt_direct_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
        {
            p_bc_source->right_ch_iso_state  = p_data->p_bt_direct_iso->pkt_status_flag;
        }

        p_mirror_queue = &app_lea_bca_left_ch_queue;
        p_insert_queue = &app_lea_bca_right_ch_queue;
        output_handle = p_bc_source->right_conn_handle;
        *output_channel = AUDIO_LOCATION_FR;
    }
    else
    {
        ret = 2;
        goto fail_get_iso;
    }

    if ((p_bc_source->left_ch_iso_state  != ISOCH_DATA_PKT_STATUS_VALID_DATA) ||
        (p_bc_source->right_ch_iso_state  != ISOCH_DATA_PKT_STATUS_VALID_DATA))
    {
        ret = 3;
        goto fail_get_iso;
    }

    if (p_mirror_queue->count == 0)
    {
        //Wait for the ISO data in another channel to combine a stereo packet
        app_lea_mgr_enqueue_iso_data(p_insert_queue, p_data->p_bt_direct_iso, output_handle);
    }
    else
    {
        *p_iso_elem = app_lea_mgr_find_iso_elem(p_mirror_queue, p_data->p_bt_direct_iso);
        if (*p_iso_elem == NULL)
        {
            app_lea_mgr_enqueue_iso_data(p_insert_queue, p_data->p_bt_direct_iso, output_handle);
            ret = 4;
            goto fail_get_iso;
        }
    }

    return true;

fail_get_iso:
    APP_PRINT_ERROR7("app_lea_bca_check_combination_iso_data: result %d, left ch %d, right ch %d, \
left handle 0x%02X, right handle 0x%02X, data handle 0x%02X, output_channel %d",
                     -ret,
                     p_bc_source->left_ch_iso_state,
                     p_bc_source->right_ch_iso_state,
                     p_bc_source->left_conn_handle,
                     p_bc_source->right_conn_handle,
                     p_data->p_bt_direct_iso->conn_handle,
                     *output_channel);

    return false;
}

void app_lea_bca_handle_iso_data(T_BT_DIRECT_CB_DATA *p_data)
{
    T_LEA_BCA_SECTION *p_bis_cb = app_lea_bca_find_con_handle(p_data->p_bt_direct_iso->conn_handle);
    T_APP_LEA_BCA_DB *p_bc_source = NULL;
    uint8_t ret = 0;

    p_bc_source = &app_lea_bca_db[app_lea_bca_bs_get_active()];
    if (p_bc_source == NULL)
    {
        ret = 1;
        goto fail_track_write;
    }

    if (p_bis_cb == NULL)
    {
        ret = 2;
        goto fail_track_write;
    }

    if (p_bc_source->left_conn_handle != 0 &&
        p_bc_source->right_conn_handle != 0)
    {
        T_OS_QUEUE *p_mirror_queue = NULL;
        T_ISO_DATA_IND *p_iso_elem = NULL;
        uint16_t pkt_seq_num;
        uint32_t pkt_timestamp;
        uint16_t written_len;
        T_AUDIO_STREAM_STATUS status;
        uint8_t *p_send_buf = NULL;
        uint16_t sdu_len = 0;
        uint8_t output_channel = AUDIO_LOCATION_MONO;
        T_AUDIO_TRACK_STATE track_state;

        audio_track_state_get(p_bis_cb->audio_track_handle, &track_state);
        if (track_state != AUDIO_TRACK_STATE_STARTED)
        {
            ret = 3;
            goto fail_track_write;
        }

        if (app_lea_bca_check_combination_iso_data(&p_iso_elem, &output_channel, p_data) == false)
        {
            ret = 4;
            goto fail_track_write;
        }

        if (p_iso_elem != NULL)
        {
            sdu_len = p_bis_cb->bis_codec_cfg.octets_per_codec_frame * p_bis_cb->frame_num;

            p_send_buf = audio_probe_media_buffer_malloc(sdu_len);

            if (p_send_buf == NULL)
            {
                ret = 5;
                goto fail_track_write;
            }

            if (p_data->p_bt_direct_iso->iso_sdu_len != 0 && p_iso_elem->iso_sdu_len != 0)
            {
                status = AUDIO_STREAM_STATUS_CORRECT;
            }
            else
            {
                status = AUDIO_STREAM_STATUS_LOST;
                sdu_len = 0;
            }

            if (output_channel == AUDIO_LOCATION_FL)
            {
                p_mirror_queue = &app_lea_bca_right_ch_queue;
                if (status == AUDIO_STREAM_STATUS_CORRECT)
                {
                    memcpy(p_send_buf, p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset,
                           p_data->p_bt_direct_iso->iso_sdu_len);

                    memcpy(p_send_buf + p_data->p_bt_direct_iso->iso_sdu_len, p_iso_elem->p_buf,
                           p_iso_elem->iso_sdu_len);
                }

                pkt_seq_num = p_data->p_bt_direct_iso->pkt_seq_num;
                pkt_timestamp = p_data->p_bt_direct_iso->time_stamp;
            }
            else
            {
                p_mirror_queue = &app_lea_bca_left_ch_queue;
                if (status == AUDIO_STREAM_STATUS_CORRECT)
                {
                    memcpy(p_send_buf, p_iso_elem->p_buf,  p_iso_elem->iso_sdu_len);
                    memcpy(p_send_buf + p_iso_elem->iso_sdu_len,
                           p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset,
                           p_data->p_bt_direct_iso->iso_sdu_len);
                }

                pkt_seq_num = p_iso_elem->pkt_seq_num;
                pkt_timestamp = p_iso_elem->time_stamp;
            }

            audio_track_write(p_bis_cb->audio_track_handle, pkt_timestamp,
                              pkt_seq_num,
                              status,
                              FIXED_FRAME_NUM,
                              p_send_buf,
                              sdu_len,
                              &written_len);

            os_queue_delete(p_mirror_queue, p_iso_elem);
            audio_probe_media_buffer_free(p_iso_elem);
            audio_probe_media_buffer_free(p_send_buf);
        }
    }
    else
    {
        T_AUDIO_TRACK_STATE track_state;
        T_AUDIO_STREAM_STATUS status;
        uint16_t written_len;
        uint8_t *p_iso_data = p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset;

        audio_track_state_get(p_bis_cb->audio_track_handle, &track_state);
        if (track_state != AUDIO_TRACK_STATE_STARTED)
        {
            ret = 6;
            goto fail_track_write;
        }

        if (p_data->p_bt_direct_iso->iso_sdu_len != 0)
        {
            status = AUDIO_STREAM_STATUS_CORRECT;
        }
        else
        {
            status = AUDIO_STREAM_STATUS_LOST;
        }

        audio_track_write(p_bis_cb->audio_track_handle,
                          p_data->p_bt_direct_iso->time_stamp,
                          p_data->p_bt_direct_iso->pkt_seq_num,
                          status,
                          FIXED_FRAME_NUM,
                          p_iso_data,
                          p_data->p_bt_direct_iso->iso_sdu_len,
                          &written_len);
    }

fail_track_write:
    if (ret != 0)
    {
        APP_PRINT_ERROR1("app_lea_bca_handle_iso_data: failed %d", -ret);
    }
}

void app_lea_bca_scan_info(T_LEA_BRS_INFO *src_info)
{
    if (app_lea_bca_tgt(APP_LEA_BCA_GET) == APP_BIS_BS_FIND_RECORD_ALIGN)
    {
        app_lea_bca_bs_handle_pa_info_sync(src_info);
    }
    else
    {
        uint8_t tgt_addr[6] = {0};

        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            app_lea_bca_tgt_addr(true, tgt_addr);
            if (app_lea_bca_tgt(APP_LEA_BCA_GET) == APP_BIS_BS_FIND_RECORD &&
                !memcmp(tgt_addr, src_info->adv_addr, 6))
            {
                app_lea_bca_tgt(APP_BIS_BS_FIND_ALIGN);
                app_lea_bca_bs_handle_pa_info_sync(src_info);
            }
            else
            {
                uint8_t non_addr[6] = {0};

                if (!memcmp(tgt_addr, non_addr, 6))
                {
                    app_lea_bca_tgt_addr(false, src_info->adv_addr);

                    app_lea_bca_tgt(APP_BIS_BS_FIND_RECORD_ALIGN);
                    app_lea_bca_bs_handle_pa_info_sync(src_info);

                }
            }
        }
    }
}

void app_lea_mgr_tri_mmi_handle_action(uint8_t action, bool inter)
{
    APP_PRINT_INFO2("app_lea_mgr_tri_mmi_handle_action: action: 0x%x, allowed_source %x",
                    action, inter);
    switch (action)
    {

    case MMI_BIG_START:
        {
            bool need_return = false;
            if (app_lea_bca_state() == LEA_BCA_STATE_IDLE)
            {
                app_lea_bca_sm(LEA_MMI_BCA, NULL);
            }
            else if (!inter)
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_AUDIO);
                break;
            }
            app_lea_bca_sm(LEA_BIG_SYNC, NULL);
            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_AUDIO);
        }
        break;

    case MMI_BIG_STOP:
        {
            app_audio_tone_type_play(TONE_BIS_STOP, false, false);

            app_lea_bca_bmr_terminate();
        }
        break;


    default:
        break;
    }
}

void app_lea_bca_init(void)
{
    os_queue_init(&app_lea_bca_left_ch_queue);
    os_queue_init(&app_lea_bca_right_ch_queue);

    ble_audio_cback_register(app_lea_bca_ble_audio_cback);
    app_lea_bca_db_init();
}

#endif

