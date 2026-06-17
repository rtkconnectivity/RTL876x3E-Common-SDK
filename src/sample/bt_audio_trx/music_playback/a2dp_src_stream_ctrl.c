/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
/* Includes -----------------------------------------------------------------*/
#include "trace.h"
#include "stdlib.h"
#include "os_msg.h"
#include "os_mem.h"
#include "stdlib.h"
#include "audio_pipe.h"
#include "os_timer.h"
#include "hw_tim.h"
#include "bt_bond.h"
#include "bt_a2dp.h"
#include "a2dp_enc_buffer.h"
#include "app_io_msg.h"
#include "app_cfg.h"
#include "app_audio_cfg.h"
#include "app_dsp_cfg.h"
#include "app_main.h"
#include "a2dp_src_stream_ctrl.h"
#include "app_dlps.h"
#include "playback_audio_file.h"
#include "audio_fs_decode.h"
#include "app_music_ctrl.h"

/**********************************************************************************************/
/***************************************a2dp source                                           */
/**********************************************************************************************/
/* Defines ------------------------------------------------------------------*/
#define A2DP_SRC_DEC_DATA_TX_PKT_NUM    4
#define A2DP_SRC_ENC_DATA_RX_PKT_NUM    6

/* Globals ------------------------------------------------------------------*/
static struct
{
    T_A2DP_SRC_PLAY_STATE       play_state;
    T_APP_A2DP_SRC_STATE        bt_strm_state;
    T_A2DP_SRC_BUF_STATE        buffer_state;
    uint8_t                     sink_addr[6];
    uint8_t                     frm_num; // check level
} a2d_src_ctrl;

extern T_OS_QUEUE   a2dp_enc_queue ;
void (*a2dp_src_stream_ctrl_stop_complete_hook)(void) = NULL;
void (*a2dp_src_stream_ctrl_delay_stop_hook)(uint16_t time_ms) = NULL;

void *p_snk_data_buf = NULL;
static T_AUDIO_PIPE_HANDLE audio_pipe_handle = NULL;
static uint8_t SRC_A2DP_STREAM_MAX_CREDITS = 8;
static uint8_t src_a2dp_credits = 8;
T_HW_TIMER_HANDLE a2dp_timer_handle = NULL;
void   *a2dp_open_stream_timer = NULL;

static const uint16_t a2dp_gain_table[] =
{
    0x8001, 0xeb00, 0xec80, 0xee00, 0xef80, 0xf100, 0xf280, 0xf400,
    0xf580, 0xf700, 0xf880, 0xfa00, 0xfb80, 0xfd00, 0xfe80, 0x0000
};
static uint8_t  cur_pair_idx = 0;
static const uint8_t  min_gain_level = 0;
static const uint8_t  max_gain_level = 15;
static uint8_t  a2dp_src_bitpool = 0x22;
static float    a2dp_src_timer = 21.333;
static float    a2dp_sbc_time = 2.6666;
static uint8_t  a2dp_sbc_block_length = 16;
static uint8_t  a2dp_sbc_subbands = 8;
static bool     dsp_tx_ack = false;

static uint16_t  a2dp_seq_num = 0;
static uint32_t  a2dp_timestamp = 0;

static bool a2dp_src_pipe_cb(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT event,
                             uint32_t param);

T_A2DP_SRC_PLAY_STATE a2dp_src_stream_get_state(void)
{
    return a2d_src_ctrl.play_state;
}

/**
  * @brief Initialize FATFS file system and sd card physical layer.
  * @param   No parameter.
  * @return  void
  */
uint8_t a2dp_src_stream_param_recfg(void)
{
    uint8_t res = A2DP_SRC_SUCCESS;
    uint16_t u16_res = 0;
    uint32_t sample_rate = 0;
//    uint16_t sample_counts = 1024; /* default */
//    uint16_t frame_duration = 20; /* default */
    uint16_t frame_size = 512;
    uint8_t channel_mode = 0;
//    uint32_t bit_rate = 0;

    T_PLAYBACK_AF_FORMAT_INFO get_fmt_info;

    if (audio_pipe_handle != NULL)
    {
        res = A2DP_SRC_PIPE_CREATE_ERROR;
        return res;
    }
    u16_res = playback_audio_file_get_audio_info(&get_fmt_info);
    if (u16_res == 0)
    {
        T_AUDIO_FORMAT_INFO format_info;

        format_info = get_fmt_info.format_info;
        frame_size = get_fmt_info.frame_size;
        a2d_src_ctrl.frm_num = 4;
        if (frame_size > 2048)
        {
            frame_size = 1024;
        }
        uint8_t frm_num = STREAM_BUF_SIZE / 2 / frame_size;
        a2d_src_ctrl.frm_num = (frm_num > 8) ? 7 : 4;

        if (format_info.type == AUDIO_FORMAT_TYPE_AAC)
        {
            APP_PRINT_INFO4("a2dp_src_stream_param_recfg: AAC, "
                            " transport_format:0x%x, sample_rate:%d, channel_mode:%d, bitrate:%d",
                            format_info.attr.aac.transport_format,
                            format_info.attr.aac.sample_rate,
                            format_info.attr.aac.chann_num,
                            format_info.attr.aac.bitrate);
        }
        else if (format_info.type == AUDIO_FORMAT_TYPE_MP3)
        {
            APP_PRINT_INFO3("a2dp_src_stream_param_recfg: MP3, sample_rate:%d, channel_mode:%d, frm_num:%d",
                            format_info.attr.mp3.sample_rate,
                            format_info.attr.mp3.chann_mode,
                            a2d_src_ctrl.frm_num);
        }

        T_AUDIO_FORMAT_INFO snk_info;
        snk_info.type = AUDIO_FORMAT_TYPE_SBC;
        snk_info.frame_num = 6;
        snk_info.attr.sbc.subband_num = 8;
        snk_info.attr.sbc.bitpool = a2dp_src_bitpool;          //change to be same with min bitpool
        snk_info.attr.sbc.sample_rate = 48000;
        snk_info.attr.sbc.block_length = 16;
        snk_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
        snk_info.attr.sbc.allocation_method = 0;

        float sbc_block = snk_info.attr.sbc.block_length;
        float sbc_subband = snk_info.attr.sbc.subband_num;

        a2dp_sbc_time = (sbc_block * sbc_subband) / 48;
        if (audio_pipe_handle == NULL)
        {
            audio_pipe_handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL, format_info, snk_info,
                                                  a2dp_gain_table[app_cfg_nv.audio_gain_level[cur_pair_idx]],
                                                  a2dp_src_pipe_cb);
        }

    }
    else
    {
        res = A2DP_SRC_SYS_ERROR;
    }
    return res;
}

static uint16_t a2dp_src_stream_data_ind(void)
{
    uint16_t res = A2DP_SRC_SUCCESS;
    uint16_t data_len = 0;
    uint32_t timestamp = 0;
    uint16_t seq = 0;
    T_AUDIO_STREAM_STATUS status;
    uint8_t  frame_number = 0;

    audio_pipe_drain(audio_pipe_handle,
                     &timestamp,
                     &seq,
                     &status,
                     &frame_number,
                     p_snk_data_buf,
                     &data_len);
    if (data_len == 0)
    {
        res = A2DP_SRC_PIPE_DRAIN_ERROR;
    }
    else
    {

        if (!a2dp_enc_data_rx((uint8_t *)p_snk_data_buf, data_len, frame_number))
        {
            res = A2DP_SRC_ENC_RX_ERROR;
        }

//#if A2DP_SRC_STREAM_DBG == 0
        if ((a2dp_enc_queue.count >= A2DP_ENC_WATER_LEVEL_H) &&
            (a2d_src_ctrl.buffer_state == A2DP_SRC_BUF_LOW))
        {
            hw_timer_restart(a2dp_timer_handle, 10 * 1000);
            a2d_src_ctrl.buffer_state = A2DP_SRC_BUF_HIGH;
        }
//#endif
    }
    APP_PRINT_INFO5("a2dp_src_stream_data_ind: res:0x%x, data_len: 0x%x, frame_number: 0x%x, source_buffer_state: 0x%x, enc_sbc_count: 0x%x,",
                    res, data_len, frame_number, a2d_src_ctrl.buffer_state, a2dp_enc_queue.count);
    return res;
}

static uint16_t a2dp_src_stream_get_data_from_fs(void)
{
    uint16_t res = A2DP_SRC_SUCCESS;
    T_PLAYBACK_FRAME_PKT playback_frame;

//    stream_check_and_request_data();
    res = playback_audio_file_get_frame(&playback_frame);
    if ((res == PLAYBACK_AF_END_ERROR) || 0) //(playback_audio_file_is_end()))
    {
        APP_PRINT_WARN0("a2dp_src_stream_get_data_from_fs, file end, and play next song!!!");
        if (a2dp_src_stream_ctrl_delay_stop_hook)
        {
            uint16_t delay_time_ms = (uint16_t)a2dp_src_timer * a2dp_enc_queue.count;
            a2dp_src_stream_ctrl_delay_stop_hook(delay_time_ms);
        }
        else
        {
            a2dp_src_stream_ctrl_stop();
            app_music_send_player_status(MUSIC_PLAYER_STOPPED);
            app_music_report_play_time();
        }
    }
    else
    {
        APP_PRINT_TRACE1("a2dp_src_stream_get_data_from_fs frame length: 0x%x", playback_frame.length);
        if (!audio_pipe_fill(audio_pipe_handle, 0, 0, AUDIO_STREAM_STATUS_CORRECT, 0,
                             (void *)playback_frame.buf, playback_frame.length))
        {
            res = A2DP_SRC_PIPE_FILL_ERROR;
        }
    }
    return res;
}

static uint16_t a2dp_src_stream_fill_data(void)
{
    uint16_t res = A2DP_SRC_SUCCESS;
    if (a2d_src_ctrl.buffer_state == A2DP_SRC_BUF_LOW)
    {
        res = a2dp_src_stream_get_data_from_fs();
    }
    else
    {
        dsp_tx_ack = true;
    }
    APP_PRINT_INFO3("a2dp_src_stream_fill_data: res:0x%x, buffer_state:0x%x, dsp_tx_ack:0x%x", res,
                    a2d_src_ctrl.buffer_state, dsp_tx_ack);
    return res;
}

static uint16_t a2dp_src_stream_send_data(void)
{
    uint16_t res = A2DP_SRC_SUCCESS;
#if A2DP_SRC_STREAM_DBG == 0
    if (a2d_src_ctrl.bt_strm_state != APP_A2DP_SRC_STREAM_START)
    {
        return A2DP_SRC_SYS_ERROR;
    }
#endif

    T_A2DP_ENC_MEIDAHEAD *da_pkt;
    da_pkt = a2dp_enc_audio_peek(0);
    if (da_pkt == NULL)
    {
        res = a2dp_src_stream_get_data_from_fs();
    }
    else
    {
#if A2DP_SRC_STREAM_DBG
        APP_PRINT_INFO3("a2dp_src_stream_send_data: len:%d, pkt num:%d, %b", da_pkt->payload_length,
                        (uint8_t)da_pkt->frame_num, TRACE_BINARY(8, da_pkt->p_data));
        a2dp_enc_audio_flush(1);
#else
        APP_PRINT_INFO1("a2dp_src_stream_send_data: src_a2dp_credits = %d", src_a2dp_credits);

        if (src_a2dp_credits)
        {
            if (bt_a2dp_stream_data_send(a2d_src_ctrl.sink_addr, a2dp_seq_num, a2dp_timestamp,
                                         (uint8_t)da_pkt->frame_num, da_pkt->p_data, da_pkt->payload_length))
            {
                a2dp_seq_num++;
                src_a2dp_credits--;
            }
            else
            {
                res = A2DP_SRC_DATA_SEND_ERROR;
            }

            a2dp_timestamp += da_pkt->frame_num * a2dp_sbc_block_length * a2dp_sbc_subbands;
            a2dp_enc_audio_flush(1);
        }
        else
        {
            APP_PRINT_WARN0("a2dp_src_stream_send_data no reason need to send");
        }
#endif

        if ((a2dp_enc_queue.count <= A2DP_ENC_WATER_LEVEL_L)/* && dsp_tx_ack*/)
        {
            dsp_tx_ack = false;
            res = a2dp_src_stream_get_data_from_fs();
        }
    }
    APP_PRINT_INFO4("a2dp_src_stream_send_data, res:0x%x, da_pkt:0x%x, dsp_tx_ack:0x%x, frameCount:0x%x",
                    res, da_pkt, dsp_tx_ack, a2dp_enc_queue.count);
    return res;
}

void a2dp_src_stream_hw_timer_callback(T_HW_TIMER_HANDLE handle)
{
    T_IO_MSG gpio_msg;

    T_A2DP_ENC_MEIDAHEAD *da_pkt;
    da_pkt = a2dp_enc_audio_peek(0);
    if (da_pkt != NULL)
    {
        a2dp_src_timer = (float)da_pkt->frame_num * a2dp_sbc_time;
        hw_timer_restart(a2dp_timer_handle, a2dp_src_timer * 1000);

        gpio_msg.type = IO_MSG_TYPE_A2DP_SRC;
        gpio_msg.subtype = AUDIO_A2DP_SRC_EVENT_DATA_SEND;
        if (app_io_msg_send(&gpio_msg) == false)
        {
            APP_PRINT_ERROR0("a2dp src hw timer: msg send fail");
        }
    }
}

static void a2dp_src_stream_hw_timer_init(void)
{
    a2dp_timer_handle = hw_timer_create("a2dp_hw_timer", 21333, true,
                                        a2dp_src_stream_hw_timer_callback);
    if (a2dp_timer_handle == NULL)
    {
        APP_PRINT_ERROR0("fail to create a2dp hw timer, check hw timer usage!!");
    }
    else
    {
        APP_PRINT_TRACE1("create a2dp hw timer instance successfully, id %d",
                         hw_timer_get_id(a2dp_timer_handle));
    }
}

void a2dp_src_stream_handle_msg(T_IO_MSG msg)
{
    uint16_t subtype = msg.subtype;

    APP_PRINT_INFO1("a2dp_src_stream_handle_msg: subtype 0x%x", subtype);

    switch (subtype)
    {
    case AUDIO_A2DP_SRC_EVENT_DATA_SEND: // timer msg peek data from buf_pool and send data
        {
            a2dp_src_stream_send_data();
        }
        break;

    default:
        break;
    }
}

static bool a2dp_src_pipe_cb(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT event,
                             uint32_t param)
{

    APP_PRINT_INFO1("a2dp_src_pipe_cb: event %d", event);
    switch (event)
    {
    case AUDIO_PIPE_EVENT_CREATED:
        {
            uint32_t snk_buf_size = param;

            APP_PRINT_TRACE1("AUDIO_PIPE_EVENT_CREATED snk_buf_size:0x%x", snk_buf_size);
            audio_pipe_start(handle);
            p_snk_data_buf = os_mem_alloc(RAM_TYPE_DSPSHARE, snk_buf_size);
            a2d_src_ctrl.play_state = A2DP_SRC_PLAY_STATE_PLAY;
        }
        break;

    case AUDIO_PIPE_EVENT_STARTED: // send first pkt data to dsp
        {
            uint16_t res_tmp = 0;
            res_tmp = a2dp_src_stream_get_data_from_fs();
            APP_PRINT_TRACE1("AUDIO_PIPE_EVENT_STARTED res_tmp 0x%x", res_tmp) ;
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_IND: // get encode data to buf_pool
        {
            if (a2d_src_ctrl.play_state == A2DP_SRC_PLAY_STATE_PLAY)
            {
                a2dp_src_stream_data_ind();
            }
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_FILLED:  // dsp buf low, need put data to share memory
        {
            if (a2d_src_ctrl.play_state == A2DP_SRC_PLAY_STATE_PLAY)
            {
                a2dp_src_stream_fill_data();
                static uint8_t s_check_cnt = 0;
                s_check_cnt++;
                if (s_check_cnt > 4)
                {
                    s_check_cnt = 0;
                    stream_check_and_request_data();
                }
            }
        }
        break;

    case AUDIO_PIPE_EVENT_RELEASED:
        {
            if (p_snk_data_buf != NULL)
            {
                free(p_snk_data_buf);
                p_snk_data_buf = NULL;
            }

            if (a2dp_src_stream_ctrl_stop_complete_hook)
            {
                a2dp_src_stream_ctrl_stop_complete_hook();
            }
            audio_pipe_handle = NULL;
        }
        break;

    case AUDIO_A2DP_SRC_EVENT_DATA_SEND: // timer msg peek data from buf_pool and send data
        {
            a2dp_src_stream_send_data();
        }
        break;

    default:
        break;
    }
    return true;
}

void a2dp_src_stream_volume_up(void)
{
    if (app_cfg_nv.audio_gain_level[cur_pair_idx] < max_gain_level)
    {
        app_cfg_nv.audio_gain_level[cur_pair_idx]++;
    }
    else
    {
        app_cfg_nv.audio_gain_level[cur_pair_idx] = max_gain_level;
    }

    APP_PRINT_TRACE2("a2dp_src_stream_volume_up,volume:%d,max:%d",
                     app_cfg_nv.audio_gain_level[cur_pair_idx], max_gain_level);

    if (audio_pipe_handle != NULL)
    {
        uint16_t gain_val = a2dp_gain_table[app_cfg_nv.audio_gain_level[cur_pair_idx]];
        audio_pipe_gain_set(audio_pipe_handle, gain_val, gain_val);
    }
    uint8_t vol = app_cfg_nv.audio_gain_level[cur_pair_idx] * 0x7F /
                  app_dsp_cfg_vol.playback_volume_max;
    bt_avrcp_absolute_volume_set(a2d_src_ctrl.sink_addr, vol);
}

void a2dp_src_stream_volume_down(void)
{
    if (app_cfg_nv.audio_gain_level[cur_pair_idx] > min_gain_level)
    {
        app_cfg_nv.audio_gain_level[cur_pair_idx]--;
    }
    else
    {
        app_cfg_nv.audio_gain_level[cur_pair_idx] = min_gain_level;
    }

    APP_PRINT_TRACE2("a2dp_src_stream_volume_down,volume:%d,min:%d",
                     app_cfg_nv.audio_gain_level[cur_pair_idx], min_gain_level);

    if (audio_pipe_handle != NULL)
    {
        uint16_t gain_val = a2dp_gain_table[app_cfg_nv.audio_gain_level[cur_pair_idx]];
        audio_pipe_gain_set(audio_pipe_handle, gain_val, gain_val);
    }
    uint8_t vol = app_cfg_nv.audio_gain_level[cur_pair_idx] * 0x7F /
                  app_dsp_cfg_vol.playback_volume_max;
    bt_avrcp_absolute_volume_set(a2d_src_ctrl.sink_addr, vol);
}

bool a2dp_src_stream_volume_set(uint8_t volume)
{
    bool ret = false;
    if (volume > max_gain_level)
    {
        volume = max_gain_level;
    }
    uint16_t gain_val = a2dp_gain_table[app_cfg_nv.audio_gain_level[cur_pair_idx]];
    APP_PRINT_INFO3("a2dp_src_stream_volume_set,volume:%d,min:%d,max:%d", volume, min_gain_level,
                    max_gain_level);
    if (audio_pipe_handle != NULL)
    {
        app_cfg_nv.audio_gain_level[cur_pair_idx] = volume;
        ret = audio_pipe_gain_set(audio_pipe_handle, gain_val, gain_val);
    }

    return ret;
}

uint8_t a2dp_src_stream_volume_get(void)
{
    uint8_t pair_idx;
    if (bt_bond_index_get(a2d_src_ctrl.sink_addr, &pair_idx) == false)
    {
        APP_PRINT_ERROR0("a2dp_src_stream_volume_get: find a2dp pair_index fail");
        return 0;
    }
    return app_cfg_nv.audio_gain_level[pair_idx];
}

void a2dp_src_stream_ctrl_start(void)
{
    APP_PRINT_INFO1("a2dp_src_stream_ctrl_start: bt stream state: %d", a2d_src_ctrl.bt_strm_state);
    a2d_src_ctrl.buffer_state = A2DP_SRC_BUF_LOW;
#if A2DP_SRC_STREAM_DBG == 0
    if (a2d_src_ctrl.bt_strm_state != APP_A2DP_SRC_STREAM_START)
    {
        bt_a2dp_stream_start_req(a2d_src_ctrl.sink_addr);
    }
    else
#endif
    {
        a2dp_src_stream_param_recfg();
        a2d_src_ctrl.play_state = A2DP_SRC_PLAY_STATE_PLAY;
        app_dlps_disable(APP_DLPS_ENTER_CHECK_PLAYBACK);
    }
}

void a2dp_src_stream_ctrl_stop(void)
{
    hw_timer_stop(a2dp_timer_handle);

    if (a2d_src_ctrl.play_state == A2DP_SRC_PLAY_STATE_PLAY)
    {
        APP_PRINT_TRACE0("a2dp_src_stream_ctrl_stop");
        a2d_src_ctrl.play_state = A2DP_SRC_PLAY_STATE_IDLE;

        if (a2d_src_ctrl.bt_strm_state != APP_A2DP_SRC_STREAM_STOP)
        {
            bt_a2dp_stream_suspend_req(a2d_src_ctrl.sink_addr);
        }

        a2dp_enc_audio_flush(a2dp_enc_queue.count);
        os_queue_init(&a2dp_enc_queue);
        a2d_src_ctrl.buffer_state = A2DP_SRC_BUF_LOW;

        if (audio_pipe_handle != NULL)
        {
            audio_pipe_release(audio_pipe_handle);
        }
        app_dlps_enable(APP_DLPS_ENTER_CHECK_PLAYBACK);
    }
}

static void a2dp_src_open_stream_callback(void *pxTimer)
{
    APP_PRINT_TRACE0("a2dp_src_open_stream_callback start");
    bt_a2dp_stream_open_req(a2d_src_ctrl.sink_addr, BT_A2DP_ROLE_SNK);
}

static void a2dp_src_stream_ctrl_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_A2DP_CONN_CMPL:
        {
            a2d_src_ctrl.bt_strm_state = APP_A2DP_SRC_CONN;
            memcpy(a2d_src_ctrl.sink_addr, param->a2dp_conn_cmpl.bd_addr, 6);
            os_timer_restart(&a2dp_open_stream_timer, 1000);
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            a2d_src_ctrl.bt_strm_state = APP_A2DP_SRC_DISCONN;
        }
        break;

    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
            if (param->a2dp_config_cmpl.role == BT_A2DP_ROLE_SNK)
            {
                bt_a2dp_stream_delay_report_req(param->a2dp_config_cmpl.bd_addr, A2DP_LATENCY_MS);
            }

            if (param->a2dp_config_cmpl.codec_type == BT_A2DP_CODEC_TYPE_SBC)
            {
                a2dp_sbc_block_length = param->a2dp_config_cmpl.codec_info.sbc.block_length;
                a2dp_sbc_subbands = param->a2dp_config_cmpl.codec_info.sbc.subbands;
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            os_timer_stop(&a2dp_open_stream_timer);
            a2d_src_ctrl.bt_strm_state = APP_A2DP_SRC_STREAM_CONN;
            a2dp_seq_num = 0;
            a2dp_timestamp = 0;
            memcpy(a2d_src_ctrl.sink_addr, param->a2dp_stream_open.bd_addr, 6);
            if (bt_bond_index_get(a2d_src_ctrl.sink_addr, &cur_pair_idx) == false)
            {
                APP_PRINT_ERROR0("A2dp stream open, get pair idx fail");
            }
            APP_PRINT_INFO2("pair idx = %d, vol level = %d", cur_pair_idx,
                            app_cfg_nv.audio_gain_level[cur_pair_idx]);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
            a2d_src_ctrl.bt_strm_state = APP_A2DP_SRC_STREAM_START;
            bt_sniff_mode_disable(a2d_src_ctrl.sink_addr);
            bt_a2dp_stream_suspend_req(param->a2dp_stream_start_ind.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            a2d_src_ctrl.bt_strm_state = APP_A2DP_SRC_STREAM_START;
            bt_sniff_mode_disable(a2d_src_ctrl.sink_addr);
            a2dp_src_stream_ctrl_start();
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            a2d_src_ctrl.bt_strm_state = APP_A2DP_SRC_STREAM_STOP;
            bt_sniff_mode_enable(a2d_src_ctrl.sink_addr, 784, 816, 0, 0);
        }
        break;

    case BT_EVENT_A2DP_STREAM_DATA_RSP:
        {
            src_a2dp_credits++;
            if (src_a2dp_credits > SRC_A2DP_STREAM_MAX_CREDITS)
            {
                src_a2dp_credits = SRC_A2DP_STREAM_MAX_CREDITS;
            }
            APP_PRINT_INFO1("get a2dp credits = %d", src_a2dp_credits);
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
        APP_PRINT_INFO1("a2dp_src_stream_ctrl_bt_cback: event_type 0x%04x", event_type);
    }
}

void a2dp_src_stream_ctrl_init(void)
{
    memset(&a2d_src_ctrl, 0, sizeof(a2d_src_ctrl));

    a2dp_src_stream_hw_timer_init();
    os_timer_create(&a2dp_open_stream_timer, "open stream timer", 1,
                    1 * 1000, false, a2dp_src_open_stream_callback);
    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        bt_mgr_cback_register(a2dp_src_stream_ctrl_bt_cback);
    }
}
#endif

