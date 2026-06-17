/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "string.h"
#include "trace.h"
#include "app_timer.h"
#include "btm.h"
#include "bt_avrcp.h"
#include "bt_bond.h"
#include "app_cfg_nv.h"
#include "app_dsp_cfg.h"
#include "app_link_util_cs.h"
#include "app_avrcp_cfg.h"
#include "app_avrcp.h"
#include "app_a2dp.h"
#include "app_main.h"

#include "audio_volume.h"
#include "app_audio_cfg.h"
#include "app_audio_policy.h"
#include "audio_track.h"
#include "app_bt_policy_api.h"
#include "app_report.h"

#include "app_cmd.h"
#include "app_multilink.h"
#include "app_bond.h"

#if F_APP_AVRCP_CMD_SUPPORT
#define ALL_ELEMENT_ATTR                        0x00
#define MAX_NUM_OF_ELEMENT_ATTR                 0x07
#endif


typedef enum
{
    CHK_ABS_VOL_SUPPORTED,
} T_APP_AVRCP_TIMER_ID;


enum STATUS
{
    AVRCP_PLAY     = 0x01,
    AVRCP_PAUSE    = 0x02,
    AVRCP_STOP     = 0x03,
    AVRCP_FORWARD  = 0x04,
    AVRCP_BACKWARD = 0x05,
    AVRCP_VOLUME_CHANGED = 0x06,
};


static struct
{
    uint8_t tmr_id;
    uint8_t chk_abs_vol_tmr_idx;
} avrcp =
{
    .tmr_id = 0,
    .chk_abs_vol_tmr_idx = 0,
};


static void app_avrcp_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case CHK_ABS_VOL_SUPPORTED:
        {
            app_avrcp_stop_abs_vol_check_timer();

            /* not support abs vol and no buds has vol control; set dac gain to max */
            if (app_cfg_nv.either_bud_has_vol_ctrl_key == false)
            {
                uint8_t pair_idx;
                uint8_t app_idx = (uint8_t)param;
                uint8_t *addr = app_db.br_link[app_idx].bd_addr;

                T_APP_BR_LINK *p_link = app_link_find_br_link(addr);

                if (p_link && app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx))
                {
                    app_cfg_nv.audio_gain_level[pair_idx] = app_dsp_cfg_vol.playback_volume_max;
                    p_link->avrcp.abs_vol_state = ABS_VOL_NOT_SUPPORTED;

                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                    {
                        if (p_link->a2dp.track_handle)
                        {
                            audio_track_volume_out_set(p_link->a2dp.track_handle, app_dsp_cfg_vol.playback_volume_max);
                            app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
                        }
                    }
                    else
                    {
                        app_avrcp_sync_abs_vol_state(p_link->bd_addr, ABS_VOL_NOT_SUPPORTED);
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}


static void app_avrcp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;



    switch (event_type)
    {
    case BT_EVENT_AVRCP_CONN_IND:
        {
            p_link = app_link_find_br_link(param->avrcp_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_avrcp_connect_cfm(p_link->bd_addr, true);
            }
        }
        break;

    case BT_EVENT_AVRCP_BROWSING_CONN_IND:
        {
            p_link = app_link_find_br_link(param->avrcp_browsing_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_avrcp_browsing_connect_cfm(p_link->bd_addr, true);
            }
        }
        break;

    case BT_EVENT_AVRCP_GET_CAPABILITIES_RSP:
        {
            p_link = app_link_find_br_link(param->avrcp_browsing_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                uint8_t  capability_count;
                uint8_t *capabilities;

                capability_count = param->avrcp_get_capabilities_rsp.capability_count;
                capabilities = param->avrcp_get_capabilities_rsp.capabilities;
                while (capability_count != 0)
                {
                    bt_avrcp_register_notification_req(p_link->bd_addr, *capabilities);
                    capability_count -= 1;
                    capabilities += 1;
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_ABSOLUTE_VOLUME_SET:
        {
            uint8_t vol;

            p_link = app_link_find_br_link(param->avrcp_absolute_volume_set.bd_addr);
            if (p_link != NULL)
            {
                uint8_t pair_idx_mapping;

                app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);

                // if enable_align_default_volume_after_factory_reset, and a support abs vol phone first connect, sync default vol to src
                if ((p_link->avrcp.abs_vol_state == ABS_VOL_SUPPORTED) &&
                    (app_bt_policy_get_first_connect_sync_default_vol_flag()))
                {
                    vol = (app_dsp_cfg_vol.playback_volume_default * 0x7F +
                           app_dsp_cfg_vol.playback_volume_max / 2) /
                          app_dsp_cfg_vol.playback_volume_max;

                    app_cfg_nv.audio_gain_level[pair_idx_mapping] = app_dsp_cfg_vol.playback_volume_default;

                    bt_avrcp_volume_change_req(param->avrcp_absolute_volume_set.bd_addr, vol);
                    app_bt_policy_set_first_connect_sync_default_vol_flag(false);
                }
                else
                {
                    app_cfg_nv.audio_gain_level[pair_idx_mapping] = (param->avrcp_absolute_volume_set.volume *
                                                                     app_dsp_cfg_vol.playback_volume_max + 0x7F / 2) / 0x7F;
                }

                if (app_audio_cfg.play_max_min_tone_when_adjust_vol_from_phone)
                {
                    static bool is_first_max_min_vol_set_from_phone = false;

                    vol = app_cfg_nv.audio_gain_level[pair_idx_mapping];
                    if ((vol == app_dsp_cfg_vol.playback_volume_max)
                        || (vol == app_dsp_cfg_vol.playback_volume_min)
                       )
                    {
                        if (!is_first_max_min_vol_set_from_phone)
                        {
                            is_first_max_min_vol_set_from_phone = true;
                            app_audio_set_max_min_vol_from_phone_flag(true);
                        }
                    }
                    else
                    {
                        is_first_max_min_vol_set_from_phone = false;
                    }
                }

                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_audio_vol_set(p_link->a2dp.track_handle, app_cfg_nv.audio_gain_level[pair_idx_mapping]);
                    app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
                }
                else
                {
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_VOLUME_UP:
        {
            p_link = app_link_find_br_link(param->avrcp_volume_up.bd_addr);
            if (p_link != NULL)
            {
                uint8_t pair_idx_mapping;
                uint8_t curr_volume;

                app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);
                curr_volume = app_cfg_nv.audio_gain_level[pair_idx_mapping];

                if (curr_volume < app_dsp_cfg_vol.playback_volume_max)
                {
                    curr_volume++;
                }
                else
                {
                    curr_volume = app_dsp_cfg_vol.playback_volume_max;
                }

                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    audio_track_volume_out_set(p_link->a2dp.track_handle, curr_volume);
                    app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;
                    app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
                }
                else
                {

                }
            }
        }
        break;

    case BT_EVENT_AVRCP_VOLUME_DOWN:
        {
            p_link = app_link_find_br_link(param->avrcp_volume_down.bd_addr);
            if (p_link != NULL)
            {
                uint8_t pair_idx_mapping;
                uint8_t curr_volume;

                app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);

                curr_volume = app_cfg_nv.audio_gain_level[pair_idx_mapping];

                if (curr_volume > app_dsp_cfg_vol.playback_volume_min)
                {
                    curr_volume--;
                }
                else
                {
                    curr_volume = app_dsp_cfg_vol.playback_volume_min;
                }

                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    audio_track_volume_out_set(p_link->a2dp.track_handle, curr_volume);
                    app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;
                    app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
                }
                else
                {

                }
            }
        }
        break;

    case BT_EVENT_AVRCP_REG_VOLUME_CHANGED:
        {
            p_link = app_link_find_br_link(param->avrcp_reg_volume_changed.bd_addr);
            if (p_link != NULL)
            {
                uint8_t pair_idx_mapping;
                uint8_t vol;

                app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);

                if (app_bt_policy_get_first_connect_sync_default_vol_flag())
                {
                    app_cfg_nv.audio_gain_level[pair_idx_mapping] = app_dsp_cfg_vol.playback_volume_default;
                }

                vol = (app_cfg_nv.audio_gain_level[pair_idx_mapping] * 0x7F +
                       app_dsp_cfg_vol.playback_volume_max / 2) /
                      app_dsp_cfg_vol.playback_volume_max;

                if (bt_avrcp_volume_change_register_rsp(p_link->bd_addr, vol))
                {
                    app_avrcp_stop_abs_vol_check_timer();

                    p_link->avrcp.abs_vol_state = ABS_VOL_SUPPORTED;
                    app_avrcp_sync_abs_vol_state(p_link->bd_addr, ABS_VOL_SUPPORTED);
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_CONN_CMPL:
        {
            p_link = app_link_find_br_link(param->avrcp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (avrcp.chk_abs_vol_tmr_idx == 0)
                {
#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE == 0)
                    app_start_timer(&avrcp.chk_abs_vol_tmr_idx, "check_abs_vol_supported",
                                    avrcp.tmr_id, CHK_ABS_VOL_SUPPORTED, p_link->id, false,
                                    1500);
#endif
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_DISCONN_CMPL:
        {
            p_link = app_link_find_br_link(param->avrcp_disconn_cmpl.bd_addr);

            if (p_link != NULL)
            {
                p_link->avrcp.abs_vol_state = ABS_VOL_NOT_SUPPORTED;
                app_avrcp_sync_abs_vol_state(p_link->bd_addr, ABS_VOL_NOT_SUPPORTED);
            }
        }
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_RSP:
    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED:
        {
            p_link = app_link_find_br_link(param->avrcp_play_status_changed.bd_addr);

            if (p_link != NULL)
            {
                p_link->avrcp.play_status = param->avrcp_play_status_changed.play_status;
                if (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    app_audio_set_bud_stream_state(BUD_STREAM_STATE_AUDIO);

                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_MEDIAPLAYER_PLAYING);
                }
                else if (param->avrcp_play_status_changed.play_status == BT_AVRCP_PLAY_STATUS_PAUSED)
                {
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_MEDIAPLAYER_PAUSED);
                }
            }
        }
        break;

//#if F_APP_DEVICE_CMD_SUPPORT
//    case BT_EVENT_SDP_ATTR_INFO:
//        {
//            T_BT_SDP_ATTR_INFO *sdp_info = &param->sdp_attr_info.info;

//            if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_AV_REMOTE_CONTROL_TARGET)
//            {
//                if (app_cmd_get_report_attr_info_flag())
//                {
//                    uint8_t temp_buff[5];

//                    temp_buff[0] = GET_AVRCP_ATTR_INFO;
//                    memcpy(&temp_buff[1], &(sdp_info->profile_version), 2);
//                    memcpy(&temp_buff[3], &(sdp_info->supported_feat), 2);

//                    app_cmd_set_report_attr_info_flag(false);
//                    app_report_event(CMD_PATH_UART, EVENT_REPORT_REMOTE_DEV_ATTR_INFO, 0, temp_buff,
//                                     sizeof(temp_buff));
//                }
//            }
//        }
//        break;
//#endif

    case BT_EVENT_AVRCP_PLAY:
        {
            enum
            {
                STATUS_POS  = 0,
                ADDR_POS    = STATUS_POS + 1,
                END_POS     = ADDR_POS + 6,
            };

            uint8_t temp_buff[END_POS];
            temp_buff[STATUS_POS] = AVRCP_PLAY;
            memcpy(&temp_buff[ADDR_POS], &param->avrcp_play.bd_addr, 6);
            app_report_event(CMD_PATH_UART, EVENT_XM_BT_AVRCP_STATUS, 0, temp_buff, sizeof(temp_buff));
        }
        break;

    case BT_EVENT_AVRCP_PAUSE:
        {
            enum
            {
                STATUS_POS  = 0,
                ADDR_POS    = STATUS_POS + 1,
                END_POS     = ADDR_POS + 6,
            };

            uint8_t temp_buff[END_POS];
            temp_buff[STATUS_POS] = AVRCP_PAUSE;
            memcpy(&temp_buff[ADDR_POS], &param->avrcp_pause.bd_addr, 6);
            app_report_event(CMD_PATH_UART, EVENT_XM_BT_AVRCP_STATUS, 0, temp_buff, sizeof(temp_buff));
        }
        break;

    case BT_EVENT_AVRCP_STOP:
        {
            enum
            {
                STATUS_POS  = 0,
                ADDR_POS    = STATUS_POS + 1,
                END_POS     = ADDR_POS + 6,
            };

            uint8_t temp_buff[END_POS];
            temp_buff[STATUS_POS] = AVRCP_STOP;
            memcpy(&temp_buff[ADDR_POS], &param->avrcp_stop.bd_addr, 6);
            app_report_event(CMD_PATH_UART, EVENT_XM_BT_AVRCP_STATUS, 0, temp_buff, sizeof(temp_buff));
        }
        break;
    case BT_EVENT_AVRCP_FORWARD:
        {
            enum AVRCP_STATUS_PARAMS
            {
                STATUS_POS  = 0,
                ADDR_POS    = STATUS_POS + 1,
                END_POS     = ADDR_POS + 6,
            };

            uint8_t temp_buff[END_POS];
            temp_buff[STATUS_POS] = AVRCP_FORWARD;
            memcpy(&temp_buff[ADDR_POS], &param->avrcp_forward.bd_addr, 6);
            app_report_event(CMD_PATH_UART, EVENT_XM_BT_AVRCP_STATUS, 0, temp_buff, sizeof(temp_buff));
        }
        break;

    case BT_EVENT_AVRCP_BACKWARD:
        {
            enum AVRCP_STATUS_PARAMS
            {
                STATUS_POS  = 0,
                ADDR_POS    = STATUS_POS + 1,
                END_POS     = ADDR_POS + 6,
            };

            uint8_t temp_buff[END_POS];
            temp_buff[STATUS_POS] = AVRCP_BACKWARD;
            memcpy(&temp_buff[ADDR_POS], &param->avrcp_backward.bd_addr, 6);
            app_report_event(CMD_PATH_UART, EVENT_XM_BT_AVRCP_STATUS, 0, temp_buff, sizeof(temp_buff));
        }
        break;
    case BT_EVENT_AVRCP_VOLUME_CHANGED:
        {
            enum AVRCP_STATUS_PARAMS
            {
                STATUS_POS  = 0,
                VOL_POS     = STATUS_POS + 1,
                ADDR_POS    = VOL_POS + 1,
                END_POS     = ADDR_POS + 6,
            };

            uint8_t temp_buff[END_POS];
            temp_buff[STATUS_POS] = AVRCP_VOLUME_CHANGED;
            temp_buff[VOL_POS] = param->avrcp_volume_changed.volume;
            memcpy(&temp_buff[ADDR_POS], &param->avrcp_volume_changed.bd_addr, 6);
            app_report_event(CMD_PATH_UART, EVENT_XM_BT_AVRCP_STATUS, 0, temp_buff, sizeof(temp_buff));
        }
        break;

    case BT_EVENT_AVRCP_VENDOR_CMD_IND:
        {
            p_link = app_link_find_br_link(param->avrcp_vendor_cmd_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_avrcp_vendor_rsp_send(p_link->bd_addr, BT_AVRCP_RESPONSE_NOT_IMPLEMENTED,
                                         param->avrcp_vendor_cmd_ind.company_id, NULL, 0);
            }
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_avrcp_bt_cback: event_type 0x%04x", event_type);
    }
}

bool app_avrcp_abs_vol_check_timer_exist(void)
{
    bool ret = false;

    if (avrcp.chk_abs_vol_tmr_idx)
    {
        ret = true;
    }

    return ret;
}

void app_avrcp_stop_abs_vol_check_timer(void)
{
    app_stop_timer(&avrcp.chk_abs_vol_tmr_idx);
}

bool app_avrcp_sync_abs_vol_state(uint8_t *bd_addr, T_APP_ABS_VOL_STATE abs_vol_state)
{
    uint8_t cmd_ptr[8] = {0};
    uint8_t pair_idx_mapping;
    bool ret = false;

    if (app_bond_get_pair_idx_mapping(bd_addr, &pair_idx_mapping) == false)
    {
        return ret;
    }

    APP_PRINT_INFO3("app_avrcp_sync_abs_vol_state: bd_addr %s, abs_vol_state %d audio_gain_level %d",
                    TRACE_BDADDR(bd_addr),
                    abs_vol_state, app_cfg_nv.audio_gain_level[pair_idx_mapping]);

    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {

    }

    return ret;
}

void app_avrcp_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                          uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    enum {PARAM_START_POS = 2};

    switch (cmd_id)
    {
#if F_APP_AVRCP_CMD_SUPPORT
    case CMD_AVRCP_LIST_SETTING_ATTR:
        {
            enum {LINK_ID_POS = PARAM_START_POS};

            uint8_t app_link_id = cmd_ptr[LINK_ID_POS];

            if (bt_avrcp_app_setting_attrs_list(app_db.br_link[app_link_id].bd_addr) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AVRCP_LIST_SETTING_VALUE:
        {
            enum
            {
                LINK_ID_POS     = PARAM_START_POS,
                STATUS_TYPE_POS = LINK_ID_POS + 1,
            };

            uint8_t app_link_id = cmd_ptr[PARAM_START_POS];

            if (bt_avrcp_app_setting_values_list(app_db.br_link[app_link_id].bd_addr,
                                                 cmd_ptr[STATUS_TYPE_POS]) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AVRCP_GET_CURRENT_VALUE:
        {
            enum
            {
                LINK_ID_POS     = PARAM_START_POS,
                ATTR_NUM_POS    = LINK_ID_POS + 1,
                ATTR_LIST_POS   = ATTR_NUM_POS + 1,
            };

            uint8_t app_link_id = cmd_ptr[LINK_ID_POS];
            uint8_t attr_num = cmd_ptr[ATTR_NUM_POS];
            uint8_t attr_list[attr_num];

            memcpy(attr_list, &cmd_ptr[ATTR_LIST_POS], attr_num);
            if (bt_avrcp_app_setting_value_get(app_db.br_link[app_link_id].bd_addr, attr_num,
                                               attr_list) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AVRCP_SET_VALUE:
        {
            enum
            {
                LINK_ID_POS             = PARAM_START_POS,
                ATTR_NUM_POS            = LINK_ID_POS + 1,
                ATTR_LIST_AND_VAL_POS   = ATTR_NUM_POS + 1,
            };

            uint8_t app_link_id = cmd_ptr[LINK_ID_POS];
            uint8_t attr_num = cmd_ptr[ATTR_NUM_POS];
            uint8_t attr_list[attr_num * 2];

            memcpy(attr_list, &cmd_ptr[ATTR_LIST_AND_VAL_POS], attr_num * 2);
            if (bt_avrcp_app_setting_value_set(app_db.br_link[app_link_id].bd_addr, attr_num,
                                               (T_BT_AVRCP_APP_SETTING *)attr_list) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AVRCP_ABORT_DATA_TRANSFER:
        {
            enum {LINK_ID_POS = PARAM_START_POS};

            uint8_t app_link_id = cmd_ptr[LINK_ID_POS];

            if (bt_avrcp_continuing_rsp_abort(app_db.br_link[app_link_id].bd_addr) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AVRCP_SET_ABSOLUTE_VOLUME:
        {
            enum
            {
                LINK_ID_POS = PARAM_START_POS,
                VOL_POS     = LINK_ID_POS + 1,
            };

            uint8_t app_link_id = cmd_ptr[LINK_ID_POS];
            uint8_t volume = cmd_ptr[VOL_POS];

            if (volume <= 0x7f)// abs volume: 0x00 ~ 0x7f
            {
                if (bt_avrcp_absolute_volume_set(app_db.br_link[app_link_id].bd_addr, volume) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AVRCP_GET_PLAY_STATUS:
        {
            enum {LINK_ID_POS = PARAM_START_POS};

            uint8_t app_link_id = cmd_ptr[LINK_ID_POS];

            if (bt_avrcp_get_play_status_req(app_db.br_link[app_link_id].bd_addr) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AVRCP_GET_ELEMENT_ATTR:
        {
            enum
            {
                LINK_ID_POS     = PARAM_START_POS,
                ATTR_NUM_POS    = LINK_ID_POS + 1,
                ATTR_IDS_POS    = ATTR_NUM_POS + 1,
            };


            uint8_t app_link_id = cmd_ptr[LINK_ID_POS];

            uint8_t attr_list[MAX_NUM_OF_ELEMENT_ATTR] = {1, 2, 3, 4, 5, 6, 7};
            uint8_t number_attr = MAX_NUM_OF_ELEMENT_ATTR;

            if (cmd_ptr[ATTR_NUM_POS] == ALL_ELEMENT_ATTR)
            {
                if (bt_avrcp_get_element_attr_req(app_db.br_link[app_link_id].bd_addr, number_attr,
                                                  attr_list) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else if (cmd_ptr[ATTR_NUM_POS] <= MAX_NUM_OF_ELEMENT_ATTR)
            {
                number_attr = cmd_ptr[ATTR_NUM_POS];
                memcpy(attr_list, &cmd_ptr[ATTR_IDS_POS], number_attr);

                if (bt_avrcp_get_element_attr_req(app_db.br_link[app_link_id].bd_addr, number_attr,
                                                  attr_list) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;
#endif

    default:
        break;
    }
}


void app_avcrp_play_pause(void)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    if ((app_db.br_link[active_a2dp_idx].connected_profile & A2DP_SRC_PROFILE_MASK) &&
        (app_db.br_link[active_a2dp_idx].connected_profile & AVRCP_TARGET_PROFILE_MASK))
    {
        APP_PRINT_TRACE3("app_avcrp_play_pause: active_a2dp_idx %d, avrcp_play_status %d, bud_stream_state %d",
                         active_a2dp_idx, app_db.br_link[active_a2dp_idx].avrcp.play_status,
                         app_audio_get_bud_stream_state());

        if (app_db.br_link[active_a2dp_idx].avrcp.play_status != BT_AVRCP_PLAY_STATUS_PLAYING)
        {
            // Update play status after AVRCP play status event received
            bt_avrcp_play(app_db.br_link[active_a2dp_idx].bd_addr);

            app_audio_tone_type_cancel(TONE_AUDIO_PAUSED, false);
            app_audio_tone_type_play(TONE_AUDIO_PLAYING, true, false);
        }
        else
        {
            bt_avrcp_pause(app_db.br_link[active_a2dp_idx].bd_addr);

            app_audio_tone_type_cancel(TONE_AUDIO_PLAYING, false);
            app_audio_tone_type_play(TONE_AUDIO_PAUSED, true, false);

        }
    }
    else
    {
        uint8_t bd_addr[6];

        if (app_bond_b2s_addr_get(1, bd_addr) == true)
        {
            app_bt_policy_default_connect(bd_addr,
                                          A2DP_SRC_PROFILE_MASK | AVRCP_TARGET_PROFILE_MASK, true);
        }
    }
}

void app_avrcp_stop(void)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    bt_avrcp_stop(app_db.br_link[active_a2dp_idx].bd_addr);
}


void app_avcrp_forward(void)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    if ((app_avrcp_cfg.enable_av_fwd_bwd_only_when_playing == 0) ||
        (app_db.br_link[active_a2dp_idx].avrcp.play_status == BT_AVRCP_PLAY_STATUS_PLAYING))
    {
        bt_avrcp_forward(app_db.br_link[active_a2dp_idx].bd_addr);
        app_audio_tone_type_play(TONE_AUDIO_FORWARD, false, false);
    }
}

void app_avcrp_backward(void)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    if ((app_avrcp_cfg.enable_av_fwd_bwd_only_when_playing == 0) ||
        (app_db.br_link[active_a2dp_idx].avrcp.play_status == BT_AVRCP_PLAY_STATUS_PLAYING))
    {

        bt_avrcp_backward(app_db.br_link[active_a2dp_idx].bd_addr);
        app_audio_tone_type_play(TONE_AUDIO_BACKWARD, false, false);
    }
}


void app_avrcp_fastforward(void)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    bt_avrcp_fast_forward_start(app_db.br_link[active_a2dp_idx].bd_addr);
}


void app_avrcp_rewind(void)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    bt_avrcp_rewind_start(app_db.br_link[active_a2dp_idx].bd_addr);
}


void app_avrcp_fast_forward_stop(void)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    bt_avrcp_fast_forward_stop(app_db.br_link[active_a2dp_idx].bd_addr);
}


void app_avrcp_rewind_stop(void)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    bt_avrcp_rewind_stop(app_db.br_link[active_a2dp_idx].bd_addr);
}

void app_avrcp_init(void)
{
    if (app_cfg_const.supported_profile_mask & AVRCP_PROFILE_MASK)
    {
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
        bt_avrcp_init(BT_AVRCP_FEATURE_CATEGORY_1 | BT_AVRCP_FEATURE_CATEGORY_2,
                      BT_AVRCP_FEATURE_CATEGORY_1);
#else
        bt_avrcp_init(BT_AVRCP_FEATURE_CATEGORY_1,
                      BT_AVRCP_FEATURE_CATEGORY_2);
#endif
        bt_mgr_cback_register(app_avrcp_bt_cback);

        app_timer_reg_cb(app_avrcp_timeout_cb, &avrcp.tmr_id);
    }
}

