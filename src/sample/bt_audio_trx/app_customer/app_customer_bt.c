/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_CUSTOMER_VD_SPP_SUPPORT
#include "btm.h"
#include "app_link_util_cs.h"
#include "app_sdp.h"
#include "app_main.h"
#include "app_report.h"
#include "trace.h"
#include "bt_spp.h"
#include "bt_hfp.h"
#include "sysm.h"
#include "gap_br.h"
#include "app_customer_bt.h"
#include "cfg_item_api.h"
#include <stdlib.h>
#include "app_cmd.h"


static const uint8_t app_customer_service_class_uuid128[16] =
{
    0xdb, 0x76, 0x4a, 0xc8, 0x4b, 0x08, 0x7f, 0x25, 0xaa, 0xfe, 0x59, 0xd0, 0x3c, 0x27, 0xba, 0xe3
};


static void spp_bt_mgr_cb(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_SPP_CONN_CMPL:
        {
            if (param->spp_conn_cmpl.local_server_chann != RFC_SPP_CUSTOMER_NUM)
            {
                return;
            }

            p_link = app_link_find_br_link(param->spp_conn_cmpl.bd_addr);

            if (p_link != NULL)
            {
                bt_sniff_mode_disable(param->spp_conn_cmpl.bd_addr);
                uint8_t temp_buff[sizeof(param->spp_conn_cmpl) + 1];

                p_link->xm_spp.credit = param->spp_conn_cmpl.link_credit;
                p_link->xm_spp.frame_size = param->spp_conn_cmpl.frame_size;

                temp_buff[0] = SPP_PROFILE_MASK;
                memcpy(&temp_buff[1], &param->spp_conn_cmpl, sizeof(param->spp_conn_cmpl));
                app_report_event(CMD_PATH_UART, EVENT_XM_SPP_CONNECT_STATUS, 0, temp_buff, sizeof(temp_buff));
                APP_PRINT_TRACE2("app_customer_vd_spp_cback: link_credit %d frame_size %d",
                                 p_link->xm_spp.credit, p_link->xm_spp.frame_size);
            }
        }
        break;

    case BT_EVENT_SPP_CREDIT_RCVD:
        {
            if (param->spp_credit_rcvd.local_server_chann != RFC_SPP_CUSTOMER_NUM)
            {
                return;
            }

            p_link = app_link_find_br_link(param->spp_credit_rcvd.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_customer_vd_spp_cback: no acl link found");
                return;
            }

            p_link->xm_spp.credit = param->spp_credit_rcvd.link_credit;
        }
        break;

    case BT_EVENT_SPP_DATA_IND:
        {
            if (param->spp_data_ind.local_server_chann != RFC_SPP_CUSTOMER_NUM)
            {
                return;
            }

            uint8_t     *tx_ptr;
            uint8_t     *p_data;
            uint16_t    len;
            uint8_t     app_link_id;
            uint16_t    data_len;

            p_link = app_link_find_br_link(param->spp_data_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_customer_vd_spp_cback: no acl link found");
                return;
            }
            app_link_id = p_link->id;
            p_data = param->spp_data_ind.data;
            len = param->spp_data_ind.len;
            data_len = len;

//            bt_spp_credits_give(app_db.br_link[app_idx].bd_addr, param->spp_data_ind.local_server_chann,
//                                1);

            tx_ptr = malloc((data_len + 3));
            if (tx_ptr != NULL)
            {
                tx_ptr[0] = app_link_id;
                tx_ptr[1] = (uint8_t)data_len;
                tx_ptr[2] = (uint8_t)(data_len >> 8);
                memcpy(&tx_ptr[3], p_data, data_len);

                app_report_event(CMD_PATH_UART, EVENT_XM_SPP_DATA_TRANSFER, 0, tx_ptr, data_len + 3);

                free(tx_ptr);
            }
        }
        break;

    case BT_EVENT_SPP_CONN_IND:
        {
            if (param->spp_conn_ind.local_server_chann != RFC_SPP_CUSTOMER_NUM)
            {
                return;
            }

            p_link = app_link_find_br_link(param->spp_conn_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_customer_vd_spp_cback: no acl link found");
                return;
            }
            uint8_t local_server_chann = param->spp_conn_ind.local_server_chann;
            uint16_t frame_size = param->spp_conn_ind.frame_size;
            bt_spp_connect_cfm(p_link->bd_addr, local_server_chann, true, frame_size, 7);
        }
        break;

    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            if (param->spp_disconn_cmpl.local_server_chann != RFC_SPP_CUSTOMER_NUM)
            {
                return;
            }

            bt_sniff_mode_enable(param->spp_disconn_cmpl.bd_addr, 0, 0, 0, 0);
            uint8_t temp_buff[sizeof(param->spp_disconn_cmpl) + 1];
            temp_buff[0] = SPP_PROFILE_MASK;
            memcpy(&temp_buff[1], &param->spp_disconn_cmpl, sizeof(param->spp_disconn_cmpl));
            app_report_event(CMD_PATH_UART, EVENT_XM_SPP_DISCONNECT_STATUS, 0, temp_buff, sizeof(temp_buff));
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true && (event_type != BT_EVENT_SPP_CREDIT_RCVD))
    {
        APP_PRINT_INFO1("app_customer_vd_spp_cback: event_type 0x%04x", event_type);
    }
}

static void vd_spp_data_send(uint8_t app_link_id, uint8_t *data, uint16_t len)
{
    if (bt_spp_data_send(app_db.br_link[app_link_id].bd_addr, RFC_SPP_CUSTOMER_NUM, data, len, false))
    {
        APP_PRINT_INFO2("vd_spp_data_send success: len %d, data %b", len, TRACE_BINARY(len, data));
    }
    else
    {
        APP_PRINT_ERROR0("vd_spp_data_send fail !!");
    }
}

static void spp_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {

        }
        break;

    case SYS_EVENT_POWER_OFF:
        {

        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_customer_spp_dm_cback: event_type 0x%04x", event_type);
    }
}

static bool spp_init(void)
{
    APP_PRINT_TRACE0("app_customer_vendor_spp_init");

    bt_spp_service_register((uint8_t *)app_customer_service_class_uuid128, RFC_SPP_CUSTOMER_NUM);
    sys_mgr_cback_register(spp_dm_cback);
    bt_mgr_cback_register(spp_bt_mgr_cb);

    return true;
}


void app_customer_bt_att_cb(uint8_t *bd_addr, T_BT_ATT_MSG_TYPE msg_type, void *p_msg)
{
    APP_PRINT_TRACE2("app_customer_bt att_cb: msg_type %d, bd_addr %s", msg_type,
                     TRACE_BDADDR(bd_addr));

    enum PARAM_POS
    {
        CONN_HANDLE_POS = 0,
        CID_POS         = CONN_HANDLE_POS + 2,
        END_POS         = CID_POS + 2,
    };

    uint8_t report_data[END_POS] = {0};

    switch (msg_type)
    {
    case BT_ATT_MSG_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(bd_addr);
            if (!p_link)
            {
                return;
            }
            report_data[CONN_HANDLE_POS] = p_link->acl.conn_handle;
            report_data[CONN_HANDLE_POS + 1] = p_link->acl.conn_handle >> 8;
            report_data[CID_POS] = 0;
            report_data[CID_POS + 1] = 0 >> 8;

            app_report_event(CMD_PATH_UART, EVENT_XM_BR_ATT_CONNECTED, 0, report_data, END_POS);
        }
        break;

    case BT_ATT_MSG_DISCONN_CMPL:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(bd_addr);
            if (!p_link)
            {
                return;
            }
            report_data[CONN_HANDLE_POS] = p_link->acl.conn_handle;
            report_data[CONN_HANDLE_POS + 1] = p_link->acl.conn_handle >> 8;
            report_data[CID_POS] = 0;
            report_data[CID_POS + 1] = 0 >> 8;

            app_report_event(CMD_PATH_UART, EVENT_XM_BR_ATT_DISCONNECTED, 0, report_data, END_POS);
        }
        break;

    default:
        break;
    }

}

static void gatt_init(void)
{
}


void app_customer_bt_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                                uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_XM_SET_MODE:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            APP_PRINT_INFO1("app_handle_cmd_set_customer set mode cmd_ptr[2] = 0x%x", cmd_ptr[2]);
            bt_device_mode_set((T_BT_DEVICE_MODE)cmd_ptr[2]);
        }
        break;

    case CMD_BT_HFP_SCO_MAG:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            APP_PRINT_INFO1("CMD_BT_HFP_SCO_CFM cfm cmd_ptr[3] = 0x%x", cmd_ptr[3]);
            uint8_t app_index;
            uint8_t cfm;
            app_index = cmd_ptr[2];
            cfm = cmd_ptr[3];
            if (cfm == 0x00)
            {
#if F_APP_HFP_AG_SUPPORT
                bt_hfp_ag_audio_connect_cfm(app_db.br_link[app_index].bd_addr, false);
#elif F_APP_HFP_HF_SUPPORT
                bt_hfp_audio_connect_cfm(app_db.br_link[app_index].bd_addr, false);
#endif
            }
            else if (cfm == 0x01)
            {
#if F_APP_HFP_AG_SUPPORT
                bt_hfp_ag_audio_connect_cfm(app_db.br_link[app_index].bd_addr, true);
#elif F_APP_HFP_HF_SUPPORT
                bt_hfp_audio_connect_cfm(app_db.br_link[app_index].bd_addr, true);
#endif
            }
            else if (cfm == 0x02)
            {
                bt_hfp_audio_disconnect_req(app_db.br_link[app_index].bd_addr);
            }
        }
        break;

    case CMD_XM_SNIFF_MODE_CTRL:
        {
            T_APP_BR_LINK *p_link = NULL;
            uint8_t app_link_id = 0xff;
            bool sniff_mode_enabled = false;
            uint16_t sniff_mode_inv = 0;
            sniff_mode_enabled = cmd_ptr[2];
            if (sniff_mode_enabled)
            {
                sniff_mode_inv = cmd_ptr[3] | (cmd_ptr[4] << 8);
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            APP_PRINT_TRACE2("CMD_XM_SNIFF_MODE_CTRL sniff_mode_enabled = 0x%x, sniff_mode_inv = 0x%x",
                             sniff_mode_enabled,
                             sniff_mode_inv);

            for (app_link_id = 0; app_link_id < MAX_BR_LINK_NUM; app_link_id++)
            {
                if (app_link_check_b2s_link_by_id(app_link_id))
                {
                    p_link = &app_db.br_link[app_link_id];

                    if (sniff_mode_enabled)
                    {
                        APP_PRINT_TRACE0("CMD_XM_SNIFF_MODE_CTRL enable");

                        bt_sniff_mode_enable(p_link->bd_addr, ((sniff_mode_inv * 8 / 5) & 0xFFFE) - 16,
                                             ((sniff_mode_inv * 8 / 5) & 0xFFFE) + 16, 0, 0);
                    }
                    else
                    {
                        APP_PRINT_TRACE0("CMD_XM_SNIFF_MODE_CTRL disable");
                        bt_sniff_mode_disable(p_link->bd_addr);
                    }
                }
            }

        }
        break;

#if (TARGET_RTL8773DO || TARGET_RTL8773DFL)
    case CMD_XM_BT_SET_ADDR:
        {
            ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#else
    case CMD_XM_BT_SET_ADDR:
        {
            if (cfg_update_mac(&cmd_ptr[2]) != true)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

    case CMD_XM_SPP_DATA_TRANSFER:
        {
            uint8_t app_link_id;
            uint16_t pkt_len;
            uint8_t  *pkt_ptr;

            app_link_id = cmd_ptr[2];
            pkt_len     = (cmd_ptr[3] | (cmd_ptr[4] << 8));
            pkt_ptr     = &cmd_ptr[5];
            vd_spp_data_send(app_link_id, pkt_ptr, pkt_len);

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_INQUIRY:
        {
            if (cmd_ptr[2] == 0x00)
            {
                if (cmd_ptr[3] > 0 && cmd_ptr[3] <= 0x30)
                {
                    bool res = bt_inquiry_start(false, cmd_ptr[3]);
                    APP_PRINT_INFO1("app_handle_cmd_set_customer cmd inquiry start res = 0x%x", res);
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                }
            }
            else if (cmd_ptr[2] == 0x001)
            {
                bt_inquiry_stop();
                APP_PRINT_INFO0("app_handle_cmd_set_customer cmd inquiry stop");
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_USER_CFM_REQ:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            APP_PRINT_INFO1("CMD_XM_USER_CFM_REQ cfm cmd_ptr[3] = 0x%x", cmd_ptr[3]);
            uint8_t app_index;
            uint8_t cfm;
            app_index = cmd_ptr[2];
            cfm = cmd_ptr[3];
            if (cfm == 0x00)
            {
                gap_br_user_cfm_req_cfm(app_db.br_link[app_index].bd_addr, GAP_CFM_CAUSE_REJECT);
                T_APP_BR_LINK *p_link = NULL;

                p_link = app_link_find_br_link(app_db.br_link[app_index].bd_addr);
                if (p_link != NULL)
                {
                    gap_br_send_acl_disconn_req(app_db.br_link[app_index].bd_addr);
                }
            }
            else if (cfm == 0x01)
            {
                gap_br_user_cfm_req_cfm(app_db.br_link[app_index].bd_addr, GAP_CFM_CAUSE_ACCEPT);
            }
        }
        break;

    default:
        break;
    }
}


void app_customer_bt_user_cfm_rpt(uint8_t bd_addr[6], bool just_works, uint32_t disp_val)
{
    enum PARAM_POS
    {
        ADDR_POS        = 0,
        JUSTWORKS_POS   = ADDR_POS + 6,
        DISP_VAL_POS    = JUSTWORKS_POS + 1,
        APP_LINK_ID_POS = DISP_VAL_POS + 4,
        END_POS         = APP_LINK_ID_POS + 1
    };

    uint8_t buf[END_POS] = {0};

    memcpy(&buf[ADDR_POS], bd_addr, 6);
    memcpy(&buf[JUSTWORKS_POS], &just_works, 1);
    memcpy(&buf[DISP_VAL_POS], &disp_val, 4);
    T_APP_BR_LINK *p_link = app_link_find_br_link(bd_addr);
    if (!p_link)
    {
        return;
    }
    buf[APP_LINK_ID_POS] = p_link->id;
    app_report_event(CMD_PATH_UART, EVENT_XM_USER_CONFIRMATION_REQ, p_link->id, buf, sizeof(buf));
}


void app_customer_bt_init(void)
{
    spp_init();
    gatt_init();
}


#endif
