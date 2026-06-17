/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_CUSTOMER_AUDIO_POLICY_SUPPORT
#include "app_customer_audio_policy.h"
#include "app_audio_policy.h"
#include "audio.h"
#include "app_report.h"

#include "app_cmd.h"
#include "app_customer_cmd.h"
#include "app_main.h"
#include "bt_hfp.h"
#include "app_hfp.h"
#include "trace.h"
#include "string.h"
#include "audio_probe.h"
#if F_APP_CUSTOMER_EXT_PA_SUPPORT_AW87390
#include "aw87390_driver_interface.h"
#endif
#include "stdlib.h"
#include "app_cfg.h"
#include "os_mem.h"
#include "ringtone.h"

#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
#include "playback_audio_file.h"
#include "app_music_ctrl.h"
#endif
#include "app_bond.h"
#include "app_tts.h"
#include "app_record.h"

#define TTS_HEADER_LEN                  0x000A
#define TTS_SYNC_BYTE                   0xB8
#define TTS_INIT_SEQ                    0x00
#define TTS_FRAME_LEN                   0x02

#define MUSIC_INIT_SEQ                  0x00


enum H2D_TEST_TYPE
{
    MIC_TEST = 1,
    SPK_TEST = 2,
};


typedef enum
{
    APP_AUDIO_TTS_STATE_OPENING  = 0x00,
    APP_AUDIO_TTS_STATE_STARTED  = 0x01,
    APP_AUDIO_TTS_STATE_ALERTED  = 0x02,
    APP_AUDIO_TTS_STATE_PAUSED   = 0x03,
    APP_AUDIO_TTS_STATE_RESUMED  = 0x04,
    APP_AUDIO_TTS_STATE_CLOSING  = 0x05,
    APP_AUDIO_TTS_STATE_ABORTING = 0x06,
    APP_AUDIO_TTS_STATE_STOPPED  = 0x07
} T_APP_AUDIO_TTS_STATE_TYPE;



typedef enum
{
    MUSIC_SESSION_OPEN = 0x00,
    MUSIC_SESSION_PAUSE = 0x01,
    MUSIC_SESSION_RESUME = 0x02,
    MUSIC_SESSION_ABORT = 0x03,
    MUSIC_SESSION_CLOSE = 0x04,
    MUSIC_SESSION_SEND_SINGLE_FRAME = 0x05,
    MUSIC_SESSION_SEND_START_FRAME = 0x06,
    MUSIC_SESSION_SEND_CONTINUE_FRAME = 0x07,
    MUSIC_SESSION_SEND_END_FRAME = 0x08
} T_MUSIC_SESSION_CMD_TYPE;



typedef enum
{
    APP_AUDIO_MUSIC_STATE_OPENING  = 0x00,
    APP_AUDIO_MUSIC_STATE_STARTED  = 0x01,
    APP_AUDIO_MUSIC_STATE_ALERTED  = 0x02,
    APP_AUDIO_MUSIC_STATE_PAUSED   = 0x03,
    APP_AUDIO_MUSIC_STATE_RESUMED  = 0x04,
    APP_AUDIO_MUSIC_STATE_CLOSING  = 0x05,
    APP_AUDIO_MUSIC_STATE_ABORTING = 0x06,
    APP_AUDIO_MUSIC_STATE_STOPPED  = 0x07
} T_APP_AUDIO_MUSIC_STATE_TYPE;


static struct
{
    struct
    {
        bool bypassed;
    } record;

    struct
    {
        uint8_t                             closed;
        uint8_t                             seq;
        T_CMD_PATH                          cmd_path;
        T_APP_AUDIO_TTS_STATE_TYPE           real_state;
        uint16_t                            data_offset;
        uint16_t                            frame_len;
        void                                *handle;
        uint8_t                             *frame_buf;
    } tts;

    struct
    {
        bool                        bypassed;
        bool                        start_flag;
        T_APP_AUDIO_MUSIC_STATE_TYPE real_state;
        uint8_t                     closed;
        void                        *handle;
    } music;

} policy =
{
    .tts = {.cmd_path = CMD_PATH_UART},
    .record = {.bypassed = false},
    .music = {.bypassed = false, .start_flag = false},
};


void app_customer_audio_set_tts_path(T_CMD_PATH path)
{
    policy.tts.cmd_path = path;
}

void app_customer_report_play_status(uint8_t play_status, uint8_t codec_type)
{
    uint8_t temp_buff[8];

    memset(&temp_buff[0], 0, 8);
    temp_buff[6] = play_status;
    temp_buff[7] = codec_type;

    app_report_event(CMD_PATH_UART, EVENT_PLAYER_STATUS, 0, temp_buff, sizeof(temp_buff));
}

void app_customer_report_play_time(uint32_t play_time)
{
    uint8_t data[4] = {0};

    memcpy(&data[0], &play_time, sizeof(uint32_t));
    app_report_event(CMD_PATH_UART, EVENT_XM_AUDIO_PLAY_TIME, 0, data, sizeof(data));
}

void app_customer_audio_request_frame(void)
{
    app_report_event(CMD_PATH_UART, EVENT_XM_MUSIC_REQ_FRAME, 0, NULL, 0);
}

static void app_customer_audio_req_tts(void)
{
    uint8_t tts_state = policy.tts.real_state;

    APP_PRINT_TRACE1("app_customer_audio_req_tts:%d", tts_state);
    app_report_event(policy.tts.cmd_path, EVENT_XM_TTS_REQ_FRAME, 0, &tts_state, 1);
}

uint8_t app_customer_tts_ack_status(uint8_t type)
{
    uint8_t ack_status = 0;

    if (policy.tts.closed == false)
    {
        if (
            policy.tts.real_state == APP_AUDIO_TTS_STATE_STARTED ||
            policy.tts.real_state == APP_AUDIO_TTS_STATE_ALERTED ||
            policy.tts.real_state == APP_AUDIO_TTS_STATE_RESUMED
        )
        {
            ack_status = CMD_SET_STATUS_TTS_REQ;
        }
    }

    APP_PRINT_INFO4("app_customer_tts_ack_status tts_closed %d tts_state %d ack_status %d, type %d",
                    policy.tts.closed, policy.tts.real_state, ack_status, type);

    return ack_status;
}

static void app_customer_audio_handle_tts_state(uint8_t tts_state)
{
    switch (tts_state)
    {
    case APP_AUDIO_TTS_STATE_STARTED:
        {
            app_customer_audio_req_tts();
        }
        break;
    case APP_AUDIO_TTS_STATE_RESUMED:
        {
            app_customer_audio_req_tts();
        }
        break;
    case APP_AUDIO_TTS_STATE_ABORTING:
        {
            app_customer_audio_req_tts();
        }
        break;
    case APP_AUDIO_TTS_STATE_STOPPED:
        {
            app_customer_audio_req_tts();
        }
        break;

    default:
        break;
    }
}

static void tts_cback(T_AUDIO_EVENT event_type, void *event_buf,
                      uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case AUDIO_EVENT_TTS_STARTED:
        if (policy.tts.cmd_path == CMD_PATH_UART)
        {
            policy.tts.real_state = APP_AUDIO_TTS_STATE_STARTED;
            app_customer_audio_handle_tts_state(policy.tts.real_state);
        }
        break;
    case AUDIO_EVENT_TTS_ALERTED:
        if (policy.tts.cmd_path == CMD_PATH_UART)
        {
//            policy.tts.real_state = APP_AUDIO_TTS_STATE_ALERTED;
            if (policy.tts.closed == false)
            {
                policy.tts.real_state = APP_AUDIO_TTS_STATE_RESUMED;
                app_customer_audio_handle_tts_state(policy.tts.real_state);
            }
        }
        break;
    case AUDIO_EVENT_TTS_PAUSED:
        if (policy.tts.cmd_path == CMD_PATH_UART)
        {
            policy.tts.real_state = APP_AUDIO_TTS_STATE_PAUSED;
            app_customer_audio_handle_tts_state(policy.tts.real_state);
        }
        break;
    case AUDIO_EVENT_TTS_RESUMED:
        if (policy.tts.cmd_path == CMD_PATH_UART)
        {
            policy.tts.real_state = APP_AUDIO_TTS_STATE_RESUMED;
            app_customer_audio_handle_tts_state(policy.tts.real_state);
        }
        break;
    case AUDIO_EVENT_TTS_STOPPED:
        {
            if (policy.tts.cmd_path == CMD_PATH_UART)
            {
                policy.tts.handle = NULL;
                policy.tts.frame_len = 0;
                policy.tts.data_offset = 0;
                policy.tts.seq = 0;
                if (policy.tts.real_state == APP_AUDIO_TTS_STATE_ABORTING)
                {
                }
                else
                {
                    policy.tts.real_state = APP_AUDIO_TTS_STATE_STOPPED;
                }
                app_customer_audio_handle_tts_state(policy.tts.real_state);
                if (policy.tts.frame_buf != NULL)
                {
                    free(policy.tts.frame_buf);
                    policy.tts.frame_buf = NULL;
                }
            }

            tts_destroy(param->tts_stopped.handle);
        }
        break;
    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_customer_audio_policy tts_cback: event %x", event_type);
    }
}

static void H2D_test_cmd(enum H2D_TEST_TYPE type, bool enable)
{
#define WORD_TO_BYTE_LENGTH (4)
#define WORD_LEN            (1)
#define CMD_PKT_LEN         ((WORD_LEN + 1) * WORD_TO_BYTE_LENGTH)

    uint8_t cmd_pkt[CMD_PKT_LEN] = {0};

    APP_PRINT_INFO1("app_customer_audio_policy mic_set: enable %d", enable);

    enum
    {
        CMD_LO_POS = 0,
        CMD_HI_POS = CMD_LO_POS + 1,
        LEN_LO_POS = CMD_HI_POS + 1,
        LEN_HI_POS = LEN_LO_POS + 1,
        TYPE_POS   = LEN_HI_POS + 1,
        ENABLE_POS = TYPE_POS + 1,
    };

    cmd_pkt[CMD_LO_POS] = 0x02; //H2D_TEST_MODE;                    //
    cmd_pkt[CMD_HI_POS] = (uint8_t)(0x02 >> 8);    //
    cmd_pkt[LEN_LO_POS] = (uint8_t)WORD_LEN;                        //
    cmd_pkt[LEN_HI_POS] = (uint8_t)(WORD_LEN >> 8);                 //
    cmd_pkt[TYPE_POS] = type;
    cmd_pkt[ENABLE_POS] = (uint8_t)enable;

    APP_PRINT_TRACE1("app_customer_audio_policy H2D_test_cmd: cmd_pkt %b", TRACE_BINARY(CMD_PKT_LEN,
                     cmd_pkt));

    audio_probe_dsp_send(cmd_pkt, CMD_PKT_LEN);
}


static void record_set_bypass(void)
{
    APP_PRINT_TRACE0("record_set_bypass");
    H2D_test_cmd(MIC_TEST, !policy.record.bypassed);
}

static void music_set_bypass(void)
{
    APP_PRINT_TRACE0("music_set_bypass");
    H2D_test_cmd(SPK_TEST, !policy.music.bypassed);
}


static void track_cback(T_AUDIO_EVENT event_type, void *event_buf,
                        uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            APP_PRINT_INFO1("app_customer_audio_policy track_cback: track_state_changed.state %d",
                            param->track_state_changed.state);

            switch (param->track_state_changed.state)
            {
            case AUDIO_TRACK_STATE_STARTED:
            case AUDIO_TRACK_STATE_RESTARTED:
                {
                    T_AUDIO_STREAM_TYPE stream_type;
                    if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == false)
                    {
                        break;
                    }

                    if (stream_type == AUDIO_STREAM_TYPE_RECORD)
                    {
                        record_set_bypass();
                    }
                    else if (stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                    {
                        music_set_bypass();
                    }
                }
                break;

            case AUDIO_TRACK_STATE_STOPPED:
                {

                }
                break;

            default:
                break;
            }
        }
        break;
    default:
        break;
    }
}



static void audio_cback(T_AUDIO_EVENT event_type, void *event_buf,
                        uint16_t buf_len)
{
    switch (event_type)
    {
    case AUDIO_EVENT_TTS_STARTED...AUDIO_EVENT_TTS_STOPPED:
        tts_cback(event_type, event_buf, buf_len);
        break;

    case AUDIO_EVENT_TRACK_STATE_CHANGED...AUDIO_EVENT_TRACK_BUFFER_HIGH:
        track_cback(event_type, event_buf, buf_len);
        break;

    default:
        break;
    }
}


static void tts_handle_call_status(void)
{
    if (policy.tts.cmd_path == CMD_PATH_UART)
    {
        policy.tts.real_state = APP_AUDIO_TTS_STATE_ABORTING;
        tts_stop(policy.tts.handle);
    }
}

#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
static void playback_handle_call_status(void)
{
    T_MUSIC_MODE playback_mode = MUSIC_IDLE;
    static struct
    {
        T_MUSIC_STATE prev;
        T_MUSIC_STATE cur;
    } playback_states = {MUSIC_STATE_IDLE, MUSIC_STATE_IDLE};

    playback_states.cur = app_music_get_state();
    playback_mode = app_music_get_mode();

    APP_PRINT_TRACE4("app_customer_audio_policy bt_cback:0x%x,sd_playback_switch:%d, old play state:%d real_state:%d",
                     app_hfp_get_call_status(), playback_mode, playback_states.prev, policy.music.real_state);

    if ((app_hfp_get_call_status() != BT_HFP_CALL_IDLE) && playback_mode)
    {
        if (playback_states.cur == MUSIC_STATE_PLAY)
        {
            playback_states.prev = playback_states.cur;
            app_music_pause();
            playback_states.cur = MUSIC_STATE_IDLE;
        }
    }


    if ((app_hfp_get_call_status() == BT_HFP_CALL_IDLE) &&
        (playback_states.prev == MUSIC_STATE_PLAY))
    {

//                if (playback_play_state_old == PLAYBACK_STATE_PLAY)
        if ((policy.music.real_state == APP_AUDIO_MUSIC_STATE_OPENING || \
             policy.music.real_state == APP_AUDIO_MUSIC_STATE_RESUMED))
        {
            app_music_resume();
        }
        playback_states.prev = MUSIC_STATE_IDLE;
    }
}
#endif

static void bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{

    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_HFP_CALL_STATUS:
        if (param->hfp_call_status.curr_status == APP_HFP_CALL_ACTIVE ||
            param->hfp_call_status.curr_status == APP_HFP_CALL_IDLE ||
            param->hfp_call_status.curr_status == APP_HFP_CALL_INCOMING ||
            param->hfp_call_status.curr_status == APP_HFP_CALL_OUTGOING)
        {
            tts_handle_call_status();
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
            playback_handle_call_status();
#endif
        }
        break;

    default:
        break;
    }
}

static void tts_play_try_again(void)
{
    uint16_t len = policy.tts.frame_len;
    uint16_t frame_len = 0;

    frame_len = (uint16_t)((policy.tts.frame_buf[0] << 8) | policy.tts.frame_buf[1]);
    APP_PRINT_TRACE2("tts_play_try_again, frame_len:%d, len:%d", frame_len, len);
    if (len == frame_len + TTS_FRAME_LEN)
    {
        if (tts_play(policy.tts.handle, policy.tts.frame_buf, len) == false)
        {
            APP_PRINT_ERROR0("tts_play_try_again fail!");
        }
    }
}

static void handle_tts_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                           uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint8_t   type;
    uint8_t   seq;
    uint16_t  len;
    uint16_t  frame_len;

    type = cmd_ptr[2];
    seq  = cmd_ptr[3];
    len  = (uint16_t)(cmd_ptr[4] | (cmd_ptr[5] << 8)); /* little endian */

    switch (type)
    {
    case TTS_SESSION_OPEN:
        {
            uint8_t   sync_byte;
            uint32_t  sess_len;
            uint16_t  frame_num;
            uint32_t  tts_cfg;
            void     *tts_handle;

            sync_byte = cmd_ptr[6]; /* big endian */
            sess_len = (uint32_t)((cmd_ptr[7] << 16) | (cmd_ptr[8] << 8) | cmd_ptr[9]); /* big endian */
            frame_num = (uint16_t)((cmd_ptr[10] << 8) | cmd_ptr[11]); /* big endian */
            tts_cfg = (uint32_t)((cmd_ptr[12] << 24) | (cmd_ptr[13] << 16) |
                                 (cmd_ptr[14] << 8) | cmd_ptr[15]); /* big endian */

            if (sync_byte != TTS_SYNC_BYTE ||
                seq       != TTS_INIT_SEQ  ||
                len       != TTS_HEADER_LEN)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                return;
            }

            app_customer_audio_set_tts_path((T_CMD_PATH)cmd_path);

            if (cmd_path == CMD_PATH_UART)
            {
                if (policy.tts.handle != NULL)
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    return;
                }

                tts_handle = tts_create(sess_len - TTS_HEADER_LEN, frame_num, tts_cfg);
                if (tts_handle != NULL)
                {
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                    policy.tts.handle = tts_handle;
                    policy.tts.real_state  = APP_AUDIO_TTS_STATE_OPENING;
                    policy.tts.closed = false;
                    tts_start(policy.tts.handle, false);
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
        }
        break;

    case TTS_SESSION_SEND_SINGLE_FRAME:
        frame_len = (uint16_t)(cmd_ptr[6] << 8 | cmd_ptr[7]); /* big endian */

        if (seq != TTS_INIT_SEQ ||
            len != frame_len + TTS_FRAME_LEN)
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            return;
        }

        if (cmd_path == CMD_PATH_UART)
        {
            if ((policy.tts.handle != NULL) && policy.tts.closed == false)
            {
                if (policy.tts.frame_buf != NULL)
                {
                    tts_play_try_again();
                    free(policy.tts.frame_buf);
                    policy.tts.frame_buf = NULL;
                }
                //app_db.xm_tts.tts_state = TTS_SESSION_SEND_SINGLE_FRAME;
                if (tts_play(policy.tts.handle, &cmd_ptr[6], len) == true)
                {
                    ack_pkt[2] = app_customer_tts_ack_status(type);
                }
                else
                {
                    APP_PRINT_ERROR0("tts_play write data fail!");
                    ack_pkt[2] = 0;
                    policy.tts.frame_buf = malloc(len);
                    if (policy.tts.frame_buf != NULL)
                    {
                        memcpy(policy.tts.frame_buf, &cmd_ptr[6], len);
                        policy.tts.frame_len = len;
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
        }

        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        break;

    case TTS_SESSION_SEND_START_FRAME:
        frame_len = (uint16_t)(cmd_ptr[6] << 8 | cmd_ptr[7]); /* big endian */

        if (seq != TTS_INIT_SEQ)
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            return;
        }

        if (cmd_path == CMD_PATH_UART)
        {
            if ((policy.tts.handle != NULL) && policy.tts.closed == false)
            {
                policy.tts.data_offset = 0;
                //app_db.xm_tts.tts_state = TTS_SESSION_SEND_START_FRAME;
                policy.tts.seq = TTS_INIT_SEQ;
                policy.tts.frame_len = frame_len + TTS_FRAME_LEN;

                if (policy.tts.frame_buf != NULL)
                {
                    tts_play_try_again();
                    free(policy.tts.frame_buf);
                }

                policy.tts.frame_buf = malloc(frame_len + TTS_FRAME_LEN);
                if (policy.tts.frame_buf != NULL)
                {
                    memcpy(policy.tts.frame_buf, &cmd_ptr[6], len);
                    policy.tts.data_offset += len;
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case TTS_SESSION_SEND_CONTINUE_FRAME:
        if (cmd_path == CMD_PATH_UART)
        {
            if (policy.tts.handle != NULL)
            {
                if (seq == policy.tts.seq + 1)
                {
                    policy.tts.seq = seq;
                    //app_db.xm_tts.tts_state = TTS_SESSION_SEND_CONTINUE_FRAME;
                    memcpy(policy.tts.frame_buf + policy.tts.data_offset, &cmd_ptr[6], len);
                    policy.tts.data_offset += len;
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case TTS_SESSION_SEND_END_FRAME:
        if (cmd_path == CMD_PATH_UART)
        {
            if (policy.tts.handle != NULL)
            {
                if (seq == policy.tts.seq + 1)
                {
                    policy.tts.seq = seq;
                    //app_db.xm_tts.tts_state = TTS_SESSION_SEND_END_FRAME;
                    memcpy(policy.tts.frame_buf + policy.tts.data_offset, &cmd_ptr[6], len);
                    policy.tts.data_offset = 0;
                    if (tts_play(policy.tts.handle, policy.tts.frame_buf, policy.tts.frame_len) == true)
                    {
                        free(policy.tts.frame_buf);
                        policy.tts.frame_buf = NULL;
                        ack_pkt[2] = app_customer_tts_ack_status(type);
                    }
                    else
                    {
                        ack_pkt[2] = 0;
                        APP_PRINT_ERROR0("tts_play write data fail!");
                    }
                    free(policy.tts.frame_buf);
                    policy.tts.frame_buf = NULL;
                    ack_pkt[2] = app_customer_tts_ack_status(type);
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;


    case TTS_SESSION_CLOSE:
        if (seq != TTS_INIT_SEQ  ||
            len != TTS_FRAME_LEN)
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            return;
        }

        if (cmd_path == CMD_PATH_UART)
        {
            if (policy.tts.handle != NULL)
            {
                policy.tts.real_state = APP_AUDIO_TTS_STATE_CLOSING;
                policy.tts.closed = true;
                tts_play(policy.tts.handle, &cmd_ptr[6], len);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case TTS_SESSION_ABORT:
        if (seq != TTS_INIT_SEQ)
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            return;
        }

        if (cmd_path == CMD_PATH_UART)
        {
            if (policy.tts.handle != NULL)
            {
                policy.tts.real_state = APP_AUDIO_TTS_STATE_ABORTING;
                policy.tts.closed = true;
                tts_stop(policy.tts.handle);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        break;
    }

    APP_PRINT_TRACE3("CMD_XM_TTS: type %x %d %b", type, cmd_len, TRACE_BINARY(6, cmd_ptr));
}

void app_customer_audio_handle_test_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                                        uint16_t cmd_len, uint8_t *ack_pkt)
{
    switch (cmd_ptr[2])
    {
    case XM_TEST_START_REQ_RRAME:
        {
            uint16_t req_interval = (uint16_t)(cmd_ptr[3] | cmd_ptr[4] << 8);
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
            policy.music.start_flag = true;
            playback_audio_file_set_state(PLAYBACK_AUDIO_FILE_START);
            app_customer_audio_request_frame();
            playback_audio_file_offset_reset();
            stream_clear_data();
#else
            customer_test_req_frame(req_interval, 0);
#endif
        }
        break;

    case XM_TEST_STOP_REQ_RRAME:
        {
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
            APP_PRINT_TRACE0("XM_TEST_STOP_REQ_RRAME");
            playback_audio_file_set_state(PLAYBACK_AUDIO_FILE_STOPPED);
#else
            customer_test_stop_req_frame();
#endif
        }
        break;

    case XM_TEST_TONE:
        {
            T_APP_AUDIO_TONE_TYPE tone_index;

            tone_index = (T_APP_AUDIO_TONE_TYPE)cmd_ptr[3];
            app_audio_tone_type_play(tone_index, false, false);
        }
        break;

    default:
        break;
    }



}


static void handle_music_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                             uint16_t cmd_len, uint8_t *ack_pkt)
{
    APP_PRINT_TRACE2("CMD_XM_MUSIC: %d %b", cmd_len, TRACE_BINARY(cmd_len, cmd_ptr));
    uint8_t   type;
    uint8_t   seq;
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
    uint8_t  *frame_ptr;
#endif
    uint16_t  frame_len;

    type = cmd_ptr[2];
    seq  = cmd_ptr[3];

#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
    frame_ptr = &cmd_ptr[4];
#endif

    frame_len  = cmd_len - 4;

    switch (type)
    {
    case MUSIC_SESSION_OPEN:
        {
            void     *music_handle;

            if (cmd_path == CMD_PATH_UART)
            {
                if (policy.music.handle != NULL)
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    return;
                }

                /*TODO: create & start music handle*/
                music_handle = (void *)1; //music_create();
                if (music_handle != NULL)
                {
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                    APP_PRINT_TRACE0("CMD_XM_MUSIC create & start music handle");
                    policy.music.handle = music_handle;
                    policy.music.real_state = APP_AUDIO_MUSIC_STATE_OPENING;
                    policy.music.closed = false;
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
                    policy.music.start_flag = true;
                    playback_audio_file_set_state(PLAYBACK_AUDIO_FILE_START);
                    app_customer_audio_request_frame();
                    playback_audio_file_offset_reset();
                    playback_audio_file_set_play_time(0);
                    app_music_create();
#endif
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
        }
        break;

    case MUSIC_SESSION_SEND_SINGLE_FRAME:
        if (seq != MUSIC_INIT_SEQ)
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            return;
        }

        if (cmd_path == CMD_PATH_UART)
        {
            if (policy.music.handle != NULL)
            {
                /*TODO: music play*/
//                APP_PRINT_TRACE2("CMD_XM_MUSIC music play %d - %b", frame_len, TRACE_BINARY(frame_len, frame_ptr));
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
                playback_audio_write_stream(frame_ptr, frame_len);
                if (stream_get_data_size() < STREAM_BUF_HIGH_LEVEL && (policy.music.closed == false))
                {
                    ack_pkt[2] = CMD_SET_STATUS_MUSIC_REQ;
                }
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                if (policy.music.start_flag == true && (stream_get_data_size() > STREAM_BUF_CHECK_LEVEL))
                {
                    if (MUSIC_SUCCESS == app_music_start())
                    {
                        policy.music.start_flag = false;
                    }
                }
                break;
#endif
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
        }

        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        break;

    case MUSIC_SESSION_CLOSE:
        if (seq != MUSIC_INIT_SEQ  ||
            frame_len != 0)
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            return;
        }

        if (cmd_path == CMD_PATH_UART)
        {
            if (policy.music.handle != NULL)
            {
                policy.music.real_state = APP_AUDIO_MUSIC_STATE_CLOSING;
                policy.music.closed = true;
                /*TODO: music close*/
                APP_PRINT_TRACE0("CMD_XM_MUSIC music close");
                //music_play(app_db.xm_music.music_handle, frame_ptr, frame_len);
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
                // ctrl data stream
                //remedy some data
                uint8_t *p_remedy_data = calloc(1, 512);
                memset(p_remedy_data, 0, 512);
                for (uint8_t i = 0; i < (STREAM_BUF_REMEDY_SIZE / 512); i++)
                {
                    playback_audio_write_stream(p_remedy_data, 512);
                }
                os_mem_free(p_remedy_data);
                playback_audio_file_set_state(PLAYBACK_AUDIO_FILE_STOPPING);
                //release audio track
//                app_music_close();
                policy.music.handle = NULL;
                if (policy.music.start_flag == true)
                {
                    app_music_is_short_audio_set(true);
                    if (MUSIC_SUCCESS == app_music_start())
                    {
                        policy.music.start_flag = false;
                    }
                }
                app_music_set_flow_state(MUSIC_FLOW_IDLE);
#endif
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case MUSIC_SESSION_PAUSE:
        if (seq != MUSIC_INIT_SEQ)
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            return;
        }

        if (cmd_path == CMD_PATH_UART)
        {
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
            if (playback_audio_file_get_state() != PLAYBACK_AUDIO_FILE_STOPPED)
#else
            if (policy.music.handle != NULL)
#endif
            {
                policy.music.real_state = APP_AUDIO_MUSIC_STATE_PAUSED;
                /*TODO: music pause*/
                APP_PRINT_TRACE0("CMD_XM_MUSIC music pause");
                //music_pause(app_db.xm_music.music_handle);
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
                app_music_pause();
#endif
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case MUSIC_SESSION_RESUME:
        if (seq != MUSIC_INIT_SEQ)
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            return;
        }

        if (cmd_path == CMD_PATH_UART)
        {
//            if (policy.music.handle != NULL)
            if (policy.music.real_state == APP_AUDIO_MUSIC_STATE_PAUSED)
            {
                policy.music.real_state = APP_AUDIO_MUSIC_STATE_RESUMED;
                /*TODO: music resume*/
                APP_PRINT_TRACE0("CMD_XM_MUSIC music resume");
                //music_resume(app_db.xm_music.music_handle);
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
                app_music_resume();
#endif
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case MUSIC_SESSION_ABORT:
        if (seq != MUSIC_INIT_SEQ)
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            return;
        }

        if (cmd_path == CMD_PATH_UART)
        {
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
            if (playback_audio_file_get_state() != PLAYBACK_AUDIO_FILE_STOPPED)
#else
            if (policy.music.handle != NULL)
#endif
            {
                policy.music.real_state = APP_AUDIO_MUSIC_STATE_ABORTING;
                policy.music.closed = true;
                /*TODO: music abort*/
                APP_PRINT_TRACE0("CMD_XM_MUSIC music abort");
                //music_stop(app_db.xm_music.music_handle);
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
                playback_audio_file_set_state(PLAYBACK_AUDIO_FILE_STOPPED);
                app_music_close();
#endif
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        break;
    }
}

static bool set_hfp_volume(uint8_t volume)
{
    uint8_t app_idx_active = app_hfp_get_active_idx();
    T_APP_BR_LINK *p_link = &app_db.br_link[app_idx_active];
    uint8_t cur_pair_idx;
    bool ret = false;
    app_bond_get_pair_idx_mapping(p_link->bd_addr, &cur_pair_idx);
    app_cfg_nv.voice_gain_level[cur_pair_idx] = volume;
    APP_PRINT_TRACE1("set_hfp_volume set_hfp_volume: hfp_volume %d", volume);

    if (bt_hfp_speaker_gain_level_report(p_link->bd_addr, volume) == false)
    {
        return ret;
    }
    if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
    {
        ret = audio_track_volume_out_set(p_link->sco.track_handle, volume);
    }
    return ret;
}

static void set_volume(uint8_t volume)
{
    if ((app_cfg_nv.voice_prompt_volume_out_level != volume) ||
        (app_cfg_nv.ringtone_volume_out_level != volume))
    {
        APP_PRINT_TRACE1("app_customer_audio_policy set_vp_and_tone_volume: local_volume %d", volume);

        // vp and tone volume should be the same level
        app_cfg_nv.ringtone_volume_out_level = volume;
        app_cfg_nv.voice_prompt_volume_out_level = volume;
        ringtone_volume_set(volume);
        voice_prompt_volume_set(volume);
        tts_volume_set(volume);
        if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
        {
            set_hfp_volume(volume);
        }
        else
        {
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
            app_music_volume_set(volume);
#endif
            app_record_play_volume_set(volume);
        }

    }
}

typedef enum t_bt_volume_type
{
    HFP_VOL      = 0x00,
    LOCAL_VOL    = 0x01,
    A2DP_SRC_VOL = 0x02,
    RINGTONE_VOL = 0x03,
    VP_VOL       = 0x04,
    TTS_VOL      = 0x05,
    RECORD_VOL   = 0x06
} T_BT_VOLUME_TYPE;

static bool app_audio_set_volume(uint8_t volume, uint8_t type)
{
    bool ret = false;
    switch (type)
    {
    case HFP_VOL:
        {
            ret = set_hfp_volume(volume);
        }
        break;
    case LOCAL_VOL:
    case A2DP_SRC_VOL:
        {
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
            ret = app_music_volume_set(volume);
#endif
        }
        break;
    case RINGTONE_VOL:
        {
            app_cfg_nv.ringtone_volume_out_level = volume;
            ret = ringtone_volume_set(volume);
        }
        break;
    case VP_VOL:
        {
            app_cfg_nv.ringtone_volume_out_level = volume;
            ret = voice_prompt_volume_set(volume);
        }
        break;
    case TTS_VOL:
        {
            ret = tts_volume_set(volume);
        }
        break;
    case RECORD_VOL:
        {
            ret = app_record_play_volume_set(volume);
        }
        break;
    }
    return ret;
}

static void handle_volume_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                              uint16_t cmd_len, uint8_t *ack_pkt)
{
#define VOLUME_MAX (15)

    uint8_t volume = cmd_ptr[2];
    uint8_t type = cmd_ptr[3];
    bool ret = false;
    if (volume > VOLUME_MAX)
    {
        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        return;
    }
    else
    {
        ret = app_audio_set_volume(volume, type);
        if (ret)
        {
            ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
        }
        else
        {
            ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;

        }
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
    }
}


void app_customer_audio_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path,
                                   uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_XM_SET_VOLUME_LEVEL:
        {
            handle_volume_cmd(app_idx, cmd_path, cmd_ptr, cmd_len, ack_pkt);
        }
        break;

    case CMD_XM_TTS:
        handle_tts_cmd(app_idx, (T_CMD_PATH)cmd_path, cmd_ptr, cmd_len, ack_pkt);
        break;

    case CMD_XM_MUSIC:
        handle_music_cmd(app_idx, (T_CMD_PATH)cmd_path, cmd_ptr, cmd_len, ack_pkt);
        break;

    default:
        break;
    }
}


void app_customer_audio_all_bypassed(bool bypassed)
{
    APP_PRINT_TRACE0("app_customer_audio_all_bypassed");

    policy.record.bypassed = bypassed;
    policy.music.bypassed = bypassed;
#if F_APP_CUSTOMER_EXT_PA_SUPPORT_AW87390
    aw87390_set_bypass(bypassed);
#endif
}


void app_customer_audio_init(void)
{
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
    app_music_init();
#endif

    audio_mgr_cback_register(audio_cback);
    bt_mgr_cback_register(bt_cback);
}

#endif
