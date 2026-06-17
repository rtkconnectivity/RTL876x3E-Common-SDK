/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include <string.h>

#include "stdlib.h"
#include "trace.h"
#include "btm.h"
#include "bt_spp.h"
#include "app_link_util_cs.h"
#include "app_spp.h"
#include "app_transfer_cfg.h"
#include "app_transfer.h"
#include "app_main.h"
#include "app_cmd.h"

#include "app_sdp.h"
#include "bt_types.h"




static const uint8_t spp_service_class_uuid128[16] =
{
    0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
};

static const uint8_t rtk_vendor_spp_service_class_uuid128[16] =
{
    0x6a, 0x24, 0xee, 0xab, 0x4b, 0x65, 0x46, 0x93, 0x98, 0x6b, 0x3c, 0x26, 0xc3, 0x52, 0x26, 0x4f
};


static void vendor_spp_handler(T_BT_EVENT event_type, T_BT_EVENT_PARAM *param)
{
    if (param->spp_conn_cmpl.local_server_chann != RFC_SPP_CHANN_NUM
        && param->spp_conn_cmpl.local_server_chann != RFC_RTK_VENDOR_CHANN_NUM)
    {
        return;
    }

    T_APP_BR_LINK *p_link = NULL;

    APP_PRINT_TRACE1("app_spp vendor_spp_handler: event_type 0x%04x", event_type);

    switch (event_type)
    {
    case BT_EVENT_SPP_CONN_CMPL:
        p_link = app_link_find_br_link(param->spp_conn_cmpl.bd_addr);
        if (p_link != NULL)
        {
            bt_sniff_mode_disable(param->spp_conn_cmpl.bd_addr);
            if (param->spp_conn_cmpl.local_server_chann == RFC_RTK_VENDOR_CHANN_NUM)
            {
                p_link->vendor_spp.is_active = true;
            }
            p_link->vendor_spp.credit = param->spp_conn_cmpl.link_credit;
            p_link->vendor_spp.frame_size = param->spp_conn_cmpl.frame_size;
            //check credit
            if (p_link->vendor_spp.credit)
            {
                app_pop_data_transfer_queue(CMD_PATH_SPP, false, 0xff);
            }
        }
        break;

    case BT_EVENT_SPP_CREDIT_RCVD:
        p_link = app_link_find_br_link(param->spp_credit_rcvd.bd_addr);
        if (p_link == NULL)
        {
            APP_PRINT_ERROR0("app_spp_bt_cback: no acl link found");
            return;
        }

        p_link->vendor_spp.credit = param->spp_credit_rcvd.link_credit;
        break;

    case BT_EVENT_SPP_DATA_IND:
        {
            uint8_t     *p_data;
            uint16_t    len;
            uint8_t     app_idx;
            uint16_t    data_len;
            uint16_t    total_len;

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

            //APP_PRINT_TRACE2("app_spp_bt_cback: data_len %d, p_data %b", data_len, TRACE_BINARY(data_len,
            //p_data));

            if (app_transfer_cfg.enable_embedded_cmd)
            {
                bt_spp_credits_give(app_db.br_link[app_idx].bd_addr, param->spp_data_ind.local_server_chann, 1);
                app_cmd_proc_data(CMD_PATH_SPP, &app_db.br_link[app_idx].cmd.buf, &app_db.br_link[app_idx].cmd.len,
                                  p_data, data_len, app_idx);

            }
            else if (app_cfg_const.enable_data_uart)
            {
                uint8_t     *tx_ptr;
                tx_ptr = malloc((data_len + 3));
                if (tx_ptr != NULL)
                {
                    tx_ptr[0] = app_idx;
                    tx_ptr[1] = (uint8_t)data_len;
                    tx_ptr[2] = (uint8_t)(data_len >> 8);
                    memcpy(&tx_ptr[3], p_data, data_len);

                    app_report_event(CMD_PATH_UART, EVENT_LEGACY_DATA_TRANSFER, 0, tx_ptr, data_len + 3);

                    free(tx_ptr);
                }
            }
        }
        break;

    case BT_EVENT_SPP_CONN_IND:
        {
            p_link = app_link_find_br_link(param->spp_conn_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_spp_bt_cback: no acl link found");
                return;
            }
            uint8_t local_server_chann = param->spp_conn_ind.local_server_chann;
            uint16_t frame_size = param->spp_conn_ind.frame_size;
            bt_spp_connect_cfm(p_link->bd_addr, local_server_chann, true, frame_size, 7);
        }
        break;

    case BT_EVENT_SPP_DISCONN_CMPL:
        p_link = app_link_find_br_link(param->spp_disconn_cmpl.bd_addr);
        if (p_link != NULL)
        {
            bt_sniff_mode_enable(param->spp_disconn_cmpl.bd_addr, 0, 0, 0, 0);
            p_link->vendor_spp.credit = 0;
            if (param->spp_disconn_cmpl.local_server_chann == RFC_RTK_VENDOR_CHANN_NUM)
            {
                p_link->vendor_spp.is_active = false;
            }
        }

        app_transfer_queue_reset(CMD_PATH_SPP);
        break;

    default:
        break;
    }
}


static void app_spp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link = NULL;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_SPP_CONN_CMPL:
        {
            uint8_t temp_buff[sizeof(param->spp_conn_cmpl) + 4];
            LE_UINT32_TO_ARRAY(&temp_buff[0], SPP_PROFILE_MASK);
            memcpy(&temp_buff[4], &param->spp_conn_cmpl, sizeof(param->spp_conn_cmpl));
            app_report_event(CMD_PATH_UART, EVENT_PROFILE_CONN_STATUS, 0, temp_buff, sizeof(temp_buff));
        }
        break;

    case BT_EVENT_SPP_CREDIT_RCVD:
        {

        }
        break;

    case BT_EVENT_SPP_DATA_IND:
        {

        }
        break;

    case BT_EVENT_SPP_CONN_IND:
        {
        }
        break;

    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            uint8_t temp_buff[sizeof(param->spp_disconn_cmpl) + 1];

            temp_buff[0] = SPP_PROFILE_MASK;
            memcpy(&temp_buff[1], &param->spp_disconn_cmpl, sizeof(param->spp_disconn_cmpl));
            app_report_event(CMD_PATH_UART, EVENT_PROFILE_DISCONN_STATUS, 0, temp_buff, sizeof(temp_buff));
        }
        break;

    case BT_EVENT_SPP_CONN_FAIL:
        {
            uint8_t temp_buff[sizeof(param->spp_conn_fail) + 1];

            temp_buff[0] = SPP_PROFILE_MASK;
            memcpy(&temp_buff[1], &param->spp_conn_fail, sizeof(param->spp_conn_fail));
            app_report_event(CMD_PATH_UART, EVENT_PROFILE_CONNECT_FAIL_STATUS, 0, temp_buff,
                             sizeof(temp_buff));
        }
        break;

    default:
        break;
    }

    vendor_spp_handler(event_type, param);
}

void app_spp_init(void)
{
    if ((app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK) == 0)
    {
        APP_PRINT_WARN0("app_spp_init: not support spp");
        return;
    }

    bt_spp_init();
    bt_mgr_cback_register(app_spp_bt_cback);
    bt_spp_service_register((uint8_t *)spp_service_class_uuid128, RFC_SPP_CHANN_NUM);
    bt_spp_service_register((uint8_t *)rtk_vendor_spp_service_class_uuid128, RFC_RTK_VENDOR_CHANN_NUM);
}
