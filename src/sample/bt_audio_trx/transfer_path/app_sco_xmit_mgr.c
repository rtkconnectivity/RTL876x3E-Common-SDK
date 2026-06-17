/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if (F_APP_SCO_XMIT_AG_SUPPORT || F_APP_SCO_XMIT_HF_SUPPORT)
#include "trace.h"
#include "btm.h"
#include "stdlib.h"
#include "os_msg.h"
#include "bt_hfp.h"
#include "app_link_util_cs.h"
#include "app_main.h"
#include "app_cmd.h"
#include "app_cfg.h"
#include "app_spi_api.h"
#include "app_sco_xmit_mgr.h"
#include "audio_pipe.h"
#include "ring_buffer.h"
#include "string.h"

//set to 0 when sco connected. from audio_track_read, currently manage by self.
/* Defines ------------------------------------------------------------------*/
#define SPI_RBUF_SIZE                   SPI_XMIT_SIZE * 2
/* Globals ------------------------------------------------------------------*/
static struct
{
    T_AUDIO_FORMAT_INFO             format_info_in;
    T_AUDIO_FORMAT_INFO             format_info_out;
    T_SCO_STATE                     connect_state;
    bool                            out_route_ready;
    bool                            in_route_ready;
    T_RING_BUFFER                   ring_buf;
    uint8_t                         *buffer;
} sco_ctrl;

static uint8_t sco_seq_num = 0;
static uint8_t seq_num = 0;
static uint8_t  cur_pair_idx = 0;

static bool flag_pipe_get_data_empty = false;
static bool flag_direct_send = false;

static void *p_drain_data_buf = NULL;
static T_AUDIO_PIPE_HANDLE audio_pipe_handle = NULL;

/* end of Globals------------------------------------------------------------------*/

T_SCO_STATE app_sco_xmit_get_state(void)
{
    return sco_ctrl.connect_state;
}

void app_sco_xmit_send_sco(uint8_t *data, uint16_t len)
{
    uint8_t active_hf_idx = app_hfp_get_active_idx();
    T_APP_BR_LINK *p_link;
    p_link = app_link_find_br_link(app_db.br_link[active_hf_idx].bd_addr);
    if (p_link == NULL)
    {
        APP_PRINT_ERROR0("app_sco_xmit_send_sco: no br link found");
        return;
    }

    if (data[0] != BT_SCO_PKT_STATUS_OK)
    {
        APP_PRINT_WARN1("app_sco_xmit_send_sco: invalid data! %d", data[0]);
        return;
    }

    sco_seq_num++;

    if (p_link->sco.duplicate_fst_data)
    {
        p_link->sco.duplicate_fst_data = false;
        bt_sco_data_send(p_link->bd_addr, sco_seq_num - 1, &data[1], len - 1);
    }
    bt_sco_data_send(p_link->bd_addr, sco_seq_num, &data[1], len - 1);
}

void app_sco_xmit_save_output_param(T_AUDIO_FORMAT_INFO *format_info)
{
    if (!sco_ctrl.out_route_ready)
    {
        memcpy(&sco_ctrl.format_info_out, format_info, sizeof(T_AUDIO_FORMAT_INFO));
        if (sco_ctrl.format_info_out.type == AUDIO_FORMAT_TYPE_MSBC)
        {
            APP_PRINT_INFO7("app_sco_xmit_save_output_param: type %d, "
                            "sample_rate %d, allocation_method %d, bitpool %d, block_length %d, chann_mode %d, subband_num %d",
                            sco_ctrl.format_info_out.type,
                            sco_ctrl.format_info_out.attr.msbc.sample_rate,
                            sco_ctrl.format_info_out.attr.msbc.allocation_method,
                            sco_ctrl.format_info_out.attr.msbc.bitpool,
                            sco_ctrl.format_info_out.attr.msbc.block_length,
                            sco_ctrl.format_info_out.attr.msbc.chann_mode,
                            sco_ctrl.format_info_out.attr.msbc.subband_num);
        }
        else if (sco_ctrl.format_info_out.type == AUDIO_FORMAT_TYPE_CVSD)
        {
            APP_PRINT_INFO4("app_sco_xmit_save_output_param: type %d, "
                            "sample_rate %d, chann_num %d, frame_duration %d",
                            sco_ctrl.format_info_out.type,
                            sco_ctrl.format_info_out.attr.cvsd.sample_rate,
                            sco_ctrl.format_info_out.attr.cvsd.chann_num,
                            sco_ctrl.format_info_out.attr.cvsd.frame_duration);
        }
    }
    sco_ctrl.out_route_ready = true;
}

void app_sco_xmit_handle_sco_param(uint8_t *param, uint16_t param_len)
{
    if (!sco_ctrl.in_route_ready)
    {
        memcpy(&sco_ctrl.format_info_in, param, param_len);
        if (sco_ctrl.format_info_in.type == AUDIO_FORMAT_TYPE_MSBC)
        {
            APP_PRINT_INFO7("app_sco_xmit_handle_sco_param: type %d, "
                            "sample_rate %d, allocation_method %d, bitpool %d, block_length %d, chann_mode %d, subband_num %d",
                            sco_ctrl.format_info_in.type,
                            sco_ctrl.format_info_in.attr.msbc.sample_rate,
                            sco_ctrl.format_info_in.attr.msbc.allocation_method,
                            sco_ctrl.format_info_in.attr.msbc.bitpool,
                            sco_ctrl.format_info_in.attr.msbc.block_length,
                            sco_ctrl.format_info_in.attr.msbc.chann_mode,
                            sco_ctrl.format_info_in.attr.msbc.subband_num);
        }
        else if (sco_ctrl.format_info_in.type == AUDIO_FORMAT_TYPE_CVSD)
        {
            APP_PRINT_INFO4("app_sco_xmit_handle_sco_param: type %d, "
                            "sample_rate %d, chann_num %d, frame_duration %d",
                            sco_ctrl.format_info_in.type,
                            sco_ctrl.format_info_in.attr.cvsd.sample_rate,
                            sco_ctrl.format_info_in.attr.cvsd.chann_num,
                            sco_ctrl.format_info_in.attr.cvsd.frame_duration);
        }
        sco_ctrl.in_route_ready = true;
    }
#if F_APP_SCO_XMIT_HF_SUPPORT
    app_sco_xmit_param_recfg();
#endif
}

static uint16_t app_sco_xmit_pipe_data_ind(void)
{
    uint16_t res = SCO_XMIT_SUCCESS;
    uint16_t data_len = 0;
    uint32_t timestamp = 0;
    uint16_t seq = 0;
    T_AUDIO_STREAM_STATUS status;

    uint8_t active_hf_idx = app_hfp_get_active_idx();
    T_APP_BR_LINK *p_link;
    p_link = app_link_find_br_link(app_db.br_link[active_hf_idx].bd_addr);

    audio_pipe_drain(audio_pipe_handle,
                     &timestamp,
                     &seq,
                     &status,
                     &sco_seq_num,
                     p_drain_data_buf,
                     &data_len);
    if (data_len == 0)
    {
        res = SCO_XMIT_PIPE_DRAIN_ERROR;
    }
    else
    {
        sco_seq_num++;
        if (p_link->sco.duplicate_fst_data)
        {
            p_link->sco.duplicate_fst_data = false;
            bt_sco_data_send(p_link->bd_addr, sco_seq_num - 1, p_drain_data_buf, data_len);
        }
        if (!bt_sco_data_send(p_link->bd_addr, sco_seq_num, p_drain_data_buf, data_len))
        {
            res = SCO_XMIT_DATA_SEND_ERROR;
        }
    }
    APP_PRINT_INFO1("app_sco_xmit_pipe_data_ind: res 0x%x", res);
    return res;
}

uint16_t app_sco_xmit_peak_data(uint8_t *rd_buf, uint32_t len, uint32_t *real_len)
{
    uint16_t ret = SCO_XMIT_SUCCESS;
    uint32_t space_size = 0;
    uint32_t used_size = 0;

    space_size = ring_buffer_get_remaining_space(&sco_ctrl.ring_buf);
    used_size = ring_buffer_get_data_count(&sco_ctrl.ring_buf);
    APP_PRINT_TRACE3("app_sco_xmit_peak_data: used_size %d, space_size %d, len %d",
                     used_size, space_size, len);
    if (len > used_size)
    {
        return SCO_XMIT_READ_ERROR;
    }
    *real_len = ring_buffer_peek(&sco_ctrl.ring_buf, len, rd_buf);
    return ret;
}

uint16_t app_sco_xmit_read_data(uint8_t *rd_buf, uint32_t len)
{
    uint16_t ret = SCO_XMIT_SUCCESS;
    uint32_t used_size = 0;

    used_size = ring_buffer_get_data_count(&sco_ctrl.ring_buf);
    APP_PRINT_INFO3("app_sco_xmit_read_data: used_size %d, len %d, flag %d", used_size, len,
                    flag_pipe_get_data_empty);
    if (len > used_size)
    {
        flag_pipe_get_data_empty = true;
        return SCO_XMIT_READ_ERROR;
    }
    else
    {
        flag_pipe_get_data_empty = false;
    }
    ring_buffer_read(&sco_ctrl.ring_buf, len, rd_buf);
    return ret;
}

static uint16_t app_sco_xmit_fill_data(void)
{
    uint16_t res = SCO_XMIT_SUCCESS;
    uint8_t frame_info[2];
    res = app_sco_xmit_read_data(frame_info, 2);
    APP_PRINT_INFO1("app_sco_xmit_fill_data: res %d", res);
    if (!res)
    {
        uint16_t audio_len = (uint16_t)(frame_info[0] | (frame_info[1] << 8));
        uint8_t *p_read_data_buf = malloc(audio_len);
        if (p_read_data_buf)
        {
            res = app_sco_xmit_read_data(p_read_data_buf, audio_len);
            if (!res)
            {
                APP_PRINT_INFO2("app_sco_xmit_fill_data: seq_num %d, audio_len %d", seq_num, audio_len);

                if (!audio_pipe_fill(audio_pipe_handle, 0, seq_num, AUDIO_STREAM_STATUS_CORRECT, 0,
                                     (void *)p_read_data_buf, audio_len))
                {
                    res = SCO_XMIT_PIPE_FILL_ERROR;
                }
                else
                {
                    seq_num++;
                }
            }
            free(p_read_data_buf);
        }
        else
        {
            res = SCO_XMIT_MEM_ERROR;
        }
    }
    return res;
}

static void app_sco_xmit_write_data(uint8_t *data, uint16_t len)
{
    if (sco_ctrl.connect_state != SCO_STATE_CONNECTED)
    {
        APP_PRINT_ERROR1("app_sco_xmit_write_data: connect_state %d err", sco_ctrl.connect_state);
        return;
    }

    uint32_t space_size = 0;
    uint32_t used_size = 0;

    space_size = ring_buffer_get_remaining_space(&sco_ctrl.ring_buf);
    used_size = ring_buffer_get_data_count(&sco_ctrl.ring_buf);

    if (space_size < len + 2)
    {
        APP_PRINT_WARN2("app_sco_xmit_write_data: space_size %d, used_size:%d", space_size, used_size);
        return;
    }
    ring_buffer_write(&sco_ctrl.ring_buf, (uint8_t *)&len, 2);
    ring_buffer_write(&sco_ctrl.ring_buf, data, len);
    if (flag_pipe_get_data_empty)
    {
        app_sco_xmit_fill_data();
    }
}

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
            sco_ctrl.connect_state = SCO_STATE_CONNECTED;
        }
        break;

    case AUDIO_PIPE_EVENT_STARTED: // send first pkt data to dsp
        {
            app_sco_xmit_fill_data();
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_IND: // get encode data to buf_pool
        {
            if (sco_ctrl.connect_state == SCO_STATE_CONNECTED)
            {
                app_sco_xmit_pipe_data_ind();
            }
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_FILLED:  // dsp buf low, need put data to share memory
        {
            if (sco_ctrl.connect_state == SCO_STATE_CONNECTED)
            {
                app_sco_xmit_fill_data();
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
        }
        break;

    default:
        break;
    }
    return true;
}

uint8_t app_sco_xmit_param_recfg(void)
{
    uint8_t res = SCO_XMIT_SUCCESS;

    if (audio_pipe_handle != NULL)
    {
        res = SCO_XMIT_PIPE_CREATE_ERROR;
        return res;
    }
    APP_PRINT_INFO2("app_sco_xmit_param_recfg: out_route_ready %d, in_route_ready %d",
                    sco_ctrl.out_route_ready, sco_ctrl.in_route_ready);
    if (sco_ctrl.out_route_ready && sco_ctrl.in_route_ready)
    {
        T_AUDIO_FORMAT_INFO format_info_in = sco_ctrl.format_info_in;
        T_AUDIO_FORMAT_INFO format_info_out = sco_ctrl.format_info_out;

        APP_PRINT_INFO2("app_sco_xmit_param_recfg: format_info_in type %d, format_info_out type %d",
                        format_info_in.type, format_info_out.type);

        if (format_info_in.type == format_info_out.type)
        {
            flag_direct_send = true;
            return res;
        }
        else
        {
            if (audio_pipe_handle == NULL)
            {
                audio_pipe_handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL,
                                                      format_info_in, format_info_out,
                                                      app_cfg_nv.voice_gain_level[cur_pair_idx],
                                                      audio_pipe_callback);
            }
            else
            {
                APP_PRINT_INFO0("app_sco_xmit_param_recfg: audio_pipe_handle not empty");
            }
        }
    }
    else
    {
        res = SCO_XMIT_SYS_ERROR;
    }
    return res;
}

void app_sco_xmit_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                 uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE3("app_spi_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);

    switch (cmd_id)
    {
    case CMD_SCO_XMIT_CONFIG:
        {
            uint8_t *p_param = &cmd_ptr[2];
            uint16_t param_len = cmd_len - 2;
            app_sco_xmit_handle_sco_param(p_param, param_len);
        }
        break;

    case CMD_SCO_XMIT_AUDIO:
        {
            uint8_t *p_data = &cmd_ptr[2];
            uint16_t audio_len = cmd_len - 2;

            if (!sco_ctrl.in_route_ready)
            {
                APP_PRINT_ERROR0("NEED SCO FORMAT FROM REMOTE DEVICE!!!");
                app_report_event(CMD_PATH_SPI, CMD_SCO_XMIT_NEED_FORMAT, 0, NULL, 0);
                break;
            }
            if (flag_direct_send)
            {
                app_sco_xmit_send_sco(p_data, audio_len);
            }
            else
            {
                app_sco_xmit_write_data(p_data, audio_len);
            }
        }
        break;

    case CMD_SCO_XMIT_NEED_FORMAT:
        {
            app_report_event(CMD_PATH_SPI, CMD_SCO_XMIT_CONFIG, 0, (uint8_t *)&sco_ctrl.format_info_out,
                             sizeof(T_AUDIO_FORMAT_INFO));
        }
        break;

    default:
        break;
    }
}

void app_sco_xmit_init(void)
{
    memset(&sco_ctrl, 0, sizeof(sco_ctrl));
    sco_ctrl.buffer = (uint8_t *)calloc(1, SPI_RBUF_SIZE);
    ring_buffer_init(&sco_ctrl.ring_buf, sco_ctrl.buffer,
                     SPI_RBUF_SIZE);
}

#endif
