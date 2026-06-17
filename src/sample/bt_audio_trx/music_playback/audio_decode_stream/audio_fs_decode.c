/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
/*============================================================================*
  *                                  Header Files
  *============================================================================*/
#include "os_mem.h"
#include "audio_fs_decode.h"
#include "playback_audio_file.h"

//-----------------------------------------------//
//#define BUF_POOL_DEEP       1024 * 5
//uint8_t *g_p_buf_pool = NULL;
//uint32_t g_buf_offset = 0;
//uint32_t g_buf_used = 0;
//-----------------------------------------------//

FRAME_CONTENT   g_frameContent;
FRAME_INFO      g_frameInfo;

ID3V2_TAG_INFO tagInfo[ALL_TAGS];

#if AAC_FORMAT_SUPPORT
extern ID3V2_INFO  aacId3v2Info;
#endif
#if MP3_FORMAT_SUPPORT
#define     LARGEST_TAG_INFO_LEN    (100)
extern      ID3V2_INFO  mp3Id3v2Info;
bool        hasFindMp3Header = 0;
#endif
#if MP4_FORMAT_SUPPORT
MP4_HEADER_INFO g_mp4HeaderInfo;
#endif



extern uint32_t aac_decode_id3v2_header(uint8_t *id3v2Header, uint16_t id3v2HeaderLen);
extern uint32_t aac_decode_id3v1_header(uint8_t *id3v1Header, uint16_t id3v1HeaderLen);
extern uint32_t aac_decode_aac_frame(uint8_t *aacHeader, uint16_t aacHeaderLen);
extern uint32_t aac_decode_judge_frame_header(uint8_t *frameHeader, uint16_t frameHeadeLen);
extern uint32_t mp3_decode_id3v2_header(uint8_t *id3v2Header, uint16_t id3v2HeaderLen);
extern uint32_t mp3_decode_id3v1_header(uint8_t *id3v1Header, uint16_t id3v1HeaderLen);
extern uint32_t mp3_decode_xing_header(uint8_t *xingHeader, uint16_t xingHeaderLen);
extern uint16_t mp3_decode_mp3_frame(uint8_t *mp3Header, uint16_t mp3HeaderLen, uint32_t *frmsize,
                                     bool isFirstFrame);
extern uint32_t mp3_decode_judge_frame_header(uint8_t *frameHeader, uint16_t frameHeadeLen);

void audio_fs_decode_deinit(void)
{
    for (int i = 0; i < ALL_TAGS; i++)
    {
        if (tagInfo[i].info != NULL)
        {
            free(tagInfo[i].info);
            tagInfo[i].info = NULL;
        }
    }
    if (g_frameContent.content != NULL)
    {
        free(g_frameContent.content);
        g_frameContent.content = NULL;
    }
}

uint16_t audio_fs_decode_init(void)
{
//    if (g_p_buf_pool != NULL)
//    {
//        free(g_p_buf_pool);
//    }
//    g_p_buf_pool = os_mem_alloc(RAM_TYPE_DSPSHARE, BUF_POOL_DEEP);

    memset(&g_frameContent, 0, sizeof(FRAME_CONTENT));
    memset(&g_frameInfo, 0, sizeof(FRAME_INFO));

    g_frameInfo.format = MP3;

#if MP3_FORMAT_SUPPORT
    memset(&mp3Id3v2Info, 0, sizeof(ID3V2_INFO));
#endif
#if AAC_FORMAT_SUPPORT
    memset(&aacId3v2Info, 0, sizeof(ID3V2_INFO));
#endif
#if MP4_FORMAT_SUPPORT
    memset(&g_mp4HeaderInfo, 0, sizeof(MP4_HEADER_INFO));
#endif

    return 0;
}

//void audio_fs_decode_set_frame_format(TCHAR *fname, uint16_t nameLen)
void audio_fs_decode_set_frame_format(char *fname, uint16_t nameLen)
{
    memset(&g_frameInfo, 0, sizeof(FRAME_INFO));
    g_frameInfo.format = MP3; //AAC;
#if 0
    for (int i = 0; (i + 3) < nameLen; i++)
    {
        if (fname[i] == _T('.') &&
            ((fname[i + 1] == _T('r')) || (fname[i + 1] == _T('R'))) &&
            ((fname[i + 2] == _T('t')) || (fname[i + 2] == _T('T'))) &&
            ((fname[i + 3] == _T('k')) || (fname[i + 3] == _T('K'))))
        {
            g_frameInfo.format = RTK;
            break;
        }
        else if (fname[i] == _T('.') &&
                 ((fname[i + 1] == _T('a')) || (fname[i + 1] == _T('A'))) &&
                 ((fname[i + 2] == _T('a')) || (fname[i + 2] == _T('A'))) &&
                 ((fname[i + 3] == _T('c')) || (fname[i + 3] == _T('C'))))
        {
            g_frameInfo.format = AAC;
            break;
        }
        else if (fname[i] == _T('.') &&
                 ((fname[i + 1] == _T('m')) || (fname[i + 1] == _T('M'))) &&
                 ((fname[i + 2] == _T('p')) || (fname[i + 2] == _T('P'))) &&
                 (fname[i + 3] == _T('3')))
        {
            g_frameInfo.format = MP3;
            break;
        }
        else if (fname[i] == _T('.') &&
                 ((fname[i + 1] == _T('m')) || (fname[i + 1] == _T('M'))) &&
                 ((fname[i + 2] == _T('p')) || (fname[i + 2] == _T('P'))) &&
                 (fname[i + 3] == _T('4')))
        {
            g_frameInfo.format = MP4;
            break;
        }
        else if (fname[i] == _T('.') &&
                 ((fname[i + 1] == _T('f')) || (fname[i + 1] == _T('F'))) &&
                 ((fname[i + 2] == _T('l')) || (fname[i + 2] == _T('L'))) &&
                 ((fname[i + 3] == _T('a')) || (fname[i + 3] == _T('A'))) &&
                 ((fname[i + 3] == _T('c')) || (fname[i + 3] == _T('C'))))
        {
            g_frameInfo.format = FLAC;
            break;
        }
        else
        {
            g_frameInfo.format = UNKNOWN;
        }
    }
#endif
}


#if MP3_FORMAT_SUPPORT
/*
return tagInfo size
*/
static uint32_t audio_fs_decode_judge_tagInfoSize(uint16_t tagIdx, uint16_t tagInfoSize)
{
    if (tagInfoSize == 0)
    {
        APP_PRINT_ERROR1("audio_fs_decode_judge_tagInfoSize: tagInfoSize(0x%x)", tagInfoSize);
        return 0;
    }
    else if (tagInfoSize > LARGEST_TAG_INFO_LEN)
    {
        tagInfo[tagIdx].length = LARGEST_TAG_INFO_LEN;
    }
    else
    {
        tagInfo[tagIdx].length = tagInfoSize;
    }

    if (tagInfo[tagIdx].info != NULL)
    {
        free(tagInfo[tagIdx].info);
        tagInfo[tagIdx].info = NULL;
    }
    tagInfo[tagIdx].info = os_mem_alloc(RAM_TYPE_DSPSHARE, tagInfo[tagIdx].length);
    if (tagInfo[tagIdx].info == NULL)
    {
        APP_PRINT_ERROR0("audio_fs_decode_judge_tagInfoSize: os_mem_alloc failed");
        return 0;
    }
    return tagInfo[tagIdx].length;
}

uint32_t audio_fs_decode_id3v2_info(void *handle, uint16_t tagIdx)
{
    uint32_t read_len = 0;
    uint32_t tmpLen = 0;
    ID3V2FH frameHeader;
    uint16_t tagInfoSize = 0;
    uint32_t offset = 0;
    uint16_t tagInfoSizeMalloc = 0;

#if AUDIO_FS_DECODE_DEBUG
    APP_PRINT_INFO2("audio_fs_decode_id3v2_info: startOffset:0x%x, length:0x%x",
                    mp3Id3v2Info.startOffset, mp3Id3v2Info.length);
#endif
    if (mp3Id3v2Info.startOffset == 0)
    {
        APP_PRINT_ERROR0("No id3v2 header found!");
        return 0;
    }
    offset = mp3Id3v2Info.startOffset;
    while ((tmpLen + sizeof(ID3V2H)) < mp3Id3v2Info.length)
    {
        tagInfoSize = 0;
        tagInfoSizeMalloc = 0;
//        f_lseek(&audio_file->fil, offset);
#if AUDIO_FS_DECODE_DEBUG
        APP_PRINT_INFO2("audio_fs_decode_id3v2_info: tagIdx:0x%x, offset:0x%x", tagIdx, offset);
#endif
        stream_peek_data((uint8_t *)&frameHeader, sizeof(ID3V2H), &read_len);
//        if (f_read(&audio_file->fil, &frameHeader, sizeof(ID3V2H), &read_len) == FR_OK)
        {
            tagInfoSize = ((frameHeader.size)[0] << 24)
                          | ((frameHeader.size)[1] << 16)
                          | ((frameHeader.size)[2] << 8)
                          | (frameHeader.size)[3];
            tmpLen += sizeof(ID3V2FH) + tagInfoSize;
            switch (tagIdx)
            {
            case TIT2:
                if (strncmp(frameHeader.frameID, "TIT2", 4) == 0)
                {
#if AUDIO_FS_DECODE_DEBUG
                    APP_PRINT_INFO1("audio_fs_decode_id3v2_info: TIT2 found - size(%b)", TRACE_BINARY(4,
                                    frameHeader.size));
#endif
                    tagInfoSizeMalloc = audio_fs_decode_judge_tagInfoSize(tagIdx, tagInfoSize);
                    if (tagInfoSizeMalloc != 0)
                    {
//                        if (f_read(&audio_file->fil, tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len) != FR_OK)
//                        {
//                            APP_PRINT_ERROR0("audio_fs_decode_id3v2_info: read file error");
//                            return AUDIO_FS_ERR_READ;
//                        }
                        stream_peek_data(tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len);
                    }
//                    offset = audio_fs_get_file_offset(handle);
//                    audio_fs_set_file_offset(handle, offset);
                    return tagInfoSizeMalloc;
                }
                break;
            case TALB:
                if (strncmp(frameHeader.frameID, "TALB", 4) == 0)
                {
#if AUDIO_FS_DECODE_DEBUG
                    APP_PRINT_INFO1("audio_fs_decode_id3v2_info: TALB found - size(%b)", TRACE_BINARY(4,
                                    frameHeader.size));
#endif
                    tagInfoSizeMalloc = audio_fs_decode_judge_tagInfoSize(tagIdx, tagInfoSize);
                    if (tagInfoSizeMalloc != 0)
                    {
//                        if (f_read(&audio_file->fil, tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len) != FR_OK)
//                        {
//                            APP_PRINT_ERROR0("audio_fs_decode_id3v2_info: read file error");
//                            return AUDIO_FS_ERR_READ;
//                        }
                        stream_peek_data(tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len);
                    }
//                    offset = audio_fs_get_file_offset(handle);
//                    audio_fs_set_file_offset(handle, offset);
                    return tagInfoSizeMalloc;
                }
                break;
            case TCOM:  /* Composer */
                if (strncmp(frameHeader.frameID, "TCOM", 4) == 0)
                {
#if AUDIO_FS_DECODE_DEBUG
                    APP_PRINT_INFO1("audio_fs_decode_id3v2_info: TCOM found - size(%b)", TRACE_BINARY(4,
                                    frameHeader.size));
#endif
                    tagInfoSizeMalloc = audio_fs_decode_judge_tagInfoSize(tagIdx, tagInfoSize);
                    if (tagInfoSizeMalloc != 0)
                    {
//                        if (f_read(&audio_file->fil, tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len) != FR_OK)
//                        {
//                            APP_PRINT_ERROR0("audio_fs_decode_id3v2_info: read file error");
//                            return AUDIO_FS_ERR_READ;
//                        }
                        stream_peek_data(tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len);
                    }
//                    offset = audio_fs_get_file_offset(handle);
//                    audio_fs_set_file_offset(handle, offset);
                    return tagInfoSizeMalloc;
                }
                break;
            case TCON:  /* content type */
                if (strncmp(frameHeader.frameID, "TCON", 4) == 0)
                {
#if AUDIO_FS_DECODE_DEBUG
                    APP_PRINT_INFO1("audio_fs_decode_id3v2_info: TCON found - size(%b)", TRACE_BINARY(4,
                                    frameHeader.size));
#endif
                    tagInfoSizeMalloc = audio_fs_decode_judge_tagInfoSize(tagIdx, tagInfoSize);
                    if (tagInfoSizeMalloc != 0)
                    {
//                        if (f_read(&audio_file->fil, tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len) != FR_OK)
//                        {
//                            APP_PRINT_ERROR0("audio_fs_decode_id3v2_info: read file error");
//                            return AUDIO_FS_ERR_READ;
//                        }
                        stream_peek_data(tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len);
                    }
//                    offset = audio_fs_get_file_offset(handle);
//                    audio_fs_set_file_offset(handle, offset);
                    return tagInfoSizeMalloc;
                }
                break;
            case TPE1:
                if (strncmp(frameHeader.frameID, "TPE1", 4) == 0)
                {
#if AUDIO_FS_DECODE_DEBUG
                    APP_PRINT_INFO1("audio_fs_decode_id3v2_info: TPE1 found - size(%b)", TRACE_BINARY(4,
                                    frameHeader.size));
#endif
                    tagInfoSizeMalloc = audio_fs_decode_judge_tagInfoSize(tagIdx, tagInfoSize);
                    if (tagInfoSizeMalloc != 0)
                    {
//                        if (f_read(&audio_file->fil, tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len) != FR_OK)
//                        {
//                            APP_PRINT_ERROR0("audio_fs_decode_id3v2_info: read file error");
//                            return AUDIO_FS_ERR_READ;
//                        }
                        stream_peek_data(tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len);
                    }
//                    offset = audio_fs_get_file_offset(handle);
//                    audio_fs_set_file_offset(handle, offset);
                    return tagInfoSizeMalloc;
                }
                break;
            case TPE2:
                if (strncmp(frameHeader.frameID, "TPE2", 4) == 0)
                {
#if AUDIO_FS_DECODE_DEBUG
                    APP_PRINT_INFO1("audio_fs_decode_id3v2_info: TPE2 found - size(%b)", TRACE_BINARY(4,
                                    frameHeader.size));
#endif
                    tagInfoSizeMalloc = audio_fs_decode_judge_tagInfoSize(tagIdx, tagInfoSize);
                    if (tagInfoSizeMalloc != 0)
                    {
//                        if (f_read(&audio_file->fil, tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len) != FR_OK)
//                        {
//                            APP_PRINT_ERROR0("audio_fs_decode_id3v2_info: read file error");
//                            return AUDIO_FS_ERR_READ;
//                        }
                        stream_peek_data(tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len);
                    }
//                    offset = audio_fs_get_file_offset(handle);
//                    audio_fs_set_file_offset(handle, offset);
                    return tagInfoSizeMalloc;
                }
                break;
            case TXXX:
                if (strncmp(frameHeader.frameID, "TXXX", 4) == 0)
                {
#if AUDIO_FS_DECODE_DEBUG
                    APP_PRINT_INFO1("audio_fs_decode_id3v2_info: TXXX found - size(%b)", TRACE_BINARY(4,
                                    frameHeader.size));
#endif
                    tagInfoSizeMalloc = audio_fs_decode_judge_tagInfoSize(tagIdx, tagInfoSize);
                    if (tagInfoSizeMalloc != 0)
                    {
//                        if (f_read(&audio_file->fil, tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len) != FR_OK)
//                        {
//                            APP_PRINT_ERROR0("audio_fs_decode_id3v2_info: read file error");
//                            return AUDIO_FS_ERR_READ;
//                        }
                        stream_peek_data(tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len);
                    }
//                    offset = audio_fs_get_file_offset(handle);
//                    audio_fs_set_file_offset(handle, offset);
                    return tagInfoSizeMalloc;
                }
                break;
            case TOFN:
                if (strncmp(frameHeader.frameID, "TOFN", 4) == 0)
                {
#if AUDIO_FS_DECODE_DEBUG
                    APP_PRINT_INFO1("audio_fs_decode_id3v2_info: TOFN found - size(%b)", TRACE_BINARY(4,
                                    frameHeader.size));
#endif
                    tagInfoSizeMalloc = audio_fs_decode_judge_tagInfoSize(tagIdx, tagInfoSize);
                    if (tagInfoSizeMalloc != 0)
                    {
//                        if (f_read(&audio_file->fil, tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len) != FR_OK)
//                        {
//                            APP_PRINT_ERROR0("audio_fs_decode_id3v2_info: read file error");
//                            return AUDIO_FS_ERR_READ;
//                        }
                        stream_peek_data(tagInfo[tagIdx].info, tagInfoSizeMalloc, &read_len);
                    }
//                    offset = audio_fs_get_file_offset(handle);
//                    audio_fs_set_file_offset(handle, offset);
                    return tagInfoSizeMalloc;
                }
                break;
            // add more case
            default:
                offset += (sizeof(ID3V2H) + tagInfoSize);
                break;
            }
            offset += (sizeof(ID3V2H) + tagInfoSize);
        }
//        else
//        {
//            APP_PRINT_ERROR0("audio_fs_decode_id3v2_info: read file error");
//            return AUDIO_FS_ERR_READ;
//        }
    }
//    offset = audio_fs_get_file_offset(handle);
//    audio_fs_set_file_offset(handle, offset);
    return 0;
}

uint16_t audio_fs_decode_get_id3v2_InfoLen(uint16_t tagIdx)
{
    return tagInfo[tagIdx].length;
}

uint8_t *audio_fs_decode_get_id3v2_Info(uint16_t tagIdx)
{
    return tagInfo[tagIdx].info;
}

void audio_fs_decode_get_mp3_playtime(void)
{
    uint32_t playTime = 0;
    if (audio_fs_decode_judge_tagInfoSize(TIME, 4))
    {
        if (g_frameInfo.format_info.mp3.mp3_sub_format == MP3_CBR)
        {
            playTime = g_frameInfo.format_info.mp3.total_file_size * 8 / (g_frameInfo.format_info.mp3.bit_rate *
                                                                          1000);
        }
        else if (g_frameInfo.format_info.mp3.mp3_sub_format == MP3_VBR)
        {
            if (g_frameInfo.format_info.mp3.total_frames)
            {
                /* if can get total_frames from xing header */
                playTime = g_frameInfo.format_info.mp3.total_frames * g_frameInfo.format_info.mp3.sample_counts /
                           g_frameInfo.format_info.mp3.sampling_frequency;
            }
            else
            {
                /* or get playTime same as CBR */
                playTime = g_frameInfo.format_info.mp3.total_file_size * 8 / (g_frameInfo.format_info.mp3.bit_rate *
                                                                              1000);
            }
        }
        memcpy(tagInfo[TIME].info, &playTime, 4);
    }
#if AUDIO_FS_DECODE_DEBUG
    APP_PRINT_INFO5("audio_fs_decode_get_mp3_playtime: total_file_size:0x%x, bitrate:%d, total_frames:%d, sample_counts:%d, sampling_freq:%d",
                    g_frameInfo.format_info.mp3.total_file_size,
                    g_frameInfo.format_info.mp3.bit_rate,
                    g_frameInfo.format_info.mp3.total_frames,
                    g_frameInfo.format_info.mp3.sample_counts,
                    g_frameInfo.format_info.mp3.sampling_frequency);
#endif
    APP_PRINT_INFO2("audio_fs_decode_get_mp3_playtime: playTime:%d, mp3_sub_format:%d",
                    playTime, g_frameInfo.format_info.mp3.mp3_sub_format);
}

static bool audio_fs_decode_find_mp3_header(uint8_t *buf, uint32_t *p_length)
{
    bool ret = false;
    uint16_t check_res = AUDIO_FS_ERR_ERROR;

    check_res = mp3_decode_mp3_frame(buf, sizeof(MP3_FHEADER), p_length, !hasFindMp3Header);
    if (check_res == AUDIO_FS_OK)
    {
        g_frameInfo.format_info.mp3.mp3_sub_format = MP3_CBR;
        // g_frameInfo.format_info.mp3.total_file_size = f_size(&audio_file->fil) - offset;
        g_frameInfo.format_info.mp3.total_file_size = 0xFFFFFF;

        ret = true;
    }

    return ret;
}

static uint16_t audio_fs_decode_judge_mp3_header(void *handle, uint32_t *pOffset)
{
    uint32_t offset = 0;
    uint32_t length = 0;
    uint32_t headerSize = 0;
    uint32_t read_len = 0;
    uint16_t res = 0;
    uint32_t scan_head_size = 0;
    uint8_t header[sizeof(ID3V1H)] = "";
    uint16_t i = 1;
    offset = (*pOffset);

    while (1)
    {
        stream_peek_data_2(header, *pOffset, sizeof(header), &read_len);
//        if ((res = f_read(&audio_file->fil, header, sizeof(header), &read_len)) == FR_OK)
        {
            if (read_len < sizeof(header))
            {
                return FS_END_OF_FILE;
            }

            if (hasFindMp3Header == 0)
            {
                headerSize = sizeof(ID3V2H);
                if ((length = mp3_decode_id3v2_header(header, headerSize)) != 0)
                {
                    offset += (headerSize + length);
                    *pOffset = offset; /* jump to the end of id3v2 header */
//                    g_buf_offset = offset;
//                    stream_remove_data();
                    return ID3V2_HEADER;
                }

                headerSize = sizeof(header);
                length = mp3_decode_judge_frame_header(header, headerSize);
                if (length == 0)
                {
                    for (i = 1; i <= (sizeof(header) - 1); i++)
                    {
                        length = mp3_decode_id3v2_header(header + i, sizeof(ID3V2H));
                        if (length != 0)
                        {
                            offset += (i + sizeof(ID3V2H) + length);
                            *pOffset = offset; /* jump to the end of id3v2 header */
                            return ID3V2_HEADER;
                        }

                        length = mp3_decode_judge_frame_header(header + i, headerSize);
                        if (length == sizeof(MP3_FHEADER))
                        {
                            length = 0;
                            if (audio_fs_decode_find_mp3_header(header + i, &length))
                            {
                                uint8_t mp3Header[sizeof(MP3_FHEADER)] = "";

                                stream_peek_data_2(mp3Header, offset + i + length, sizeof(mp3Header), &read_len);
                                if (0 != mp3_decode_judge_frame_header(mp3Header, sizeof(mp3Header)))
                                {
                                    hasFindMp3Header = true;
                                    offset += i;
                                    *pOffset = offset; /* jump to the start of mp3 frame header, start to decode mp3 frame */
                                    return MP3_HEADER;
                                }
                                else
                                {
                                    length = 0;
                                }
                            }
                        }
                        else if ((length != 0) && (length != sizeof(MP3_FHEADER)))
                        {
                            /* jump to the start of xing header, to decode frames and fileSize */
                            offset += length;
                            uint8_t xingHeader[16] = "";
                            memcpy(xingHeader, header + length, sizeof(xingHeader));
                            length = mp3_decode_xing_header(xingHeader, sizeof(xingHeader));
                            if (length != 0)/* find Xing header */
                            {
                                *pOffset = offset + length; /* jump to the end of xing header, start to decode mp3 frame */
                                g_frameInfo.format_info.mp3.mp3_sub_format = MP3_VBR;
                                if (g_frameInfo.format_info.mp3.total_file_size > offset)
                                {
                                    /* if can get total_file_size from xing header */
                                    g_frameInfo.format_info.mp3.total_file_size -= offset;
                                }
                                else
                                {
                                    /* or get total_file_size from f_size */
                                    //                            g_frameInfo.format_info.mp3.total_file_size = f_size(&audio_file->fil) - offset;
                                    g_frameInfo.format_info.mp3.total_file_size = 0xFFFFFF;
                                }
                                return XING_HEADER;
                            }
                        }
                    }
                }
                else if (length == sizeof(MP3_FHEADER))
                {
                    length = 0;
                    if (audio_fs_decode_find_mp3_header(header, &length))
                    {
                        uint8_t mp3Header[sizeof(MP3_FHEADER)] = "";

                        stream_peek_data_2(mp3Header, offset + length, sizeof(mp3Header), &read_len);
                        if (0 != mp3_decode_judge_frame_header(mp3Header, sizeof(mp3Header)))
                        {
                            hasFindMp3Header = true;
                            *pOffset = offset; /* jump to the start of mp3 frame header, start to decode mp3 frame */
                            return MP3_HEADER;
                        }
                        else
                        {
                            length = 0;
                        }
                    }
                }
                else if ((length != 0) && (length != sizeof(MP3_FHEADER)))
                {
                    /* jump to the start of xing header, to decode frames and fileSize */
                    offset += length;
                    uint8_t xingHeader[16] = "";
                    memcpy(xingHeader, header + length, sizeof(xingHeader));
                    length = mp3_decode_xing_header(xingHeader, sizeof(xingHeader));
                    if (length != 0)/* find Xing header */
                    {
                        *pOffset = offset + length; /* jump to the end of xing header, start to decode mp3 frame */
                        g_frameInfo.format_info.mp3.mp3_sub_format = MP3_VBR;
                        if (g_frameInfo.format_info.mp3.total_file_size > offset)
                        {
                            /* if can get total_file_size from xing header */
                            g_frameInfo.format_info.mp3.total_file_size -= offset;
                        }
                        else
                        {
                            /* or get total_file_size from f_size */
//                            g_frameInfo.format_info.mp3.total_file_size = f_size(&audio_file->fil) - offset;
                            g_frameInfo.format_info.mp3.total_file_size = 0xFFFFFF;
                        }
                        return XING_HEADER;
                    }
                }
            }

            if (hasFindMp3Header == 1)
            {
                length = mp3_decode_judge_frame_header(header, sizeof(MP3_FHEADER));
                if (length == 0)
                {
                    for (i = 1; i <= (sizeof(header) - 1); i++)
                    {
                        length = mp3_decode_judge_frame_header(header + i, sizeof(MP3_FHEADER));
                        if (length == sizeof(MP3_FHEADER))
                        {
                            length = 0;
                            if (audio_fs_decode_find_mp3_header(header + i, &length))
                            {
                                uint8_t mp3Header[sizeof(MP3_FHEADER)] = "";

                                stream_peek_data_2(mp3Header, offset + i + length, sizeof(mp3Header), &read_len);
                                if (0 != mp3_decode_judge_frame_header(mp3Header, sizeof(mp3Header)))
                                {
                                    hasFindMp3Header = true;
                                    offset += i;
                                    *pOffset = offset; /* jump to the start of mp3 frame header, start to decode mp3 frame */
                                    return MP3_HEADER;
                                }
                                else
                                {
                                    length = 0;
                                }
                            }
                        }
                    }
                }
                else if (length == sizeof(MP3_FHEADER))
                {
                    length = 0;
                    if (audio_fs_decode_find_mp3_header(header, &length))
                    {
                        uint8_t mp3Header[sizeof(MP3_FHEADER)] = "";

                        stream_peek_data_2(mp3Header, offset + length, sizeof(mp3Header), &read_len);
                        if (0 != mp3_decode_judge_frame_header(mp3Header, sizeof(mp3Header)))
                        {
                            hasFindMp3Header = true;
                            *pOffset = offset; /* jump to the start of mp3 frame header, start to decode mp3 frame */
                            return MP3_HEADER;
                        }
                        else
                        {
                            length = 0;
                        }
                    }
                }
            }

            if (length == 0)
            {
                scan_head_size += i;
                offset += i;
                *pOffset = offset;
//                audio_fs_set_file_offset(handle, offset);
                if (((scan_head_size >= (50 * sizeof(header))) && (length == 0)) ||
                    (offset >= stream_get_data_size()))//f_eof(&audio_file->fil))
                {
                    APP_PRINT_ERROR1("audio_fs_decode_judge_mp3_header: not find any header, offset: 0x%x", offset);
                    return FS_END_OF_FILE;
                }
            }
        }
//        else
//        {
//            APP_PRINT_ERROR1("audio_fs_decode_judge_mp3_header: Read file content Fail, res:%d", res);
//            return AUDIO_FS_ERR_READ;
//        }
    }
}

#endif

// need
#if AAC_FORMAT_SUPPORT
static uint16_t audio_fs_decode_judge_aac_header(void *handle, uint32_t *pOffset)
{
    uint32_t offset = 0;
    uint32_t length = 0;
    uint32_t headerSize = 0;
    uint32_t read_len = 0;
    uint16_t res = 0;
    uint8_t header[sizeof(ID3V1H)] = "";
    T_AUDIO_FILE *audio_file = NULL;
    audio_file = (T_AUDIO_FILE *)handle;
    uint8_t rtkFileType[4] = "";
    HEADER_TYPE headerType = HEADER_TYPE_NONE;

    offset = (*pOffset);
#if AUDIO_FS_DECODE_DEBUG
    APP_PRINT_INFO1("audio_fs_decode_judge_aac_header: offset:0x%x", offset);
#endif
    while (1)
    {
        memcpy(header, g_p_buf_pool + g_buf_offset, sizeof(header));
        read_len += sizeof(header);
//        if ((res = f_read(&audio_file->fil, header, sizeof(header), &read_len)) == FR_OK)
        if (read_len <= BUF_POOL_DEEP)
        {
            headerSize = sizeof(ID3V2H);
            if ((length = aac_decode_id3v2_header(header, headerSize)) != 0)
            {
                offset += length;
                *pOffset = offset; /* jump to the end of id3v2 header */
                if (g_frameInfo.format == RTK)
                {
                    memcpy(rtkFileType, header + sizeof(ID3V2H) + sizeof(ID3V2EH), sizeof(rtkFileType));
                    if ((memcmp(rtkFileType, "LATM", 4) == 0) ||
                        (memcmp(rtkFileType, "latm", 4) == 0))
                    {
                        g_frameInfo.format_info.rtk.rtkTransFormat = RTK_LATM;
                    }
                    else if ((memcmp(rtkFileType, "SBC", 3) == 0) ||
                             (memcmp(rtkFileType, "sbc", 3) == 0))
                    {
                        g_frameInfo.format_info.rtk.rtkTransFormat = RTK_SBC;
                    }
                    else
                    {
                        g_frameInfo.format_info.rtk.rtkTransFormat = RTK_ADTS;
                    }
#if AUDIO_FS_DECODE_DEBUG
                    APP_PRINT_INFO2("audio_fs_decode_judge_aac_header: find ID3V2 header, offset:0x%x, rtkTransFormat:0x%x",
                                    offset, g_frameInfo.format_info.rtk.rtkTransFormat);
#endif
                }
                return ID3V2_HEADER;
            }

            headerSize = sizeof(header);
            if ((length = aac_decode_id3v1_header(header, headerSize)) != 0)
            {
                offset += headerSize;
                *pOffset = offset; /* jump to the end of id3v1 header */
#if AUDIO_FS_DECODE_DEBUG
                APP_PRINT_INFO1("audio_fs_decode_judge_aac_header: find ID3V1 header, offset:0x%x", offset);
#endif
                return ID3V1_HEADER;
            }

            if (g_frameInfo.format == RTK)/* for RTK */
            {
                if (g_frameInfo.format_info.rtk.rtkTransFormat == RTK_LATM)
                {
                    headerSize = LATM_SYNC_WORD_LEN;
                }
                else if (g_frameInfo.format_info.rtk.rtkTransFormat == RTK_SBC)
                {
                    headerSize = SBC_SYNC_WORD_LEN;
                }
                else if (g_frameInfo.format_info.rtk.rtkTransFormat == RTK_ADTS)
                {
                    headerSize = ADTS_SYNC_WORD_LEN;
                }
            }
            else/* for AAC */
            {
                headerSize = ADTS_SYNC_WORD_LEN;
            }
            if ((length = aac_decode_judge_frame_header(header, headerSize)) != 0)
            {
                *pOffset = offset; /* jump to the start of frame header */
                if (length == LATM_SYNC_WORD_LEN)
                {
                    headerType = RTK_LATM_HEADER;
                }
                else if (length == SBC_SYNC_WORD_LEN)
                {
                    headerType = RTK_SBC_HEADER;
                }
                else if ((length == ADTS_SYNC_WORD_LEN) && (g_frameInfo.format == RTK))
                {
                    headerType = RTK_ADTS_HEADER;
                }
                else if ((length == ADTS_SYNC_WORD_LEN) && (g_frameInfo.format == AAC))
                {
                    headerType = AAC_HEADER;
                }
#if AUDIO_FS_DECODE_DEBUG
                APP_PRINT_INFO2("audio_fs_decode_judge_aac_header: find 0x%x header, offset:0x%x",
                                headerType, offset);
#endif
                return headerType;
            }

            offset++;
            audio_fs_set_file_offset(handle, offset);
            if (f_eof(&audio_file->fil))
            {
                APP_PRINT_ERROR0("audio_fs_decode_judge_aac_header: end of file, not find any header");
                return FS_END_OF_FILE;
            }
        }
        else
        {
            APP_PRINT_ERROR1("audio_fs_decode_judge_aac_header: Read file content Fail, res:%d", res);
            return AUDIO_FS_ERR_READ;
        }
    }
}
#endif

uint16_t audio_fs_get_file_header_length(void *handle, uint32_t *p_offset)
{
    uint32_t offset = 0;
    uint16_t headerType = 0;
    g_frameInfo.format = MP3;
    uint32_t id3v2_offset = 0;

    if (g_frameInfo.format == MP3)
    {
        g_frameInfo.format_info.mp3.mp3_sub_format = MP3_CBR;
    }
    else if (g_frameInfo.format == RTK)
    {
        g_frameInfo.format_info.rtk.rtkTransFormat = RTK_ADTS;
    }
    while (1)
    {
        //audio_fs_get_file_offset(handle);
        switch (g_frameInfo.format)
        {
#if AAC_FORMAT_SUPPORT
        case RTK:
        case AAC:
            headerType = audio_fs_decode_judge_aac_header(handle, &offset);
            break;
#endif
#if MP3_FORMAT_SUPPORT
        case MP3:
            hasFindMp3Header = 0;
            headerType = audio_fs_decode_judge_mp3_header(handle, &offset);

            break;
#endif
#if MP4_FORMAT_SUPPORT
        case MP4:
            headerType = audio_fs_decode_judge_mp4_header(handle, &offset);
            break;
#endif
        default:
            break;
        }
//        audio_fs_set_file_offset(handle, offset);
        uint32_t read_len = 0;
        if ((headerType == ID3V2_HEADER) ||
            (headerType == XING_HEADER))
        {
            id3v2_offset = offset;
            *p_offset = offset;
//            stream_remove_data(offset, &read_len);
            return headerType;
        }
        APP_PRINT_INFO2("audio_fs_get_file_header_length: headerType: 0x%x, offset:0x%x", headerType,
                        offset);
        if (headerType == AUDIO_FS_ERR_READ)
        {
            return AUDIO_FS_ERR_READ;
        }
        else if (headerType == FS_END_OF_FILE)
        {
            return FS_END_OF_FILE;
        }
        else if ((headerType == RTK_ADTS_HEADER) ||
                 (headerType == RTK_LATM_HEADER) ||
                 (headerType == RTK_SBC_HEADER) ||
                 (headerType == AAC_HEADER) ||
                 (headerType == MP3_HEADER) ||
                 (headerType == MP4_HEADER))
        {
            if (offset != id3v2_offset)
            {
                stream_remove_data(offset, &read_len);
            }
            *p_offset = offset;
            return 0;
        }
    }
}
// need
uint16_t audio_fs_decode_before_get_frame(void *handle)
{
    uint32_t offset = 0;
    uint16_t headerType = 0;
    g_frameInfo.format = MP3;
    uint32_t id3v2_offset = 0;

    if (g_frameInfo.format == MP3)
    {
        g_frameInfo.format_info.mp3.mp3_sub_format = MP3_CBR;
    }
    else if (g_frameInfo.format == RTK)
    {
        g_frameInfo.format_info.rtk.rtkTransFormat = RTK_ADTS;
    }
    while (1)
    {
        //audio_fs_get_file_offset(handle);
        switch (g_frameInfo.format)
        {
#if AAC_FORMAT_SUPPORT
        case RTK:
        case AAC:
            headerType = audio_fs_decode_judge_aac_header(handle, &offset);
            break;
#endif
#if MP3_FORMAT_SUPPORT
        case MP3:
            hasFindMp3Header = 0;
            headerType = audio_fs_decode_judge_mp3_header(handle, &offset);
            break;
#endif
#if MP4_FORMAT_SUPPORT
        case MP4:
            headerType = audio_fs_decode_judge_mp4_header(handle, &offset);
            break;
#endif
        default:
            break;
        }
//        audio_fs_set_file_offset(handle, offset);
        uint32_t read_len = 0;
        if (headerType == ID3V2_HEADER)
        {
            id3v2_offset = offset;
            stream_remove_data(offset, &read_len);
        }
        APP_PRINT_INFO2("audio_fs_decode_before_get_frame: headerType: 0x%x, offset:0x%x", headerType,
                        offset);
        if (headerType == AUDIO_FS_ERR_READ)
        {
            return AUDIO_FS_ERR_READ;
        }
        else if (headerType == FS_END_OF_FILE)
        {
            return FS_END_OF_FILE;
        }
        else if ((headerType == RTK_ADTS_HEADER) ||
                 (headerType == RTK_LATM_HEADER) ||
                 (headerType == RTK_SBC_HEADER) ||
                 (headerType == AAC_HEADER) ||
                 (headerType == MP3_HEADER) ||
                 (headerType == MP4_HEADER))
        {
            if (offset != id3v2_offset)
            {
                stream_remove_data(offset, &read_len);
            }
            return 0;
        }
    }
}
// need
uint16_t audio_fs_decode_get_frame(void *handle, FRAME_CONTENT *p_frameContent)
{
    uint32_t read_len = 0;
    uint16_t ret = 0;
    uint32_t offset = 0;
    uint32_t frameSize = 0;
#if AAC_FORMAT_SUPPORT
    uint16_t pktHeaderSize = 0;
    uint8_t rtkFrame[16] = {0};
#endif
#if MP3_FORMAT_SUPPORT
    uint8_t mp3Frame[4] = {0};
#endif
#if MP4_FORMAT_SUPPORT
    uint8_t tempFrameSize[4] = "";
    uint32_t frameTableCurOffset = 0;
#endif
#if (AAC_FORMAT_SUPPORT || MP3_FORMAT_SUPPORT || MP4_FORMAT_SUPPORT)
    uint16_t headerType = 0;
#endif
//    RTK_PKT_HEADER pktHeader;

//    offset = g_buf_offset;//audio_fs_get_file_offset(handle);
//    if (g_p_buf_pool == NULL)
//    {
//        return AUDIO_FS_ERR_READ;
//    }
    g_frameInfo.format = MP3;
    switch (g_frameInfo.format)
    {
#if AAC_FORMAT_SUPPORT
    case RTK:
    case AAC:
//        if ((ret = f_read(&audio_file->fil, rtkFrame, sizeof(rtkFrame), &read_len)) == FR_OK)
        memcpy(rtkFrame, g_p_buf_pool + g_buf_offset, sizeof(rtkFrame));
//        read_len += sizeof(rtkFrame);
        {
            if ((g_frameInfo.format == RTK) && (g_frameInfo.format_info.rtk.rtkTransFormat == RTK_LATM))
            {
                pktHeaderSize = LATM_SYNC_WORD_LEN;
            }
            else if ((g_frameInfo.format == RTK) && (g_frameInfo.format_info.rtk.rtkTransFormat == RTK_SBC))
            {
                pktHeaderSize = SBC_SYNC_WORD_LEN;
            }
            else if ((g_frameInfo.format == RTK) && (g_frameInfo.format_info.rtk.rtkTransFormat == RTK_ADTS))
            {
                pktHeaderSize = ADTS_SYNC_WORD_LEN;
            }
            // TODO    another AAC format decode
            else if (g_frameInfo.format == AAC)
            {
                pktHeaderSize = ADTS_SYNC_WORD_LEN;
            }
            /* for sbc and latm, frameSize is equal to pkt length
               for aadts and aac, frameSize is equal to frame length */
            frameSize = aac_decode_aac_frame(rtkFrame, pktHeaderSize);
            if (frameSize == 0)
            {
                APP_PRINT_INFO4("audio_fs_decode_get_frame: offset:0x%x, format:0x%x, pktHeaderSize:0x%x, rtkFrame(%b)",
                                offset, g_frameInfo.format, pktHeaderSize, TRACE_BINARY(16, rtkFrame));
                audio_fs_set_file_offset(handle, offset);
                headerType = audio_fs_decode_judge_aac_header(handle, &offset);
                audio_fs_set_file_offset(handle, offset);
                if ((headerType == RTK_ADTS_HEADER) || (headerType == AAC_HEADER))
                {
                    frameSize = aac_decode_aac_frame(rtkFrame, pktHeaderSize);
                }
                else
                {
                    return headerType;
                }
            }
        }
        break;
#endif
#if MP3_FORMAT_SUPPORT
    case MP3:
        {
            stream_peek_data(mp3Frame, sizeof(mp3Frame), &read_len);
            // if ((ret = f_read(&audio_file->fil, mp3Frame, sizeof(mp3Frame), &read_len)) == FR_OK)
            // {
            if (audio_fs_decode_find_mp3_header(mp3Frame, &frameSize) == false)
            {
                APP_PRINT_INFO5("audio_fs_decode_get_frame: offest:0x%x, mp3Frame[0]:0x%x, mp3Frame[1]: 0x%x, mp3Frame[2]: 0x%x, mp3Frame[3]: 0x%x",
                                offset, mp3Frame[0], mp3Frame[1], mp3Frame[2], mp3Frame[3]);
                // audio_fs_set_file_offset(handle, offset);
                headerType = audio_fs_decode_judge_mp3_header(handle, &offset);
                // audio_fs_set_file_offset(handle, offset);
                stream_remove_data(offset, &read_len);
                if (headerType == MP3_HEADER)
                {
                    stream_peek_data(mp3Frame, sizeof(mp3Frame), &read_len);
                    audio_fs_decode_find_mp3_header(mp3Frame, &frameSize);
                }
                else
                {
                    return headerType;
                }
            }
        }
        break;
#endif
#if MP4_FORMAT_SUPPORT
    case MP4:
        if (g_mp4HeaderInfo.curFrameNum < g_mp4HeaderInfo.totalFrameCnt)
        {
            frameTableCurOffset = g_mp4HeaderInfo.frameTableStartOffset + g_mp4HeaderInfo.curFrameNum * 4 + 4;
            f_lseek(&audio_file->fil, frameTableCurOffset);
            if ((ret = f_read(&audio_file->fil, tempFrameSize, sizeof(tempFrameSize), &read_len)) != FR_OK)
            {
                APP_PRINT_ERROR1("audio_fs_decode_get_frame: read header bin fail, res:0x%x", res);
                ret = AUDIO_FS_ERR_READ;
                return ret;
            }
            frameSize = (((uint32_t)tempFrameSize[0] & 0xFF) << 24) |
                        (((uint32_t)tempFrameSize[1] & 0xFF) << 16) |
                        (((uint32_t)tempFrameSize[2] & 0xFF) << 8) |
                        (((uint32_t)tempFrameSize[3] & 0xFF));
            offset = audio_fs_get_file_offset(handle);
            APP_PRINT_INFO5("audio_fs_decode_get_frame: fileOffset:0x%x-0x%x, frameTableCurOffset:0x%x, curFrameNum:0x%x, frameSize:0x%x",
                            offset, (offset + frameSize), frameTableCurOffset,
                            g_mp4HeaderInfo.curFrameNum, frameSize);
        }
        else
        {
            return FS_END_OF_FILE;
        }
        break;
#endif
    default:
        break;
    }
    if (frameSize == 0)
    {
        APP_PRINT_ERROR2("audio_fs_decode_get_frame: frameSize is zero! g_frameInfo.format: 0x%x, rtkTransFormat:0x%x",
                         g_frameInfo.format, g_frameInfo.format_info.rtk.rtkTransFormat);
        return ret;
    }
//    if ((g_frameInfo.format_info.rtk.rtkTransFormat == RTK_LATM) ||
//        (g_frameInfo.format_info.rtk.rtkTransFormat == RTK_SBC))
//    {
//        audio_fs_set_file_offset(handle, offset);
//        if ((ret = f_read(&audio_file->fil, &pktHeader, sizeof(RTK_PKT_HEADER), &read_len)) != FR_OK)
//        {
//            APP_PRINT_ERROR2("audio_fs_decode_get_frame: frameSize:0x%x, res:0x%x", frameSize, res);
//            return ret;
//        }
//        offset += sizeof(RTK_PKT_HEADER);
//        if (pktHeader.pktIsInValid == 1) /* pkt is Invalid */
//        {
//            offset += frameSize;
//            audio_fs_set_file_offset(handle, offset);
//            if (g_frameContent.content != NULL)
//            {
//                memcpy(p_frameContent, &g_frameContent, sizeof(FRAME_CONTENT));
//            }
//            return 0;
//        }
//    }
//    if (g_frameInfo.format_info.rtk.rtkTransFormat == RTK_SBC)
//    {
//        g_frameContent.frameNum = pktHeader.pktFrameNum;
//    }
//    else
    {
        g_frameContent.frameNum = 1;
    }
    if (g_frameContent.content != NULL)
    {
        free(g_frameContent.content);
        g_frameContent.content = NULL;
    }
    //APP_PRINT_TRACE1("audio_fs_decode_get_frame: memory: 0x%x", mem_peek());
    g_frameContent.length = frameSize;
    uint32_t ramSize = mem_peek();
    if (ramSize == 0)
    {
        return AUDIO_FS_NO_SPACE;
    }
    if (frameSize > ramSize)
    {
        frameSize = ramSize - 1;
    }
    g_frameContent.content = os_mem_alloc(RAM_TYPE_DSPSHARE, frameSize);
    if (g_frameContent.content == NULL)
    {
        APP_PRINT_ERROR0("audio_fs_decode_get_frame: os_mem_alloc failed!");
        return AUDIO_FS_ERR_MALLOC;
    }
    memset(g_frameContent.content, 0, frameSize);
    //APP_PRINT_TRACE1("audio_fs_decode_get_frame: memory: 0x%x", mem_peek());
//    audio_fs_set_file_offset(handle, offset);
//    if ((ret = f_read(&audio_file->fil, g_frameContent.content, frameSize, &read_len)) != FR_OK)
    stream_read_data(g_frameContent.content, frameSize, &read_len);
    if (read_len > stream_get_data_size())
    {
        ret = AUDIO_FS_ERR_BIG_FRAME_LEN;
        APP_PRINT_ERROR2("audio_fs_decode_get_frame: frameSize:0x%x, remain size:0x%x", frameSize,
                         stream_get_data_size());
        return ret;
    }
#if MP4_FORMAT_SUPPORT
    g_mp4HeaderInfo.curFrameNum ++;
#endif
    offset += frameSize;
//    audio_fs_set_file_offset(handle, offset);
#if AUDIO_FS_DECODE_DEBUG
    if (frameSize >= 32)
    {
        APP_PRINT_INFO3("audio_fs_decode_get_frame: frameSize:0x%x, (%b), tail(%b)",
                        frameSize, TRACE_BINARY(20, g_frameContent.content),
                        TRACE_BINARY(4, g_frameContent.content + frameSize - 4));
    }
    else
    {
        APP_PRINT_INFO2("audio_fs_decode_get_frame: frameSize, 0x%x(%b)",
                        frameSize, TRACE_BINARY(frameSize, g_frameContent.content));
    }
#endif
    memcpy(p_frameContent, &g_frameContent, sizeof(FRAME_CONTENT));
    return 0;
}

void audio_fs_decode_get_frame_info(FRAME_INFO *p_frameInfo)
{
    switch (g_frameInfo.format)
    {
#if AAC_FORMAT_SUPPORT
    case RTK:
    case AAC:
#endif
#if MP3_FORMAT_SUPPORT
    case MP3:
#endif
#if MP4_FORMAT_SUPPORT
    case MP4:
        g_mp4HeaderInfo.curFrameNum--;
#endif
        memcpy(p_frameInfo, &g_frameInfo, sizeof(FRAME_INFO));
        break;
    default:
        break;
    }
}
#endif
