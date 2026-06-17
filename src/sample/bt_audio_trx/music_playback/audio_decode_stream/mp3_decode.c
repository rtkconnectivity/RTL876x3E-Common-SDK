/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
#include "audio_fs_decode.h"

#if MP3_FORMAT_SUPPORT
/*============================================================================*
  *                                  Variables
  *============================================================================*/
ID3V2_INFO  mp3Id3v2Info;
extern FRAME_INFO g_frameInfo;

const static uint8_t mpeg_index[4] = {2, 0, 1, 0};
const static uint8_t layer_index[4] = {0, 2, 1, 0};
const static uint16_t MP3_Bitrate_Table[9][15] =
{
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},//V1,L1
    {0, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384},//V1,L2
    {0, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320},//V1,L3

    {0, 32, 48, 56, 64,  80,  96,  112, 128, 144, 160, 176, 192, 224, 256},//V2,L1
    {0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160},//V2,L2
    {0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160},//V2,L3

    {0, 32, 48, 56, 64,  80,  96,  112, 128, 144, 160, 176, 192, 224, 256},//V2,L1
    {0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160},//V2,L2
    {0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160},//V2,L3
};
const static uint16_t MP3_Counts_Table[3][3] =
{
    {384, 1152, 1152},//MPEG1 (L1, L2, L3)
    {384, 1152, 576 },//MPEG2 (L1, L2, L3)
    {384, 1152, 576,},//MPEG2.5 (L1, L2, L3)
};
const static uint16_t MP3_Samplingrate_Table[3][3] =
{
    {44100, 48000, 32000},//MPEG1
    {22050, 24000, 16000},//MPEG2
    {11025, 12000, 8000},//MPEG2.5
};

/*============================================================================*
  *                                  Functions
  *============================================================================*/
uint32_t mp3_decode_id3v2_header(uint8_t *id3v2Header, uint16_t id3v2HeaderLen)
{
    uint32_t header_len = 0;
    ID3V2H *tagHeader = (ID3V2H *)id3v2Header;

    if ((id3v2HeaderLen == sizeof(ID3V2H)) &&
        ((memcmp(tagHeader->header, "ID3", 3) == 0) &&
         ((tagHeader->version & 0xFF) != 0xFF) &&
         ((tagHeader->revision & 0xFF) != 0xFF) &&
         /* ((tagHeader->flag & 0xFF) != 0x80) && */
         ((tagHeader->size[0] & 0xFF) != 0x80) &&
         ((tagHeader->size[1] & 0xFF) != 0x80) &&
         ((tagHeader->size[2] & 0xFF) != 0x80) &&
         ((tagHeader->size[3] & 0xFF) != 0x80)))
    {
        header_len = (tagHeader->size[0] << 21) | (tagHeader->size[1] << 14) |
                     (tagHeader->size[2] << 7) | (tagHeader->size[3]);

        mp3Id3v2Info.startOffset = (sizeof(ID3V2H));
        mp3Id3v2Info.length = header_len;
    }
    return header_len;
}

uint32_t mp3_decode_id3v1_header(uint8_t *id3v1Header, uint16_t id3v1HeaderLen)
{
    uint32_t header_len = 0;
    char temp[30] = {0};
    ID3V1H *header = (ID3V1H *)id3v1Header;

    if (id3v1HeaderLen == sizeof(ID3V1H) && ((memcmp(header->tag, "TAG", 3) == 0) &&
                                             (memcmp(header->title, temp, 30) != 0)))
    {
        header_len = sizeof(ID3V1H);
    }
    return header_len;
}

uint32_t mp3_decode_xing_header(uint8_t *xingHeader, uint16_t xingHeaderLen)
{
    uint16_t length = 0;
    uint32_t xing_flag = 0;
    XING_HEADER_INFO *tHeader = (XING_HEADER_INFO *)xingHeader;

    if ((xingHeaderLen == sizeof(XING_HEADER_INFO)) && ((memcmp(tHeader->header, "Xing", 4) == 0) ||
                                                        (memcmp(tHeader->header, "Info", 4) == 0)))
    {
        g_frameInfo.format_info.mp3.total_frames = 0;
        g_frameInfo.format_info.mp3.total_file_size = 0;
        length += sizeof(tHeader->header);
        xing_flag = (tHeader->xingFlag[0] << 24) + (tHeader->xingFlag[1] << 16) +
                    (tHeader->xingFlag[2] << 8) + tHeader->xingFlag[3];
        length += sizeof(tHeader->xingFlag);
        if ((xing_flag & 0x0001) == 0x0001) /* exist total_frames */
        {
            g_frameInfo.format_info.mp3.total_frames = (tHeader->totalFrameLen[0] << 24) +
                                                       (tHeader->totalFrameLen[1] << 16) +
                                                       (tHeader->totalFrameLen[2] << 8) + tHeader->totalFrameLen[3];
            length += sizeof(tHeader->totalFrameLen);
#if AUDIO_FS_DECODE_DEBUG
            APP_PRINT_INFO1("mp3_decode_xing_header: totalFrameLen(%b)",
                            TRACE_BINARY(sizeof(tHeader->totalFrameLen), tHeader->totalFrameLen));
#endif
        }
        if ((xing_flag & 0x0002) == 0x0002) /* exist total_file_size */
        {
            g_frameInfo.format_info.mp3.total_file_size = (tHeader->fileSize[0] << 24) +
                                                          (tHeader->fileSize[1] << 16) +
                                                          (tHeader->fileSize[2] << 8) + tHeader->fileSize[3];
            length += sizeof(tHeader->fileSize);
#if AUDIO_FS_DECODE_DEBUG
            APP_PRINT_INFO1("mp3_decode_xing_header: fileSize(%b)",
                            TRACE_BINARY(sizeof(tHeader->fileSize), tHeader->fileSize));
#endif
        }
        if ((xing_flag & 0x0004) == 0x0004) /* exist TOC */
        {
            length += 100;/* TOC length is 100 bytes*/
        }
        if ((xing_flag & 0x0008) == 0x0008) /* exist audio_quality */
        {
            length += 4;/* audio_quality length is 4 bytes*/
        }
#if AUDIO_FS_DECODE_DEBUG
        APP_PRINT_INFO3("mp3_decode_xing_header: total_frames:0x%x, file_size:0x%x, length:0x%x",
                        g_frameInfo.format_info.mp3.total_frames, g_frameInfo.format_info.mp3.total_file_size, length);
#endif
    }
    return length;
}

static bool mp3_decode_mp3_header_check(MP3_FHEADER *p_mp3fHeader)
{
    bool ret = false;

    // MPEG-1, Layer III
    if (((p_mp3fHeader->sync1 & 0xFF) == 0xFF) &&
        ((p_mp3fHeader->sync2 & 0xFF) == 0x07) &&
        ((p_mp3fHeader->layer & 0xFF) == 0x1) &&
        ((p_mp3fHeader->version & 0xFF) != 0x1) &&
        ((p_mp3fHeader->bit_rate_index & 0xFF) != 0xF) &&
        ((p_mp3fHeader->sample_rate_index & 0xFF) != 0x3))
    {
        ret = true;
    }

    return ret;
}

uint16_t mp3_decode_mp3_frame(uint8_t *mp3Header, uint16_t mp3HeaderLen, uint32_t *frmsize,
                              bool isFirstFrame)
{
    uint16_t ret = AUDIO_FS_ERR_ERROR;
    uint16_t mp3_counts = 0;
    uint32_t mp3_sampling_freq = 0;
    uint32_t mp3_bitrate = 0;
    uint8_t channel = 0;
    uint8_t sample_depth = 16;
    MP3_FHEADER *mp3fHeader = (MP3_FHEADER *)mp3Header;

    if ((mp3HeaderLen == sizeof(MP3_FHEADER)) && mp3_decode_mp3_header_check(mp3fHeader))
    {
        ret = AUDIO_FS_OK;
        mp3_bitrate = MP3_Bitrate_Table[mpeg_index[mp3fHeader->version] * 3 +
                                        layer_index[mp3fHeader->layer]][mp3fHeader->bit_rate_index];
        if (mp3_bitrate == 0)
        {
            ret = AUDIO_FS_ERR_MP3_BITRATE_ZERO;
        }
        else
        {
            mp3_counts = MP3_Counts_Table[mpeg_index[mp3fHeader->version]][layer_index[mp3fHeader->layer]];
            mp3_sampling_freq =
                MP3_Samplingrate_Table[mpeg_index[mp3fHeader->version]][mp3fHeader->sample_rate_index];
            if (mp3fHeader->channel_mode == CHANNEL_SINGLE)
            {
                channel = 1;
            }
            else
            {
                channel = 2;
            }
        }

        if ((ret == AUDIO_FS_OK) && !isFirstFrame)
        {
            if ((g_frameInfo.format_info.mp3.sample_counts != mp3_counts) ||
                (g_frameInfo.format_info.mp3.sampling_frequency != mp3_sampling_freq) ||
                (g_frameInfo.format_info.mp3.channel_mode != mp3fHeader->channel_mode))
            {
                APP_PRINT_INFO3("mp3_decode_mp3_frame before: bitrate: %d, sampling_freq: %d, sample_counts: %d",
                                g_frameInfo.format_info.mp3.bit_rate, g_frameInfo.format_info.mp3.sampling_frequency,
                                g_frameInfo.format_info.mp3.sample_counts);
                APP_PRINT_INFO3("mp3_decode_mp3_frame current: bitrate: %d, sampling_freq: %d, sample_counts: %d",
                                mp3_bitrate, mp3_sampling_freq, mp3_counts);
                ret = AUDIO_FS_ERR_MP3_HEADER_SYNC;
            }
        }

        if (ret == AUDIO_FS_OK)
        {
            g_frameInfo.format_info.mp3.version = mp3fHeader->version;
            g_frameInfo.format_info.mp3.layer = mp3fHeader->layer;
            g_frameInfo.format_info.mp3.frame_duration = (mp3_counts * 1000) / mp3_sampling_freq;
            g_frameInfo.format_info.mp3.sample_counts = mp3_counts;
            g_frameInfo.format_info.mp3.sampling_frequency = mp3_sampling_freq;
            g_frameInfo.format_info.mp3.bit_rate = mp3_bitrate;
            g_frameInfo.format_info.mp3.channel_mode = mp3fHeader->channel_mode;
            g_frameInfo.format_info.mp3.frame_size_after_decode = (mp3_counts * channel * sample_depth) / 8;

            float aver_frames = (float)mp3_sampling_freq / (float)mp3_counts;
            float frame_bytes = (float)mp3_bitrate * 1000 / 8 / aver_frames;
            *frmsize = (uint16_t)frame_bytes + mp3fHeader->padding;
        }
    }

    APP_PRINT_INFO2("mp3_decode_mp3_frame: ret: 0x%x, frameHeader(%b)", ret, TRACE_BINARY(mp3HeaderLen,
                    mp3Header));

    return ret;
}

uint32_t mp3_decode_judge_frame_header(uint8_t *frameHeader, uint16_t frameHeadeLen)
{
    MP3_FHEADER *mp3fHeader = (MP3_FHEADER *)frameHeader;
#if AUDIO_FS_DECODE_DEBUG
    if (((mp3fHeader->sync1 & 0xFF) == 0xff) && ((mp3fHeader->sync2 & 0xFF) == 0x07))
    {
        APP_PRINT_INFO1("mp3_decode_judge_frame_header: frameHeader(%b)",
                        TRACE_BINARY(frameHeadeLen, frameHeader));
        APP_PRINT_INFO6("mp3_decode_judge_frame_header: sync1:0x%x, sync2:0x%x, bit_rate_index:0x%x, sample_rate_index:0x%x, layer:0x%x, version:0x%x",
                        mp3fHeader->sync1, mp3fHeader->sync2, mp3fHeader->bit_rate_index, mp3fHeader->sample_rate_index,
                        mp3fHeader->layer, mp3fHeader->version);
    }
#endif
    if (mp3_decode_mp3_header_check(mp3fHeader))
    {
        if (frameHeadeLen == sizeof(MP3_FHEADER))
        {
            return sizeof(MP3_FHEADER);
        }
        else if (frameHeadeLen > sizeof(MP3_FHEADER))
        {
            uint16_t xing_offset = 0;
            xing_offset += sizeof(MP3_FHEADER);
//            if (mp3fHeader->error_protection == 0)
//            {
//                xing_offset += 2;
//            }
            if (mp3fHeader->channel_mode == CHANNEL_SINGLE)
            {
                if (mp3fHeader->version == MPEG_1)
                {
                    xing_offset += 17;
                }
                else
                {
                    xing_offset += 9;
                }
            }
            else
            {
                if (mp3fHeader->version == MPEG_1)
                {
                    xing_offset += 32;
                }
                else
                {
                    xing_offset += 17;
                }
            }
            for (int offset = xing_offset; offset < frameHeadeLen && (offset + 4 < frameHeadeLen); offset++)
            {
                uint8_t header[4] = "";
                memcpy(header, frameHeader + offset, 4);
                if ((memcmp(header, "Xing", 4) == 0) || (memcmp(header, "Info", 4) == 0))
                {
                    return offset;
                }
            }
            return sizeof(MP3_FHEADER);
        }
    }
    return 0;
}
#endif /* (MP3_FORMAT_SUPPORT == 1) */
#endif
