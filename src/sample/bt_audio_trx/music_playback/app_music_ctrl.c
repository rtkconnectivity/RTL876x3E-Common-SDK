/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
/* Includes -----------------------------------------------------------------*/
#include "trace.h"
#include "bt_bond.h"
#include "os_mem.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_dlps.h"
#include "app_timer.h"
#include "playback_audio_file.h"
#include "audio_fs_decode.h"
#include "playback_stream_ctrl.h"
#include "app_customer_cmd.h"
#include "app_music_ctrl.h"
#include "app_customer_audio_policy.h"
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
#include "a2dp_src_stream_ctrl.h"
#endif

/* Defines ------------------------------------------------------------------*/
/* Globals ------------------------------------------------------------------*/

static struct
{
    T_MUSIC_MODE          play_mode;
    T_MUSIC_STATE         play_state;
    uint8_t               play_flow;
    uint8_t               preq_pkts;
    uint16_t              frame_size;
    uint32_t              header_len;
    bool                  seamless_start;
    bool                  en_report_play_time;
    bool                  is_short_audio;

} music_info;

void app_music_is_short_audio_set(bool value)
{
    music_info.is_short_audio = value;
}

typedef enum
{
    APP_TIMER_DELAY_STOP,
} T_APP_MUSIC_TIMER;
static uint8_t timer_idx_music = 0;
static uint8_t app_music_time_id = 0;

static void app_music_seamless_switch(uint8_t play_mode);

void app_music_handle_xmmi_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                               uint16_t cmd_len, uint8_t *ack_pkt)
{
    switch (cmd_ptr[2])
    {
    case XM_MMI_MODE_LOCAL_PLAY:
        {
            /*switch to local playback mode*/
            app_music_set_mode(MUSIC_LOCAL_PLAY);
        }
        break;

    case XM_MMI_SEAMLESS_SWITCH_LOCAL_PLAY:
        {
            /*seamless switch to local playback mode*/
            if (music_info.play_mode == MUSIC_A2DP_SRC)
            {
                app_music_seamless_switch(MUSIC_LOCAL_PLAY);
            }
        }
        break;

#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    case XM_MMI_MODE_A2DP_SOURCE:
        {
            /*switch to a2dp source mode*/
            app_music_set_mode(MUSIC_A2DP_SRC);
        }
        break;

    case XM_MMI_SEAMLESS_SWITCH_A2DP_SRC:
        {
            /*seamless switch to a2dp source mode*/
            if (music_info.play_mode == MUSIC_LOCAL_PLAY)
            {
                app_music_seamless_switch(MUSIC_A2DP_SRC);
            }
        }
        break;
#endif
    case XM_MMI_ENABLE_REPORT_PLAY_TIME:
        {
            music_info.en_report_play_time = 1;
        }
        break;

    case XM_MMI_DISABLE_REPORT_PLAY_TIME:
        {
            music_info.en_report_play_time = 0;
        }
        break;

    default:
        break;
    }
}

T_MUSIC_STATE app_music_get_state(void)
{
    T_MUSIC_STATE play_state = MUSIC_STATE_IDLE;
    uint8_t state;

    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        state = playback_stream_get_state();
        switch (state)
        {
        case PLAYBACK_STATE_IDLE:
            play_state = MUSIC_STATE_IDLE;
            break;

        case PLAYBACK_STATE_PAUSE:
            play_state = MUSIC_STATE_PAUSE;
            break;

        case PLAYBACK_STATE_PLAY:
            play_state = MUSIC_STATE_PLAY;
            break;

        default:
            break;
        }
    }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        state = a2dp_src_stream_get_state();
        switch (state)
        {
        case A2DP_SRC_PLAY_STATE_IDLE:
            play_state = MUSIC_STATE_IDLE;
            break;

        case A2DP_SRC_PLAY_STATE_PAUSE:
            play_state = MUSIC_STATE_PAUSE;
            break;

        case A2DP_SRC_PLAY_STATE_PLAY:
            play_state = MUSIC_STATE_PLAY;
            break;

        default:
            break;
        }
    }
#endif

    return play_state;
}

uint8_t app_music_send_player_status(uint8_t status)
{
    APP_PRINT_INFO1("app_music_send_player_status status %d", status);
    if (status == MUSIC_PLAYER_PLAYING)
    {
        app_dlps_disable(APP_DLPS_ENTER_CHECK_MUSIC);
    }
    else if (status == MUSIC_PLAYER_STOPPED)
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_MUSIC);
    }
    else if (status == MUSIC_PLAYER_PAUSED)
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_MUSIC);
    }
    app_customer_report_play_status(status, APP_AUDIO_CODEC_TYPE_MP3);

    return 0;
}

void app_music_report_play_time(void)
{
    if (music_info.en_report_play_time == 0)
    {
        return;
    }
    uint32_t playtime = playback_audio_file_get_play_time();
    APP_PRINT_INFO1("app_music_report_play_time time %d", playtime);

    app_customer_report_play_time(playtime);
}

T_MUSIC_MODE app_music_get_mode(void)
{
    return music_info.play_mode;
}

uint8_t app_music_set_mode(uint8_t mode)
{
    APP_PRINT_INFO1("app_music_set_mode:0x%x", mode);

    if (app_music_get_state() != MUSIC_STATE_IDLE)
    {
        app_music_send_player_status(MUSIC_PLAYER_STOPPED);
    }

    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        playback_stream_ctrl_stop();
    }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        a2dp_src_stream_ctrl_stop();
    }
#endif

    music_info.play_mode = (T_MUSIC_MODE)mode;
    music_info.play_state = MUSIC_STATE_IDLE;
    music_info.play_flow = MUSIC_FLOW_IDLE;
    music_info.en_report_play_time = 0;
    playback_audio_file_set_play_time(0);

    return 0;
}

//*************************************************************************************//
//***********************************music_info api************************************//
//*************************************************************************************//

static bool app_music_jump_file_header(void)
{
    bool head_jump_ok = false;
    uint8_t head_type = 0xFF;
    uint32_t read_len = 0;
    uint32_t ring_buf_size = stream_get_data_size();

    APP_PRINT_WARN2("app_watch_music_judge_file_header, header len: 0x%x, ring_buf_size: 0x%x",
                    music_info.header_len, ring_buf_size);

    if (ring_buf_size <= 0)
    {
        return false;
    }

    if (music_info.header_len == 0)
    {
        head_type = playback_audio_file_get_header(&music_info.header_len);
        switch (head_type)
        {
        case PLAYBACK_AF_ERR_HEADER:
        case PLAYBACK_AF_NOT_HEADER:
            ring_buf_size = stream_get_data_size();
            stream_remove_data(ring_buf_size, &read_len);
            music_info.header_len -= read_len;
            head_jump_ok = false;
            break;

        case PLAYBACK_AF_IS_HEADER:
            if (stream_get_data_size() > music_info.header_len)
            {
                stream_remove_data(music_info.header_len, &read_len);
                music_info.header_len -= read_len;
                head_jump_ok = true;
            }
            else if (music_info.header_len > STREAM_BUF_HIGH_LEVEL)
            {
                ring_buf_size = stream_get_data_size();
                stream_remove_data(ring_buf_size, &read_len);
                music_info.header_len -= read_len;
                head_jump_ok = false;
            }
            break;

        case PLAYBACK_AF_DATA_FRAME:
            head_jump_ok = true;
            break;

        default:
            break;
        }
    }
    else
    {
        ring_buf_size = stream_get_data_size();
        if (ring_buf_size >= music_info.header_len)
        {
            stream_remove_data(music_info.header_len, &read_len);
            music_info.header_len -= read_len;
        }
        else
        {
            stream_remove_data(ring_buf_size, &read_len);
            music_info.header_len -= read_len;
        }

        if (music_info.header_len == 0)
        {
            head_jump_ok = true;
        }
    }

    return head_jump_ok;
}

static uint32_t app_music_play_get_threshold(void)
{
    uint32_t start_level = 0;

    if (music_info.is_short_audio)
    {
        start_level = (music_info.preq_pkts) * music_info.frame_size;
    }
    else
    {
        uint8_t frame_num_max = STREAM_BUF_HIGH_LEVEL / music_info.frame_size;

        if (music_info.frame_size > 2048)
        {
            start_level = STREAM_BUF_HIGH_LEVEL;
        }
        else if (music_info.frame_size > 1024)
        {
            if (frame_num_max > 10)
            {
                start_level = 10 * music_info.frame_size;
            }
            else
            {
                start_level = STREAM_BUF_HIGH_LEVEL;
            }
        }
        else
        {
            start_level = (music_info.preq_pkts + 8) * music_info.frame_size;
        }
    }

    APP_PRINT_INFO2("app_music_play_get_threshold: 0x%x, is_short_audio: %d", start_level,
                    music_info.is_short_audio);

    return start_level;
}

static bool app_music_enter_state(uint8_t state)
{
    bool ret_state = false;

    switch (state)
    {
    case MUSIC_FLOW_IDLE:
        {
            ret_state = true;
        }
        break;

    case MUSIC_FLOW_JUMP_HEAD:
        {
            ret_state = app_music_jump_file_header();
        }
        break;

    case MUSIC_FLOW_CREAT_TRACK:
        {
            uint16_t u16_res = 0;
            T_PLAYBACK_AF_FORMAT_INFO get_fmt_info;
            T_PLAY_SET_INFO set_play_info;
            music_info.header_len = 0;

            u16_res = audio_fs_decode_before_get_frame(NULL);
            if (u16_res != 0)
            {
                ret_state = false;
                break;
            }
            u16_res = playback_audio_file_get_audio_info(&get_fmt_info);
            if (u16_res != 0)
            {
                ret_state = false;
                break;
            }
            else
            {
                music_info.frame_size = get_fmt_info.frame_size;
                playback_stream_get_music_info(get_fmt_info, &set_play_info);
                music_info.preq_pkts = set_play_info.preq_pkts;
                ret_state = true;
            }
        }
        break;

    case MUSIC_FLOW_START:
        {
            if (music_info.play_mode == MUSIC_LOCAL_PLAY)
            {
                playback_stream_ctrl_start();
            }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
            else if (music_info.play_mode == MUSIC_A2DP_SRC)
            {
                a2dp_src_stream_ctrl_start();
            }
#endif
            ret_state = true;
        }
        break;

    default:
        ret_state = true;
        break;
    }

    return ret_state;
}

uint8_t app_music_create(void)
{
    uint8_t res = MUSIC_SUCCESS;

    APP_PRINT_TRACE1("app_music_create ++, memory: 0x%x", mem_peek());

    app_dlps_disable(APP_DLPS_ENTER_CHECK_MUSIC);

    audio_fs_decode_init();
    stream_clear_data();
    app_music_is_short_audio_set(false);
    music_info.header_len = 0;

    return res;
}

void app_music_set_flow_state(uint8_t flow_state)
{
    music_info.play_flow = flow_state;
}

uint8_t app_music_start(void)
{
    uint8_t res = MUSIC_STATE_ERROR;
    APP_PRINT_INFO1("app_music_start: play_flow:%d", music_info.play_flow);

    if (music_info.play_flow == MUSIC_FLOW_IDLE)
    {
        music_info.play_flow = MUSIC_FLOW_JUMP_HEAD;
    }

    if ((music_info.play_flow == MUSIC_FLOW_JUMP_HEAD) &&
        (stream_get_data_size() > STREAM_BUF_CHECK_LEVEL ||
         PLAYBACK_AUDIO_FILE_STOPPING == playback_audio_file_get_state()))
    {
        if (app_music_enter_state(MUSIC_FLOW_JUMP_HEAD))
        {
            music_info.play_flow = MUSIC_FLOW_CREAT_TRACK;
            res = MUSIC_NEXT_STATE_ERROR;
        }
    }

    if ((music_info.play_flow == MUSIC_FLOW_CREAT_TRACK) &&
        (stream_get_data_size() > STREAM_BUF_CHECK_LEVEL ||
         PLAYBACK_AUDIO_FILE_STOPPING == playback_audio_file_get_state()))
    {
        if (app_music_enter_state(MUSIC_FLOW_CREAT_TRACK))
        {
            music_info.play_flow = MUSIC_FLOW_START;
            res = MUSIC_NEXT_STATE_ERROR;
        }
    }

    if (music_info.play_flow == MUSIC_FLOW_START)
    {
        if (stream_get_data_size() > app_music_play_get_threshold())
        {
            if (app_music_enter_state(MUSIC_FLOW_START))
            {
                res = MUSIC_SUCCESS;
                music_info.play_flow = MUSIC_FLOW_STARTED;
                app_music_send_player_status(MUSIC_PLAYER_PLAYING);
            }
        }
    }

    return res;
}

uint8_t app_music_pause(void)
{
    uint8_t res = MUSIC_SUCCESS;

    APP_PRINT_INFO0("app_music_pause ++");
    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        playback_stream_ctrl_stop();
    }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        a2dp_src_stream_ctrl_stop();
    }
#endif
    app_music_send_player_status(MUSIC_PLAYER_PAUSED);
    music_info.play_flow = MUSIC_FLOW_IDLE;

    return res;
}

uint8_t app_music_resume(void)
{
    uint8_t res = MUSIC_SUCCESS;

    APP_PRINT_INFO0("app_music_resume ++");

    audio_fs_decode_before_get_frame(NULL);
    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        playback_stream_ctrl_start();
    }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        a2dp_src_stream_ctrl_start();
    }
#endif
    app_music_send_player_status(MUSIC_PLAYER_PLAYING);

    return res;
}

uint8_t app_music_close(void)
{
    uint8_t res = MUSIC_SUCCESS;
    APP_PRINT_INFO0("app_music_close ++");

    audio_fs_decode_deinit();

    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        playback_stream_ctrl_stop();
    }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        a2dp_src_stream_ctrl_stop();
    }
#endif
    app_music_send_player_status(MUSIC_PLAYER_STOPPED);
    music_info.play_flow = MUSIC_FLOW_IDLE;

    return res;
}

void app_music_volume_up(void)
{
    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        playback_stream_volume_up();
    }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        a2dp_src_stream_volume_up();
    }
#endif
}

void app_music_volume_down(void)
{
    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        playback_stream_volume_down();
    }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        a2dp_src_stream_volume_down();
    }
#endif
}

bool app_music_volume_set(uint8_t volume)
{
    bool ret = false;
    if (music_info.play_mode == MUSIC_LOCAL_PLAY || music_info.play_mode == MUSIC_IDLE)
    {
        ret = playback_stream_volume_set(volume);
    }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        ret = a2dp_src_stream_volume_set(volume);
    }
#endif
    return ret;
}

uint8_t app_music_volume_get(void)
{
    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        return playback_stream_volume_get();
    }
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        return a2dp_src_stream_volume_get();
    }
#endif
    else
    {
        return 0;
    }
}

//*************************************************************************************//
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
static void app_music_seamless_switch(uint8_t play_mode)
{
    if (play_mode == music_info.play_mode)
    {
        return;
    }
    APP_PRINT_TRACE3("app_music_seamless_switch cur_mode:%d, status:%d, to mode:%d",
                     music_info.play_mode, app_music_get_state(), play_mode);
    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        // cannot use app_music_set_mode();  it need not notify master
        if (app_music_get_state() == MUSIC_STATE_PLAY)
        {
            playback_stream_ctrl_stop();
            //a2dp_src_stream_ctrl_start();
            music_info.seamless_start = true;
        }
        music_info.play_mode = MUSIC_A2DP_SRC;
    }
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        if (app_music_get_state() == MUSIC_STATE_PLAY)
        {
            a2dp_src_stream_ctrl_stop();
            //playback_stream_ctrl_start();
            music_info.seamless_start = true;
        }
        music_info.play_mode = MUSIC_LOCAL_PLAY;
    }
    music_info.play_state = MUSIC_STATE_IDLE;
    music_info.play_flow = MUSIC_FLOW_IDLE;
}

void app_music_seamless_start(void)
{
    if (music_info.seamless_start == false)
    {
        return;
    }
//    audio_fs_decode_before_get_frame(NULL);
    if (music_info.play_mode == MUSIC_LOCAL_PLAY)
    {
        playback_stream_ctrl_start();
    }
    else if (music_info.play_mode == MUSIC_A2DP_SRC)
    {
        a2dp_src_stream_ctrl_start();
    }
    music_info.seamless_start = false;
}

#if 0
static void app_music_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_A2DP_CONN_CMPL:
        {
            app_music_seamless_switch(MUSIC_A2DP_SRC);
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            app_music_seamless_switch(MUSIC_LOCAL_PLAY);
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
        APP_PRINT_INFO1("app_music_bt_cback: event_type 0x%04x", event_type);
    }
}
#endif
#endif

//------------------------timer------------------------------------//
static void app_music_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_music_timeout_cb: timer_evt 0x%02x, param 0x%x", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_DELAY_STOP:
        {
            app_stop_timer(&timer_idx_music);
            app_music_close();
            app_music_report_play_time();
        }
        break;

    default:
        break;
    }
}

void app_music_delay_stop(uint16_t time_ms)
{
    uint16_t delay_ms = time_ms;
    APP_PRINT_TRACE1("app_music_delay_stop, time_ms:%d", time_ms);
    delay_ms = time_ms + 50;
    app_start_timer(&timer_idx_music, "music stop", app_music_time_id,
                    APP_TIMER_DELAY_STOP, 0, false, delay_ms);
}

//========================timer end===============================//

void app_music_init(void)
{
    memset(&music_info, 0, sizeof(music_info));

    playback_audio_file_init();
    playback_stream_ctrl_init();
#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    a2dp_src_stream_ctrl_init();
#if 0
    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        bt_mgr_cback_register(app_music_bt_cback);
    }
#endif
    playback_stream_ctrl_stop_complete_hook = app_music_seamless_start;
    a2dp_src_stream_ctrl_stop_complete_hook = app_music_seamless_start;
#endif
    playback_audio_report_playtime_hook = app_music_report_play_time;
    playback_stream_ctrl_delay_stop_hook = app_music_delay_stop;
    a2dp_src_stream_ctrl_delay_stop_hook = app_music_delay_stop;
    app_timer_reg_cb(app_music_timeout_cb, &app_music_time_id);
}
#endif

