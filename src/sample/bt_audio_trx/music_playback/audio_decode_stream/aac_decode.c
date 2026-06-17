/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "audio_fs_decode.h"

#if AAC_FORMAT_SUPPORT
/*============================================================================*
  *                                  Variables
  *============================================================================*/
ID3V2_INFO  aacId3v2Info;
extern FRAME_INFO g_frameInfo;

const uint32_t aacSamplefrequencyTable[16] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
                                              16000, 12000, 11025, 8000, 7350, 0, 0, 0
                                             };
const uint16_t sbcSamplefrequencyTable[4] = {16000, 32000, 44100, 48000};
const uint8_t sbcBlocksTable[4] = {4, 8, 12, 16};
//const uint8_t sbcChannelModeTable[4] = {1, 2, 2, 2};
const uint8_t sbcSubbandsTable[2] = {4, 8};

/*============================================================================*
  *                                  Functions
  *============================================================================*/
uint32_t aac_decode_id3v2_header(uint8_t *id3v2Header, uint16_t id3v2HeaderLen)
{
    uint32_t header_len = 0;
    ID3V2H *tagHeader = (ID3V2H *)id3v2Header;
#if AUDIO_FS_DECODE_DEBUG
    APP_PRINT_INFO1("mp3_decode_id3v2_header, header(%b)", TRACE_BINARY(3, tagHeader));
#endif

    if ((id3v2HeaderLen == sizeof(ID3V2H)) && ((memcmp(tagHeader->header, "ID3", 3) == 0) &&
                                               ((tagHeader->version & 0xFF) != 0xFF)
                                               && ((tagHeader->revision & 0xFF) != 0xFF)/* && ((tagHeader->flag & 0x80) != 0x80)*/
                                               && ((tagHeader->size[0] & 0x80) != 0x80) && ((tagHeader->size[1] & 0x80) != 0x80)
                                               && ((tagHeader->size[2] & 0x80) != 0x80) && ((tagHeader->size[3] & 0x80) != 0x80)))
    {
        header_len = (tagHeader->size[0] << 21) | (tagHeader->size[1] << 14) |
                     (tagHeader->size[2] << 7) | (tagHeader->size[3]);
        aacId3v2Info.startOffset = sizeof(ID3V2H);
        aacId3v2Info.length = header_len;
        APP_PRINT_INFO3("aac_decode_id3v2_header: find ID3V2 header, header_len:0x%x, id3v2_startOffset:0x%x, id3v2_length:0x%x",
                        header_len, aacId3v2Info.startOffset, aacId3v2Info.length);
    }
    return header_len;
}

uint32_t aac_decode_id3v1_header(uint8_t *id3v1Header, uint16_t id3v1HeaderLen)
{
    uint32_t header_len = 0;
    char temp[30] = {0};
    ID3V1H *header = (ID3V1H *)id3v1Header;

    if (id3v1HeaderLen == sizeof(ID3V1H) && ((memcmp(header->tag, "TAG", 3) == 0) &&
                                             (memcmp(header->title, temp, 30) != 0)))
    {
        header_len = sizeof(ID3V1H);
        //APP_PRINT_INFO1("aac_decode_id3v1_header: find ID3V1 header, header_len:0x%x", header_len);
    }
    return header_len;
}

uint32_t aac_decode_aac_frame(uint8_t *aacHeader, uint16_t aacHeaderLen)
{
    uint16_t frame_duration = 0;
    uint32_t sampling_frequency = 0;
    uint8_t channel_mode = 0;
    uint8_t block_length = 0;
    uint8_t subbands = 0;
    uint8_t allocation_method = 0;
    uint8_t bitpool = 0;
    uint32_t fm_size = 0;
    uint8_t sampling_frequency_index = 0;
    RTK_PKT_HEADER *pPktHeader = (RTK_PKT_HEADER *)aacHeader;
    const uint16_t startOfFrame = sizeof(RTK_PKT_HEADER);
    SBC_FHEADER *pSbcHeader = (SBC_FHEADER *)&aacHeader[startOfFrame];

    if ((g_frameInfo.format == AAC) && (aacHeaderLen == ADTS_SYNC_WORD_LEN))
    {
        if (((aacHeader[0] & 0xFF) == 0xFF)
            && ((aacHeader[1] & 0xF0) == 0xF0) && ((aacHeader[1] & 0x6) == 0x0))
        {
            /* return frame length */
            fm_size |= ((aacHeader[3] & 0x03) << 11);       //high 2 bit
            fm_size |= aacHeader[4] << 3;                   //middle 8 bit
            fm_size |= ((aacHeader[5] & 0xe0) >> 5);        //low 3bit
            sampling_frequency_index = (aacHeader[2] & 0x3C) >> 2;
            sampling_frequency = aacSamplefrequencyTable[sampling_frequency_index];
            channel_mode = (((aacHeader[2] & 0x01) << 2) | ((aacHeader[3] & 0xC0) >> 6));
            frame_duration = (1024 * 1000) / sampling_frequency;
            g_frameInfo.format_info.aac.frame_duration = frame_duration;
            g_frameInfo.format_info.aac.sampling_frequency = sampling_frequency;
            g_frameInfo.format_info.aac.channel_mode = channel_mode;
        }
    }
    else if ((g_frameInfo.format == RTK) && (aacHeaderLen == ADTS_SYNC_WORD_LEN))
    {
        if (((aacHeader[0] & 0xFF) == 0xFF) && ((aacHeader[1] & 0xF0) == 0xF0) &&
            ((aacHeader[1] & 0x6) == 0x0))
        {
            /* return frame length */
            fm_size |= ((aacHeader[3] & 0x03) << 11);       //high 2 bit
            fm_size |= aacHeader[4] << 3;                   //middle 8 bit
            fm_size |= ((aacHeader[5] & 0xe0) >> 5);        //low 3bit
            sampling_frequency_index = (aacHeader[2] & 0x3C) >> 2;
            sampling_frequency = aacSamplefrequencyTable[sampling_frequency_index];
            channel_mode = (((aacHeader[2] & 0x01) << 2) | ((aacHeader[3] & 0xC0) >> 6));
            frame_duration = (1024 * 1000) / sampling_frequency;
            g_frameInfo.format_info.rtk.rtk_trans_info.adts.frame_duration = frame_duration;
            g_frameInfo.format_info.rtk.rtk_trans_info.adts.sampling_frequency = sampling_frequency;
            g_frameInfo.format_info.rtk.rtk_trans_info.adts.channel_mode = channel_mode;
        }
    }
    else if ((g_frameInfo.format == RTK) && (aacHeaderLen == LATM_SYNC_WORD_LEN))
    {
        if (((aacHeader[0] == 0xFF) && (aacHeader[startOfFrame] == 0x47) &&
             (aacHeader[startOfFrame + 1] == 0xFC) && (aacHeader[startOfFrame + 2] == 0x00) &&
             (aacHeader[startOfFrame + 3] == 0x00) && (aacHeader[startOfFrame + 4] == 0xB0) &&
             (aacHeader[startOfFrame + 5] == 0x90) && aacHeader[startOfFrame + 6] == 0x80) &&
            (aacHeader[startOfFrame + 7] == 0x03) && (aacHeader[startOfFrame + 8] == 0x00))
        {
            /* return pkt length */
            fm_size = pPktHeader->pktLength;
            sampling_frequency_index = ((aacHeader[startOfFrame + 5] >> 2) & 0xF);
            sampling_frequency = aacSamplefrequencyTable[sampling_frequency_index];
            channel_mode = (((aacHeader[startOfFrame + 5] & 0x01) << 2) |
                            ((aacHeader[startOfFrame + 6] & 0xC0) >> 6));
            g_frameInfo.format_info.rtk.rtk_trans_info.latm.sampling_frequency = sampling_frequency;
            g_frameInfo.format_info.rtk.rtk_trans_info.latm.channel_mode = channel_mode;
        }
    }
    else if ((g_frameInfo.format == RTK) && (aacHeaderLen == SBC_SYNC_WORD_LEN))
    {
        if ((aacHeader[0] == 0xFF) && (aacHeader[startOfFrame] == 0x9C))
        {
            /* return pkt length */
            fm_size = pPktHeader->pktLength;
            sampling_frequency_index = pSbcHeader->samplingFrequency;
            sampling_frequency = sbcSamplefrequencyTable[sampling_frequency_index];
            channel_mode = pSbcHeader->channelMode;
            block_length = sbcBlocksTable[pSbcHeader->blocks];
            subbands = sbcSubbandsTable[pSbcHeader->subbands];
            allocation_method = pSbcHeader->allocationMethod;
            bitpool = pSbcHeader->bitpool;
            g_frameInfo.format_info.rtk.rtk_trans_info.sbc.sampling_frequency = sampling_frequency;
            g_frameInfo.format_info.rtk.rtk_trans_info.sbc.channel_mode = channel_mode;
            g_frameInfo.format_info.rtk.rtk_trans_info.sbc.block_length = block_length;
            g_frameInfo.format_info.rtk.rtk_trans_info.sbc.subbands = subbands;
            g_frameInfo.format_info.rtk.rtk_trans_info.sbc.allocation_method = allocation_method;
            g_frameInfo.format_info.rtk.rtk_trans_info.sbc.bitpool = bitpool;
            if ((channel_mode == 0 || channel_mode == 1) && (bitpool > (16 * subbands)))
            {
                g_frameInfo.format_info.rtk.rtk_trans_info.sbc.bitpool = 16 * subbands;
            }
            else if ((channel_mode == 2 || channel_mode == 3) && (bitpool > (32 * subbands)))
            {
                g_frameInfo.format_info.rtk.rtk_trans_info.sbc.bitpool = 32 * subbands;
            }
#if AUDIO_FS_DECODE_DEBUG
            APP_PRINT_INFO4("aac_decode_aac_frame: block_length: %d, subbands: %d, allocation_method: %d, bitpool: %d",
                            block_length, subbands, allocation_method, bitpool);
#endif
        }
    }
    else
    {
        APP_PRINT_ERROR0("aac_decode_aac_frame: file format is not support!");
    }
// #if AUDIO_FS_DECODE_DEBUG
    APP_PRINT_INFO6("aac_decode_aac_frame: format:0x%x, rtkTransFormat:0x%x, fm_size: 0x%x, "
                    "sampling_freq: %d, channel: %d, frame_duration: %d",
                    g_frameInfo.format, g_frameInfo.format_info.rtk.rtkTransFormat, fm_size,
                    sampling_frequency, channel_mode, frame_duration);
// #endif
    return fm_size;
}

uint32_t aac_decode_judge_frame_header(uint8_t *aacHeader, uint16_t aacHeaderLen)
{
    const uint16_t startOfFrame = sizeof(RTK_PKT_HEADER);

    if ((aacHeaderLen == LATM_SYNC_WORD_LEN) && (aacHeader[0] == 0xFF) &&
        (aacHeader[startOfFrame] == 0x47) && (aacHeader[startOfFrame + 1] == 0xFC) &&
        (aacHeader[startOfFrame + 2] == 0x00) && (aacHeader[startOfFrame + 3] == 0x00) &&
        (aacHeader[startOfFrame + 4] == 0xB0) && (aacHeader[startOfFrame + 5] == 0x90) &&
        (aacHeader[startOfFrame + 6] == 0x80) && (aacHeader[startOfFrame + 7] == 0x03) &&
        (aacHeader[startOfFrame + 8] == 0x00))
    {
        return LATM_SYNC_WORD_LEN;
    }
    else if ((aacHeaderLen == SBC_SYNC_WORD_LEN) && (aacHeader[0] == 0xFF) &&
             (aacHeader[startOfFrame] == 0x9C))
    {
        return SBC_SYNC_WORD_LEN;
    }
    else if ((aacHeaderLen == ADTS_SYNC_WORD_LEN) && ((aacHeader[0] & 0xFF) == 0xFF)
             && ((aacHeader[1] & 0xF0) == 0xF0) && ((aacHeader[1] & 0x6) == 0x0))
    {
        return ADTS_SYNC_WORD_LEN;
    }
    return 0;
}
#endif /* AAC_FORMAT_SUPPORT == 1 */
