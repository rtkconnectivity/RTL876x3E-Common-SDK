/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_SD_CARD_PLAY
#include "trace.h"
#include "string.h"
#include "app_src_playback.h"
#include "app_src_play.h"
#include "app_dlps.h"
#include "app_link_util_cs.h"
#include "app_cfg.h"
#include "app_dsp_cfg.h"
#include "app_main.h"
#include "app_cmd.h"
#include "app_timer.h"
#include "btm.h"
#include "diskio.h"     /* Declarations of disk functions */
#include "audio.h"
#include "audio_type.h"
#include "bt_bond.h"
#include "audio_track.h"
#include "audio_fs.h"
#include "audio_fs_decode.h"
#include "ff.h"
#include "app_lea_ini_audio_data.h"
#include "hw_tim.h"
#include "ring_buffer.h"
#include "audio_pipe.h"
#include "app_src_play_a2dp.h"
#include "fmc_api.h"

/* dlps related */
static uint16_t s_sd_pd_bitmap = 0;
static uint16_t s_sd_dlps_bitmap = 0;

static uint8_t sd_music_name[64];
static uint8_t sd_music_name_len;

static uint16_t audio_fs_init_result;

FATFS record_fs;
FIL record_file;
bool is_record_save_file = true;

#define SECTOR_SIZE             0x1000
#define NAME_BUF_SIZE           (100*sizeof(T_HEAD_INFO))
uint8_t name_buf[NAME_BUF_SIZE];

/* pipe related */
typedef enum
{
    SD_PLAY_STATE_NONE,
    SD_PLAY_STATE_FILLING,
    SD_PLAY_STATE_FILLED,
} T_SD_PIPE_STATE;

/* timer related */
typedef enum
{
    APP_TIMER_SD_LOCAL_PUT_DATA,
    APP_TIMER_SD_LOCAL_RESUME,
    APP_TIMER_SD_DELAY_STOP_LOCAL,
    APP_TIMER_SD_DELAY_STOP_PIPE,
} T_APP_SRC_PLAYBACK_TIMER;
static uint8_t timer_idx_sd_local_put_data = 0;
static uint8_t timer_idx_sd_delay_stop_local = 0;
static uint8_t timer_idx_sd_delay_stop_pipe = 0;
static uint8_t app_src_play_sd_timer_id = 0;

/* le audio related */
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
#define LC3_RBUF_SIZE                  1024 * 3 + 256
static struct
{
    bool                    timer_started;
    T_HW_TIMER_HANDLE       timer_handle;
    uint32_t                timer_duration;
    uint16_t                lc3_tx_seq;
    uint8_t                 *p_lea_send_buf;
    uint16_t                pkt_len;
    T_RING_BUFFER           ring_buf;
    uint8_t                 target_threshold;
    uint8_t                 pre_fill_num;
} lea_tx_mgr =
{
    .timer_handle = NULL,
    .timer_duration = 10000,
    .timer_started = false,
    .lc3_tx_seq = 0,
    .p_lea_send_buf = NULL,
    .pkt_len = 0,
    .pre_fill_num = 0,
};
#endif

/* sd card manager */
static struct
{
    T_AUDIO_FS_HANDLE           fs_handle;
    FRAME_CONTENT               *frame_content;
    uint8_t                     op_next_action;
    uint8_t                     playlist_idx;
    uint16_t                    file_idx;
#if F_APP_SD_CARD_LOCALPLAY
    struct
    {
        T_AUDIO_TRACK_HANDLE    handle;
        T_SD_PLAY_STATE         play_state;
        T_PLAYBACK_DATA         play_monitor;
        uint8_t                 volume;
    } local_play;
#endif
    struct
    {
        T_AUDIO_PIPE_HANDLE     handle;
        T_SD_PLAY_STATE         play_state;
        uint8_t                 volume;
        T_SD_PIPE_STATE         fill_state;
        uint8_t                 fill_seq;
        void                    *p_drain_buf;
    } pipe_play;
} sd =
{
    .fs_handle = NULL,
    .frame_content = NULL,
#if F_APP_SD_CARD_LOCALPLAY
    .local_play =
    {
        .play_state = SD_PLAY_STATE_IDLE,
        .handle = NULL,
        .volume = 10,
    },
#endif
    .pipe_play =
    {
        .handle = NULL,
        .volume = 10,
        .play_state = SD_PLAY_STATE_IDLE,
        .fill_state = SD_PLAY_STATE_NONE,
        .p_drain_buf = NULL,
    }
};

static struct
{
    uint8_t             *sd_file_name;
    uint16_t            sd_length;
} sd_file_info =
{
    .sd_file_name = NULL,
    .sd_length = 0,
};

static bool src_play_check_and_fill_data(void);
static void app_src_play_sd_pipe_stop(void);
static void app_src_sd_card_dlps_enable(uint16_t bit);
void app_src_play_sd_file_close(void);

/* LE Audio Related*/
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
static void app_src_play_pipe_sd_lea_timer_stop(void)
{
    lea_tx_mgr.timer_started = false;
    hw_timer_stop(lea_tx_mgr.timer_handle);
}

static void app_src_play_pipe_sd_lea_timer_start(void)
{
    lea_tx_mgr.timer_started = true;
    hw_timer_start(lea_tx_mgr.timer_handle);
    hw_timer_restart(lea_tx_mgr.timer_handle, lea_tx_mgr.timer_duration);
}

void app_src_play_sd_pipe_send_lea_data(void)
{
    T_AUDIO_FORMAT_INFO format_info;
    T_RING_BUFFER *p_rbuf = &(lea_tx_mgr.ring_buf);
    uint16_t len = lea_tx_mgr.pkt_len;
    uint8_t *buf = lea_tx_mgr.p_lea_send_buf;

    if (!p_rbuf || !len || !buf)
    {
        return;
    }

    T_PLAY_ROUTE play_route = app_src_play_get_play_route();

    if ((play_route != PLAY_ROUTE_BIS) &&
        (play_route != PLAY_ROUTE_CIS))
    {
        return;
    }

    if (!ring_buffer_read(p_rbuf, len, buf))
    {
        APP_PRINT_WARN2("lea_tx_timer_callback: ringbuf len %d want %d send fake data",
                        ring_buffer_get_data_count(p_rbuf), len);
        memset(buf, 0, len);
    }

    uint32_t ts = lea_tx_mgr.lc3_tx_seq * lea_tx_mgr.timer_duration;
    app_lea_iso_data_send(buf, len, true, ts, lea_tx_mgr.lc3_tx_seq);
    lea_tx_mgr.lc3_tx_seq++;

    src_play_check_and_fill_data();
}

static void sd_pipe_lea_tx_timer_callback(T_HW_TIMER_HANDLE handle)
{
    app_src_play_send_msg(SD_PIPE_LC3_TIMER, NULL);
}

static void app_src_play_sd_lc3_timer_init(void)
{
    lea_tx_mgr.timer_handle = hw_timer_create("lc3_tx", lea_tx_mgr.timer_duration, true,
                                              sd_pipe_lea_tx_timer_callback);

    hw_timer_lpm_set(lea_tx_mgr.timer_handle, false);
    if (lea_tx_mgr.timer_handle == NULL)
    {
        APP_PRINT_ERROR0("app_src_play_sd_lc3_timer_init: create fail");
    }
}

void app_src_play_sd_handle_lea_stop(void)
{
    lea_tx_mgr.lc3_tx_seq = 0;
}
#endif

#if F_APP_SD_CARD_LOCALPLAY
static void app_src_playback_put_data_start_timer(uint16_t time_ms)
{
    app_start_timer(&timer_idx_sd_local_put_data, "src_playback_put_data",
                    app_src_play_sd_timer_id, APP_TIMER_SD_LOCAL_PUT_DATA, 0, false,
                    time_ms);
}

static void app_src_playback_put_data(uint8_t pkt_num)
{
    uint16_t res = 0;
    uint8_t frame_cnt = 0;
    uint16_t time_ms = sd.local_play.play_monitor.put_data_time_ms;
    static uint16_t s_seq_num = 0;

    APP_PRINT_TRACE1("app_src_playback_put_data: pkt_num %d", pkt_num);
    while ((frame_cnt < pkt_num) && (res != FS_END_OF_FILE))
    {
        // This maybe AUDIO_EVENT_TRACK_BUFFER_HIGH event
        if (sd.local_play.play_monitor.buffer_state == PLAYBACK_BUF_HIGH)
        {
            time_ms = sd.local_play.play_monitor.put_data_time_ms * 2;
            break;
        }
        res = audio_fs_get_frame(sd.fs_handle, sd.frame_content);
        if (res != 0)
        {
            APP_PRINT_ERROR1("app_src_playback_put_data ERROR, RES 0x%x", res);
            break;
        }

        uint16_t written_len;
        s_seq_num++;
        if (audio_track_write(sd.local_play.handle,
                              0,
                              s_seq_num,
                              AUDIO_STREAM_STATUS_CORRECT,
                              sd.frame_content->frameNum,
                              sd.frame_content->content,
                              sd.frame_content->length,
                              &written_len) == false)
        {
            res = PLAYBACK_WRITE_ERROR;
            break;
        }
        frame_cnt++;
    }
    sd.local_play.play_monitor.buffer_state = PLAYBACK_BUF_NORMAL;

    if (res == FS_END_OF_FILE)
    {
        app_stop_timer(&timer_idx_sd_local_put_data);
    }
    else if (sd.local_play.play_state == SD_PLAY_STATE_PLAY)
    {
        app_src_playback_put_data_start_timer(time_ms);
    }
}

static void app_src_playback_put_data_stop_timer(uint8_t pkt_num)
{
    APP_PRINT_TRACE0("app_src_playback_put_data_stop_timer");
    app_stop_timer(&timer_idx_sd_local_put_data);

    if (sd.local_play.play_state != SD_PLAY_STATE_PLAY)
    {
        return;
    }
    app_src_playback_put_data(pkt_num);
}

static void app_src_play_sd_local_delay_stop(uint16_t time_ms)
{
    uint16_t delay_ms = time_ms;
    APP_PRINT_TRACE1("app_src_play_sd_local_delay_stop, time_ms:%d", time_ms);
    delay_ms = time_ms + 50;
    app_start_timer(&timer_idx_sd_delay_stop_local, "src_playback delay stop",
                    app_src_play_sd_timer_id,
                    APP_TIMER_SD_DELAY_STOP_LOCAL, 0, false, delay_ms);
}

static void app_src_playback_buffer_low_handle(void)
{
    sd.local_play.play_monitor.buffer_state = PLAYBACK_BUF_LOW;
    if (audio_fs_end_of_file(sd.fs_handle))
    {
        APP_PRINT_WARN0("app_src_playback_buffer_low_handle file end, and play next song!!!");
        app_src_play_sd_local_delay_stop(sd.local_play.play_monitor.delay_stop_ms);
        sd.op_next_action = SD_STOPPED_FILE_END_TO_NEXT_ACTION;
    }
    else
    {
//        app_src_playback_put_data();
        app_src_playback_put_data_stop_timer(sd.local_play.play_monitor.frame_num + 2);
    }

}

static void app_src_playback_buffer_high_handle(void)
{
//    app_stop_timer(&timer_idx_sd_local_put_data);
    sd.local_play.play_monitor.buffer_state = PLAYBACK_BUF_HIGH;
}

static void app_src_playback_stopped_start_next_action(uint8_t next_action)
{
    // TODO:
}

static void app_src_playback_audio_policy_cback(T_AUDIO_EVENT event_type, void *event_buf,
                                                uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    if (param->track_state_changed.handle != sd.local_play.handle)
    {
        return;
    }

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            APP_PRINT_INFO1("AUDIO_EVENT_TRACK_STATE_CHANGED: %d", param->track_state_changed.state);
            sd.local_play.play_monitor.local_track_state = param->track_state_changed.state;

            switch (param->track_state_changed.state)
            {
            case AUDIO_TRACK_STATE_RELEASED:
                {
                    sd.local_play.handle = NULL;
                    app_src_playback_stopped_start_next_action(sd.op_next_action);
                }
                break;

            case AUDIO_TRACK_STATE_STARTED:
                {
                    app_src_playback_put_data(sd.local_play.play_monitor.preq_pkts);
                }
                break;
            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_LOW:
        {
            if (sd.local_play.play_state == SD_PLAY_STATE_PLAY)
            {
                app_src_playback_buffer_low_handle();
            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_HIGH:
        {
            if (sd.local_play.play_state == SD_PLAY_STATE_PLAY)
            {
                app_src_playback_buffer_high_handle();
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_src_playback_audio_policy_cback: event_type 0x%04x", event_type);
    }
}

// Input file format and get set info
static uint8_t app_src_play_sd_set_local_play_info(T_FILE_FORMAT_INFO   *file_format,
                                                   T_LOCALPLAY_SET_INFO *set_info)
{
    uint32_t sample_rate = 0;
    uint16_t sample_counts = 1024; /* default */
    uint16_t frame_duration = 20; /* default */
    uint16_t frame_size = 512;
    uint8_t channel_mode = 0;
    uint8_t media_buf_pkt_max;

    if (file_format->format_info.type == AUDIO_FORMAT_TYPE_AAC)
    {
        sample_rate = file_format->format_info.attr.aac.sample_rate;
    }
    else if (file_format->format_info.type == AUDIO_FORMAT_TYPE_MP3)
    {
        sample_rate = file_format->format_info.attr.mp3.sample_rate;
    }
    frame_duration = file_format->frame_duration;
    sample_counts = file_format->sample_counts;
    frame_size = file_format->frame_size;

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
        set_info->lower_level   = 1 * frame_duration;
        set_info->upper_level = 15 * frame_duration;
        sd.local_play.play_monitor.frame_num = 4;
    }
    else if (media_buf_pkt_max > 10)
    {
        set_info->latency      = 3 * frame_duration;
        set_info->lower_level   = 1 * frame_duration;
        set_info->upper_level = 9 * frame_duration;
        sd.local_play.play_monitor.frame_num = (frm_num > 4) ? 4 : 3;
    }
    else
    {
        set_info->latency      = 2 * frame_duration;
        set_info->lower_level   = 1 * frame_duration;
        set_info->upper_level = (media_buf_pkt_max - 2) * frame_duration;
        sd.local_play.play_monitor.frame_num = (frm_num > 4) ? 4 : 3;
    }
    set_info->play_duration = sd.local_play.play_monitor.frame_num * frame_duration;
    set_info->preq_pkts = sample_rate * (set_info->latency  + set_info->lower_level) / 1000 /
                          sample_counts + 2;//3;

    APP_PRINT_TRACE4("app_src_play_sd_set_local_play_info: MP3,"
                     " type %d, frame_duration %d, sample_counts %d, frame_size %d",
                     file_format->format_info.type, file_format->frame_duration,
                     file_format->sample_counts, file_format->frame_size);

    APP_PRINT_WARN6("app_src_play_sd_set_local_play_info: put_data_time_ms %d, preq_pkts %d " \
                    "media_buf_pkt_max %d, s_frame_num %d, size %d, frame_duration %d",
                    set_info->play_duration, set_info->preq_pkts,
                    media_buf_pkt_max, sd.local_play.play_monitor.frame_num, frame_size, frame_duration);
    return 0;
}

static void app_src_play_sd_local_start(T_FILE_FORMAT_INFO *file_format)
{
    if (sd.local_play.handle != NULL)
    {
        audio_track_release(sd.local_play.handle);
        sd.local_play.handle = NULL;
    }

    T_LOCALPLAY_SET_INFO set_play_info;

    app_src_play_sd_set_local_play_info(file_format, &set_play_info);
    sd.local_play.play_monitor.put_data_time_ms = set_play_info.play_duration;
    sd.local_play.play_monitor.preq_pkts = set_play_info.preq_pkts;

    uint32_t device = app_db.playback_device;

    sd.local_play.handle = audio_track_create(AUDIO_STREAM_TYPE_PLAYBACK, //stream_type
                                              AUDIO_STREAM_MODE_NORMAL, // mode
                                              AUDIO_STREAM_USAGE_SNOOP, // usage
                                              file_format->format_info, //format_info
                                              sd.local_play.volume, //volume
                                              0,
                                              device, // device
                                              NULL,
                                              NULL);

    if (sd.local_play.handle != NULL)
    {
        audio_track_latency_set(sd.local_play.handle, set_play_info.latency, true);
        audio_track_threshold_set(sd.local_play.handle, set_play_info.upper_level,
                                  set_play_info.lower_level);
        sd.local_play.play_monitor.delay_stop_ms = set_play_info.latency;
    }
    sd.op_next_action = SD_STOPPED_IDLE_ACTION;
    // app_src_playback_volume_set(sd.local_play.volume);
    sd.local_play.play_state = SD_PLAY_STATE_PLAY;
    sd.local_play.play_monitor.local_track_state = PLAYBACK_TRACK_STATE_CLEAR;
    sd.local_play.play_monitor.sec_track_state = PLAYBACK_TRACK_STATE_CLEAR;
    sd.local_play.play_monitor.buffer_state = PLAYBACK_BUF_NORMAL;
    audio_track_start(sd.local_play.handle);
}

static void app_src_play_sd_local_stop(void)
{
    uint8_t res = PLAYBACK_SUCCESS;
    audio_fs_decode_deinit();
    app_stop_timer(&timer_idx_sd_local_put_data);
    sd.local_play.play_state = SD_PLAY_STATE_IDLE;
    if (sd.local_play.handle != NULL)
    {
        sd.local_play.play_monitor.local_track_state = PLAYBACK_TRACK_STATE_CLEAR;
        audio_track_release(sd.local_play.handle);
    }

    if (sd.fs_handle != NULL)
    {
        if (0 != audio_fs_close(sd.fs_handle))
        {
            // return;
        }
        sd.fs_handle = NULL;
    }
    app_src_sd_card_dlps_enable(APP_SD_DLPS_ENTER_CHECK_PLAYING);
    // TODO: FIX ME
    // app_sd_card_power_down_enable(APP_SD_POWER_DOWN_ENTER_CHECK_PLAYBACK);

}
#endif

static void app_src_play_sd_pipe_delay_stop(uint16_t time_ms)
{
    uint16_t delay_ms = time_ms;
    APP_PRINT_TRACE1("app_src_play_sd_pipe_delay_stop, time_ms:%d", time_ms);
    app_start_timer(&timer_idx_sd_delay_stop_pipe, "sd_pipe_delay_stop",
                    app_src_play_sd_timer_id,
                    APP_TIMER_SD_DELAY_STOP_PIPE, 0, false, delay_ms);
}

static void app_src_play_sd_pipe_fill_data(void)
{
    if (audio_fs_end_of_file(sd.fs_handle))
    {
        // release pipe until lea ring buf is empty
        if (sd.pipe_play.play_state == SD_PLAY_STATE_PLAY)
        {
            app_src_play_sd_pipe_delay_stop(50);
            sd.pipe_play.play_state = SD_PLAY_STATE_STOPPING;
        }
        sd.op_next_action = SD_STOPPED_FILE_END_TO_NEXT_ACTION;
        return;
    }

    if (audio_fs_get_frame(sd.fs_handle, sd.frame_content) != 0)
    {
        return;
    }

    bool res = audio_pipe_fill(sd.pipe_play.handle,
                               0,
                               sd.pipe_play.fill_seq,
                               AUDIO_STREAM_STATUS_CORRECT,
                               sd.frame_content->frameNum,
                               sd.frame_content->content,
                               sd.frame_content->length);
    if (res)
    {
        sd.pipe_play.fill_seq++;
        sd.pipe_play.fill_state = SD_PLAY_STATE_FILLING;
        APP_PRINT_INFO1("app_src_play_sd_pipe_fill_data: fill_seq %d", sd.pipe_play.fill_seq);
    }
}

static void src_play_pre_fill_data(void)
{
    T_PLAY_ROUTE play_route = app_src_play_get_play_route();

    if (play_route == PLAY_ROUTE_A2DP)
    {
        // TODO: Support A2DP TX
    }
    else if (play_route == PLAY_ROUTE_HFP_AG)
    {
        // TODO: Support HFP TX
    }
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    else if (play_route == PLAY_ROUTE_BIS || play_route == PLAY_ROUTE_CIS)
    {
        if (lea_tx_mgr.pre_fill_num > 0)
        {
            app_src_play_sd_pipe_fill_data();
            lea_tx_mgr.pre_fill_num--;
        }
    }
#endif
}

static bool src_play_check_and_fill_data(void)
{
    bool ret = false;
    if (sd.pipe_play.fill_state == SD_PLAY_STATE_FILLED)
    {
        T_PLAY_ROUTE play_route = app_src_play_get_play_route();

        if (play_route == PLAY_ROUTE_A2DP)
        {
            // TODO: Support A2DP TX
            return false;
        }
        else if (play_route == PLAY_ROUTE_HFP_AG)
        {
            // TODO: Support HFP TX
            return false;
        }
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
        else if (play_route == PLAY_ROUTE_BIS || play_route == PLAY_ROUTE_CIS)
        {
            uint32_t used_size = ring_buffer_get_data_count(&lea_tx_mgr.ring_buf);
            uint8_t  left_frame_num = used_size / lea_tx_mgr.pkt_len;
            APP_PRINT_INFO2("src_play_check_and_fill_data: used_size %d left_frame_num %d",
                            used_size, left_frame_num);
            if (left_frame_num <= lea_tx_mgr.target_threshold)
            {
                app_src_play_sd_pipe_fill_data();
                ret = true;
            }
        }
#endif
    }

    return ret;
}

static void app_src_play_sd_pipe_start_next_action(uint8_t next_action)
{
    // TODO: restart
}

static void app_src_play_sd_pipe_handle_release(void)
{
    APP_PRINT_INFO0("app_src_play_sd_pipe_handle_release");
    T_PLAY_ROUTE play_route = app_src_play_get_play_route();
    sd.pipe_play.handle = NULL;
    if (sd.pipe_play.p_drain_buf)
    {
        free(sd.pipe_play.p_drain_buf);
        sd.pipe_play.p_drain_buf = NULL;
    }

    if (play_route == PLAY_ROUTE_A2DP)
    {
        // TODO: Support A2DP TX
        return;
    }
    else if (play_route == PLAY_ROUTE_HFP_AG)
    {
        // TODO: Support HFP TX
        return;
    }
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    else if (play_route == PLAY_ROUTE_BIS || play_route == PLAY_ROUTE_CIS)
    {
        if (lea_tx_mgr.p_lea_send_buf)
        {
            free(lea_tx_mgr.p_lea_send_buf);
            lea_tx_mgr.p_lea_send_buf = NULL;
        }
        app_src_play_pipe_sd_lea_timer_stop();
        ring_buffer_clear(&lea_tx_mgr.ring_buf);
        lea_tx_mgr.lc3_tx_seq = 0;
    }
#endif

    sd.pipe_play.play_state = SD_PLAY_STATE_IDLE;

    app_src_play_sd_pipe_start_next_action(sd.op_next_action);
}

static void app_src_play_sd_pipe_handle_data_ind(void)
{
    uint16_t len = 0;
    uint32_t timestamp = 0;
    uint16_t seq = 0;
    T_AUDIO_STREAM_STATUS status;
    uint8_t  frame_num = 0;

    bool ret = audio_pipe_drain(sd.pipe_play.handle,
                                &timestamp,
                                &seq,
                                &status,
                                &frame_num,
                                sd.pipe_play.p_drain_buf,
                                &len);
    if (!ret)
    {
        APP_PRINT_ERROR0("pipe_handle_data_ind: drain failed");
        return;
    }

    T_PLAY_ROUTE play_route = app_src_play_get_play_route();

    if (play_route == PLAY_ROUTE_A2DP)
    {
        // TODO: Support A2DP TX
        return;
    }
    else if (play_route == PLAY_ROUTE_HFP_AG)
    {
        // TODO: Support HFP TX
        return;
    }
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    else if (play_route == PLAY_ROUTE_BIS || play_route == PLAY_ROUTE_CIS)
    {
        uint32_t remain_size = ring_buffer_get_remaining_space(&lea_tx_mgr.ring_buf);
        if (remain_size < len)
        {
            APP_PRINT_ERROR0("BUF_FULL!!");
            return;
        }

        ring_buffer_write(&(lea_tx_mgr.ring_buf), sd.pipe_play.p_drain_buf, len);
    }
#endif
}

static bool app_src_play_sd_pipe_cback(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT event,
                                       uint32_t param)
{
    APP_PRINT_INFO1("app_src_play_sd_pipe_cback: event 0x%x", event);

    switch (event)
    {
    case AUDIO_PIPE_EVENT_CREATED:
        {
            uint32_t snk_buf_size = param;

            APP_PRINT_TRACE1("app_src_play_sd_pipe_cback: snk_buf_size:0x%x", snk_buf_size);
            audio_pipe_start(handle);
            if (sd.pipe_play.p_drain_buf == NULL)
            {
                sd.pipe_play.p_drain_buf = malloc(snk_buf_size);
            }
            sd.pipe_play.play_state = SD_PLAY_STATE_PLAY;
        }
        break;

    case AUDIO_PIPE_EVENT_STARTED:
        {
            src_play_pre_fill_data();
            T_PLAY_ROUTE play_route = app_src_play_get_play_route();
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
            if (play_route == PLAY_ROUTE_BIS || play_route == PLAY_ROUTE_CIS)
            {
                if (!lea_tx_mgr.timer_started)
                {
                    app_src_play_pipe_sd_lea_timer_start();
                }
            }
#endif
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_IND:
        {
            app_src_play_sd_pipe_handle_data_ind();
        }
        break;

    case AUDIO_PIPE_EVENT_DATA_FILLED:
        {
            sd.pipe_play.fill_state = SD_PLAY_STATE_FILLED;
            // src_play_check_and_fill_data();
            src_play_pre_fill_data();
        }
        break;

    case AUDIO_PIPE_EVENT_RELEASED:
        {
            app_src_play_sd_pipe_handle_release();
        }
        break;

    default:
        break;
    }
    return true;
}

void app_src_play_sd_pipe_start(T_FILE_FORMAT_INFO *file_format)
{
    T_AUDIO_FORMAT_INFO src_info = file_format->format_info;
    T_AUDIO_FORMAT_INFO snk_info;
    T_PLAY_ROUTE play_route = app_src_play_get_play_route();

    if (play_route == PLAY_ROUTE_A2DP)
    {
        // TODO: Support A2DP TX
        return;
    }
    else if (play_route == PLAY_ROUTE_HFP_AG)
    {
        // TODO: Support HFP TX
        return;
    }
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    else if (play_route == PLAY_ROUTE_BIS || play_route == PLAY_ROUTE_CIS)
    {
        if (!app_lea_get_data_format(LEA_CODEC_DIR_ENCODE, &snk_info))
        {
            APP_PRINT_ERROR0("app_src_play_sd_pipe_start: lc3 format info does not exist");
            return;
        }

        uint8_t chnl_cnt;
        if (snk_info.attr.lc3.chann_location == AUDIO_LOCATION_MONO)
        {
            chnl_cnt = 1;
        }
        else
        {
            chnl_cnt = __builtin_popcount(snk_info.attr.lc3.chann_location);
        }

        lea_tx_mgr.pkt_len = snk_info.attr.lc3.frame_length * chnl_cnt;
        lea_tx_mgr.p_lea_send_buf = calloc(1, lea_tx_mgr.pkt_len);
        if (lea_tx_mgr.p_lea_send_buf == NULL)
        {
            APP_PRINT_ERROR0("app_src_play_sd_pipe_start: p_lea_send_buf malloc fail");
            return;
        }
        lea_tx_mgr.target_threshold = 5; // frame_cnt
        lea_tx_mgr.pre_fill_num = 4;
        APP_PRINT_INFO2("app_src_play_sd_pipe_start: pkt_len %d, target_threshold %d",
                        lea_tx_mgr.pkt_len, lea_tx_mgr.target_threshold);
        if (snk_info.attr.lc3.frame_duration == AUDIO_LC3_FRAME_DURATION_10_MS)
        {
            lea_tx_mgr.timer_duration = 10000;
        }
        else
        {
            lea_tx_mgr.timer_duration = 7500;
        }
    }
#endif
    src_info.frame_num = 1;
    snk_info.frame_num = 1;
    if (sd.pipe_play.handle == NULL)
    {
        sd.pipe_play.handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL,
                                                src_info, snk_info,
                                                sd.pipe_play.volume,
                                                app_src_play_sd_pipe_cback);
    }

    sd.op_next_action = SD_STOPPED_IDLE_ACTION;
}

static bool app_src_play_sd_get_file_format(T_FILE_FORMAT_INFO *file_format)
{
    T_AUDIO_FORMAT_INFO format_info = {};
    FRAME_INFO playback_frame_info;

    uint32_t sample_rate = 0;
    uint8_t channel_mode = 0;
    uint32_t bit_rate = 0;

    if (sd.frame_content != NULL)
    {
        free(sd.frame_content);
    }
    sd.frame_content = (FRAME_CONTENT *)malloc(sizeof(FRAME_CONTENT));

    uint16_t u16_res = audio_fs_get_frame(sd.fs_handle, sd.frame_content);
    audio_fs_decode_get_frame_info(&playback_frame_info);
    if (u16_res != 0)
    {
        APP_PRINT_INFO1("app_src_play_sd_get_file_format: get frame fail %d", u16_res);
        return false;
    }

    if ((playback_frame_info.format == RTK) &&
        (playback_frame_info.format_info.rtk.rtkTransFormat == RTK_SBC))
    {
        format_info.type = AUDIO_FORMAT_TYPE_SBC;
        format_info.attr.sbc.sample_rate =
            playback_frame_info.format_info.rtk.rtk_trans_info.sbc.sampling_frequency;
        format_info.attr.sbc.block_length =
            playback_frame_info.format_info.rtk.rtk_trans_info.sbc.block_length;
        format_info.attr.sbc.subband_num = playback_frame_info.format_info.rtk.rtk_trans_info.sbc.subbands;
        format_info.attr.sbc.allocation_method =
            playback_frame_info.format_info.rtk.rtk_trans_info.sbc.allocation_method;
        format_info.attr.sbc.bitpool = playback_frame_info.format_info.rtk.rtk_trans_info.sbc.bitpool;
        switch (playback_frame_info.format_info.rtk.rtk_trans_info.sbc.channel_mode)
        {
        case 0:
            format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO; // Mono
            break;
        case 1:
            format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_DUAL; // Mono
            break;
        case 2:
            format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_STEREO; // Stereo
            break;
        case 3:
            format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO; // Stereo
            break;
        default:
            APP_PRINT_ERROR0("channel not supprot");
            break;
        }
        sample_rate = format_info.attr.sbc.sample_rate;
        APP_PRINT_TRACE6("app_src_playback_parameter_recfg: RTK_SBC, samplefre %d, channel_mode %d,"
                         "block_length %d, subbands %d, allocation_method %d, bitpool %d",
                         format_info.attr.sbc.sample_rate, format_info.attr.sbc.chann_mode,
                         format_info.attr.sbc.block_length, format_info.attr.sbc.subband_num,
                         format_info.attr.sbc.allocation_method, format_info.attr.sbc.bitpool);
    }
    else if ((playback_frame_info.format == AAC) ||
             ((playback_frame_info.format == RTK) &&
              (playback_frame_info.format_info.rtk.rtkTransFormat == RTK_LATM)) ||
             ((playback_frame_info.format == RTK) &&
              (playback_frame_info.format_info.rtk.rtkTransFormat == RTK_ADTS)))
    {
        format_info.type = AUDIO_FORMAT_TYPE_AAC;

        if ((playback_frame_info.format == RTK) &&
            (playback_frame_info.format_info.rtk.rtkTransFormat == RTK_ADTS))
        {
            format_info.attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_ADTS;

            sample_rate = playback_frame_info.format_info.rtk.rtk_trans_info.adts.sampling_frequency;
            channel_mode = playback_frame_info.format_info.rtk.rtk_trans_info.adts.channel_mode;
            bit_rate = playback_frame_info.format_info.rtk.rtk_trans_info.adts.bit_rate;
        }
        else if ((playback_frame_info.format == RTK) &&
                 (playback_frame_info.format_info.rtk.rtkTransFormat == RTK_LATM))
        {
            format_info.attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_LATM;

            sample_rate = playback_frame_info.format_info.rtk.rtk_trans_info.latm.sampling_frequency;
            channel_mode = playback_frame_info.format_info.rtk.rtk_trans_info.latm.channel_mode;
            bit_rate = 0;
        }
        else if (playback_frame_info.format == AAC)
        {
            format_info.attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_ADTS;

            sample_rate = playback_frame_info.format_info.aac.sampling_frequency;
            channel_mode = playback_frame_info.format_info.aac.channel_mode;
            bit_rate = 0;
        }

        format_info.attr.aac.sample_rate = sample_rate;
        format_info.attr.aac.bitrate = bit_rate;
        switch (channel_mode)
        {
        case AOT_SPECIFIC_CONFIG:
            format_info.attr.aac.chann_num = 1; // Mono
            break;
        case CHANNEL_1PCS:
        case CHANNEL_2PCS:
        case CHANNEL_3PCS:
        case CHANNEL_4PCS:
        case CHANNEL_5PCS:
        case CHANNEL_6PCS:
        case CHANNEL_7PCS:
            format_info.attr.aac.chann_num = 2; // Stereo
            break;
        default:
            APP_PRINT_ERROR0("channel not support");
            break;
        }
        APP_PRINT_TRACE4("app_src_playback_parameter_recfg: AAC,"
                         " transport_format %d, samplefrequency %d, channel_mode %d, bitrate %d",
                         format_info.attr.aac.transport_format, format_info.attr.aac.sample_rate,
                         format_info.attr.aac.chann_num, format_info.attr.aac.bitrate);
    }
    else if (playback_frame_info.format == MP3)
    {
        format_info.type = AUDIO_FORMAT_TYPE_MP3;
        format_info.frame_num = 1;
        format_info.attr.mp3.sample_rate = playback_frame_info.format_info.mp3.sampling_frequency;
        format_info.attr.mp3.bitrate = playback_frame_info.format_info.mp3.bit_rate;
        format_info.attr.mp3.version = playback_frame_info.format_info.mp3.version;
        format_info.attr.mp3.layer = playback_frame_info.format_info.mp3.layer;
        switch (playback_frame_info.format_info.mp3.channel_mode)
        {
        case 0:
            format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_STEREO;
            break;
        case 1:
            format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_JOINT_STEREO;
            break;
        case 2:
            format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_DUAL;
            break;
        case 3:
            format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_MONO;
            break;

        default:
            break;
        }
        sample_rate = format_info.attr.mp3.sample_rate;
        APP_PRINT_TRACE5("app_src_playback_parameter_recfg: MP3,"
                         "  samplefrequency %d, version %d, layer %d, channel_mode %d, bitrate %d",
                         format_info.attr.mp3.sample_rate, format_info.attr.mp3.version, format_info.attr.mp3.layer,
                         format_info.attr.mp3.chann_mode, format_info.attr.mp3.bitrate);
    }
    // sd.local_play.play_monitor.sample_rate = sample_rate;
    file_format->format_info = format_info;
    file_format->frame_size = sd.frame_content->length;
    file_format->frame_duration = playback_frame_info.format_info.mp3.frame_duration;
    file_format->sample_counts = playback_frame_info.format_info.mp3.sample_counts;

    return true;
}

static uint16_t app_src_playback_set_playlist(uint8_t play_mode)
{
    uint16_t res = 0;

    switch (play_mode)
    {
    case SINGLE_FILE:
        // fileInfo.g_file_cur not change
        if ((sd.playlist_idx < 1) ||
            (sd.playlist_idx > PLAYBACK_PLAYLIST_NUM))
        {
            sd.playlist_idx = 1;
        }
        break;
    case NEXT_FILE:
        sd.playlist_idx++;
        if ((sd.playlist_idx < 1) ||
            (sd.playlist_idx > PLAYBACK_PLAYLIST_NUM))
        {
            sd.playlist_idx = 1;
        }
        break;
    case PREVERSE_FILE:
        if (sd.playlist_idx == 1)
        {
            sd.playlist_idx = PLAYBACK_PLAYLIST_NUM;
        }
        else if ((sd.playlist_idx < 1) ||
                 (sd.playlist_idx > PLAYBACK_PLAYLIST_NUM))
        {
            sd.playlist_idx = 1;
        }
        else
        {
            sd.playlist_idx--;
        }
        break;
    case RANDOM_FILE:
        // sd.file_idx = rand() % (playlist_file_count - 1);
        break;

    default:
        sd.playlist_idx = 1;
        break;
    }

    res = audio_fs_set_playlist(sd.playlist_idx);
    APP_PRINT_TRACE3("app_src_playback_set_playlist: play_mode %d,file_idx %d, res 0x%x",
                     play_mode, sd.playlist_idx, res);

    return res;
}

static uint8_t app_src_playback_update_playlist(uint8_t playlist_mode)
{
    uint8_t res = 0;
    uint8_t playlist_cnt = 0;
    uint8_t playlist_mode_tmp = playlist_mode;
    uint16_t playlist_file_count = 0;

    do
    {
        res = app_src_playback_set_playlist(playlist_mode_tmp);
        if (res != AUDIO_FS_OK)
        {
            res = PLAYBACK_PLAYLIST_ERROR;
        }
        else
        {
            playlist_file_count = audio_fs_get_filecount_from_playlist();
            playlist_cnt++;
            if (((playlist_mode_tmp == SINGLE_FILE) && (res != 0)) || \
                ((playlist_mode_tmp == SINGLE_FILE) && (playlist_file_count == 0)))
            {
                playlist_mode_tmp = NEXT_FILE;
            }
        }
    }
    while (((playlist_cnt < PLAYBACK_PLAYLIST_NUM) && (res != 0)) || \
           ((playlist_cnt < PLAYBACK_PLAYLIST_NUM) && (playlist_file_count == 0)));

    if ((res != 0) || (playlist_cnt > PLAYBACK_PLAYLIST_NUM) || (playlist_file_count == 0))
    {
        res = PLAYBACK_PLAYLIST_ERROR;
    }
    APP_PRINT_TRACE3("app_src_playback_update_playlist: playlist_cnt %d, playlist_idx %d, file_cnt %d",
                     playlist_cnt, sd.playlist_idx, playlist_file_count);
    return res;
}

static uint8_t app_src_playback_get_index_file(T_PLAYBACK_FILE play_mode, uint16_t *index)
{
    //TODO: Not used currently, for multi file
    uint8_t res = PLAYBACK_SUCCESS;
    uint16_t playlist_file_count;

    playlist_file_count = audio_fs_get_filecount_from_playlist();
    switch (play_mode)
    {
    case SINGLE_FILE:
        if (sd.file_idx >= playlist_file_count)
        {
            sd.file_idx = 0;
        }
        break;
    case NEXT_FILE:
        sd.file_idx++;
        if (sd.file_idx >= playlist_file_count)
        {
            sd.file_idx = 0;
        }
        break;
    case PREVERSE_FILE:
        if (sd.file_idx == 0)
        {
            sd.file_idx = playlist_file_count - 1;
        }
        else if (sd.file_idx >= playlist_file_count)
        {
            sd.file_idx = 0;
        }
        else
        {
            sd.file_idx--;
        }
        break;
    case RANDOM_FILE:
        // sd.file_idx = rand() % (playlist_file_count - 1);
        break;
    case RETURN_IDX:
        *index = sd.file_idx;
        return res;

    default:
        sd.file_idx = 0;
        break;
    }

    *index = sd.file_idx;
    if (playlist_file_count == 0)
    {
        res = PLAYBACK_PLAYLIST_ERROR;
    }
    APP_PRINT_TRACE3("app_src_playback_get_index_file: play_mode %d,file_idx %d, total file %d",
                     play_mode, sd.file_idx, playlist_file_count);

    return res;
}

void app_src_playback_create_and_open_file(void)
{
    uint16_t res = PLAYBACK_SUCCESS;
    const TCHAR *driver_num = (const TCHAR *)_T("0:");

    res = f_mount(&record_fs, driver_num, 1);
    if (res)
    {
        APP_PRINT_ERROR1("app_src_playback_create_and_open_file: f_mount fail, res %d", res);
    }

    res = f_open(&record_file, (const TCHAR *)TEMP_FILE_NAME_STRING, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        APP_PRINT_ERROR1("app_src_playback_create_and_open_file: f_open fail, res %d", res);
        f_close(&record_file);
        return;
    }
    return;
}

uint8_t app_src_playback_write_file(uint8_t *buf, uint16_t len)
{
    uint8_t res = PLAYBACK_SUCCESS;
    uint32_t wrd_len;
    uint32_t write_file_result = FR_OK;

    write_file_result = f_write(&record_file, buf, len, &wrd_len);
    if (write_file_result != FR_OK)
    {
        APP_PRINT_ERROR1("app_src_playback_write_file FAIL, err %x", write_file_result);
        res = PLAYBACK_WRITE_ERROR;
    }
    return res;
}

void app_src_play_sd_file_close(void)
{
    if (is_record_save_file)
    {
        f_close(&record_file);
    }
    else
    {
        if (sd.fs_handle != NULL)
        {
            audio_fs_close(sd.fs_handle);
            sd.fs_handle = NULL;
        }
    }
}

static uint8_t app_src_playback_open_and_get_file_info(uint8_t play_mode, uint8_t *file_name,
                                                       uint16_t *p_name_len,
                                                       uint32_t *p_playback_offset)
{
    uint16_t res = PLAYBACK_SUCCESS;
    uint16_t file_index;
    uint8_t filename[FILE_NAME_LEN]  = {0};
    uint16_t name_len;
    uint8_t play_mode_tmp = play_mode;
    uint16_t open_file_cnt = 0;

    app_src_play_sd_file_close();

    // do
    // {
    // res = app_src_playback_get_index_file((T_PLAYBACK_FILE)play_mode_tmp, &file_index);
    // // if (res != PLAYBACK_SUCCESS)
    // // {
    // //     res = PLAYBACK_PLAYLIST_ERROR;
    // //     goto L_Return;
    // // }
    // res = audio_fs_get_fileinfo(file_index, filename, &name_len);
    // if (res != 0)
    // {
    //     res = PLAYBACK_PLAYLIST_ERROR;
    //     goto L_Return;
    // }
    // memcpy(file_name, filename, name_len);
    // *p_name_len = name_len;

    // init and open file system
    sd.fs_handle = audio_fs_open(sd_music_name,
                                 sd_music_name_len, FA_OPEN_EXISTING | FA_READ);
    if (sd.fs_handle == NULL)
    {
        open_file_cnt++;
        if (SINGLE_FILE == play_mode_tmp)
        {
            play_mode_tmp = NEXT_FILE;
        }
        res = PLAYBACK_PLAYLIST_ERROR;
    }
    // }
    // while ((sd.fs_handle == NULL) &&
    //        (open_file_cnt < audio_fs_get_filecount_from_playlist()));

    if (sd.fs_handle != NULL)
    {
        *p_playback_offset = audio_fs_get_file_offset(sd.fs_handle);
    }
    else
    {
        *p_playback_offset = 0;
    }
    APP_PRINT_TRACE2("app_src_playback_open_and_get_file_info: playback_offset 0x%x, name_len %d",
                     *p_playback_offset, *p_name_len);
    return res;
// L_Return:
//     APP_PRINT_ERROR1("app_src_playback_open_and_get_file_info fail, res 0x%x", res);
//     return res;
}

static void app_src_play_sd_sw_timer_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO2("app_src_play_sd_sw_timer_cb: timer_evt 0x%02x, param 0x%x", timer_evt,
                    param);

    switch (timer_evt)
    {
#if F_APP_SD_CARD_LOCALPLAY
    case APP_TIMER_SD_LOCAL_PUT_DATA:
        {
            app_src_playback_put_data_stop_timer(sd.local_play.play_monitor.frame_num);
        }
        break;

    case APP_TIMER_SD_DELAY_STOP_LOCAL:
        {
            app_stop_timer(&timer_idx_sd_delay_stop_local);
            app_src_play_sd_local_stop();
        }
        break;
#endif

    case APP_TIMER_SD_DELAY_STOP_PIPE:
        {
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
            // TODO: Only le audio need to delay stop?
            T_PLAY_ROUTE play_route = app_src_play_get_play_route();
            if (play_route == PLAY_ROUTE_BIS || play_route == PLAY_ROUTE_CIS)
            {
                uint32_t used_size = ring_buffer_get_data_count(&lea_tx_mgr.ring_buf);

                if (used_size < lea_tx_mgr.pkt_len)
                {
                    app_stop_timer(&timer_idx_sd_delay_stop_pipe);
                    app_src_play_sd_pipe_stop();
                }
                else
                {
                    app_src_play_sd_pipe_delay_stop(50);
                }
            }
#endif
        }

    default:
        break;
    }
}

static void app_src_sd_card_power_down(void)
{
    // TODO: Note used currently, may need to call when power off
    APP_PRINT_TRACE0("app_src_sd_card_power_down");
    audio_fs_sd_power_off();
}

static bool app_src_sd_card_power_down_check_idle(void)
{
    if (s_sd_pd_bitmap == APP_SD_POWER_DOWN_ENTER_CHECK_IDLE)
    {
        return true;
    }
    return false;
}

static void app_src_sd_card_power_down_disable(uint16_t bit)
{
    if ((s_sd_pd_bitmap & bit) == 0)
    {
        APP_PRINT_TRACE3("app_src_sd_card_power_down_disable: %08x %08x -> %08x", bit, s_sd_pd_bitmap,
                         (s_sd_pd_bitmap | bit));
    }

    if (app_src_sd_card_power_down_check_idle() == true)
    {
        audio_fs_sd_power_on();
    }

    s_sd_pd_bitmap |= bit;
}

static void app_src_sd_card_dlps_enable(uint16_t bit)
{
    if (s_sd_dlps_bitmap & bit)
    {
        APP_PRINT_TRACE3("app_src_sd_card_dlps_enable: %08x %08x -> %08x", bit, s_sd_dlps_bitmap,
                         (s_sd_dlps_bitmap & ~bit));
    }
    s_sd_dlps_bitmap &= ~bit;
}

void app_src_sd_card_dlps_disable(uint16_t bit)
{
    if ((s_sd_dlps_bitmap & bit) == 0)
    {
        APP_PRINT_TRACE3("app_src_sd_card_dlps_disable: %08x %08x -> %08x", bit, s_sd_dlps_bitmap,
                         (s_sd_dlps_bitmap | bit));
    }
    app_dlps_disable(APP_DLPS_ENTER_CHECK_PLAYBACK);

    s_sd_dlps_bitmap |= bit;
}

void app_src_play_sd_update_name(uint8_t *name, uint8_t name_len)
{
    sd_music_name_len = name_len > sizeof(sd_music_name) ? sizeof(sd_music_name) : name_len;
    memcpy(sd_music_name, name, sd_music_name_len);
}

// route in start
bool app_src_play_sd_start(uint8_t play_route)
{
    uint8_t err_code = 0;
    T_FILE_FORMAT_INFO file_format_info;
    uint32_t playback_offset = 0;
    uint16_t name_len = 0;

    //select a file TEMP_FILE_NAME_STRING
    if (sd.fs_handle == NULL)
    {
        // init file system and get sd.fs_handle
        uint8_t res = app_src_playback_open_and_get_file_info(SINGLE_FILE, sd_music_name,
                                                              &name_len, &playback_offset);
        if (res != PLAYBACK_SUCCESS)
        {
            err_code = 1;
            goto ERR;
        }
    }
    else
    {
        uint8_t *p_file_name = audio_fs_get_filename(sd.fs_handle);
        name_len = audio_fs_get_filenameLen(sd.fs_handle);
        // memcpy((uint8_t *)TEMP_FILE_NAME_STRING, p_file_name, name_len);
        playback_offset = audio_fs_get_file_offset(sd.fs_handle);
    }

    // read file header to get format
    if (app_src_play_sd_get_file_format(&file_format_info) == false)
    {
        err_code = 2;
        goto ERR;
    }

    if (play_route == PLAY_ROUTE_A2DP || play_route == PLAY_ROUTE_HFP_AG
#if BAP_BROADCAST_SOURCE
        || play_route == PLAY_ROUTE_BIS
#endif
#if BAP_UNICAST_CLIENT
        || play_route == PLAY_ROUTE_CIS
#endif
       )
    {
        app_src_play_sd_pipe_start(&file_format_info);
    }
#if F_APP_SD_CARD_LOCALPLAY
    else if (play_route == PLAY_ROUTE_LOCAL)
    {
        app_src_play_sd_local_start(&file_format_info);
    }
#endif
    else
    {
        err_code = 3;
        goto ERR;
    }
    app_src_sd_card_dlps_disable(APP_SD_DLPS_ENTER_CHECK_PLAYING);
    app_src_sd_card_power_down_disable(APP_SD_POWER_DOWN_ENTER_CHECK_PLAYBACK);
    return true;

ERR:
    app_src_play_sd_file_close();
    APP_PRINT_ERROR1("app_src_play_sd_start: err_code %d", -err_code);
    return false;
}

static void app_src_play_sd_pipe_stop(void)
{
    audio_fs_decode_deinit();
    if (sd.pipe_play.handle != NULL)
    {
        audio_pipe_release(sd.pipe_play.handle);
    }
    app_src_play_sd_file_close();
    app_src_sd_card_dlps_enable(APP_SD_DLPS_ENTER_CHECK_PLAYING);
}

void app_src_play_sd_stop(uint8_t play_route)
{
    if (play_route == PLAY_ROUTE_A2DP || play_route == PLAY_ROUTE_HFP_AG
#if BAP_BROADCAST_SOURCE
        || play_route == PLAY_ROUTE_BIS
#endif
#if BAP_UNICAST_CLIENT
        || play_route == PLAY_ROUTE_CIS
#endif
       )
    {
        app_src_play_sd_pipe_stop();
    }
#if F_APP_SD_CARD_LOCALPLAY
    else if (play_route == PLAY_ROUTE_LOCAL)
    {
        app_src_play_sd_local_stop();
    }
#endif
}

#if F_APP_SD_CARD_LOCALPLAY
// TODO: Support all route
void app_src_playback_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                     uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE4("app_src_playback_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u, play_state %d",
                     cmd_id, cmd_len, cmd_path, sd.local_play.play_state);

    uint8_t res = PLAYBACK_SUCCESS;
    // uint8_t *filename = _T("8kbps_8khz.mp3");
    uint16_t name_len = 0;
    uint32_t playback_offset = 0;

    switch (cmd_id)
    {
    case CMD_SRC_SD_PLAYBACK_FWD:
    case CMD_SRC_SD_PLAYBACK_BWD:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            if (cmd_id == CMD_SRC_SD_PLAYBACK_FWD)
            {
                sd.op_next_action = SD_STOPPED_SWITCH_FWD_ACTION;
            }
            else if (cmd_id == CMD_SRC_SD_PLAYBACK_BWD)
            {
                sd.op_next_action = SD_STOPPED_SWITCH_BWD_ACTION;
            }
            if (sd.local_play.play_state == SD_PLAY_STATE_PLAY)
            {
                app_src_play_sd_local_stop();
            }
            else
            {
                app_src_playback_stopped_start_next_action(sd.op_next_action);
            }
        }
        break;

    case CMD_SRC_SD_PLAYBACK_FWD_PLAYLIST:
    case CMD_SRC_SD_PLAYBACK_BWD_PLAYLIST:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            if ((AUDIO_TRACK_STATE_STARTED == sd.local_play.play_monitor.local_track_state) || \
                (AUDIO_TRACK_STATE_PAUSED == sd.local_play.play_monitor.local_track_state))
            {
                app_src_play_sd_local_stop();
                //close file
                app_src_play_sd_file_close();
            }

            if (cmd_id == CMD_SRC_SD_PLAYBACK_FWD_PLAYLIST)
            {
                res = app_src_playback_update_playlist(NEXT_FILE);
            }
            else if (cmd_id == CMD_SRC_SD_PLAYBACK_BWD_PLAYLIST)
            {
                res = app_src_playback_update_playlist(PREVERSE_FILE);
            }

            if (res == 0)
            {
                // single
                APP_PRINT_TRACE0("CMD_SRC_SD_PLAYBACK_FWD_PLAYLIST SINGLE");
            }
        }
        break;

    case CMD_SRC_PLAY_SET_SRC_FILE_INDEX:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            uint8_t file_len = cmd_ptr[2];
            uint8_t *file_name = &cmd_ptr[3];
            app_src_play_sd_update_name(file_name, file_len);
        }
        break;

    default:
        break;
    }
}
#endif

uint16_t audio_fs_check_init_result(void)
{
    return audio_fs_init_result;
}

#if F_APP_SD_CARD_PLAY
uint8_t audio_fs_get_local_music_files_num(void)
{
    uint8_t files_num = *(uint16_t *)(MUSIC_HEADER_BIN_ADDR + FS_HEADER_COUNT_OFFSET);
    return files_num;
}

void audio_fs_read_local_music_files_name_by_idx(uint8_t index)
{
    uint8_t music_files_num = *(uint16_t *)(MUSIC_HEADER_BIN_ADDR + FS_HEADER_COUNT_OFFSET);
    T_HEAD_INFO *header_info = (T_HEAD_INFO *)(MUSIC_HEADER_BIN_ADDR + FS_HEADER_INFO_START);

    uint8_t *name = (uint8_t *)(MUSIC_NAME_BIN_ADDR + ((header_info[index].offset)));
    sd_file_info.sd_file_name = name;
    sd_file_info.sd_length = header_info[index].length;
}

void app_audio_fs_report_local_music_files_info(void)
{
    uint8_t music_files_num;

    music_files_num = audio_fs_get_local_music_files_num();
    if (music_files_num != 0)
    {
        for (uint8_t i = 0; i < music_files_num; i++)
        {
            audio_fs_read_local_music_files_name_by_idx(i);

            if (sd_file_info.sd_file_name == NULL || sd_file_info.sd_length == 0)
            {
                APP_PRINT_ERROR0("Invalid app_audio_fs_report_local_music_files_info!!!");
                continue;
            }

            uint8_t temp_buff[sd_file_info.sd_length + 1];
            temp_buff[0] = sd_file_info.sd_length;
            memcpy(&temp_buff[1], sd_file_info.sd_file_name, sd_file_info.sd_length);
            app_report_event(CMD_PATH_UART, EVENT_AUDIO_FILE_READ_RESULT, 0, temp_buff,
                             sd_file_info.sd_length + 1);
        }
    }
}

void audio_fs_filter_del_header_info(uint8_t *header_in, uint16_t len_in, uint16_t *len_out)
{
    T_HEAD_INFO  *header_info;
    uint16_t offset = 0;
    *len_out = len_in;

    while (len_in >= sizeof(T_HEAD_INFO))
    {
        header_info = (T_HEAD_INFO *)(header_in + offset);
        if (header_info->isDeleted)
        {
            if (len_in > sizeof(T_HEAD_INFO))
            {
                memmove(header_in + offset, header_in + offset + sizeof(T_HEAD_INFO), len_in - sizeof(T_HEAD_INFO));
            }
            *len_out -= sizeof(T_HEAD_INFO);
        }
        else
        {
            offset += sizeof(T_HEAD_INFO);
        }

        len_in -= sizeof(T_HEAD_INFO);
    }

}

void audio_fs_fil_sync_to_flash(void)
{
    uint32_t s;

    for (uint8_t i = 0; i < MUSIC_NAME_BIN_SIZE / SECTOR_SIZE; i++)
    {
        fmc_flash_nor_erase(MUSIC_NAME_BIN_ADDR + i * SECTOR_SIZE, FMC_FLASH_NOR_ERASE_SECTOR);
    }

    uint32_t name_offset = 0;
    uint32_t name_size = audio_fs_get_name_list_size();
    uint32_t read_len = 0;

    APP_PRINT_INFO1("audio_fs_fil_sync_to_flash: name bin size %d", name_size);

    for (uint16_t i = 0; i < name_size / NAME_BUF_SIZE; i++)
    {
        audio_fs_read_name_list(i * NAME_BUF_SIZE, name_buf, NAME_BUF_SIZE, &read_len);
        APP_PRINT_INFO2("audio_fs_fil_sync_to_flash: name index %d, name %s", i, name_buf);
        fmc_flash_nor_write(MUSIC_NAME_BIN_ADDR + i * NAME_BUF_SIZE, name_buf, NAME_BUF_SIZE);
        name_offset += NAME_BUF_SIZE;
    }
    if (name_size % NAME_BUF_SIZE)
    {
        audio_fs_read_name_list(name_offset, name_buf, name_size % NAME_BUF_SIZE, &read_len);
        APP_PRINT_INFO1("audio_fs_fil_sync_to_flash: name %b", TRACE_BINARY(name_size % NAME_BUF_SIZE,
                                                                            name_buf));
        fmc_flash_nor_write(MUSIC_NAME_BIN_ADDR + name_offset, name_buf, name_size % NAME_BUF_SIZE);
    }

#ifndef TARGET_RTL8773DO
    s = os_lock();
    SCB_InvalidateDCache_by_Addr((uint32_t *)MUSIC_NAME_BIN_ADDR, 0xA000);
    os_unlock(s);
#endif

    uint32_t head_size = audio_fs_get_header_list_size();

    APP_PRINT_INFO1("audio_fs_fil_sync_to_flash: header bin size %d", head_size);

    for (uint8_t i = 0; i < MUSIC_HEADER_BIN_SIZE / SECTOR_SIZE; i++)
    {
        fmc_flash_nor_erase(MUSIC_HEADER_BIN_ADDR + i * SECTOR_SIZE, FMC_FLASH_NOR_ERASE_SECTOR);
    }

    uint16_t del_num = 0;
    uint32_t write_offset = MUSIC_HEADER_BIN_ADDR;
    uint32_t read_offset = 0;
    uint8_t header_cnt[FS_HEADER_INFO_START];
    uint16_t len_out = 0;

    if (head_size >= FS_HEADER_INFO_START)
    {
        audio_fs_read_header_list(read_offset, header_cnt, FS_HEADER_INFO_START, &read_len);
        write_offset += FS_HEADER_INFO_START;
        read_offset += FS_HEADER_INFO_START;

        for (uint16_t i = 0; i < (head_size - FS_HEADER_INFO_START) / NAME_BUF_SIZE; i++)
        {
            audio_fs_read_header_list(read_offset, name_buf, NAME_BUF_SIZE, &read_len);
            audio_fs_filter_del_header_info(name_buf, NAME_BUF_SIZE, &len_out);
            del_num += (NAME_BUF_SIZE - len_out) / sizeof(T_HEAD_INFO);
            fmc_flash_nor_write(write_offset, name_buf, len_out);
            write_offset += len_out;
            read_offset += NAME_BUF_SIZE;
        }
        if ((head_size - FS_HEADER_INFO_START) % NAME_BUF_SIZE)
        {
            uint16_t len_in = (head_size - FS_HEADER_INFO_START) % NAME_BUF_SIZE;
            audio_fs_read_header_list(read_offset, name_buf, len_in, &read_len);
            audio_fs_filter_del_header_info(name_buf, len_in, &len_out);
            del_num += (len_in - len_out) / sizeof(T_HEAD_INFO);
            fmc_flash_nor_write(write_offset, name_buf, len_out);
        }
    }
    else
    {
        audio_fs_read_header_list(read_offset, header_cnt, head_size, &read_len);
    }

    uint16_t *count = (uint16_t *)header_cnt;
    *count -= del_num;
    fmc_flash_nor_write(MUSIC_HEADER_BIN_ADDR, header_cnt, FS_HEADER_INFO_START);

#ifndef TARGET_RTL8773DO
    s = os_lock();
    SCB_InvalidateDCache_by_Addr((uint32_t *)MUSIC_HEADER_BIN_ADDR, 0x5000);
    os_unlock(s);
#endif
    APP_PRINT_INFO2("audio_fs_fil_sync_to_flash: del_num %d, Header bin %b", del_num,
                    TRACE_BINARY(head_size,
                                 (uint8_t *)MUSIC_HEADER_BIN_ADDR));
}
#endif

void app_src_playback_init(void)
{
    audio_fs_init_result = audio_fs_init((TCHAR *)_T("0:"), 1024, true, NULL);

#if F_APP_SD_CARD_PLAY
    if (audio_fs_init_result == AUDIO_FS_OK)
    {
        audio_fs_fil_sync_to_flash();
    }
#endif

    app_timer_reg_cb(app_src_play_sd_sw_timer_cb, &app_src_play_sd_timer_id);

#if F_APP_SD_CARD_LOCALPLAY
    audio_mgr_cback_register(app_src_playback_audio_policy_cback);
#endif

#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    uint8_t *p_buf = NULL;
    p_buf = (uint8_t *)calloc(1, LC3_RBUF_SIZE);
    ring_buffer_init(&lea_tx_mgr.ring_buf, p_buf, LC3_RBUF_SIZE);
    app_src_play_sd_lc3_timer_init();
#endif
}

#endif
