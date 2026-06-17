/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if (F_APP_A2DP_XMIT_SNK_SUPPORT || F_APP_A2DP_XMIT_SRC_SUPPORT || \
     F_APP_A2DP_XMIT_SNK_LEA_SUPPORT || F_APP_A2DP_XMIT_SRC_LEA_SUPPORT)

#include <stdlib.h>
#include "trace.h"
#include "app_a2dp_xmit_mgr.h"
#include "app_cmd.h"
#include "string.h"
#include "ring_buffer.h"
#include "app_dlps.h"

#if F_APP_A2DP_XMIT_SRC_SUPPORT
#include "app_a2dp_xmit_src.h"
#endif

#if F_APP_A2DP_XMIT_SRC_LEA_SUPPORT
#include "app_a2dp_xmit_lea.h"
#endif

#if (F_APP_A2DP_XMIT_SNK_SUPPORT || F_APP_A2DP_XMIT_SNK_LEA_SUPPORT)

T_AUDIO_FORMAT_INFO             a2dp_sink_format;

void app_a2dp_xmit_mgr_save_a2dp_snk_param(uint8_t *a2dp_param, uint16_t len)
{
    memcpy(&a2dp_sink_format, a2dp_param, len);
    if (a2dp_sink_format.type == AUDIO_FORMAT_TYPE_SBC)
    {
        if (a2dp_sink_format.attr.sbc.bitpool == 0)
        {
            a2dp_sink_format.attr.sbc.bitpool = 0x22;
        }
    }
}

void app_a2dp_xmit_mgr_report_a2dp_param(void)
{
    app_report_event(CMD_PATH_SPI, CMD_A2DP_XMIT_CONFIG, 0, (uint8_t *)&a2dp_sink_format,
                     sizeof(T_AUDIO_FORMAT_INFO));
}

void app_a2dp_xmit_mgr_report_a2dp_data(uint8_t *p_data, uint16_t len)
{
    app_report_event(CMD_PATH_SPI, CMD_A2DP_XMIT_AUDIO, 0, p_data, len);
}
#endif

#if (F_APP_A2DP_XMIT_SRC_SUPPORT || F_APP_A2DP_XMIT_SRC_LEA_SUPPORT)

#define A2DP_RBUF_SIZE              1024 * 3
static struct
{
    T_AUDIO_FORMAT_INFO             a2dp_in_format;
    bool                            a2dp_in_format_ready;
    T_A2DP_XMIT_PLAY_ROUTE          xmit_play_route;
    T_RING_BUFFER                   ring_buf;
    uint8_t                         *buffer;
} a2dp_xmit_mgr =
{
    .a2dp_in_format_ready = false,
    .xmit_play_route = XMIT_PLAY_ROUTE_INVALID,
};

void app_a2dp_xmit_mgr_mgr_a2dp_raw_data_clear(void)
{
    ring_buffer_clear(&a2dp_xmit_mgr.ring_buf);
}

uint16_t app_a2dp_xmit_mgr_a2dp_raw_data_read(uint8_t *p_data, uint32_t len)
{
    uint16_t ret = A2DP_XMIT_MGR_SUCCESS;
    uint32_t used_size = 0;
    used_size = ring_buffer_get_data_count(&a2dp_xmit_mgr.ring_buf);
    if (len > used_size)
    {
        return A2DP_XMIT_MGR_READ_ERROR;
    }
    ring_buffer_read(&a2dp_xmit_mgr.ring_buf, len, p_data);
    return ret;
}

uint16_t app_a2dp_xmit_mgr_a2dp_raw_data_write(uint8_t *p_data, T_A2DP_XMIT_AUDIO_INFO audio_info)
{
    uint16_t ret = A2DP_XMIT_MGR_SUCCESS;

    uint32_t space_size = 0;
    uint32_t used_size = 0;

    space_size = ring_buffer_get_remaining_space(&a2dp_xmit_mgr.ring_buf);
    used_size = ring_buffer_get_data_count(&a2dp_xmit_mgr.ring_buf);

    if (space_size < audio_info.len + sizeof(T_A2DP_XMIT_AUDIO_INFO))
    {
        APP_PRINT_WARN2("app_a2dp_xmit_mgr_a2dp_raw_data_write: space_size %d, used_size:%d", space_size,
                        used_size);
        ret = A2DP_XMIT_MGR_WRITE_ERROR;
    }
    else
    {
        ring_buffer_write(&a2dp_xmit_mgr.ring_buf, (uint8_t *)&audio_info, sizeof(T_A2DP_XMIT_AUDIO_INFO));
        ring_buffer_write(&a2dp_xmit_mgr.ring_buf, p_data, audio_info.len);
    }

    return ret;
}

void app_a2dp_xmit_mgr_print_format(const char *title, T_AUDIO_FORMAT_INFO format_info)
{
    APP_PRINT_TRACE1("@@@@@%s@@@@@", TRACE_STRING(title));
    switch (format_info.type)
    {
    case AUDIO_FORMAT_TYPE_SBC:
        {
            APP_PRINT_INFO6("SBC, "
                            "sample_rate %d, allocation_method %d, bitpool %d, block_length %d, chann_mode %d, subband_num %d",
                            format_info.attr.sbc.sample_rate,
                            format_info.attr.sbc.allocation_method,
                            format_info.attr.sbc.bitpool,
                            format_info.attr.sbc.block_length,
                            format_info.attr.sbc.chann_mode,
                            format_info.attr.sbc.subband_num);
        }
        break;

    case AUDIO_FORMAT_TYPE_AAC:
        {
            APP_PRINT_INFO4("AAC, "
                            "transport_format:0x%x, sample_rate:%d, channel_mode:%d, bitrate:%d",
                            format_info.attr.aac.transport_format,
                            format_info.attr.aac.sample_rate,
                            format_info.attr.aac.chann_num,
                            format_info.attr.aac.bitrate);
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            APP_PRINT_INFO5("LC3, "
                            "sample_rate %d, chann %d, frame_length %d, duration %d, delay %d",
                            format_info.attr.lc3.sample_rate,
                            format_info.attr.lc3.chann_location,
                            format_info.attr.lc3.frame_length,
                            format_info.attr.lc3.frame_duration,
                            format_info.attr.lc3.presentation_delay);
        }
        break;

    default:
        break;
    }
}

void app_a2dp_xmit_mgr_save_a2dp_in_format(uint8_t *format_info, uint16_t param_len)
{
    if (!a2dp_xmit_mgr.a2dp_in_format_ready)
    {
        memcpy(&a2dp_xmit_mgr.a2dp_in_format, format_info, param_len);
        a2dp_xmit_mgr.a2dp_in_format_ready = true;
    }
    app_a2dp_xmit_mgr_print_format("app_a2dp_xmit_mgr_save_a2dp_in_format: ",
                                   a2dp_xmit_mgr.a2dp_in_format);
}

bool app_a2dp_xmit_mgr_get_a2dp_in_format(uint8_t *format_info)
{
    if (a2dp_xmit_mgr.a2dp_in_format_ready)
    {
        memcpy(format_info, &a2dp_xmit_mgr.a2dp_in_format, sizeof(T_AUDIO_FORMAT_INFO));
        return true;
    }
    return false;
}

void app_a2dp_xmit_mgr_route_out_start_stop(T_A2DP_XMIT_PLAY_STATE type)
{
    if (type == XMIT_PLAY_STATE_START)
    {
        app_dlps_disable(APP_DLPS_ENTER_CHECK_XMIT);
    }
    else
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_XMIT);
    }

    switch (a2dp_xmit_mgr.xmit_play_route)
    {
#if F_APP_A2DP_XMIT_SRC_SUPPORT
    case XMIT_PLAY_ROUTE_A2DP_SRC:
        {
            app_a2dp_xmit_src_start_stop(type);
        }
        break;
#endif
#if F_APP_A2DP_XMIT_SRC_LEA_SUPPORT
    case XMIT_PLAY_ROUTE_BIS:
        {
            app_a2dp_xmit_lea_src_start_stop(type);
        }
        break;
#endif
    default:
        APP_PRINT_ERROR1("app_a2dp_xmit_mgr_route_out_start_stop: invalid route path %d",
                         a2dp_xmit_mgr.xmit_play_route);
        break;
    }
}

void app_a2dp_xmit_mgr_handle_a2dp_data_in(uint8_t *p_audio, T_A2DP_XMIT_AUDIO_INFO audio_info)
{
    switch (a2dp_xmit_mgr.xmit_play_route)
    {
#if F_APP_A2DP_XMIT_SRC_SUPPORT
    case XMIT_PLAY_ROUTE_A2DP_SRC:
        {
            app_a2dp_xmit_src_handle_a2dp_data_ind(p_audio, audio_info);
        }
        break;
#endif
#if F_APP_A2DP_XMIT_SRC_LEA_SUPPORT
    case XMIT_PLAY_ROUTE_BIS:
        {
            app_a2dp_xmit_lea_handle_a2dp_data_ind(p_audio, audio_info);
        }
        break;
#endif
    default:
        APP_PRINT_ERROR1("app_a2dp_xmit_mgr_handle_a2dp_data_in: invalid route path %d",
                         a2dp_xmit_mgr.xmit_play_route);
        break;
    }
}

void app_a2dp_xmit_mgr_init(void)
{
    a2dp_xmit_mgr.buffer = (uint8_t *)calloc(1, A2DP_RBUF_SIZE);
    ring_buffer_init(&a2dp_xmit_mgr.ring_buf, a2dp_xmit_mgr.buffer,
                     A2DP_RBUF_SIZE);
#if F_APP_A2DP_XMIT_SRC_SUPPORT
    app_a2dp_xmit_src_init();
#endif
#if F_APP_A2DP_XMIT_SRC_LEA_SUPPORT
    app_a2dp_xmit_lea_init();
#endif
}

#endif

void app_a2dp_xmit_mgr_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                      uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE3("app_a2dp_xmit_mgr_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);

    switch (cmd_id)
    {
#if (F_APP_A2DP_XMIT_SRC_SUPPORT || F_APP_A2DP_XMIT_SRC_LEA_SUPPORT)
    case CMD_A2DP_XMIT_ROUTE_OUT_CTRL:
        {
            T_A2DP_XMIT_PLAY_STATE type = (T_A2DP_XMIT_PLAY_STATE)cmd_ptr[2];
            app_a2dp_xmit_mgr_route_out_start_stop(type);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_A2DP_XMIT_CONFIG:
        {
            uint8_t *p_param = &cmd_ptr[2];
            uint16_t param_len = cmd_len - 2;
            app_a2dp_xmit_mgr_save_a2dp_in_format(p_param, param_len);
        }
        break;

    case CMD_A2DP_XMIT_AUDIO:
        {
            if (!a2dp_xmit_mgr.a2dp_in_format_ready)
            {
                APP_PRINT_ERROR0("NEED A2DP FORMAT FROM REMOTE DEVICE!!!");
                app_report_event(CMD_PATH_SPI, CMD_A2DP_XMIT_NEED_FORMAT, 0, NULL, 0);
                break;
            }

            struct
            {
                uint16_t    cmd_id;
                T_A2DP_XMIT_AUDIO_INFO  audio_info;
                uint8_t     p_audio[];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            app_a2dp_xmit_mgr_handle_a2dp_data_in(cmd->p_audio, cmd->audio_info);
        }
        break;

    case CMD_A2DP_XMIT_SET_ROUTE_OUT:
        {
            a2dp_xmit_mgr.xmit_play_route = (T_A2DP_XMIT_PLAY_ROUTE)cmd_ptr[2];
            APP_PRINT_INFO1("app_a2dp_xmit_mgr_handle_cmd_set: xmit_play_route %d",
                            a2dp_xmit_mgr.xmit_play_route);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if (F_APP_A2DP_XMIT_SNK_SUPPORT || F_APP_A2DP_XMIT_SNK_LEA_SUPPORT)
    case CMD_A2DP_XMIT_NEED_FORMAT:
        {
            app_a2dp_xmit_mgr_report_a2dp_param();
        }
        break;
#endif

    default:
        break;
    }
}

#endif

