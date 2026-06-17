/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "gap.h"
#include "btm.h"
#include "bt_bond.h"
#include "bt_spp.h"
#include "gap_br.h"
#include "app_bt_spp_demo_gap.h"
#include "app_bt_spp_demo_sdp.h"
#include "app_bt_spp_demo_link.h"

static bool is_spp_sdp_ok = false;
static uint8_t remote_server_channel = 0;
static uint8_t local_server_chann = 0;

static void spp_demo_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_SPP_DEMO_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {

        }
        break;

    case BT_EVENT_INQUIRY_RESULT:
        {

        }
        break;

    case BT_EVENT_ACL_CONN_IND:
        {
            p_link = spp_demo_find_link(param->acl_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_acl_conn_reject(param->acl_conn_ind.bd_addr, BT_ACL_REJECT_UNACCEPTABLE_ADDR);
            }
            else
            {
                bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
            }
        }
        break;

    case BT_EVENT_LINK_KEY_REQ:
        {
            uint8_t link_key[16];
            T_BT_LINK_KEY_TYPE type;

            if (bt_bond_key_get(param->link_key_req.bd_addr, link_key, (uint8_t *)&type))
            {
                bt_link_key_cfm(param->link_key_req.bd_addr, true, type, link_key);
            }
            else
            {
                bt_link_key_cfm(param->link_key_req.bd_addr, false, type, link_key);
            }
        }
        break;

    case BT_EVENT_LINK_PIN_CODE_REQ:
        {
            uint8_t pin_code[4] = {1, 2, 3, 4};
            bt_link_pin_code_cfm(param->link_pin_code_req.bd_addr, pin_code, 4, true);
        }
        break;

    case BT_EVENT_LINK_KEY_INFO:
        break;

    case BT_EVENT_LINK_USER_CONFIRMATION_REQ:
        {
            gap_br_user_cfm_req_cfm(param->link_user_confirmation_req.bd_addr, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            spp_demo_alloc_link(param->acl_conn_success.bd_addr);
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            p_link = spp_demo_find_link(param->acl_conn_disconn.bd_addr);
            if (p_link != NULL)
            {
                spp_demo_free_link(p_link);
            }
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            p_link = spp_demo_find_link(param->sdp_attr_info.bd_addr);
            if (p_link != NULL)
            {
                uint8_t temp_local_server_chann = 0;

                if (bt_spp_registered_uuid_check((T_BT_SPP_UUID_TYPE)param->sdp_attr_info.info.srv_class_uuid_type,
                                                 (T_BT_SPP_UUID_DATA *)(&param->sdp_attr_info.info.srv_class_uuid_data), &temp_local_server_chann))
                {
                    is_spp_sdp_ok = true;
                    local_server_chann = temp_local_server_chann;
                    remote_server_channel = param->sdp_attr_info.info.server_channel;
                }
            }
        }
        break;

    case BT_EVENT_SDP_DISCOV_CMPL:
        {
            p_link = spp_demo_find_link(param->sdp_discov_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (is_spp_sdp_ok)
                {
                    if (param->sdp_discov_cmpl.cause == 0x00)
                    {
                        is_spp_sdp_ok = false;
                        bt_spp_connect_req(param->sdp_discov_cmpl.bd_addr, remote_server_channel,
                                           SPP_DEMO_SPP_DEFAULT_MTU_SIZE,
                                           SPP_DEMO_SPP_DEFAULT_CREDITS,
                                           local_server_chann);
                    }
                }
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("spp_demo_gap_bt_cback: event_type 0x%04x", event_type);
    }
}

void spp_demo_gap_init(void)
{
    bt_mgr_cback_register(spp_demo_gap_bt_cback);
}
