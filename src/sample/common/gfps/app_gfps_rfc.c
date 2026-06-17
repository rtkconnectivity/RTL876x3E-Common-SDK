/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_GFPS_COMMON_BASIC_FEATURE_SUPPORT
#include "stdlib.h"
#include "trace.h"
#include "bt_gfps.h"
#include "app_gfps.h"
#include "app_gfps_cfg.h"
#include "app_gfps_rfc.h"
#include "app_gfps_link_util.h"
#include "app_main.h"
#include "stdlib.h"
#if F_GFPS_COMMON_BATTERY_INFO_REPORT
#include "app_gfps_battery.h"
#endif
#include "app_gfps_msg.h"
#include "app_sdp.h"
#include "bt_sdp.h"
#if F_GFPS_COMMON_FINDER_SUPPORT
#include "app_gfps_finder.h"
#endif

static const uint8_t gfps_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x4F,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID128_HDR,
    0xdf, 0x21, 0xfe, 0x2c, 0x25, 0x15, 0x4f, 0xdb, 0x88, 0x86, 0xf1, 0x2c, 0x4d, 0x67, 0x92, 0x7c,

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0c,
    SDP_DATA_ELEM_SEQ_HDR,
    03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)UUID_L2CAP,
    SDP_DATA_ELEM_SEQ_HDR,
    0x05,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_RFCOMM >> 8),
    (uint8_t)UUID_RFCOMM,
    SDP_UNSIGNED_ONE_BYTE,
    RFC_GFPS_CHANN_NUM,

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,

    //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)SDP_LANG_ENGLISH,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)SDP_CHARACTER_UTF8,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)SDP_BASE_LANG_OFFSET,

    //attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x09,
    0x67, 0x66, 0x70, 0x73, 0x20, 0x74, 0x65, 0x73, 0x74 //"gfps test"
};

void app_gfps_rfc_sdp_record_add(void)
{
    if (app_cfg_const.supported_profile_mask & GFPS_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)gfps_sdp_record);
    }
    else
    {
        APP_PRINT_ERROR0("app_gfps_rfc_sdp_record_add: not support");
    }
}

void app_gfps_rfc_cback(uint8_t *bd_addr, T_BT_RFC_MSG_TYPE msg_type, void *msg_buf)
{
    T_APP_BR_LINK *p_link;
    bool handle = true;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    switch (msg_type)
    {
    case BT_RFC_MSG_CONN_IND:
        {
            T_BT_RFC_CONN_IND *p_ind = (T_BT_RFC_CONN_IND *)msg_buf;

            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                bt_gfps_connect_cfm(p_link->bd_addr, p_ind->local_server_chann, true, p_ind->frame_size, 7);
            }
            else
            {
                APP_PRINT_TRACE0("app_gfps_rfc_cback: no acl link found");
                bt_gfps_connect_cfm(bd_addr, p_ind->local_server_chann, false, p_ind->frame_size, 7);
            }
        }
        break;

    case BT_RFC_MSG_CONN_CMPL:
        {
            T_BT_RFC_CONN_CMPL *p_cmpl = (T_BT_RFC_CONN_CMPL *)msg_buf;

            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                uint8_t ble_addr[6];
                //uint8_t battery[3] = {app_db.local_batt_level, app_db.remote_batt_level, app_db.case_battery};

                p_link->gfps_rfc_chann = p_cmpl->local_server_chann;
                app_bt_policy_msg_prof_conn(bd_addr, GFPS_PROFILE_MASK);
                bt_gfps_send_model_id(0xFF, 0, bd_addr, p_cmpl->local_server_chann,
                                      extend_app_cfg_const.gfps_model_id,
                                      GFPS_MODEL_ID_LEN);

                app_gfps_get_ble_addr(ble_addr);
                app_gfps_msg_reverse_data(ble_addr, 6);
                bt_gfps_send_ble_addr(0xFF, 0, bd_addr, p_cmpl->local_server_chann, ble_addr);
#if F_GFPS_COMMON_BATTERY_INFO_REPORT
                app_gfps_battery_info_report(GFPS_BATTERY_REPORT_RFCOMM);
#endif

                bt_gfps_send_firmware_revision(0xFF, 0, bd_addr, p_cmpl->local_server_chann,
                                               extend_app_cfg_const.gfps_version,
                                               sizeof(extend_app_cfg_const.gfps_version));

#if CONFIG_REALTEK_GFPS_SASS_SUPPORT
                uint8_t pair_idx_mapping;
                app_gfps_msg_report_session_nonce(bd_addr, p_link->gfps_rfc_chann);
                app_gfps_msg_notify_connection_status();
#endif
#if F_GFPS_COMMON_FINDER_SUPPORT
                if (extend_app_cfg_const.gfps_finder_support)
                {
                    app_gfps_finder_send_eddystone_capability(bd_addr, p_cmpl->local_server_chann);
                }
#endif
            }
        }
        break;

    case BT_RFC_MSG_DISCONN_CMPL:
        {
            T_BT_RFC_DISCONN_CMPL *p_info = (T_BT_RFC_DISCONN_CMPL *)msg_buf;

#if F_GFPS_COMMON_FINDER_SUPPORT
            if (extend_app_cfg_const.gfps_finder_support)
            {
                //keep ringing left and right ear Find Me tone
            }
            else
#endif
            {
                /*stop left and right ear Find Me tone when b2s disconnect*/
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
                    uint8_t event =  GFPS_ALL_STOP;
#if F_GFPS_COMMON_TWS_MODE
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_GFPS_MSG, GFPS_REMOTE_MSG_FINDME_STOP,
                                                        &event, 1);
#endif
                }
                else
                {
                    app_gfps_msg_ring_stop();
                }
                T_GFPS_MSG_RING_STATE ring_state = GFPS_MSG_RING_STOP;
                app_gfps_msg_set_ring_state(ring_state);
            }

            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                app_bt_policy_msg_prof_disconn(bd_addr, GFPS_PROFILE_MASK, p_info->cause);
                if (p_link->p_gfps_cmd != NULL)
                {
                    free(p_link->p_gfps_cmd);
                    p_link->p_gfps_cmd = NULL;
                }
            }
        }
        break;

    case BT_RFC_MSG_DATA_IND:
        {
            T_BT_RFC_DATA_IND *p_ind = (T_BT_RFC_DATA_IND *)msg_buf;

            p_link = app_link_find_br_link(bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_gfps_rfc_cback: no acl link found");
                return;
            }

            bt_rfc_credits_give(bd_addr, p_ind->local_server_chann, 1);
            app_gfps_msg_loop_check_data_complete(bd_addr, p_ind->buf, p_ind->length,
                                                  p_ind->local_server_chann);
        }
        break;

    case BT_RFC_MSG_CREDIT_INFO:
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_gfps_rfc_cback: msg_type 0x%02x", msg_type);
    }
}
#endif
