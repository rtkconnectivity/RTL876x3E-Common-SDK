/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#include <stdlib.h>
#include "stdlib_corecrt.h"
#include "os_mem.h"
#include "os_timer.h"
#include "os_sync.h"
#include "gap.h"
#include "gap_br.h"
#include "trace.h"
#include "bt_types.h"
#include "app_timer.h"
#include "transmit_service.h"
#include "app_main.h"
#include "app_transfer.h"
#include "app_cmd.h"
#include "bt_spp.h"
#include "app_sdp.h"

#define UART_TX_QUEUE_NO                    16
#define DT_QUEUE_NO                         16

#define DT_STATUS_IDLE                      0
#define DT_STATUS_ACTIVE                    1

typedef enum
{
    APP_TIMER_UART_ACK,
    APP_TIMER_UART_WAKE_UP,
    APP_TIMER_UART_TX_WAKE_UP,
    APP_TIMER_DATA_TRANSFER,
} T_APP_TRANSFER_TIMER;

typedef struct
{
    uint8_t     *pkt_ptr;
    uint16_t    pkt_len;
    uint8_t     active;
    uint8_t     link_idx;
} T_DT_QUEUE;

typedef struct
{
    uint8_t     dt_queue_w_idx;
    uint8_t     dt_queue_r_idx;
    uint8_t     dt_resend_count;
    uint8_t     dt_status;
} T_DT_QUEUE_CTRL;

static uint8_t app_transfer_timer_id = 0;
static uint8_t timer_idx_data_transfer = 0;

static T_DT_QUEUE_CTRL     dt_queue_ctrl;
static T_DT_QUEUE          dt_queue[DT_QUEUE_NO];

void app_transfer_queue_recv_ack_check(uint16_t event_id, uint8_t cmd_path)
{
    if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_NONE)
    {
        // queue empty
        return;
    }

    uint16_t    tx_queue_id = ((dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[4]) |
                               (dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[5] << 8));

    bool move_to_next = (event_id == tx_queue_id) ? true : false;

    app_transfer_pop_data_queue(cmd_path, move_to_next);
}

void app_transfer_queue_reset(uint8_t cmd_path)
{
    if (cmd_path == CMD_PATH_SPP)
    {
        app_stop_timer(&timer_idx_data_transfer);

        for (uint8_t idx = 0 ; idx < DT_QUEUE_NO; idx++)
        {
            if (dt_queue[idx].active)
            {
                dt_queue[idx].active = 0;

                if (dt_queue[idx].pkt_ptr != NULL)
                {
                    free(dt_queue[idx].pkt_ptr);
                    dt_queue[idx].pkt_ptr = NULL;
                }
            }
        }

        dt_queue_ctrl.dt_queue_w_idx = 0;
        dt_queue_ctrl.dt_queue_r_idx = 0;
        dt_queue_ctrl.dt_resend_count = 0;
        dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
    }
}

void app_transfer_pop_data_queue(uint8_t cmd_path, bool next_flag)
{
    uint8_t     app_idx;
    uint8_t     *pkt_ptr;
    uint16_t    pkt_len;
    uint16_t    event_id;

    APP_PRINT_TRACE2("app_transfer_pop_data_queue: cmd_path %d, next_flag %d", cmd_path, next_flag);

    if (cmd_path == CMD_PATH_SPP)
    {
        if (next_flag == true)
        {
            app_stop_timer(&timer_idx_data_transfer);
            if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active)
            {
                dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;

                if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr != NULL)
                {
                    free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                }

                dt_queue_ctrl.dt_queue_r_idx++;
                if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                {
                    dt_queue_ctrl.dt_queue_r_idx = 0;
                }
            }

            dt_queue_ctrl.dt_resend_count = 0;
            dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
        }

        if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_NONE)
        {
            // queue empty
            return;
        }

        app_idx = dt_queue[dt_queue_ctrl.dt_queue_r_idx].link_idx;
        pkt_ptr = dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr;
        pkt_len = dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_len;
        event_id = ((dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[4]) |
                    (dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[5] << 8));

        APP_PRINT_INFO7("app_transfer_pop_data_queue: dt_status %d, active %d, connected_profile 0x%x, rfc_credit %d, pkt_len:%d, event_id:0x%x, r idx:%d",
                        dt_queue_ctrl.dt_status,
                        dt_queue[dt_queue_ctrl.dt_queue_r_idx].active,
                        app_db.br_link[app_idx].connected_profile,
                        app_db.br_link[app_idx].rfc_credit,
                        pkt_len, event_id, dt_queue_ctrl.dt_queue_r_idx);

        if (dt_queue_ctrl.dt_status == DT_STATUS_IDLE)
        {
            if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_SPP)
            {
                if (app_db.br_link[app_idx].connected_profile & SPP_PROFILE_MASK)
                {
                    if (app_db.br_link[app_idx].rfc_credit)
                    {
                        uint8_t local_server_chann =  RFC_SPP_CHANN_NUM;

                        if (app_db.br_link[app_idx].rtk_vendor_spp_active)
                        {
                            local_server_chann = RFC_RTK_VENDOR_CHANN_NUM;
                        }

                        APP_PRINT_INFO4("app_transfer_pop_data_queue: local_server_chann %d dt_w %d dt_r %d tx_id 0x%x",
                                        local_server_chann,
                                        dt_queue_ctrl.dt_queue_r_idx,
                                        dt_queue_ctrl.dt_queue_w_idx,
                                        dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[1]);

                        if (bt_spp_data_send(app_db.br_link[app_idx].bd_addr, local_server_chann,
                                             pkt_ptr, pkt_len, false) == true)
                        {
                            {
                                if (event_id == EVENT_ACK)
                                {
                                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;
                                    free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                                    dt_queue_ctrl.dt_queue_r_idx++;
                                    if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                                    {
                                        dt_queue_ctrl.dt_queue_r_idx = 0;
                                    }
                                    {

                                        app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                                        app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                                        1);
                                    }
                                }
                                else
                                {
                                    dt_queue_ctrl.dt_status = DT_STATUS_ACTIVE;
                                    app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                                    app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x01, false,
                                                    2000);

                                }
                            }
                        }
                        else
                        {
                            APP_PRINT_TRACE1("send spp fail, app_idx =%d", app_idx);
                            {
                                app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                                app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                                100);
                            }
                        }
                    }
                    else
                    {
                        {
                            free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                            dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                            dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;
                            dt_queue_ctrl.dt_queue_r_idx++;
                            if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                            {
                                dt_queue_ctrl.dt_queue_r_idx = 0;
                            }
                            dt_queue_ctrl.dt_resend_count = 0;
                            app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                            app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                            1);
                        }
                    }
                }
            }
        }
    }
}

bool app_transfer_push_data_queue(uint8_t cmd_path, uint8_t *data, uint16_t data_len,
                                  uint8_t extra_param)
{
    APP_PRINT_TRACE4("app_transfer_push_data_queue: cmd_path %d, data_len %d w_idx %d r_idx %d",
                     cmd_path,
                     data_len,
                     dt_queue_ctrl.dt_queue_w_idx,
                     dt_queue_ctrl.dt_queue_r_idx);

    if (cmd_path == CMD_PATH_SPP)
    {
        if (dt_queue[dt_queue_ctrl.dt_queue_w_idx].active == 0)
        {
            dt_queue[dt_queue_ctrl.dt_queue_w_idx].active = cmd_path;
            dt_queue[dt_queue_ctrl.dt_queue_w_idx].link_idx = extra_param;
            dt_queue[dt_queue_ctrl.dt_queue_w_idx].pkt_ptr = data;
            dt_queue[dt_queue_ctrl.dt_queue_w_idx].pkt_len = data_len;
            dt_queue_ctrl.dt_queue_w_idx++;
            if (dt_queue_ctrl.dt_queue_w_idx == DT_QUEUE_NO)
            {
                dt_queue_ctrl.dt_queue_w_idx = 0;
            }


            uint8_t idx = extra_param;
            uint8_t *bd_addr = app_db.br_link[idx].bd_addr;
            APP_PRINT_TRACE2("app_transfer_push_data_queue: idx %d, bd_addr %s", idx, TRACE_BDADDR(bd_addr));

            app_transfer_pop_data_queue(cmd_path, false);
            return true;
        }
        else
        {
            APP_PRINT_TRACE1("app_transfer_push_data_queue: dt_queue[%d] full", dt_queue_ctrl.dt_queue_w_idx);
            return false;
        }
    }
    return false;
}

static void app_transfer_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_transfer_timeout_cb: timer_id %d, timer_chann %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_DATA_TRANSFER:
        app_stop_timer(&timer_idx_data_transfer);
        dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
        if (param == 0x01) //ack timeout
        {
            APP_PRINT_TRACE0("app_transfer_timeout_cb: ack timeout");
            dt_queue_ctrl.dt_resend_count++;

            if (dt_queue_ctrl.dt_resend_count >= 2)
            {
                app_transfer_pop_data_queue(dt_queue[dt_queue_ctrl.dt_queue_r_idx].active, true);
            }
            else
            {
                app_transfer_pop_data_queue(dt_queue[dt_queue_ctrl.dt_queue_r_idx].active, false);
            }
        }
        else
        {
            app_transfer_pop_data_queue(dt_queue[dt_queue_ctrl.dt_queue_r_idx].active, false);
        }
        break;

    default:
        break;
    }
}

void app_transfer_init(void)
{
    app_timer_reg_cb(app_transfer_timeout_cb, &app_transfer_timer_id);
}



