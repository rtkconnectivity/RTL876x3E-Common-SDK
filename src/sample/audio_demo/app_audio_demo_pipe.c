/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "console.h"
#include "audio_type.h"
#include "audio_pipe.h"
#include "app_audio_demo_pipe.h"
#include "app_audio_data.h"

#define SBC_FRAME_LEN  81
#define MSBC_FRAME_LEN 57
#define LC3_FRAME_LEN  120
#define PCM_FRAME_LEN  480

static T_AUDIO_PIPE_HANDLE audio_pipe_handle = NULL;
static uint8_t *p_audio_pipe_buf             = NULL;
static uint8_t  app_src_type;
static uint16_t seq_num                      = 0;
static uint32_t data_offset                  = 0;

void app_audio_pipe_fill_sbc_data(void)
{
    if (data_offset + SBC_FRAME_LEN <= sbc_data_len)
    {
        if (audio_pipe_fill(audio_pipe_handle, 0, seq_num, AUDIO_STREAM_STATUS_CORRECT, 1,
                            (void *)(sbc_data + data_offset), SBC_FRAME_LEN) == true)
        {
            seq_num++;
            data_offset += SBC_FRAME_LEN;
        }
    }
    else
    {
        data_offset = 0;
        app_audio_pipe_fill_sbc_data();
    }
}

void app_audio_pipe_fill_msbc_data(void)
{
    if (data_offset + MSBC_FRAME_LEN <= msbc_data_len)
    {
        if (audio_pipe_fill(audio_pipe_handle, 0, seq_num, AUDIO_STREAM_STATUS_CORRECT, 1,
                            (void *)(msbc_data + data_offset), MSBC_FRAME_LEN) == true)
        {
            seq_num++;
            data_offset += MSBC_FRAME_LEN;
        }
    }
    else
    {
        data_offset = 0;
        app_audio_pipe_fill_msbc_data();
    }
}

void app_audio_pipe_fill_lc3_data(void)
{
    if (data_offset + LC3_FRAME_LEN <= lc3_data_len)
    {
        if (audio_pipe_fill(audio_pipe_handle, 0, seq_num, AUDIO_STREAM_STATUS_CORRECT, 1,
                            (void *)(lc3_data + data_offset), LC3_FRAME_LEN) == true)
        {
            seq_num++;
            data_offset += LC3_FRAME_LEN;
        }
    }
    else
    {
        data_offset = 0;
        app_audio_pipe_fill_lc3_data();
    }
}

void app_audio_pipe_fill_pcm_data(void)
{
    if (data_offset + PCM_FRAME_LEN <= pcm_data_len)
    {
        if (audio_pipe_fill(audio_pipe_handle, 0, seq_num, AUDIO_STREAM_STATUS_CORRECT, 1,
                            (void *)(pcm_data + data_offset), PCM_FRAME_LEN) == true)
        {
            seq_num++;
            data_offset += PCM_FRAME_LEN;
        }
    }
    else
    {
        data_offset = 0;
        app_audio_pipe_fill_pcm_data();
    }
}

void app_audio_pipe_fill_data(void)
{
    switch (app_src_type)
    {
    case AUDIO_FORMAT_TYPE_PCM:
        {
            app_audio_pipe_fill_pcm_data();
        }
        break;

    case AUDIO_FORMAT_TYPE_MSBC:
        {
            app_audio_pipe_fill_msbc_data();
        }
        break;

    case AUDIO_FORMAT_TYPE_SBC:
        {
            app_audio_pipe_fill_sbc_data();
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            app_audio_pipe_fill_lc3_data();
        }
        break;

    default:
        break;
    }
}

bool app_audio_pipe_callback(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT event,
                             uint32_t param)
{
    switch (event)
    {
    case AUDIO_PIPE_EVENT_CREATED:
        {
            uint32_t snk_buf_size = param;

            if (p_audio_pipe_buf == NULL)
            {
                p_audio_pipe_buf = os_mem_alloc2(snk_buf_size);
                if (p_audio_pipe_buf == NULL)
                {
                    return false;
                }
            }

            seq_num = 0;

            char *temp_buff = "Audio Pipe Created!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case AUDIO_PIPE_EVENT_STARTED:
        {
            char *temp_buff = "Audio Pipe Started!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));

            app_audio_pipe_fill_data();
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_IND:
        {
            uint32_t              timestamp;
            uint16_t              seq;
            T_AUDIO_STREAM_STATUS status;
            uint8_t               frame_num;
            uint16_t              data_len;

            audio_pipe_drain(audio_pipe_handle,
                             &timestamp,
                             &seq,
                             &status,
                             &frame_num,
                             p_audio_pipe_buf,
                             &data_len);
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_FILLED:
        {
            app_audio_pipe_fill_data();
        }
        break;

    case AUDIO_PIPE_EVENT_STOPPED:
        {
            char *temp_buff = "Audio Pipe Stopped!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case AUDIO_PIPE_EVENT_RELEASED:
        {
            if (p_audio_pipe_buf != NULL)
            {
                os_mem_free(p_audio_pipe_buf);
                p_audio_pipe_buf = NULL;
            }

            char *temp_buff = "Audio Pipe Released!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    default:
        break;
    }

    return true;
}

bool app_audio_pipe_create(uint8_t src_type, uint8_t snk_type)
{
    bool param_check = true;

    if (audio_pipe_handle == NULL)
    {
        T_AUDIO_FORMAT_INFO src_info;
        T_AUDIO_FORMAT_INFO snk_info;

        src_info.type = (T_AUDIO_FORMAT_TYPE)src_type;
        switch (src_type)
        {
        case AUDIO_FORMAT_TYPE_PCM:
            {
                src_info.frame_num = 1;
                src_info.attr.pcm.sample_rate = 48000;
                src_info.attr.pcm.frame_length = 240;
                src_info.attr.pcm.chann_num = 2;
                src_info.attr.pcm.bit_width = 16;
            }
            break;

        case AUDIO_FORMAT_TYPE_MSBC:
            {
                src_info.frame_num = 1;
                src_info.attr.msbc.sample_rate = 16000;
                src_info.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
                src_info.attr.msbc.block_length = 15;
                src_info.attr.msbc.subband_num = 8;
                src_info.attr.msbc.allocation_method = 0;
                src_info.attr.msbc.bitpool = 26;
            }
            break;

        case AUDIO_FORMAT_TYPE_LC3:
            {
                src_info.frame_num = 1;
                src_info.attr.lc3.sample_rate = 48000;
                src_info.attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO;
                src_info.attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
                src_info.attr.lc3.frame_length = 120;
                src_info.attr.lc3.presentation_delay = 0;
            }
            break;

        case AUDIO_FORMAT_TYPE_SBC:
            {
                src_info.frame_num = 1;
                src_info.attr.sbc.sample_rate = 48000;
                src_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
                src_info.attr.sbc.block_length = 16;
                src_info.attr.sbc.subband_num = 8;
                src_info.attr.sbc.allocation_method = 0;
                src_info.attr.sbc.bitpool = 0x22;
            }
            break;

        default:
            param_check = false;
            break;
        }

        snk_info.type = (T_AUDIO_FORMAT_TYPE)snk_type;
        switch (snk_type)
        {
        case AUDIO_FORMAT_TYPE_PCM:
            {
                snk_info.frame_num = 1;
                snk_info.attr.pcm.sample_rate = 48000;
                snk_info.attr.pcm.frame_length = 240;
                snk_info.attr.pcm.chann_num = 2;
                snk_info.attr.pcm.bit_width = 16;
            }
            break;

        case AUDIO_FORMAT_TYPE_MSBC:
            {
                snk_info.frame_num = 1;
                snk_info.attr.msbc.sample_rate = 16000;
                snk_info.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
                snk_info.attr.msbc.block_length = 15;
                snk_info.attr.msbc.subband_num = 8;
                snk_info.attr.msbc.allocation_method = 0;
                snk_info.attr.msbc.bitpool = 26;
            }
            break;

        case AUDIO_FORMAT_TYPE_LC3:
            {
                snk_info.frame_num = 1;
                snk_info.attr.lc3.sample_rate = 48000;
                snk_info.attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO;
                snk_info.attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
                snk_info.attr.lc3.frame_length = 120;
                snk_info.attr.lc3.presentation_delay = 0;
            }
            break;

        case AUDIO_FORMAT_TYPE_SBC:
            {
                snk_info.frame_num = 1;
                snk_info.attr.sbc.subband_num = 8;
                snk_info.attr.sbc.bitpool = 0x22;
                snk_info.attr.sbc.sample_rate = 48000;
                snk_info.attr.sbc.block_length = 16;
                snk_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
                snk_info.attr.sbc.allocation_method = 0;
            }
            break;

        default:
            param_check = false;
            break;
        }

        if (param_check)
        {
            app_src_type = src_type;

            audio_pipe_handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL, src_info, snk_info,
                                                  0, app_audio_pipe_callback);
        }
        else
        {
            char *temp_buff = "Audio Pipe Codec Not Supported!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
    }
    else
    {
        char *temp_buff = "Audio Pipe Already Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_pipe_start(void)
{
    if (audio_pipe_handle != NULL)
    {
        if (audio_pipe_start(audio_pipe_handle) == false)
        {
            char *temp_buff = "Audio Pipe Start Failed!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
    }
    else
    {
        char *temp_buff = "Audio Pipe Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_pipe_stop(void)
{
    if (audio_pipe_handle != NULL)
    {
        if (audio_pipe_stop(audio_pipe_handle) == false)
        {
            char *temp_buff = "Audio Pipe Stop Failed!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
    }
    else
    {
        char *temp_buff = "Audio Pipe Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_pipe_release(void)
{
    if (audio_pipe_handle != NULL)
    {
        if (audio_pipe_release(audio_pipe_handle) == false)
        {
            char *temp_buff = "Audio Pipe Release Failed!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }

        audio_pipe_handle = NULL;
    }
    else
    {
        char *temp_buff = "Audio Pipe Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

