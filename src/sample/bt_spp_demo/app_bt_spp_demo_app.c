/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include "trace.h"
#include "bt_types.h"
#include "gap_br.h"
#include "btm.h"
#include "bt_spp.h"
#include "app_bt_spp_demo_sdp.h"
#include "app_bt_spp_demo_app.h"
#include "app_bt_spp_demo_link.h"
#include "console.h"

/* SPP UUID */
static const uint8_t spp_demo_service_class_uuid128[16] =
{
    0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
};

static void spp_demo_app_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_SPP_DEMO_LINK  *p_link;
    T_SPP_SERVICE    *service;
    bool              handle = true;

    switch (event_type)
    {
    case BT_EVENT_SPP_CONN_CMPL:
        {
            p_link = spp_demo_find_link(param->spp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                service = spp_demo_find_service(param->spp_conn_cmpl.bd_addr,
                                                param->spp_conn_cmpl.local_server_chann);
                if (service == NULL)
                {
                    service = spp_demo_alloc_service(param->spp_conn_cmpl.bd_addr,
                                                     param->spp_conn_cmpl.local_server_chann);
                    if (service != NULL)
                    {
                        char console_buf[110];

                        service->credit = param->spp_conn_cmpl.link_credit;
                        service->frame_size = param->spp_conn_cmpl.frame_size;

                        sprintf((char *)console_buf,
                                "SPP Connected: addr 0x%02x::0x%02x::0x%02x::0x%02x::0x%02x::0x%02x, local_server_chann 0x%02x \r\n",
                                param->spp_conn_cmpl.bd_addr[0],
                                param->spp_conn_cmpl.bd_addr[1],
                                param->spp_conn_cmpl.bd_addr[2],
                                param->spp_conn_cmpl.bd_addr[3],
                                param->spp_conn_cmpl.bd_addr[4],
                                param->spp_conn_cmpl.bd_addr[5],
                                param->spp_conn_cmpl.local_server_chann);

                        console_write((uint8_t *)console_buf, strlen(console_buf));
                    }
                }

                bt_device_mode_set(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
            }
        }
        break;

    case BT_EVENT_SPP_CREDIT_RCVD:
        {
            service = spp_demo_find_service(param->spp_credit_rcvd.bd_addr,
                                            param->spp_credit_rcvd.local_server_chann);
            if (service != NULL)
            {
                service->credit = param->spp_credit_rcvd.link_credit;
            }
        }
        break;

    case BT_EVENT_SPP_DATA_IND:
        {
            service = spp_demo_find_service(param->spp_data_ind.bd_addr,
                                            param->spp_data_ind.local_server_chann);
            if (service != NULL)
            {
                char console_buf[110];

                bt_spp_credits_give(param->spp_data_ind.bd_addr, param->spp_data_ind.local_server_chann, 1);

                sprintf((char *)console_buf,
                        "SPP receive data from addr 0x%02x::0x%02x::0x%02x::0x%02x::0x%02x::0x%02x, local_server_chann 0x%02x \r\n",
                        param->spp_data_ind.bd_addr[0],
                        param->spp_data_ind.bd_addr[1],
                        param->spp_data_ind.bd_addr[2],
                        param->spp_data_ind.bd_addr[3],
                        param->spp_data_ind.bd_addr[4],
                        param->spp_data_ind.bd_addr[5],
                        param->spp_data_ind.local_server_chann);

                console_write((uint8_t *)console_buf, strlen(console_buf));
                console_write(param->spp_data_ind.data, param->spp_data_ind.len);
            }
        }
        break;

    case BT_EVENT_SPP_CONN_IND:
        {
            p_link = spp_demo_find_link(param->spp_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                service = spp_demo_find_service(param->spp_conn_ind.bd_addr,
                                                param->spp_conn_ind.local_server_chann);

                if (service == NULL)
                {
                    bt_spp_connect_cfm(p_link->bd_addr,
                                       param->spp_conn_ind.local_server_chann,
                                       true,
                                       param->spp_conn_ind.frame_size,
                                       SPP_DEMO_SPP_DEFAULT_CREDITS);
                }
                else
                {
                    bt_spp_connect_cfm(p_link->bd_addr,
                                       param->spp_conn_ind.local_server_chann,
                                       false,
                                       param->spp_conn_ind.frame_size,
                                       SPP_DEMO_SPP_DEFAULT_CREDITS);
                }
            }
        }
        break;

    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            service = spp_demo_find_service(param->spp_disconn_cmpl.bd_addr,
                                            param->spp_disconn_cmpl.local_server_chann);
            if (service != NULL)
            {
                spp_demo_free_service(param->spp_disconn_cmpl.bd_addr, service);
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("spp_demo_app_bt_cback: event_type 0x%04x", event_type);
    }
}

void spp_demo_app_tx_data(uint8_t *bd_addr, uint8_t *buf, uint8_t len)
{
    T_SPP_DEMO_LINK *p_link;

    p_link = spp_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        T_SPP_SERVICE *service;

        service = os_queue_peek(&p_link->service_list, 0);
        while (service != NULL)
        {
            APP_PRINT_TRACE3("spp_demo_app_tx_data: spp_credit %d, local_server_chann %d, len %d",
                             service->credit, service->local_server_chann, len);

            if (service->credit)
            {
                if (bt_spp_data_send(bd_addr,
                                     service->local_server_chann,
                                     buf,
                                     len,
                                     false))
                {
                    char console_buf[110];

                    service->credit--;

                    sprintf((char *)console_buf,
                            "SPP send data to addr 0x%02x::0x%02x::0x%02x::0x%02x::0x%02x::0x%02x, local_server_chann 0x%02x \r\n",
                            bd_addr[0],
                            bd_addr[1],
                            bd_addr[2],
                            bd_addr[3],
                            bd_addr[4],
                            bd_addr[5],
                            service->local_server_chann);

                    console_write((uint8_t *)console_buf, strlen(console_buf));
                    console_write(buf, len);
                }
            }

            service = service->next;
        }
    }
}

void spp_demo_app_connect_req(uint8_t *bd_addr)
{
    T_GAP_UUID_DATA uuid;
    T_GAP_UUID_TYPE uuid_type = GAP_UUID16;

    uuid.uuid_16 = UUID_RFCOMM;
    gap_br_start_sdp_discov(bd_addr, uuid_type, uuid);
}

void spp_demo_app_disconnect_all_req(uint8_t *bd_addr)
{
    T_SPP_DEMO_LINK *p_link;

    p_link = spp_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        bt_spp_disconnect_all_req(bd_addr);
    }
}

void spp_demo_app_disconnect_req(uint8_t *bd_addr, uint8_t local_server_chann)
{
    T_SPP_DEMO_LINK *p_link;

    p_link = spp_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        bt_spp_disconnect_req(bd_addr, local_server_chann);
    }
}

void spp_demo_app_init(void)
{
    spp_demo_int();
    bt_spp_init();
    bt_spp_service_register((uint8_t *)spp_demo_service_class_uuid128, SPP_DEMO_RFC_SPP_CHANN_NUM);
    bt_mgr_cback_register(spp_demo_app_bt_cback);
    bt_spp_ertm_mode_set(false);
}
