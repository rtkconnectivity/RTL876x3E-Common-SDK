/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_ACC_MGR_H_
#define _APP_LEA_ACC_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */
#include "os_queue.h"
#include "bt_direct_msg.h"

typedef struct t_iso_data_ind
{
    struct t_iso_data_ind *p_next;
    uint16_t conn_handle;
    uint8_t pkt_status_flag;
    uint32_t time_stamp;
    uint16_t iso_sdu_len;
    uint16_t pkt_seq_num;
    uint8_t *p_buf;
} T_ISO_DATA_IND;

void app_lea_mgr_set_media_state(uint8_t para);

void app_lea_mgr_clear_iso_queue(T_OS_QUEUE *p_queue);
void app_lea_mgr_enqueue_iso_data(T_OS_QUEUE *p_queue, T_BT_DIRECT_ISO_DATA_IND *p_iso_data,
                                  uint16_t output_handle);
T_ISO_DATA_IND *app_lea_mgr_find_iso_elem(T_OS_QUEUE *p_queue,
                                          T_BT_DIRECT_ISO_DATA_IND *p_direct_iso);
void app_lea_mgr_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
