/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "os_queue.h"
#include "stdlib.h"
#include "os_queue.h"
#include "a2dp_enc_buffer.h"

#define USE_RAM_FROM_MEDIABUF_POOL_EN   1
#if USE_RAM_FROM_MEDIABUF_POOL_EN
#include "audio_probe.h"
#endif

T_OS_QUEUE a2dp_enc_queue;

void *a2dp_enc_audio_peek(int offset)
{
    void *p_media_dspkt = os_queue_peek(&a2dp_enc_queue, offset);;
    return p_media_dspkt;
}

uint8_t a2dp_enc_audio_flush(uint16_t cnt)
{
    uint16_t i;
    T_A2DP_ENC_MEIDAHEAD *p_media_head;
    APP_PRINT_INFO1("media_buffer_a2dp_enc_flush: %d", cnt);
    if (cnt > a2dp_enc_queue.count)
    {
        cnt = a2dp_enc_queue.count;
    }
    for (i = 0; i < cnt; i++)
    {
        p_media_head = (T_A2DP_ENC_MEIDAHEAD *)os_queue_out(&a2dp_enc_queue);

#if USE_RAM_FROM_MEDIABUF_POOL_EN
        audio_probe_media_buffer_free(p_media_head);
#else
        free(p_media_head);
#endif
    }
    return 0;
}

bool a2dp_enc_data_rx(uint8_t *packet_pt, uint16_t packet_length, uint8_t frame_num)
{
    T_A2DP_ENC_MEIDAHEAD media_head;
    T_A2DP_ENC_MEIDAHEAD *mediapacket = NULL;

    if (packet_pt == NULL)
    {
        APP_PRINT_ERROR0("a2dp_enc_data_rx pointer null");
        return false;
    }
    media_head.p_next = NULL;
    media_head.payload_length = packet_length;
    media_head.frame_num = frame_num;
//    mediapacket = (T_A2DP_ENC_MEIDAHEAD *)os_mem_alloc(RAM_TYPE_DSPSHARE, sizeof(T_A2DP_ENC_MEIDAHEAD) + packet_length);
#if USE_RAM_FROM_MEDIABUF_POOL_EN
    mediapacket = audio_probe_media_buffer_malloc(sizeof(T_A2DP_ENC_MEIDAHEAD) + packet_length);
#else
//    mediapacket = (T_A2DP_ENC_MEIDAHEAD *)os_mem_alloc(RAM_TYPE_DSPSHARE, sizeof(T_A2DP_ENC_MEIDAHEAD) + packet_length);
    mediapacket = (T_A2DP_ENC_MEIDAHEAD *)os_mem_alloc(RAM_TYPE_DSPSHARE,
                                                       sizeof(T_A2DP_ENC_MEIDAHEAD) + packet_length);
#endif

    if (mediapacket == NULL)
    {
        APP_PRINT_ERROR0("a2dp_enc_data_rx get buffer error");
        return false;
    }
    memcpy(mediapacket, &media_head, sizeof(T_A2DP_ENC_MEIDAHEAD));
    memcpy((uint8_t *)mediapacket + sizeof(T_A2DP_ENC_MEIDAHEAD), packet_pt, packet_length);

    os_queue_in(&a2dp_enc_queue, mediapacket);

    return true;
}
