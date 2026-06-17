/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include "trace.h"
#include "os_timer.h"
#include "hw_tim.h"
#include "console.h"
#include "audio.h"
#include "btm.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "app_io_msg.h"
#include "app_sbc_data.h"
#include "app_bt_audio_link.h"
#include "app_bt_audio_a2dp.h"

#define  SRC_A2DP_STREAM_MAX_CREDITS  8
#define  A2DP_FRAME_DURATION_MS       16
#define  A2DP_LATENCY_MS              (6 * A2DP_FRAME_DURATION_MS)

T_HW_TIMER_HANDLE a2dp_timer_handle = NULL;
static uint8_t src_a2dp_credits = 8;
static uint32_t sbc_offset = 0;

static uint16_t  a2dp_seq_num = 0;
static uint32_t  a2dp_timestamp = 0;

static const uint8_t a2dp_sink_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x39,//0x55,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AUDIO_SINK >> 8),
    (uint8_t)(UUID_AUDIO_SINK),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVDTP >> 8),
    (uint8_t)(PSM_AVDTP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVDTP >> 8),
    (uint8_t)(UUID_AVDTP),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x03,

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP),
    /*
        //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
        (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x09,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_LANG_ENGLISH >> 8),
        (uint8_t)SDP_LANG_ENGLISH,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
        (uint8_t)SDP_CHARACTER_UTF8,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
        (uint8_t)SDP_BASE_LANG_OFFSET,
    */
    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_ADVANCED_AUDIO_DISTRIBUTION >> 8),
    (uint8_t)(UUID_ADVANCED_AUDIO_DISTRIBUTION),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,//version 1.3
    0x03,

    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES >> 8),
    (uint8_t)SDP_ATTR_SUPPORTED_FEATURES,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,
    0x03
    /*
        //attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x09,
        0x61, 0x32, 0x64, 0x70, 0x5f, 0x73, 0x69, 0x6e, 0x6b //a2dp_sink
    */
};

static const uint8_t a2dp_src_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x39,//0x55,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AUDIO_SOURCE >> 8),
    (uint8_t)(UUID_AUDIO_SOURCE),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVDTP >> 8),
    (uint8_t)(PSM_AVDTP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVDTP >> 8),
    (uint8_t)(UUID_AVDTP),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x03,

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP),
    /*
        //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
        (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x09,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_LANG_ENGLISH >> 8),
        (uint8_t)SDP_LANG_ENGLISH,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
        (uint8_t)SDP_CHARACTER_UTF8,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
        (uint8_t)SDP_BASE_LANG_OFFSET,
    */
    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_ADVANCED_AUDIO_DISTRIBUTION >> 8),
    (uint8_t)(UUID_ADVANCED_AUDIO_DISTRIBUTION),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,//version 1.3
    0x03,

    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES >> 8),
    (uint8_t)SDP_ATTR_SUPPORTED_FEATURES,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,
    0x03
    /*
        //attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x09,
        0x61, 0x32, 0x64, 0x70, 0x5f, 0x73, 0x69, 0x6e, 0x6b //a2dp_sink
    */
};


static void app_a2dp_src_send_data(void)
{
    T_APP_BT_AUDIO_LINK *p_link;

    p_link = app_bt_audio_find_link(app_db.remote_addr);
    if (p_link == NULL)
    {
        return;
    }

    if (src_a2dp_credits)
    {
        if (sbc_offset < sbc_data_len)
        {
            if (sbc_offset + 486 > sbc_data_len)
            {
                if (bt_a2dp_stream_data_send(app_db.remote_addr, a2dp_seq_num, a2dp_timestamp, 6,
                                             (uint8_t *)(sbc_data + sbc_offset),
                                             sbc_data_len - sbc_offset + 1))
                {
                    a2dp_seq_num++;
                    a2dp_timestamp += 6 * p_link->a2dp_codec_info.sbc.block_length *
                                      p_link->a2dp_codec_info.sbc.subbands;
                    src_a2dp_credits--;
                }
            }
            else
            {
                if (bt_a2dp_stream_data_send(app_db.remote_addr, a2dp_seq_num, a2dp_timestamp, 6,
                                             (uint8_t *)(sbc_data + sbc_offset), 486))
                {
                    a2dp_seq_num++;
                    a2dp_timestamp += 6 * p_link->a2dp_codec_info.sbc.block_length *
                                      p_link->a2dp_codec_info.sbc.subbands;
                    src_a2dp_credits--;
                }
            }

            sbc_offset += 486;
        }
        else
        {
            sbc_offset = 0;
            app_a2dp_src_send_data();
        }
    }
}

static void app_a2dp_src_set_stream_status(T_APP_BT_AUDIO_LINK *p_link, bool streaming)
{
    if (p_link->is_streaming == true && streaming == false)
    {
        bt_avrcp_play_status_change_req(p_link->bd_addr, BT_AVRCP_PLAY_STATUS_PAUSED);
    }
    else if (p_link->is_streaming == false && streaming == true)
    {
        bt_avrcp_play_status_change_req(p_link->bd_addr, BT_AVRCP_PLAY_STATUS_PLAYING);
    }

    p_link->is_streaming = streaming;
}

static void app_bt_audio_a2dp_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BT_AUDIO_LINK *p_link;

    switch (event_type)
    {
    case BT_EVENT_SDP_ATTR_INFO:
        {
            T_BT_SDP_ATTR_INFO *sdp_info = &param->sdp_attr_info.info;
            if (app_db.a2dp_role == BT_A2DP_ROLE_SRC)
            {
                if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_AUDIO_SINK)
                {
                    bt_a2dp_connect_req(param->sdp_attr_info.bd_addr, sdp_info->protocol_version, BT_A2DP_ROLE_SNK, 0);
                }
            }
            else if (app_db.a2dp_role == BT_A2DP_ROLE_SNK)
            {
                if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_AUDIO_SOURCE)
                {
                    bt_a2dp_connect_req(param->sdp_attr_info.bd_addr, sdp_info->protocol_version, BT_A2DP_ROLE_SRC, 0);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_CONN_IND:
        p_link = app_bt_audio_find_link(param->a2dp_conn_ind.bd_addr);
        if (p_link != NULL)
        {
            bt_a2dp_connect_cfm(p_link->bd_addr, 0, true);
        }
        break;

    case BT_EVENT_A2DP_CONN_CMPL:
        {
            char *temp_buff = "A2DP Connected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));

            if (app_db.a2dp_role == BT_A2DP_ROLE_SRC)
            {
                p_link = app_bt_audio_find_link(param->a2dp_conn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    memcpy(app_db.remote_addr, param->a2dp_conn_cmpl.bd_addr, 6);

                    src_a2dp_credits = SRC_A2DP_STREAM_MAX_CREDITS;

                    bt_avrcp_connect_req(param->a2dp_conn_cmpl.bd_addr);
                }
            }

            bt_a2dp_active_link_set(param->a2dp_conn_cmpl.bd_addr);
        }

        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            char *temp_buff = "A2DP Disconnected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));

            if (app_db.a2dp_role == BT_A2DP_ROLE_SNK)
            {
                p_link = app_bt_audio_find_link(param->a2dp_disconn_cmpl.bd_addr);
                if (p_link != NULL)
                {
                    if (p_link->a2dp_track_handle != NULL)
                    {
                        audio_track_release(p_link->a2dp_track_handle);
                        p_link->a2dp_track_handle = NULL;
                    }
                }
            }
        }
        break;

    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
            p_link = app_bt_audio_find_link(param->a2dp_config_cmpl.bd_addr);
            if (p_link)
            {
                if (param->a2dp_config_cmpl.role == BT_A2DP_ROLE_SNK)
                {
                    bt_a2dp_stream_delay_report_req(param->a2dp_config_cmpl.bd_addr, 280);
                }

                if (param->a2dp_config_cmpl.codec_type == BT_A2DP_CODEC_TYPE_SBC)
                {
                    p_link->a2dp_codec_info.sbc.sampling_frequency =
                        param->a2dp_config_cmpl.codec_info.sbc.sampling_frequency;
                    p_link->a2dp_codec_info.sbc.channel_mode = param->a2dp_config_cmpl.codec_info.sbc.channel_mode;
                    p_link->a2dp_codec_info.sbc.block_length = param->a2dp_config_cmpl.codec_info.sbc.block_length;
                    p_link->a2dp_codec_info.sbc.subbands = param->a2dp_config_cmpl.codec_info.sbc.subbands;
                    p_link->a2dp_codec_info.sbc.allocation_method =
                        param->a2dp_config_cmpl.codec_info.sbc.allocation_method;
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            a2dp_seq_num = 0;
            a2dp_timestamp = 0;
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        if (app_db.a2dp_role == BT_A2DP_ROLE_SNK)
        {
            p_link = app_bt_audio_find_link(param->a2dp_stream_start_ind.bd_addr);
            if (p_link != NULL)
            {
                T_AUDIO_FORMAT_INFO format_info = {};
                T_AUDIO_STREAM_MODE mode = AUDIO_STREAM_MODE_NORMAL;
                T_AUDIO_STREAM_USAGE stream = AUDIO_STREAM_USAGE_LOCAL;

                bt_a2dp_stream_start_cfm(param->a2dp_stream_start_ind.bd_addr, true);

                if (param->a2dp_stream_start_ind.codec_type == BT_A2DP_CODEC_TYPE_SBC)
                {
                    format_info.type = AUDIO_FORMAT_TYPE_SBC;

                    switch (param->a2dp_stream_start_ind.codec_info.sbc.sampling_frequency)
                    {
                    case BT_A2DP_SBC_SAMPLING_FREQUENCY_16KHZ:
                        format_info.attr.sbc.sample_rate = 16000;
                        break;
                    case BT_A2DP_SBC_SAMPLING_FREQUENCY_32KHZ:
                        format_info.attr.sbc.sample_rate = 32000;
                        break;
                    case BT_A2DP_SBC_SAMPLING_FREQUENCY_44_1KHZ:
                        format_info.attr.sbc.sample_rate = 44100;
                        break;
                    case BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ:
                        format_info.attr.sbc.sample_rate = 48000;
                        break;
                    }

                    switch (param->a2dp_stream_start_ind.codec_info.sbc.channel_mode)
                    {
                    case BT_A2DP_SBC_CHANNEL_MODE_MONO:
                        format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
                        break;
                    case BT_A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL:
                        format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_DUAL;
                        break;
                    case BT_A2DP_SBC_CHANNEL_MODE_STEREO:
                        format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_STEREO;
                        break;
                    case BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO:
                        format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
                        break;
                    }

                    switch (param->a2dp_stream_start_ind.codec_info.sbc.block_length)
                    {
                    case BT_A2DP_SBC_BLOCK_LENGTH_4:
                        format_info.attr.sbc.block_length = 4;
                        break;
                    case BT_A2DP_SBC_BLOCK_LENGTH_8:
                        format_info.attr.sbc.block_length = 8;
                        break;
                    case BT_A2DP_SBC_BLOCK_LENGTH_12:
                        format_info.attr.sbc.block_length = 12;
                        break;
                    case BT_A2DP_SBC_BLOCK_LENGTH_16:
                        format_info.attr.sbc.block_length = 16;
                        break;
                    }

                    switch (param->a2dp_stream_start_ind.codec_info.sbc.subbands)
                    {
                    case BT_A2DP_SBC_SUBBANDS_4:
                        format_info.attr.sbc.subband_num = 4;
                        break;
                    case BT_A2DP_SBC_SUBBANDS_8:
                        format_info.attr.sbc.subband_num = 8;
                        break;
                    }

                    switch (param->a2dp_stream_start_ind.codec_info.sbc.allocation_method)
                    {
                    case BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS:
                        format_info.attr.sbc.allocation_method = 0;
                        break;
                    case BT_A2DP_SBC_ALLOCATION_METHOD_SNR:
                        format_info.attr.sbc.allocation_method = 1;
                        break;
                    }
                }

                if (p_link->a2dp_track_handle)
                {
                    audio_track_release(p_link->a2dp_track_handle);
                }

                p_link->a2dp_track_handle = audio_track_create(AUDIO_STREAM_TYPE_PLAYBACK,
                                                               mode,
                                                               stream,
                                                               format_info,
                                                               p_link->a2dp_curr_volume,
                                                               0,
                                                               AUDIO_DEVICE_OUT_DEFAULT,
                                                               NULL,
                                                               NULL);


                if (p_link->a2dp_track_handle != NULL)
                {
                    audio_track_latency_set(p_link->a2dp_track_handle, A2DP_LATENCY_MS, true);
                    audio_track_threshold_set(p_link->a2dp_track_handle,
                                              12 * A2DP_FRAME_DURATION_MS,
                                              2 * A2DP_FRAME_DURATION_MS);

                    bt_a2dp_stream_delay_report_req(p_link->bd_addr, A2DP_LATENCY_MS);
                    audio_track_start(p_link->a2dp_track_handle);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        if (app_db.a2dp_role == BT_A2DP_ROLE_SRC)
        {
            p_link = app_bt_audio_find_link(param->a2dp_stream_start_rsp.bd_addr);
            if (p_link != NULL)
            {
                char *temp_buff = "A2DP Streaming Started!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));

                app_a2dp_src_set_stream_status(p_link, true);

                //start timer to send media data
                hw_timer_restart(a2dp_timer_handle, A2DP_FRAME_DURATION_MS * 1000);
                app_a2dp_src_send_data();
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_DATA_IND:
        if (app_db.a2dp_role == BT_A2DP_ROLE_SNK)
        {
            p_link = app_bt_audio_find_link(param->a2dp_stream_data_ind.bd_addr);
            if (p_link != NULL)
            {
                uint16_t written_len;

                audio_track_write(p_link->a2dp_track_handle,
                                  param->a2dp_stream_data_ind.bt_clock,
                                  param->a2dp_stream_data_ind.seq_num,
                                  AUDIO_STREAM_STATUS_CORRECT,
                                  param->a2dp_stream_data_ind.frame_num,
                                  param->a2dp_stream_data_ind.payload,
                                  param->a2dp_stream_data_ind.len,
                                  &written_len);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_DATA_RSP:
        if (app_db.a2dp_role == BT_A2DP_ROLE_SRC)
        {
            src_a2dp_credits++;
            if (src_a2dp_credits > SRC_A2DP_STREAM_MAX_CREDITS)
            {
                src_a2dp_credits = SRC_A2DP_STREAM_MAX_CREDITS;
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            p_link = app_bt_audio_find_link(param->a2dp_stream_stop.bd_addr);
            if (p_link != NULL)
            {
                if (app_db.a2dp_role == BT_A2DP_ROLE_SNK)
                {
                    if (p_link->a2dp_track_handle != NULL)
                    {
                        audio_track_release(p_link->a2dp_track_handle);
                        p_link->a2dp_track_handle = NULL;
                    }
                }
                else
                {
                    char *temp_buff = "A2DP Streaming Suspended!\r\n";
                    console_write((uint8_t *)temp_buff, strlen(temp_buff));

                    hw_timer_stop(a2dp_timer_handle);
                    app_a2dp_src_set_stream_status(p_link, false);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        if (app_db.a2dp_role == BT_A2DP_ROLE_SNK)
        {
            p_link = app_bt_audio_find_link(param->a2dp_stream_close.bd_addr);
            if (p_link != NULL)
            {
                if (p_link->a2dp_track_handle != NULL)
                {
                    audio_track_release(p_link->a2dp_track_handle);
                    p_link->a2dp_track_handle = NULL;
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_bt_audio_send_stream_callback(void *xtimer)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_A2DP_SRC;
    gpio_msg.subtype = IO_MSG_SUBTYPE_A2DP_DATA_REQ;
    if (app_io_send_msg(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("a2dp hw timer: msg send fail");
    }
}

void app_bt_audio_a2dp_handle_msg(T_IO_MSG msg)
{
    uint16_t subtype = msg.subtype;

    switch (subtype)
    {
    case IO_MSG_SUBTYPE_A2DP_DATA_REQ: // timer msg peek data from buf_pool and send data
        {
            app_a2dp_src_send_data();
        }
        break;

    default:
        break;
    }
}

void app_bt_audio_a2dp_init(uint8_t role)
{
    int32_t ret = 0;
    T_BT_A2DP_STREAM_ENDPOINT sep;

    app_db.a2dp_role = role;

    if (role == BT_A2DP_ROLE_SNK)
    {
        if (bt_sdp_record_add((void *)a2dp_sink_sdp_record) == false)
        {
            ret = -1;
            goto fail_sdp_add;
        }
    }
    else
    {
        if (bt_sdp_record_add((void *)a2dp_src_sdp_record) == false)
        {
            ret = -1;
            goto fail_sdp_add;
        }

        a2dp_timer_handle = hw_timer_create("a2dp_hw_timer", 21333, true,
                                            app_bt_audio_send_stream_callback);
    }

    if (bt_a2dp_init(BT_A2DP_CAPABILITY_MEDIA_TRANSPORT | BT_A2DP_CAPABILITY_MEDIA_CODEC |
                     BT_A2DP_CAPABILITY_DELAY_REPORTING) == false)
    {
        ret = -2;
        goto fail_init;
    }

    if (role == BT_A2DP_ROLE_SRC)
    {
        sep.role = BT_A2DP_ROLE_SRC;
        sep.codec_type = BT_A2DP_CODEC_TYPE_SBC;
        sep.u.codec_sbc.sampling_frequency_mask = BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ;
        sep.u.codec_sbc.channel_mode_mask = BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO;
        sep.u.codec_sbc.block_length_mask = BT_A2DP_SBC_BLOCK_LENGTH_16;
        sep.u.codec_sbc.subbands_mask = BT_A2DP_SBC_SUBBANDS_8;
        sep.u.codec_sbc.allocation_method_mask = BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS;
        sep.u.codec_sbc.min_bitpool = 0x02;
        sep.u.codec_sbc.max_bitpool = 0x23;
    }
    else
    {
        sep.role = BT_A2DP_ROLE_SNK;
        sep.codec_type = BT_A2DP_CODEC_TYPE_SBC;
        sep.u.codec_sbc.sampling_frequency_mask = BT_A2DP_SBC_SAMPLING_FREQUENCY_44_1KHZ |
                                                  BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ;
        sep.u.codec_sbc.channel_mode_mask = BT_A2DP_SBC_CHANNEL_MODE_MONO |
                                            BT_A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL |
                                            BT_A2DP_SBC_CHANNEL_MODE_STEREO |
                                            BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO;
        sep.u.codec_sbc.block_length_mask = BT_A2DP_SBC_BLOCK_LENGTH_4 |
                                            BT_A2DP_SBC_BLOCK_LENGTH_8 |
                                            BT_A2DP_SBC_BLOCK_LENGTH_12 |
                                            BT_A2DP_SBC_BLOCK_LENGTH_16;
        sep.u.codec_sbc.subbands_mask = BT_A2DP_SBC_SUBBANDS_4 |
                                        BT_A2DP_SBC_SUBBANDS_8;
        sep.u.codec_sbc.allocation_method_mask = BT_A2DP_SBC_ALLOCATION_METHOD_SNR |
                                                 BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS;
        sep.u.codec_sbc.min_bitpool = 0x02;
        sep.u.codec_sbc.max_bitpool = 0x23;
    }

    if (bt_a2dp_stream_endpoint_add(sep) == false)
    {
        ret = -3;
        goto fail_set_sep;
    }

    bt_mgr_cback_register(app_bt_audio_a2dp_cback);

    return;

fail_set_sep:
fail_init:
fail_sdp_add:
    APP_PRINT_ERROR1("app_bt_audio_a2dp_init: failed %d", ret);
}

bool app_bt_audio_a2dp_connect(uint8_t *bd_addr)
{
    T_BT_SDP_UUID_DATA uuid;

    if (app_db.a2dp_role == BT_A2DP_ROLE_SRC)
    {
        uuid.uuid_16 = UUID_AUDIO_SINK;
    }
    else
    {
        uuid.uuid_16 = UUID_AUDIO_SOURCE;
    }

    return bt_sdp_discov_start(bd_addr, BT_SDP_UUID16, uuid);
}

bool app_bt_audio_a2dp_disconnect(uint8_t *bd_addr)
{
    T_APP_BT_AUDIO_LINK *p_link;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_a2dp_disconnect_req(bd_addr);
    }

    return false;
}

bool app_bt_audio_a2dp_start(uint8_t *bd_addr)
{
    T_APP_BT_AUDIO_LINK *p_link;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_a2dp_stream_start_req(bd_addr);
    }

    return false;
}

bool app_bt_audio_a2dp_suspend(uint8_t *bd_addr)
{
    T_APP_BT_AUDIO_LINK *p_link;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_a2dp_stream_suspend_req(bd_addr);
    }

    return false;
}

bool app_bt_audio_a2dp_volume_up(uint8_t *bd_addr)
{
    T_APP_BT_AUDIO_LINK *p_link;
    uint8_t              vol;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t temp_buff[40];
        uint16_t buf_len;

        if (p_link->a2dp_curr_volume < A2DP_VOLUME_MAX_LEVEL)
        {
            p_link->a2dp_curr_volume++;

            buf_len = sprintf((char *)temp_buff, "A2DP Volume Level: %d \r\n", p_link->a2dp_curr_volume);
            console_write(temp_buff, buf_len);
        }
        else
        {
            p_link->a2dp_curr_volume = A2DP_VOLUME_MAX_LEVEL;

            buf_len = sprintf((char *)temp_buff, "A2DP Volume Max Level: %d \r\n", A2DP_VOLUME_MAX_LEVEL);
            console_write(temp_buff, buf_len);

            return false;
        }

        vol = (p_link->a2dp_curr_volume * 0x7F + A2DP_VOLUME_MAX_LEVEL / 2) /
              A2DP_VOLUME_MAX_LEVEL;

        if (app_db.a2dp_role == BT_A2DP_ROLE_SNK)
        {
            if (p_link->a2dp_track_handle != NULL)
            {
                audio_track_volume_out_set(p_link->a2dp_track_handle, p_link->a2dp_curr_volume);

                bt_avrcp_volume_change_req(bd_addr, vol);
            }
        }
        else
        {
            bool abs_vol_support = true;

            if (abs_vol_support)
            {
                bt_avrcp_absolute_volume_set(bd_addr, vol);
            }
            else
            {
                /* adjust the volume of audio source */
            }
        }

        return true;
    }

    return false;
}

bool app_bt_audio_a2dp_volume_down(uint8_t *bd_addr)
{
    T_APP_BT_AUDIO_LINK *p_link;
    uint8_t              vol;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t temp_buff[40];
        uint16_t buf_len;

        if (p_link->a2dp_curr_volume > A2DP_VOLUME_MIN_LEVEL)
        {
            p_link->a2dp_curr_volume--;

            buf_len = sprintf((char *)temp_buff, "A2DP Volume Level: %d \r\n", p_link->a2dp_curr_volume);
            console_write(temp_buff, buf_len);
        }
        else
        {
            p_link->a2dp_curr_volume = A2DP_VOLUME_MIN_LEVEL;

            buf_len = sprintf((char *)temp_buff, "A2DP Volume Min Level: %d \r\n", A2DP_VOLUME_MIN_LEVEL);
            console_write(temp_buff, buf_len);

            return false;
        }

        vol = (p_link->a2dp_curr_volume * 0x7F + A2DP_VOLUME_MAX_LEVEL / 2) /
              A2DP_VOLUME_MAX_LEVEL;

        if (app_db.a2dp_role == BT_A2DP_ROLE_SNK)
        {
            if (p_link->a2dp_track_handle != NULL)
            {
                audio_track_volume_out_set(p_link->a2dp_track_handle, p_link->a2dp_curr_volume);

                bt_avrcp_volume_change_req(bd_addr, vol);
            }
        }
        else
        {
            bool abs_vol_support = true;

            if (abs_vol_support)
            {
                bt_avrcp_absolute_volume_set(bd_addr, vol);
            }
            else
            {
                /* adjust the volume of audio source */
            }
        }

        return true;
    }

    return false;
}
