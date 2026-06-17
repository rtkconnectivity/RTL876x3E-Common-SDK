/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "gap.h"
#include "btm.h"
#include "gap_br.h"
#include "bt_bond.h"
#include "bt_pan.h"
#include "app_bt_pan_demo_link.h"
#include "app_bt_pan_demo_gap.h"

static void app_bt_pan_demo_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BT_PAN_DEMO_LINK *p_link;
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
            p_link = app_bt_pan_demo_find_link(param->acl_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_acl_conn_reject(param->acl_conn_ind.bd_addr, BT_ACL_REJECT_UNACCEPTABLE_ADDR);
            }
            else
            {
                bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
            }

            gap_br_set_radio_mode(GAP_RADIO_MODE_NONE_DISCOVERABLE, false, 0);
        }
        break;

    case BT_EVENT_LINK_KEY_INFO:
        {
            bt_bond_key_set(param->link_key_info.bd_addr, param->link_key_info.link_key,
                            param->link_key_info.key_type);
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

    case BT_EVENT_LINK_USER_CONFIRMATION_REQ:
        {
            gap_br_user_cfm_req_cfm(param->link_user_confirmation_req.bd_addr, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            app_bt_pan_demo_alloc_link(param->acl_conn_success.bd_addr);
        }
        break;

    case BT_EVENT_ACL_CONN_ENCRYPTED:
        {

        }
        break;


    case BT_EVENT_ACL_CONN_DISCONN:
        {
            p_link = app_bt_pan_demo_find_link(param->acl_conn_disconn.bd_addr);
            if (p_link != NULL)
            {
                app_bt_pan_demo_free_link(p_link);
            }
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            T_BT_SDP_ATTR_INFO *sdp_info = &param->sdp_attr_info.info;
            if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_NAP)
            {
                bt_pan_connect_req(app_db.local_addr, param->sdp_attr_info.bd_addr,
                                   BT_PAN_ROLE_PANU, BT_PAN_ROLE_NAP);
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_bt_pan_demo_gap_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_bt_pan_demo_gap_init(void)
{
    bt_mgr_cback_register(app_bt_pan_demo_gap_bt_cback);
}
