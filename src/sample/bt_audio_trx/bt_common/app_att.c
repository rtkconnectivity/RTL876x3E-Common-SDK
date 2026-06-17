/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "bt_att.h"
#include "gap_chann.h"
#include "app_att.h"
#include "app_link_util_cs.h"
#include "app_bt_policy_api.h"
#include "gatt_builtin_services.h"

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
#include "bt_gatt_client.h"
#endif

#define POS_GATT_SERVICE_START_HANDLE   29

static uint8_t att_gatt_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x2b,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_GATT >> 8),
    (uint8_t)(UUID_GATT),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x13,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_ATT >> 8),
    (uint8_t)(PSM_ATT),
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_ATT >> 8),
    (uint8_t)(UUID_ATT),
    SDP_UNSIGNED_TWO_BYTE,
    0x00, /* start handle */
    0x01, /* start handle */
    SDP_UNSIGNED_TWO_BYTE,
    0x00, /* end handle */
    0x06, /* end handle */

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP),
};

void app_att_update_gatt_sdp(void)
{
    uint16_t starting_handle = 0x0000;
    uint16_t ending_handle = 0x0000;
    bool ret = gatts_get_service_handle_range(&starting_handle, &ending_handle);
    if (ret)
    {
        att_gatt_sdp_record[POS_GATT_SERVICE_START_HANDLE] = (uint8_t)(starting_handle >> 8);
        att_gatt_sdp_record[POS_GATT_SERVICE_START_HANDLE + 1] = (uint8_t)starting_handle;
        att_gatt_sdp_record[POS_GATT_SERVICE_START_HANDLE + 3] = (uint8_t)(ending_handle >> 8);
        att_gatt_sdp_record[POS_GATT_SERVICE_START_HANDLE + 4] = (uint8_t)ending_handle;
    }
    APP_PRINT_INFO2("app_att_update_gatt_sdp: starting_handle 0x%04x, ending_handle 0x%04x",
                    starting_handle, ending_handle);
}

void app_att_cb(uint8_t *bd_addr, T_BT_ATT_MSG_TYPE msg_type, void *p_msg)
{
    T_APP_BR_LINK *p_link;

    APP_PRINT_TRACE2("app_bt_att_cb: msg_type %d, bd_addr %s", msg_type, TRACE_BDADDR(bd_addr));

    switch (msg_type)
    {
    case BT_ATT_MSG_CONN_CMPL:
        {
            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                T_BT_ATT_CONN_CMPL *param = (T_BT_ATT_CONN_CMPL *)p_msg;

                app_bt_policy_msg_prof_conn(p_link->bd_addr, GATT_PROFILE_MASK);

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
                APP_PRINT_INFO3("BT_ATT_MSG_CONN_CMPL: conn_handle 0x%x, cid 0x%x, mtu_size 0x%x",
                                param->conn_handle, param->cid, param->mtu_size);
#if F_APP_RESET_DISCOVERY_BY_BLE_CONN
#if GATTC_TBL_STORAGE_SUPPORT
                gattc_tbl_storage_del(param->conn_handle);
#endif
#endif
                gatt_client_start_discovery_all(param->conn_handle, NULL);
#endif
            }
        }
        break;

    case BT_ATT_MSG_DISCONN_CMPL:
        {
            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                app_bt_policy_msg_prof_disconn(p_link->bd_addr, GATT_PROFILE_MASK, 0);
            }
        }
        break;

    default:
        break;
    }
}

void app_att_init(void)
{
    app_att_update_gatt_sdp();
    bt_sdp_record_add((void *)att_gatt_sdp_record);

    bt_att_init(app_att_cb);
}
