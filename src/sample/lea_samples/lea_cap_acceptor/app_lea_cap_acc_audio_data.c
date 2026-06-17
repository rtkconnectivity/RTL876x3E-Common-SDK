/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "os_queue.h"
#include "stdlib.h"
#include "trace.h"
#include "gap.h"
#include "gap_iso_data.h"
#include "bt_direct_msg.h"
#include "app_lea_cap_acc_main.h"
#include "app_lea_cap_acc_audio_data.h"
#include "ble_audio_def.h"
#include "codec_qos.h"
#if APP_LEA_INPUT_AUDIO_DATA_TEST
#include "app_timer.h"
#endif

#if APP_LEA_INPUT_AUDIO_DATA_TEST
typedef enum
{
    APP_LEA_SEND_ISO_DATA = 0x00,
} T_APP_LEA_TIMER;

static uint8_t app_lea_timer_idx_send_iso_data = 0;
static uint8_t app_lea_timer_id = 0;

const uint8_t lc3_16_2_encoded_data[40] = {0x46, 0xC2, 0x6C, 0x27, 0x41, 0x23, 0x1E, 0x02, 0xE6, 0x7A, 0x04,
                                           0xAB, 0x54, 0x34, 0xFB, 0x6A, 0x0A, 0x24, 0x02, 0x8A, 0xFA, 0x00,
                                           0x00, 0x39, 0x4F, 0x96, 0x0D, 0x38, 0xA7, 0x57, 0xE2, 0x24, 0x56,
                                           0x5F, 0xF1, 0xCB, 0x97, 0x20, 0xF8, 0x9B
                                          };
const uint8_t lc3_16_2_2c_encoded_data[80] = {0x46, 0xC2, 0x6C, 0x27, 0x41, 0x23, 0x1E, 0x02, 0xE6, 0x7A, 0x04,
                                              0xAB, 0x54, 0x34, 0xFB, 0x6A, 0x0A, 0x24, 0x02, 0x8A, 0xFA, 0x00,
                                              0x00, 0x39, 0x4F, 0x96, 0x0D, 0x38, 0xA7, 0x57, 0xE2, 0x24, 0x56,
                                              0x5F, 0xF1, 0xCB, 0x97, 0x20, 0xF8, 0x9B,
                                              0x46, 0xC2, 0x6C, 0x27, 0x41, 0x23, 0x1E, 0x02, 0xE6, 0x7A, 0x04,
                                              0xAB, 0x54, 0x34, 0xFB, 0x6A, 0x0A, 0x24, 0x02, 0x8A, 0xFA, 0x00,
                                              0x00, 0x39, 0x4F, 0x96, 0x0D, 0x38, 0xA7, 0x57, 0xE2, 0x24, 0x56,
                                              0x5F, 0xF1, 0xCB, 0x97, 0x20, 0xF8, 0x9B
                                             };
const uint8_t lc3_16_1_encoded_data[30] = {0x1D,  0x92,  0xB3,  0x67,  0xDE,  0xF5,  0xD0,  0x46,  0xEB,  0xC7,  0xDD,  0x69,  0x89,  0xD8,
                                           0xEA,  0xD5,  0x42,  0x22,  0x4C,  0xBB,  0x53,  0xF9,  0x0D,  0xED,  0xD8,  0xE5,  0x3C,  0xA0,
                                           0xCA,  0x57
                                          };
void app_lea_test_iso_data_send(void)
{
    T_GAP_CAUSE cause = GAP_CAUSE_SUCCESS;
    T_APP_LEA_ISO_CHANN *p_iso_chann = NULL;

    for (uint8_t i = 0; i < app_db.iso_input_queue.count; i++)
    {
        p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_input_queue, i);
        if (p_iso_chann->iso_sdu_len != 0)
        {
            cause = gap_iso_send_data((uint8_t *)p_iso_chann->p_encoded_data,
                                      p_iso_chann->iso_conn_handle,
                                      p_iso_chann->iso_sdu_len,
                                      false, (p_iso_chann->pkt_seq_num * p_iso_chann->sdu_interval),
                                      p_iso_chann->pkt_seq_num);
            if (cause != GAP_CAUSE_SUCCESS)
            {
                goto error;
            }
        }
        p_iso_chann->pkt_seq_num++;
    }
    return;
error:
    APP_PRINT_ERROR1("app_lea_test_iso_data_send: failed, cause 0x%x", cause);
    return;
}

static void app_lea_timer_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_LEA_SEND_ISO_DATA:
        {
            app_lea_test_iso_data_send();
        }
        break;

    default:
        break;
    }
}

static void app_lea_test_start_iso_data_timer(uint32_t timeout_ms)
{
    if (app_lea_timer_id == 0)
    {
        app_timer_reg_cb(app_lea_timer_timeout_cb, &app_lea_timer_id);
    }
    app_start_timer(&app_lea_timer_idx_send_iso_data, "iso_data",
                    app_lea_timer_id, APP_LEA_SEND_ISO_DATA, 0, true,
                    timeout_ms);
}


void app_lea_test_iso_chann_remove(void)
{
    APP_PRINT_INFO1("app_lea_test_iso_chann_remove: iso_input_queue %d",
                    app_db.iso_input_queue.count);
    if (app_db.iso_input_queue.count == 0)
    {
        app_stop_timer(&app_lea_timer_idx_send_iso_data);
    }
}

void app_lea_test_iso_chann_add(T_APP_LEA_ISO_CHANN *p_iso_chann)
{
    uint8_t err_idx = 0;

    codec_max_sdu_len_get(&p_iso_chann->codec_data, &p_iso_chann->iso_sdu_len);
    codec_sdu_interval_get(&p_iso_chann->codec_data, &p_iso_chann->sdu_interval);
    p_iso_chann->pkt_seq_num = 1;

    if (p_iso_chann->codec_data.sample_frequency == SAMPLING_FREQUENCY_CFG_16K)
    {
        if (p_iso_chann->codec_data.frame_duration == FRAME_DURATION_CFG_10_MS)
        {
            if (p_iso_chann->iso_sdu_len == sizeof(lc3_16_2_encoded_data))
            {
                p_iso_chann->p_encoded_data = lc3_16_2_encoded_data;
            }
            else if (p_iso_chann->iso_sdu_len == sizeof(lc3_16_2_2c_encoded_data))
            {
                p_iso_chann->p_encoded_data = lc3_16_2_2c_encoded_data;
            }
            else
            {
                err_idx = 3;
                goto failed;
            }
        }
        else if (p_iso_chann->codec_data.frame_duration == FRAME_DURATION_CFG_7_5_MS)
        {
            if (p_iso_chann->iso_sdu_len == sizeof(lc3_16_1_encoded_data))
            {
                p_iso_chann->p_encoded_data = lc3_16_1_encoded_data;
            }
            else
            {
                err_idx = 4;
                goto failed;
            }
        }
    }
    else
    {
        err_idx = 10;
        goto failed;
    }
    APP_PRINT_INFO2("app_lea_test_iso_chann_add: sdu %d, interval %d", p_iso_chann->iso_sdu_len,
                    p_iso_chann->sdu_interval / 1000);

    app_lea_test_start_iso_data_timer(p_iso_chann->sdu_interval / 1000);

    return;
failed:
    APP_PRINT_ERROR1("app_lea_test_iso_chann_add: failed err_idx %d", err_idx);
    return;
}
#endif

T_APP_LEA_ISO_CHANN *app_lea_find_iso_chann(uint16_t iso_conn_handle, uint8_t direction)
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

static void app_lea_remove_iso_chann(uint16_t iso_conn_handle, uint8_t direction)
{
    int i;
    T_APP_LEA_ISO_CHANN *p_iso_chann = NULL;
    APP_PRINT_INFO2("app_lea_remove_iso_chann: iso_conn_handle 0x%x, direction %d",
                    iso_conn_handle, direction);
    if (direction == DATA_PATH_INPUT_FLAG)
    {
        for (i = 0; i < app_db.iso_input_queue.count; i++)
        {
            p_iso_chann = (T_APP_LEA_ISO_CHANN *)os_queue_peek(&app_db.iso_input_queue, i);
            if (p_iso_chann->iso_conn_handle == iso_conn_handle)
            {
                os_queue_delete(&app_db.iso_input_queue, p_iso_chann);
                free(p_iso_chann);
#if APP_LEA_INPUT_AUDIO_DATA_TEST
                app_lea_test_iso_chann_remove();
#endif
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


void app_lea_handle_data_path_setup(T_LEA_SETUP_DATA_PATH *p_data)
{
    T_APP_LEA_ISO_CHANN *p_iso_chann = app_lea_find_iso_chann(p_data->iso_conn_handle,
                                                              p_data->path_direction);
    uint8_t chnl_cnt;
    uint8_t blocks_num = 1;

    if (p_iso_chann != NULL)
    {
        APP_PRINT_WARN0("app_lea_handle_data_path_setup: iso channel already exist");
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
    p_iso_chann->pkt_seq_num = 0;
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
    p_iso_chann->frame_num = blocks_num * chnl_cnt;

    APP_PRINT_INFO6("app_lea_handle_data_path_setup: cis handle 0x%04x, frame_num %d, "
                    "dir %u, sample_frequency 0x%x, audio_channel_allocation 0x%08x, presentation_delay 0x%x",
                    p_iso_chann->iso_conn_handle, p_iso_chann->frame_num,
                    p_iso_chann->path_direction,
                    p_iso_chann->codec_data.sample_frequency,
                    p_iso_chann->codec_data.audio_channel_allocation,
                    p_iso_chann->presentation_delay);
#if APP_LEA_INPUT_AUDIO_DATA_TEST
    if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
    {
        app_lea_test_iso_chann_add(p_iso_chann);
    }
#endif
}

void app_lea_handle_data_path_remove(T_LEA_REMOVE_DATA_PATH *p_data)
{
    T_APP_LEA_ISO_CHANN *p_iso_chann = app_lea_find_iso_chann(p_data->iso_conn_handle,
                                                              p_data->path_direction);
    if (p_iso_chann != NULL)
    {
        app_lea_remove_iso_chann(p_data->iso_conn_handle, p_data->path_direction);
    }
}

void app_lea_data_direct_cb(uint8_t cb_type, void *p_cb_data)
{
    T_BT_DIRECT_CB_DATA *p_data = (T_BT_DIRECT_CB_DATA *)p_cb_data;

    switch (cb_type)
    {
    case BT_DIRECT_MSG_ISO_DATA_IND:
        {
#if 0
            APP_PRINT_TRACE5("app_lea_data_direct_cb: conn_handle 0x%x, iso_sdu_len %d, pkt_seq_num 0x%x, time_stamp 0x%x, pkt_status_flag 0x%x",
                             p_data->p_bt_direct_iso->conn_handle, p_data->p_bt_direct_iso->iso_sdu_len,
                             p_data->p_bt_direct_iso->pkt_seq_num, p_data->p_bt_direct_iso->time_stamp,
                             p_data->p_bt_direct_iso->pkt_status_flag);
#endif
#if 0
            uint8_t *p_iso_data = p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset;
            APP_PRINT_INFO5("app_audio_data_direct_callback: BT_DIRECT_MSG_ISO_DATA_IND, iso_sdu_len 0x%x, p_buf %p, offset %d, p_data %p, data %b",
                            p_data->p_bt_direct_iso->iso_sdu_len, p_data->p_bt_direct_iso->p_buf,
                            p_data->p_bt_direct_iso->offset, p_iso_data, TRACE_BINARY(p_data->p_bt_direct_iso->iso_sdu_len,
                                                                                      p_iso_data));
#endif

            if (p_data->p_bt_direct_iso->pkt_status_flag != ISOCH_DATA_PKT_STATUS_VALID_DATA)
            {
                gap_iso_data_cfm(p_data->p_bt_direct_iso->p_buf);
                break;
            }

            T_APP_LEA_ISO_CHANN *p_iso_chann = app_lea_find_iso_chann(p_data->p_bt_direct_iso->conn_handle,
                                                                      DATA_PATH_OUTPUT_FLAG);
            if (p_iso_chann != NULL)
            {
#if 0
                //Just for sample. Application can send the data to DSP.
                uint16_t written_len;
                T_AUDIO_STREAM_STATUS status;

                if (p_data->p_bt_direct_iso->iso_sdu_len != 0)
                {
                    status = AUDIO_STREAM_STATUS_CORRECT;
                }
                else
                {
                    status = AUDIO_STREAM_STATUS_LOST;
                }
                audio_track_write(handle, p_data->p_bt_direct_iso->time_stamp,
                                  p_data->p_bt_direct_iso->pkt_seq_num,
                                  status,
                                  p_iso_chann->frame_num,
                                  p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset,
                                  p_data->p_bt_direct_iso->iso_sdu_len,
                                  &written_len);
#endif
            }

            gap_iso_data_cfm(p_data->p_bt_direct_iso->p_buf);
        }
        break;

    default:
        APP_PRINT_ERROR1("app_lea_data_direct_cb: unhandled cb_type 0x%x", cb_type);
        break;
    }
}

void app_lea_audio_data_init(void)
{
    os_queue_init(&app_db.iso_input_queue);
    os_queue_init(&app_db.iso_output_queue);
    gap_register_direct_cb(app_lea_data_direct_cb);
}

