/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "bt_types.h"
#include "os_mem.h"
#include "app_msg.h"
#include "app_console_msg.h"
#include "app_audio_demo_console.h"
#include "app_audio_demo_policy.h"
#include "app_audio_demo_passthrough.h"
#include "app_audio_demo_line_in.h"
#include "app_audio_demo_anc.h"
#include "app_audio_demo_pipe.h"
#include "app_audio_demo_track.h"

#define LLAPT_SCENARIO_ID     0x00
#define ANC_SCENARIO_ID       0x00

void app_console_handle_msg(T_IO_MSG console_msg)
{
    uint16_t  subtype;
    uint16_t  id;
    uint16_t  action;
    uint8_t  *p;

    p       = console_msg.u.buf;
    subtype = console_msg.subtype;

    switch (subtype)
    {
    case IO_MSG_CONSOLE_STRING_RX:
        LE_STREAM_TO_UINT16(id, p);
        LE_STREAM_TO_UINT8(action, p);

        if (id == AUDIO_ID)
        {

        }
        else if (id == NOTIFICATION_ID)
        {
            switch (action)
            {
            case AUDIO_DEMO_ACTION_RING_TONE_PLAY:
                app_audio_notification_play(0x01, false, true);
                break;

            case AUDIO_DEMO_ACTION_VOICE_PROMPT_PLAY:
                app_audio_notification_play(0x85, false, true);
                break;

            default:
                break;
            }
        }
        else if (id == APT_ID)
        {
            switch (action)
            {
            case AUDIO_DEMO_ACTION_APT_PLAY:
                app_audio_normal_apt_enable();
                break;

            case AUDIO_DEMO_ACTION_APT_STOP:
                app_audio_normal_apt_disable();
                break;

            default:
                break;
            }
        }
        else if (id == LLAPT_ID)
        {
            switch (action)
            {
            case AUDIO_DEMO_ACTION_LLAPT_PLAY:
                app_audio_llapt_enable(LLAPT_SCENARIO_ID);
                break;

            case AUDIO_DEMO_ACTION_LLAPT_STOP:
                app_audio_llapt_disable();
                break;

            default:
                break;
            }
        }
        else if (id == LINE_IN_ID)
        {
            switch (action)
            {
            case AUDIO_DEMO_ACTION_LINE_IN_START:
                app_audio_line_in_start();
                break;

            case AUDIO_DEMO_ACTION_LINE_IN_STOP:
                app_audio_line_in_stop();
                break;

            default:
                break;
            }
        }
        else if (id == ANC_ID)
        {
            switch (action)
            {
            case AUDIO_DEMO_ACTION_ANC_ENABLE:
                app_audio_anc_enable(ANC_SCENARIO_ID);
                break;

            case AUDIO_DEMO_ACTION_ANC_DISABLE:
                app_audio_anc_disable();
                break;

            default:
                break;
            }
        }
        else if (id == PIPE_ID)
        {
            switch (action)
            {
            case AUDIO_DEMO_ACTION_PIPE_CREATE:
                {
                    uint8_t src_type;
                    uint8_t snk_type;

                    LE_STREAM_TO_UINT8(src_type, p);
                    LE_STREAM_TO_UINT8(snk_type, p);

                    app_audio_pipe_create(src_type, snk_type);
                }
                break;

            case AUDIO_DEMO_ACTION_PIPE_START:
                app_audio_pipe_start();
                break;

            case AUDIO_DEMO_ACTION_PIPE_STOP:
                app_audio_pipe_stop();
                break;

            case AUDIO_DEMO_ACTION_PIPE_RELEASE:
                app_audio_pipe_release();
                break;

            default:
                break;
            }
        }
        else if (id == TRACK_ID)
        {
            switch (action)
            {
            case AUDIO_DEMO_ACTION_TRACK_CREATE:
                {
                    uint8_t stream_type;
                    uint8_t format_type;
                    uint8_t volume_out;
                    uint8_t volume_in;

                    LE_STREAM_TO_UINT8(stream_type, p);
                    LE_STREAM_TO_UINT8(format_type, p);
                    LE_STREAM_TO_UINT8(volume_out, p);
                    LE_STREAM_TO_UINT8(volume_in, p);

                    app_audio_track_create(stream_type,
                                           format_type,
                                           volume_out,
                                           volume_in);
                }
                break;

            case AUDIO_DEMO_ACTION_TRACK_START:
                app_audio_track_start();
                break;

            case AUDIO_DEMO_ACTION_TRACK_STOP:
                app_audio_track_stop();
                break;

            case AUDIO_DEMO_ACTION_TRACK_PAUSE:
                app_audio_track_pause();
                break;

            case AUDIO_DEMO_ACTION_TRACK_RESTART:
                app_audio_track_restart();
                break;

            case AUDIO_DEMO_ACTION_TRACK_RELEASE:
                app_audio_track_release();
                break;

            case AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_SET:
                {
                    uint8_t volume_out;

                    LE_STREAM_TO_UINT8(volume_out, p);

                    app_audio_track_volume_out_set(volume_out);
                }
                break;

            case AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_MUTE:
                app_audio_track_volume_out_mute();
                break;

            case AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_UNMUTE:
                app_audio_track_volume_out_unmute();
                break;

            default:
                break;
            }
        }

        os_mem_free(console_msg.u.buf);
        break;

    default:
        break;
    }
}
