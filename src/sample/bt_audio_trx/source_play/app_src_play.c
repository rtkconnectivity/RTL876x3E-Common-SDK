/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_SOURCE_PLAY_SUPPORT
#include "app_src_play.h"
#include "app_src_play_a2dp.h"
#include "app_src_play_hfp.h"
#include "audio.h"
#include "audio_type.h"
#include "app_cfg.h"
#include "app_main.h"
#include "audio_track.h"
#include "trace.h"
#include "app_report.h"
#include "bt_types.h"

#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio.h"
#endif

#include "section.h"

#if F_APP_FWK_PIPE_DEMO_SUPPORT
#include "audio_pipe.h"
#include "ring_buffer.h"
#include "hw_tim.h"
#include <stdlib.h>
#include <string.h>
#endif

#if BAP_BROADCAST_SOURCE
#include "app_lea_ini_bap_bsrc.h"
#include "app_lea_ini_cap.h"
#endif

#if (BAP_UNICAST_CLIENT || BAP_BROADCAST_SOURCE)
#include "app_lea_ini_audio_data.h"
#endif

#if F_APP_SD_CARD_PLAY
#include "app_src_playback.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_uac.h"
#include "app_tri_dongle_cmd.h"
#include "app_tri_dongle_mgr.h"
#include "app_tri_dongle_volume.h"
#endif

#if F_APP_ATTACH_NREC_SUPPORT
#include "nrec.h"
#endif

#if F_APP_INTEGRATED_TRANSCEIVER
#include "app_src_play_pipe.h"
#endif

#if F_APP_FWK_PIPE_DEMO_SUPPORT
typedef struct
{
    T_AUDIO_PIPE_HANDLE     handle;
    T_RING_BUFFER           src_rbuf;
    uint8_t                *src_fill_buf;
    uint16_t                src_fill_len;
    uint8_t                *snk_drain_buf;
    uint8_t                 default_volume;
    uint16_t                seq_num;
    uint8_t                 state;
    uint16_t                keep_pcm_threshold;
} T_SRC_PIPE;
#endif

typedef struct
{
    bool     active;
    bool     enable;
    uint8_t  chann_num;
    uint8_t  bit_width;
    uint32_t sample_rate;
} T_SRC_USB_ATTR;

typedef struct rbuf_hdr
{
    uint16_t len;
    uint8_t  it;
    uint8_t  flag;
    uint32_t timestamp;
} T_RBUF_HDR;

typedef struct
{
    T_SOURCE_ROUTE src_route;
    T_PLAY_ROUTE play_route;
} T_ROUTE_COMBINATION;

#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
static const T_ROUTE_COMBINATION valid_routes_with_localplay[] =
{
    {SOURCE_ROUTE_MIC, PLAY_ROUTE_A2DP},
    {SOURCE_ROUTE_MIC, PLAY_ROUTE_HFP_AG},
    {SOURCE_ROUTE_MIC, PLAY_ROUTE_BIS},
    {SOURCE_ROUTE_MIC, PLAY_ROUTE_CIS},

    {SOURCE_ROUTE_LINE_IN, PLAY_ROUTE_A2DP},
    {SOURCE_ROUTE_LINE_IN, PLAY_ROUTE_HFP_AG},
    {SOURCE_ROUTE_LINE_IN, PLAY_ROUTE_BIS},
    {SOURCE_ROUTE_LINE_IN, PLAY_ROUTE_CIS},

    {SOURCE_ROUTE_SPDIF, PLAY_ROUTE_A2DP},
    {SOURCE_ROUTE_SPDIF, PLAY_ROUTE_HFP_AG},
    {SOURCE_ROUTE_SPDIF, PLAY_ROUTE_BIS},
    {SOURCE_ROUTE_SPDIF, PLAY_ROUTE_CIS},

    {SOURCE_ROUTE_I2S, PLAY_ROUTE_A2DP},
    {SOURCE_ROUTE_I2S, PLAY_ROUTE_HFP_AG},
    {SOURCE_ROUTE_I2S, PLAY_ROUTE_BIS},
    {SOURCE_ROUTE_I2S, PLAY_ROUTE_CIS},

    {SOURCE_ROUTE_A2DP, PLAY_ROUTE_A2DP},
};
#endif

typedef    struct
{
    T_AUDIO_TRACK_HANDLE    default_handle;
    T_AUDIO_STREAM_TYPE     default_type;
#if F_APP_MULTI_CHANNEL_SUPPORT
    T_AUDIO_TRACK_HANDLE    sec_handle;
    T_AUDIO_STREAM_TYPE     sec_type;
#endif
    uint16_t                play_seq;
    uint8_t                 record_volume;
    uint8_t                 play_volume;
    bool                    first_received;
#if F_APP_ATTACH_NREC_SUPPORT
    bool                    require_nrec;
    T_AUDIO_EFFECT_INSTANCE nrec_instance;
#endif
} T_SRC_TRACK;

typedef struct
{
    uint16_t feature_bits;
#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
    struct
    {
        T_AUDIO_TRACK_HANDLE track_handle;
        uint8_t              play_volume;
    } local_play;
#endif
} T_ATTACH_FEATURE;

typedef struct
{
    T_SOURCE_ROUTE      src_route;
    T_PLAY_ROUTE        play_route;
    T_ATTACH_FEATURE    attached_features;
    T_SRC_TRACK         record;
#if F_APP_FWK_PIPE_DEMO_SUPPORT
    T_SRC_PIPE          pipe[2];             //0 encode_pipe; 1 decode_pipe
#endif
} T_SOURCE_PLAY;

T_SOURCE_PLAY source_play =
{
    .src_route = SOURCE_ROUTE_INVALID,
    .play_route = PLAY_ROUTE_INVALID,
    .record =
    {
        .default_handle = NULL,
        .default_type = AUDIO_STREAM_TYPE_RECORD,
#if F_APP_MULTI_CHANNEL_SUPPORT
        .sec_handle = NULL,
        .sec_type = AUDIO_STREAM_TYPE_RECORD,
#endif
        .play_seq = 0,
        .record_volume = 10,
        .play_volume = 10,
        .first_received = false,
    },
#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
    .attached_features =
    {
        .local_play.play_volume = 10,
    },
#endif
};

#if F_APP_FWK_PIPE_DEMO_SUPPORT
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#define USB_ENCODE_SRC_RINGBUF_SIZE         (6*1024)
#else
#define USB_ENCODE_SRC_RINGBUF_SIZE         (3*1024)
#endif
#define LC3_TX_RINGBUF_SIZE                 (1024)
#define ENCODE_PIPE_DRAIN_BUF_LEN           (512)

#define USB_DECODE_SRC_RINGBUF_SIZE         (2*1024)
#define DECODE_PIPE_DRAIN_BUF_LEN           (1024)      // fwk supported max drain buf size 

#define DECODE_PIPE_PCM_CNT                 (2)

#define LEA_SEND_TIMER_LOADCOUNT            (10000)

#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
typedef struct
{
    T_HW_TIMER_HANDLE           lea_tx_timer_handle;
    bool                        lea_tx_timer_start;
    T_RING_BUFFER               lc3_tx_rbuf;
    uint8_t                    *lea_data_buf;
    uint16_t                    frame_len;
    uint32_t                    frame_duration;
    uint16_t                    lc3_tx_seq;
} T_LEA_DATA_DB;

T_LEA_DATA_DB src_lea_db;
#endif

typedef enum
{
    PIPE_STATE_IDLE = 0,
    PIPE_STATE_CREATED = 1,
    PIPE_STATE_STARTED = 2,
    PIPE_STATE_FILLING = 3,
} T_PIPE_STATE;

#if F_APP_USB_AUDIO_SUPPORT
T_SRC_USB_ATTR usb_ds_attr;
T_SRC_USB_ATTR usb_us_attr;

static bool usb_encode_pipe_start(void);
static void usb_encode_pipe_stop(void);
static void usb_encode_pipe_buf_release(void);
static void usb_encode_pipe_fill_pcm(void);

static bool usb_decode_pipe_start(void);
static void usb_decode_pipe_stop(void);
static void usb_decode_pipe_buf_release(void);
static void usb_decode_pipe_fill(void);
#endif

#endif

#if 0
static void mic_dump_record_data(const char *title, uint8_t *record_data_buf, uint32_t data_len)
{
    const uint32_t bat_num = 8;
    uint32_t times = data_len / bat_num;
    uint32_t residue = data_len % bat_num;
    uint8_t *residue_buf = record_data_buf + times * bat_num;

    APP_PRINT_TRACE3("mic_dump_record_data: data_len %d, times %d, residue %d", data_len,
                     times, residue);
    APP_PRINT_TRACE2("mic_dump_record_data: record_data_buf is 0x%08x, residue_buf is 0x%08x\r\n",
                     (uint32_t)record_data_buf,
                     (uint32_t)residue_buf);

    APP_PRINT_TRACE1("@@@@@@@@@@@@@@@@@@@@@%s@@@@@@@@@@@@@@@@@@@@@@@@@@@", TRACE_STRING(title));

    for (int32_t i = 0; i < times; i++)
    {
        APP_PRINT_TRACE8("mic_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         record_data_buf[i * bat_num], record_data_buf[i * bat_num + 1], record_data_buf[i * bat_num + 2],
                         record_data_buf[i * bat_num + 3],
                         record_data_buf[i * bat_num + 4], record_data_buf[i * bat_num + 5],
                         record_data_buf[i * bat_num + 6],
                         record_data_buf[i * bat_num + 7]);
    }

    switch (residue)
    {
    case 1:
        APP_PRINT_TRACE1("mic_dump_record_data: 0x%02x\r\n", residue_buf[0]);
        break;
    case 2:
        APP_PRINT_TRACE2("mic_dump_record_data: 0x%02x, 0x%02x\r\n", residue_buf[0], residue_buf[1]);
        break;
    case 3:
        APP_PRINT_TRACE3("mic_dump_record_data: 0x%02x, 0x%02x, 0x%02x\r\n", residue_buf[0], residue_buf[1],
                         residue_buf[2]);
        break;
    case 4:
        APP_PRINT_TRACE4("mic_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n", residue_buf[0],
                         residue_buf[1], residue_buf[2], residue_buf[3]);
        break;
    case 5:
        APP_PRINT_TRACE5("mic_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4]);
        break;
    case 6:
        APP_PRINT_TRACE6("mic_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4], residue_buf[5]);
        break;
    case 7:
        APP_PRINT_TRACE7("mic_dump_record_data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n",
                         residue_buf[0], residue_buf[1], residue_buf[2], residue_buf[3], residue_buf[4], residue_buf[5],
                         residue_buf[6]);
        break;

    default:
        break;
    }

    APP_PRINT_TRACE0("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
}
#else

static void mic_dump_record_data(const char *title, uint8_t *record_data_buf, uint32_t data_len)
{
}

#endif

#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
void app_src_play_attach_local_play_handle_data(uint8_t *p_data, uint16_t len, uint16_t seq_num,
                                                uint8_t frame_num, uint32_t timestamp)
{
    T_AUDIO_TRACK_HANDLE track_handle = source_play.attached_features.local_play.track_handle;
    if (track_handle)
    {
        uint16_t written_len;
        audio_track_write(track_handle, timestamp,
                          seq_num,
                          AUDIO_STREAM_STATUS_CORRECT,
                          frame_num,
                          p_data,
                          len,
                          &written_len);
    }
}
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
    // APP_PRINT_TRACE2("app_mic_record_read_cb: buf 0x%08x, required_len %d", buf, required_len);

    {
        mic_dump_record_data("app_mic_record_read_cb", buf, required_len);
        uint8_t actual_frame_num = *frame_num;
        if (source_play.record.default_handle)
        {
            if (source_play.play_route == PLAY_ROUTE_A2DP)
            {
                uint16_t res = app_src_play_a2dp_handle_data(buf, required_len, actual_frame_num);
            }
            else if (source_play.play_route == PLAY_ROUTE_HFP_AG)
            {
                uint8_t empty_addr[6] = {0}; //The parameter is invalid here
                app_src_play_hfp_send_sco(buf, required_len, empty_addr);
            }
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
            else if (source_play.play_route == PLAY_ROUTE_BIS ||
                     source_play.play_route == PLAY_ROUTE_CIS)
            {
                if (source_play.record.first_received == false)
                {
                    source_play.record.first_received = true;
                    uint32_t ap_delta = 0;
                    uint32_t iso_interval = 0;
                    le_audio_get_ap_delta(&ap_delta, &iso_interval);
                    if ((ap_delta < 500) || (ap_delta > REF_GUARD_TIME_US))
                    {
                        APP_PRINT_WARN0("record_read_cb: drop");
                        return true;
                    }
                }
                app_lea_iso_data_send(buf, required_len, true, *timestamp, *seq_num);
            }
#endif
            else if (source_play.play_route == PLAY_ROUTE_MULTI_A2DP)
            {
                uint16_t res = app_src_play_multi_a2dp_handle_data(buf, required_len, 0, actual_frame_num);
            }
            else
            {
                // TODO:;
            }
        }
#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
        if (source_play.attached_features.feature_bits & SRC_PLAY_FEATURE_APPEND_LOCALPLAY)
        {
            if (source_play.play_route == PLAY_ROUTE_A2DP && actual_frame_num == 1)
            {
                /*
                    Handle data in app_src_play_a2dp_handle_data().
                    For DSPs that don't support specifying encode SBC frame number,
                    the APP needs to package first. In order to save RAM resources,
                    reusing the ring buffer in A2DP is necessary.
                */
            }
            else
            {
                app_src_play_attach_local_play_handle_data(buf, required_len, *seq_num, actual_frame_num,
                                                           *timestamp);
            }
        }
#endif
    }

    *actual_len = required_len;

    return true;
}

#if F_APP_MULTI_CHANNEL_SUPPORT
static bool sec_record_read_cb(T_AUDIO_TRACK_HANDLE  handle,
                               uint32_t             *timestamp,
                               uint16_t             *seq_num,
                               T_AUDIO_STREAM_STATUS *status,
                               uint8_t              *frame_num,
                               void                 *buf,
                               uint16_t              required_len,
                               uint16_t             *actual_len)
{
    {
        uint8_t actual_frame_num = *frame_num;
        if (source_play.record.sec_handle)
        {
            if (source_play.play_route == PLAY_ROUTE_MULTI_A2DP)
            {
                uint16_t res = app_src_play_multi_a2dp_handle_data(buf, required_len, 1, actual_frame_num);
            }
            else
            {
                // TODO:;
            }

        }

    }

    *actual_len = required_len;

    return true;
}
#endif

#if F_APP_ATTACH_NREC_SUPPORT
bool app_src_play_is_nrec_mode(void)
{
    return source_play.attached_features.feature_bits & SRC_PLAY_FEATURE_NREC ? true : false;
}

static T_AUDIO_EFFECT_INSTANCE app_src_play_nrec_attach(T_AUDIO_TRACK_HANDLE handle,
                                                        bool enable)
{
    T_AUDIO_EFFECT_INSTANCE nrec_instance;

    nrec_instance = nrec_create(NREC_CONTENT_TYPE_RECORD, NREC_MODE_HIGH_SOUND_QUALITY, 0);

    if (nrec_instance != NULL)
    {
        if (enable)
        {
            nrec_enable(nrec_instance);
        }
        else
        {
            nrec_disable(nrec_instance);
        }

        audio_track_effect_attach(handle, nrec_instance);

        return nrec_instance;
    }

    return NULL;
}

static void app_src_play_nrec_detach(T_AUDIO_TRACK_HANDLE handle,
                                     T_AUDIO_EFFECT_INSTANCE nrec_instance)
{
    if (nrec_instance != NULL)
    {
        audio_track_effect_detach(handle, nrec_instance);
        nrec_disable(nrec_instance);
        nrec_release(nrec_instance);

        nrec_instance = NULL;
    }
}
#endif

static void app_src_play_track_cback(T_AUDIO_EVENT event_type, void *event_buf,
                                     uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            if (param->track_state_changed.handle != source_play.record.default_handle)
            {
                break;
            }
            APP_PRINT_INFO1("AUDIO_EVENT_TRACK_STATE_CHANGED: %d", param->track_state_changed.state);

            switch (param->track_state_changed.state)
            {
            case AUDIO_TRACK_STATE_STARTED:
            case AUDIO_TRACK_STATE_RESTARTED:
                {
                    if (source_play.play_route == PLAY_ROUTE_BIS ||
                        source_play.play_route == PLAY_ROUTE_CIS)
                    {
                        // read some data to tirgger running
                        uint8_t buf[10];
                        uint32_t timestamp;
                        uint16_t seq_num;
                        uint8_t frame_num;
                        uint16_t read_len;
                        T_AUDIO_STREAM_STATUS status;
                        if (audio_track_read(source_play.record.default_handle, &timestamp, &seq_num, &status, &frame_num,
                                             buf, 10,
                                             &read_len))
                        {
                            APP_PRINT_WARN0("AUDIO_TRACK_STATE_RESTARTED, audio track read error!!");
                        }
                    }
                    source_play.record.first_received = false;
#if F_APP_ATTACH_NREC_SUPPORT
                    if (source_play.attached_features.feature_bits & SRC_PLAY_FEATURE_NREC)
                    {
                        source_play.record.nrec_instance = app_src_play_nrec_attach(source_play.record.default_handle,
                                                                                    true);
                        audio_track_effect_control(source_play.record.default_handle, 0);
                    }
#endif
                }
                break;
            case AUDIO_TRACK_STATE_STOPPED:
            case AUDIO_TRACK_STATE_PAUSED:
                {
#if F_APP_ATTACH_NREC_SUPPORT
                    if (source_play.attached_features.feature_bits & SRC_PLAY_FEATURE_NREC)
                    {
                        app_src_play_nrec_detach(source_play.record.default_handle, source_play.record.nrec_instance);
                    }
#endif
                }
                break;
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_src_play_track_cback: event_type 0x%04x", event_type);
    }
}

static bool app_src_play_get_format(T_AUDIO_FORMAT_INFO *format_info)
{
    uint8_t err_code = 0;
    if (source_play.play_route == PLAY_ROUTE_A2DP)
    {
        if (!app_src_play_get_a2dp_format(format_info))
        {
            err_code = 1;
        }
    }
    else if (source_play.play_route == PLAY_ROUTE_HFP_AG)
    {
        if (!app_src_play_get_hfp_format(format_info))
        {
            err_code = 2;
        }
    }
#if BAP_BROADCAST_SOURCE
    else if (source_play.play_route == PLAY_ROUTE_BIS)
    {
        if (!app_lea_get_data_format(LEA_CODEC_DIR_ENCODE, format_info))
        {
            err_code = 3;
        }
    }
#endif
#if BAP_UNICAST_CLIENT
    else if (source_play.play_route == PLAY_ROUTE_CIS)
    {
        if (!app_lea_get_data_format(LEA_CODEC_DIR_ENCODE, format_info))
        {
            err_code = 4;
        }
    }
#endif
    else if (source_play.play_route == PLAY_ROUTE_MULTI_A2DP)
    {
        if (!app_src_play_get_a2dp_format(format_info))
        {
            err_code = 5;
        }
    }
    else
    {
        err_code = 6;
    }
    if (err_code != 0)
    {
        APP_PRINT_ERROR1("app_src_play_get_format: failed %d", -err_code);
        return false;
    }

    return true;
}

static bool record_start(void)
{
    APP_PRINT_TRACE0("record_start!!");
    int8_t err_code = 0;
    uint32_t device_default = 0;
#if F_APP_MULTI_CHANNEL_SUPPORT
    uint32_t device_sec = 0;
#endif
    T_AUDIO_STREAM_MODE mode = AUDIO_STREAM_MODE_NORMAL;
    if (source_play.play_route == PLAY_ROUTE_BIS ||
        source_play.play_route == PLAY_ROUTE_CIS)
    {
        mode = AUDIO_STREAM_MODE_DIRECT;
    }

    T_AUDIO_FORMAT_INFO format_info = {};
    if (!app_src_play_get_format(&format_info))
    {
        err_code = 1;
        goto err;
    }

    if (source_play.record.default_handle != NULL)
    {
        err_code = 2;
        goto err;
    }

    if (source_play.src_route == SOURCE_ROUTE_MIC)
    {
        device_default = AUDIO_DEVICE_IN_MIC;
    }
    else if (source_play.src_route == SOURCE_ROUTE_LINE_IN)
    {
        device_default = AUDIO_DEVICE_IN_AUX;
    }
#if F_APP_SPDIF_SUPPORT
    else if (source_play.src_route == SOURCE_ROUTE_SPDIF)
    {
        device_default = AUDIO_DEVICE_IN_SPDIF;
    }
#endif
    else if (source_play.src_route == SOURCE_ROUTE_I2S)
    {
        device_default = AUDIO_DEVICE_IN_MIC;
    }
#if F_APP_MULTI_CHANNEL_SUPPORT
    else if (source_play.src_route == SOURCE_ROUTE_MULTI_DEV)
    {
        device_default = AUDIO_DEVICE_IN_MIC;
        device_sec = AUDIO_DEVICE_IN_AUX;
    }
#endif
    else
    {
        err_code = 3;
        goto err;
    }

    if (source_play.record.default_type == AUDIO_STREAM_TYPE_VOICE)
    {
        device_default |= AUDIO_DEVICE_OUT_SPK;
    }


    source_play.record.play_seq = 0;
    source_play.record.first_received = false;

    source_play.record.default_handle = audio_track_create(source_play.record.default_type,
                                                           mode,
                                                           AUDIO_STREAM_USAGE_LOCAL,
                                                           format_info,
                                                           source_play.record.play_volume,
                                                           source_play.record.record_volume,
                                                           device_default,
                                                           NULL,
                                                           record_read_cb);

    if (source_play.record.default_handle == NULL)
    {
        err_code = 4;
        goto err;
    }
    audio_track_start(source_play.record.default_handle);

#if F_APP_MULTI_CHANNEL_SUPPORT
    source_play.record.sec_handle = audio_track_create(source_play.record.sec_type,
                                                       mode,
                                                       AUDIO_STREAM_USAGE_LOCAL,
                                                       format_info,
                                                       source_play.record.play_volume,
                                                       source_play.record.record_volume,
                                                       device_sec,
                                                       NULL,
                                                       sec_record_read_cb);

    if (source_play.record.sec_handle == NULL)
    {
        err_code = 5;
        goto err;
    }

    audio_track_start(source_play.record.sec_handle);
#endif

    return true;
err:
    APP_PRINT_ERROR1("record_start: err_code %d", -err_code);
    return false;
}

static void record_stop(void)
{
    source_play.record.first_received = false;
    if (source_play.record.default_handle)
    {
        audio_track_release(source_play.record.default_handle);
        source_play.record.default_handle = NULL;
    }

#if F_APP_MULTI_CHANNEL_SUPPORT
    if (source_play.record.sec_handle)
    {
        audio_track_release(source_play.record.sec_handle);
        source_play.record.sec_handle = NULL;
    }
#endif

}

void app_src_play_attach_features(uint16_t feature_bits)
{
    source_play.attached_features.feature_bits |= feature_bits;
}

void app_src_play_detach_features(uint16_t feature_bits)
{
    source_play.attached_features.feature_bits &= ~feature_bits;
}

bool app_src_play_check_attach_feature_nrec(void)
{
#if F_APP_ATTACH_NREC_SUPPORT
    if (source_play.src_route == SOURCE_ROUTE_MIC)
    {
        if (source_play.play_route == PLAY_ROUTE_CIS ||
            source_play.play_route == PLAY_ROUTE_HFP_AG ||
            (source_play.play_route == PLAY_ROUTE_BIS &&
             app_db.bsrc_db.codec_cfg.sample_frequency == SAMPLING_FREQUENCY_CFG_16K))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
#else
    return false;
#endif
}



bool app_src_play_check_attach_feature_append_localplay(void)
{
#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
    for (size_t i = 0; i < sizeof(valid_routes_with_localplay); i++)
    {
        if (source_play.src_route == valid_routes_with_localplay[i].src_route &&
            source_play.play_route == valid_routes_with_localplay[i].play_route)
        {
            return true;
        }
    }
    return false;
#else
    return false;
#endif
}

bool app_src_play_set_src_route(T_SOURCE_ROUTE src_route, uint8_t dir)
{
#if F_APP_AUTO_POWER_TEST_LOG
    TEST_PRINT_INFO2("app_src_play_set_src_route: src_route %d dir %d", src_route, dir);
#else
    APP_PRINT_INFO2("app_src_play_set_src_route: src_route %d dir %d", src_route, dir);
#endif
    if (src_route == SOURCE_ROUTE_USB)
    {
#if F_APP_USB_AUDIO_SUPPORT
        usb_ds_attr.enable = (dir & SOURCE_DIR_DS_BIT);
        usb_us_attr.enable = (dir & SOURCE_DIR_US_BIT);
#endif
    }
    else if (src_route == SOURCE_ROUTE_MIC || src_route == SOURCE_ROUTE_I2S)
    {
        if ((dir & SOURCE_DIR_DS_BIT) && (dir & SOURCE_DIR_US_BIT))
        {
            source_play.record.default_type = AUDIO_STREAM_TYPE_VOICE;
        }
        else
        {
            source_play.record.default_type = AUDIO_STREAM_TYPE_RECORD;
        }
    }
    else
    {
        source_play.record.default_type = AUDIO_STREAM_TYPE_RECORD;
    }

#if F_APP_SD_CARD_PLAY
    if (src_route == SOURCE_ROUTE_SD_CARD)
    {
        app_audio_fs_report_local_music_files_info();
    }
#endif

    source_play.src_route = src_route;
    return true;
}

T_SOURCE_ROUTE app_src_play_get_src_route(void)
{
#if !F_APP_INTEGRATED_TRANSCEIVER
    APP_PRINT_INFO1("app_src_play_get_src_route: src_route %d", source_play.src_route);
#endif
    return source_play.src_route;
}

void app_src_play_set_play_route(T_PLAY_ROUTE play_route)
{
#if F_APP_AUTO_POWER_TEST_LOG
    TEST_PRINT_INFO1("app_src_play_set_play_route: play_route %d", play_route);
#else
    APP_PRINT_INFO1("app_src_play_set_play_route: play_route %d", play_route);
#endif
    source_play.play_route = play_route;
}

T_PLAY_ROUTE app_src_play_get_play_route(void)
{
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    APP_PRINT_INFO1("app_src_play_get_play_route: play_route %d", source_play.play_route);
#endif

    return source_play.play_route;
}

#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
bool app_src_play_is_local_play_attached(void)
{
    return source_play.attached_features.feature_bits & SRC_PLAY_FEATURE_APPEND_LOCALPLAY ? true :
           false;
}

static void app_src_play_attach_feature_local_play_stop(void)
{
    if (source_play.attached_features.local_play.track_handle)
    {
        audio_track_release(source_play.attached_features.local_play.track_handle);
        source_play.attached_features.local_play.track_handle = NULL;
    }
}

static bool app_src_play_attach_feature_local_play_start(void)
{
    T_AUDIO_FORMAT_INFO format_info = {};
    app_src_play_get_format(&format_info);
    source_play.attached_features.local_play.track_handle = audio_track_create(
                                                                AUDIO_STREAM_TYPE_PLAYBACK,
                                                                AUDIO_STREAM_MODE_NORMAL,
                                                                AUDIO_STREAM_USAGE_LOCAL,
                                                                format_info,
                                                                source_play.attached_features.local_play.play_volume,
                                                                0,
                                                                AUDIO_DEVICE_OUT_SPK,
                                                                NULL,
                                                                NULL);
    if (source_play.attached_features.local_play.track_handle == NULL)
    {
        return false;
    }
    audio_track_start(source_play.attached_features.local_play.track_handle);
    return true;
}
#endif

bool app_src_play_route_in_start(void)
{
    bool result = false;

    if (source_play.src_route == SOURCE_ROUTE_USB)
    {
#if F_APP_USB_AUDIO_SUPPORT
        if (usb_ds_attr.enable)
        {
            result = usb_encode_pipe_start();
        }

        if (usb_us_attr.enable)
        {
            result = usb_decode_pipe_start();
        }
#endif
    }
#if F_APP_SD_CARD_PLAY
    else if (source_play.src_route == SOURCE_ROUTE_SD_CARD)
    {
        result = app_src_play_sd_start(source_play.play_route);
    }
#endif
#if F_APP_INTEGRATED_TRANSCEIVER
    else if (source_play.src_route == SOURCE_ROUTE_A2DP || source_play.src_route == SOURCE_ROUTE_HFP_HF)
    {
        result = app_src_play_downstream_pipe_start(source_play.src_route, source_play.play_route)
                 && app_src_play_upstream_pipe_start(source_play.src_route, source_play.play_route);
    }
#endif
    else
    {
        result = record_start();
    }

#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
    if (result &&
        source_play.attached_features.feature_bits & SRC_PLAY_FEATURE_APPEND_LOCALPLAY)
    {
        result = app_src_play_attach_feature_local_play_start();
    }
#endif

    return result;
}

void app_src_play_route_in_stop(void)
{
    if (source_play.src_route == SOURCE_ROUTE_USB)
    {
#if F_APP_USB_AUDIO_SUPPORT
        if (usb_ds_attr.enable)
        {
            usb_encode_pipe_stop();
        }

        if (usb_us_attr.enable)
        {
            usb_decode_pipe_stop();
        }
#endif
    }
#if F_APP_SD_CARD_PLAY
    else if (source_play.src_route == SOURCE_ROUTE_SD_CARD)
    {
        app_src_play_sd_stop(source_play.play_route);
    }
#endif
#if F_APP_INTEGRATED_TRANSCEIVER
    else if (source_play.src_route == SOURCE_ROUTE_A2DP || source_play.src_route == SOURCE_ROUTE_HFP_HF)
    {
        app_src_play_pipe_stop();
    }
#endif
    else
    {
        record_stop();
    }
#if F_APP_ATTACH_LOCAL_PLAY_SUPPORT
    if (source_play.attached_features.feature_bits & SRC_PLAY_FEATURE_APPEND_LOCALPLAY)
    {
        app_src_play_attach_feature_local_play_stop();
    }
#endif
}

bool app_src_play_route_out_start(void)
{
    bool result = false;
    switch (source_play.play_route)
    {
    case PLAY_ROUTE_A2DP:
        {
            result = app_src_play_a2dp_start_req();
        }
        break;

    case PLAY_ROUTE_HFP_AG:
        {
            result = app_src_play_hfp_start_req();
        }
        break;

#if BAP_BROADCAST_SOURCE
    case PLAY_ROUTE_BIS:
        {
            result = app_lea_bsrc_start();
        }
        break;
#endif

#if BAP_UNICAST_CLIENT
    case PLAY_ROUTE_CIS:
        {
            if (source_play.src_route == SOURCE_ROUTE_USB)
            {
#if F_APP_USB_AUDIO_SUPPORT
                if (usb_ds_attr.enable == true && usb_us_attr.enable == false)
                {
                    result = app_lea_ini_cis_media_start(0);
                }
                else if (usb_ds_attr.enable == true && usb_us_attr.enable == true)
                {
                    result = app_lea_ini_cis_conversation_start(0);
                }
                else
#endif
                {
                    APP_PRINT_ERROR0("app_src_play_route_out_start: unsupported usb attr");
                }
            }
            else
            {
                result = app_lea_ini_cis_media_start(0);
            }
        }
        break;
#endif

    case PLAY_ROUTE_LOCAL:
        {
            // DO Nothing
            result = true;
        }
        break;

    case PLAY_ROUTE_MULTI_A2DP:
        {
            result = app_src_play_a2dp_start_req();
        }
        break;

    default:
        break;
    }
    return result;
}

bool app_src_play_route_out_stop(void)
{
    bool result = false;

    switch (source_play.play_route)
    {
    case PLAY_ROUTE_A2DP:
        {
            result = app_src_play_a2dp_stop();
        }
        break;

    case PLAY_ROUTE_HFP_AG:
        {
            result = app_src_play_hfp_stop();
        }
        break;

#if BAP_BROADCAST_SOURCE
    case PLAY_ROUTE_BIS:
        {
            result = app_lea_bsrc_stop(true);
        }
        break;
#endif

#if BAP_UNICAST_CLIENT
    case PLAY_ROUTE_CIS:
        {
            result = app_lea_ini_cis_stream_stop(0, true);
        }
        break;
#endif

    case PLAY_ROUTE_MULTI_A2DP:
        {
            result = app_src_play_a2dp_stop();
        }
        break;

    default:
        break;
    }

    if (source_play.play_route == PLAY_ROUTE_BIS || source_play.play_route == PLAY_ROUTE_CIS)
    {
#if F_APP_FWK_PIPE_DEMO_SUPPORT
        if (source_play.src_route == SOURCE_ROUTE_USB)
        {
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
            src_lea_db.lc3_tx_seq = 0;
#endif
        }
#endif

#if F_APP_SD_CARD_PLAY
        if (source_play.src_route == SOURCE_ROUTE_SD_CARD)
        {
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
            app_src_play_sd_handle_lea_stop();
#endif
        }
#endif
    }
    return result;
}

#if F_APP_FWK_PIPE_DEMO_SUPPORT
bool app_src_play_route_in_pipe_no_existed(bool encode)
{
    if (source_play.src_route == SOURCE_ROUTE_USB)
    {
        T_SRC_PIPE *p_pipe = NULL;

        if (encode)
        {
            p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);
        }
        else
        {
            p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);
        }

        if (p_pipe->handle != NULL)
        {
            return false;
        }
    }

    return true;
}
#endif

void app_src_play_handle_msg(T_IO_MSG *msg)
{
    switch (msg->subtype)
    {
#if F_APP_FWK_PIPE_DEMO_SUPPORT
    case PIPE_MSG_FILL:
        {
#if F_APP_USB_AUDIO_SUPPORT
            usb_encode_pipe_fill_pcm();
#endif
        }
        break;
#endif

#if F_APP_SD_CARD_PLAY
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    case SD_PIPE_LC3_TIMER:
        {
            app_src_play_sd_pipe_send_lea_data();
        }
        break;
#endif
#endif
    default:
        break;
    }
}

bool app_src_play_send_msg(T_SRC_PLAY_MSG sub_type, void *param_buf)
{
    T_IO_MSG msg;
    bool ret = false;

    msg.type = IO_MSG_TYPE_SRC_PLAY;
    msg.subtype = sub_type;
    msg.u.buf = param_buf;

    ret = app_io_msg_send(&msg);
    if (!ret)
    {
        APP_PRINT_INFO0("app_src_play_send_msg: failed");
    }
    return ret;
}

#if F_APP_FWK_PIPE_DEMO_SUPPORT
#if F_APP_USB_AUDIO_SUPPORT
static void pipe_fill_signal(void)
{
    app_src_play_send_msg(PIPE_MSG_FILL, NULL);
}

static bool encode_pipe_handle_data_ind(void)
{
    bool ret = false;
    uint16_t len = 0;
    uint32_t timestamp = 0;
    uint16_t seq = 0;
    T_AUDIO_STREAM_STATUS status;
    uint8_t  frame_num = 0;
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);

    ret = audio_pipe_drain(p_pipe->handle,
                           &timestamp,
                           &seq,
                           &status,
                           &frame_num,
                           p_pipe->snk_drain_buf,
                           &len);
    if (!ret)
    {
        APP_PRINT_ERROR1("encode_pipe_handle_data_ind: drain %x", ret);
        return false;
    }

    if (source_play.play_route == PLAY_ROUTE_A2DP)
    {
        ret = app_src_play_a2dp_handle_data(p_pipe->snk_drain_buf, len, frame_num);
    }
    else if (source_play.play_route == PLAY_ROUTE_HFP_AG)
    {
        uint8_t empty_addr[6] = {0}; //The parameter is invalid here
        app_src_play_hfp_send_sco(p_pipe->snk_drain_buf, len, empty_addr);
    }
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    else if (source_play.play_route == PLAY_ROUTE_BIS ||
             source_play.play_route == PLAY_ROUTE_CIS)
    {
        APP_PRINT_ERROR1("encode_pipe_handle_data_ind: drain len %x", len);

        uint16_t remain_size = ring_buffer_get_remaining_space(&src_lea_db.lc3_tx_rbuf);

        if (remain_size >= len &&  src_lea_db.frame_duration)
        {
            ring_buffer_write(&(src_lea_db.lc3_tx_rbuf), p_pipe->snk_drain_buf, len);
        }
        else
        {
            APP_PRINT_ERROR2("encode_pipe_handle_data_ind: write fail remain_size %d len %d ",
                             remain_size, len);
        }

        if (src_lea_db.lea_tx_timer_start == false && src_lea_db.frame_duration)
        {
            lea_tx_timer_start();
            lea_tx_timer_reload(src_lea_db.frame_duration);
        }
    }
#endif
    else
    {
        // TODO:;
    }
    return true;
}

static bool encode_pipe_handle_data_filled(void)
{
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);
    uint16_t len = p_pipe->src_fill_len;
    T_RING_BUFFER *p_pcm_rbuf = &(p_pipe->src_rbuf);

    p_pipe->state = PIPE_STATE_STARTED;

    if (ring_buffer_get_data_count(p_pcm_rbuf) >= len)
    {
        usb_encode_pipe_fill_pcm();
    }
    return true;
}

static bool encode_pipe_cb(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT event, uint32_t param)
{
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);

    if ((event != AUDIO_PIPE_EVENT_DATA_IND) && (event != AUDIO_PIPE_EVENT_DATA_FILLED))
    {
        APP_PRINT_INFO2("encode_pipe_cb: handle %p event %x", handle, event);
    }

    switch (event)
    {
    case AUDIO_PIPE_EVENT_RELEASED:
        {
            p_pipe->state = PIPE_STATE_IDLE;
            usb_encode_pipe_buf_release();
            p_pipe->handle = NULL;
        }
        break;
    case AUDIO_PIPE_EVENT_CREATED:
        {
            p_pipe->state = PIPE_STATE_CREATED;
            audio_pipe_start(handle);
        }
        break;
    case AUDIO_PIPE_EVENT_STARTED:
        {
            ring_buffer_clear(&(source_play.pipe[0].src_rbuf));
            p_pipe->state = PIPE_STATE_STARTED;
        }
        break;
    case AUDIO_PIPE_EVENT_STOPPED:
        {
            p_pipe->state = PIPE_STATE_CREATED;
        }
        break;
    case AUDIO_PIPE_EVENT_DATA_IND:
        {
            encode_pipe_handle_data_ind();
        }
        break;
    case AUDIO_PIPE_EVENT_DATA_FILLED:
        {
            encode_pipe_handle_data_filled();
        }
        break;
    case AUDIO_PIPE_EVENT_MIXED:
        break;
    case AUDIO_PIPE_EVENT_DEMIXED:
        break;
    default:
        break;
    }

    return true;

}

static void usb_encode_pipe_buf_release(void)
{
    APP_PRINT_INFO0("usb_encode_pipe_buf_release");
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);

    if (p_pipe->src_fill_buf)
    {
        free(p_pipe->src_fill_buf);
        p_pipe->src_fill_buf = NULL;
    }
    p_pipe->src_fill_len = 0;

    if (p_pipe->snk_drain_buf)
    {
        free(p_pipe->snk_drain_buf);
        p_pipe->snk_drain_buf = NULL;
    }
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    if (source_play.play_route == PLAY_ROUTE_BIS || source_play.play_route == PLAY_ROUTE_CIS)
    {
        if (src_lea_db.lea_data_buf)
        {
            free(src_lea_db.lea_data_buf);
            src_lea_db.lea_data_buf = NULL;
        }
        lea_tx_timer_stop();
        ring_buffer_clear(&src_lea_db.lc3_tx_rbuf);
        src_lea_db.frame_len = 0;
        src_lea_db.frame_duration = 0;
        src_lea_db.lc3_tx_seq = 0;
    }
#endif
}

static bool usb_encode_pipe_start(void)
{
    T_AUDIO_FORMAT_INFO src_info;
    T_AUDIO_FORMAT_INFO snk_info;
    uint16_t src_len = 0;
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);

    if (source_play.play_route == PLAY_ROUTE_A2DP)
    {
        if (!app_src_play_get_a2dp_format(&snk_info))
        {
            APP_PRINT_ERROR0("usb_encode_pipe_start: a2dp format info does not exist");
            return false;
        }
    }
    else if (source_play.play_route == PLAY_ROUTE_HFP_AG)
    {
        if (!app_src_play_get_hfp_format(&snk_info))
        {
            APP_PRINT_ERROR0("usb_encode_pipe_start: hfp format info does not exist");
            return false;
        }
    }
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    else if (source_play.play_route == PLAY_ROUTE_BIS || source_play.play_route == PLAY_ROUTE_CIS)
    {
        if (!app_lea_get_data_format(LEA_CODEC_DIR_ENCODE, &snk_info))
        {
            APP_PRINT_ERROR0("usb_encode_pipe_start: lc3 format info does not exist");
            return false;
        }
    }
#endif
    else
    {
        APP_PRINT_ERROR1("usb_encode_pipe_start: play_route", source_play.play_route);
        return false;
    }

    if (p_pipe->handle != NULL)
    {
        APP_PRINT_ERROR0("usb_encode_pipe_start: already started");
        return false;
    }

    if (usb_ds_attr.active == false)
    {
        APP_PRINT_ERROR0("usb_encode_pipe_start: usb ds inactive");
        return false;
    }

    src_info.type = AUDIO_FORMAT_TYPE_PCM;
    src_info.frame_num = 1;
    src_info.attr.pcm.sample_rate = usb_ds_attr.sample_rate;
    src_info.attr.pcm.chann_num = usb_ds_attr.chann_num;
    src_info.attr.pcm.bit_width = usb_ds_attr.bit_width;

    APP_PRINT_INFO3("usb_encode_pipe_start: snk type %x, %d, %d ", snk_info.type,
                    src_info.attr.pcm.sample_rate, src_info.attr.pcm.bit_width);
    switch (snk_info.type)
    {
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    case AUDIO_FORMAT_TYPE_LC3:
        {
            src_len = 512;
            snk_info.frame_num = 1;
            src_lea_db.lea_data_buf = calloc(1, snk_info.attr.lc3.frame_length * 2);
            if (src_lea_db.lea_data_buf == NULL)
            {
                APP_PRINT_ERROR0("usb_encode_pipe_start: lea_data_buf NULL");
                return false;
            }
            src_lea_db.frame_len = snk_info.attr.lc3.frame_length;
            if (snk_info.attr.lc3.frame_duration == AUDIO_LC3_FRAME_DURATION_7_5_MS)
            {
                src_lea_db.frame_duration = 7500;
            }
            else
            {
                src_lea_db.frame_duration = 10000;
            }
        }
        break;
#endif
    case AUDIO_FORMAT_TYPE_SBC:
        {
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            src_len = 1536;
#else
            src_len = 512;
#endif
            snk_info.frame_num = 1;
        }
        break;

    case AUDIO_FORMAT_TYPE_MSBC:
        {
            src_len = 720;
            snk_info.frame_num = 1;
        }
        break;

    case AUDIO_FORMAT_TYPE_CVSD:
        {
            src_len = 360;
            snk_info.frame_num = 1;

        }
        break;

    default:
        return false;
    }

    /*Frame length per channel in octets for encoding or decoding.*/
    src_info.attr.pcm.frame_length = src_len / src_info.attr.pcm.chann_num;
    p_pipe->src_fill_buf = calloc(1, src_len);
    if (p_pipe->src_fill_buf == NULL)
    {
        APP_PRINT_ERROR0("usb_encode_pipe_start: src_fill_buf NULL");
        usb_encode_pipe_buf_release();
        return false;
    }
    p_pipe->src_fill_len = src_len;

    p_pipe->snk_drain_buf = calloc(1, ENCODE_PIPE_DRAIN_BUF_LEN);
    if (p_pipe->snk_drain_buf == NULL)
    {
        APP_PRINT_ERROR0("usb_encode_pipe_start: snk_drain_buf NULL");
        usb_encode_pipe_buf_release();
        return false;
    }
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    if (!app_tri_dongle_mgr_get_abs_state())
    {
        uint16_t uac_current_spk_mute = app_tri_dongle_uac_spk_mute_get();
        uint16_t gain;
        uint8_t level = app_tri_dongle_get_spk_uac_vol();

        if (uac_current_spk_mute)
        {
            level = 0;
        }

        if (app_tri_dongle_uac_get_dac_gain(level, &gain))
        {
            p_pipe->handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL,
                                               src_info,
                                               snk_info,
                                               gain,
                                               encode_pipe_cb);
        }
    }
    else
#endif
    {
        p_pipe->handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL,
                                           src_info,
                                           snk_info,
                                           p_pipe->default_volume,
                                           encode_pipe_cb);
    }

    if (!p_pipe->handle)
    {
        APP_PRINT_ERROR0("usb_encode_pipe_start: handle NULL");
        usb_encode_pipe_buf_release();
        return false;
    }

    return true;
}

static void usb_encode_pipe_stop(void)
{
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);
    if (p_pipe->handle)
    {
        audio_pipe_release(p_pipe->handle);
    }
}

static void usb_encode_pipe_fill_pcm(void)
{
    uint8_t cause = 0;
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);
    uint16_t len = p_pipe->src_fill_len;
    T_RING_BUFFER *p_pcm_rbuf = &(p_pipe->src_rbuf);

    if (p_pipe->handle == NULL)
    {
        cause = 1;
        goto RESULT;
    }
    if ((p_pipe->src_fill_buf == NULL) || (!len))
    {
        cause = 2;
        goto RESULT;
    }

    if (p_pipe->state != PIPE_STATE_STARTED)
    {
        cause = 3;
        goto RESULT;
    }

    if (ring_buffer_get_data_count(p_pcm_rbuf) < len)
    {
        cause = 4;
        goto RESULT;
    }

    ring_buffer_peek(p_pcm_rbuf, len, p_pipe->src_fill_buf);
    if (audio_pipe_fill(p_pipe->handle,
                        0,
                        p_pipe->seq_num,
                        AUDIO_STREAM_STATUS_CORRECT,
                        1,
                        p_pipe->src_fill_buf,
                        len))
    {
        ring_buffer_read(p_pcm_rbuf, len, p_pipe->src_fill_buf);
        p_pipe->state = PIPE_STATE_FILLING;
        p_pipe->seq_num ++;
    }
    else
    {
RESULT:
        if (cause != 4)
        {
            APP_PRINT_INFO3("usb_encode_pipe_fill_pcm: audio_pipe_fill fail cause %d, seq_num %d, len %d",
                            cause,
                            p_pipe->seq_num, len);
        }
    }
}



static bool decode_pipe_handle_data_ind(void)
{
    bool ret = false;
    uint16_t len = 0;
    uint8_t frame_num = 0;
    uint32_t timestamp = 0;
    uint16_t seq = 0;
    T_AUDIO_STREAM_STATUS status;

    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);

    ret = audio_pipe_drain(p_pipe->handle,
                           &timestamp,
                           &seq,
                           &status,
                           &frame_num,
                           p_pipe->snk_drain_buf,
                           &len);
    if (!ret)
    {
        APP_PRINT_ERROR1("decode_pipe_handle_data_ind: drain %x", ret);
        return false;
    }

    return app_usb_audio_us_data_write(p_pipe->snk_drain_buf, len);
}

static bool decode_pipe_handle_data_filled(void)
{
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);
    p_pipe->state = PIPE_STATE_STARTED;
    usb_decode_pipe_fill();
    return true;
}

static bool decode_pipe_cb(T_AUDIO_PIPE_HANDLE handle, T_AUDIO_PIPE_EVENT event, uint32_t  param)
{
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);

    if ((event != AUDIO_PIPE_EVENT_DATA_IND) && (event != AUDIO_PIPE_EVENT_DATA_FILLED))
    {
        APP_PRINT_INFO2("decode_pipe_cb: handle %p event %x", handle, event);
    }

    switch (event)
    {
    case AUDIO_PIPE_EVENT_RELEASED:
        {
            p_pipe->state = PIPE_STATE_IDLE;
            usb_decode_pipe_buf_release();
            p_pipe->handle = NULL;
            p_pipe->keep_pcm_threshold = 0;
        }
        break;
    case AUDIO_PIPE_EVENT_CREATED:
        {
            p_pipe->state = PIPE_STATE_CREATED;
            audio_pipe_start(handle);
        }
        break;
    case AUDIO_PIPE_EVENT_STARTED:
        {
            ring_buffer_clear(&(source_play.pipe[1].src_rbuf));
            if (app_usb_audio_us_get_data_len() > 0)
            {
                app_usb_audio_us_clear_up();
            }
            p_pipe->state = PIPE_STATE_STARTED;
        }
        break;
    case AUDIO_PIPE_EVENT_STOPPED:
        {
            p_pipe->state = PIPE_STATE_CREATED;
        }
        break;
    case AUDIO_PIPE_EVENT_DATA_IND:
        {
            decode_pipe_handle_data_ind();
        }
        break;
    case AUDIO_PIPE_EVENT_DATA_FILLED:
        {
            decode_pipe_handle_data_filled();
        }
        break;
    case AUDIO_PIPE_EVENT_MIXED:
        break;
    case AUDIO_PIPE_EVENT_DEMIXED:
        break;
    default:
        break;
    }

    return true;

}

static void usb_decode_pipe_buf_release(void)
{
    APP_PRINT_INFO0("usb_decode_pipe_buf_release");
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);
    if (p_pipe->src_fill_buf)
    {
        free(p_pipe->src_fill_buf);
        p_pipe->src_fill_buf = NULL;
    }
    p_pipe->src_fill_len = 0;

    if (p_pipe->snk_drain_buf)
    {
        free(p_pipe->snk_drain_buf);
        p_pipe->snk_drain_buf = NULL;
    }

#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    if (source_play.play_route == PLAY_ROUTE_BIS || source_play.play_route == PLAY_ROUTE_CIS)
    {
        if (src_lea_db.lea_data_buf)
        {
            free(src_lea_db.lea_data_buf);
            src_lea_db.lea_data_buf = NULL;
        }
        lea_tx_timer_stop();
        ring_buffer_clear(&src_lea_db.lc3_tx_rbuf);
        src_lea_db.frame_len = 0;
        src_lea_db.frame_duration = 0;
        src_lea_db.lc3_tx_seq = 0;
    }
#endif
}


uint16_t calc_sbc_frame_size(uint8_t chann_mode, uint8_t blocks,
                             uint8_t subbands, uint8_t bitpool)
{
    uint8_t channel_num = 0;
    uint16_t frame_size = 0;
    uint8_t joint = 0;
    uint16_t temp = 0;

    APP_PRINT_INFO4("calc_sbc_frame_size: ch %u, bl %u, subb %u, bitpool %u",
                    chann_mode, blocks, subbands, bitpool);

    switch (chann_mode)
    {
    case 3: /* joint-stereo */
        joint = 1;
        channel_num = 2;
        break;
    case 2: /* stereo */
        channel_num = 2;
        break;
    case 1: /* dual channels */
        channel_num = 2;
        break;
    case 0: /* mono */
        channel_num = 1;
        break;
    default:
        break;
    }

    if (chann_mode == 0 || chann_mode == 1)
    {
        //mono or dual channel
        temp = blocks * channel_num * bitpool;

        frame_size = 4 + subbands * channel_num / 2 + temp / 8;

        if (temp % 8 != 0)
        {
            frame_size++;
        }
    }
    else if (chann_mode == 2 || chann_mode == 3)
    {
        //stereo or joint stereo
        temp = joint * subbands + blocks * bitpool;

        frame_size = 4 + subbands * channel_num / 2 + temp / 8;

        if (temp % 8 != 0)
        {
            frame_size++;
        }
    }

    APP_PRINT_INFO1("calc_sbc_frame_size: frame size %d", frame_size);

    return frame_size;
}


static bool usb_decode_pipe_start(void)
{
    T_AUDIO_FORMAT_INFO src_info;
    T_AUDIO_FORMAT_INFO snk_info;
    uint16_t src_len = 0;
    uint32_t frame_duration_us = 0;

    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);

    if (source_play.play_route == PLAY_ROUTE_HFP_AG)
    {
        if (!app_src_play_get_hfp_format(&src_info))
        {
            APP_PRINT_ERROR0("usb_decode_pipe_start: hfp format info does not exist");
            return false;
        }
    }
#if BAP_UNICAST_CLIENT
    else if (source_play.play_route == PLAY_ROUTE_CIS)
    {
        if (!app_lea_get_data_format(LEA_CODEC_DIR_DECODE, &src_info))
        {
            APP_PRINT_ERROR0("usb_decode_pipe_start: lc3 decode format info does not exist");
            return false;
        }
    }
#endif
    else
    {
        APP_PRINT_ERROR1("usb_decode_pipe_start: play_route", source_play.play_route);
        return false;
    }

    if (p_pipe->handle != NULL)
    {
        APP_PRINT_ERROR0("usb_decode_pipe_start: already started");
        return false;
    }

    if (usb_us_attr.active == false)
    {
        APP_PRINT_ERROR0("usb_decode_pipe_start: usb us inactive");
        return false;
    }

    snk_info.type = AUDIO_FORMAT_TYPE_PCM;
    snk_info.frame_num = 1;
    snk_info.attr.pcm.sample_rate = usb_us_attr.sample_rate;
    snk_info.attr.pcm.chann_num = usb_us_attr.chann_num;
    snk_info.attr.pcm.bit_width = usb_us_attr.bit_width;

    APP_PRINT_INFO1("usb_decode_pipe_start: src type %x", src_info.type);
    switch (src_info.type)
    {
    case AUDIO_FORMAT_TYPE_LC3:
        {
            if (src_info.attr.lc3.frame_duration == AUDIO_LC3_FRAME_DURATION_10_MS)
            {
                frame_duration_us = 10000;
            }
            else
            {
                frame_duration_us = 7500;
            }
            src_len = src_info.attr.lc3.frame_length * 2;
        }
        break;
    case AUDIO_FORMAT_TYPE_SBC:
        {
            frame_duration_us = 2670;
        }
        break;
    case AUDIO_FORMAT_TYPE_MSBC:
        {
            src_len = calc_sbc_frame_size(src_info.attr.msbc.chann_mode,
                                          src_info.attr.msbc.block_length,
                                          src_info.attr.msbc.subband_num,
                                          src_info.attr.msbc.bitpool);
            frame_duration_us = 7500;
        }
        break;
    case AUDIO_FORMAT_TYPE_CVSD:
        {
            if (src_info.attr.cvsd.frame_duration == AUDIO_CVSD_FRAME_DURATION_3_75_MS)
            {
                src_len = 30;
                frame_duration_us = 3750;
            }
            else
            {
                src_len = 60;
                frame_duration_us = 7500;
            }
        }
        break;
    default:
        return false;
    }

    snk_info.attr.pcm.frame_length = ((frame_duration_us * snk_info.attr.pcm.sample_rate) / 1000000) *
                                     snk_info.attr.pcm.chann_num * (snk_info.attr.pcm.bit_width / 8);
    p_pipe->keep_pcm_threshold = snk_info.attr.pcm.frame_length;

    p_pipe->src_fill_buf = calloc(1, src_len);
    if (p_pipe->src_fill_buf == NULL)
    {
        APP_PRINT_ERROR0("usb_decode_pipe_start: src_fill_buf NULL");
        usb_decode_pipe_buf_release();
        return false;
    }
    p_pipe->src_fill_len = src_len;

    p_pipe->snk_drain_buf = calloc(1, DECODE_PIPE_DRAIN_BUF_LEN);
    if (p_pipe->snk_drain_buf == NULL)
    {
        APP_PRINT_ERROR0("usb_encode_pipe_start: snk_drain_buf NULL");
        usb_decode_pipe_buf_release();
        return false;
    }

    p_pipe->handle = audio_pipe_create(AUDIO_STREAM_MODE_NORMAL,
                                       src_info,
                                       snk_info,
                                       p_pipe->default_volume,
                                       decode_pipe_cb);
    APP_PRINT_INFO1("usb_decode_pipe_start: decode handle %x", p_pipe->handle);
    if (!p_pipe->handle)
    {
        APP_PRINT_ERROR0("usb_decode_pipe_start: handle NULL");
        usb_decode_pipe_buf_release();
        return false;
    }

    return true;
}

static void usb_decode_pipe_stop(void)
{
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);
    if (p_pipe->handle)
    {
        audio_pipe_release(p_pipe->handle);
    }
}

static void usb_decode_pipe_fill(void)
{
    bool ret = false;
    T_AUDIO_STREAM_STATUS status = AUDIO_STREAM_STATUS_CORRECT;
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);
    uint16_t len = p_pipe->src_fill_len;
    T_RBUF_HDR hdr = {0x0};
    T_RING_BUFFER *p_rbuf = &(p_pipe->src_rbuf);
    uint8_t error_code = 0;

    if (p_pipe->handle == NULL)
    {
        error_code = 1;
        goto END_PROCESS;
    }
    if ((p_pipe->src_fill_buf == NULL) || (!len))
    {
        error_code = 2;
        goto END_PROCESS;
    }

    if (p_pipe->state != PIPE_STATE_STARTED)
    {
        error_code = 3;
        goto END_PROCESS;
    }

    if (ring_buffer_get_data_count(p_rbuf) < sizeof(T_RBUF_HDR))
    {
        error_code = 4;
        goto END_PROCESS;
    }

    if (app_usb_audio_us_get_remaining_pool_size() < (p_pipe->keep_pcm_threshold * DECODE_PIPE_PCM_CNT))
    {
        error_code = 5;
        goto END_PROCESS;
    }

    if (hdr.flag)
    {
        status = AUDIO_STREAM_STATUS_LOST;
    }
    else
    {
        status = AUDIO_STREAM_STATUS_CORRECT;
    }

    /*parse hdr data*/
    ring_buffer_read(p_rbuf, sizeof(T_RBUF_HDR), (void *)&hdr);
    ring_buffer_read(p_rbuf, len, p_pipe->src_fill_buf);

    ret = audio_pipe_fill(p_pipe->handle,
                          0,
                          p_pipe->seq_num,
                          status,
                          1,
                          p_pipe->src_fill_buf,
                          len);
    if (ret)
    {
        p_pipe->seq_num ++;
        p_pipe->state = PIPE_STATE_FILLING;
    }

END_PROCESS:
    if (error_code > 0 && error_code != 4)
    {
        APP_PRINT_ERROR5("usb_decode_pipe_fill: ret %d, error_code %d, data_count %d, bufferremaining_space %d p_pipe->src_fill_len %d",
                         ret, error_code, ring_buffer_get_data_count(p_rbuf), app_usb_audio_us_get_remaining_pool_size(),
                         len);
    }
}

#endif

RAM_TEXT_SECTION
static void downstream_pipe_fill_signal(void)
{
    T_IO_MSG msg;
    bool ret = false;
    msg.type = IO_MSG_TYPE_SRC_PLAY;
    msg.subtype = PIPE_MSG_FILL;
    ret = app_io_msg_send(&msg);
    if (!ret)
    {
        APP_PRINT_INFO0("pipe_fill_signal: failed");
    }
}

RAM_TEXT_SECTION
bool app_usb_audio_downstream_fill(uint8_t *data, uint16_t len)
{
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);
    T_RING_BUFFER *p_pcm_rbuf = &(p_pipe->src_rbuf);
    uint16_t remain_size = ring_buffer_get_remaining_space(p_pcm_rbuf);

#if F_APP_UAC_MEDIA_SILENCE_DETECT
    app_tri_dongle_silence_proc(data, len);
#endif

    if (p_pipe->handle == NULL)
    {
        return false;
    }

    if (remain_size >= len)
    {
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
        if (get_dongle_host_mute())
        {
            memset(data, 0, len);
        }
#endif
        if (!ring_buffer_write(p_pcm_rbuf, data, len))
        {
            APP_PRINT_ERROR0("app_usb_audio_downstream_fill: write fail");
        }
    }
    else
    {
        APP_PRINT_ERROR3("app_usb_audio_downstream_fill: remain_size %d len %d p_pipe->state %d",
                         remain_size, len, p_pipe->state);
    }

    if (p_pipe->state == PIPE_STATE_STARTED)
    {
        downstream_pipe_fill_signal();
    }
    return true;
}

#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
void lea_tx_timer_callback(T_HW_TIMER_HANDLE handle)
{
    T_AUDIO_FORMAT_INFO format_info;
    T_RING_BUFFER *p_rbuf = &(src_lea_db.lc3_tx_rbuf);
    uint16_t len = src_lea_db.frame_len;
    uint8_t *buf = src_lea_db.lea_data_buf;

    if (!p_rbuf || !len || !buf)
    {
        APP_PRINT_WARN3("lea_tx_timer_callback: p_rbuf %x, len %x, buf %x",
                        p_rbuf, len, buf);
        return;
    }

    if ((source_play.play_route != PLAY_ROUTE_BIS) &&
        (source_play.play_route != PLAY_ROUTE_CIS))
    {
        APP_PRINT_WARN1("lea_tx_timer_callback: play_route %x", source_play.play_route);
        return;
    }

    if (ring_buffer_get_data_count(p_rbuf) >= (len * 2))
    {
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
        app_tri_dongle_set_busy_state(APP_TRI_DONGLE_BUSY_EVENT_LEA_DATA_ENABLE);
#endif
        ring_buffer_read(p_rbuf, len * 2, buf);
    }
    else
    {
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
        app_tri_dongle_set_busy_state(APP_TRI_DONGLE_BUSY_EVENT_LEA_DATA_DISABLE);
#endif
        APP_PRINT_WARN2("lea_tx_timer_callback: ringbuf len %d want %x send fake data",
                        ring_buffer_get_data_count(p_rbuf), len);
        memset(buf, 0, len * 2);
    }

    uint32_t ts = src_lea_db.lc3_tx_seq * src_lea_db.frame_duration;
    app_lea_iso_data_send(buf, len * 2, true, ts, src_lea_db.lc3_tx_seq);
    src_lea_db.lc3_tx_seq++;
}

static void lea_tx_timer_init(void)
{
    src_lea_db.lea_tx_timer_handle = hw_timer_create("lea_send", LEA_SEND_TIMER_LOADCOUNT, true,
                                                     lea_tx_timer_callback);
    hw_timer_lpm_set(src_lea_db.lea_tx_timer_handle, false);
    if (src_lea_db.lea_tx_timer_handle == NULL)
    {
        APP_PRINT_ERROR0("Could not create lea send timer, check hw timer usage");
    }

    APP_PRINT_INFO0("lea_send_timer_init");
}

void lea_tx_timer_start(void)
{
    APP_PRINT_INFO0("lea_tx_timer_start");
    src_lea_db.lea_tx_timer_start = true;
    hw_timer_start(src_lea_db.lea_tx_timer_handle);
}
void lea_tx_timer_stop(void)
{
    APP_PRINT_INFO0("lea_tx_timer_stop");
    src_lea_db.lea_tx_timer_start = false;
    hw_timer_stop(src_lea_db.lea_tx_timer_handle);
}

static void app_usb_lc3_init(void)
{
    uint8_t *p_buf = NULL;

    memset(&src_lea_db, 0, sizeof(T_LEA_DATA_DB));
    p_buf = calloc(1, LC3_TX_RINGBUF_SIZE);
    if (!p_buf)
    {
        return;
    }
    ring_buffer_init(&(src_lea_db.lc3_tx_rbuf), p_buf, LC3_TX_RINGBUF_SIZE);

    lea_tx_timer_init();
}

void lea_tx_timer_reload(uint32_t period_us)
{
    hw_timer_restart(src_lea_db.lea_tx_timer_handle, period_us);
}
#endif

#if F_APP_USB_AUDIO_SUPPORT
void app_src_play_handle_usb_active(T_USB_AUDIO_PIPE_ATTR attr)
{
    /*  usb audio downstream*/
    if (attr.dir == 0)
    {
        usb_ds_attr.active = true;
        usb_ds_attr.sample_rate =  attr.content.audio.sample_rate;;
        usb_ds_attr.chann_num = attr.content.audio.channels;
        usb_ds_attr.bit_width = attr.content.audio.bit_width;
    }
    else
    {
        usb_us_attr.active = true;
        usb_us_attr.sample_rate =  attr.content.audio.sample_rate;;
        usb_us_attr.chann_num = attr.content.audio.channels;
        usb_us_attr.bit_width = attr.content.audio.bit_width;
    }
}

void app_src_play_handle_usb_inactive(uint8_t dir)
{
    if (dir == 0)
    {
        usb_ds_attr.active = false;
    }
    else
    {
        usb_us_attr.active = false;
    }
}
#endif

bool app_audio_path_set_mic_vol_gain(uint16_t gain)
{
    bool ret = false;
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);
    if (p_pipe->handle)
    {
        APP_PRINT_INFO1("app_audio_pipe_set_mic_vol_gain: Set gain %04x", gain);
        ret = audio_pipe_gain_set(p_pipe->handle, gain, gain);
    }
    return ret;
}

bool app_audio_path_set_spk_vol_gain(uint16_t gain)
{
    bool ret = false;
    T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);
    if (p_pipe->handle)
    {
        APP_PRINT_INFO1("app_audio_path_set_spk_vol_gain: Set gain %04x", gain);
        ret = audio_pipe_gain_set(p_pipe->handle, gain, gain);
    }
    return ret;
}

static void app_usb_src_init(void)
{
    uint8_t *p_buf = NULL;
    T_SRC_PIPE *p_encode_pipe = (T_SRC_PIPE *) & (source_play.pipe[0]);
    T_SRC_PIPE *p_decode_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);

    memset(p_encode_pipe, 0, sizeof(T_SRC_PIPE));
    memset(p_decode_pipe, 0, sizeof(T_SRC_PIPE));

    p_buf = calloc(1, USB_ENCODE_SRC_RINGBUF_SIZE);
    if (!p_buf)
    {
        return;
    }
    ring_buffer_init(&(p_encode_pipe->src_rbuf), p_buf, USB_ENCODE_SRC_RINGBUF_SIZE);

    p_buf = calloc(1, USB_DECODE_SRC_RINGBUF_SIZE);
    if (!p_buf)
    {
        return;
    }
    ring_buffer_init(&(p_decode_pipe->src_rbuf), p_buf, USB_DECODE_SRC_RINGBUF_SIZE);
}
#endif

bool app_src_play_bt_recv(uint8_t *buf, uint16_t len, uint8_t flag, uint32_t timestamp)
{
    if (source_play.src_route == SOURCE_ROUTE_USB)
    {
#if F_APP_USB_AUDIO_SUPPORT
        T_RBUF_HDR hdr = {0x0};
        T_SRC_PIPE *p_pipe = (T_SRC_PIPE *) & (source_play.pipe[1]);
        T_RING_BUFFER *p_rbuf = &(p_pipe->src_rbuf);

        //APP_PRINT_INFO3("app_src_play_bt_recv: len %d, flag %x p_pipe->src_fill_len %d", len, flag,
        //                p_pipe->src_fill_len);

        if (len != p_pipe->src_fill_len)
        {
            APP_PRINT_ERROR2("app_src_play_bt_recv: len %d, src_fill_len %d error", len, p_pipe->src_fill_len);
            return false;
        }

        hdr.len = len;

        if (!flag)
        {
            hdr.flag = 0;
        }
        else
        {
            hdr.flag = 1;
        }

        hdr.it = 0;
        hdr.timestamp = timestamp;

        ring_buffer_write(p_rbuf, (void *)&hdr, sizeof(T_RBUF_HDR));
        ring_buffer_write(p_rbuf, buf, len);

        if (p_pipe->state == PIPE_STATE_STARTED)
        {
            APP_PRINT_ERROR0("app_src_play_bt_recv: FIRST");
            usb_decode_pipe_fill();
        }

        return true;
#else
        return false;
#endif
    }
    else if (source_play.src_route == SOURCE_ROUTE_MIC ||
             source_play.src_route == SOURCE_ROUTE_I2S)
    {
        if (source_play.record.default_type == AUDIO_STREAM_TYPE_VOICE)
        {
            uint16_t written_len;
            T_AUDIO_STREAM_STATUS  status;
            if (flag == BT_SCO_PKT_STATUS_OK)
            {
                status = AUDIO_STREAM_STATUS_CORRECT;
            }
            else
            {
                status = AUDIO_STREAM_STATUS_LOST;
            }
            audio_track_write(source_play.record.default_handle, timestamp,
                              source_play.record.play_seq++,
                              status,
                              1,
                              buf,
                              len,
                              &written_len);
            return true;
        }
    }
    return false;
}

void app_src_play_init(void)
{
    app_src_play_a2dp_init();
    app_src_play_hfp_init();

#if F_APP_INTEGRATED_TRANSCEIVER
    app_src_play_pipe_init();
#endif

    audio_mgr_cback_register(app_src_play_track_cback);

#if F_APP_SD_CARD_PLAY
    app_src_playback_init();
#endif
#if F_APP_FWK_PIPE_DEMO_SUPPORT
    app_usb_src_init();
#if (BAP_BROADCAST_SOURCE || BAP_UNICAST_CLIENT)
    app_usb_lc3_init();
#endif
#endif
}

#endif
