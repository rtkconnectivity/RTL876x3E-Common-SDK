/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
/* Includes -----------------------------------------------------------------*/
#include "os_mem.h"
#include "trace.h"
#include "ring_buffer.h"
#include "audio_fs_decode.h"
#include "playback_audio_file.h"
#include "app_customer_audio_policy.h"


/* Defines ------------------------------------------------------------------*/

/* Globals ------------------------------------------------------------------*/
static struct
{
    T_RING_BUFFER       ring_buf;
    uint8_t             *buffer;
    uint32_t            write_offset;
    uint32_t            read_offset;
    uint16_t            frame_size;
    uint8_t             state;
    uint32_t            total_play_time;
} playback_stream;

static FRAME_INFO *s_frame_info;
void (*playback_audio_report_playtime_hook)(void) = NULL;

uint8_t playback_audio_file_init(void)
{
    memset(&playback_stream, 0, sizeof(playback_stream));

#if (TARGET_RTL8773DO || TARGET_RTL8773DFL || TARGET_RTL87X3EP)
    playback_stream.buffer = (uint8_t *)malloc(STREAM_BUF_SIZE);
#else
    playback_stream.buffer = (uint8_t *)os_mem_alloc(RAM_TYPE_DSPSHARE, STREAM_BUF_SIZE);
#endif

    memset(playback_stream.buffer, 0, STREAM_BUF_SIZE);
    ring_buffer_init(&playback_stream.ring_buf, playback_stream.buffer, STREAM_BUF_SIZE);

    if (s_frame_info != NULL)
    {
        free(s_frame_info);
    }

#if (TARGET_RTL8773DO || TARGET_RTL8773DFL || TARGET_RTL87X3EP)
    s_frame_info = (FRAME_INFO *)malloc(sizeof(FRAME_INFO));
#else
    s_frame_info = (FRAME_INFO *)os_mem_alloc(RAM_TYPE_DSPSHARE, sizeof(FRAME_INFO));
#endif

    audio_fs_decode_deinit();

    return PLAYBACK_AF_SUCCESS;
}

static void playback_audio_file_offset_print(void)
{
    APP_PRINT_INFO2("steam_file_offset wr:0x%x, rd:0x%x",
                    playback_stream.write_offset, playback_stream.read_offset);
}

uint8_t playback_audio_write_stream(uint8_t *data, uint16_t len)
{
    uint32_t space_size = 0;
    uint32_t used_size = 0;
    uint32_t real_wr = 0;

    space_size = ring_buffer_get_remaining_space(&playback_stream.ring_buf);
    used_size = ring_buffer_get_data_count(&playback_stream.ring_buf);
    APP_PRINT_TRACE2("playback_audio_write_stream space_size %d, used_size:%d", space_size, used_size);

    if (space_size < len)
    {
        return PLAYBACK_AF_LENGTH_ERROR;
    }
    real_wr = ring_buffer_write(&playback_stream.ring_buf, data, len);
    playback_stream.write_offset += real_wr;
    used_size = ring_buffer_get_data_count(&playback_stream.ring_buf);
//    APP_PRINT_TRACE2("playback_audio_write_stream, usedlen %d, %b", used_size, TRACE_BINARY(8, data));
    playback_audio_file_offset_print();

    return 0;
}

uint8_t stream_check_and_request_data(void)
{
    uint32_t space_size = 0;
    uint32_t used_size = 0;
    space_size = ring_buffer_get_remaining_space(&playback_stream.ring_buf);
    used_size = ring_buffer_get_data_count(&playback_stream.ring_buf);
    APP_PRINT_TRACE2("stream_check_and_request_data space_size %d, used_size:%d", space_size,
                     used_size);
    if ((space_size > STREAM_BUF_SIZE / 4) && (playback_stream.state == PLAYBACK_AUDIO_FILE_START))
    {
        app_customer_audio_request_frame();
    }
    return 0;
}

uint16_t stream_read_data(uint8_t *rd_buf, uint32_t len, uint32_t *real_len)
{
    uint16_t ret = AUDIO_STREAM_SUCCESS;
    uint32_t space_size = 0;
    uint32_t used_size = 0;

    space_size = ring_buffer_get_remaining_space(&playback_stream.ring_buf);
    used_size = ring_buffer_get_data_count(&playback_stream.ring_buf);
    APP_PRINT_TRACE3("stream_read_data used_size %d, space_size %d, len:%d",
                     used_size, space_size, len);
    if (len > used_size)
    {
        return AUDIO_STREAM_READ_ERROR;
    }
    *real_len = ring_buffer_read(&playback_stream.ring_buf, len, rd_buf);
    playback_stream.read_offset += *real_len;
    playback_audio_file_offset_print();
    return ret;
}

uint16_t stream_peek_data(uint8_t *rd_buf, uint32_t len, uint32_t *real_len)
{
    uint16_t ret = AUDIO_STREAM_SUCCESS;
    uint32_t space_size = 0;
    uint32_t used_size = 0;

    space_size = ring_buffer_get_remaining_space(&playback_stream.ring_buf);
    used_size = ring_buffer_get_data_count(&playback_stream.ring_buf);
    APP_PRINT_TRACE3("stream_peek_data used_size %d, space_size %d, len:%d",
                     used_size, space_size, len);
    if (len > used_size)
    {
        return AUDIO_STREAM_READ_ERROR;
    }
    *real_len = ring_buffer_peek(&playback_stream.ring_buf, len, rd_buf);
    return ret;
}

uint16_t stream_peek_data_2(uint8_t *rd_buf, uint32_t offset, uint32_t len, uint32_t *real_len)
{
    uint16_t ret = AUDIO_STREAM_SUCCESS;
    uint32_t space_size = 0;
    uint32_t used_size = 0;
    uint8_t *p_buf;
    uint32_t ring_len;

    space_size = ring_buffer_get_remaining_space(&playback_stream.ring_buf);
    used_size = ring_buffer_get_data_count(&playback_stream.ring_buf);
    ring_len = offset + len;
    APP_PRINT_WARN4("stream_peek_data_2: used_size: 0x%x, space_size: 0x%x, offset: 0x%x, len: 0x%x",
                    used_size, space_size, offset, len);
    if (ring_len > used_size)
    {
        ret = AUDIO_STREAM_READ_ERROR;
        goto fail_state;
    }

    if (offset == 0)
    {
        *real_len = ring_buffer_peek(&playback_stream.ring_buf, len, rd_buf);
    }
    else if (offset > 5120)
    {
        ret = AUDIO_STREAM_READ_ERROR;
        goto fail_state;
    }
    else
    {

#if (TARGET_RTL8773DO || TARGET_RTL8773DFL || TARGET_RTL87X3EP)
        p_buf = (uint8_t *)malloc(ring_len);
#else
        p_buf = (uint8_t *)os_mem_alloc(RAM_TYPE_DSPSHARE, ring_len);
#endif

        if (p_buf == NULL)
        {
            ret = PLAYBACK_AF_MALLOC_ERROR;
            goto fail_state;
        }
        *real_len = ring_buffer_peek(&playback_stream.ring_buf, ring_len, p_buf);
        memcpy(rd_buf, p_buf + offset, len);
        free(p_buf);
    }

    return ret;
fail_state:
    APP_PRINT_ERROR1("stream_peek_data_2,ret:%d", ret);
    return ret;
}

uint16_t stream_remove_data(uint32_t len, uint32_t *real_len)
{
    uint16_t ret = AUDIO_STREAM_SUCCESS;
    uint32_t space_size = 0;
    uint32_t used_size = 0;

    space_size = ring_buffer_get_remaining_space(&playback_stream.ring_buf);
    used_size = ring_buffer_get_data_count(&playback_stream.ring_buf);

    if (len > used_size)
    {
        ret = AUDIO_STREAM_READ_ERROR;
    }
    *real_len = ring_buffer_remove(&playback_stream.ring_buf, len);
    playback_stream.read_offset += *real_len;

    APP_PRINT_INFO4("stream_remove_data: ret: %d, used_size: 0x%x, space_size: 0x%x, len: 0x%x",
                    ret, used_size, space_size, len);
    return ret;
}

uint32_t stream_get_data_size(void)
{
    uint32_t used_size;

    used_size = ring_buffer_get_data_count(&playback_stream.ring_buf);
    return used_size;
}

void stream_clear_data(void)
{
    ring_buffer_clear(&playback_stream.ring_buf);
}

void playback_audio_file_offset_reset(void)
{
    playback_stream.write_offset = 0;
    playback_stream.read_offset = 0;
}

/**
  * @brief Initialize FATFS file system and sd card physical layer.
  * @param   No parameter.
  * @return  void
  */

uint8_t playback_audio_file_open(uint8_t *pFilename, uint16_t namelen)
{
    uint8_t ret = PLAYBACK_AF_SUCCESS;

    return ret;
}

uint8_t playback_audio_file_close(void)
{
    uint8_t ret = PLAYBACK_AF_SUCCESS;

    return ret;
}

uint8_t playback_audio_file_read(uint8_t *rd_buf, uint32_t rd_len,
                                 uint32_t *actual_len)
{
    uint16_t ret = PLAYBACK_AF_SUCCESS;

    return ret;
}

uint8_t playback_audio_file_write(uint8_t *wr_buf, uint32_t wr_len, uint32_t *actual_len)
{
    return PLAYBACK_AF_WRITE_ERROR;

}

uint8_t playback_audio_file_get_audio_info(T_PLAYBACK_AF_FORMAT_INFO *as_format_info)
{
    uint8_t ret = PLAYBACK_AF_SUCCESS;
    uint16_t ret_fs = 0;
    uint32_t sample_rate = 0;
    uint16_t sample_counts = 1024; /* default */
    uint16_t frame_duration = 20; /* default */
    uint8_t channel_mode = 0;
    uint32_t bit_rate = 0;

    FRAME_CONTENT frameContent;
    if (s_frame_info != NULL)
    {
        ret_fs = audio_fs_decode_get_frame(NULL, &frameContent);
        if (ret_fs == 0)
        {
            audio_fs_decode_get_frame_info(s_frame_info);
            //audio_fs_set_file_offset(handle, offset);
            as_format_info->frame_size = frameContent.length;
            playback_stream.frame_size = frameContent.length;
        }
    }
    else
    {
        ret = PLAYBACK_AF_READ_ERROR;
        return ret;
    }

    if (ret_fs == 0)
    {
        if ((s_frame_info->format == RTK) && (s_frame_info->format_info.rtk.rtkTransFormat == RTK_SBC))
        {
            as_format_info->format_info.type = AUDIO_FORMAT_TYPE_SBC;
            as_format_info->format_info.frame_num = 6;
            as_format_info->format_info.attr.sbc.sample_rate =
                s_frame_info->format_info.rtk.rtk_trans_info.sbc.sampling_frequency;
            as_format_info->format_info.attr.sbc.block_length =
                s_frame_info->format_info.rtk.rtk_trans_info.sbc.block_length;
            as_format_info->format_info.attr.sbc.subband_num =
                s_frame_info->format_info.rtk.rtk_trans_info.sbc.subbands;
            as_format_info->format_info.attr.sbc.allocation_method =
                s_frame_info->format_info.rtk.rtk_trans_info.sbc.allocation_method;
            as_format_info->format_info.attr.sbc.bitpool =
                s_frame_info->format_info.rtk.rtk_trans_info.sbc.bitpool;
            switch (s_frame_info->format_info.rtk.rtk_trans_info.sbc.channel_mode)
            {
            case 0:
                as_format_info->format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO; // Mono
                break;
            case 1:
                as_format_info->format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_DUAL; // Mono
                break;
            case 2:
                as_format_info->format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_STEREO; // Stereo
                break;
            case 3:
                as_format_info->format_info.attr.sbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO; // Stereo
                break;
            default:
                APP_PRINT_ERROR0("channel not supprot");
                break;
            }
            sample_rate = as_format_info->format_info.attr.sbc.sample_rate;
            APP_PRINT_INFO6("playback_audio_file_get_audio_info: RTK_SBC, samplefre:%d, channel_mode:%d,"
                            "block_length:%d, subbands:%d, allocation_method:%d, bitpool:%d",
                            as_format_info->format_info.attr.sbc.sample_rate,
                            as_format_info->format_info.attr.sbc.chann_mode,
                            as_format_info->format_info.attr.sbc.block_length,
                            as_format_info->format_info.attr.sbc.subband_num,
                            as_format_info->format_info.attr.sbc.allocation_method,
                            as_format_info->format_info.attr.sbc.bitpool);
        }
        else if ((s_frame_info->format == AAC) ||
                 ((s_frame_info->format == RTK) && (s_frame_info->format_info.rtk.rtkTransFormat == RTK_LATM)) ||
                 ((s_frame_info->format == RTK) && (s_frame_info->format_info.rtk.rtkTransFormat == RTK_ADTS)))
        {
            as_format_info->format_info.type = AUDIO_FORMAT_TYPE_AAC;
            as_format_info->format_info.frame_num = 1;
            if ((s_frame_info->format == RTK) && (s_frame_info->format_info.rtk.rtkTransFormat == RTK_ADTS))
            {
                as_format_info->format_info.attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_ADTS;

                sample_rate = s_frame_info->format_info.rtk.rtk_trans_info.adts.sampling_frequency;
                channel_mode = s_frame_info->format_info.rtk.rtk_trans_info.adts.channel_mode;
                bit_rate = 0;
                frame_duration = s_frame_info->format_info.rtk.rtk_trans_info.adts.frame_duration;  // lemom_test
            }
            else if ((s_frame_info->format == RTK) &&
                     (s_frame_info->format_info.rtk.rtkTransFormat == RTK_LATM))
            {
                as_format_info->format_info.attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_LATM;

                sample_rate = s_frame_info->format_info.rtk.rtk_trans_info.latm.sampling_frequency;
                channel_mode = s_frame_info->format_info.rtk.rtk_trans_info.latm.channel_mode;
                bit_rate = 0;
            }
            else if (s_frame_info->format == AAC)
            {
                as_format_info->format_info.attr.aac.transport_format = AUDIO_AAC_TRANSPORT_FORMAT_ADTS;

                sample_rate = s_frame_info->format_info.aac.sampling_frequency;
                channel_mode = s_frame_info->format_info.aac.channel_mode;
                bit_rate = 0;
                frame_duration = s_frame_info->format_info.aac.frame_duration;  // lemom_test
            }

            as_format_info->format_info.attr.aac.sample_rate = sample_rate;
            as_format_info->format_info.attr.aac.bitrate = bit_rate;
            switch (channel_mode)
            {
            case AOT_SPECIFIC_CONFIG:
                as_format_info->format_info.attr.aac.chann_num = 1; // Mono
                break;
            case CHANNEL_1PCS:
            case CHANNEL_2PCS:
            case CHANNEL_3PCS:
            case CHANNEL_4PCS:
            case CHANNEL_5PCS:
            case CHANNEL_6PCS:
            case CHANNEL_7PCS:
                as_format_info->format_info.attr.aac.chann_num = 2; // Stereo
                break;
            default:
                APP_PRINT_ERROR0("channel not supprot");
                break;
            }
            APP_PRINT_INFO4("playback_audio_file_get_audio_info: AAC,"
                            " transport_format:%d, samplefrequency:%d, channel_mode:%d, bitrate:%d",
                            as_format_info->format_info.attr.aac.transport_format,
                            as_format_info->format_info.attr.aac.sample_rate,
                            as_format_info->format_info.attr.aac.chann_num,
                            as_format_info->format_info.attr.aac.bitrate);
        }
        else if (s_frame_info->format == MP3)
        {
            as_format_info->format_info.type = AUDIO_FORMAT_TYPE_MP3;
            as_format_info->format_info.frame_num = 1;
            as_format_info->format_info.attr.mp3.sample_rate = s_frame_info->format_info.mp3.sampling_frequency;
            as_format_info->format_info.attr.mp3.bitrate = s_frame_info->format_info.mp3.bit_rate;
            as_format_info->format_info.attr.mp3.version = s_frame_info->format_info.mp3.version;
            as_format_info->format_info.attr.mp3.layer = s_frame_info->format_info.mp3.layer;
            switch (s_frame_info->format_info.mp3.channel_mode)
            {
            case CHANNEL_STEREO:
            case CHANNEL_JOINT_STEREO:
            case CHANNEL_DOUBLE:
                as_format_info->format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_DUAL;
                break;
            case CHANNEL_SINGLE:
                as_format_info->format_info.attr.mp3.chann_mode = AUDIO_MP3_CHANNEL_MODE_MONO;
                break;

            default:
                break;
            }
            sample_rate = as_format_info->format_info.attr.mp3.sample_rate;
            sample_counts = s_frame_info->format_info.mp3.sample_counts;
            frame_duration = s_frame_info->format_info.mp3.frame_duration;
            APP_PRINT_INFO5("playback_audio_file_get_audio_info: MP3, samplefrequency:%d, channel_mode:%d, "
                            "bitrate:%d, version: %d, layer:%d",
                            as_format_info->format_info.attr.mp3.sample_rate,
                            as_format_info->format_info.attr.mp3.chann_mode,
                            as_format_info->format_info.attr.mp3.bitrate,
                            as_format_info->format_info.attr.mp3.version,
                            as_format_info->format_info.attr.mp3.layer);
            APP_PRINT_INFO2("sample_counts %d, frame_duration %d", sample_counts, frame_duration);
        }
        else
        {
            APP_PRINT_ERROR1("playback_audio_file_get_audio_info: music type error, 0x%x",
                             s_frame_info->format);
            return PLAYBACK_AF_TYPE_ERROR;
        }
        as_format_info->frame_duration = frame_duration;
        as_format_info->sample_counts = sample_counts;
    }
    else
    {
        ret = PLAYBACK_AF_READ_ERROR;
    }
    return ret;
}

uint8_t playback_audio_file_get_frame(T_PLAYBACK_FRAME_PKT *p_frame_pkt)
{
    uint8_t ret = PLAYBACK_AF_SUCCESS;
    uint16_t ret_fs = 0;
    FRAME_CONTENT playback_frame;
    uint32_t play_time = 0;

//    if (playback_audio_fs_handle == NULL)
//    {
//        return PLAYBACK_AF_READ_ERROR;
//    }
    if (p_frame_pkt == NULL)
    {
        return PLAYBACK_AF_READ_ERROR;
    }

    // check the end of file
    if (playback_audio_file_is_end())
    {
        ret = PLAYBACK_AF_END_ERROR;
    }
    else
    {
        playback_audio_file_offset_print();
        ret_fs = audio_fs_decode_get_frame(NULL, &playback_frame);
        if (ret_fs != 0)
        {
            APP_PRINT_INFO1("playback_audio_file_get_frame: res:0x%x", ret_fs);
            ret = PLAYBACK_AF_END_ERROR;
            // remove STREAM_BUF_REMEDY_SIZE
            if (stream_get_data_size() >= STREAM_BUF_REMEDY_SIZE)
            {
                uint32_t read_len = 0;
                stream_remove_data(STREAM_BUF_REMEDY_SIZE, &read_len);
            }
        }
        else
        {
            audio_fs_decode_get_frame_info(s_frame_info);
            if (s_frame_info->format == MP3)
            {
                play_time = playback_audio_file_get_play_time();
                play_time += s_frame_info->format_info.mp3.frame_duration;
                playback_audio_file_set_play_time(play_time);
            }
            else
            {
                ret = PLAYBACK_AF_TYPE_ERROR;
            }

        }
    }

    if (ret != 0)
    {
        APP_PRINT_ERROR1("playback_audio_file_get_frame ERROR,RES:0x%x", ret);
    }
    else
    {
        p_frame_pkt->frame_num = playback_frame.frameNum;
        p_frame_pkt->buf = playback_frame.content;
        p_frame_pkt->length = playback_frame.length;
    }

    return ret;
}

void playback_audio_file_set_play_time(uint32_t play_time_ms)
{
    uint32_t now_play_time_s = play_time_ms / 1000;
    uint32_t old_play_time_s = playback_stream.total_play_time / 1000;

    playback_stream.total_play_time = play_time_ms;
    if (now_play_time_s > old_play_time_s)
    {
        //report the time to master;
        APP_PRINT_TRACE3("playback_audio_file_set_play_time, old time: %d s, now time:%d s, playtime:%d ms",
                         old_play_time_s, now_play_time_s, play_time_ms);
        if (playback_audio_report_playtime_hook)
        {
            playback_audio_report_playtime_hook();
        }
    }
}

uint32_t playback_audio_file_get_play_time(void)
{
    return playback_stream.total_play_time;
}

uint8_t playback_audio_file_get_header(uint32_t *p_length)
{
    uint32_t offset = 0;
    uint16_t header_type = 0;
    uint8_t type = PLAYBACK_AF_DATA_FRAME;

    header_type = audio_fs_get_file_header_length(NULL, &offset);
    *p_length = offset;

    switch (header_type)
    {
    case AUDIO_FS_ERR_READ:
    case FS_END_OF_FILE:
        type = PLAYBACK_AF_ERR_HEADER;
        break;

    case ID3V2_HEADER:
    case ID3V1_HEADER:
    case XING_HEADER:
    case XING_OR_MP3_HEADER:
    case HEADER_TYPE_NONE:
        type = PLAYBACK_AF_IS_HEADER;
        break;

    case MP3_HEADER:
    case AAC_HEADER:
    case RTK_ADTS_HEADER:
    case RTK_LATM_HEADER:
    case RTK_SBC_HEADER:
    case MP4_HEADER:
        type = PLAYBACK_AF_DATA_FRAME;
        break;

    default:
        break;
    }

    return type;
}

void playback_audio_file_set_state(uint8_t file_state)
{
    playback_stream.state = file_state;
}

uint8_t playback_audio_file_get_state(void)
{
    return playback_stream.state;
}

bool playback_audio_file_is_end(void)
{
    bool file_is_end = false;
    uint16_t empty_threshold = STREAM_BUF_EMPTY_LEVEL;

    if (playback_stream.frame_size < STREAM_BUF_EMPTY_LEVEL)
    {
        empty_threshold = 2 * playback_stream.frame_size;
    }

    if ((playback_stream.state == PLAYBACK_AUDIO_FILE_STOPPED) ||
        (playback_stream.state == PLAYBACK_AUDIO_FILE_STOPPING &&
         (stream_get_data_size() < empty_threshold)))
    {
        APP_PRINT_WARN2("playback_audio_file_is_end, remain size:%d, empty threshold:%d",
                        stream_get_data_size(), empty_threshold);
        file_is_end = true;
    }

    return file_is_end;
}

#endif


