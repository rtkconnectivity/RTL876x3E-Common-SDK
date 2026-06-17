/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#include <stdlib.h>
#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "btm.h"
#include "bt_spp.h"
#include "app_link_util.h"
#include "app_spp.h"
#include "app_transfer.h"
#include "app_main.h"
#include "app_cmd.h"
#include "app_sdp.h"

static const uint8_t rtk_vendor_spp_service_class_uuid128[16] =
{
    0x6a, 0x24, 0xee, 0xab, 0x4b, 0x65, 0x46, 0x93, 0x98, 0x6b, 0x3c, 0x26, 0xc3, 0x52, 0x26, 0x4f
};

static void app_spp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_SPP_CONN_CMPL:
        {
            APP_PRINT_TRACE0("BT_EVENT_SPP_CONN_CMPL");
            if (param->spp_conn_cmpl.local_server_chann != RFC_RTK_VENDOR_CHANN_NUM)
            {
                return;
            }

            p_link = app_link_find_br_link(param->spp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (param->spp_conn_cmpl.local_server_chann == RFC_RTK_VENDOR_CHANN_NUM)
                {
                    p_link->rtk_vendor_spp_active = 1;
                }
                p_link->rfc_credit = param->spp_conn_cmpl.link_credit;
                p_link->rfc_frame_size = param->spp_conn_cmpl.frame_size;
                //check credit
                if (p_link->rfc_credit)
                {
                    app_transfer_pop_data_queue(CMD_PATH_SPP, false);
                }
            }
        }
        break;

    case BT_EVENT_SPP_CREDIT_RCVD:
        {
            APP_PRINT_TRACE0("BT_EVENT_SPP_CREDIT_RCVD");
            if (param->spp_credit_rcvd.local_server_chann != RFC_RTK_VENDOR_CHANN_NUM)
            {
                return;
            }

            p_link = app_link_find_br_link(param->spp_credit_rcvd.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_spp_bt_cback: no acl link found");
                return;
            }
            if ((p_link->rfc_credit == 0) && (param->spp_credit_rcvd.link_credit))
            {
                app_transfer_pop_data_queue(CMD_PATH_SPP, false);
            }
            p_link->rfc_credit = param->spp_credit_rcvd.link_credit;
        }
        break;

    case BT_EVENT_SPP_DATA_IND:
        {
            if (param->spp_data_ind.local_server_chann != RFC_RTK_VENDOR_CHANN_NUM)
            {
                return;
            }

            uint8_t     *p_data;
            uint16_t    len;
            uint8_t     app_idx;
            uint16_t    data_len;

            p_link = app_link_find_br_link(param->spp_data_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_spp_bt_cback: no acl link found");
                return;
            }
            app_idx = p_link->id;
            p_data = param->spp_data_ind.data;
            len = param->spp_data_ind.len;
            data_len = len;

            bt_spp_credits_give(app_db.br_link[app_idx].bd_addr, param->spp_data_ind.local_server_chann,
                                1);

            {
                if (app_db.br_link[app_idx].p_embedded_cmd == NULL)
                {
                    uint16_t cmd_len;

                    //ios will auto combine two cmd into one pkt
                    while (data_len)
                    {
                        if (p_data[0] == CMD_SYNC_BYTE)
                        {
                            cmd_len = (p_data[2] | (p_data[3] << 8)) + 4; //sync_byte, seqn, length
                            if (data_len >= cmd_len)
                            {
                                app_cmd_handler(&p_data[4], (cmd_len - 4), CMD_PATH_SPP, p_data[1], app_idx);
                                data_len -= cmd_len;
                                p_data += cmd_len;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            data_len--;
                            p_data++;
                        }
                    }

                    if (data_len)
                    {
                        app_db.br_link[app_idx].p_embedded_cmd = malloc(data_len);
                        memcpy(app_db.br_link[app_idx].p_embedded_cmd, p_data, data_len);
                        app_db.br_link[app_idx].embedded_cmd_len = data_len;
                    }
                }
                else
                {
                    uint8_t *p_temp;
                    uint16_t cmd_len;

                    p_temp = app_db.br_link[app_idx].p_embedded_cmd;
                    uint16_t total_len = app_db.br_link[app_idx].embedded_cmd_len + data_len;
                    app_db.br_link[app_idx].p_embedded_cmd = malloc(total_len);
                    memcpy(app_db.br_link[app_idx].p_embedded_cmd, p_temp,
                           app_db.br_link[app_idx].embedded_cmd_len);
                    free(p_temp);
                    memcpy(app_db.br_link[app_idx].p_embedded_cmd +
                           app_db.br_link[app_idx].embedded_cmd_len,
                           p_data, data_len);
                    app_db.br_link[app_idx].embedded_cmd_len = total_len;

                    p_data = app_db.br_link[app_idx].p_embedded_cmd;
                    data_len = total_len;
                    p_temp = app_db.br_link[app_idx].p_embedded_cmd;
                    app_db.br_link[app_idx].p_embedded_cmd = NULL;
                    //ios will auto combine two cmd into one pkt
                    while (data_len)
                    {
                        if (p_data[0] == CMD_SYNC_BYTE)
                        {
                            cmd_len = (p_data[2] | (p_data[3] << 8)) + 4; //sync_byte, seqn, length
                            if (data_len >= cmd_len)
                            {
                                app_cmd_handler(&p_data[4], (cmd_len - 4), CMD_PATH_SPP, p_data[1], app_idx);
                                data_len -= cmd_len;
                                p_data += cmd_len;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            data_len--;
                            p_data++;
                        }
                    }

                    if (data_len)
                    {
                        app_db.br_link[app_idx].p_embedded_cmd = malloc(data_len);
                        memcpy(app_db.br_link[app_idx].p_embedded_cmd, p_data, data_len);
                    }
                    app_db.br_link[app_idx].embedded_cmd_len = data_len;
                    free(p_temp);
                }
            }
        }
        break;

    case BT_EVENT_SPP_CONN_IND:
        {
            APP_PRINT_TRACE0("BT_EVENT_SPP_CONN_IND");
            if (param->spp_conn_ind.local_server_chann != RFC_RTK_VENDOR_CHANN_NUM)
            {
                return;
            }

            p_link = app_link_find_br_link(param->spp_conn_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_spp_bt_cback: no acl link found");
                return;
            }

            p_link->connected_profile |=  SPP_PROFILE_MASK;

            uint8_t local_server_chann = param->spp_conn_ind.local_server_chann;
            uint16_t frame_size = param->spp_conn_ind.frame_size;
            bt_spp_connect_cfm(p_link->bd_addr, local_server_chann, true, frame_size, 7);

        }
        break;

    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            if (param->spp_disconn_cmpl.local_server_chann != RFC_RTK_VENDOR_CHANN_NUM)
            {
                return;
            }

            p_link = app_link_find_br_link(param->spp_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                p_link->rfc_credit = 0;
                if (param->spp_disconn_cmpl.local_server_chann == RFC_RTK_VENDOR_CHANN_NUM)
                {
                    p_link->rtk_vendor_spp_active = 0;
                }
            }
            app_transfer_queue_reset(CMD_PATH_SPP);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_spp_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_spp_init(void)
{
    APP_PRINT_INFO0("app_spp_init");

    bt_spp_init();
    bt_mgr_cback_register(app_spp_bt_cback);
    bt_spp_service_register((uint8_t *)rtk_vendor_spp_service_class_uuid128, RFC_RTK_VENDOR_CHANN_NUM);

    //enable or disable ERTM mode
    bt_spp_ertm_mode_set(false);//disable spp l2cap ERTM mode by default.
}
