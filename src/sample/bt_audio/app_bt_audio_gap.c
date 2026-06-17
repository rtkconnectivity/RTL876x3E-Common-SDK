/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "console.h"
#include "btm.h"
#include "gap_br.h"
#include "bt_bond.h"
#include "app_bt_audio_link.h"
#include "app_bt_audio_gap.h"

static void app_bt_audio_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BT_AUDIO_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
#if (BT_AUDIO_DEMO_ROLE == 1)
            bt_inquiry_start(false, 10);
#endif
        }
        break;

    case BT_EVENT_INQUIRY_RESULT:
        {
            if (memcmp(app_db.remote_addr, param->inquiry_result.bd_addr, 6) == 0)
            {
                char *temp_buff = "Find Target Device!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));

                bt_inquiry_stop();
            }
        }
        break;

    case BT_EVENT_ACL_CONN_IND:
        {
            p_link = app_bt_audio_find_link(param->acl_conn_ind.bd_addr);
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

    case BT_EVENT_LINK_USER_CONFIRMATION_REQ:
        {
            gap_br_user_cfm_req_cfm(param->link_user_confirmation_req.bd_addr, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            app_bt_audio_alloc_link(param->acl_conn_success.bd_addr);
            bt_device_mode_set(BT_DEVICE_MODE_IDLE);
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            p_link = app_bt_audio_find_link(param->acl_conn_disconn.bd_addr);
            if (p_link != NULL)
            {
                app_bt_audio_free_link(p_link);
            }

            bt_device_mode_set(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_bt_audio_gap_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_bt_audio_gap_local_addr_set(uint8_t *bd_addr)
{
    gap_set_bd_addr(bd_addr);
}

void app_bt_audio_gap_remote_addr_set(uint8_t *bd_addr)
{
    memcpy(app_db.remote_addr, bd_addr, 6);
}

bool app_bt_audio_gap_local_bt_name_set(uint8_t *p_name, uint8_t  len)
{
    return bt_local_name_set(p_name, len);
}

bool app_bt_audio_gap_inquiry_start(void)
{
    return bt_inquiry_start(false, 10);
}

bool app_bt_audio_gap_device_mode_set(uint8_t value)
{
    return bt_device_mode_set((T_BT_DEVICE_MODE)value);
}

void app_bt_audio_gap_init(void)
{
    bt_mgr_cback_register(app_bt_audio_gap_bt_cback);
}
