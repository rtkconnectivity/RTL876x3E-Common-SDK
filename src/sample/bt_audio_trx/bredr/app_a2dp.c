/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "btm.h"
#include "bt_a2dp.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_multilink.h"

#include "app_link_util_cs.h"
#include "app_a2dp_cfg.h"
#include "app_a2dp.h"

#include "app_sniff_mode_cs.h"
#include "app_hfp.h"
#include "app_audio_cfg.h"
#include "app_audio_policy.h"

static uint8_t active_a2dp_idx = 0;

static void app_a2dp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_active_a2dp_link = &app_db.br_link[active_a2dp_idx];
    bool handle = true;
    T_APP_BR_LINK *p_link = NULL;

    switch (event_type)
    {
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE != 1
    case BT_EVENT_A2DP_CONN_IND:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->a2dp_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_a2dp_connect_cfm(p_link->bd_addr, 0, true);
            }
        }
        break;
#endif

    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->a2dp_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                if (param->a2dp_config_cmpl.role == BT_A2DP_ROLE_SNK)
                {
                    bt_a2dp_stream_delay_report_req(param->a2dp_config_cmpl.bd_addr, A2DP_LATENCY_MS);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_SNIFFING_CONN_CMPL:
        {
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {

        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            if ((p_active_a2dp_link->a2dp.is_streaming == false ||
                 p_active_a2dp_link->avrcp.play_status == BT_AVRCP_PLAY_STATUS_PAUSED) ||
                (memcmp(p_active_a2dp_link->bd_addr,
                        param->a2dp_stream_start_ind.bd_addr, 6) == 0))
            {
                APP_PRINT_INFO3("app_a2dp_bt_cback: BT_EVENT_A2DP_STREAM_START_IND active_a2dp_idx %d, a2dp.is_streaming %d, avrcp_play_status %d",
                                active_a2dp_idx, p_active_a2dp_link->a2dp.is_streaming, p_active_a2dp_link->avrcp.play_status);
                app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_A2DP);

                app_audio_set_bud_stream_state(BUD_STREAM_STATE_AUDIO);

                if ((memcmp(p_active_a2dp_link->bd_addr, param->a2dp_stream_start_ind.bd_addr, 6) == 0))
                {
                    bt_a2dp_stream_start_cfm(p_active_a2dp_link->bd_addr, true);
                    p_active_a2dp_link->a2dp.is_streaming = true;
                    app_judge_active_a2dp_idx_and_qos(p_active_a2dp_link->id, JUDGE_EVENT_A2DP_START);
                }
                else
                {
                    bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            if (p_active_a2dp_link->a2dp.is_streaming == false ||
                (memcmp(p_active_a2dp_link->bd_addr,
                        param->a2dp_stream_start_rsp.bd_addr, 6) == 0))
            {
                APP_PRINT_INFO2("app_a2dp_bt_cback: BT_EVENT_A2DP_STREAM_START_RSP active_a2dp_idx %d, a2dp.is_streaming %d",
                                active_a2dp_idx, p_active_a2dp_link->a2dp.is_streaming);

                app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_A2DP);

                app_audio_set_bud_stream_state(BUD_STREAM_STATE_AUDIO);
                p_link = app_link_find_br_link(param->a2dp_stream_start_rsp.bd_addr);
                if (p_link != NULL)
                {
                    p_link->a2dp.is_streaming = true;
                    app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_START);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            if (memcmp(p_active_a2dp_link->bd_addr,
                       param->a2dp_stream_stop.bd_addr, 6) == 0)
            {
                if (app_link_get_a2dp_start_num() <= 1)
                {
                    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_A2DP);
                }

                if (app_hfp_get_call_status() == APP_HFP_CALL_IDLE)
                {
                    app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
                }

            }
            p_link = app_link_find_br_link(param->a2dp_stream_stop.bd_addr);
            if (p_link != NULL)
            {
                p_link->a2dp.is_streaming = false;
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_STOP);
            }


        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            if (memcmp(p_active_a2dp_link->bd_addr,
                       param->a2dp_stream_close.bd_addr, 6) == 0)
            {
                if (app_hfp_get_call_status() == APP_HFP_CALL_IDLE)
                {
                    app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
                }
            }
            p_link = app_link_find_br_link(param->a2dp_stream_close.bd_addr);
            if (p_link != NULL)
            {
                p_link->a2dp.is_streaming = false;
                app_judge_active_a2dp_idx_and_qos(p_link->id, JUDGE_EVENT_A2DP_STOP);
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
        APP_PRINT_INFO1("app_a2dp_bt_cback: event_type 0x%04x", event_type);
    }
}

bool app_a2dp_is_streaming(void)
{
    T_APP_BR_LINK *p_link = NULL;

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        p_link = &app_db.br_link[i];

        if (p_link->a2dp.is_streaming == true)
        {
            return true;
        }
    }

    return false;
}

void app_a2dp_set_active_idx(uint8_t idx)
{
    active_a2dp_idx = idx;
    APP_PRINT_TRACE1("app_a2dp_set_active_idx to %d", active_a2dp_idx);
}

uint8_t app_a2dp_get_active_idx(void)
{
#if F_APP_MULTILINK_ENABLE
    return active_a2dp_idx;
#else
    return 0;
#endif
}

bool app_a2dp_get_inactive_idx(uint8_t *inactive_a2dp_idx)
{
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_link_check_b2s_link_by_id(i))
        {
            if (i != active_a2dp_idx)
            {
                *inactive_a2dp_idx = i;
                return true;
            }
        }
    }
    return false;
}

void app_a2dp_init(void)
{
    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        bt_a2dp_init(BT_A2DP_CAPABILITY_MEDIA_TRANSPORT | BT_A2DP_CAPABILITY_MEDIA_CODEC |
                     BT_A2DP_CAPABILITY_DELAY_REPORTING);

        if (app_a2dp_cfg.a2dp_codec_type_sbc)
        {

#if F_APP_A2DP_SOURCE_SUPPORT
            T_BT_A2DP_STREAM_ENDPOINT sep_src;

            sep_src.role = BT_A2DP_ROLE_SRC;
            sep_src.codec_type = BT_A2DP_CODEC_TYPE_SBC;
            sep_src.u.codec_sbc.sampling_frequency_mask = app_a2dp_cfg.sbc_sampling_frequency;
            sep_src.u.codec_sbc.channel_mode_mask = app_a2dp_cfg.sbc_channel_mode;
            sep_src.u.codec_sbc.block_length_mask = app_a2dp_cfg.sbc_block_length;
            sep_src.u.codec_sbc.subbands_mask = app_a2dp_cfg.sbc_subbands;
            sep_src.u.codec_sbc.allocation_method_mask = app_a2dp_cfg.sbc_allocation_method;
            sep_src.u.codec_sbc.min_bitpool = app_a2dp_cfg.sbc_min_bitpool;
            sep_src.u.codec_sbc.max_bitpool = app_a2dp_cfg.sbc_max_bitpool;
            bt_a2dp_stream_endpoint_add(sep_src);
#endif

#if F_APP_A2DP_SINK_SUPPORT
            T_BT_A2DP_STREAM_ENDPOINT sep_snk;

            sep_snk.role = BT_A2DP_ROLE_SNK;
            sep_snk.codec_type = BT_A2DP_CODEC_TYPE_SBC;
            sep_snk.u.codec_sbc.sampling_frequency_mask = app_a2dp_cfg.sbc_sampling_frequency;
            sep_snk.u.codec_sbc.channel_mode_mask = app_a2dp_cfg.sbc_channel_mode;
            sep_snk.u.codec_sbc.block_length_mask = app_a2dp_cfg.sbc_block_length;
            sep_snk.u.codec_sbc.subbands_mask = app_a2dp_cfg.sbc_subbands;
            sep_snk.u.codec_sbc.allocation_method_mask = app_a2dp_cfg.sbc_allocation_method;
            sep_snk.u.codec_sbc.min_bitpool = app_a2dp_cfg.sbc_min_bitpool;
            sep_snk.u.codec_sbc.max_bitpool = app_a2dp_cfg.sbc_max_bitpool;
            bt_a2dp_stream_endpoint_add(sep_snk);
#endif

        }

        if (app_a2dp_cfg.a2dp_codec_type_aac)
        {

#if F_APP_A2DP_SOURCE_SUPPORT
            T_BT_A2DP_STREAM_ENDPOINT sep_src;

            sep_src.role = BT_A2DP_ROLE_SRC;
            sep_src.codec_type = BT_A2DP_CODEC_TYPE_AAC;
            sep_src.u.codec_aac.object_type_mask = app_a2dp_cfg.aac_object_type;
            sep_src.u.codec_aac.sampling_frequency_mask = app_a2dp_cfg.aac_sampling_frequency;
            sep_src.u.codec_aac.channel_number_mask = app_a2dp_cfg.aac_channel_number;
            sep_src.u.codec_aac.vbr_supported = app_a2dp_cfg.aac_vbr_supported;
            sep_src.u.codec_aac.bit_rate = app_a2dp_cfg.aac_bit_rate;
            bt_a2dp_stream_endpoint_add(sep_src);
#endif

#if F_APP_A2DP_SINK_SUPPORT
            T_BT_A2DP_STREAM_ENDPOINT sep_snk;

            sep_snk.role = BT_A2DP_ROLE_SNK;
            sep_snk.codec_type = BT_A2DP_CODEC_TYPE_AAC;
            sep_snk.u.codec_aac.object_type_mask = app_a2dp_cfg.aac_object_type;
            sep_snk.u.codec_aac.sampling_frequency_mask = app_a2dp_cfg.aac_sampling_frequency;
            sep_snk.u.codec_aac.channel_number_mask = app_a2dp_cfg.aac_channel_number;
            sep_snk.u.codec_aac.vbr_supported = app_a2dp_cfg.aac_vbr_supported;
            sep_snk.u.codec_aac.bit_rate = app_a2dp_cfg.aac_bit_rate;
            bt_a2dp_stream_endpoint_add(sep_snk);
#endif

        }

#if F_APP_A2DP_CODEC_LDAC_SUPPORT
        if (app_a2dp_cfg.a2dp_codec_type_ldac)
        {
            T_BT_A2DP_STREAM_ENDPOINT sep;

#if F_APP_A2DP_SOURCE_SUPPORT
            T_BT_A2DP_STREAM_ENDPOINT sep_src;

            sep_src.role = BT_A2DP_ROLE_SRC;
            sep_src.codec_type = BT_A2DP_CODEC_TYPE_LDAC;
            sep_src.u.codec_ldac.sampling_frequency_mask = app_a2dp_cfg.ldac_sampling_frequency;
            sep_src.u.codec_ldac.channel_mode_mask = app_a2dp_cfg.ldac_channel_mode;
            bt_a2dp_stream_endpoint_add(sep_src);
#endif

#if F_APP_A2DP_SINK_SUPPORT
            T_BT_A2DP_STREAM_ENDPOINT sep_snk;

            sep_snk.role = BT_A2DP_ROLE_SNK;
            sep_snk.codec_type = BT_A2DP_CODEC_TYPE_LDAC;
            sep_snk.u.codec_ldac.sampling_frequency_mask = app_a2dp_cfg.ldac_sampling_frequency;
            sep_snk.u.codec_ldac.channel_mode_mask = app_a2dp_cfg.ldac_channel_mode;
            bt_a2dp_stream_endpoint_add(sep_snk);
#endif

        }
#endif

#if F_APP_A2DP_CODEC_LC3_SUPPORT
        if (app_a2dp_cfg.a2dp_codec_type_lc3)
        {
#if F_APP_A2DP_SOURCE_SUPPORT
            T_BT_A2DP_STREAM_ENDPOINT sep_src;

            sep_src.role = BT_A2DP_ROLE_SRC;
            sep_src.codec_type = BT_A2DP_CODEC_TYPE_LC3;
            sep_src.u.codec_lc3.sampling_frequency_mask = app_a2dp_cfg.lc3_sampling_frequency;
            sep_src.u.codec_lc3.channel_num_mask = app_a2dp_cfg.lc3_channel_num;
            sep_src.u.codec_lc3.frame_duration_mask = app_a2dp_cfg.lc3_frame_duration;
            sep_src.u.codec_lc3.frame_length = app_a2dp_cfg.lc3_frame_length;
            bt_a2dp_stream_endpoint_add(sep_src);
#endif

#if F_APP_A2DP_SINK_SUPPORT
            T_BT_A2DP_STREAM_ENDPOINT sep_snk;
            sep_snk.codec_type = BT_A2DP_CODEC_TYPE_LC3;
            sep_snk.u.codec_lc3.sampling_frequency_mask = app_a2dp_cfg.lc3_sampling_frequency;
            sep_snk.u.codec_lc3.channel_num_mask = app_a2dp_cfg.lc3_channel_num;
            sep_snk.u.codec_lc3.frame_duration_mask = app_a2dp_cfg.lc3_frame_duration;
            sep_snk.u.codec_lc3.frame_length = app_a2dp_cfg.lc3_frame_length;
            bt_a2dp_stream_endpoint_add(sep_snk);
#endif

        }
#endif

        bt_mgr_cback_register(app_a2dp_bt_cback);
    }
}
