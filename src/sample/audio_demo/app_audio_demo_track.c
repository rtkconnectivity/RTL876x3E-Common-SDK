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
#include "audio_track.h"
#include "app_audio_data.h"
#include "app_audio_demo_track.h"
#include "app_audio_data.h"

#define MSBC_FRAME_LEN 57

static T_AUDIO_TRACK_HANDLE audio_track_handle = NULL;
static uint8_t  app_format_type;
static uint16_t seq_num = 0;
static uint32_t data_offset = 0;

void app_audio_track_write_sbc_data(void)
{
    uint16_t written_len;

    if (data_offset + 486 <= sbc_data_len)
    {
        if (audio_track_write(audio_track_handle,
                              0,
                              seq_num,
                              AUDIO_STREAM_STATUS_CORRECT,
                              6,
                              (uint8_t *)(sbc_data + data_offset),
                              486,
                              &written_len))
        {
            seq_num++;
            data_offset += 486;
        }
    }
    else
    {
        data_offset = 0;
        app_audio_track_write_sbc_data();
    }
}

void app_audio_track_write_msbc_data(void)
{
    uint16_t written_len;

    if (data_offset + MSBC_FRAME_LEN <= msbc_data_len)
    {
        if (audio_track_write(audio_track_handle,
                              0,
                              seq_num,
                              AUDIO_STREAM_STATUS_CORRECT,
                              1,
                              (uint8_t *)(msbc_data + data_offset),
                              MSBC_FRAME_LEN,
                              &written_len))
        {
            seq_num++;
            data_offset += MSBC_FRAME_LEN;
        }
    }
    else
    {
        data_offset = 0;
        app_audio_track_write_msbc_data();
    }
}

void app_audio_track_write(void)
{
    switch (app_format_type)
    {
    case AUDIO_FORMAT_TYPE_PCM:
        {
            //app_audio_pipe_fill_pcm_data();
        }
        break;

    case AUDIO_FORMAT_TYPE_MSBC:
        {
            app_audio_track_write_msbc_data();
        }
        break;

    case AUDIO_FORMAT_TYPE_SBC:
        {
            app_audio_track_write_sbc_data();
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            //app_audio_pipe_fill_lc3_data();
        }
        break;

    default:
        break;
    }
}

bool app_audio_track_create(uint8_t stream_type,
                            uint8_t format_type,
                            uint8_t volume_out,
                            uint8_t volume_in)
{
    uint32_t device = 0;
    bool     param_check = true;

    if (audio_track_handle == NULL)
    {
        T_AUDIO_FORMAT_INFO format = {};

        format.type = (T_AUDIO_FORMAT_TYPE)format_type;
        switch (format_type)
        {
        case AUDIO_FORMAT_TYPE_PCM:
            {
                format.attr.pcm.sample_rate = 48000;
                format.attr.pcm.frame_length = 240;
                format.attr.pcm.chann_num = 2;
                format.attr.pcm.bit_width = 16;
            }
            break;

        case AUDIO_FORMAT_TYPE_MSBC:
            {
                format.attr.msbc.sample_rate = 16000;
                format.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
                format.attr.msbc.block_length = 15;
                format.attr.msbc.subband_num = 8;
                format.attr.msbc.allocation_method = 0;
                format.attr.msbc.bitpool = 26;
            }
            break;

        case AUDIO_FORMAT_TYPE_LC3:
            {
                format.attr.lc3.sample_rate = 48000;
                format.attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_MONO;
                format.attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
                format.attr.lc3.frame_length = 120;
                format.attr.lc3.presentation_delay = 0;
            }
            break;

        case AUDIO_FORMAT_TYPE_SBC:
            {
                format.attr.sbc.sample_rate = 48000;
                format.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
                format.attr.sbc.block_length = 16;
                format.attr.sbc.subband_num = 8;
                format.attr.sbc.allocation_method = 0;
                format.attr.sbc.bitpool = 0x22;
            }
            break;

        default:
            param_check = false;
            break;
        }

        switch (stream_type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            device = AUDIO_DEVICE_OUT_SPK;
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            device = AUDIO_DEVICE_OUT_SPK | AUDIO_DEVICE_IN_MIC;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            device = AUDIO_DEVICE_IN_MIC;
            break;

        default:
            break;
        }

        if (param_check)
        {
            app_format_type = format_type;

            audio_track_handle = audio_track_create((T_AUDIO_STREAM_TYPE)stream_type,
                                                    AUDIO_STREAM_MODE_NORMAL,
                                                    AUDIO_STREAM_USAGE_LOCAL,
                                                    format,
                                                    volume_out,
                                                    volume_in,
                                                    device,
                                                    NULL,
                                                    NULL);
            audio_track_threshold_set(audio_track_handle, 330, 60);
            audio_track_latency_set(audio_track_handle, 100, false);
        }
        else
        {
            char *temp_buff = "Audio Track Format Not Supported!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
    }
    else
    {
        char *temp_buff = "Audio Track Already Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_track_start(void)
{
    if (audio_track_handle != NULL)
    {
        if (audio_track_start(audio_track_handle) == false)
        {
            char *temp_buff = "Audio Track Start Failed!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
    }
    else
    {
        char *temp_buff = "Audio Track Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_track_stop(void)
{
    if (audio_track_handle != NULL)
    {
        if (audio_track_stop(audio_track_handle) == false)
        {
            char *temp_buff = "Audio Track Start Failed!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
    }
    else
    {
        char *temp_buff = "Audio Track Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_track_pause(void)
{
    if (audio_track_handle != NULL)
    {
        if (audio_track_pause(audio_track_handle) == false)
        {
            char *temp_buff = "Audio Track Start Failed!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
    }
    else
    {
        char *temp_buff = "Audio Track Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_track_restart(void)
{
    if (audio_track_handle != NULL)
    {
        if (audio_track_restart(audio_track_handle) == false)
        {
            char *temp_buff = "Audio Track Start Failed!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
    }
    else
    {
        char *temp_buff = "Audio Track Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_track_release(void)
{
    if (audio_track_handle != NULL)
    {
        if (audio_track_release(audio_track_handle) == false)
        {
            char *temp_buff = "Audio Track Start Failed!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }

        audio_track_handle = NULL;
    }
    else
    {
        char *temp_buff = "Audio Track Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_track_volume_out_set(uint8_t volume_out)
{
    if (audio_track_handle != NULL)
    {
        audio_track_volume_out_set(audio_track_handle, volume_out);
    }
    else
    {
        char *temp_buff = "Audio Track Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_track_volume_out_mute(void)
{
    if (audio_track_handle != NULL)
    {
        audio_track_volume_out_mute(audio_track_handle);
    }
    else
    {
        char *temp_buff = "Audio Track Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}

bool app_audio_track_volume_out_unmute(void)
{
    if (audio_track_handle != NULL)
    {
        audio_track_volume_out_unmute(audio_track_handle);
    }
    else
    {
        char *temp_buff = "Audio Track Not Exist!\r\n";
        console_write((uint8_t *)temp_buff, strlen(temp_buff));
    }

    return true;
}
