/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
/* Includes -----------------------------------------------------------------*/
#include "trace.h"
#include "audio_track.h"
#include "audio_type.h"
#include "app_cfg.h"
#include "app_dsp_cfg.h"
#include "app_timer.h"
#include "audio.h"
#include "playback_stream_ctrl.h"
#include "app_dlps.h"
//#include "playback_audio_file.h"
#include "audio_fs_decode.h"
#include "app_main.h"
#include "app_music_ctrl.h"

/* Defines ------------------------------------------------------------------*/

/* Globals ------------------------------------------------------------------*/
void (*playback_stream_ctrl_stop_complete_hook)(void) = NULL;
void (*playback_stream_ctrl_delay_stop_hook)(uint16_t time_ms) = NULL;

static struct
{
    T_AUDIO_EFFECT_INSTANCE     eq_instance;
    T_PLAYBACK_STATE            play_state;
    uint8_t                     track_state;
    uint8_t                     volume;
    T_PLAYBACK_BUF_STATE        buffer_state;
    uint16_t                    put_data_time_ms;
    uint16_t                    preq_pkts;
    uint8_t                     frame_num;
    uint16_t                    delay_stop_ms;
} playback;

T_AUDIO_TRACK_HANDLE    playback_track_handle;

typedef enum
{
    APP_TIMER_PLAYBACK_PUT_DATA,
} T_APP_PLAYBACK_TIMER;
static uint8_t timer_idx_playback_put_data = 0;
static uint8_t app_playback_time_id = 0;

static void playback_stream_put_data_stop_timer(uint8_t pkt_num);
static void playback_stream_put_data_start_timer(uint16_t time_ms);

T_PLAYBACK_STATE playback_stream_get_state(void)
{
    return playback.play_state;
}

uint8_t playback_stream_get_music_info(T_PLAYBACK_AF_FORMAT_INFO fmt_info,
                                       T_PLAY_SET_INFO *set_info)
{
    uint32_t sample_rate = 0;
    uint16_t sample_counts = 1024; /* default */
    uint16_t frame_duration = 20; /* default */
    uint16_t frame_size = 512;
    uint8_t channel_mode = 0;
    uint8_t media_buf_pkt_max;

    if (fmt_info.format_info.type == AUDIO_FORMAT_TYPE_AAC)
    {
        sample_rate = fmt_info.format_info.attr.aac.sample_rate;
    }
    else if (fmt_info.format_info.type == AUDIO_FORMAT_TYPE_MP3)
    {
        sample_rate = fmt_info.format_info.attr.mp3.sample_rate;
    }
    frame_duration = fmt_info.frame_duration;
    sample_counts = fmt_info.sample_counts;
    frame_size = fmt_info.frame_size;

    if (frame_duration > 100)
    {
        frame_duration = 50;
    }
    if (frame_size > 2048)
    {
        frame_size = 1024;
    }

    media_buf_pkt_max = PLAYBACK_POOL_SIZE / frame_size;
    uint8_t frm_num = STREAM_BUF_SIZE / 2 / frame_size;
    if (media_buf_pkt_max > 20)
    {
        set_info->latency     = 4 * frame_duration;
        set_info->low_level   = 1 * frame_duration;
        set_info->upper_level = 15 * frame_duration;
        playback.frame_num = 6;
    }
    else if (media_buf_pkt_max > 10)
    {
        set_info->latency      = 3 * frame_duration;
        set_info->low_level   = 1 * frame_duration;
        set_info->upper_level = 9 * frame_duration;
        playback.frame_num = (frm_num > 4) ? 4 : 3;
    }
    else
    {
        set_info->latency      = 2 * frame_duration;
        set_info->low_level   = 1 * frame_duration;
        set_info->upper_level = (media_buf_pkt_max - 2) * frame_duration;
        playback.frame_num = (frm_num > 4) ? 4 : 3;
    }
    set_info->play_duration = playback.frame_num * frame_duration;
    set_info->preq_pkts = sample_rate * (set_info->latency  + set_info->low_level) / 1000 /
                          sample_counts + 2;//3;
    APP_PRINT_WARN6("playback_stream_get_music_info: put_data_time_ms: %d, preq_pkts: %d " \
                    "media_buf_pkt_max:%d, s_frame_num:%d, size:%d, frame_duration:%d",
                    set_info->play_duration, set_info->preq_pkts,
                    media_buf_pkt_max, playback.frame_num, frame_size, frame_duration);
    return 0;
}

/**
  * @brief Initialize FATFS file system and sd card physical layer.
  * @param   No parameter.
  * @return  void
  */
uint8_t playback_stream_parameter_recfg(void)
{
    uint8_t res = PLAYBACK_SUCCESS;
    uint16_t u16_res = 0;

    T_PLAYBACK_AF_FORMAT_INFO get_fmt_info;
    T_PLAY_SET_INFO set_play_info;

    u16_res = playback_audio_file_get_audio_info(&get_fmt_info);
    if (u16_res == 0)
    {
        T_AUDIO_FORMAT_INFO format_info;
        uint32_t device = app_db.playback_device;

        format_info = get_fmt_info.format_info;
        playback_stream_get_music_info(get_fmt_info, &set_play_info);
        playback.put_data_time_ms = set_play_info.play_duration;
        playback.preq_pkts = set_play_info.preq_pkts;

        playback_track_handle = audio_track_create(AUDIO_STREAM_TYPE_PLAYBACK, //stream_type
                                                   AUDIO_STREAM_MODE_NORMAL, // mode
                                                   AUDIO_STREAM_USAGE_SNOOP, // usage
                                                   format_info, //format_info
                                                   playback.volume, //volume
                                                   0,
                                                   device, // device
                                                   NULL,
                                                   NULL);

        if (playback_track_handle != NULL)
        {
            audio_track_latency_set(playback_track_handle, set_play_info.latency, true);
            audio_track_threshold_set(playback_track_handle, set_play_info.upper_level,
                                      set_play_info.low_level);
            playback.delay_stop_ms = set_play_info.latency;
        }
    }
    else
    {
        res = PLAYBACK_SYS_ERROR;
    }
    return res;
}

/**
  * @brief playback get sample rate function
  * @param   get sample rate 8000, 32000, 44100,48000 and so on.
  * @return  void
  */
bool playback_stream_get_sample_rate(uint32_t *p_sample_rate)
{
    bool ret = false;
    T_AUDIO_FORMAT_INFO format;
    uint32_t sample_rate = 0;

    if (playback_track_handle)
    {
        audio_track_format_info_get(playback_track_handle, &format);
        switch (format.type)
        {
        case AUDIO_FORMAT_TYPE_AAC:
            sample_rate = format.attr.aac.sample_rate;
            ret = true;
            break;

        case AUDIO_FORMAT_TYPE_MP3:
            sample_rate = format.attr.mp3.sample_rate;
            ret = true;
            break;

        default:
            break;
        }
        *p_sample_rate = sample_rate;
    }

    return ret;
}

/**
  * @brief playback start function
  * @param   No parameter.
  * @return  void
  */
uint8_t playback_stream_ctrl_start(void)
{
    uint8_t res = PLAYBACK_SUCCESS;
    uint32_t sampling_frequency = 0;

    APP_PRINT_TRACE0("playback_stream_ctrl_start ++");

    app_dlps_disable(APP_DLPS_ENTER_CHECK_PLAYBACK);

    if (playback_track_handle)
    {
        audio_track_release(playback_track_handle);
        playback_track_handle = NULL;
    }

    if (playback.eq_instance != NULL)
    {
        eq_release(playback.eq_instance);
        playback.eq_instance = NULL;
    }

    if ((res = playback_stream_parameter_recfg()) != 0)
    {
        return res;
    }

    playback.buffer_state = PLAYBACK_BUF_NORMAL;
    playback.play_state = PLAYBACK_STATE_PLAY;
    playback_stream_volume_set(playback.volume);
    if (playback_track_handle != NULL)
    {
        playback_stream_get_sample_rate(&sampling_frequency);
        if ((sampling_frequency == SAMPLE_RATE_44K) || (sampling_frequency == SAMPLE_RATE_48K))
        {
            app_eq_idx_check_accord_mode();
            playback.eq_instance = app_eq_create(EQ_CONTENT_TYPE_AUDIO, EQ_STREAM_TYPE_AUDIO, SPK_SW_EQ,
                                                 app_db.spk_eq_mode, app_cfg_nv.eq_idx);
            if (playback.eq_instance != NULL)
            {
                eq_enable(playback.eq_instance);
                audio_track_effect_attach(playback_track_handle, playback.eq_instance);
            }
        }
        else
        {
            APP_PRINT_WARN1("EQ don't support this sample rate: %d", sampling_frequency);
        }

        audio_track_start(playback_track_handle);
    }

    return res;
}

/**
  * @brief playback stop function
  * @param   No parameter.
  * @return  void
  */
uint8_t playback_stream_ctrl_stop(void)
{
    uint8_t res = PLAYBACK_SUCCESS;

    if (playback.play_state == PLAYBACK_STATE_PLAY)
    {
        app_stop_timer(&timer_idx_playback_put_data);
        playback.play_state = PLAYBACK_STATE_IDLE;

        if (playback_track_handle != NULL)
        {
            audio_track_release(playback_track_handle);
            //        playback_track_handle = NULL;
        }

        if (playback.eq_instance != NULL)
        {
            eq_release(playback.eq_instance);
            playback.eq_instance = NULL;
        }
        app_dlps_enable(APP_DLPS_ENTER_CHECK_PLAYBACK);
    }

    return res;
}

void playback_stream_volume_up(void)
{
    if (playback.volume < app_dsp_cfg_vol.playback_volume_max)
    {
        playback.volume++;
    }
    else
    {
        playback.volume = app_dsp_cfg_vol.playback_volume_max;
    }

    APP_PRINT_WARN2("playback_stream_volume_up,volume:%d,max:%d", playback.volume,
                    app_dsp_cfg_vol.playback_volume_max);

    if (playback_track_handle != NULL)
    {
        audio_track_volume_out_set(playback_track_handle, playback.volume);
    }
}

void playback_stream_volume_down(void)
{
    if (playback.volume > app_dsp_cfg_vol.playback_volume_min)
    {
        playback.volume--;
    }
    else
    {
        playback.volume = app_dsp_cfg_vol.playback_volume_min;
    }

    APP_PRINT_WARN2("playback_stream_volume_down,volume:%d,min:%d", playback.volume,
                    app_dsp_cfg_vol.playback_volume_min);

    if (playback_track_handle != NULL)
    {
        audio_track_volume_out_set(playback_track_handle, playback.volume);
    }

}

bool playback_stream_volume_set(uint8_t volume)
{
    uint8_t max_volume = app_dsp_cfg_vol.playback_volume_max;
    uint8_t min_volume = app_dsp_cfg_vol.playback_volume_min;
    bool ret = false;
    if (volume > max_volume)
    {
        volume = max_volume;
    }
    playback.volume = volume;
    APP_PRINT_WARN1("playback_stream_volume_set vol:%d", volume);
    if (playback_track_handle != NULL)
    {
        if ((volume <= max_volume) && (volume >= min_volume))
        {
            ret = audio_track_volume_out_set(playback_track_handle, playback.volume);
        }
    }
    return ret;
}

uint8_t playback_stream_volume_get(void)
{
    return playback.volume;
}

void playback_stream_put_data(uint8_t pkt_num)
{
    uint16_t res = 0;
    uint8_t frame_cnt = 0;
    uint16_t time_ms = playback.put_data_time_ms;
    T_PLAYBACK_FRAME_PKT playback_frame;
    static uint16_t s_seq_num = 0;

    APP_PRINT_INFO1("playback_stream_put_data pkt_num: %d", pkt_num);

    while (frame_cnt < pkt_num)
    {
        // This maybe AUDIO_EVENT_TRACK_BUFFER_HIGH event
        if (playback.buffer_state == PLAYBACK_BUF_HIGH)
        {
            time_ms = playback.put_data_time_ms * 2;
            break;
        }
        res = playback_audio_file_get_frame(&playback_frame);
        if (res != 0)
        {
            APP_PRINT_ERROR1("playback_stream_put_data ERROR,RES:0x%x", res);
            break;
        }

        uint16_t written_len;
        s_seq_num++;
        if (audio_track_write(playback_track_handle,
                              0,//              timestamp,
                              s_seq_num,
                              AUDIO_STREAM_STATUS_CORRECT,
                              playback_frame.frame_num,//            frame_num,
                              playback_frame.buf,
                              playback_frame.length,
                              &written_len) == false)
        {
            res = PLAYBACK_AF_WRITE_ERROR;
            break;
        }
        frame_cnt++;
    }
    stream_check_and_request_data(); // request data from host
    playback.buffer_state = PLAYBACK_BUF_NORMAL;

    if (res == PLAYBACK_AF_END_ERROR)
    {
        app_stop_timer(&timer_idx_playback_put_data);
        if (frame_cnt == 0)
        {
            APP_PRINT_WARN0("playback_stream_put_data,file end, and paly next song!!!");
            playback_stream_ctrl_stop();
            app_music_send_player_status(MUSIC_PLAYER_STOPPED);
        }

    }
    else //if (playback_db.sd_play_state == APP_AUDIO_FS_STATE_PLAY)
    {
        playback_stream_put_data_start_timer(time_ms);
    }
}

static void playback_stream_track_state_changed_handle(void)
{
    APP_PRINT_TRACE1("playback_stream_track_state_changed_handle, track_state:%d",
                     playback.track_state);
    if (AUDIO_TRACK_STATE_STARTED == playback.track_state)
    {
        playback_stream_put_data(playback.preq_pkts);
    }
    else if (AUDIO_TRACK_STATE_RELEASED == playback.track_state)
    {
        playback_track_handle = NULL;
        // TO DO report host
        if (playback_stream_ctrl_stop_complete_hook)
        {
            playback_stream_ctrl_stop_complete_hook();
        }
    }
}

static void playback_stream_buffer_low_handle(void)
{
    playback.buffer_state = PLAYBACK_BUF_LOW;
    if (playback_audio_file_is_end())
    {
        APP_PRINT_WARN0("playback_stream_buffer_low_handle,file end, and play next song!!!");
        if (playback_stream_ctrl_delay_stop_hook)
        {
            playback_stream_ctrl_delay_stop_hook(playback.delay_stop_ms);
        }
        else
        {
            playback_stream_ctrl_stop();
            app_music_send_player_status(MUSIC_PLAYER_STOPPED);
            app_music_report_play_time();
        }
    }
    else
    {
        playback_stream_put_data_stop_timer(playback.frame_num + 2);
    }

}

static void playback_stream_buffer_high_handle(void)
{
//    app_stop_timer(&timer_idx_playback_put_data);
    playback.buffer_state = PLAYBACK_BUF_HIGH;
}

static void playback_stream_audio_policy_cback(T_AUDIO_EVENT event_type, void *event_buf,
                                               uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    if (param->track_state_changed.handle != playback_track_handle)
    {
        return;
    }

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            playback.track_state = param->track_state_changed.state;
            playback_stream_track_state_changed_handle();
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_LOW:
        {
            if (playback.play_state == PLAYBACK_STATE_PLAY)
            {
                playback_stream_buffer_low_handle();
            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_HIGH:
        {
            if (playback.play_state == PLAYBACK_STATE_PLAY)
            {
                playback_stream_buffer_high_handle();
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("playback_stream_audio_policy_cback: event_type 0x%04x", event_type);
    }
}

//////////////////////////////////////////////////////////////////////////////////////

static void playback_stream_put_data_stop_timer(uint8_t pkt_num)
{
    APP_PRINT_TRACE0("playback_stream_put_data_stop_timer");
    app_stop_timer(&timer_idx_playback_put_data);

    if (playback.play_state != PLAYBACK_STATE_PLAY)
    {
        return;
    }
    playback_stream_put_data(pkt_num);
}

static void playback_stream_put_data_start_timer(uint16_t time_ms)
{
    app_start_timer(&timer_idx_playback_put_data, "playback_stream_put_data",
                    app_playback_time_id, APP_TIMER_PLAYBACK_PUT_DATA, 0, false,
                    time_ms);
}

static void playback_stream_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO2("playback_stream_timeout_cb: timer_evt 0x%02x, param 0x%x", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_PLAYBACK_PUT_DATA:
        {
            playback_stream_put_data_stop_timer(playback.frame_num);
        }
        break;

    default:
        break;
    }
}

void playback_stream_ctrl_init(void)
{
    memset(&playback, 0, sizeof(playback));
    playback.volume = app_dsp_cfg_vol.playback_volume_default;
    audio_mgr_cback_register(playback_stream_audio_policy_cback);

    app_timer_reg_cb(playback_stream_timeout_cb, &app_playback_time_id);
}
#endif

