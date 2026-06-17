/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "audio.h"
#include "app_cfg.h"
#include "app_dsp_cfg.h"
#include "app_record.h"
#include "app_main.h"
#include "audio_track.h"
#include "trace.h"
#include "pm.h"
#include "os_timer.h"
#include "app_report.h"
#include "app_timer.h"
#include "app_audio_policy.h"
#include "bt_types.h"
#include "app_customer_audio_policy.h"
#include "app_src_playback.h"

typedef enum
{
    PLAY_BUF_NORMAL,
    PLAY_BUF_LOW,
    PLAY_BUF_HIGH,
} PLAY_BUF_STATE;

typedef enum
{
    RC_PLAY_STOPPED,
    RC_PLAY_ING,
    RC_PLAY_CLOSING,
} RC_PLAY_STATE;


static struct
{
    struct
    {
        T_AUDIO_TRACK_HANDLE    handle;
        uint8_t                 default_volume;
    } record;

    struct
    {
        T_AUDIO_TRACK_HANDLE    handle;
        RC_PLAY_STATE           play_state;
        uint8_t                 volume;
        PLAY_BUF_STATE          buf_state;
        uint16_t                frame_duration_ms;
    } play;

    struct
    {
        uint8_t             id;
        uint8_t             sp_idx;
        uint8_t             sp_stop_idx;
    } timer;

} mic_record =
{
    .record =
    {
        .handle = NULL,
        .default_volume = 10,
    },

    .play =
    {
        .handle = NULL,
        .volume = 12,
        .play_state = RC_PLAY_STOPPED,
        .buf_state = PLAY_BUF_NORMAL,
        .frame_duration_ms = 0,
    },

    .timer =
    {
        .id = 0,
        .sp_idx = 0,
        .sp_stop_idx = 0,
    }
};

/*============================================================================*
  *                                        Variables
  *============================================================================*/

typedef enum
{
    TIMER_SP,
    TIMER_SP_STOP,
} TIMER_ID;

#if F_APP_SD_CARD_PLAY
uint8_t *write_buf;
#endif

#if 0
static void mic_dump_record_data(const char *title, uint8_t *record_data_buf, uint32_t data_len)
{
    const uint32_t bat_num = 8;
    uint32_t times = data_len / bat_num;
    uint32_t residue = data_len % bat_num;
    uint8_t *residue_buf = record_data_buf + times * bat_num;

    APP_PRINT_TRACE3("ama_dump_record_data: data_len %d, times %d, residue %d", data_len,
                     times, residue);
    APP_PRINT_TRACE2("ama_dump_record_data: record_data_buf is 0x%08x, residue_buf is 0x%08x\r\n",
                     (uint32_t)record_data_buf,
                     (uint32_t)residue_buf);

    APP_PRINT_TRACE1("@@@@@@@@@@@@@@@@@@@@@%s@@@@@@@@@@@@@@@@@@@@@@@@@@@", TRACE_STRING(title));

    for (int32_t i = 0; i < times; i++)
    {
        APP_PRINT_TRACE8("ama_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         record_data_buf[i * bat_num], record_data_buf[i * bat_num + 1], record_data_buf[i * bat_num + 2],
                         record_data_buf[i * bat_num + 3],
                         record_data_buf[i * bat_num + 4], record_data_buf[i * bat_num + 5],
                         record_data_buf[i * bat_num + 6],
                         record_data_buf[i * bat_num + 7]);
    }

    switch (residue)
    {
    case 1:
        APP_PRINT_TRACE1("ama_dump_record_data: 0x%02x\r\n", residue_buf[0]);
        break;
    case 2:
        APP_PRINT_TRACE2("ama_dump_record_data: 0x%02x, 0x%02x\r\n", residue_buf[0], residue_buf[1]);
        break;
    case 3:
        APP_PRINT_TRACE3("ama_dump_record_data: 0x%02x, 0x%02x, 0x%02x\r\n", residue_buf[0], residue_buf[1],
                         residue_buf[2]);
        break;
    case 4:
        APP_PRINT_TRACE4("ama_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n", residue_buf[0],
                         residue_buf[1], residue_buf[2], residue_buf[3]);
        break;
    case 5:
        APP_PRINT_TRACE5("ama_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4]);
        break;
    case 6:
        APP_PRINT_TRACE6("ama_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4], residue_buf[5]);
        break;
    case 7:
        APP_PRINT_TRACE7("ama_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4], residue_buf[5],
                         residue_buf[6]);
        break;

    default:
        break;
    }

    APP_PRINT_TRACE0("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
}
#else
#if 0
static void mic_dump_record_data(const char *title, uint8_t *record_data_buf, uint32_t data_len)
{
}
#endif
#endif

static bool record_read_cb(T_AUDIO_TRACK_HANDLE  handle,
                           uint32_t             *timestamp,
                           uint16_t             *seq_num,
                           T_AUDIO_STREAM_STATUS *status,
                           uint8_t              *frame_num,
                           void                 *buf,
                           uint16_t              required_len,
                           uint16_t             *actual_len)
{
    //APP_PRINT_TRACE2("app_mic_record_read_cb: buf 0x%08x, required_len %d", buf, required_len);

    {
        //mic_dump_record_data("app_mic_record_read_cb", buf, required_len);
        unsigned int actual_write_len;
        uint8_t res;

        if (mic_record.record.handle)
        {
            app_report_event(CMD_PATH_UART, EVENT_XM_RECORDING_DATA, 0, buf, required_len);
            //APP_PRINT_INFO2("record data write: data_len %d, actual len %d", required_len, actual_write_len);

#if F_APP_SD_CARD_PLAY
            write_buf = malloc(required_len + 4);
            memset(write_buf, 0, required_len + 4);
            write_buf[0] = 0;
            write_buf[1] = 0;
            write_buf[2] = (required_len >> 8) & 0xFF;
            write_buf[3] = required_len & 0xFF;
            memcpy(&write_buf[4], buf, required_len);
            res = app_src_playback_write_file(write_buf, required_len + 4);
            free(write_buf);
            APP_PRINT_INFO1("record_read_cb, res %x", res);
#endif

        }

    }

    *actual_len = required_len;

    return true;
}

static void record_start(T_AUDIO_FORMAT_INFO format_info)
{
    if (mic_record.record.handle != NULL)
    {
        APP_PRINT_ERROR0("app_customer_mic_record record_start: already started");
        return;
    }

    mic_record.record.handle = audio_track_create(AUDIO_STREAM_TYPE_RECORD,
                                                  AUDIO_STREAM_MODE_NORMAL,
                                                  AUDIO_STREAM_USAGE_LOCAL,
                                                  format_info,
                                                  0,
                                                  mic_record.record.default_volume,
                                                  AUDIO_DEVICE_IN_MIC,
                                                  NULL,
                                                  record_read_cb);

    if (mic_record.record.handle == NULL)
    {
        APP_PRINT_ERROR0("app_customer_mic_record record_start: handle is NULL");
        return;
    }

    bt_power_mode_set(BTPOWER_ACTIVE);
    audio_track_start(mic_record.record.handle);
}

#if 0
static void force_stop_recording(void)
{
    if (mic_record.record.handle)
    {
        audio_track_release(mic_record.record.handle);
        mic_record.record.handle = NULL;
    }
}
#endif

static void record_stop(void)
{
    if (mic_record.record.handle)
    {
        audio_track_release(mic_record.record.handle);
        mic_record.record.handle = NULL;
    }
}

static bool is_recording(void)
{
    return (mic_record.record.handle != NULL);
}

#if 0
static uint8_t record_play_send_player_status(uint8_t status)
{
    app_customer_report_play_status(status, APP_AUDIO_CODEC_TYPE_PCM);
    return 0;
}
#endif

static void record_play_start(T_AUDIO_FORMAT_INFO format_info)
{
    uint16_t frame_len = 0;
    uint16_t frame_duration_ms = 20;
    uint16_t play_latency = 0;
    uint16_t low_level = 30;
    uint16_t upper_level = 200;
    uint32_t device = app_db.playback_device;

    mic_record.play.handle = audio_track_create(AUDIO_STREAM_TYPE_PLAYBACK,
                                                AUDIO_STREAM_MODE_NORMAL,
                                                AUDIO_STREAM_USAGE_LOCAL,
                                                format_info,
                                                mic_record.play.volume,
                                                0,
                                                device,
                                                NULL,
                                                NULL);

    if (mic_record.play.handle == NULL)
    {
        APP_PRINT_ERROR0("app_customer_mic_record record_play_start: handle is NULL");
        return;
    }

    if (format_info.type == AUDIO_FORMAT_TYPE_PCM)
    {
        // (1024/(16K*16bit/8))=32ms
        frame_len = format_info.attr.pcm.frame_length;
        frame_duration_ms = frame_len / (format_info.attr.pcm.sample_rate / 1000) /
                            (format_info.attr.pcm.bit_width / 8);
        play_latency = frame_duration_ms * 6;
        low_level = frame_duration_ms * 3;
        upper_level = frame_duration_ms * 12;
    }
    else if (format_info.type == AUDIO_FORMAT_TYPE_OPUS)
    {
        frame_duration_ms = 20;
        play_latency = frame_duration_ms * 6;
        low_level = frame_duration_ms * 3;
        upper_level = frame_duration_ms * 12;
    }

    audio_track_latency_set(mic_record.play.handle, play_latency, true);
    audio_track_threshold_set(mic_record.play.handle, upper_level, low_level);
    if (audio_track_start(mic_record.play.handle))
    {
        APP_PRINT_TRACE0("app_customer_mic_record record_play_start: audio_track_start success");
    }
    else
    {
        APP_PRINT_ERROR0("app_customer_mic_record record_play_start: audio_track_start fail");
    }
    mic_record.play.frame_duration_ms = frame_duration_ms;
    mic_record.play.buf_state = PLAY_BUF_NORMAL;
}

static void record_play_stop(void)
{
    if (mic_record.play.handle)
    {
        audio_track_release(mic_record.play.handle);
        mic_record.play.handle = NULL;
    }
}


static void timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_customer_mic_record timeout_cb: timer_id 0x%02x, timer_chann %d",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case TIMER_SP:
        {
            app_stop_timer(&mic_record.timer.sp_idx);
            app_audio_tone_type_play(TONE_POWER_OFF, false, false); // mcuconfig TONE_POWER_OFF set to saopin
        }
        break;

    case TIMER_SP_STOP:
        {
            app_stop_timer(&mic_record.timer.sp_stop_idx);
            record_stop();
            app_report_event(CMD_PATH_UART, EVENT_XM_REOCRD_SP_STOP, 0, NULL, 0);
        }
        break;

    default:
        break;
    }
}


static void record_play_request_frame(void)
{
    app_report_event(CMD_PATH_UART, EVENT_XM_RECORD_PLAY_REQ_FRAME, 0, NULL, 0);
}


static void audio_mgr_cb(T_AUDIO_EVENT event_type, void *event_buf,
                         uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    if (mic_record.record.handle == NULL && mic_record.play.handle == NULL)
    {
        return;
    }

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            APP_PRINT_INFO1("app_customer_mic_record audio_mgr_cb: track_state_changed.state %d",
                            param->track_state_changed.state);

            switch (param->track_state_changed.state)
            {
            case AUDIO_TRACK_STATE_STARTED:
                {
                    if (mic_record.play.handle)
                    {
                        record_play_request_frame();
                    }

                }
                break;

            case AUDIO_TRACK_STATE_STOPPED:
                {
                    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
                }
                break;

            default:
                break;
            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_LOW:
        {
            if (mic_record.play.handle)
            {
                record_play_request_frame();
                mic_record.play.buf_state = PLAY_BUF_LOW;
            }
        }
        break;

    case AUDIO_EVENT_TRACK_BUFFER_HIGH:
        {
            if (mic_record.play.handle)
            {
                mic_record.play.buf_state = PLAY_BUF_HIGH;
            }
        }
        break;

    default:
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_customer_mic_record audio_mgr_cb: event_type 0x%04x", event_type);
    }
}


static void record_play_data_write(uint8_t *buf, uint16_t len)
{
    static uint16_t  seq_num = 0;
    uint16_t written_len;
    uint16_t level_ms = 0;

    if (mic_record.play.handle == NULL)
    {
        return;
    }

    seq_num++;
    audio_track_write(mic_record.play.handle, 0xFFFFFFFF,
                      seq_num,
                      AUDIO_STREAM_STATUS_CORRECT,
                      1,
                      buf,
                      len,
                      &written_len);

    if (audio_track_buffer_level_get(mic_record.play.handle, &level_ms))
    {
        if (0 < level_ms && level_ms < 3 * mic_record.play.frame_duration_ms)
        {
            record_play_request_frame();
        }
    }
}

bool app_record_play_volume_set(uint8_t volume)
{
    uint8_t max_volume = app_dsp_cfg_vol.playback_volume_max;
    uint8_t min_volume = app_dsp_cfg_vol.playback_volume_min;
    bool ret = false;
    if (volume > max_volume)
    {
        volume = max_volume;
    }
    APP_PRINT_TRACE1("app_record_play_volume_set vol:%d", volume);
    if (mic_record.play.handle != NULL)
    {
        if ((volume <= max_volume) && (volume >= min_volume))
        {
            ret = audio_track_volume_out_set(mic_record.play.handle, volume);
            mic_record.play.volume = volume;
        }
    }
    return ret;
}

uint8_t app_record_play_volume_get(void)
{
    return mic_record.play.volume;
}

static uint8_t record_play_buf_state(void)
{
    return mic_record.play.buf_state;
}

void app_record_init(void)
{
    app_timer_reg_cb(timeout_cb, &mic_record.timer.id);
    audio_mgr_cback_register(audio_mgr_cb);
}


static T_AUDIO_FORMAT_INFO parse_audio_format(uint8_t *cmd_payload)
{
#define AUDIO_TYPE_POS      (0)
    T_AUDIO_FORMAT_INFO format_info = {.type = (T_AUDIO_FORMAT_TYPE)cmd_payload[AUDIO_TYPE_POS]};



    switch (format_info.type)
    {
    case AUDIO_FORMAT_TYPE_PCM:
        {
            enum PCM_POS
            {
                SAMPLE_RATE_POS = AUDIO_TYPE_POS + 1,
                FRAME_LEN_POS = SAMPLE_RATE_POS + 4,
                CHANN_NUM_POS = FRAME_LEN_POS + 2,
                BIT_WIDTH_POS = CHANN_NUM_POS + 1,
            };

            LE_ARRAY_TO_UINT32(format_info.attr.pcm.sample_rate, &cmd_payload[SAMPLE_RATE_POS]);
            LE_ARRAY_TO_UINT16(format_info.attr.pcm.frame_length, &cmd_payload[FRAME_LEN_POS]);
            format_info.attr.pcm.chann_num = cmd_payload[CHANN_NUM_POS];
            format_info.attr.pcm.bit_width = cmd_payload[BIT_WIDTH_POS];
            APP_PRINT_TRACE4("app_customer_mic_record parse_audio_format: pcm, sample_rate %d, frame_len %d, chann_num %d, bit_width %d",
                             format_info.attr.pcm.sample_rate, format_info.attr.pcm.frame_length, format_info.attr.pcm.chann_num,
                             format_info.attr.pcm.bit_width);
        }
        break;

    case AUDIO_FORMAT_TYPE_OPUS:
        {
            enum OPUS_POS
            {
                SAMPLE_RATE_POS = AUDIO_TYPE_POS + 1,
                CHANN_NUM_POS = SAMPLE_RATE_POS + 4,
                CBR_POS = CHANN_NUM_POS + 1,
                CVBR_POS = CBR_POS + 1,
                MODE_POS = CVBR_POS + 1,
                COMPLEXITY_POS = MODE_POS + 1,
                DURATION_POS = COMPLEXITY_POS + 1,
                BITRATE_POS = DURATION_POS + 1,
            };
            LE_ARRAY_TO_UINT32(format_info.attr.opus.sample_rate, &cmd_payload[SAMPLE_RATE_POS]);
            format_info.attr.opus.chann_num = cmd_payload[CHANN_NUM_POS];
            format_info.attr.opus.cbr = cmd_payload[CBR_POS];
            format_info.attr.opus.cvbr = cmd_payload[CVBR_POS];
            format_info.attr.opus.mode = cmd_payload[MODE_POS];
            format_info.attr.opus.complexity = cmd_payload[COMPLEXITY_POS];
            format_info.attr.opus.frame_duration = (T_AUDIO_OPUS_FRAME_DURATION)cmd_payload[DURATION_POS];
            LE_ARRAY_TO_UINT32(format_info.attr.opus.bitrate, &cmd_payload[BITRATE_POS]);

            APP_PRINT_TRACE8("app_customer_mic_record parse_audio_format: opus, sample_rate %d, chann_num %d, cbr %d, cvbr %d, mode %d, complexity %d, duration %d, bitrate %d",
                             format_info.attr.opus.sample_rate, format_info.attr.opus.chann_num, format_info.attr.opus.cbr,
                             format_info.attr.opus.cvbr, format_info.attr.opus.mode,
                             format_info.attr.opus.complexity, format_info.attr.opus.frame_duration,
                             format_info.attr.opus.bitrate);
        }
        break;

    case AUDIO_FORMAT_TYPE_MSBC:
    case AUDIO_FORMAT_TYPE_SBC:
        {
            struct
            {
                uint8_t format_type;
                uint32_t sample_rate;
                uint8_t chann_mode;
                uint8_t block_length;
                uint8_t subband_num;
                uint8_t allocation_method;
                uint8_t bitpool;
            } __attribute__((packed)) *params = (typeof(params))cmd_payload;

            format_info.attr.msbc.sample_rate = params->sample_rate;
            format_info.attr.msbc.chann_mode = (T_AUDIO_SBC_CHANNEL_MODE)params->chann_mode;
            format_info.attr.msbc.block_length = params->block_length;
            format_info.attr.msbc.subband_num = params->subband_num;
            format_info.attr.msbc.allocation_method = params->allocation_method;
            format_info.attr.msbc.bitpool = params->bitpool;
            APP_PRINT_TRACE7("app_customer_mic_record parse_audio_format: \
                        type %d, sample_rate %d, chann_mode %d, block_length %d, subband_num %d, allocation_method %d, bitpool %d",
                             format_info.type,
                             format_info.attr.msbc.sample_rate, format_info.attr.msbc.chann_mode,
                             format_info.attr.msbc.block_length,
                             format_info.attr.msbc.subband_num, format_info.attr.msbc.allocation_method,
                             format_info.attr.msbc.bitpool);
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            struct
            {
                uint8_t format_type;
                uint32_t sample_rate;
                uint32_t chann_location;
                uint16_t frame_length;
                uint8_t frame_duration;
                uint32_t presentation_delay;
            } __attribute__((packed)) *params = (typeof(params))cmd_payload;

            format_info.attr.lc3.sample_rate = params->sample_rate;
            format_info.attr.lc3.chann_location = params->chann_location;
            format_info.attr.lc3.frame_length = params->frame_length;
            format_info.attr.lc3.frame_duration = (T_AUDIO_LC3_FRAME_DURATION)
                                                  params->frame_duration; //by default
            format_info.attr.lc3.presentation_delay = params->presentation_delay;

            APP_PRINT_TRACE6("app_customer_mic_record parse_audio_format: \
                             type %d, sample_rate %d, chann_location %d, frame_length %d, frame_duration %d, presentation_delay %d",
                             format_info.type,
                             format_info.attr.lc3.sample_rate, format_info.attr.lc3.chann_location,
                             format_info.attr.lc3.frame_length,
                             format_info.attr.lc3.frame_duration,
                             format_info.attr.lc3.presentation_delay);
        }
        break;

    default:

        break;
    }

    return format_info;
}


void app_record_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                           uint16_t cmd_len, uint8_t *ack_pkt)
{
    enum CMD_POS
    {
        CMD_ID_POS = 0,
        CMD_PAYLOAD_POS = CMD_ID_POS + 2,
    };

    uint16_t cmd_id = (uint16_t)(cmd_ptr[CMD_ID_POS] | (cmd_ptr[CMD_ID_POS + 1] << 8));

    APP_PRINT_TRACE1("customer_record_handle_cmd: cmd_id 0x%04x", cmd_id);

    switch (cmd_id)
    {
    case CMD_XM_START_RECORD:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            T_AUDIO_FORMAT_INFO format_info = parse_audio_format(&cmd_ptr[CMD_PAYLOAD_POS]);

            if (is_recording() == false)
            {

#if F_APP_SD_CARD_PLAY
                app_src_playback_create_and_open_file();
#endif

                record_start(format_info);
            }
        }
        break;

    case CMD_XM_STOP_RECORD:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            record_stop();
#if F_APP_SD_CARD_PLAY
            app_src_play_sd_file_close();
#endif
        }
        break;

    case CMD_XM_START_RECORD_PLAY:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            T_AUDIO_FORMAT_INFO format_info = parse_audio_format(&cmd_ptr[CMD_PAYLOAD_POS]);

            record_play_start(format_info);
        }
        break;

    case CMD_XM_STOP_RECORD_PLAY:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            record_play_stop();
        }
        break;

    case CMD_XM_RECORD_PLAY_DATA:
        {
            ack_pkt[2] = CMD_SET_STATUS_COMPLETE;

            record_play_data_write(cmd_ptr + 2, cmd_len - 2);
            if (record_play_buf_state() != PLAY_BUF_HIGH)
            {
                ack_pkt[2] = CMD_SET_STATUS_MUSIC_REQ;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_RECORD_THEN_PLAY_VP:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            T_AUDIO_FORMAT_INFO format_info = parse_audio_format(&cmd_ptr[CMD_PAYLOAD_POS]);
            record_start(format_info);
            app_start_timer(&mic_record.timer.sp_idx,
                            "customer_sp",
                            mic_record.timer.id,
                            TIMER_SP,
                            0,
                            false,
                            1000);
            app_start_timer(&mic_record.timer.sp_stop_idx,
                            "customer_sp_stop",
                            mic_record.timer.id,
                            TIMER_SP_STOP,
                            0,
                            false,
                            4000);
        }
        break;
    default:
        break;
    }
}
