/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os_sched.h"
#include "trace.h"
#include "audio.h"
#include "audio_type.h"
#include "console.h"
#include "ringtone.h"
#include "voice_prompt.h"
#include "app_timer.h"
#include "app_audio_demo_track.h"
#include "app_audio_demo_policy.h"

#define VOICE_PROMPT_INDEX              0x80
#define TONE_INVALID_INDEX              0xFF
#define VOICE_PROMPT_LANGUAGE           0

#define AUDIO_TRACK_SBC_PLAY_INTERVAL   48

static uint8_t timer_idx_sbc_play = 0;
static uint8_t timer_idx_msbc_play = 0;
static uint8_t read_count = 0;

uint8_t audio_policy_timer_id = 0;

typedef enum t_app_audio_policy_timer
{
    APP_TIMER_AUDIO_DEMO_SBC_PLAY  = 0x00,
    APP_TIMER_AUDIO_DEMO_MSBC_PLAY = 0x01,
} T_APP_AUDIO_POLICY_TIMER;

static void app_audio_policy_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    T_AUDIO_STREAM_TYPE stream_type;

    switch (event_type)
    {
    case AUDIO_EVENT_TTS_STARTED:
        {

        }
        break;
    case AUDIO_EVENT_TTS_ALERTED:
        {

        }
        break;
    case AUDIO_EVENT_TTS_PAUSED:
        {

        }
        break;
    case AUDIO_EVENT_TTS_RESUMED:
        {

        }
        break;
    case AUDIO_EVENT_TTS_STOPPED:
        {

        }
        break;
    case AUDIO_EVENT_RINGTONE_STARTED:
        {
            char *temp_buff = "Ringtone played!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;
    case AUDIO_EVENT_RINGTONE_STOPPED:
        {

        }
        break;
    case AUDIO_EVENT_VOICE_PROMPT_STARTED:
        {
            char *temp_buff = "VP played!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;
    case AUDIO_EVENT_VOICE_PROMPT_STOPPED:
        {

        }
        break;
    case AUDIO_EVENT_PASSTHROUGH_ENABLED:
        {
            char *temp_buff = "Passthrough enabled!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;
    case AUDIO_EVENT_PASSTHROUGH_DISABLED:
        {
            char *temp_buff = "Passthrough disabled!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;
    case AUDIO_EVENT_LINE_IN_STARTED:
        {
            char *temp_buff = "Line in started!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;
    case AUDIO_EVENT_LINE_IN_STOPPED:
        {
            char *temp_buff = "Line in stopped!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;
    case AUDIO_EVENT_ANC_ENABLED:
        {
            char *temp_buff = "ANC enabled!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;
    case AUDIO_EVENT_ANC_DISABLED:
        {
            char *temp_buff = "ANC disabled!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            if (param->track_state_changed.state == AUDIO_TRACK_STATE_RELEASED)
            {
                char *temp_buff = "Audio Track Released!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));

                break;
            }

            if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == false)
            {
                break;
            }

            if (param->track_state_changed.state == AUDIO_TRACK_STATE_CREATED)
            {
                char *temp_buff = "Audio Track Created!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
            else if (param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED ||
                     param->track_state_changed.state == AUDIO_TRACK_STATE_RESTARTED)
            {
                char *temp_buff = "Audio Track Started!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));

                if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                {
                    app_start_timer(&timer_idx_sbc_play,
                                    "audio_demo_sbc_play",
                                    audio_policy_timer_id,
                                    APP_TIMER_AUDIO_DEMO_SBC_PLAY,
                                    0,
                                    true,
                                    AUDIO_TRACK_SBC_PLAY_INTERVAL);
                }
                else if (stream_type == AUDIO_STREAM_TYPE_VOICE)
                {
                    app_start_timer(&timer_idx_msbc_play,
                                    "audio_demo_msbc_play",
                                    audio_policy_timer_id,
                                    APP_TIMER_AUDIO_DEMO_MSBC_PLAY,
                                    0,
                                    false,
                                    AUDIO_TRACK_SBC_PLAY_INTERVAL);
                }
                else if (stream_type == AUDIO_STREAM_TYPE_RECORD)
                {

                }
            }
            else if (param->track_state_changed.state == AUDIO_TRACK_STATE_STOPPED)
            {
                char *temp_buff = "Audio Track Stopped!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));

                if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                {
                    app_stop_timer(&timer_idx_sbc_play);
                }
            }
            else if (param->track_state_changed.state == AUDIO_TRACK_STATE_PAUSED)
            {
                char *temp_buff = "Audio Track Paused!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));

                if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                {
                    app_stop_timer(&timer_idx_sbc_play);
                }
            }
        }
        break;

    case AUDIO_EVENT_TRACK_DATA_IND:
        {
            T_AUDIO_STREAM_STATUS status;
            uint32_t timestamp;
            uint16_t seq_num;
            uint8_t  frame_num;
            uint16_t read_len;
            uint8_t *buf;

            buf = (uint8_t *)malloc(sizeof(uint8_t) + param->track_data_ind.len);
            if (buf != NULL)
            {
                if (!audio_track_read(param->track_data_ind.handle,
                                      &timestamp,
                                      &seq_num,
                                      &status,
                                      &frame_num,
                                      buf,
                                      param->track_data_ind.len,
                                      &read_len))
                {
                    APP_PRINT_TRACE0("AUDIO_EVENT_TRACK_DATA_IND: audio_track_read fail");
                    free(buf);
                    break;
                }

                //bt_sco_data_send(p_link->bd_addr, seq_num, buf, read_len);
                free(buf);
            }

            if (audio_track_stream_type_get(param->track_data_ind.handle, &stream_type) == true)
            {
                read_count++;

                if (stream_type == AUDIO_STREAM_TYPE_VOICE)
                {
                    if (read_count == 100)
                    {
                        char *temp_buff = "Voice Data In!\r\n";
                        console_write((uint8_t *)temp_buff, strlen(temp_buff));

                        read_count = 0;
                    }

                    app_audio_track_write();
                }
                else if (stream_type == AUDIO_STREAM_TYPE_RECORD)
                {
                    if (read_count == 100)
                    {
                        char *temp_buff = "Record Data In!\r\n";
                        console_write((uint8_t *)temp_buff, strlen(temp_buff));

                        read_count = 0;
                    }
                }
            }
        }
        break;

    case AUDIO_EVENT_TRACK_VOLUME_OUT_CHANGED:
        {
            uint8_t temp_buff[40];
            uint16_t buf_len;

            buf_len =  sprintf((char *)temp_buff, "Audio Track Volume Out Level: %d \r\n",
                               param->track_volume_out_changed.curr_volume);
            console_write(temp_buff, buf_len);
        }
        break;

    case AUDIO_EVENT_TRACK_VOLUME_OUT_MUTED:
        {
            char *temp_buff = "Audio Track Volume Out Muted!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case AUDIO_EVENT_TRACK_VOLUME_OUT_UNMUTED:
        {
            char *temp_buff = "Audio Track Volume Out Unmuted!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_LOW:
        {
            if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == true)
            {
                if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                {
                    app_audio_track_write();
                }
            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_HIGH:
        {
            if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == true)
            {
                if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                {
                    app_stop_timer(&timer_idx_sbc_play);
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_audio_policy_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_audio_policy_timeout_cb: timer_evt %d, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_AUDIO_DEMO_SBC_PLAY:
        {
            for (int i = 0; i < 3; i++)
            {
                app_audio_track_write();
            }
        }
        break;

    case APP_TIMER_AUDIO_DEMO_MSBC_PLAY:
        {
            for (int i = 0; i < 15; i++)
            {
                app_audio_track_write();
            }
            app_stop_timer(&timer_idx_msbc_play);
        }
        break;

    default:
        break;
    }
}

void app_audio_init(void)
{
    audio_mgr_cback_register(app_audio_policy_cback);
    app_timer_reg_cb(app_audio_policy_timeout_cb, &audio_policy_timer_id);
}

bool app_audio_notification_play(uint8_t tone_index,  bool flush, bool relay)
{
    bool ret;

    ret = false;

    if (tone_index < VOICE_PROMPT_INDEX)
    {
        if (flush)
        {
            ringtone_cancel(tone_index, true);
        }
        ret = ringtone_play(tone_index, relay);
    }
    else if (tone_index < TONE_INVALID_INDEX)
    {
        if (flush)
        {
            voice_prompt_cancel(tone_index - VOICE_PROMPT_INDEX, true);
        }
        ret = voice_prompt_play(tone_index - VOICE_PROMPT_INDEX,
                                (T_VOICE_PROMPT_LANGUAGE_ID)VOICE_PROMPT_LANGUAGE, relay);
    }

    return ret;
}
