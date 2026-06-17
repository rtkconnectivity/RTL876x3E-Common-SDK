/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include "stdlib.h"
#include "os_timer.h"
#include "os_sync.h"
#include "console.h"
#include "gap.h"
#include "gap_br.h"
#include "trace.h"
#include "bt_types.h"
#include "btm.h"
#include "hal_gpio.h"
#include "app_timer.h"
#include "transmit_service.h"
#include "app_transfer_cfg.h"

#if F_APP_IAP_SUPPORT
#include "bt_iap.h"
#include "app_iap.h"
#include "app_iap_rtk.h"
#endif

#include "app_main.h"
#include "app_dlps.h"
#include "app_sdp.h"
#include "bt_spp.h"
#include "app_ble_service.h"
#include "app_cmd.h"

#include "app_cfg.h"
#include "app_transfer.h"

#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 1)
#include "console_uart.h"
#endif

#define UART_TX_QUEUE_NO                    40
#define DT_QUEUE_NO                         16

#define DT_STATUS_IDLE                      0
#define DT_STATUS_ACTIVE                    1

#define DT_TRANS_POLL                       5

typedef enum
{
#if F_APP_ENABLE_TWO_ONE_WIRE_UART
    APP_TIMER_UART0_ACK,
    APP_TIMER_UART2_ACK,
#else
    APP_TIMER_UART_ACK,
#endif
    APP_TIMER_UART_WAKE_UP,
    APP_TIMER_UART_TX_WAKE_UP,
    APP_TIMER_DATA_TRANSFER,
    APP_TRANSFER_TIMER_CHECK_CONSOLE_COMP,
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

typedef struct
{
    uint8_t     *packet_ptr;
    uint16_t    packet_len;
    uint8_t     active;
    uint8_t     extra_param;
} T_UART_TX_QUEUE;

static uint8_t app_transfer_timer_id = 0;

#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0)
static uint8_t timer_idx_uart_ack = 0;
#else
static uint8_t timer_idx_uart0_ack = 0;
static uint8_t timer_idx_uart2_ack = 0;
#endif
static uint8_t timer_idx_uart_wake_up = 0;
static uint8_t timer_idx_uart_tx_wake_up = 0;
static uint8_t timer_idx_data_transfer = 0;
static uint8_t timer_handle_console_send_comp = 0;

static T_DT_QUEUE_CTRL     dt_queue_ctrl;
static T_DT_QUEUE          dt_queue[DT_QUEUE_NO];

#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0)
static T_UART_TX_QUEUE uart_tx_queue[UART_TX_QUEUE_NO];
static uint8_t uart_tx_rIndex;                 /**<uart transfer packet read index*/
static uint8_t uart_tx_wIndex;                 /**<uart transfer packet write index*/
static uint8_t uart_resend_count;              /**<uart resend count*/
uint8_t uart_tx_status = DT_STATUS_IDLE;
uint32_t uart_console_write_comp = 1;
#else
static T_UART_TX_QUEUE uart0_tx_queue[UART_TX_QUEUE_NO];
static T_UART_TX_QUEUE uart2_tx_queue[UART_TX_QUEUE_NO];
static uint8_t uart0_tx_rIndex = 0;                 /**<uart transfer packet read index*/
static uint8_t uart0_tx_wIndex = 0;                 /**<uart transfer packet write index*/
static uint8_t uart2_tx_rIndex = 0;                 /**<uart transfer packet read index*/
static uint8_t uart2_tx_wIndex = 0;                 /**<uart transfer packet write index*/
static uint8_t uart0_resend_count = 0;              /**<uart resend count*/
static uint8_t uart2_resend_count = 0;              /**<uart resend count*/
uint8_t uart0_tx_status = DT_STATUS_IDLE;
uint8_t uart2_tx_status = DT_STATUS_IDLE;
uint32_t uart0_console_write_comp = 1;
uint32_t uart2_console_write_comp = 1;
#endif

// for CMD_LEGACY_DATA_TRANSFER and CMD_LE_DATA_TRANSFER
static uint8_t *uart_rx_dt_pkt_ptr = NULL;
static uint16_t uart_rx_dt_pkt_len;

void app_transfer_queue_recv_ack_check(uint16_t event_id, uint8_t cmd_path)
{
    uint16_t    tx_queue_id = ((dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[4]) |
                               (dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[5] << 8));

    bool move_to_next = (event_id == tx_queue_id) ? true : false;

    app_pop_data_transfer_queue(cmd_path, move_to_next, 0xff);
}

void app_transfer_queue_reset(uint8_t cmd_path)
{
    if (cmd_path == CMD_PATH_SPP || cmd_path == CMD_PATH_LE || cmd_path == CMD_PATH_IAP ||
        cmd_path == CMD_PATH_GATT_OVER_BREDR)
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

void app_pop_data_transfer_queue(uint8_t cmd_path, bool next_flag, uint8_t index)
{
    T_UART_TX_QUEUE *p_queue;
    uint8_t     app_idx;
    uint8_t     *pkt_ptr;
    uint16_t    pkt_len;
    uint16_t    event_id;

    APP_PRINT_TRACE2("app_pop_data_transfer_queue: cmd_path %d, next_flag %d", cmd_path, next_flag);

    if (CMD_PATH_UART == cmd_path)
    {
#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0)
        T_UART_TX_QUEUE *tx_queue;

        if (next_flag)
        {
            app_stop_timer(&timer_idx_uart_ack);
            app_stop_timer(&timer_handle_console_send_comp);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_UART_ACK);
            tx_queue = &(uart_tx_queue[uart_tx_rIndex]);
            if (tx_queue->active)
            {
                event_id = ((tx_queue->packet_ptr[4]) | (tx_queue->packet_ptr[5] << 8));

                if (event_id == EVENT_LEGACY_DATA_TRANSFER || event_id == EVENT_XM_SPP_DATA_TRANSFER)
                {
                    uint8_t app_index;
                    uint8_t local_server_chann =  RFC_SPP_CHANN_NUM;


                    app_index = tx_queue->packet_ptr[6];
                    if (event_id == EVENT_XM_SPP_DATA_TRANSFER)
                    {
                        local_server_chann =  RFC_SPP_CUSTOMER_NUM;
                    }
                    if (app_db.br_link[app_index].vendor_spp.is_active)
                    {
                        local_server_chann = RFC_RTK_VENDOR_CHANN_NUM;
                    }
                    bt_spp_credits_give(app_db.br_link[app_index].bd_addr, local_server_chann, 1);

                }
                free(tx_queue->packet_ptr);
                tx_queue->active = 0;
                uart_tx_rIndex++;
                if (uart_tx_rIndex >= UART_TX_QUEUE_NO)
                {
                    uart_tx_rIndex = 0;
                }
            }
            uart_resend_count = 0;
            uart_tx_status = DT_STATUS_IDLE;
        }

        p_queue = &(uart_tx_queue[uart_tx_rIndex]);
        if (p_queue->active)
        {
            //app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_TX);
            if (app_cfg_const.enable_tx_wake_up)
            {
                hal_gpio_set_level(app_cfg_const.tx_wake_up_pinmux, GPIO_LEVEL_LOW);
                app_start_timer(&timer_idx_uart_tx_wake_up, "uart_tx_wake_up",
                                app_transfer_timer_id, APP_TIMER_UART_TX_WAKE_UP, 0, false,
                                10);
            }
            else
            {
                if (uart_tx_status == DT_STATUS_IDLE)
                {
#if F_APP_CONSOLE_SUPPORT
                    uint32_t s = os_lock();
                    uart_console_write_comp = console_write(p_queue->packet_ptr, p_queue->packet_len);
                    os_unlock(s);
#endif

                    event_id = ((p_queue->packet_ptr[4]) | (p_queue->packet_ptr[5] << 8));
                    if (((event_id == EVENT_ACK) && (app_transfer_cfg.report_uart_event_only_once == 0)) ||
                        app_transfer_cfg.report_uart_event_only_once ||
                        (event_id == EVENT_LEGACY_DATA_TRANSFER) ||
                        (event_id == EVENT_XM_SPP_DATA_TRANSFER) ||
                        (event_id == EVENT_XM_RECORDING_DATA)
                       )
                    {
                        if (uart_console_write_comp)
                        {
                            app_pop_data_transfer_queue(CMD_PATH_UART, true, 0);
                        }
                        else
                        {
                            uart_tx_status = DT_STATUS_ACTIVE;
                            app_start_timer(&timer_handle_console_send_comp, "uart_console_comp", app_transfer_timer_id,
                                            APP_TRANSFER_TIMER_CHECK_CONSOLE_COMP, event_id, false, 10);
                        }
                    }
                    else if (event_id == EVENT_MAP_FOLDER_LISTING_DATA_CONTINUE ||
                             event_id == EVENT_MAP_FOLDER_LISTING_DATA_END ||
                             event_id == EVENT_MAP_MSG_LISTING_DATA_CONTINUE ||
                             event_id == EVENT_MAP_MSG_LISTING_DATA_END ||
                             event_id == EVENT_MAP_MSG_DATA_CONTINUE ||
                             event_id == EVENT_MAP_MSG_DATA_END)
                    {
                        APP_PRINT_ERROR2("map console_write: %p, %d", p_queue->packet_ptr, p_queue->packet_len);
                        app_pop_data_transfer_queue(CMD_PATH_UART, true, 0);
                    }
                    else
                    {
                        //wait ack or timeout
                        uart_tx_status = DT_STATUS_ACTIVE;
                        app_start_timer(&timer_idx_uart_ack, "uart_ack",
                                        app_transfer_timer_id, APP_TIMER_UART_ACK, event_id, false, 800);
                        app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_ACK);
                    }
                }
            }
        }
        else
        {
            if (app_cfg_const.enable_tx_wake_up)
            {
                hal_gpio_set_level(app_cfg_const.tx_wake_up_pinmux, GPIO_LEVEL_HIGH);
            }
        }
#else
        T_UART_TX_QUEUE *tx_queue;

        if (index == 0)
        {
            if (next_flag)
            {
                app_stop_timer(&timer_idx_uart0_ack);
                app_stop_timer(&timer_handle_console_send_comp);
                app_dlps_enable(APP_DLPS_ENTER_CHECK_UART_ACK);
                tx_queue = &(uart0_tx_queue[uart0_tx_rIndex]);
                if (tx_queue->active)
                {
                    event_id = ((tx_queue->packet_ptr[4]) | (tx_queue->packet_ptr[5] << 8));
                    free(tx_queue->packet_ptr);
                    tx_queue->active = 0;
                    uart0_tx_rIndex++;
                    if (uart0_tx_rIndex >= UART_TX_QUEUE_NO)
                    {
                        uart0_tx_rIndex = 0;
                    }
                }
                uart0_resend_count = 0;
                uart0_tx_status = DT_STATUS_IDLE;
            }

            p_queue = &(uart0_tx_queue[uart0_tx_rIndex]);
            if (p_queue->active)
            {
                //app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_TX);
                if (app_cfg_const.enable_tx_wake_up)
                {
                    hal_gpio_set_level(app_cfg_const.tx_wake_up_pinmux, GPIO_LEVEL_LOW);
                    app_start_timer(&timer_idx_uart_tx_wake_up, "uart_tx_wake_up",
                                    app_transfer_timer_id, APP_TIMER_UART_TX_WAKE_UP, 0, false,
                                    10);
                }
                else
                {
                    if (uart0_tx_status == DT_STATUS_IDLE)
                    {
#if F_APP_CONSOLE_SUPPORT
                        uart0_console_write_comp = console_uart_write(p_queue->packet_ptr, p_queue->packet_len, 0);
#endif

                        event_id = ((p_queue->packet_ptr[4]) | (p_queue->packet_ptr[5] << 8));
                        if (((event_id == EVENT_ACK) && (app_transfer_cfg.report_uart_event_only_once == 0)) ||
                            app_transfer_cfg.report_uart_event_only_once)
                        {
                            if (uart0_console_write_comp)
                            {
                                app_pop_data_transfer_queue(CMD_PATH_UART, true, 0);
                            }
                        }
                        else
                        {
                            //wait ack or timeout
                            uart0_tx_status = DT_STATUS_ACTIVE;
                            app_start_timer(&timer_idx_uart0_ack, "uart_ack",
                                            app_transfer_timer_id, APP_TIMER_UART0_ACK, event_id, false, 800);
                            app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_ACK);
                        }
                    }
                }
            }
            else
            {
                if (app_cfg_const.enable_tx_wake_up)
                {
                    hal_gpio_set_level(app_cfg_const.tx_wake_up_pinmux, GPIO_LEVEL_HIGH);
                }
            }
        }
        else if (index == 2)
        {
            if (next_flag)
            {
                app_stop_timer(&timer_idx_uart2_ack);
                app_stop_timer(&timer_handle_console_send_comp);
                app_dlps_enable(APP_DLPS_ENTER_CHECK_UART_ACK);
                tx_queue = &(uart2_tx_queue[uart2_tx_rIndex]);
                if (tx_queue->active)
                {
                    event_id = ((tx_queue->packet_ptr[4]) | (tx_queue->packet_ptr[5] << 8));
                    free(tx_queue->packet_ptr);
                    tx_queue->active = 0;
                    uart2_tx_rIndex++;
                    if (uart2_tx_rIndex >= UART_TX_QUEUE_NO)
                    {
                        uart2_tx_rIndex = 0;
                    }
                }
                uart2_resend_count = 0;
                uart2_tx_status = DT_STATUS_IDLE;
            }

            p_queue = &(uart2_tx_queue[uart2_tx_rIndex]);
            if (p_queue->active)
            {
                //app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_TX);
                if (app_cfg_const.enable_tx_wake_up)
                {
                    hal_gpio_set_level(app_cfg_const.tx_wake_up_pinmux, GPIO_LEVEL_LOW);
                    app_start_timer(&timer_idx_uart_tx_wake_up, "uart_tx_wake_up",
                                    app_transfer_timer_id, APP_TIMER_UART_TX_WAKE_UP, 0, false,
                                    10);
                }
                else
                {
                    if (uart2_tx_status == DT_STATUS_IDLE)
                    {
#if F_APP_CONSOLE_SUPPORT
                        uart2_console_write_comp = console_uart_write(p_queue->packet_ptr, p_queue->packet_len, 2);
#endif
                        event_id = ((p_queue->packet_ptr[4]) | (p_queue->packet_ptr[5] << 8));
                        if (((event_id == EVENT_ACK) && (app_transfer_cfg.report_uart_event_only_once == 0)) ||
                            app_transfer_cfg.report_uart_event_only_once)
                        {
                            if (uart2_console_write_comp)
                            {
                                app_pop_data_transfer_queue(CMD_PATH_UART, true, 2);
                            }
                        }
                        else
                        {
                            //wait ack or timeout
                            uart2_tx_status = DT_STATUS_ACTIVE;
                            app_start_timer(&timer_idx_uart2_ack, "uart_ack",
                                            app_transfer_timer_id, APP_TIMER_UART2_ACK, event_id, false, 800);
                            app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_ACK);
                        }
                    }
                }
            }
            else
            {
                if (app_cfg_const.enable_tx_wake_up)
                {
                    hal_gpio_set_level(app_cfg_const.tx_wake_up_pinmux, GPIO_LEVEL_HIGH);
                }
            }
        }
#endif
    }
    else
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
            else
            {
                return;
            }
            dt_queue_ctrl.dt_resend_count = 0;
            dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
        }

        app_idx = dt_queue[dt_queue_ctrl.dt_queue_r_idx].link_idx;
        pkt_ptr = dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr;
        pkt_len = dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_len;
        event_id = ((dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[4]) |
                    (dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[5] << 8));

        APP_PRINT_INFO8("app_pop_data_transfer_queue: dt_status %d, active %d, connected_profile 0x%x, tx_mask 0x%x, rfc_credit %d, pkt_len:%d, event_id:0x%x, r idx:%d",
                        dt_queue_ctrl.dt_status,
                        dt_queue[dt_queue_ctrl.dt_queue_r_idx].active,
                        app_db.br_link[app_idx].connected_profile,
                        app_db.br_link[app_idx].cmd.tx_mask,
                        app_db.br_link[app_idx].vendor_spp.credit,
                        pkt_len, event_id, dt_queue_ctrl.dt_queue_r_idx);

        if (dt_queue_ctrl.dt_status == DT_STATUS_IDLE)
        {
            if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_IAP)
            {
#if (F_APP_IAP_RTK_SUPPORT && F_APP_IAP_SUPPORT)
                T_APP_IAP_HDL app_iap_hdl = NULL;
                APP_PRINT_INFO2("app_pop_data_transfer_queue: iap credit %d, rtk iap connected %d",
                                app_iap_get_credit(app_iap_hdl), app_iap_rtk_connected(app_db.br_link[app_idx].bd_addr));

                if (app_db.br_link[app_idx].connected_profile & IAP_PROFILE_MASK)
                {
                    if (app_iap_rtk_connected(app_db.br_link[app_idx].bd_addr))
                    {
                        if (app_iap_rtk_send(app_db.br_link[app_idx].bd_addr,
                                             pkt_ptr, pkt_len) == true)
                        {
                            if (app_transfer_cfg.enable_embedded_cmd)
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

                                    app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                                    app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                                    1);
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
                            app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                            app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                            100);
                        }
                    }
                    else
                    {
                        if (app_iap_is_authened(app_iap_hdl))
                        {
                            app_iap_rtk_launch(app_db.br_link[app_idx].bd_addr, BT_IAP_APP_LAUNCH_WITH_USER_ALERT);
                        }
                        else
                        {
                            free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                            dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                            dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;
                            dt_queue_ctrl.dt_queue_r_idx++;
                            if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                            {
                                dt_queue_ctrl.dt_queue_r_idx = 0;
                            }
                            app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                            app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                            1);
                        }
                    }
                }
#endif
            }
            else if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_SPP)
            {
                if (app_db.br_link[app_idx].connected_profile & SPP_PROFILE_MASK)
                {
                    if (app_db.br_link[app_idx].vendor_spp.credit)
                    {
                        uint8_t local_server_chann =  RFC_SPP_CHANN_NUM;

                        if (app_db.br_link[app_idx].vendor_spp.is_active)
                        {
                            local_server_chann = RFC_RTK_VENDOR_CHANN_NUM;
                        }

                        APP_PRINT_INFO4("app_pop_data_transfer_queue: local_server_chann %d dt_w %d dt_r %d tx_id %d",
                                        local_server_chann,
                                        dt_queue_ctrl.dt_queue_r_idx,
                                        dt_queue_ctrl.dt_queue_w_idx,
                                        dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[1]);

                        if (bt_spp_data_send(app_db.br_link[app_idx].bd_addr, local_server_chann,
                                             pkt_ptr, pkt_len, false) == true)
                        {
                            app_db.br_link[app_idx].vendor_spp.credit--;

                            if (app_transfer_cfg.enable_embedded_cmd)
                            {
                                if ((event_id == EVENT_ACK)
                                   )
                                {
                                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;
                                    free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                                    dt_queue_ctrl.dt_queue_r_idx++;
                                    if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                                    {
                                        dt_queue_ctrl.dt_queue_r_idx = 0;
                                    }

                                    app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                                    app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                                    1);
                                }
                                else
                                {
#if F_APP_DATA_CAPTURE_SUPPORT
                                    if ((event_id == EVENT_DATA_CAPTURE_START_STOP) ||
                                        (event_id == EVENT_DATA_CAPTURE_DATA) ||
                                        (event_id == EVENT_DATA_CAPTURE_ENTER_EXIT))
                                    {

                                        app_pop_data_transfer_queue(CMD_PATH_SPP, true, 0xff);
                                    }
                                    else
#endif
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
                                app_pop_data_transfer_queue(CMD_PATH_SPP, true, 0xff);
                            }
                        }
                        else
                        {
                            APP_PRINT_TRACE1("send spp fail, app_idx =%d", app_idx);
#if F_APP_DATA_CAPTURE_SUPPORT
                            if ((event_id == EVENT_DATA_CAPTURE_START_STOP) ||
                                (event_id == EVENT_DATA_CAPTURE_DATA) ||
                                (event_id == EVENT_DATA_CAPTURE_ENTER_EXIT) ||
                                (event_id == EVENT_ACK))
                            {
                                app_pop_data_transfer_queue(CMD_PATH_SPP, true, 0xff);
                            }
                            else
#endif
                            {
                                app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                                app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                                100);
                            }
                        }
                    }
                    else
                    {
                        dt_queue_ctrl.dt_status = DT_STATUS_ACTIVE;
                        app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                        app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x01, false,
                                        DT_TRANS_POLL);
                    }
                }
            }
            else if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_LE)
            {
                if (app_db.le_link[app_idx].state == LE_LINK_STATE_CONNECTED)
                {
                    if (app_db.le_link[app_idx].cmd.tx_mask == TX_ENABLE_READY)
                    {
                        uint16_t cid;
                        uint8_t cid_num;

                        gap_chann_get_cid(app_db.le_link[app_idx].conn_handle, 1, &cid, &cid_num);

                        if (transmit_srv_tx_data(app_db.le_link[app_idx].conn_handle, cid, pkt_len, pkt_ptr) == true)
                        {
                            dt_queue_ctrl.dt_status = DT_STATUS_ACTIVE;
                            if (app_transfer_cfg.enable_embedded_cmd)
                            {
                                if (event_id != EVENT_ACK)
                                {
                                    app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                                    app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x01, false,
                                                    2000);
                                }
                            }
                        }
                        else
                        {
                            app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                            app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                            100);
                        }
                    }
                    else
                    {
                        app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                        app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                        1000);
                    }
                }
                else
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
                    //set timer to pop queue
                    app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                    app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                    1);
                }
            }
            else if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_GATT_OVER_BREDR)
            {
                if (app_db.br_link[app_idx].connected_profile & GATT_PROFILE_MASK)
                {
                    if (app_db.br_link[app_idx].cmd.tx_mask == TX_ENABLE_READY)
                    {
                        uint16_t cid;
                        uint8_t cid_num;

                        gap_chann_get_cid(app_db.br_link[app_idx].acl.conn_handle, 1, &cid, &cid_num);

                        if (transmit_srv_tx_data(app_db.br_link[app_idx].acl.conn_handle, cid, pkt_len, pkt_ptr) == true)
                        {
                            dt_queue_ctrl.dt_status = DT_STATUS_ACTIVE;
                            if (app_transfer_cfg.enable_embedded_cmd)
                            {
                                if (event_id != EVENT_ACK)
                                {
                                    app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                                    app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x01, false,
                                                    2000);
                                }
                            }
                        }
                        else
                        {
                            app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                            app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                            100);
                        }
                    }
                    else
                    {
                        app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                        app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                        1000);
                    }
                }
                else
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
                    //set timer to pop queue
                    app_start_timer(&timer_idx_data_transfer, "data_transfer",
                                    app_transfer_timer_id, APP_TIMER_DATA_TRANSFER, 0x00, false,
                                    1);
                }
            }
        }
    }
}

bool app_push_data_transfer_queue(uint8_t cmd_path, uint8_t *data, uint16_t data_len,
                                  uint8_t extra_param)
{
    APP_PRINT_TRACE4("app_push_data_transfer_queue: cmd_path %d, data_len %d w_idx %d r_idx %d",
                     cmd_path,
                     data_len,
                     dt_queue_ctrl.dt_queue_w_idx,
                     dt_queue_ctrl.dt_queue_r_idx);

    if (CMD_PATH_UART == cmd_path)
    {
#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0)
        if (uart_tx_queue[uart_tx_wIndex].active == 0)
        {
            uart_tx_queue[uart_tx_wIndex].active = 1;
            uart_tx_queue[uart_tx_wIndex].packet_ptr = data;
            uart_tx_queue[uart_tx_wIndex].packet_len = data_len;
            uart_tx_queue[uart_tx_wIndex].extra_param = extra_param;
            uart_tx_wIndex++;
            APP_PRINT_INFO4("seq 0x%x write index %d, read index = %d, queue number = %d", data[1],
                            uart_tx_wIndex, uart_tx_rIndex, UART_TX_QUEUE_NO);
            if (uart_tx_wIndex >= UART_TX_QUEUE_NO)
            {
                uart_tx_wIndex = 0;
            }

            app_pop_data_transfer_queue(cmd_path, false, 0);
            return true;
        }
        else
        {
            APP_PRINT_TRACE0("app_push_data_transfer_queue: uart_tx_queue full");
            return false;
        }
#else
        uint8_t index = extra_param;

        if (index == 0)
        {
            APP_PRINT_TRACE4("app_transfer_push_data_queue_uart0_tx_wIndex %d, active %d, packet_ptr %p, packet_len %d",
                             uart0_tx_wIndex, uart0_tx_queue[uart0_tx_wIndex].active, uart0_tx_queue[uart0_tx_wIndex].packet_ptr,
                             uart0_tx_queue[uart0_tx_wIndex].packet_len);
            if (uart0_tx_queue[uart0_tx_wIndex].active == 0)
            {
                uart0_tx_queue[uart0_tx_wIndex].active = 1;
                uart0_tx_queue[uart0_tx_wIndex].packet_ptr = data;
                uart0_tx_queue[uart0_tx_wIndex].packet_len = data_len;
                uart0_tx_queue[uart0_tx_wIndex].extra_param = extra_param;
                uart0_tx_wIndex++;
                if (uart0_tx_wIndex >= UART_TX_QUEUE_NO)
                {
                    uart0_tx_wIndex = 0;
                }

                app_pop_data_transfer_queue(cmd_path, false, index);
                return true;
            }
            else
            {
                APP_PRINT_TRACE0("app_transfer_push_data_queue: uart_tx_queue full");
                return false;
            }
        }
        else if (index == 2)
        {
            APP_PRINT_TRACE4("app_transfer_push_data_queue_uart2_tx_wIndex %d, active %d, packet_ptr %p, packet_len %d",
                             uart2_tx_wIndex, uart2_tx_queue[uart2_tx_wIndex].active, uart2_tx_queue[uart2_tx_wIndex].packet_ptr,
                             uart2_tx_queue[uart2_tx_wIndex].packet_len);

            if (uart2_tx_queue[uart2_tx_wIndex].active == 0)
            {
                uart2_tx_queue[uart2_tx_wIndex].active = 1;
                uart2_tx_queue[uart2_tx_wIndex].packet_ptr = data;
                uart2_tx_queue[uart2_tx_wIndex].packet_len = data_len;
                uart0_tx_queue[uart2_tx_wIndex].extra_param = extra_param;
                uart2_tx_wIndex++;
                if (uart2_tx_wIndex >= UART_TX_QUEUE_NO)
                {
                    uart2_tx_wIndex = 0;
                }

                app_pop_data_transfer_queue(cmd_path, false, index);
                return true;
            }
            else
            {
                APP_PRINT_TRACE0("app_transfer_push_data_queue: uart_tx_queue full");
                return false;
            }
        }

        return false;
#endif
    }
    else
    {
        if (dt_queue[dt_queue_ctrl.dt_queue_w_idx].active != 0)
        {
            APP_PRINT_WARN1("app_push_data_transfer_queue: dt_queue[%d] full", dt_queue_ctrl.dt_queue_w_idx);
            app_stop_timer(&timer_idx_data_transfer);
            dt_queue[dt_queue_ctrl.dt_queue_w_idx].active = 0;

            if (dt_queue[dt_queue_ctrl.dt_queue_w_idx].pkt_ptr != NULL)
            {
                free(dt_queue[dt_queue_ctrl.dt_queue_w_idx].pkt_ptr);
                dt_queue[dt_queue_ctrl.dt_queue_w_idx].pkt_ptr = NULL;
            }

            dt_queue_ctrl.dt_resend_count = 0;
            dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
        }

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
        APP_PRINT_TRACE2("app_push_data_transfer_queue: idx %d, bd_addr %s", idx, TRACE_BDADDR(bd_addr));

        app_pop_data_transfer_queue(cmd_path, false, 0xff);
        return true;
    }
}

bool app_transfer_check_active(uint8_t cmd_path)
{
    if (CMD_PATH_UART == cmd_path)
    {
#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0)
        if (uart_tx_queue[uart_tx_wIndex].active == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
#else
        if (uart0_tx_queue[uart0_tx_wIndex].active == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
#endif
    }
    else
    {
        if (dt_queue[dt_queue_ctrl.dt_queue_w_idx].active == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
}

void app_transfer_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_RX);
    app_start_timer(&timer_idx_uart_wake_up, "uart_wake_up",
                    app_transfer_timer_id, APP_TIMER_UART_WAKE_UP, 0, false,
                    2000);
}

static void app_transfer_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_transfer_timeout_cb: timer_id %d, timer_chann %d", timer_evt, param);

    switch (timer_evt)
    {
#if F_APP_ENABLE_TWO_ONE_WIRE_UART
    case APP_TIMER_UART0_ACK:
        app_stop_timer(&timer_idx_uart0_ack);
        app_dlps_enable(APP_DLPS_ENTER_CHECK_UART_ACK);
        uart0_tx_status = DT_STATUS_IDLE;
        uart0_resend_count++;
        if (uart0_resend_count > app_transfer_cfg.dt_resend_num)
        {
            app_pop_data_transfer_queue(CMD_PATH_UART, true, 0);
        }
        else
        {
            app_pop_data_transfer_queue(CMD_PATH_UART, false, 0);
        }
        break;

    case APP_TIMER_UART2_ACK:
        app_stop_timer(&timer_idx_uart2_ack);
        app_dlps_enable(APP_DLPS_ENTER_CHECK_UART_ACK);
        uart2_tx_status = DT_STATUS_IDLE;
        uart2_resend_count++;
        if (uart2_resend_count > app_transfer_cfg.dt_resend_num)
        {
            app_pop_data_transfer_queue(CMD_PATH_UART, true, 2);
        }
        else
        {
            app_pop_data_transfer_queue(CMD_PATH_UART, false, 2);
        }
        break;
#else
    case APP_TRANSFER_TIMER_CHECK_CONSOLE_COMP:
        {
            app_stop_timer(&timer_handle_console_send_comp);
            uart_tx_status = DT_STATUS_IDLE;
            app_pop_data_transfer_queue(CMD_PATH_UART, false, 0);
        }
        break;
    case APP_TIMER_UART_ACK:
        app_stop_timer(&timer_idx_uart_ack);
        app_dlps_enable(APP_DLPS_ENTER_CHECK_UART_ACK);
        uart_tx_status = DT_STATUS_IDLE;
        uart_resend_count++;
        if (uart_resend_count > app_transfer_cfg.dt_resend_num)
        {
            app_pop_data_transfer_queue(CMD_PATH_UART, true, 0);
        }
        else
        {
            app_pop_data_transfer_queue(CMD_PATH_UART, false, 0);
        }
        break;

    case APP_TIMER_UART_WAKE_UP:
        app_stop_timer(&timer_idx_uart_wake_up);
        app_dlps_enable(APP_DLPS_ENTER_CHECK_UART_RX);
        break;

    case APP_TIMER_UART_TX_WAKE_UP:
        app_stop_timer(&timer_idx_uart_tx_wake_up);
        app_pop_data_transfer_queue(CMD_PATH_UART, false, 0);
        break;

    case APP_TIMER_DATA_TRANSFER:
        app_stop_timer(&timer_idx_data_transfer);
        dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
        if (param == 0x01) //ack timeout
        {
            dt_queue_ctrl.dt_resend_count++;
            if (dt_queue_ctrl.dt_resend_count >= app_transfer_cfg.dt_resend_num)
            {
                app_pop_data_transfer_queue(dt_queue[dt_queue_ctrl.dt_queue_r_idx].active, true, 0xff);
            }
            else
            {
                app_pop_data_transfer_queue(dt_queue[dt_queue_ctrl.dt_queue_r_idx].active, false, 0xff);
            }
        }
        else
        {
            app_pop_data_transfer_queue(dt_queue[dt_queue_ctrl.dt_queue_r_idx].active, false, 0xff);
        }
        break;
#endif

    default:
        break;
    }
}

void app_transfer_init(void)
{
    app_timer_reg_cb(app_transfer_timeout_cb, &app_transfer_timer_id);
}

static void app_transfer_bt_data(uint8_t *cmd_ptr, uint8_t cmd_path, uint8_t app_idx,
                                 uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    uint16_t total_len;
    uint16_t pkt_len;
    uint8_t  idx;
    uint8_t  pkt_type;
    uint8_t  *pkt_ptr;

    idx        = cmd_ptr[2];
    pkt_type   = cmd_ptr[3];
    total_len  = (cmd_ptr[4] | (cmd_ptr[5] << 8));
    pkt_len    = (cmd_ptr[6] | (cmd_ptr[7] << 8));
    pkt_ptr    = &cmd_ptr[8];

    if (((cmd_id == CMD_LEGACY_DATA_TRANSFER) &&
         ((app_db.br_link[idx].connected_profile & SPP_PROFILE_MASK) ||
          (app_db.br_link[idx].connected_profile & IAP_PROFILE_MASK))) ||
        ((cmd_id == CMD_LE_DATA_TRANSFER) &&
         (app_db.le_link[idx].state == LE_LINK_STATE_CONNECTED)))
    {
        if (cmd_path == CMD_PATH_UART)
        {
            if (pkt_len)
            {
                if ((pkt_type == PKT_TYPE_SINGLE) || (pkt_type == PKT_TYPE_START))
                {
                    if ((cmd_id == CMD_LEGACY_DATA_TRANSFER) &&
                        (((app_db.br_link[idx].connected_profile & SPP_PROFILE_MASK) &&
                          (!app_transfer_check_active(CMD_PATH_SPP))) ||
                         (((app_db.br_link[idx].connected_profile & IAP_PROFILE_MASK) &&
                           (!app_transfer_check_active(CMD_PATH_IAP))))))
                    {
                        if (app_db.br_link[idx].cmd.uart.rx_dt_pkt)
                        {
                            free(app_db.br_link[idx].cmd.uart.rx_dt_pkt);
                        }

                        app_db.br_link[idx].cmd.uart.rx_dt_pkt = malloc(total_len);
                        memcpy(app_db.br_link[idx].cmd.uart.rx_dt_pkt, pkt_ptr, pkt_len);
                        app_db.br_link[idx].cmd.uart.rx_dt_pkt_len = pkt_len;
                    }
                    else if ((cmd_id == CMD_LE_DATA_TRANSFER) && (!app_transfer_check_active(CMD_PATH_LE)))
                    {
                        if (uart_rx_dt_pkt_ptr)
                        {
                            free(uart_rx_dt_pkt_ptr);
                        }

                        uart_rx_dt_pkt_ptr = malloc(total_len);
                        memcpy(uart_rx_dt_pkt_ptr, pkt_ptr, pkt_len);
                        uart_rx_dt_pkt_len = pkt_len;
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    }
                }
                else
                {
                    if ((cmd_id == CMD_LEGACY_DATA_TRANSFER) && (app_db.br_link[idx].cmd.uart.rx_dt_pkt))
                    {
                        uint8_t *temp_ptr;

                        temp_ptr = app_db.br_link[idx].cmd.uart.rx_dt_pkt +
                                   app_db.br_link[idx].cmd.uart.rx_dt_pkt_len;
                        memcpy(temp_ptr, pkt_ptr, pkt_len);
                        app_db.br_link[idx].cmd.uart.rx_dt_pkt_len += pkt_len;
                    }
                    else if ((cmd_id == CMD_LE_DATA_TRANSFER) && uart_rx_dt_pkt_ptr)
                    {
                        uint8_t *temp_ptr;

                        temp_ptr = uart_rx_dt_pkt_ptr + uart_rx_dt_pkt_len;
                        memcpy(temp_ptr, pkt_ptr, pkt_len);
                        uart_rx_dt_pkt_len += pkt_len;
                    }
                    else//maybe start packet been lost
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    }
                }

                if ((pkt_type == PKT_TYPE_SINGLE) || (pkt_type == PKT_TYPE_END))
                {
                    if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
                    {
                        if (cmd_id == CMD_LEGACY_DATA_TRANSFER)
                        {
                            if (app_db.br_link[idx].connected_profile & SPP_PROFILE_MASK)
                            {
                                app_report_raw_data(CMD_PATH_SPP, app_idx, app_db.br_link[idx].cmd.uart.rx_dt_pkt,
                                                    app_db.br_link[idx].cmd.uart.rx_dt_pkt_len);

                                free(app_db.br_link[idx].cmd.uart.rx_dt_pkt);
                                app_db.br_link[idx].cmd.uart.rx_dt_pkt = NULL;

                                if (app_transfer_check_active(CMD_PATH_SPP))
                                {
                                    ack_pkt[2] = CMD_SET_STATUS_BUSY;
                                    app_db.br_link[idx].cmd.resume = true;
                                }
                            }
                            else if (app_db.br_link[idx].connected_profile & IAP_PROFILE_MASK)
                            {
                                app_report_raw_data(CMD_PATH_IAP, app_idx, app_db.br_link[idx].cmd.uart.rx_dt_pkt,
                                                    app_db.br_link[idx].cmd.uart.rx_dt_pkt_len);

                                free(app_db.br_link[idx].cmd.uart.rx_dt_pkt);
                                app_db.br_link[idx].cmd.uart.rx_dt_pkt = NULL;

                                if (app_transfer_check_active(CMD_PATH_IAP))
                                {
                                    ack_pkt[2] = CMD_SET_STATUS_BUSY;
                                    app_db.br_link[idx].cmd.resume = true;
                                }
                            }
                        }
                        else if (cmd_id == CMD_LE_DATA_TRANSFER)
                        {
                            app_report_raw_data(CMD_PATH_LE, app_idx, uart_rx_dt_pkt_ptr, uart_rx_dt_pkt_len);

                            free(uart_rx_dt_pkt_ptr);
                            uart_rx_dt_pkt_ptr = NULL;

                            if (app_transfer_check_active(CMD_PATH_LE))
                            {
                                ack_pkt[2] = CMD_SET_STATUS_BUSY;
                            }
                        }
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
        }
    }
    else
    {
        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
    }

    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
}

void app_transfer_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                             uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_LEGACY_DATA_TRANSFER:
    case CMD_LE_DATA_TRANSFER:
        {
            app_transfer_bt_data(&cmd_ptr[0], cmd_path, app_idx, &ack_pkt[0]);
        }
        break;

    default:
        {
            ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;
    }
}

