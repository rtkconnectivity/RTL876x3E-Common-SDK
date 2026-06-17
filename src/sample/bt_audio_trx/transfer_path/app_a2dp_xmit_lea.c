/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_A2DP_XMIT_SRC_LEA_SUPPORT
#include <stdlib.h>
#include "trace.h"
#include "app_a2dp_xmit_lea.h"
#include "app_a2dp_xmit_mgr.h"
#include "hw_tim.h"
#include "app_cfg.h"
#include "app_dsp_cfg.h"
#include "ring_buffer.h"
#include "audio_pipe.h"
#include "audio_type.h"
#include "string.h"
#include "app_io_msg.h"
#include "app_main.h"
#include "app_lea_ini_audio_data.h"

static T_AUDIO_PIPE_HANDLE audio_pipe_handle = NULL;
static void *p_drain_data_buf = NULL;
static bool flag_pipe_get_data_empty = false;

#define DATA_OUT_RBUF_SIZE                  1024 * 4

static bool app_a2dp_xmit_lea_iso_data_read(uint8_t *data, uint32_t len);
static uint16_t app_a2dp_xmit_lea_fill_pipe(void);
static void app_a2dp_xmit_lea_send_iso_data(void);

static struct
{
    T_A2DP_XMIT_PLAY_STATE          play_state;
    T_AUDIO_FORMAT_INFO             format_in;
    T_AUDIO_FORMAT_INFO             format_out;
    uint8_t                         pipe_fill_seq;
    uint8_t                         pipe_ind_seq;
    bool                            timer_started;
    T_HW_TIMER_HANDLE               hw_timer_handle;
    uint8_t                         *p_lea_send_buf;
    uint8_t                         chnl_cnt;
    T_RING_BUFFER                   ring_buf;
    uint8_t                         *buffer;
} a2dp_xmit_lea_ctrl =
{
    .hw_timer_handle = NULL,
    .play_state = XMIT_PLAY_STATE_IDLE,
    .timer_started = false,
    .p_lea_send_buf = NULL,
};

static bool app_a2dp_xmit_lea_msg_send(IO_LEA_SRC_MSG_TYPE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type = IO_MSG_TYPE_LEA_SRC;
    msg.subtype = subtype;
    msg.u.buf = param_buf;

    return app_io_msg_send(&msg);
}

void app_a2dp_xmit_lea_msg_handle(T_IO_MSG io_msg)
{
    IO_LEA_SRC_MSG_TYPE sub_type = (IO_LEA_SRC_MSG_TYPE)io_msg.subtype;
    if (sub_type == IO_LEA_SRC_SYNC_TIMER)
    {
        app_a2dp_xmit_lea_send_iso_data();
    }
}

static void app_a2dp_xmit_lea_send_iso_data(void)
{
    uint16_t res = A2DP_XMIT_MGR_SUCCESS;
    uint16_t len = a2dp_xmit_lea_ctrl.format_out.attr.lc3.frame_length * a2dp_xmit_lea_ctrl.chnl_cnt;

    if (a2dp_xmit_lea_ctrl.p_lea_send_buf == NULL ||
        a2dp_xmit_lea_ctrl.play_state == XMIT_PLAY_STATE_IDLE)
    {
        APP_PRINT_ERROR0("app_a2dp_xmit_lea_send_iso_data: check buf and play status");
        return;
    }

    uint8_t read_ret = app_a2dp_xmit_lea_iso_data_read(a2dp_xmit_lea_ctrl.p_lea_send_buf, len);
    if (read_ret != A2DP_XMIT_MGR_SUCCESS)
    {
        memset(a2dp_xmit_lea_ctrl.p_lea_send_buf, 0, len);
    }
    app_lea_iso_data_send(a2dp_xmit_lea_ctrl.p_lea_send_buf, len, false, 0, 0);
}

static void app_a2dp_xmit_lea_sync_timer_start(void)
{
    a2dp_xmit_lea_ctrl.timer_started = true;
    hw_timer_start(a2dp_xmit_lea_ctrl.hw_timer_handle);
}

static void app_a2dp_xmit_lea_sync_timer_stop(void)
{
    a2dp_xmit_lea_ctrl.timer_started = false;
    hw_timer_stop(a2dp_xmit_lea_ctrl.hw_timer_handle);
}

static void app_a2dp_xmit_lea_sync_timer_handler(T_HW_TIMER_HANDLE handle)
{
    app_a2dp_xmit_lea_msg_send(IO_LEA_SRC_SYNC_TIMER, NULL);
}

static void app_a2dp_xmit_lea_sync_timer_init(uint32_t period)
{
    if (a2dp_xmit_lea_ctrl.hw_timer_handle == NULL)
    {
        a2dp_xmit_lea_ctrl.hw_timer_handle = hw_timer_create("leaudio_sync", period, true,
                                                             app_a2dp_xmit_lea_sync_timer_handler);
    }

    hw_timer_lpm_set(a2dp_xmit_lea_ctrl.hw_timer_handle, false);
    if (a2dp_xmit_lea_ctrl.hw_timer_handle == NULL)
    {
        APP_PRINT_ERROR0("app_a2dp_xmit_lea_sync_timer_init: create fail");
    }
}

static void app_a2dp_xmit_lea_state_restore(void)
{
    a2dp_xmit_lea_ctrl.play_state = XMIT_PLAY_STATE_IDLE;
    if (a2dp_xmit_lea_ctrl.timer_started)
    {
        app_a2dp_xmit_lea_sync_timer_stop();
    }
    if (audio_pipe_handle != NULL)
    {
        audio_pipe_release(audio_pipe_handle);
    }

    if (a2dp_xmit_lea_ctrl.p_lea_send_buf != NULL)
    {
        free(a2dp_xmit_lea_ctrl.p_lea_send_buf);
        a2dp_xmit_lea_ctrl.p_lea_send_buf = NULL;
    }

    ring_buffer_clear(&a2dp_xmit_lea_ctrl.ring_buf);
    app_a2dp_xmit_mgr_mgr_a2dp_raw_data_clear();
}

static bool app_a2dp_xmit_lea_iso_data_read(uint8_t *data, uint32_t len)
{
    uint16_t res = A2DP_XMIT_MGR_SUCCESS;
    uint32_t used_size = 0;

    used_size = ring_buffer_get_data_count(&a2dp_xmit_lea_ctrl.ring_buf);

    if (len > used_size)
    {
        APP_PRINT_WARN2("app_a2dp_xmit_lea_iso_data_read: read_len %d, used_size %d",
                        len, used_size);

        res = A2DP_XMIT_MGR_READ_ERROR;
    }
    else
    {
        ring_buffer_read(&a2dp_xmit_lea_ctrl.ring_buf, len, data);
    }

    return res;
}

static uint16_t app_a2dp_xmit_lea_iso_data_storage(uint8_t *data, uint16_t len)
{
    uint16_t res = A2DP_XMIT_MGR_SUCCESS;
    uint32_t space_size = 0;
    uint32_t used_size = 0;

    space_size = ring_buffer_get_remaining_space(&a2dp_xmit_lea_ctrl.ring_buf);
    used_size = ring_buffer_get_data_count(&a2dp_xmit_lea_ctrl.ring_buf);

    if (space_size < len)
    {
        res = A2DP_XMIT_MGR_WRITE_ERROR;
        APP_PRINT_WARN2("app_a2dp_xmit_lea_iso_data_storage: space_size %d, used_size:%d",
                        space_size, used_size);
    }
    else
    {
        ring_buffer_write(&a2dp_xmit_lea_ctrl.ring_buf, data, len);
    }

    return res;
}

static uint16_t app_a2dp_xmit_lea_fill_pipe(void)
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
                APP_PRINT_INFO3("app_a2dp_xmit_lea_fill_pipe: pipe_fill_seq %d, len %d, frame num %d",
                                a2dp_xmit_lea_ctrl.pipe_fill_seq, audio_info.len, audio_info.frame_num);

                if (!audio_pipe_fill(audio_pipe_handle, 0, a2dp_xmit_lea_ctrl.pipe_fill_seq,
                                     AUDIO_STREAM_STATUS_CORRECT, audio_info.frame_num,
                                     (void *)p_read_data_buf, audio_info.len))
                {
                    res = A2DP_XMIT_MGR_PIPE_FILL_ERROR;
                }
                else
                {
                    a2dp_xmit_lea_ctrl.pipe_fill_seq++;
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

static uint16_t app_a2dp_xmit_lea_pipe_data_ind_handler(void)
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
        res = app_a2dp_xmit_lea_iso_data_storage(p_drain_data_buf, data_len);
    }
    APP_PRINT_INFO3("app_a2dp_xmit_lea_pipe_data_ind_handler: data_len %d, frame_number %d, res %d",
                    data_len, frame_number, res);
    return res;
}

static bool app_a2dp_xmit_lea_pipe_callback(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT event,
                                            uint32_t param)
{
    APP_PRINT_INFO1("app_a2dp_xmit_lea_pipe_callback: event 0x%x", event);

    switch (event)
    {
    case AUDIO_PIPE_EVENT_CREATED:
        {
            uint32_t snk_buf_size = param;

            APP_PRINT_TRACE1("app_a2dp_xmit_lea_pipe_callback: snk_buf_size:0x%x", snk_buf_size);
            audio_pipe_start(handle);
            if (p_drain_data_buf == NULL)
            {
                p_drain_data_buf = malloc(snk_buf_size);
            }
            a2dp_xmit_lea_ctrl.play_state = XMIT_PLAY_STATE_START;
        }
        break;

    case AUDIO_PIPE_EVENT_STARTED:
        {
            app_a2dp_xmit_lea_fill_pipe();
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_IND:
        {
            if (app_a2dp_xmit_lea_pipe_data_ind_handler() == A2DP_XMIT_MGR_SUCCESS)
            {
                a2dp_xmit_lea_ctrl.pipe_ind_seq++;
            }
            if (!a2dp_xmit_lea_ctrl.timer_started &&
                a2dp_xmit_lea_ctrl.pipe_ind_seq > 2)
            {
                app_a2dp_xmit_lea_sync_timer_start();
            }
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_FILLED:
        {
            app_a2dp_xmit_lea_fill_pipe();
        }
        break;

    case AUDIO_PIPE_EVENT_RELEASED:
        {
            if (p_drain_data_buf != NULL)
            {
                free(p_drain_data_buf);
                p_drain_data_buf = NULL;
                a2dp_xmit_lea_ctrl.pipe_fill_seq = 0;
                a2dp_xmit_lea_ctrl.pipe_ind_seq = 0;
            }
            audio_pipe_handle = NULL;
        }
        break;

    default:
        break;
    }
    return true;
}


void app_a2dp_xmit_lea_pipe_rcfg(void)
{
    if (!app_a2dp_xmit_mgr_get_a2dp_in_format((uint8_t *)&a2dp_xmit_lea_ctrl.format_in))
    {
        APP_PRINT_ERROR0("app_a2dp_xmit_lea_pipe_rcfg: a2dp format info does not exist");
        return;
    }

    if (!app_lea_get_data_format(LEA_CODEC_DIR_ENCODE, &a2dp_xmit_lea_ctrl.format_out))
    {
        APP_PRINT_ERROR0("app_a2dp_xmit_lea_pipe_rcfg: lc3 format info does not exist");
        return;
    }

    app_a2dp_xmit_mgr_print_format("app_a2dp_xmit_lea_pipe_rcfg: ",
                                   a2dp_xmit_lea_ctrl.format_in);
    app_a2dp_xmit_mgr_print_format("app_a2dp_xmit_lea_pipe_rcfg: ",
                                   a2dp_xmit_lea_ctrl.format_out);

    if (a2dp_xmit_lea_ctrl.format_out.attr.lc3.chann_location == AUDIO_LOCATION_MONO)
    {
        a2dp_xmit_lea_ctrl.chnl_cnt = 1;
    }
    else
    {
        a2dp_xmit_lea_ctrl.chnl_cnt = __builtin_popcount(
                                          a2dp_xmit_lea_ctrl.format_out.attr.lc3.chann_location);
    }

    uint16_t len = a2dp_xmit_lea_ctrl.format_out.attr.lc3.frame_length * a2dp_xmit_lea_ctrl.chnl_cnt;
    a2dp_xmit_lea_ctrl.p_lea_send_buf = calloc(1, len);
    APP_PRINT_INFO1("app_a2dp_xmit_lea_pipe_rcfg: p_lea_send_buf len %d", len);
    if (a2dp_xmit_lea_ctrl.p_lea_send_buf == NULL)
    {
        APP_PRINT_ERROR0("app_a2dp_xmit_lea_pipe_rcfg: p_lea_send_buf malloc fail");
        return;
    }

    if (audio_pipe_handle == NULL)
    {
        audio_pipe_handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL,
                                              a2dp_xmit_lea_ctrl.format_in, a2dp_xmit_lea_ctrl.format_out,
                                              app_dsp_cfg_vol.playback_volume_default,
                                              app_a2dp_xmit_lea_pipe_callback);
    }
    uint32_t sync_timer_period = 0;
    if (a2dp_xmit_lea_ctrl.format_out.attr.lc3.frame_duration == AUDIO_LC3_FRAME_DURATION_10_MS)
    {
        app_a2dp_xmit_lea_sync_timer_init(10000);
    }
    else
    {
        app_a2dp_xmit_lea_sync_timer_init(7500);
    }
}

void app_a2dp_xmit_lea_handle_a2dp_data_ind(uint8_t *p_audio, T_A2DP_XMIT_AUDIO_INFO audio_info)
{
    if (a2dp_xmit_lea_ctrl.play_state == XMIT_PLAY_STATE_IDLE)
    {
        APP_PRINT_ERROR0("app_a2dp_xmit_lea_handle_a2dp_data_ind: brsc not started");
        return;
    }

    if (app_a2dp_xmit_mgr_a2dp_raw_data_write(p_audio, audio_info) == A2DP_XMIT_MGR_SUCCESS)
    {
        if (flag_pipe_get_data_empty)
        {
            app_a2dp_xmit_lea_fill_pipe();
        }
    }
}

void app_a2dp_xmit_lea_src_start_stop(T_A2DP_XMIT_PLAY_STATE type)
{
    APP_PRINT_INFO1("app_a2dp_xmit_lea_src_start_stop: %d", type);
    if (type == XMIT_PLAY_STATE_START)
    {
        app_lea_bsrc_start();
    }
    else if (type == XMIT_PLAY_STATE_IDLE)
    {
        app_a2dp_xmit_lea_state_restore();
        app_lea_bsrc_stop(true);
    }
}

void app_a2dp_xmit_lea_init(void)
{
    a2dp_xmit_lea_ctrl.buffer = (uint8_t *)calloc(1, DATA_OUT_RBUF_SIZE);
    ring_buffer_init(&a2dp_xmit_lea_ctrl.ring_buf, a2dp_xmit_lea_ctrl.buffer,
                     DATA_OUT_RBUF_SIZE);
}
#endif
