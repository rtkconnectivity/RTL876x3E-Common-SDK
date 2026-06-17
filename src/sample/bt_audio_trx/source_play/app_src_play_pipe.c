/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_SOURCE_PLAY_SUPPORT
#if F_APP_INTEGRATED_TRANSCEIVER

#include <stdlib.h>
#include "trace.h"
#include "string.h"

#include "app_src_play_a2dp.h"
#include "app_src_play_hfp.h"
#include "app_dlps.h"
#include "app_link_util_cs.h"
#include "app_timer.h"
#include "app_cfg.h"
#include "app_bt_policy_cfg.h"
#include "ring_buffer.h"
#include "btm.h"
#include "audio_type.h"
#include "bt_a2dp.h"
#include "bt_bond.h"
#include "audio_pipe.h"
#include "app_src_play_pipe.h"
#include "app_src_play.h"
#include "app_audio_policy.h"

#define A2DP_FRAME_BUF              1024

#define A2DP_MAX_CREDITS               8

typedef enum
{
    SRC_PIPE_STATE_NONE,
    SRC_PIPE_STATE_CREATED,
    SRC_PIPE_STATE_STARTED,
    SRC_PIPE_STATE_FILLING,
    SRC_PIPE_STATE_FILLED,
} T_SRC_PIPE_STATE;

typedef struct
{
    T_AUDIO_PIPE_HANDLE     handle;
    T_SRC_PIPE_PLAY_STATE   play_state;
    uint8_t                 volume;
    T_SRC_PIPE_STATE        fill_state;
    uint8_t                 fill_seq;
    uint8_t                 *p_buf;
    T_RING_BUFFER           ring_buf;
    uint8_t                 fill_frame_num;
    uint16_t                fill_len;
    void                    *p_drain_buf;
    T_SRC_PIPE_TYPE         pipe_type;
    bool                    get_data_empty;
} SRC_PLAY_PIPE_MGR;

static SRC_PLAY_PIPE_MGR play_downstream_pipe_mgr =
{
    .handle = NULL,
    .play_state = SRC_PIPE_PLAY_STATE_IDLE,
    .volume = 10,
    .fill_state = SRC_PIPE_STATE_NONE,
    .fill_seq = 0,
    .fill_frame_num = 0,
    .fill_len = 0,
    .p_drain_buf = NULL,
    .pipe_type = SRC_PIPE_DOWNSTREAM_PLAY_TYPE,
    .get_data_empty = false,
};

static SRC_PLAY_PIPE_MGR play_upstream_pipe_mgr =
{
    .handle = NULL,
    .play_state = SRC_PIPE_PLAY_STATE_IDLE,
    .volume = 10,
    .fill_state = SRC_PIPE_STATE_NONE,
    .fill_seq = 0,
    .fill_frame_num = 0,
    .fill_len = 0,
    .p_drain_buf = NULL,
    .pipe_type = SRC_PIPE_UPSTREAM_PLAY_TYPE,
    .get_data_empty = false,
};


static bool flag_a2dp_direct_send = false;
static bool flag_hfp_direct_send = false;

void app_src_play_pipe_raw_data_clear(void)
{
    if (play_downstream_pipe_mgr.handle)
    {
        ring_buffer_clear(&play_downstream_pipe_mgr.ring_buf);
    }
    if (play_upstream_pipe_mgr.handle)
    {
        ring_buffer_clear(&play_upstream_pipe_mgr.ring_buf);
    }
}

uint8_t app_src_play_pipe_raw_data_read(SRC_PLAY_PIPE_MGR *pipe_mgr, uint8_t *p_data, uint32_t len)
{
    uint8_t ret = SRC_PIPE_MGR_SUCCESS;
    uint32_t used_size = 0;
    used_size = ring_buffer_get_data_count(&pipe_mgr->ring_buf);
    if (len > used_size)
    {
        return SRC_PIPE_MGR_READ_ERROR;
    }
    ring_buffer_read(&pipe_mgr->ring_buf, len, p_data);
    return ret;
}

uint8_t app_src_play_pipe_raw_data_write(SRC_PLAY_PIPE_MGR *pipe_mgr, uint8_t *p_data, uint32_t len,
                                         uint8_t frame_number)
{
    uint8_t ret = SRC_PIPE_MGR_SUCCESS;

    uint32_t space_size = 0;
    uint32_t used_size = 0;

    space_size = ring_buffer_get_remaining_space(&pipe_mgr->ring_buf);
    used_size = ring_buffer_get_data_count(&pipe_mgr->ring_buf);

    if (space_size < len + 4)
    {
        APP_PRINT_WARN2("app_src_play_pipe_raw_data_write: space_size %d, used_size:%d", space_size,
                        used_size);
        ret = SRC_PIPE_MGR_WRITE_ERROR;
    }
    else
    {
        ring_buffer_write(&pipe_mgr->ring_buf, (uint8_t *)&len, 2);
        ring_buffer_write(&pipe_mgr->ring_buf, &frame_number, 2);
        ring_buffer_write(&pipe_mgr->ring_buf, p_data, len);
    }

    return ret;
}

static uint8_t app_src_play_pipe_fill_data(SRC_PLAY_PIPE_MGR *pipe_mgr)
{
    uint8_t res = SRC_PIPE_MGR_SUCCESS;
    uint8_t frame_info[4];
    res = app_src_play_pipe_raw_data_read(pipe_mgr, frame_info, 4);

    if (res == SRC_PIPE_MGR_SUCCESS)
    {
        uint16_t fill_len = (uint16_t)(frame_info[0] | (frame_info[1] << 8));
        uint8_t fill_frame_num = (frame_info[2] | (frame_info[3] << 8));
        APP_PRINT_INFO3("app_src_play_pipe_fill_data: pipe type %d fill_len %d fill_frame_num %d",
                        pipe_mgr->pipe_type, fill_len,
                        fill_frame_num);
        uint8_t *p_read_data_buf = malloc(fill_len);

        if (p_read_data_buf)
        {
            res = app_src_play_pipe_raw_data_read(pipe_mgr, p_read_data_buf, fill_len);
            if (res == SRC_PIPE_MGR_SUCCESS)
            {
                pipe_mgr->get_data_empty = false;

                if (!audio_pipe_fill(pipe_mgr->handle,
                                     0,
                                     pipe_mgr->fill_seq,
                                     AUDIO_STREAM_STATUS_CORRECT,
                                     fill_frame_num,
                                     (void *)p_read_data_buf,
                                     fill_len))
                {
                    res = SRC_PIPE_MGR_PIPE_FILL_ERROR;
                }
                else
                {
                    pipe_mgr->fill_seq++;
                    pipe_mgr->fill_state = SRC_PIPE_STATE_FILLING;
                }
            }
            else
            {
                pipe_mgr->get_data_empty = true;
            }
            free(p_read_data_buf);
        }
        else
        {
            res = SRC_PIPE_MGR_MEM_ERROR;
        }

    }
    else
    {
        pipe_mgr->get_data_empty = true;
    }
    return res;
}

bool app_src_play_pipe_handle_a2dp_stream_data(uint8_t *p_data, uint16_t data_len,
                                               uint8_t frame_number, uint16_t seq_num)
{
    if (flag_a2dp_direct_send)
    {
        app_src_play_a2dp_handle_data(p_data, data_len, frame_number);

#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
        if (app_src_play_is_local_play_attached())
        {
            if (app_src_play_get_play_route() == PLAY_ROUTE_A2DP && frame_number == 1)
            {
                /*
                    Handle data in app_src_play_a2dp_handle_data().
                    For DSPs that don't support specifying encode SBC frame number,
                    the APP needs to package first. In order to save RAM resources,
                    reusing the ring buffer in A2DP is necessary.
                */
            }
            else
            {
                app_src_play_attach_local_play_handle_data(p_data,
                                                           data_len,
                                                           seq_num,
                                                           frame_number,
                                                           0);
            }
        }
#endif
    }

    if (play_downstream_pipe_mgr.handle)
    {
        if (play_downstream_pipe_mgr.play_state == SRC_PIPE_PLAY_STATE_START)
        {
            play_downstream_pipe_mgr.fill_frame_num = frame_number;
            play_downstream_pipe_mgr.fill_len = data_len;

            if (app_src_play_pipe_raw_data_write(&play_downstream_pipe_mgr, p_data, data_len,
                                                 frame_number) == SRC_PIPE_MGR_SUCCESS)
            {
                if (play_downstream_pipe_mgr.get_data_empty)
                {
                    app_src_play_pipe_fill_data(&play_downstream_pipe_mgr);
                }
            }
        }
    }
    return true;
}

bool app_src_play_pipe_handle_sco_stream_data(uint8_t *p_data, uint16_t data_len,
                                              uint8_t bd_addr[6], bool is_hfp_ag_data)
{
    if (flag_hfp_direct_send)
    {
        app_src_play_hfp_send_sco(p_data, data_len, bd_addr);
    }
    else
    {
        if (is_hfp_ag_data)
        {
            if (play_downstream_pipe_mgr.handle)
            {
                if (play_downstream_pipe_mgr.play_state == SRC_PIPE_PLAY_STATE_START)
                {
                    play_downstream_pipe_mgr.fill_frame_num = 1;
                    play_downstream_pipe_mgr.fill_len = data_len;

                    if (app_src_play_pipe_raw_data_write(&play_downstream_pipe_mgr, p_data, data_len,
                                                         1) == SRC_PIPE_MGR_SUCCESS)
                    {
                        if (play_downstream_pipe_mgr.get_data_empty)
                        {
                            app_src_play_pipe_fill_data(&play_downstream_pipe_mgr);
                        }
                    }
                }
            }
        }
        else
        {
            if (play_upstream_pipe_mgr.handle)
            {
                if (play_upstream_pipe_mgr.play_state == SRC_PIPE_PLAY_STATE_START)
                {
                    play_upstream_pipe_mgr.fill_frame_num = 1;
                    play_upstream_pipe_mgr.fill_len = data_len;

                    if (app_src_play_pipe_raw_data_write(&play_upstream_pipe_mgr, p_data, data_len,
                                                         1) == SRC_PIPE_MGR_SUCCESS)
                    {
                        if (play_upstream_pipe_mgr.get_data_empty)
                        {
                            app_src_play_pipe_fill_data(&play_upstream_pipe_mgr);
                        }
                    }
                }
            }
        }
    }

    return true;
}

static void app_src_play_pipe_handle_data_ind(SRC_PLAY_PIPE_MGR *pipe_mgr)
{
    uint16_t len = 0;
    uint32_t timestamp = 0;
    uint16_t seq = 0;
    T_AUDIO_STREAM_STATUS status;
    uint8_t frame_num = 0;
    uint8_t src_route;
    uint8_t send_bd_addr[6];

    bool ret = audio_pipe_drain(pipe_mgr->handle,
                                &timestamp,
                                &seq,
                                &status,
                                &frame_num,
                                pipe_mgr->p_drain_buf,
                                &len);
    if (!ret)
    {
        APP_PRINT_ERROR0("app_src_play_pipe_handle_data_ind: drain failed");
        return;
    }

    src_route = app_src_play_get_src_route();

    if (pipe_mgr->pipe_type == SRC_PIPE_DOWNSTREAM_PLAY_TYPE)
    {
        if (src_route == PLAY_ROUTE_A2DP)
        {
            ret = app_src_play_a2dp_handle_data(pipe_mgr->p_drain_buf, len, frame_num);
        }
        else if (src_route == SOURCE_ROUTE_HFP_HF)
        {
            memcpy(send_bd_addr, hfp_play.ag_addr, 6);
            app_src_play_hfp_send_sco(pipe_mgr->p_drain_buf, len, send_bd_addr);
        }
        else if (src_route == PLAY_ROUTE_BIS || src_route == PLAY_ROUTE_CIS)
        {
            //TODO:
        }
    }
    else if (pipe_mgr->pipe_type == SRC_PIPE_UPSTREAM_PLAY_TYPE)
    {
        if (src_route == SOURCE_ROUTE_HFP_HF)
        {
            memcpy(send_bd_addr, hfp_play.hf_addr, 6);
            app_src_play_hfp_send_sco(pipe_mgr->p_drain_buf, len, send_bd_addr);
        }
    }
}

static bool app_src_play_downstream_pipe_cback(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT event,
                                               uint32_t param)
{
    APP_PRINT_INFO1("app_src_play_downstream_pipe_cback: event 0x%x", event);

    switch (event)
    {
    case AUDIO_PIPE_EVENT_CREATED:
        {
            uint32_t snk_buf_size = 1024;

            APP_PRINT_TRACE1("app_src_play_downstream_pipe_cback: snk_buf_size %d", snk_buf_size);
            audio_pipe_start(handle);

            if (play_downstream_pipe_mgr.p_drain_buf == NULL)
            {
                play_downstream_pipe_mgr.p_drain_buf = malloc(snk_buf_size);
            }
            play_downstream_pipe_mgr.play_state = SRC_PIPE_PLAY_STATE_START;
        }
        break;

    case AUDIO_PIPE_EVENT_STARTED:
        {
            play_downstream_pipe_mgr.fill_state = SRC_PIPE_STATE_STARTED;
            app_src_play_pipe_fill_data(&play_downstream_pipe_mgr);
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_IND:
        {
            if (play_downstream_pipe_mgr.play_state == SRC_PIPE_PLAY_STATE_START)
            {
                app_src_play_pipe_handle_data_ind(&play_downstream_pipe_mgr);
            }
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_FILLED:
        {
            if (play_downstream_pipe_mgr.play_state == SRC_PIPE_PLAY_STATE_START)
            {
                play_downstream_pipe_mgr.fill_state = SRC_PIPE_STATE_FILLED;
                app_src_play_pipe_fill_data(&play_downstream_pipe_mgr);
            }
        }
        break;

    case AUDIO_PIPE_EVENT_RELEASED:
        {
            play_downstream_pipe_mgr.fill_seq = 0;
            flag_a2dp_direct_send = false;
            flag_hfp_direct_send = false;

            if (play_downstream_pipe_mgr.p_drain_buf != NULL)
            {
                free(play_downstream_pipe_mgr.p_drain_buf);
                play_downstream_pipe_mgr.p_drain_buf = NULL;
            }
            play_downstream_pipe_mgr.handle = NULL;
        }
        break;

    default:
        break;
    }
    return true;
}

static bool app_src_play_upstream_pipe_cback(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT event,
                                             uint32_t param)
{
    APP_PRINT_INFO1("app_src_play_upstream_pipe_cback: event 0x%x", event);

    switch (event)
    {
    case AUDIO_PIPE_EVENT_CREATED:
        {
            uint32_t snk_buf_size = 1024;

            APP_PRINT_TRACE1("app_src_play_upstream_pipe_cback: snk_buf_size %d", snk_buf_size);
            audio_pipe_start(handle);

            if (play_upstream_pipe_mgr.p_drain_buf == NULL)
            {
                play_upstream_pipe_mgr.p_drain_buf = malloc(snk_buf_size);
            }
            play_upstream_pipe_mgr.play_state = SRC_PIPE_PLAY_STATE_START;
        }
        break;

    case AUDIO_PIPE_EVENT_STARTED:
        {
            play_upstream_pipe_mgr.fill_state = SRC_PIPE_STATE_STARTED;
            app_src_play_pipe_fill_data(&play_upstream_pipe_mgr);
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_IND:
        {
            if (play_upstream_pipe_mgr.play_state == SRC_PIPE_PLAY_STATE_START)
            {
                app_src_play_pipe_handle_data_ind(&play_upstream_pipe_mgr);
            }
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_FILLED:
        {
            if (play_upstream_pipe_mgr.play_state == SRC_PIPE_PLAY_STATE_START)
            {
                play_upstream_pipe_mgr.fill_state = SRC_PIPE_STATE_FILLED;
                app_src_play_pipe_fill_data(&play_upstream_pipe_mgr);
            }
        }
        break;

    case AUDIO_PIPE_EVENT_RELEASED:
        {
            play_upstream_pipe_mgr.fill_seq = 0;
            flag_a2dp_direct_send = false;
            flag_hfp_direct_send = false;

            if (play_upstream_pipe_mgr.p_drain_buf != NULL)
            {
                free(play_upstream_pipe_mgr.p_drain_buf);
                play_upstream_pipe_mgr.p_drain_buf = NULL;
            }
            play_upstream_pipe_mgr.handle = NULL;
        }
        break;

    default:
        break;
    }
    return true;
}

/* snk + src pipe encode */
bool app_src_play_downstream_pipe_start(uint8_t src_route, uint8_t play_route)
{
    uint8_t err_code = 0;
    T_AUDIO_FORMAT_INFO src_info = {};
    T_AUDIO_FORMAT_INFO snk_info = {};

    if (src_route == SOURCE_ROUTE_A2DP)
    {
        src_info = a2dp_play.src_a2dp_format;
        snk_info = a2dp_play.sink_a2dp_format[0];
        app_src_play_print_a2dp_format("app_src_play_downstream_pipe_start: src",
                                       &a2dp_play.src_a2dp_format);
        app_src_play_print_a2dp_format("app_src_play_downstream_pipe_start: snk",
                                       &a2dp_play.sink_a2dp_format[0]);
        if (play_route == PLAY_ROUTE_A2DP || play_route == PLAY_ROUTE_BIS
            || play_route == PLAY_ROUTE_CIS)
        {
            if (play_route == PLAY_ROUTE_A2DP)
            {
                if (!app_src_play_get_a2dp_format(&snk_info))
                {
                    APP_PRINT_ERROR0("app_src_play_downstream_pipe_start: a2dp format info does not exist");
                    return false;
                }
                if (!memcmp(&a2dp_play.src_a2dp_format, &a2dp_play.sink_a2dp_format[0],
                            sizeof(T_AUDIO_FORMAT_INFO)))
                {
                    flag_a2dp_direct_send = true;
                    return true;
                }
                else
                {
                    APP_PRINT_ERROR0("app_src_play_downstream_pipe_start: inconsistent format!!!");
                    return false;
                }
            }
            else if (play_route == PLAY_ROUTE_BIS || play_route == PLAY_ROUTE_CIS)
            {
                //TODO:bis cis format get
            }

            src_info.frame_num = 1;
            snk_info.frame_num = 1;
        }
    }
    else if (src_route == SOURCE_ROUTE_HFP_HF)
    {
        src_info = hfp_play.hfp_ag_format;
        snk_info = hfp_play.hfp_hf_format;
        app_src_play_print_hfp_format("app_src_play_downstream_pipe_start hfp ag: ",
                                      &hfp_play.hfp_ag_format);
        app_src_play_print_hfp_format("app_src_play_downstream_pipe_start hfp hf: ",
                                      &hfp_play.hfp_hf_format);
        if (play_route == PLAY_ROUTE_HFP_AG)
        {
            if (!app_src_play_get_hfp_format(&snk_info))
            {
                APP_PRINT_ERROR0("app_src_play_downstream_pipe_start: hfp format info does not exist");
                return false;
            }
            if (!memcmp(&hfp_play.hfp_ag_format, &hfp_play.hfp_hf_format, sizeof(T_AUDIO_FORMAT_INFO)))
            {
                flag_hfp_direct_send = true;
                return true;
            }
        }
    }
    else
    {
        err_code = 1;
        goto ERR;
    }

    if (play_downstream_pipe_mgr.handle != NULL)
    {
        err_code = 2;
        goto ERR;
    }
    else
    {
        play_downstream_pipe_mgr.handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL,
                                                            src_info, snk_info,
                                                            play_downstream_pipe_mgr.volume,
                                                            app_src_play_downstream_pipe_cback);
        play_downstream_pipe_mgr.fill_state = SRC_PIPE_STATE_CREATED;
    }
    return true;
ERR:
    APP_PRINT_ERROR1("app_src_play_downstream_pipe_start: err_code %d", -err_code);
    return false;
}

bool app_src_play_upstream_pipe_start(uint8_t src_route, uint8_t play_route)
{
    uint8_t err_code = 0;
    T_AUDIO_FORMAT_INFO src_info = {};
    T_AUDIO_FORMAT_INFO snk_info = {};

    if (src_route == SOURCE_ROUTE_HFP_HF)
    {
        app_src_play_print_hfp_format("app_src_play_upstream_pipe_start hfp ag: ",
                                      &hfp_play.hfp_ag_format);
        app_src_play_print_hfp_format("app_src_play_upstream_pipe_start hfp hf: ",
                                      &hfp_play.hfp_hf_format);
        if (play_route == PLAY_ROUTE_HFP_AG)
        {
            if (!app_src_play_get_hfp_format(&snk_info))
            {
                APP_PRINT_ERROR0("app_src_play_upstream_pipe_start: hfp format info does not exist");
                return false;
            }
            if (!memcmp(&hfp_play.hfp_ag_format, &hfp_play.hfp_hf_format, sizeof(T_AUDIO_FORMAT_INFO)))
            {
                flag_hfp_direct_send = true;
                return true;
            }
        }
        src_info = hfp_play.hfp_hf_format;
        snk_info = hfp_play.hfp_ag_format;
    }
    else
    {
        err_code = 1;
        goto ERR;
    }

    if (play_upstream_pipe_mgr.handle != NULL)
    {
        err_code = 2;
        goto ERR;
    }
    else
    {
        play_upstream_pipe_mgr.handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL,
                                                          src_info, snk_info,
                                                          play_upstream_pipe_mgr.volume,
                                                          app_src_play_upstream_pipe_cback);
        play_upstream_pipe_mgr.fill_state = SRC_PIPE_STATE_CREATED;
    }
    return true;
ERR:
    APP_PRINT_ERROR1("app_src_play_upstream_pipe_start: err_code %d", -err_code);
    return false;
}

void app_src_play_pipe_stop(void)
{
    if (play_downstream_pipe_mgr.handle != NULL)
    {
        audio_pipe_release(play_downstream_pipe_mgr.handle);
    }
    if (play_upstream_pipe_mgr.handle != NULL)
    {
        audio_pipe_release(play_upstream_pipe_mgr.handle);
    }
    app_src_play_pipe_raw_data_clear();
}

static void app_src_play_pipe_buf_init(void)
{
    if (play_downstream_pipe_mgr.p_buf == NULL)
    {
        play_downstream_pipe_mgr.p_buf = calloc(1, A2DP_FRAME_BUF);
        ring_buffer_init(&play_downstream_pipe_mgr.ring_buf, play_downstream_pipe_mgr.p_buf,
                         A2DP_FRAME_BUF);
    }
    if (play_upstream_pipe_mgr.p_buf == NULL)
    {
        play_upstream_pipe_mgr.p_buf = calloc(1, A2DP_FRAME_BUF);
        ring_buffer_init(&play_upstream_pipe_mgr.ring_buf, play_upstream_pipe_mgr.p_buf, A2DP_FRAME_BUF);
    }
}

void app_src_play_pipe_init(void)
{
    app_src_play_pipe_buf_init();
}

#endif
#endif
