/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_queue.h"
#include "trace.h"
#include "gap.h"
#include "btm.h"
#include "gap_br.h"
#include "bt_bond.h"
#include "bt_sdp.h"
#include "bt_hid_host.h"
#include "app_bt_hid_demo_link.h"
#include "app_bt_hid_demo_gap.h"
#include "app_bt_hid_demo.h"

T_OS_QUEUE descriptor_list;
bool is_hid_sdp_ok = false;

static void app_bt_hid_demo_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BT_HID_DEMO_LINK *p_link;
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
            p_link = app_bt_hid_demo_find_link(param->acl_conn_ind.bd_addr);
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
            app_bt_hid_demo_alloc_link(param->acl_conn_success.bd_addr);
            //bt_device_mode_set(BT_DEVICE_MODE_IDLE);
        }
        break;

    case BT_EVENT_ACL_CONN_ENCRYPTED:
        {
            bt_sniff_mode_enable(param->acl_conn_encrypted.bd_addr, 36, 36, 0, 0);
        }
        break;


    case BT_EVENT_ACL_CONN_DISCONN:
        {
            p_link = app_bt_hid_demo_find_link(param->acl_conn_disconn.bd_addr);
            if (p_link != NULL)
            {
                app_bt_hid_demo_free_link(p_link);
            }
            bt_device_mode_set(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            p_link = app_bt_hid_demo_find_link(param->sdp_attr_info.bd_addr);
            if (p_link != NULL)
            {
                T_BT_SDP_ATTR_INFO *sdp_info = &param->sdp_attr_info.info;

                if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_HUMAN_INTERFACE_DEVICE_SERVICE)
                {
                    if (app_hid_host_descriptor_find(param->sdp_attr_info.bd_addr) == NULL)
                    {
                        uint8_t  *p_attr_start = sdp_info->p_attr;
                        uint8_t  *p_attr_end = p_attr_start + sdp_info->attr_len;
                        uint8_t  *p_attr = NULL;
                        uint8_t  *p_attr_param = NULL;
                        uint8_t  *p_elem = NULL;
                        uint8_t  *descriptor = NULL;
                        uint32_t  descriptor_len = 0;
                        uint8_t   type = 0;

                        p_attr = bt_sdp_attr_find(p_attr_start, p_attr_end - p_attr_start, SDP_ATTR_HID_DESC_LIST);
                        if (p_attr)
                        {
                            uint8_t loop = 1;

                            while ((p_elem = bt_sdp_elem_access(p_attr, p_attr_end - p_attr, loop)) != NULL)
                            {
                                p_attr_param = bt_sdp_elem_access(p_elem, p_attr_end - p_elem, 2);

                                if (p_attr_param)
                                {
                                    descriptor = bt_sdp_elem_decode(p_attr_param, p_attr_end - p_attr_param, &descriptor_len, &type);
                                    break;
                                }
                                loop++;
                            }
                        }
                        if (descriptor)
                        {
                            T_HID_HOST_DESCRIPTOR *hid_descriptor;

                            is_hid_sdp_ok = true;
                            hid_descriptor = app_hid_host_alloc_descriptor(param->sdp_attr_info.bd_addr,
                                                                           descriptor,
                                                                           descriptor_len);
                            if (p_link->hid_conn_cmpl)
                            {
                                bt_hid_host_descriptor_set(param->sdp_attr_info.bd_addr,
                                                           hid_descriptor->descriptor,
                                                           hid_descriptor->descriptor_len);
                            }
                        }
                    }
                }
            }
        }
        break;

    case BT_EVENT_SDP_DISCOV_CMPL:
        {
            p_link = app_bt_hid_demo_find_link(param->sdp_discov_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (is_hid_sdp_ok)
                {
                    if (param->sdp_discov_cmpl.cause == 0x00)
                    {
                        is_hid_sdp_ok = false;
                        if (p_link->hid_conn_cmpl == false)
                        {
                            bt_hid_host_connect_req(param->sdp_attr_info.bd_addr, BT_HID_HOST_REPORT_PROTO_MODE);
                        }
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
        APP_PRINT_INFO1("app_bt_hid_demo_gap_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_bt_hid_demo_gap_init(void)
{
    bt_mgr_cback_register(app_bt_hid_demo_gap_bt_cback);
}
