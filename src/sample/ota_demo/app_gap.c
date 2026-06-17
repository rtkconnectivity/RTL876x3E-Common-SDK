/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "bt_bond.h"
#include "gap.h"
#include "gap_br.h"
#include "app_gap.h"
#include "app_main.h"
#include "remote.h"
#include "btm.h"
#include "app_cfg.h"

static void app_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link = NULL;


    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            APP_PRINT_INFO0("BT_EVENT_READY");

            memcpy(app_db.factory_addr, param->ready.bd_addr, 6);
            memcpy(app_cfg_nv.bud_local_addr, app_db.factory_addr, 6);
        }
        break;

    case BT_EVENT_ACL_CONN_IND:
        {
            APP_PRINT_TRACE1("app_gap_bt_cback: conn ind device cod 0x%08x", param->acl_conn_ind.cod);

            p_link = app_link_find_br_link(param->acl_conn_ind.bd_addr);

            if (p_link != NULL)
            {
                bt_acl_conn_reject(param->acl_conn_ind.bd_addr, BT_ACL_REJECT_UNACCEPTABLE_ADDR);
            }
            else
            {
                bt_acl_conn_accept(param->acl_conn_ind.bd_addr, BT_LINK_ROLE_SLAVE);
            }
            //gap_br_set_radio_mode(GAP_RADIO_MODE_NONE_DISCOVERABLE, false, 0);
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
            app_link_alloc_br_link(param->acl_conn_success.bd_addr);
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            p_link = app_link_find_br_link(param->acl_conn_disconn.bd_addr);
            if (p_link != NULL)
            {
                app_link_free_br_link(p_link);
            }
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            APP_PRINT_INFO0("BT_EVENT_SDP_ATTR_INFO");
        }
        break;

    case BT_EVENT_SDP_DISCOV_CMPL:
        {
            APP_PRINT_INFO0("BT_EVENT_SDP_DISCOV_CMPL");
        }
        break;

    default:
        break;
    }
}

void app_gap_init(void)
{
    APP_PRINT_INFO0("app_gap_init");
    bt_mgr_cback_register(app_gap_bt_cback);
}
