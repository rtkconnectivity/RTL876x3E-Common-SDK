/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_VCS_SUPPORT
#include <string.h>
#include "trace.h"
#include "gap_conn_le.h"
#include "audio_track.h"
#include "ble_audio.h"
#include "metadata_def.h"
#include "vcs_mgr.h"
#include "app_lea_acc_ascs.h"
#include "app_lea_acc_profile.h"
#include "app_lea_acc_vcs.h"
#include "app_lea_acc_vol_def.h"
#include "app_lea_acc_mcp.h"
#include "app_lea_acc_ccp.h"
#include "app_lea_acc_broadcast_audio.h"
#include "app_lea_acc_vol_policy.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_types.h"

typedef struct
{
    uint8_t vol_gain;
    bool    mute;
} T_LEA_VOL_SYNC_INFO;

typedef enum
{
    LEA_VOL_MSG_VOLUME_SYNC = 0x01,
    LEA_VOL_MSG_TOTAL
} T_LEA_VOL_REMOTE_MSG;

T_LEA_VOL_CHG_RESULT app_lea_vol_local_volume_change(T_LEA_VOL_CHG_TYPE type)
{
    uint8_t level;
    T_LEA_VOL_CHG_RESULT result = LEA_VOL_NONE;
    T_VCS_PARAM vcs_param;

    vcs_get_param(&vcs_param);
    app_cfg_nv.lea_vol_setting = vcs_param.volume_setting;
    app_cfg_nv.lea_vol_out_mute = vcs_param.mute;

    switch (type)
    {
    case LEA_SPK_UP:
        {
            if (app_cfg_nv.lea_vol_setting != MAX_VOLUME_SETTING)
            {
                if ((MAX_VOLUME_SETTING - app_cfg_nv.lea_vol_setting) > VOLUME_STEP_SIZE)
                {
                    app_cfg_nv.lea_vol_setting += VOLUME_STEP_SIZE;
                }
                else
                {
                    app_cfg_nv.lea_vol_setting = MAX_VOLUME_SETTING;
                }
                result = LEA_VOL_LEVEL_CHANGE;
            }
            else
            {
                result = LEA_VOL_LEVEL_LIMIT;
                goto level_limit;
            }
        }
        break;

    case LEA_SPK_DOWN:
        {
            if (app_cfg_nv.lea_vol_setting != 0)
            {
                if (app_cfg_nv.lea_vol_setting > VOLUME_STEP_SIZE)
                {
                    app_cfg_nv.lea_vol_setting -= VOLUME_STEP_SIZE;
                }
                else
                {
                    app_cfg_nv.lea_vol_setting = 0;
                }
                result = LEA_VOL_LEVEL_CHANGE;
            }
            else
            {
                result = LEA_VOL_LEVEL_LIMIT;
                goto level_limit;
            }
        }
        break;

    case LEA_SPK_MUTE:
        {
            if (app_cfg_nv.lea_vol_out_mute != VCS_MUTED)
            {
                app_cfg_nv.lea_vol_out_mute = VCS_MUTED;
                result = LEA_VOL_MUTE_CHANGE;
            }
            else
            {
                result = LEA_VOL_MUTE_UNCHANGE;
                goto mute_unchange;
            }
        }
        break;

    case LEA_SPK_UNMUTE:
        {
            if (app_cfg_nv.lea_vol_out_mute != VCS_NOT_MUTED)
            {
                app_cfg_nv.lea_vol_out_mute = VCS_NOT_MUTED;
                result = LEA_VOL_MUTE_CHANGE;
            }
            else
            {
                result = LEA_VOL_MUTE_UNCHANGE;
                goto mute_unchange;
            }
        }
        break;

    default:
        break;
    }

    if (result == LEA_VOL_LEVEL_CHANGE)
    {
        level = VOLUME_LEVEL(app_cfg_nv.lea_vol_setting);
        if ((level == 0) && app_cfg_nv.lea_vol_setting)
        {
            level = 1;
        }

        if (level > 0)
        {
            vcs_param.mute = VCS_NOT_MUTED;
            app_cfg_nv.lea_vol_out_mute = VCS_NOT_MUTED;
        }
    }

    vcs_param.volume_setting = app_cfg_nv.lea_vol_setting;
    vcs_param.mute = app_cfg_nv.lea_vol_out_mute;
    vcs_param.volume_flags = VCS_USER_SET_VOLUME_SETTING;
    vcs_param.change_counter++;

    app_cfg_nv.lea_vcs_change_cnt = vcs_param.change_counter;

    vcs_set_param(&vcs_param);

level_limit:
mute_unchange:
    APP_PRINT_TRACE2("app_lea_vol_local_volume_change: type 0x%02X, ret %d",
                     type, result);

    return result;
}

void app_lea_vol_update_track_volume(void)
{
    T_APP_LEA_BCA_STATE bis_state = app_lea_bca_state();
    T_AUDIO_TRACK_HANDLE track_handle = NULL;
    uint8_t level;
    uint8_t curr_vol;
    uint8_t ret = 0;

    if (bis_state == LEA_BCA_STATE_STREAMING)
    {
        track_handle = app_lea_bca_get_track_handle();
        if (track_handle == NULL)
        {
            ret = 4;
            goto fail;
        }
    }
    else
    {
        T_APP_LE_LINK *p_link = NULL;
        T_LEA_ASE_ENTRY *p_ase_entry;
        uint16_t conn_handle = 0;

        if (app_lea_ccp_get_call_status() != APP_CALL_IDLE)
        {
            conn_handle = app_lea_ccp_get_active_conn_handle();
        }
        else
        {
            conn_handle = app_lea_mcp_get_active_conn_handle();
        }

        p_link = app_link_find_le_link_by_conn_handle(conn_handle);
        if (p_link == NULL)
        {
            ret = 1;
            goto fail;
        }

        p_ase_entry = app_lea_ascs_find_ase_entry_by_direction(p_link, DATA_PATH_OUTPUT_FLAG);
        if (p_ase_entry == NULL)
        {
            ret = 2;
            goto fail;
        }

        track_handle = p_ase_entry->track_handle;
        if (track_handle == NULL)
        {
            ret = 3;
            goto fail;
        }
    }

    level = VOLUME_LEVEL(app_cfg_nv.lea_vol_setting);
    if ((level == 0) && app_cfg_nv.lea_vol_setting)
    {
        level = 1;
    }

    if (audio_track_volume_out_get(track_handle, &curr_vol) && (level != curr_vol))
    {
        audio_track_volume_out_set(track_handle, level);
    }

    if (app_cfg_nv.lea_vol_out_mute == VCS_MUTED)
    {
        audio_track_volume_out_mute(track_handle);
    }
    else
    {
        audio_track_volume_out_unmute(track_handle);
    }

fail:
    APP_PRINT_ERROR4("app_lea_vol_update_track_volume: bis_state 0x%02X, level %d, mute %d, ret %d",
                     bis_state, level, app_cfg_nv.lea_vol_out_mute, -ret);
}

static uint16_t app_lea_vol_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    if ((msg & 0xFF00) == LE_AUDIO_MSG_GROUP_VCS)
    {
        APP_PRINT_TRACE1("app_lea_vol_ble_audio_cback: msg 0x%04X", msg);
    }
    else
    {
        return cb_result;
    }

    switch (msg)
    {
    case LE_AUDIO_MSG_VCS_VOLUME_CP_IND:
        {
            T_APP_LE_LINK *p_link;
            T_VCS_VOLUME_CP_IND *p_vcs_vol_state = (T_VCS_VOLUME_CP_IND *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_vcs_vol_state->conn_handle);
            if ((p_link != NULL))
            {
                app_lea_vol_update_track_volume();
            }
        }
        break;

    default:
        break;
    }

    return cb_result;
}

void app_lea_vol_init(void)
{
    ble_audio_cback_register(app_lea_vol_ble_audio_cback);
}
#endif
