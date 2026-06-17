/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_SOURCE_PLAY_SUPPORT

#include "trace.h"
#include "string.h"
#include "app_src_play_hfp.h"
#include "app_dlps.h"
#include "app_link_util_cs.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_hfp.h"
#include "btm.h"
#include "audio_type.h"
#include "app_src_play.h"
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_uac.h"
#endif

#if F_APP_INTEGRATED_TRANSCEIVER
#include "app_src_play_pipe.h"
#include "app_src_playback.h"
#endif

static uint16_t  hfp_hf_seq_num = 0;
static uint16_t  hfp_ag_seq_num = 0;

T_SRC_PLAY_HFP hfp_play;

void app_src_play_hfp_send_sco(uint8_t *p_data, uint16_t len, uint8_t bd_addr[6])
{
    uint8_t active_hf_idx = app_hfp_get_active_idx();
    uint16_t hfp_seq_num;
    T_APP_BR_LINK *p_link;
    bool sco_tx_result = false;

#if F_APP_INTEGRATED_TRANSCEIVER
    p_link = app_link_find_br_link(bd_addr);
#else
    p_link = app_link_find_br_link(app_db.br_link[active_hf_idx].bd_addr);
#endif

    if (p_link == NULL)
    {
        APP_PRINT_ERROR0("app_src_play_hfp_send_sco: no br link found");
        return;
    }

#if F_APP_INTEGRATED_TRANSCEIVER
    if (!memcmp(hfp_play.ag_addr, bd_addr, 6))
    {
        hfp_ag_seq_num++;
        hfp_seq_num = hfp_ag_seq_num;
        memcpy(bd_addr, hfp_play.hf_addr, 6);
    }
    else
    {
        hfp_hf_seq_num++;
        hfp_seq_num = hfp_hf_seq_num;
        memcpy(bd_addr, hfp_play.ag_addr, 6);
    }
#else
    hfp_hf_seq_num++;
    hfp_seq_num = hfp_hf_seq_num;
    memcpy(bd_addr, p_link->bd_addr, 6);
#endif

    if (p_link->sco.duplicate_fst_data)
    {
        p_link->sco.duplicate_fst_data = false;
        bt_sco_data_send(bd_addr, hfp_seq_num - 1, p_data, len);
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
        APP_PRINT_ERROR0("app_src_play_hfp_send_sco: duplicate_fst_data");
#endif
    }

    sco_tx_result = bt_sco_data_send(bd_addr, hfp_seq_num, p_data, len);
    if (sco_tx_result == false)
    {
        APP_PRINT_ERROR0("app_src_play_hfp_send_sco: bt_sco_data_send fail");
    }
}

bool app_src_play_hfp_start_req(void)
{
#if F_APP_AUTO_POWER_TEST_LOG
    TEST_PRINT_INFO1("app_src_play_hfp_start_req: hfp_hf_ready %d", hfp_play.hfp_hf_format_ready);
#else
    APP_PRINT_INFO1("app_src_play_hfp_start_req: hfp_hf_ready %d", hfp_play.hfp_hf_format_ready);
#endif

    return bt_hfp_ag_audio_connect_req(hfp_play.hf_addr);
}

bool app_src_play_hfp_stop(void)
{
#if F_APP_AUTO_POWER_TEST_LOG
    TEST_PRINT_TRACE1("app_src_play_hfp_stop: hfp_hf_ready %d", hfp_play.hfp_hf_format_ready);
#else
    APP_PRINT_TRACE1("app_src_play_hfp_stop: hfp_hf_ready %d", hfp_play.hfp_hf_format_ready);
#endif
    return bt_hfp_ag_audio_disconnect_req(hfp_play.hf_addr);
}

static void app_src_play_hfp_handle_sco_ind(T_BT_EVENT_PARAM_SCO_DATA_IND param)
{
    if (param.p_data == NULL)
    {
        return;
    }
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    if (!app_tri_dongle_uac_upstream_open())
    {
        return;
    }
#endif

    app_src_play_bt_recv(param.p_data, param.length, param.status, param.bt_clk);
}

static void app_src_play_hfp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;
    bool is_hfp_ag_data = false;

    switch (event_type)
    {
    case BT_EVENT_HFP_AG_CONN_CMPL:
        {
            hfp_play.hfp_hf_format_ready = false;
            memcpy(hfp_play.hf_addr, param->hfp_ag_conn_cmpl.bd_addr, 6);
        }
        break;

#if F_APP_INTEGRATED_TRANSCEIVER
    case BT_EVENT_HFP_CONN_CMPL:
        {
            hfp_play.hfp_ag_format_ready = false;
            memcpy(hfp_play.ag_addr, param->hfp_conn_cmpl.bd_addr, 6);
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            if (!memcmp(hfp_play.ag_addr, param->sco_conn_cmpl.bd_addr, 6))
            {
                bt_sco_link_retrans_window_set(hfp_play.ag_addr, 1);
            }
            else if (!memcmp(hfp_play.hf_addr, param->sco_conn_cmpl.bd_addr, 6))
            {
                bt_sco_link_retrans_window_set(hfp_play.hf_addr, 1);
            }
        }
        break;

#endif

    case BT_EVENT_SCO_DATA_IND:
        {
            T_SOURCE_ROUTE src_route = app_src_play_get_src_route();
            if (src_route == SOURCE_ROUTE_USB ||
                src_route == SOURCE_ROUTE_SPDIF ||
                src_route == SOURCE_ROUTE_MIC ||
                src_route == SOURCE_ROUTE_I2S)
            {
                app_src_play_hfp_handle_sco_ind(param->sco_data_ind);
            }

#if F_APP_INTEGRATED_TRANSCEIVER
            if (param->sco_data_ind.status == 0)
            {
                if (!memcmp(hfp_play.ag_addr, param->sco_data_ind.bd_addr, 6))
                {
                    is_hfp_ag_data = true;
                }
                else if (!memcmp(hfp_play.hf_addr, param->sco_data_ind.bd_addr, 6))
                {
                    is_hfp_ag_data = false;
                }
                app_src_play_pipe_handle_sco_stream_data(param->sco_data_ind.p_data, param->sco_data_ind.length,
                                                         param->sco_data_ind.bd_addr, is_hfp_ag_data);
            }
#endif

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
        APP_PRINT_INFO1("app_src_play_hfp_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_src_play_print_hfp_format(const char *title, T_AUDIO_FORMAT_INFO *format_info)
{
    APP_PRINT_TRACE1("@@@@@%s@@@@@", TRACE_STRING(title));
    switch (format_info->type)
    {
    case AUDIO_FORMAT_TYPE_CVSD:
        {
            APP_PRINT_INFO3("CVSD, "
                            "sample_rate %d, chann_num %d, frame_duration %d",
                            format_info->attr.cvsd.sample_rate,
                            format_info->attr.cvsd.chann_num,
                            format_info->attr.cvsd.frame_duration);
        }
        break;

    case AUDIO_FORMAT_TYPE_MSBC:
        {
            APP_PRINT_INFO6("MSBC, "
                            "sample_rate %d, allocation_method %d, bitpool %d, block_length %d, chann_mode %d, subband_num %d",
                            format_info->attr.msbc.sample_rate,
                            format_info->attr.msbc.allocation_method,
                            format_info->attr.msbc.bitpool,
                            format_info->attr.msbc.block_length,
                            format_info->attr.msbc.chann_mode,
                            format_info->attr.msbc.subband_num);
        }
        break;

    default:
        break;
    }
}

void app_src_play_save_hfp_format(T_AUDIO_FORMAT_INFO *format_info, uint8_t *bd_addr)
{
    if ((!hfp_play.hfp_hf_format_ready) && (!memcmp(hfp_play.hf_addr, bd_addr, 6)))
    {
        memcpy(&hfp_play.hfp_hf_format, format_info, sizeof(T_AUDIO_FORMAT_INFO));
        hfp_play.hfp_hf_format_ready = true;
        app_src_play_print_hfp_format("app_src_play_save_hfp_hf_format: ",
                                      &hfp_play.hfp_hf_format);
    }

#if F_APP_INTEGRATED_TRANSCEIVER
    else if ((!hfp_play.hfp_ag_format_ready) && (!memcmp(hfp_play.ag_addr, bd_addr, 6)))
    {
        memcpy(&hfp_play.hfp_ag_format, format_info, sizeof(T_AUDIO_FORMAT_INFO));
        hfp_play.hfp_ag_format_ready = true;
        app_src_play_print_hfp_format("app_src_play_save_hfp_ag_format: ",
                                      &hfp_play.hfp_ag_format);
    }
#endif

}

bool app_src_play_get_hfp_format(T_AUDIO_FORMAT_INFO *format_info)
{
    if (hfp_play.hfp_hf_format_ready)
    {
        memcpy(format_info, &hfp_play.hfp_hf_format, sizeof(T_AUDIO_FORMAT_INFO));
        app_src_play_print_hfp_format("app_src_play_get_hfp_hf_format", format_info);
        return true;
    }
    return false;
}

void app_src_play_hfp_init(void)
{
    if (app_cfg_const.supported_profile_mask & HFP_PROFILE_MASK)
    {
        bt_mgr_cback_register(app_src_play_hfp_bt_cback);
    }
}

#endif
