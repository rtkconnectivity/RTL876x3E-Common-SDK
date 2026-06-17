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
#include "bt_iap.h"
#include "gap_br.h"
#include "app_bt_iap_demo_gap.h"
#include "app_bt_iap_demo_sdp.h"
#include "app_bt_iap_demo_link.h"

static bool is_iap_sdp_ok = false;
static uint8_t remote_server_channel = 0;
uint8_t iap_uuid[16] = {0x00, 0x00, 0x00, 0x00, 0xde, 0xca, 0xfa, 0xde, 0xde, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xca, 0xfe};
/**
 * @brief    BT Manager Gap related events are handled in this function
 * @note     Then the event handling function shall be called according to the
 *           event_type of T_BT_EVENT.
 * @param[in] event_type BT manager event type
 * @param[in] event_buf  Pointer to BT manager event buffer.
 * @param[in] buf_len    BT manager event buffer length.
 * @return   void
 */
static void iap_demo_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_IAP_DEMO_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            bt_device_mode_set(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
        }
        break;

    case BT_EVENT_INQUIRY_RESULT:
        {
        }
        break;

    case BT_EVENT_ACL_CONN_IND:
        {
            p_link = iap_demo_find_link(param->acl_conn_ind.bd_addr);
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
        {
            bt_bond_key_set(param->link_key_info.bd_addr, param->link_key_info.link_key,
                            param->link_key_info.key_type);
        }
        break;

    case BT_EVENT_LINK_USER_CONFIRMATION_REQ:
        {
            gap_br_user_cfm_req_cfm(param->link_user_confirmation_req.bd_addr, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            iap_demo_alloc_link(param->acl_conn_success.bd_addr);
            bt_device_mode_set(BT_DEVICE_MODE_IDLE);
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            p_link = iap_demo_find_link(param->acl_conn_disconn.bd_addr);
            if (p_link != NULL)
            {
                iap_demo_free_link(p_link);
            }
            bt_device_mode_set(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            if (param->sdp_attr_info.info.srv_class_uuid_type == BT_SDP_UUID128)
            {
                if (!memcmp(param->sdp_attr_info.info.srv_class_uuid_data.uuid_128, iap_uuid, 16))
                {
                    is_iap_sdp_ok = true;
                    remote_server_channel = param->sdp_attr_info.info.server_channel;
                }
            }
        }
        break;

    case BT_EVENT_SDP_DISCOV_CMPL:
        {
            if (is_iap_sdp_ok)
            {
                if (param->sdp_discov_cmpl.cause == 0x00)
                {
                    bt_iap_connect_req(param->sdp_discov_cmpl.bd_addr, remote_server_channel,
                                       IAP_DEMO_IAP_DEFAULT_MTU_SIZE, IAP_DEMO_IAP_DEFAULT_CREDITS);
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
        APP_PRINT_INFO1("iap_demo_gap_bt_cback: event_type 0x%04x", event_type);
    }
}

void iap_demo_gap_init(void)
{
    APP_PRINT_INFO0("iap_demo_gap_init");
    bt_mgr_cback_register(iap_demo_gap_bt_cback);
}

