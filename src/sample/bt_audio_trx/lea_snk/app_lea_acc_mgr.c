/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "syncclk_driver.h"
#include "audio_probe.h"
#include "gap.h"
#include "gap_iso_data.h"
#include "bt_direct_msg.h"
#include "app_lea_acc_adv.h"
#include "app_lea_acc_def.h"
#include "app_lea_acc_broadcast_audio.h"
#include "app_lea_acc_unicast_audio.h"
#include "app_lea_acc_mgr.h"

static T_ISOCH_DATA_PKT_STATUS app_lea_mgr_media_state = ISOCH_DATA_PKT_STATUS_LOST_DATA;
#if F_APP_LE_AUDIO_REF_CLK
static T_SYNCCLK_LATCH_INFO_TypeDef *app_lea_mgr_syncclk_latch_info;
#endif

//For two CISes, pixel sometimes sends 6 packets for one handle and exceeds iso queue size
#define LEA_ISO_QUEUE_MAX       8

#define LEA_ISO_DATA_DELTA_TIMESTAMP 3000

void app_lea_mgr_set_media_state(uint8_t para)
{
    app_lea_mgr_media_state = (T_ISOCH_DATA_PKT_STATUS)para;
}

void app_lea_mgr_clear_iso_queue(T_OS_QUEUE *p_queue)
{
    while (p_queue->count > 0)
    {
        audio_probe_media_buffer_free(os_queue_out(p_queue));
    }
}

void app_lea_mgr_enqueue_iso_data(T_OS_QUEUE *p_queue, T_BT_DIRECT_ISO_DATA_IND *p_iso_data,
                                  uint16_t output_handle)
{
    T_ISO_DATA_IND *p_iso_buf = NULL;

    if (output_handle != p_iso_data->conn_handle)
    {
        APP_PRINT_ERROR2("app_lea_uca_enqueue_iso_data: output handle 0x%x not match data_handle 0x%x error",
                         output_handle,
                         p_iso_data->conn_handle);
        return;
    }

    p_iso_buf = audio_probe_media_buffer_malloc(sizeof(T_ISO_DATA_IND) + p_iso_data->iso_sdu_len);

    if (p_iso_buf == NULL)
    {
        return;
    }

    p_iso_buf->conn_handle = p_iso_data->conn_handle;
    p_iso_buf->time_stamp = p_iso_data->time_stamp;
    p_iso_buf->pkt_seq_num = p_iso_data->pkt_seq_num;
    p_iso_buf->pkt_status_flag = p_iso_data->pkt_status_flag;
    p_iso_buf->p_buf = (uint8_t *)(p_iso_buf + 1);

    if (p_iso_data->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
    {
        memcpy(p_iso_buf->p_buf, p_iso_data->p_buf + p_iso_data->offset, p_iso_data->iso_sdu_len);
        p_iso_buf->iso_sdu_len = p_iso_data->iso_sdu_len;
    }
    else
    {
        p_iso_buf->iso_sdu_len = 0;
    }

    if (p_queue->count > LEA_ISO_QUEUE_MAX)
    {
        T_ISO_DATA_IND *p_iso_elem = (T_ISO_DATA_IND *)os_queue_out(p_queue);

        if (p_iso_elem != NULL)
        {
            audio_probe_media_buffer_free(p_iso_elem);
        }
        APP_PRINT_WARN0("app_lea_uca_enqueue_iso_data: more than LEA_ISO_QUEUE_MAX");
    }

    os_queue_in(p_queue, (void *)p_iso_buf);
    return;
}

static bool app_lea_mgr_is_iso_queue_timestamp_valid(T_BT_DIRECT_ISO_DATA_IND *p_direct_iso,
                                                     T_ISO_DATA_IND *p_iso_elem)
{
    if ((p_direct_iso == NULL) || (p_iso_elem == NULL))
    {
        return false;
    }

    if (p_direct_iso->time_stamp >= p_iso_elem->time_stamp)
    {
        if (p_direct_iso->time_stamp - p_iso_elem->time_stamp < LEA_ISO_DATA_DELTA_TIMESTAMP)
        {
            return true;
        }
        else if (0xFFFFFFFF - p_direct_iso->time_stamp + p_iso_elem->time_stamp <
                 LEA_ISO_DATA_DELTA_TIMESTAMP)
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
        if (p_iso_elem->time_stamp - p_direct_iso->time_stamp < LEA_ISO_DATA_DELTA_TIMESTAMP)
        {
            return true;
        }
        else if (0xFFFFFFFF - p_iso_elem->time_stamp + p_direct_iso->time_stamp <
                 LEA_ISO_DATA_DELTA_TIMESTAMP)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

T_ISO_DATA_IND *app_lea_mgr_find_iso_elem(T_OS_QUEUE *p_queue,
                                          T_BT_DIRECT_ISO_DATA_IND *p_direct_iso)
{
    if ((p_queue == NULL) || (p_direct_iso == NULL))
    {
        return NULL;
    }

    while (p_queue->count)
    {
        T_ISO_DATA_IND *p_iso_elem = (T_ISO_DATA_IND *)os_queue_peek(p_queue, 0);

        if (p_iso_elem != NULL &&
            app_lea_mgr_is_iso_queue_timestamp_valid(p_direct_iso, p_iso_elem))
        {
            return p_iso_elem;
        }
        else
        {
            if (p_iso_elem == NULL)
            {
                APP_PRINT_INFO0("app_lea_mgr_find_iso_elem: not find iso elem");
                return NULL;
            }

            APP_PRINT_WARN6("app_lea_mgr_find_iso_elem: p_iso_time %x, queue_time %x, p_iso_handle 0x%x, queue_handle0x%x, p_iso_seq 0x%x, queue_seq 0x%x",
                            p_direct_iso->time_stamp, p_iso_elem->time_stamp,
                            p_direct_iso->conn_handle, p_iso_elem->conn_handle,
                            p_direct_iso->pkt_seq_num, p_iso_elem->pkt_seq_num);

            if (p_direct_iso->time_stamp > p_iso_elem->time_stamp)
            {
                os_queue_delete(p_queue, p_iso_elem);
                audio_probe_media_buffer_free(p_iso_elem);
            }
            else
            {
                return NULL;
            }
        }
    }

    return NULL;
}

static void app_lea_mgr_direct_cback(uint8_t cb_type, void *p_cb_data)
{
    T_BT_DIRECT_CB_DATA *p_data = (T_BT_DIRECT_CB_DATA *)p_cb_data;

    switch (cb_type)
    {
    case BT_DIRECT_MSG_ISO_DATA_IND:
        {
            if (p_data->p_bt_direct_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
            {
                app_lea_mgr_media_state = p_data->p_bt_direct_iso->pkt_status_flag;
            }

            if (app_lea_mgr_media_state != ISOCH_DATA_PKT_STATUS_VALID_DATA)
            {
                gap_iso_data_cfm(p_data->p_bt_direct_iso->p_buf);
                return;
            }
#if 1
            uint8_t *p_iso_data = p_data->p_bt_direct_iso->p_buf + p_data->p_bt_direct_iso->offset;
#if F_APP_LE_AUDIO_REF_CLK
            app_lea_mgr_syncclk_latch_info = synclk_drv_time_get(LEA_SYNC_CLK_REF);
#endif
            APP_PRINT_INFO6("app_lea_mgr_direct_cback: BT_DIRECT_MSG_ISO_DATA_IND, pkt_status_flag 0x%x, cis_conn_handle 0x%x, pkt_seq_num 0x%x, ts_flag 0x%x, time_stamp 0x%x.media_state=%d",
                            p_data->p_bt_direct_iso->pkt_status_flag, p_data->p_bt_direct_iso->conn_handle,
                            p_data->p_bt_direct_iso->pkt_seq_num, p_data->p_bt_direct_iso->ts_flag,
                            p_data->p_bt_direct_iso->time_stamp,
                            app_lea_mgr_media_state);
            APP_PRINT_INFO5("app_lea_mgr_direct_cback: BT_DIRECT_MSG_ISO_DATA_IND, iso_sdu_len 0x%x, p_buf %p, offset %d, p_data %p, data %b",
                            p_data->p_bt_direct_iso->iso_sdu_len, p_data->p_bt_direct_iso->p_buf,
                            p_data->p_bt_direct_iso->offset, p_iso_data, TRACE_BINARY(p_data->p_bt_direct_iso->iso_sdu_len,
                                                                                      p_iso_data));
#if F_APP_LE_AUDIO_REF_CLK
            APP_PRINT_INFO4("app_lea_mgr_direct_cback: syncclk latch result net_id 0x%x, conn_type 0x%x role 0x%x exp_sync_clock 0x%x",
                            app_lea_mgr_syncclk_latch_info->net_id,
                            app_lea_mgr_syncclk_latch_info->conn_type,
                            app_lea_mgr_syncclk_latch_info->role,
                            app_lea_mgr_syncclk_latch_info->exp_sync_clock);
#endif
#endif
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
            if (app_lea_uca_get_stream_link())
            {
                app_lea_uca_handle_iso_data(p_data);
            }
            else
#endif
            {
#if F_APP_TMAP_BMR_SUPPORT
                app_lea_bca_handle_iso_data(p_data);
#endif
            }
            gap_iso_data_cfm(p_data->p_bt_direct_iso->p_buf);
        }
        break;

    default:
        APP_PRINT_ERROR1("app_lea_mgr_direct_cback: unhandled cb_type 0x%x", cb_type);
        break;
    }
}


void app_lea_mgr_init(void)
{
    gap_register_direct_cb(app_lea_mgr_direct_cback);

#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT
    app_lea_adv_init();
#endif
#if F_APP_TMAP_BMR_SUPPORT
    app_lea_scan_init();
#endif
}
#endif
