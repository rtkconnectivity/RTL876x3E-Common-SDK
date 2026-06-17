/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_SOURCE_PLAY_SUPPORT

#include <stdlib.h>
#include "trace.h"
#include "string.h"
#include "app_src_play.h"
#include "app_src_play_a2dp.h"
#include "app_dlps.h"
#include "app_link_util_cs.h"
#include "app_cfg.h"
#include "app_bt_policy_cfg.h"
#include "ring_buffer.h"
#include "btm.h"
#include "audio_type.h"
#include "bt_a2dp.h"
#include "bt_bond.h"
#include "app_timer.h"
#include "app_audio_cfg.h"
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_uac.h"
#include "app_tri_dongle_mgr.h"
#include "section.h"
#endif
#define A2DP_FRAME_BUF              1024

#if F_APP_INTEGRATED_TRANSCEIVER
#define A2DP_PACKET_FRAME_NUM       6
#else
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#define A2DP_PACKET_FRAME_NUM       8
#else
#define A2DP_PACKET_FRAME_NUM       4
#endif
#endif

#define A2DP_MAX_CREDITS            8

static uint8_t   a2dp_credits = 8;

static uint8_t   cur_pair_idx = 0;
static uint16_t  a2dp_seq_num = 0;
static uint32_t  a2dp_timestamp = 0;
static uint8_t  stream_idx = 0;
static uint8_t  app_src_play_a2dp_timer_id = 0;          // app timer
static uint8_t  timer_idx_src_play_a2dp_open = 0;
static uint8_t  src_play_a2dp_temp_addr[6];

#if F_APP_INTEGRATED_TRANSCEIVER
static bool  src_addr_ready = false;
#endif

typedef enum
{
    APP_TIMER_SRC_PLAY_A2DP_OPEN_STREAM,
} T_SRC_PLAY_A2DP_TIMER;                             // app timer

T_SRC_PLAY_A2DP a2dp_play;

uint8_t app_src_play_a2dp_get_connected_idx(uint8_t *bd_addr)
{
    uint8_t get_a2dp_idx = 0;
    uint8_t i;
    if (bd_addr != NULL)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (!memcmp(a2dp_play.sink_addr[i], bd_addr, 6))
            {
                get_a2dp_idx = i;
                break;
            }
        }
    }
    return get_a2dp_idx;
}

bool app_src_play_a2dp_get_idle_idx(uint8_t *idle_idx)
{
    uint8_t i;
    static uint8_t bd_addr[6] = {0};
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (!a2dp_play.sink_a2dp_param[i].a2dp_is_used)
        {
            *idle_idx = i;
            return true;
        }
    }
    return false;
}

static uint16_t app_src_play_a2dp_send_data(uint8_t *p_data, uint16_t data_len,
                                            uint8_t frame_number)
{
    uint16_t res = SRC_PLAY_A2DP_SUCCESS;
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    if ((app_tri_dongle_uac_no_ds_get_status()) ||
        (app_src_play_get_a2dp_state(0) != A2DP_STATE_STREAM_START_RSP))
    {
        APP_PRINT_INFO2("app_src_play_a2dp_send_data skip %d,%d", app_tri_dongle_uac_no_ds_get_status(),
                        app_src_play_get_a2dp_state(0));
    }
    else
#endif
    {
        if (data_len == 0)
        {
            res = SRC_PLAY_A2DP_ERR_DATA_LEN;
        }
        else
        {
            if (a2dp_credits)
            {
                if (bt_a2dp_stream_data_send(a2dp_play.sink_addr[0],
                                             a2dp_seq_num,
                                             a2dp_timestamp,
                                             frame_number,
                                             p_data,
                                             data_len))
                {
                    a2dp_seq_num++;
                    a2dp_credits--;
                }
                else
                {
                    res = SRC_PLAY_A2DP_ERR_LOWERSTACK;
                }

                a2dp_timestamp += frame_number * a2dp_play.sink_a2dp_format[0].attr.sbc.block_length *
                                  a2dp_play.sink_a2dp_format[0].attr.sbc.subband_num;
            }
            else
            {
                res = SRC_PLAY_A2DP_CREDIT_LOW;
            }
        }
    }

    if (res != SRC_PLAY_A2DP_SUCCESS)
    {
        APP_PRINT_INFO1("app_src_play_a2dp_send_data: res 0x%x", res);
    }
    return res;
}

uint16_t app_src_play_a2dp_handle_data(uint8_t *p_data, uint16_t data_len, uint8_t frame_number)
{
    uint16_t res = SRC_PLAY_A2DP_SUCCESS;
    if (frame_number > 1)
    {
        res = app_src_play_a2dp_send_data(p_data, data_len, frame_number);
        return res;
    }

    if (ring_buffer_write(&a2dp_play.ring_buf, p_data, data_len))
    {
        a2dp_play.num_frame_buf += frame_number;
    }
    else
    {
        res = SRC_PLAY_A2DP_ERR_RINGBUF;
        APP_PRINT_ERROR0("app_src_play_a2dp_handle_data: a2dp_play.ring_buf is full, drop pkt");
    }

    if (a2dp_play.num_frame_buf == A2DP_PACKET_FRAME_NUM)
    {
        a2dp_play.num_frame_buf -= A2DP_PACKET_FRAME_NUM;
        uint16_t data_len_to_send = data_len * A2DP_PACKET_FRAME_NUM;
        uint8_t *p_data_to_send = malloc(data_len_to_send);
        if (p_data_to_send)
        {
            uint32_t actual_len = ring_buffer_read(&a2dp_play.ring_buf, data_len_to_send, p_data_to_send);
            APP_PRINT_INFO1("app_src_play_a2dp_handle_data: actual_len %d sent", actual_len);

            res = app_src_play_a2dp_send_data(p_data_to_send, data_len_to_send, A2DP_PACKET_FRAME_NUM);
#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
            if (app_src_play_is_local_play_attached())
            {
                app_src_play_attach_local_play_handle_data(p_data_to_send,
                                                           data_len_to_send,
                                                           a2dp_seq_num,
                                                           A2DP_PACKET_FRAME_NUM,
                                                           0);
            }
#endif
            free(p_data_to_send);
        }
        else
        {
            res = SRC_PLAY_A2DP_ERR_RAM;
        }
    }
    return res;
}

static uint16_t app_src_play_multi_a2dp_send_data(uint8_t *p_data, uint16_t data_len,
                                                  uint8_t idx,
                                                  uint8_t frame_number)
{
    uint16_t res = SRC_PLAY_A2DP_SUCCESS;

    if (data_len == 0)
    {
        res = SRC_PLAY_A2DP_ERR_DATA_LEN;
    }
    else
    {
        if (a2dp_play.sink_a2dp_param[idx].multi_a2dp_credits)
        {
            if (bt_a2dp_stream_data_send(a2dp_play.sink_addr[idx],
                                         a2dp_play.sink_a2dp_param[idx].multi_a2dp_seq_num,
                                         a2dp_play.sink_a2dp_param[idx].multi_a2dp_timestamp,
                                         frame_number,
                                         p_data,
                                         data_len))
            {
                a2dp_play.sink_a2dp_param[idx].multi_a2dp_seq_num++;
                a2dp_play.sink_a2dp_param[idx].multi_a2dp_credits--;
            }
            else
            {
                res = SRC_PLAY_A2DP_ERR_LOWERSTACK;
            }
            a2dp_play.sink_a2dp_param[idx].multi_a2dp_timestamp += frame_number *
                                                                   a2dp_play.sink_a2dp_format[idx].attr.sbc.block_length *
                                                                   a2dp_play.sink_a2dp_format[idx].attr.sbc.subband_num;
        }
        else
        {
            res = SRC_PLAY_A2DP_CREDIT_LOW;
        }
    }
    APP_PRINT_INFO1("app_src_play_multi_a2dp_send_data: res 0x%x", res);
    return res;
}

uint16_t app_src_play_multi_a2dp_handle_data(uint8_t *p_data, uint16_t data_len, uint8_t active_idx,
                                             uint8_t frame_number)
{
    uint16_t res = SRC_PLAY_A2DP_SUCCESS;
    uint8_t cur_idx;

    if (frame_number > 1)
    {

#if F_APP_MULTI_CHANNEL_SUPPORT
        if (a2dp_play.sink_a2dp_param[active_idx].a2dp_is_used)
        {
            res = app_src_play_multi_a2dp_send_data(p_data, data_len, active_idx, frame_number);
        }
#else
        for (cur_idx = 0; cur_idx < MAX_BR_LINK_NUM; cur_idx++)
        {
            if (a2dp_play.sink_a2dp_param[cur_idx].a2dp_is_used)
            {
                res &= app_src_play_multi_a2dp_send_data(p_data, data_len, cur_idx, frame_number);
            }
        }
#endif

        return res;
    }

    if (ring_buffer_write(&a2dp_play.ring_buf, p_data, data_len))
    {
        a2dp_play.num_frame_buf += frame_number;
    }
    else
    {
        res = SRC_PLAY_A2DP_ERR_RINGBUF;
        APP_PRINT_ERROR0("app_src_play_multi_a2dp_handle_data: a2dp_play.ring_buf is full, drop pkt");
    }

    if (a2dp_play.num_frame_buf == A2DP_PACKET_FRAME_NUM)
    {
        a2dp_play.num_frame_buf -= A2DP_PACKET_FRAME_NUM;
        uint16_t data_len_to_send = data_len * A2DP_PACKET_FRAME_NUM;
        uint8_t *p_data_to_send = malloc(data_len_to_send);
        if (p_data_to_send)
        {
            uint32_t actual_len = ring_buffer_read(&a2dp_play.ring_buf, data_len_to_send, p_data_to_send);
            APP_PRINT_INFO1("app_src_play_multi_a2dp_handle_data: actual_len %d sent", actual_len);

#if F_APP_MULTI_CHANNEL_SUPPORT
            if (a2dp_play.sink_a2dp_param[active_idx].a2dp_is_used)
            {
                res = app_src_play_multi_a2dp_send_data(p_data, data_len, active_idx, A2DP_PACKET_FRAME_NUM);
            }
#else
            for (cur_idx = 0; cur_idx < MAX_BR_LINK_NUM; cur_idx++)
            {
                if (a2dp_play.sink_a2dp_param[cur_idx].a2dp_is_used)
                {
                    res &= app_src_play_multi_a2dp_send_data(p_data_to_send, data_len_to_send, cur_idx,
                                                             A2DP_PACKET_FRAME_NUM);
                }
            }
#endif

            free(p_data_to_send);
        }
        else
        {
            res = SRC_PLAY_A2DP_ERR_RAM;
        }
    }
    return res;
}

bool app_src_play_a2dp_start_req(void)
{
    uint8_t i;
    bool result = true;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (a2dp_play.sink_a2dp_param[i].a2dp_is_used)
        {
#if F_APP_AUTO_POWER_TEST_LOG
            TEST_PRINT_INFO2("app_src_play_a2dp_start_req: link_idx %d, sink_a2dp_state %d", i,
                             a2dp_play.sink_a2dp_state[i]);
#else
            APP_PRINT_INFO2("app_src_play_a2dp_start_req: link_idx %d, sink_a2dp_state %d", i,
                            a2dp_play.sink_a2dp_state[i]);
#endif
            if (app_bt_policy_cfg_nv.enable_multi_link)
            {
                result &= bt_a2dp_stream_start_req(a2dp_play.sink_addr[i]);
            }
            else
            {
                result = bt_a2dp_stream_start_req(a2dp_play.sink_addr[0]);
                break;
            }
        }
    }
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_tri_dongle_set_ds_only_update(false);
#endif
    return result;
}

bool app_src_play_a2dp_stop(void)
{
    uint8_t i;
    bool result = true;

    a2dp_play.num_frame_buf = 0;
    ring_buffer_clear(&a2dp_play.ring_buf);
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (a2dp_play.sink_a2dp_param[i].a2dp_is_used)
        {
#if F_APP_AUTO_POWER_TEST_LOG
            TEST_PRINT_INFO2("app_src_play_a2dp_stop: link_idx %d, sink_a2dp_state %d", i,
                             a2dp_play.sink_a2dp_state[i]);
#else
            APP_PRINT_INFO2("app_src_play_a2dp_stop: link_idx %d, sink_a2dp_state %d", i,
                            a2dp_play.sink_a2dp_state[i]);
#endif
            if (a2dp_play.sink_a2dp_state[i] != A2DP_STATE_STREAM_STOP)
            {
                result &= bt_a2dp_stream_suspend_req(a2dp_play.sink_addr[i]);
            }
        }
    }
    app_dlps_enable(APP_DLPS_ENTER_CHECK_PLAYBACK);

    return result;
}
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
RAM_TEXT_SECTION T_SRC_PLAY_A2DP_STATE app_src_play_get_a2dp_state(uint8_t link_idx)
#else
T_SRC_PLAY_A2DP_STATE app_src_play_get_a2dp_state(uint8_t link_idx)
#endif
{
    return a2dp_play.sink_a2dp_state[link_idx];
}

static void app_src_play_a2dp_start_rsp_handler(void)
{
    app_dlps_disable(APP_DLPS_ENTER_CHECK_PLAYBACK);
}

static void app_src_play_a2dp_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_src_play_a2dp_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_SRC_PLAY_A2DP_OPEN_STREAM:
        {
            bt_a2dp_stream_open_req(src_play_a2dp_temp_addr, BT_A2DP_ROLE_SNK);
        }
        break;

    default:
        break;
    }
}

static void app_src_play_a2dp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;
    uint8_t curr_idx;
    uint8_t conn_idx;

    switch (event_type)
    {
    case BT_EVENT_A2DP_CONN_CMPL:
        {

#if F_APP_INTEGRATED_TRANSCEIVER
            if (src_addr_ready && memcmp(a2dp_play.src_addr, param->a2dp_conn_cmpl.bd_addr, 6))
            {
#else
            {
#endif

                memcpy(&src_play_a2dp_temp_addr[0], param->a2dp_conn_cmpl.bd_addr, 6);
                app_start_timer(&timer_idx_src_play_a2dp_open, "a2dp_open_stream",
                                app_src_play_a2dp_timer_id, APP_TIMER_SRC_PLAY_A2DP_OPEN_STREAM, 0, false,
                                1 * 1000);
            }

#if F_APP_A2DP_MULTI_SINK_SUPPORT
            bool ret = app_src_play_a2dp_get_idle_idx(&conn_idx);
            if (ret)
            {
                a2dp_play.sink_a2dp_state[conn_idx] = A2DP_STATE_CONN;
                memcpy(&a2dp_play.sink_addr[conn_idx][0], param->a2dp_conn_cmpl.bd_addr, 6);
                a2dp_play.sink_a2dp_param[conn_idx].a2dp_is_used = 1;
            }
#endif

        }
        break;

#if !F_APP_A2DP_MULTI_SINK_SUPPORT
    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
            if (param->a2dp_config_cmpl.role == BT_A2DP_ROLE_SRC)
            {
                a2dp_play.sink_a2dp_state[0] = A2DP_STATE_CONN;
                memcpy(&a2dp_play.sink_addr[0][0], param->a2dp_config_cmpl.bd_addr, 6);
                a2dp_play.sink_a2dp_param[0].a2dp_is_used = 1;

#if F_APP_INTEGRATED_TRANSCEIVER
                bt_acl_pkt_type_set(param->a2dp_config_cmpl.bd_addr, BT_ACL_PKT_TYPE_3M);
                bt_link_preferred_data_rate_set(param->a2dp_config_cmpl.bd_addr, BT_LINK_1M5S, BT_LINK_3M3S);
#endif

            }
            else if (param->a2dp_config_cmpl.role == BT_A2DP_ROLE_SNK)
            {
                bt_a2dp_stream_delay_report_req(param->a2dp_config_cmpl.bd_addr, A2DP_LATENCY_MS);

                memcpy(&a2dp_play.src_addr[0], param->a2dp_config_cmpl.bd_addr, 6);

#if F_APP_INTEGRATED_TRANSCEIVER
                src_addr_ready = true;
#endif

            }
        }
        break;
#endif

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            curr_idx = app_src_play_a2dp_get_connected_idx(param->a2dp_disconn_cmpl.bd_addr);
            a2dp_play.sink_a2dp_state[curr_idx] = A2DP_STATE_DISCONN;
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            app_tri_dongle_mgr_set_a2dp_packet();
#endif
            memset(a2dp_play.sink_addr[curr_idx], 0, 6);
            a2dp_play.sink_a2dp_param[curr_idx].multi_a2dp_credits = 8;
            a2dp_play.sink_a2dp_param[curr_idx].multi_a2dp_seq_num = 0;
            a2dp_play.sink_a2dp_param[curr_idx].multi_a2dp_timestamp = 0;
            a2dp_play.sink_a2dp_param[curr_idx].a2dp_is_used = 0;
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            app_stop_timer(&timer_idx_src_play_a2dp_open);
            a2dp_play.sink_a2dp_state[stream_idx] = A2DP_STATE_STREAM_OPEN;
            a2dp_seq_num = 0;
            a2dp_timestamp = 0;
            memcpy(&a2dp_play.sink_addr[stream_idx][0], param->a2dp_stream_open.bd_addr, 6);
            cur_pair_idx = stream_idx;
            if (bt_bond_index_get(a2dp_play.sink_addr[stream_idx], &cur_pair_idx) == false)
            {
                APP_PRINT_ERROR1("app_src_play_a2dp_bt_cback: get pair idx fail %d", cur_pair_idx);
            }
#if F_APP_MULTILINK_ENABLE
            stream_idx++;
#endif
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            // bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
            curr_idx = app_src_play_a2dp_get_connected_idx(param->a2dp_stream_start_ind.bd_addr);
            a2dp_play.sink_a2dp_state[curr_idx] = A2DP_STATE_STREAM_START;
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            app_tri_dongle_mgr_set_a2dp_packet();
#endif
            bt_sniff_mode_disable(a2dp_play.sink_addr[curr_idx]);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            curr_idx = app_src_play_a2dp_get_connected_idx(param->a2dp_stream_start_rsp.bd_addr);
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            a2dp_play.sink_a2dp_state[curr_idx] = A2DP_STATE_STREAM_START_RSP;
            app_tri_dongle_mgr_set_a2dp_packet();
#else
            a2dp_play.sink_a2dp_state[curr_idx] = A2DP_STATE_STREAM_START;
#endif
            bt_sniff_mode_disable(a2dp_play.sink_addr[curr_idx]);
            app_src_play_a2dp_start_rsp_handler();
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            curr_idx = app_src_play_a2dp_get_connected_idx(param->a2dp_stream_stop.bd_addr);
            a2dp_play.sink_a2dp_state[curr_idx] = A2DP_STATE_STREAM_STOP;
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            app_tri_dongle_mgr_set_a2dp_packet();
#endif
            bt_sniff_mode_enable(a2dp_play.sink_addr[curr_idx], 784, 816, 0, 0);
        }
        break;

    case BT_EVENT_A2DP_STREAM_DATA_RSP:
        {
            curr_idx = app_src_play_a2dp_get_connected_idx(param->a2dp_stream_data_rsp.bd_addr);
            a2dp_credits++;
            if (a2dp_credits > A2DP_MAX_CREDITS)
            {
                a2dp_credits = A2DP_MAX_CREDITS;
            }
            a2dp_play.sink_a2dp_param[curr_idx].multi_a2dp_credits++;
            if (a2dp_play.sink_a2dp_param[curr_idx].multi_a2dp_credits > A2DP_MAX_CREDITS)
            {
                a2dp_play.sink_a2dp_param[curr_idx].multi_a2dp_credits = A2DP_MAX_CREDITS;
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {

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
        APP_PRINT_INFO1("app_src_play_a2dp_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_src_play_print_a2dp_format(const char *title, T_AUDIO_FORMAT_INFO *format_info)
{
    APP_PRINT_TRACE1("@@@@@%s@@@@@", TRACE_STRING(title));
    switch (format_info->type)
    {
    case AUDIO_FORMAT_TYPE_SBC:
        {
            APP_PRINT_INFO7("SBC, "
                            "frame_num %d, sample_rate %d, allocation_method %d, bitpool %d, block_length %d, chann_mode %d, subband_num %d",
                            format_info->frame_num,
                            format_info->attr.sbc.sample_rate,
                            format_info->attr.sbc.allocation_method,
                            format_info->attr.sbc.bitpool,
                            format_info->attr.sbc.block_length,
                            format_info->attr.sbc.chann_mode,
                            format_info->attr.sbc.subband_num);
        }
        break;

    case AUDIO_FORMAT_TYPE_AAC:
        {
            APP_PRINT_INFO4("AAC, "
                            "transport_format:0x%x, sample_rate:%d, channel_mode:%d, bitrate:%d",
                            format_info->attr.aac.transport_format,
                            format_info->attr.aac.sample_rate,
                            format_info->attr.aac.chann_num,
                            format_info->attr.aac.bitrate);
        }
        break;

    default:
        break;
    }
}

void app_src_play_save_a2dp_format(T_AUDIO_FORMAT_INFO *format_info, uint8_t *bd_addr, uint8_t role)
{
    uint8_t link_idx;
    link_idx = app_src_play_a2dp_get_connected_idx(bd_addr);
    if (role == BT_A2DP_ROLE_SRC)
    {
        if (!a2dp_play.sink_a2dp_format_ready[link_idx])
        {
            memcpy(&a2dp_play.sink_a2dp_format[link_idx], format_info, sizeof(T_AUDIO_FORMAT_INFO));

            if (a2dp_play.sink_a2dp_format[link_idx].attr.sbc.bitpool == 0)
            {
                a2dp_play.sink_a2dp_format[link_idx].attr.sbc.bitpool = 0x22;
            }

#if F_APP_INTEGRATED_TRANSCEIVER
            a2dp_play.sink_a2dp_format[link_idx].attr.sbc.bitpool = 0x25;
#endif
            a2dp_play.sink_a2dp_format_ready[link_idx] = true;
        }
        app_src_play_print_a2dp_format("app_src_play_save_a2dp_snk_format: ",
                                       &a2dp_play.sink_a2dp_format[link_idx]);
    }
    else if (role == BT_A2DP_ROLE_SNK)
    {
        if (!a2dp_play.src_a2dp_format_ready)
        {
            memcpy(&a2dp_play.src_a2dp_format, format_info, sizeof(T_AUDIO_FORMAT_INFO));

            if (a2dp_play.src_a2dp_format.attr.sbc.bitpool == 0)
            {
                a2dp_play.src_a2dp_format.attr.sbc.bitpool = 0x22;
            }

#if F_APP_INTEGRATED_TRANSCEIVER
            a2dp_play.src_a2dp_format.attr.sbc.bitpool = 0x25;
#endif
            a2dp_play.src_a2dp_format_ready = true;
        }
        app_src_play_print_a2dp_format("app_src_play_save_a2dp_src_format: ",
                                       &a2dp_play.src_a2dp_format);
    }



}

bool app_src_play_get_a2dp_format(T_AUDIO_FORMAT_INFO *format_info)
{
    uint8_t link_idx;
    uint8_t link_count = 0;
    for (link_idx = 0; link_idx < MAX_BR_LINK_NUM; link_idx++)
    {
        if (a2dp_play.sink_a2dp_format_ready[link_idx])
        {
            memcpy(format_info, &a2dp_play.sink_a2dp_format[link_idx], sizeof(T_AUDIO_FORMAT_INFO));
            link_count++;
            app_src_play_print_a2dp_format("app_src_play_get_a2dp_format: ",
                                           format_info);
        }
        if (!a2dp_play.sink_a2dp_param[link_idx].a2dp_is_used)
        {
            break;
        }
    }
    APP_PRINT_INFO2("app_src_play_get_a2dp_format: link_idx %d, link_count %d", link_idx, link_count);

#if F_APP_INTEGRATED_TRANSCEIVER
    if (link_count == 2)
    {
        bt_device_mode_set(BT_DEVICE_MODE_IDLE);
    }
#endif

    return (link_idx == link_count && link_count != 0);
}

static void app_src_play_a2dp_buf_init(void)
{
    if (a2dp_play.p_buf == NULL)
    {
        a2dp_play.p_buf = calloc(1, A2DP_FRAME_BUF);
        ring_buffer_init(&a2dp_play.ring_buf, a2dp_play.p_buf, A2DP_FRAME_BUF);
    }
}

void app_src_play_a2dp_init(void)
{
    app_src_play_a2dp_buf_init();
    a2dp_play.num_frame_buf = 0;

    if (app_bt_policy_cfg_nv.enable_multi_link)
    {
        for (int i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            a2dp_play.sink_a2dp_param[i].multi_a2dp_credits = 8;
        }
    }

    app_timer_reg_cb(app_src_play_a2dp_timeout_cb, &app_src_play_a2dp_timer_id);

    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        bt_mgr_cback_register(app_src_play_a2dp_bt_cback);
    }

}

#endif
