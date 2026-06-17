/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_A2DP_XMIT_SRC_SUPPORT
/* Includes -----------------------------------------------------------------*/
#include "trace.h"
#include "stdlib.h"
#include "os_msg.h"
#include "stdlib.h"
#include "audio_pipe.h"
#include "os_timer.h"
#include "bt_bond.h"
#include "bt_a2dp.h"
#include "app_io_msg.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_a2dp_xmit_src.h"
#include "app_a2dp_xmit_mgr.h"
#include "app_dlps.h"
#include "string.h"
#include "app_spi_api.h"
#include "app_timer.h"


/* Defines ------------------------------------------------------------------*/
#define SPI_A2DP_XMIT_SRC_RBUF_SIZE                   1024 * 3

/* Globals ------------------------------------------------------------------*/
static struct
{
    T_AUDIO_FORMAT_INFO             format_info_in;
    T_AUDIO_FORMAT_INFO             format_info_out;
    T_A2DP_XMIT_PLAY_STATE          play_state;
    T_A2DP_XMIT_SRC_STREAM_STATE    bt_strm_state;
    bool                            data_route_out_ready;
    uint8_t                         sink_addr[6];
} a2d_src_ctrl =
{
    .play_state = XMIT_PLAY_STATE_IDLE,
};

static uint8_t SRC_A2DP_STREAM_MAX_CREDITS = 8;
static uint8_t src_a2dp_credits = 8;
static bool    flag_pipe_get_data_empty = false;

static uint8_t  cur_pair_idx = 0;
static const uint8_t  min_gain_level = 0;               // Reserve for volume control
static const uint8_t  max_gain_level = 15;              // Reserve for volume control

static T_AUDIO_PIPE_HANDLE audio_pipe_handle = NULL;

static uint16_t  a2dp_seq_num = 0;
static uint32_t  a2dp_timestamp = 0;

static const uint16_t a2dp_gain_table[] =
{
    0x8001, 0xeb00, 0xec80, 0xee00, 0xef80, 0xf100, 0xf280, 0xf400,
    0xf580, 0xf700, 0xf880, 0xfa00, 0xfb80, 0xfd00, 0xfe80, 0x0000
};

static void     *p_drain_data_buf = NULL;
static uint8_t seq_num = 0;

static uint8_t  app_a2dp_xmit_src_timer_id = 0;          // app timer
static uint8_t  timer_idx_a2dp_src_open = 0;            // app timer
typedef enum
{
    APP_TIMER_A2DP_XMIT_SRC_OPEN_STREAM,
} T_APP_A2DP_XMIT_SRC_TIMER;                             // app timer

/* end of Globals------------------------------------------------------------------*/

T_A2DP_XMIT_PLAY_STATE app_a2dp_xmit_src_get_state(void)
{
    return a2d_src_ctrl.play_state;
}

void app_a2dp_xmit_src_save_a2dp_out_param(uint8_t *format_info)
{
    if (!a2d_src_ctrl.data_route_out_ready)
    {
        memcpy(&a2d_src_ctrl.format_info_out, format_info, sizeof(T_AUDIO_FORMAT_INFO));

        if (a2d_src_ctrl.format_info_out.type == AUDIO_FORMAT_TYPE_SBC)
        {
            if (a2d_src_ctrl.format_info_out.attr.sbc.bitpool == 0)
            {
                a2d_src_ctrl.format_info_out.attr.sbc.bitpool = 0x22;
            }
        }
        a2d_src_ctrl.data_route_out_ready = true;
    }
    app_a2dp_xmit_mgr_print_format("app_a2dp_xmit_src_save_a2dp_out_param: ",
                                   a2d_src_ctrl.format_info_out);
}

static uint16_t app_a2dp_xmit_src_pipe_data_ind(void)
{
    uint16_t res = A2DP_XMIT_MGR_SUCCESS;
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
                     p_drain_data_buf,
                     &data_len);
    if (data_len == 0)
    {
        res = A2DP_XMIT_MGR_PIPE_DRAIN_ERROR;
    }
    else
    {
        if (src_a2dp_credits)
        {
            if (bt_a2dp_stream_data_send(a2d_src_ctrl.sink_addr, a2dp_seq_num, a2dp_timestamp,
                                         frame_number, p_drain_data_buf, data_len))
            {
                a2dp_seq_num++;
                a2dp_timestamp += frame_number * a2d_src_ctrl.format_info_out.attr.sbc.block_length *
                                  a2d_src_ctrl.format_info_out.attr.sbc.subband_num;
                src_a2dp_credits--;
            }
            else
            {
                res = A2DP_XMIT_MGR_DATA_SEND_ERROR;
            }
        }
        else
        {
            APP_PRINT_WARN0("app_a2dp_xmit_src_pipe_data_ind: no reason need to send");
        }
    }
    APP_PRINT_INFO1("app_a2dp_xmit_src_pipe_data_ind: res 0x%x", res);
    return res;
}

static uint16_t app_a2dp_xmit_src_fill_pipe(void)
{
    uint16_t res = A2DP_XMIT_MGR_SUCCESS;
    T_A2DP_XMIT_AUDIO_INFO audio_info = {0};
    res = app_a2dp_xmit_mgr_a2dp_raw_data_read((uint8_t *)&audio_info, sizeof(T_A2DP_XMIT_AUDIO_INFO));
    if (res == A2DP_XMIT_MGR_SUCCESS)
    {
        uint8_t *p_read_data_buf = malloc(audio_info.len);
        if (p_read_data_buf)
        {
            res = app_a2dp_xmit_mgr_a2dp_raw_data_read(p_read_data_buf, audio_info.len);
            if (res == A2DP_XMIT_MGR_SUCCESS)
            {
                flag_pipe_get_data_empty = false;
                APP_PRINT_INFO3("app_a2dp_xmit_src_fill_pipe: seq_num %d, len %d, frame_num %d", seq_num,
                                audio_info.len, audio_info.frame_num);

                if (!audio_pipe_fill(audio_pipe_handle, 0, seq_num, AUDIO_STREAM_STATUS_CORRECT,
                                     audio_info.frame_num,
                                     (void *)p_read_data_buf, audio_info.len))
                {
                    res = A2DP_XMIT_MGR_PIPE_FILL_ERROR;
                }
                else
                {
                    seq_num++;
                }
            }
            else
            {
                flag_pipe_get_data_empty = true;
            }
            free(p_read_data_buf);
        }
        else
        {
            res = A2DP_XMIT_MGR_MEM_ERROR;
        }
    }
    else
    {
        flag_pipe_get_data_empty = true;
    }

    return res;
}

void app_a2dp_xmit_src_handle_a2dp_data_ind(uint8_t *data, T_A2DP_XMIT_AUDIO_INFO audio_info)
{
    if (a2d_src_ctrl.play_state != XMIT_PLAY_STATE_START)
    {
        APP_PRINT_ERROR1("app_a2dp_xmit_src_handle_a2dp_data_ind: play_state %d err",
                         a2d_src_ctrl.play_state);
        return;
    }

    if (app_a2dp_xmit_mgr_a2dp_raw_data_write(data, audio_info) == A2DP_XMIT_MGR_SUCCESS)
    {
        if (flag_pipe_get_data_empty)
        {
            app_a2dp_xmit_src_fill_pipe();
        }
    }
}

/* send spi a2pd sink data to dsp, get SBC data */
static bool audio_pipe_callback(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT  event,
                                uint32_t  param)
{
    APP_PRINT_INFO1("audio_pipe_callback: event 0x%x", event);

    switch (event)
    {
    case AUDIO_PIPE_EVENT_CREATED:
        {
            uint32_t snk_buf_size = param;

            APP_PRINT_TRACE1("audio_pipe_callback: snk_buf_size:0x%x", snk_buf_size);
            audio_pipe_start(handle);
            if (p_drain_data_buf == NULL)
            {
                p_drain_data_buf = malloc(snk_buf_size);
            }
            a2d_src_ctrl.play_state = XMIT_PLAY_STATE_START;
        }
        break;

    case AUDIO_PIPE_EVENT_STARTED: // send first pkt data to dsp
        {
            app_a2dp_xmit_src_fill_pipe();
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_IND: // get encode data to buf_pool
        {
            if (a2d_src_ctrl.play_state == XMIT_PLAY_STATE_START)
            {
                app_a2dp_xmit_src_pipe_data_ind();
            }
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_FILLED:  // dsp buf low, need put data to share memory
        {
            if (a2d_src_ctrl.play_state == XMIT_PLAY_STATE_START)
            {
                app_a2dp_xmit_src_fill_pipe();
            }
        }
        break;

    case AUDIO_PIPE_EVENT_RELEASED:
        {
            if (p_drain_data_buf != NULL)
            {
                free(p_drain_data_buf);
                p_drain_data_buf = NULL;
                seq_num = 0;
            }
            audio_pipe_handle = NULL;
        }
        break;

    default:
        break;
    }
    return true;
}

static uint8_t app_a2dp_xmit_src_param_rcfg(void)
{
    uint8_t res = A2DP_XMIT_MGR_SUCCESS;

    if (audio_pipe_handle != NULL)
    {
        res = A2DP_XMIT_MGR_PIPE_CREATE_ERROR;
        return res;
    }

    if (!app_a2dp_xmit_mgr_get_a2dp_in_format((uint8_t *)&a2d_src_ctrl.format_info_in))
    {
        APP_PRINT_ERROR0("app_a2dp_xmit_lea_pipe_rcfg: a2dp format info does not exist");
        res = A2DP_XMIT_MGR_SYS_ERROR;
        return res;
    }

    if (a2d_src_ctrl.data_route_out_ready)
    {
        T_AUDIO_FORMAT_INFO format_info_in = a2d_src_ctrl.format_info_in;
        T_AUDIO_FORMAT_INFO format_info_out = a2d_src_ctrl.format_info_out;

        if (audio_pipe_handle == NULL)
        {
            audio_pipe_handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL, format_info_in, format_info_out,
                                                  a2dp_gain_table[app_cfg_nv.audio_gain_level[cur_pair_idx]],
                                                  audio_pipe_callback);
        }
    }
    else
    {
        res = A2DP_XMIT_MGR_SYS_ERROR;
    }
    return res;
}

static void app_a2dp_xmit_src_stream_stop(void)
{
    if (a2d_src_ctrl.bt_strm_state != A2DP_XMIT_SRC_STREAM_STOP)
    {
        bt_a2dp_stream_suspend_req(a2d_src_ctrl.sink_addr);
    }

    if (a2d_src_ctrl.play_state == XMIT_PLAY_STATE_START)
    {
        a2d_src_ctrl.play_state = XMIT_PLAY_STATE_IDLE;
    }
    if (audio_pipe_handle != NULL)
    {
        audio_pipe_release(audio_pipe_handle);
    }

    app_a2dp_xmit_mgr_mgr_a2dp_raw_data_clear();
    app_dlps_enable(APP_DLPS_ENTER_CHECK_PLAYBACK);
}

static void app_a2dp_xmit_src_stream_start_req(void)
{
    bt_a2dp_stream_start_req(a2d_src_ctrl.sink_addr);
}

void app_a2dp_xmit_src_start_stop(T_A2DP_XMIT_PLAY_STATE type)
{
    APP_PRINT_INFO1("app_a2dp_xmit_src_start_stop: %d", type);
    if (type == XMIT_PLAY_STATE_START)
    {
        app_a2dp_xmit_src_stream_start_req();
    }
    else if (type == XMIT_PLAY_STATE_IDLE)
    {
        app_a2dp_xmit_src_stream_stop();
    }
}


static void app_a2dp_xmit_src_start_rsp_handler(void)
{
    uint8_t ret = app_a2dp_xmit_src_param_rcfg();
    APP_PRINT_INFO1("app_a2dp_xmit_src_start_rsp_handler: ret %d", ret);
    app_dlps_disable(APP_DLPS_ENTER_CHECK_PLAYBACK);
}

static void app_a2dp_xmit_src_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_a2dp_xmit_src_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_A2DP_XMIT_SRC_OPEN_STREAM:
        {
            bt_a2dp_stream_open_req(a2d_src_ctrl.sink_addr, BT_A2DP_ROLE_SNK);
        }
        break;

    default:
        break;
    }
}

static void app_a2dp_xmit_src_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_A2DP_CONN_CMPL:
        {
            a2d_src_ctrl.bt_strm_state = A2DP_XMIT_SRC_CONN;
            memcpy(a2d_src_ctrl.sink_addr, param->a2dp_conn_cmpl.bd_addr, 6);
            app_start_timer(&timer_idx_a2dp_src_open, "a2dp_open_stream",
                            app_a2dp_xmit_src_timer_id, APP_TIMER_A2DP_XMIT_SRC_OPEN_STREAM, 0, false,
                            1 * 1000);
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            a2d_src_ctrl.bt_strm_state = A2DP_XMIT_SRC_DISCONN;
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            a2d_src_ctrl.bt_strm_state = A2DP_XMIT_SRC_STREAM_CONN;
            a2dp_seq_num = 0;
            a2dp_timestamp = 0;
            memcpy(a2d_src_ctrl.sink_addr, param->a2dp_stream_open.bd_addr, 6);
            if (bt_bond_index_get(a2d_src_ctrl.sink_addr, &cur_pair_idx) == false)
            {
                APP_PRINT_ERROR0("app_a2dp_xmit_src_bt_cback: get pair idx fail");
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);
            a2d_src_ctrl.bt_strm_state = A2DP_XMIT_SRC_STREAM_START;
            bt_sniff_mode_disable(a2d_src_ctrl.sink_addr);
            bt_a2dp_stream_suspend_req(param->a2dp_stream_start_ind.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            a2d_src_ctrl.bt_strm_state = A2DP_XMIT_SRC_STREAM_START;
            bt_sniff_mode_disable(a2d_src_ctrl.sink_addr);
            app_a2dp_xmit_src_start_rsp_handler();
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            a2d_src_ctrl.bt_strm_state = A2DP_XMIT_SRC_STREAM_STOP;
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
        APP_PRINT_INFO1("app_a2dp_xmit_src_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_a2dp_xmit_src_init(void)
{
    app_timer_reg_cb(app_a2dp_xmit_src_timeout_cb, &app_a2dp_xmit_src_timer_id);

    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        bt_mgr_cback_register(app_a2dp_xmit_src_bt_cback);
    }
}
#endif
