/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "gap.h"
#include "gap_iso_data.h"
#include "bt_direct_msg.h"
#include "app_lea_ini_audio_data.h"
#include "ble_audio_def.h"
#include "codec_qos.h"
#include "app_main.h"
#include "string.h"
#include "os_sync.h"
#include "app_src_play.h"
#if F_APP_A2DP_XMIT_SRC_LEA_SUPPORT
#include "app_a2dp_xmit_lea.h"
#endif

#if LE_AUDIO_ISO_REF_CLK
#include "audio_probe.h"
#include "bt_syscall.h"
#include "syncclk_driver.h"
#endif

#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
#include "app_tri_dongle_bond_manager.h"
#include "app_tri_dongle_cmd.h"
#endif

#if F_SOURCE_PLAY_SUPPORT
#define AUDIO_LEFT_OUTPUT_MASK    0x01
#define AUDIO_RIGHT_OUTPUT_MASK   0x02

#define ISO_DATA_CHNL_DELTA_TIMESTAP   3000
#endif

#if F_SOURCE_PLAY_SUPPORT

typedef struct t_iso_data_ind
{
    struct t_iso_data_ind   *p_next;
    uint16_t                conn_handle;
    T_ISOCH_DATA_PKT_STATUS pkt_status_flag;
    uint32_t                time_stamp;
    uint16_t                iso_sdu_len;
    uint16_t                pkt_seq_num;
    uint8_t                 *p_buf;
} T_ISO_DATA_IND;

// static T_AUDIO_FORMAT_INFO             save_format;

static void clear_iso_channel_queue(uint8_t output_mask);

static bool lea_data_check_iso(T_BT_DIRECT_ISO_DATA_IND *p_direct, uint16_t sdu_len);
#endif

void app_lea_print_lc3_format(const char *title, T_AUDIO_FORMAT_INFO *format_info)
{
    APP_PRINT_TRACE1("@@@@@%s@@@@@", TRACE_STRING(title));
    switch (format_info->type)
    {
    case AUDIO_FORMAT_TYPE_LC3:
        {
            APP_PRINT_INFO5("LC3, "
                            "sample_rate %d, chann_location %d, frame_length %d, frame_duration %d, presentation_delay %d",
                            format_info->attr.lc3.sample_rate,
                            format_info->attr.lc3.chann_location,
                            format_info->attr.lc3.frame_length,
                            format_info->attr.lc3.frame_duration,
                            format_info->attr.lc3.presentation_delay
                           );
        }
        break;

    default:
        break;
    }
}

static T_APP_LEA_ISO_CHANN *app_lea_find_iso_pending_chann(uint16_t iso_conn_handle,
                                                           uint8_t direction)
{
    int i;
    T_APP_LEA_ISO_CHANN *p_iso_chann = NULL;
    if (direction == DATA_PATH_INPUT_FLAG)
    {
        for (i = 0; i < app_db.input_path_pending_q.count; i++)
        {
            p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.input_path_pending_q, i);
            if (p_iso_chann->iso_conn_handle == iso_conn_handle)
            {
                return p_iso_chann;
            }
        }
    }
    return NULL;
}

static T_APP_LEA_ISO_CHANN *app_lea_find_iso_chann(uint16_t iso_conn_handle, uint8_t direction)
{
    int i;
    T_APP_LEA_ISO_CHANN *p_iso_chann = NULL;
    if (direction == DATA_PATH_INPUT_FLAG)
    {
        for (i = 0; i < app_db.iso_input_queue.count; i++)
        {
            p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_input_queue, i);
            if (p_iso_chann->iso_conn_handle == iso_conn_handle)
            {
                return p_iso_chann;
            }
        }
    }
    else
    {
        for (i = 0; i < app_db.iso_output_queue.count; i++)
        {
            p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_output_queue, i);
            if (p_iso_chann->iso_conn_handle == iso_conn_handle)
            {
                return p_iso_chann;
            }
        }
    }
    return NULL;
}

T_APP_LEA_ISO_CHANN *app_lea_add_iso_pending_chann(uint16_t iso_conn_handle, uint8_t direction)
{
    T_APP_LEA_ISO_CHANN *p_iso_chann;

    p_iso_chann = calloc(1, sizeof(T_APP_LEA_ISO_CHANN));
    if (p_iso_chann)
    {
        p_iso_chann->iso_conn_handle = iso_conn_handle;
        p_iso_chann->path_direction = direction;
        if (direction == DATA_PATH_INPUT_FLAG)
        {
            os_queue_in(&app_db.input_path_pending_q, p_iso_chann);
        }
        APP_PRINT_INFO2("app_lea_add_iso_pending_chann: iso_conn_handle 0x%x, direction %d",
                        iso_conn_handle, direction);
    }
    return p_iso_chann;
}

T_APP_LEA_ISO_CHANN *app_lea_add_iso_chann(uint16_t iso_conn_handle, uint8_t direction)
{
    T_APP_LEA_ISO_CHANN *p_iso_chann;

    p_iso_chann = app_lea_find_iso_chann(iso_conn_handle, direction);
    if (p_iso_chann)
    {
        return p_iso_chann;
    }
    p_iso_chann = calloc(1, sizeof(T_APP_LEA_ISO_CHANN));
    if (p_iso_chann)
    {
        p_iso_chann->iso_conn_handle = iso_conn_handle;
        p_iso_chann->path_direction = direction;
        if (direction == DATA_PATH_INPUT_FLAG)
        {
            os_queue_in(&app_db.iso_input_queue, p_iso_chann);
        }
        else
        {
            os_queue_in(&app_db.iso_output_queue, p_iso_chann);
        }
        APP_PRINT_INFO2("app_lea_add_iso_chann: iso_conn_handle 0x%x, direction %d",
                        iso_conn_handle, direction);
    }
    return p_iso_chann;
}

static void app_lea_remove_iso_pending_chann(uint16_t iso_conn_handle, uint8_t direction)
{
    int i;
    T_APP_LEA_ISO_CHANN *p_iso_chann = NULL;
    APP_PRINT_INFO2("app_lea_remove_iso_pending_chann: iso_conn_handle 0x%x, direction %d",
                    iso_conn_handle, direction);
    if (direction == DATA_PATH_INPUT_FLAG)
    {
        for (i = 0; i < app_db.input_path_pending_q.count; i++)
        {
            p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.input_path_pending_q, i);
            if (p_iso_chann->iso_conn_handle == iso_conn_handle)
            {
                os_queue_delete(&app_db.input_path_pending_q, p_iso_chann);
                free(p_iso_chann);
                return;
            }
        }
    }
}

static void app_lea_remove_iso_chann(uint16_t iso_conn_handle, uint8_t direction)
{
    int i;
    T_APP_LEA_ISO_CHANN *p_iso_chann = NULL;
    APP_PRINT_INFO3("app_lea_remove_iso_chann: iso_conn_handle 0x%x, direction %d, count %d",
                    iso_conn_handle, direction, app_db.iso_input_queue.count);
    if (direction == DATA_PATH_INPUT_FLAG)
    {
        for (i = 0; i < app_db.iso_input_queue.count; i++)
        {
            p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_input_queue, i);
            if (p_iso_chann->iso_conn_handle == iso_conn_handle)
            {
                os_queue_delete(&app_db.iso_input_queue, p_iso_chann);
                free(p_iso_chann);
                return;
            }
        }
    }
    else
    {
        for (i = 0; i < app_db.iso_output_queue.count; i++)
        {
            p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_output_queue, i);
            if (p_iso_chann->iso_conn_handle == iso_conn_handle)
            {
                os_queue_delete(&app_db.iso_output_queue, p_iso_chann);
                free(p_iso_chann);
                return;
            }
        }
    }
}

static uint32_t app_lea_get_sample_rate(T_APP_LEA_ISO_CHANN *p_iso_chann)
{
    uint32_t sample_rate = 0;

    switch (p_iso_chann->codec_data.sample_frequency)
    {
    case SAMPLING_FREQUENCY_CFG_16K:
        {
            sample_rate = 16000;
        }
        break;
    case SAMPLING_FREQUENCY_CFG_44_1K:
        {
            sample_rate = 44100;
        }
        break;
    case SAMPLING_FREQUENCY_CFG_48K:
        {
            sample_rate = 48000;
        }
        break;

    default:
        APP_PRINT_WARN0("app_lea_get_sample_rate: unsupport cfg");
        break;
    }
    return sample_rate;
}

static bool app_lea_get_next_ap(T_SYNCCLK_LATCH_INFO_TypeDef *p_latch_info,
                                T_BT_LE_ISO_SYNC_REF_AP_INFO *info, uint32_t *p_ap)
{
    if (!p_latch_info || !info || !p_ap)
    {
        return false;
    }

    uint32_t anchor_point = 0;
    uint32_t iso_interval = 0;
    uint32_t freerun_clk = 0;
    uint32_t cnt = 10;

    anchor_point = info->group_anchor_point;
    iso_interval = info->iso_interval_us;
    freerun_clk = p_latch_info->exp_sync_clock;

    if (!iso_interval)
    {
        APP_PRINT_ERROR0("app_lea_get_next_ap: iso_interval invalid");
        return false;
    }

    if ((freerun_clk == 0) || (anchor_point == 0))
    {
        APP_PRINT_ERROR2("app_lea_get_next_ap: freerun_clk %x anchor_point %x",
                         freerun_clk, anchor_point);
        return false;
    }

    while (cnt)
    {
        if (freerun_clk >= anchor_point)
        {
            if (0xFFFFFFFF - freerun_clk + anchor_point <= iso_interval)
            {
                *p_ap = anchor_point;
                return true;
            }
        }
        else
        {
            if (anchor_point - freerun_clk <= iso_interval)
            {
                *p_ap = anchor_point;
                return true;
            }
        }
        anchor_point += iso_interval;
        cnt -- ;
    }

    APP_PRINT_ERROR2("app_lea_get_next_ap: error freerun_clk %x anchor_point %x",
                     freerun_clk, info->group_anchor_point);
    return false;
}

bool le_audio_get_ap_delta(uint32_t *ap_delta, uint32_t *iso_interval)
{
    if (!ap_delta || !iso_interval)
    {
        return false;
    }

    T_BT_LE_ISO_SYNC_REF_AP_INFO temp_info;
    T_APP_LEA_ISO_CHANN *iso_elem = NULL;
    uint32_t anchor_point = 0;
    uint32_t delta = 0;
    uint32_t freerun_clk = 0;
    T_SYNCCLK_LATCH_INFO_TypeDef *p_latch_info = NULL;

    iso_elem = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_input_queue, 0);
    if (iso_elem == NULL)
    {
        APP_PRINT_ERROR0("le_audio_get_ap_delta: no data path valid");
        return false;
    }

    temp_info.dir = 1;
    temp_info.conn_handle = iso_elem->iso_conn_handle;
    bt_get_le_iso_sync_ref_ap_info(&temp_info);
    p_latch_info = synclk_drv_time_get(LEA_SYNC_CLK_REF);
    freerun_clk = p_latch_info->exp_sync_clock;

    if (app_lea_get_next_ap(p_latch_info, &temp_info, &anchor_point) == false)
    {
        APP_PRINT_ERROR0("app_lea_get_next_ap: get fail");
        return false;
    }

    if (freerun_clk <= anchor_point)
    {
        delta = anchor_point - freerun_clk;
    }
    else
    {
        delta = 0xFFFFFFFF - freerun_clk + anchor_point;
    }

    *ap_delta = delta;
    *iso_interval = temp_info.iso_interval_us;
    APP_PRINT_INFO5("le_audio_get_ap_delta: freerun_clk %d anchor_point %d diff %d origin ap %d iso %x",
                    freerun_clk, anchor_point, delta, temp_info.group_anchor_point,
                    temp_info.iso_interval_us);
    return true;

}

void app_lea_iso_data_send(uint8_t *p_data, uint16_t len, bool ext_flag, uint32_t ts, uint16_t seq)
{
    T_GAP_CAUSE cause = GAP_CAUSE_SUCCESS;
    T_APP_LEA_ISO_CHANN *p_iso_chann = NULL;
    uint8_t chnl_cnt = 0;
    uint32_t time_stamp = 0;
    uint16_t seq_num = 0;

    uint32_t s = os_lock();
    for (uint8_t i = 0; i < app_db.iso_input_queue.count; i++)
    {
        p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_input_queue, i);
        if (p_iso_chann != NULL)
        {
            if (p_iso_chann->codec_data.audio_channel_allocation == AUDIO_LOCATION_MONO)
            {
                chnl_cnt = 1;
            }
            else
            {
                chnl_cnt = __builtin_popcount(p_iso_chann->codec_data.audio_channel_allocation);
            }
            if (ext_flag)
            {
                time_stamp = ts;
                seq_num = seq;
            }
            else
            {
                time_stamp = (p_iso_chann->time_stamp + p_iso_chann->pkt_seq_num) *
                             (p_iso_chann->sdu_interval);
                seq_num = p_iso_chann->pkt_seq_num;
            }


            if (chnl_cnt == 2)
            {
                cause = gap_iso_send_data(p_data,
                                          p_iso_chann->iso_conn_handle,
                                          len,
                                          false,
                                          time_stamp,
                                          seq_num);

            }
            else if (chnl_cnt == 1)
            {
                if (p_iso_chann->codec_data.audio_channel_allocation & (AUDIO_LOCATION_FL |
                                                                        AUDIO_LOCATION_SIL))
                {
                    cause = gap_iso_send_data(p_data,
                                              p_iso_chann->iso_conn_handle,
                                              len / 2,
                                              0,
                                              time_stamp,
                                              seq_num);
                }
                else
                {
                    cause = gap_iso_send_data(p_data + (len / 2),
                                              p_iso_chann->iso_conn_handle,
                                              len / 2,
                                              0,
                                              time_stamp,
                                              seq_num);
                }
            }
            p_iso_chann->pkt_seq_num++;
            if (cause != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR1("app_lea_iso_data_send: failed, cause 0x%x", cause);
            }
        }
    }
    os_unlock(s);
    return;
}

bool app_lea_get_data_format(T_LEA_CODEC_DIR direction, T_AUDIO_FORMAT_INFO *format_info)
{
    memset(format_info, 0, sizeof(T_AUDIO_FORMAT_INFO));
    format_info->type = AUDIO_FORMAT_TYPE_LC3;
    format_info->frame_num = 1;

    T_OS_QUEUE *queue = NULL;
    T_APP_LEA_ISO_CHANN *iso_elem = NULL;

    if (direction == LEA_CODEC_DIR_ENCODE)
    {
        queue = &app_db.iso_input_queue;
    }
    else if (direction == LEA_CODEC_DIR_DECODE)
    {
        queue = &app_db.iso_output_queue;
    }
    else
    {
        APP_PRINT_ERROR1("app_lea_get_encode_data_format: direction error %d", direction);
        return false;
    }

    if (queue->count > 0)
    {
        for (uint8_t i = 0; i < queue->count; i++)
        {
            iso_elem = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_input_queue, i);
            APP_PRINT_INFO2("app_lea_get_data_format: iso %d audio_channel_allocation %d",
                            i, iso_elem->codec_data.audio_channel_allocation);
            // format_info->attr.lc3.chann_location |= iso_elem->codec_data.audio_channel_allocation;
        }
        iso_elem = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_input_queue, 0);

        format_info->attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_FL | AUDIO_CHANNEL_LOCATION_FR;

        format_info->attr.lc3.sample_rate = app_lea_get_sample_rate(iso_elem);
        format_info->attr.lc3.frame_length = iso_elem->codec_data.octets_per_codec_frame;
        format_info->attr.lc3.presentation_delay = iso_elem->presentation_delay;
        if (iso_elem->codec_data.frame_duration == FRAME_DURATION_CFG_7_5_MS)
        {
            format_info->attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_7_5_MS;
        }
        else
        {
            format_info->attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
        }
    }
    else
    {
        APP_PRINT_ERROR1("app_lea_get_encode_data_format: queue_count %d", queue->count);
        return false;
    }
    app_lea_print_lc3_format("app_lea_get_data_format", format_info);
    return true;
}

static uint8_t app_lea_calc_frame_num(T_APP_LEA_ISO_CHANN *p_iso_chann)
{
    uint8_t chnl_cnt = 0;
    uint8_t blocks_num = 1;
    if (p_iso_chann->codec_data.audio_channel_allocation == AUDIO_LOCATION_MONO)
    {
        chnl_cnt = 1;
    }
    else
    {
        chnl_cnt = __builtin_popcount(p_iso_chann->codec_data.audio_channel_allocation);
    }
    if (p_iso_chann->codec_data.type_exist & CODEC_CFG_TYPE_BLOCKS_PER_SDU_EXIST)
    {
        blocks_num = p_iso_chann->codec_data.codec_frame_blocks_per_sdu;
    }
    return blocks_num * chnl_cnt;
}

void app_lea_handle_cis_data_path_setup(T_LEA_SETUP_DATA_PATH *p_data)
{
    T_APP_LEA_ISO_CHANN *p_iso_chann = app_lea_find_iso_chann(p_data->iso_conn_handle,
                                                              p_data->path_direction);

    if (p_iso_chann != NULL)
    {
        APP_PRINT_WARN0("app_lea_handle_cis_data_path_setup: iso channel already exist");
        return;
    }
    else
    {
        if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
        {
            p_iso_chann = app_lea_add_iso_pending_chann(p_data->iso_conn_handle,
                                                        p_data->path_direction);
        }
        else
        {
            p_iso_chann = app_lea_add_iso_chann(p_data->iso_conn_handle,
                                                p_data->path_direction);

        }

        if (p_iso_chann == NULL)
        {
            return;
        }
        p_iso_chann->iso_mode = p_data->iso_mode;
    }
    p_iso_chann->codec_data = p_data->codec_parsed_data;
    p_iso_chann->presentation_delay = p_data->presentation_delay;
    p_iso_chann->time_stamp = 0;
    p_iso_chann->pkt_seq_num = 0;
    p_iso_chann->frame_num = app_lea_calc_frame_num(p_iso_chann);
    //lea src
    if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
    {
        uint8_t current_iso_cnt = app_db.input_path_pending_q.count + app_db.iso_input_queue.count;
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
        app_tri_dongle_set_busy_state(APP_TRI_DONGLE_BUSY_EVENT_LEA_DATA_ENABLE);
        uint8_t link_num = app_link_get_lea_link_num();
#else
        uint8_t link_num = app_link_get_le_link_num();
#endif
        APP_PRINT_INFO3("app_lea_handle_cis_data_path_setup: current_iso_cnt %d conn_dev %d, link_num %d",
                        current_iso_cnt,
                        app_db.conn_dev_queue.count,
                        link_num);
        if (current_iso_cnt >= link_num)
        {
            uint8_t i;
            uint8_t n = app_db.input_path_pending_q.count;

            APP_PRINT_INFO1("handle_cis_data_path_setup_cmplt_msg: pending num %u",
                            app_db.input_path_pending_q.count);
            for (i = 0; i < n; i++)
            {
                p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_out(&app_db.input_path_pending_q);
                APP_PRINT_INFO2("app_lea_handle_cis_data_path_setup: adding path handle %u curr_num %x",
                                p_iso_chann->iso_conn_handle,
                                app_db.iso_input_queue.count);
                os_queue_in(&app_db.iso_input_queue, (void *)p_iso_chann);
            }
        }
    }
    else
    {
#if F_SOURCE_PLAY_SUPPORT
        uint8_t chnl_cnt;
        if (p_data->codec_parsed_data.audio_channel_allocation == AUDIO_LOCATION_MONO)
        {
            chnl_cnt = 1;
        }
        else
        {
            chnl_cnt = __builtin_popcount(p_data->codec_parsed_data.audio_channel_allocation);
        }

        uint8_t codec_len = 0;
        uint8_t blocks_num = 1;
        if (p_data->codec_parsed_data.type_exist & CODEC_CFG_OCTET_PER_CODEC_FRAME_EXIST)
        {
            codec_len = p_data->codec_parsed_data.octets_per_codec_frame;
        }
        if (p_data->codec_parsed_data.type_exist & CODEC_CFG_TYPE_BLOCKS_PER_SDU_EXIST)
        {
            blocks_num = p_data->codec_parsed_data.codec_frame_blocks_per_sdu;
        }

        if (p_data->codec_parsed_data.frame_duration == FRAME_DURATION_CFG_7_5_MS)
        {
            app_db.output_frame_duration = 7500;
        }
        else
        {
            app_db.output_frame_duration = 10000;
        }

        if (chnl_cnt == 2)
        {
            app_db.output_chnl_mask = AUDIO_LEFT_OUTPUT_MASK | AUDIO_RIGHT_OUTPUT_MASK;
            app_db.output_left_handle = p_iso_chann->iso_conn_handle;
            app_db.output_right_handle = p_iso_chann->iso_conn_handle;
            app_db.output_left_len = app_db.output_right_len = codec_len;
            app_db.output_block_num = blocks_num;
        }
        else
        {
            APP_PRINT_INFO2("handle_cis_data_path_setup_cmplt_msg: codec_len %x block_num %x", codec_len,
                            blocks_num);
            if (p_data->codec_parsed_data.audio_channel_allocation & AUDIO_CHANNEL_LOCATION_FL)
            {
                app_db.output_chnl_mask |= AUDIO_LEFT_OUTPUT_MASK;
                app_db.output_left_handle = p_iso_chann->iso_conn_handle;
                app_db.output_left_len = codec_len;
                app_db.output_block_num = blocks_num;
            }
            else
            {
                app_db.output_chnl_mask |= AUDIO_RIGHT_OUTPUT_MASK;     //all left use right channel
                app_db.output_right_handle = p_iso_chann->iso_conn_handle;
                app_db.output_right_len = codec_len;
                app_db.output_block_num = blocks_num;
            }
        }

        clear_iso_channel_queue(app_db.output_chnl_mask);
#endif
    }

    APP_PRINT_INFO6("app_lea_handle_cis_data_path_setup: iso handle 0x%04x, frame_num %d, "
                    "dir %u, sample_frequency 0x%x, audio_channel_allocation 0x%08x, presentation_delay 0x%x",
                    p_iso_chann->iso_conn_handle, p_iso_chann->frame_num,
                    p_iso_chann->path_direction,
                    p_iso_chann->codec_data.sample_frequency,
                    p_iso_chann->codec_data.audio_channel_allocation,
                    p_iso_chann->presentation_delay);
}


void app_lea_handle_bis_data_path_setup(T_LEA_SETUP_DATA_PATH *p_data)
{
    T_APP_LEA_ISO_CHANN *p_iso_chann = app_lea_find_iso_chann(p_data->iso_conn_handle,
                                                              p_data->path_direction);

    if (p_iso_chann != NULL)
    {
        APP_PRINT_WARN0("app_lea_handle_bis_data_path_setup: iso channel already exist");
        return;
    }
    else
    {
        p_iso_chann = app_lea_add_iso_chann(p_data->iso_conn_handle,
                                            p_data->path_direction);
        if (p_iso_chann == NULL)
        {
            return;
        }
        p_iso_chann->iso_mode = p_data->iso_mode;
    }
    p_iso_chann->codec_data = p_data->codec_parsed_data;
    p_iso_chann->presentation_delay = p_data->presentation_delay;
    p_iso_chann->time_stamp = 0;
    p_iso_chann->pkt_seq_num = 0;
    p_iso_chann->frame_num = app_lea_calc_frame_num(p_iso_chann);

    APP_PRINT_INFO6("app_lea_handle_bis_data_path_setup: iso handle 0x%04x, frame_num %d, "
                    "dir %u, sample_frequency 0x%x, audio_channel_allocation 0x%08x, presentation_delay 0x%x",
                    p_iso_chann->iso_conn_handle, p_iso_chann->frame_num,
                    p_iso_chann->path_direction,
                    p_iso_chann->codec_data.sample_frequency,
                    p_iso_chann->codec_data.audio_channel_allocation,
                    p_iso_chann->presentation_delay);

    if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
    {
        if (app_db.bsrc_db.cfg_bis_num == app_db.iso_input_queue.count)
        {
#if F_APP_A2DP_XMIT_SRC_LEA_SUPPORT
            app_a2dp_xmit_lea_pipe_rcfg(); // TODO: put to BROADCAST_SOURCE_STATE_CONFIGURED?
#endif
        }
    }
}

void app_lea_handle_cis_data_path_remove(T_LEA_REMOVE_DATA_PATH *p_data)
{
    T_APP_LEA_ISO_CHANN *p_iso_chann = app_lea_find_iso_chann(p_data->iso_conn_handle,
                                                              p_data->path_direction);

    if (p_iso_chann != NULL)
    {
#if F_SOURCE_PLAY_SUPPORT
        if (p_iso_chann->iso_conn_handle == app_db.output_left_handle)
        {
            if (app_db.output_left_handle == app_db.output_right_handle)
            {
                app_db.output_chnl_mask = 0;
            }
            else
            {
                app_db.output_chnl_mask &= (~AUDIO_LEFT_OUTPUT_MASK);
            }
            app_db.output_left_handle = 0;
            app_db.output_left_ready = false;
            clear_iso_channel_queue(AUDIO_LEFT_OUTPUT_MASK);
        }
        else if (p_iso_chann->iso_conn_handle == app_db.output_right_handle)
        {
            app_db.output_chnl_mask &= (~AUDIO_RIGHT_OUTPUT_MASK);
            app_db.output_right_handle = 0;
            app_db.output_right_ready = false;
            clear_iso_channel_queue(AUDIO_RIGHT_OUTPUT_MASK);
        }
#endif
        app_lea_remove_iso_chann(p_data->iso_conn_handle, p_data->path_direction);
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
        app_tri_dongle_set_busy_state(APP_TRI_DONGLE_BUSY_EVENT_LEA_DATA_DISABLE);
#endif
    }

    p_iso_chann = app_lea_find_iso_pending_chann(p_data->iso_conn_handle,
                                                 p_data->path_direction);
    if (p_iso_chann != NULL)
    {
        app_lea_remove_iso_pending_chann(p_data->iso_conn_handle, p_data->path_direction);
    }
}

void app_lea_handle_bis_data_path_remove(T_LEA_REMOVE_DATA_PATH *p_data)
{
    T_APP_LEA_ISO_CHANN *p_iso_chann = app_lea_find_iso_chann(p_data->iso_conn_handle,
                                                              p_data->path_direction);
    if (p_iso_chann != NULL)
    {
        app_lea_remove_iso_chann(p_data->iso_conn_handle, p_data->path_direction);
    }
}

#if F_SOURCE_PLAY_SUPPORT
static bool lea_data_check_iso(T_BT_DIRECT_ISO_DATA_IND *p_direct, uint16_t sdu_len)
{
    if (!p_direct)
    {
        return false;
    }

    if (p_direct->pkt_status_flag != ISOCH_DATA_PKT_STATUS_VALID_DATA)
    {
        return false;
    }

    if (p_direct->iso_sdu_len != sdu_len)
    {
        return false;
    }
    return true;
}



static void clear_iso_channel_queue(uint8_t output_mask)
{
    int i;
    T_ISO_DATA_IND *iso_elem = NULL;
    T_OS_QUEUE *p_queue = NULL;

    if (AUDIO_LEFT_OUTPUT_MASK & output_mask)
    {
        p_queue = &(app_db.output_left_queue);
        for (i = 0; i < p_queue->count; i++)
        {
            iso_elem = (T_ISO_DATA_IND *)os_queue_peek(p_queue, i);
            os_queue_delete(p_queue, iso_elem);
            free(iso_elem);
        }
    }

    if (AUDIO_RIGHT_OUTPUT_MASK & output_mask)
    {
        p_queue = &(app_db.output_right_queue);
        for (i = 0; i < p_queue->count; i++)
        {
            iso_elem = (T_ISO_DATA_IND *)os_queue_peek(p_queue, i);
            os_queue_delete(p_queue, iso_elem);
            free(iso_elem);
        }
    }
}

static bool enqueue_iso_data(uint8_t output_mask, T_BT_DIRECT_ISO_DATA_IND *p_iso_data)
{
    T_ISO_DATA_IND *p_iso_buf = NULL;
    T_OS_QUEUE *p_queue = NULL;
    uint16_t output_handle = 0;

    if (output_mask == AUDIO_LEFT_OUTPUT_MASK)
    {
        p_queue = &(app_db.output_left_queue);
        output_handle = app_db.output_left_handle;
    }
    else
    {
        p_queue = &(app_db.output_right_queue);
        output_handle = app_db.output_right_handle;
    }

    if (output_handle != p_iso_data->conn_handle)
    {
        APP_PRINT_ERROR3("enqueue_iso_data mask %x handle %x data_handle %x", output_mask, output_handle,
                         p_iso_data->conn_handle);
        return false;
    }

    if (p_iso_data->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
    {
        p_iso_buf = calloc(1, sizeof(T_ISO_DATA_IND) + p_iso_data->iso_sdu_len);
    }
    else
    {
        p_iso_buf = calloc(1, sizeof(T_ISO_DATA_IND));
    }
    if (p_iso_buf == NULL)
    {
        APP_PRINT_ERROR1("enqueue_and_check_send_iso_data: alloc 0x%x fail", p_iso_data->iso_sdu_len);
        return false;
    }
    p_iso_buf->conn_handle = p_iso_data->conn_handle;
    p_iso_buf->time_stamp = p_iso_data->time_stamp;
    p_iso_buf->pkt_seq_num = p_iso_data->pkt_seq_num;
    p_iso_buf->pkt_status_flag = p_iso_data->pkt_status_flag;

    if (p_iso_data->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
    {
        p_iso_buf->p_buf = (uint8_t *)(p_iso_buf + 1);
        memcpy(p_iso_buf->p_buf, p_iso_data->p_buf + p_iso_data->offset, p_iso_data->iso_sdu_len);
        p_iso_buf->iso_sdu_len = p_iso_data->iso_sdu_len;
    }
    else
    {
        p_iso_buf->p_buf = NULL;
        p_iso_buf->iso_sdu_len = 0;
    }

    if (p_queue->count > 10)
    {
        T_ISO_DATA_IND *p_iso_elem = (T_ISO_DATA_IND *)os_queue_out(p_queue);
        free(p_iso_elem);
    }
    os_queue_in(p_queue, (void *)p_iso_buf);
    return true;
}

static bool check_enqueue_iso_timestamp(T_BT_DIRECT_ISO_DATA_IND *p_direct,
                                        T_ISO_DATA_IND *p_queue_iso)
{
    if ((!p_direct) || (!p_queue_iso))
    {
        return false;
    }

    if (p_direct->time_stamp >= p_queue_iso->time_stamp)
    {
        if (p_direct->time_stamp - p_queue_iso->time_stamp < ISO_DATA_CHNL_DELTA_TIMESTAP)
        {
            return true;
        }
        else if (0xFFFFFFFF - p_direct->time_stamp + p_queue_iso->time_stamp < ISO_DATA_CHNL_DELTA_TIMESTAP)
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
        if (p_queue_iso->time_stamp - p_direct->time_stamp < ISO_DATA_CHNL_DELTA_TIMESTAP)
        {
            return true;
        }
        else if (0xFFFFFFFF - p_queue_iso->time_stamp + p_direct->time_stamp < ISO_DATA_CHNL_DELTA_TIMESTAP)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

static T_ISO_DATA_IND *find_enqueue_proper_iso(T_OS_QUEUE *p_iso_queue,
                                               T_BT_DIRECT_ISO_DATA_IND *p_iso)
{
    if ((!p_iso_queue) || (!p_iso))
    {
        return NULL;
    }

    T_ISO_DATA_IND *p_iso_elem = NULL;
    while (p_iso_queue->count)
    {
        p_iso_elem = (T_ISO_DATA_IND *)os_queue_peek(p_iso_queue, 0);
        if (check_enqueue_iso_timestamp(p_iso, p_iso_elem))
        {
            return p_iso_elem;
        }
        else
        {
            APP_PRINT_WARN2("enqueue_and_check_send_iso_data: direct_time %x queue_time %x",
                            p_iso->time_stamp, p_iso_elem->time_stamp);
            APP_PRINT_WARN4("enqueue_and_check_send_iso_data: mis handle 0x%x : 0x%x, seq 0x%x : 0x%x",
                            p_iso->conn_handle, p_iso_elem->conn_handle,
                            p_iso->pkt_seq_num, p_iso_elem->pkt_seq_num);
            if (p_iso->time_stamp > p_iso_elem->time_stamp)
            {
                os_queue_delete(p_iso_queue, p_iso_elem);
                free(p_iso_elem);
            }
            else
            {
                return NULL;
            }
        }
    }

    return NULL;
}

static bool lea_data_handle_single_src_direct(T_APP_DB *p_db,
                                              T_BT_DIRECT_ISO_DATA_IND *p_direct)
{
    uint8_t octets = 0;
    uint8_t block_num = 1;
    uint8_t chnl_cnt = 0;
    uint8_t *p_temp = NULL;
    uint16_t block_len = 0;
    uint32_t timestamp = 0;
    uint32_t offset = 0;
    uint8_t i;
    T_APP_LEA_ISO_CHANN *p_iso = NULL;

    if ((!p_direct) || (!p_db))
    {
        return false;
    }

    if ((p_direct->conn_handle != p_db->output_left_handle) &&
        (p_direct->conn_handle != p_db->output_right_handle))
    {
        return false;
    }

    p_iso =  app_lea_find_iso_chann(p_direct->conn_handle, DATA_PATH_OUTPUT_FLAG);
    if (!p_iso)
    {
        return false;
    }

    octets = p_iso->codec_data.octets_per_codec_frame;
    if (p_iso->codec_data.type_exist & CODEC_CFG_TYPE_BLOCKS_PER_SDU_EXIST)
    {
        block_num = p_iso->codec_data.codec_frame_blocks_per_sdu;
    }

    if (p_iso->codec_data.audio_channel_allocation == AUDIO_LOCATION_MONO)
    {
        chnl_cnt = 1;
    }
    else
    {
        chnl_cnt = __builtin_popcount(p_iso->codec_data.audio_channel_allocation);
    }

    block_len = octets * chnl_cnt;

    p_temp = calloc(1, block_len * 2);
    if (!p_temp)
    {
        APP_PRINT_INFO1("le_audio_data_handle_direct alloc %x failed", block_len * 2);
        return false;
    }
    if (lea_data_check_iso(p_direct, block_len * block_num))
    {
        for (i = 0; i < block_num; i++)
        {
            offset = p_direct->offset + (i * block_len);
            memcpy(p_temp, p_direct->p_buf + offset, block_len);
            memcpy(p_temp + block_len, p_direct->p_buf + offset, block_len);
            timestamp = p_direct->time_stamp + i * p_db->output_frame_duration;
            app_src_play_bt_recv(p_temp, block_len * 2, ISOCH_DATA_PKT_STATUS_VALID_DATA,
                                 timestamp);
        }
    }
    else
    {
        for (i = 0; i < block_num; i++)
        {
            timestamp = p_direct->time_stamp + i * p_db->output_frame_duration;
            app_src_play_bt_recv(p_temp, block_len * 2, ISOCH_DATA_PKT_STATUS_LOST_DATA,
                                 timestamp);
        }
    }
    free(p_temp);
    return true;
}

static bool lea_data_handle_multi_src_direct(T_APP_DB *p_db,
                                             T_BT_DIRECT_ISO_DATA_IND *p_direct)
{
    if ((!p_db) || (!p_direct))
    {
        return false;
    }

    T_ISO_DATA_IND *p_iso_elem = NULL;
    uint8_t *p_send_buf = NULL;
    uint8_t codec_len = 0;
    uint16_t sdu_len = 0;
    T_OS_QUEUE *p_mirror_queue = NULL;
    T_ISOCH_DATA_PKT_STATUS pkt_flag = ISOCH_DATA_PKT_STATUS_VALID_DATA;
    bool direct_iso_flag = false;
    bool mirror_iso_flag = false;
    uint16_t offset = 0;
    uint32_t timestamp = 0;
    uint8_t output_mask = 0;

    if (p_direct->conn_handle == p_db->output_left_handle)
    {
        codec_len = p_db->output_left_len;
        p_mirror_queue = &(p_db->output_right_queue);
        sdu_len = p_db->output_left_len * p_db->output_block_num;
        output_mask = AUDIO_LEFT_OUTPUT_MASK;
    }
    else if (p_direct->conn_handle == p_db->output_right_handle)
    {
        codec_len = p_db->output_right_len;
        p_mirror_queue = &(p_db->output_left_queue);
        sdu_len = p_db->output_right_len * p_db->output_block_num;
        output_mask = AUDIO_RIGHT_OUTPUT_MASK;
    }
    else
    {
        return false;
    }

    if ((codec_len == 0) || (p_direct->iso_sdu_len % codec_len != 0))
    {
        APP_PRINT_ERROR0("enqueue_and_check_send_iso_data: iso_sdu_len wrong");
        return false;
    }

    if (p_mirror_queue->count == 0)
    {
        enqueue_iso_data(output_mask, p_direct);
        return true;
    }

    p_iso_elem = find_enqueue_proper_iso(p_mirror_queue, p_direct);
    if (p_iso_elem == NULL)
    {
        enqueue_iso_data(output_mask, p_direct);
        return true;
    }

    p_send_buf = calloc(1, sdu_len * 2);
    if (p_send_buf == NULL)
    {
        APP_PRINT_ERROR1("enqueue_and_check_send_iso_data: alloc 0x%x fail", sdu_len * 2);
        return false;
    }

    if ((p_direct->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA) &&
        (p_direct->iso_sdu_len != 0))
    {
        direct_iso_flag = true;
    }

    if ((p_iso_elem->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA) &&
        (p_iso_elem->iso_sdu_len != 0))
    {
        mirror_iso_flag = true;
    }

    for (int j = 0; j < app_db.output_block_num; j++)
    {
        pkt_flag = ISOCH_DATA_PKT_STATUS_VALID_DATA;
        offset = j * codec_len;
        //If one of two channels is not valid data, use the other channel instead of invalid channel
        if (direct_iso_flag && mirror_iso_flag)
        {
            memcpy(p_send_buf, p_direct->p_buf + p_direct->offset + offset, codec_len);
            memcpy(p_send_buf + codec_len, p_iso_elem->p_buf + offset, codec_len);
        }
        else if (direct_iso_flag && (!mirror_iso_flag))
        {
            APP_PRINT_WARN1("enqueue_and_check_send_iso_data: lost one channel data 0x%x",
                            p_iso_elem->pkt_seq_num);
            memcpy(p_send_buf, p_direct->p_buf + p_direct->offset + offset, codec_len);
            memcpy(p_send_buf + codec_len, p_direct->p_buf + p_direct->offset + offset, codec_len);

        }
        else if ((!direct_iso_flag) && (mirror_iso_flag))
        {
            APP_PRINT_WARN1("enqueue_and_check_send_iso_data: lost one channel data 0x%x",
                            p_direct->pkt_seq_num);
            memcpy(p_send_buf, p_iso_elem->p_buf + offset, codec_len);
            memcpy(p_send_buf + codec_len, p_iso_elem->p_buf + offset, codec_len);
            p_direct->time_stamp = p_iso_elem->time_stamp;
        }
        else
        {
            APP_PRINT_ERROR0("enqueue_and_check_send_iso_data: lost two channel data");
            pkt_flag = ISOCH_DATA_PKT_STATUS_LOST_DATA;
        }

        timestamp = p_direct->time_stamp + j * app_db.output_frame_duration;
        app_src_play_bt_recv(p_send_buf, codec_len * 2, pkt_flag, timestamp);
    }

    free(p_send_buf);
    os_queue_delete(p_mirror_queue, p_iso_elem);
    free(p_iso_elem);
    return true;
}
#endif

void app_lea_data_direct_cb(uint8_t cb_type, void *p_cb_data)
{
    T_BT_DIRECT_CB_DATA *p_data = (T_BT_DIRECT_CB_DATA *)p_cb_data;
    if (!p_data || !(p_data->p_bt_direct_iso))
    {
        return;
    }

    T_BT_DIRECT_ISO_DATA_IND *p_iso = p_data->p_bt_direct_iso;

    switch (cb_type)
    {
    case BT_DIRECT_MSG_ISO_DATA_IND:
        {
#if 0
            uint8_t *p_iso_data = p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset;
            APP_PRINT_INFO5("app_audio_data_direct_callback: BT_DIRECT_MSG_ISO_DATA_IND, pkt_status_flag 0x%x, cis_conn_handle 0x%x, pkt_seq_num 0x%x, ts_flag 0x%x, time_stamp 0x%x",
                            p_data->p_bt_direct_iso->pkt_status_flag, p_data->p_bt_direct_iso->conn_handle,
                            p_data->p_bt_direct_iso->pkt_seq_num, p_data->p_bt_direct_iso->ts_flag,
                            p_data->p_bt_direct_iso->time_stamp);
            APP_PRINT_INFO5("app_audio_data_direct_callback: BT_DIRECT_MSG_ISO_DATA_IND, iso_sdu_len 0x%x, p_buf %p, offset %d, p_data %p, data %b",
                            p_data->p_bt_direct_iso->iso_sdu_len, p_data->p_bt_direct_iso->p_buf,
                            p_data->p_bt_direct_iso->offset, p_iso_data, TRACE_BINARY(p_data->p_bt_direct_iso->iso_sdu_len,
                                                                                      p_iso_data));
#endif
            APP_PRINT_TRACE5("app_lea_data_direct_cb: conn_handle 0x%x, iso_sdu_len %d, pkt_seq_num 0x%x, time_stamp 0x%x, pkt_status_flag 0x%x",
                             p_data->p_bt_direct_iso->conn_handle, p_data->p_bt_direct_iso->iso_sdu_len,
                             p_data->p_bt_direct_iso->pkt_seq_num, p_data->p_bt_direct_iso->time_stamp,
                             p_data->p_bt_direct_iso->pkt_status_flag);
#if F_SOURCE_PLAY_SUPPORT
            if (app_db.output_left_handle == p_iso->conn_handle)
            {
                if (p_iso->iso_sdu_len != app_db.output_left_len)
                {
                    p_iso->pkt_status_flag = ISOCH_DATA_PKT_STATUS_POSSIBLE_ERROR_DATA;
                }
                if (!app_db.output_left_ready)
                {
                    if (p_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
                    {
                        APP_PRINT_INFO1("lea_data_handle_multi_src_direct pkt_seq_num %x left ready",
                                        p_iso->pkt_seq_num);
                        app_db.output_left_ready = true;
                    }
                }
            }
            else
            {
                if (p_iso->iso_sdu_len != app_db.output_right_len)
                {
                    p_iso->pkt_status_flag = ISOCH_DATA_PKT_STATUS_POSSIBLE_ERROR_DATA;
                }
                if (!app_db.output_right_ready)
                {
                    if (p_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
                    {
                        APP_PRINT_INFO1("lea_data_handle_multi_src_direct pkt_seq_num %x right ready",
                                        p_iso->pkt_seq_num);
                        app_db.output_right_ready = true;
                    }
                }
            }

            if (app_db.output_right_ready && app_db.output_left_ready)
            {
                lea_data_handle_multi_src_direct(&app_db, p_iso);
            }
            else if (app_db.output_right_ready &&
                     (app_db.output_right_handle == p_iso->conn_handle))
            {
                lea_data_handle_single_src_direct(&app_db, p_iso);
            }
            else if (app_db.output_left_ready &&
                     (app_db.output_left_handle == p_iso->conn_handle))
            {
                lea_data_handle_single_src_direct(&app_db, p_iso);
            }
#endif
            gap_iso_data_cfm(p_iso->p_buf);
        }
        break;

    default:
        APP_PRINT_ERROR1("app_lea_data_direct_cb: unhandled cb_type 0x%x", cb_type);
        break;
    }
}

#if (LE_AUDIO_ISO_REF_CLK == 1)
static void app_lea_send_ref(T_BT_LE_ISO_SYNC_REF_AP_INFO *info, uint32_t guard_time_us,
                             uint32_t session_id)
{
    uint16_t ref_info_len = sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO) + sizeof(guard_time_us);
    uint16_t cmd_len = ref_info_len + 4; // First 4 bytes parameters are session id
    uint8_t *cmd_buf = calloc(1, cmd_len + 4); // 4 bytes dsp command header: id, length

    if (cmd_buf != NULL)
    {
        uint8_t *p_buf = cmd_buf;
        LE_UINT16_TO_STREAM(p_buf, AUDIO_PROBE_UPLINK_SYNCREF);
        LE_UINT16_TO_STREAM(p_buf, cmd_len);
        LE_UINT32_TO_STREAM(p_buf, session_id);
        LE_UINT32_TO_STREAM(p_buf, guard_time_us);
        memcpy(p_buf, info, sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO));
        audio_probe_dsp_send(cmd_buf, cmd_len + 4);

        free(cmd_buf);
    }
}

static void app_lea_probe_cback(T_AUDIO_PROBE_EVENT event, void *buf)
{
    uint32_t session_id = (uint32_t)buf;
    T_BT_LE_ISO_SYNC_REF_AP_INFO temp_info, info;
    T_APP_LEA_ISO_CHANN *iso_elem = NULL;
    T_SYNCCLK_LATCH_INFO_TypeDef *p_latch_info = NULL;

    if (event != PROBE_SYNC_REF_REQUEST)
    {
        return;
    }

    memset(&info, 0, sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO));
    memset(&temp_info, 0, sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO));
    /* FIXME: what to do if get multi active conn_handle */

    for (uint8_t i = 0; i < app_db.iso_input_queue.count; i++)
    {
        iso_elem = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_input_queue, i);
        if (iso_elem->path_direction != DATA_PATH_INPUT_FLAG)
        {
            continue;
        }
        APP_PRINT_TRACE2("app_lea_probe_cback: iso %d, 0x%08x", i, iso_elem->iso_conn_handle);
        temp_info.dir = 1;
        temp_info.conn_handle = iso_elem->iso_conn_handle;
        bt_get_le_iso_sync_ref_ap_info(&temp_info);

        if (temp_info.group_anchor_point == 0)
        {
            APP_PRINT_WARN1("app_lea_probe_cback handle %x group_anchor_point invalid",
                            iso_elem->iso_conn_handle);
        }

        if (info.group_anchor_point == 0)
        {
            memcpy(&info, &temp_info, sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO));
        }
        else
        {
            if (info.group_anchor_point != temp_info.group_anchor_point)
            {
                APP_PRINT_WARN4("app_lea_probe_cback handle %x anchor %x handle %x anchor %x",
                                info.conn_handle,
                                info.group_anchor_point,
                                temp_info.conn_handle,
                                temp_info.group_anchor_point);
            }
        }
    }
    app_lea_send_ref(&info, REF_GUARD_TIME_US, session_id);
}
#endif

void app_lea_audio_data_init(void)
{
    os_queue_init(&app_db.iso_input_queue);
    os_queue_init(&app_db.input_path_pending_q);
    os_queue_init(&app_db.iso_output_queue);
    gap_register_direct_cb(app_lea_data_direct_cb);

#if (LE_AUDIO_ISO_REF_CLK == 1)
    audio_probe_dsp_cback_register(app_lea_probe_cback);
#endif
}
#endif
