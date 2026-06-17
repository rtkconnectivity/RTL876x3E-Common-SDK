/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "console.h"
#include "os_mem.h"
#include "os_sched.h"
#include "pm.h"
#include "console.h"
#include "gap_br.h"
#include "btm.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "bt_pan.h"
#include "app_bt_pan_demo_link.h"
#include "app_bt_pan_demo.h"
#include "bnepif.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"


extern uint8_t bd_addr_local[6];

#if F_APP_PAN_PANU_SUPPORT
static const uint8_t pan_panu_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x76,//0x59,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PANU >> 8),
    (uint8_t)(UUID_PANU),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x1E,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_BNEP >> 8),
    (uint8_t)(PSM_BNEP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x14,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_BNEP >> 8),
    (uint8_t)(UUID_BNEP),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x00,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0C,
    SDP_UNSIGNED_TWO_BYTE,
    0x08,
    0x00,
    SDP_UNSIGNED_TWO_BYTE,
    0x08,
    0x06,
    SDP_UNSIGNED_TWO_BYTE,
    0x81,
    0x00,
    SDP_UNSIGNED_TWO_BYTE,
    0x86,
    0xdd,

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP),

    //Attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)(SDP_LANG_ENGLISH),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)(SDP_CHARACTER_UTF8),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)(SDP_BASE_LANG_OFFSET),

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PANU >> 8),
    (uint8_t)(UUID_PANU),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0100 >> 8),
    (uint8_t)(0x0100),

    //Attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0C,
    'R', 'e', 'a', 'l', 't', 'e', 'k', ' ', 'P', 'A', 'N', 'U',

    //Attribute SDP_ATTR_SRV_DESC
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0C,
    'R', 'e', 'a', 'l', 't', 'e', 'k', ' ', 'P', 'A', 'N', 'U',

    //Attribute SDP_ATTR_SECURITY_DESC
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SECURITY_DESC >> 8),
    (uint8_t)SDP_ATTR_SECURITY_DESC,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,
    0x00
};
#endif

#if F_APP_PAN_NAP_SUPPORT
static const uint8_t pan_nap_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x82,//0x59,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_NAP >> 8),
    (uint8_t)(UUID_NAP),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x1B,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_BNEP >> 8),
    (uint8_t)(PSM_BNEP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_BNEP >> 8),
    (uint8_t)(UUID_BNEP),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x00,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    0x08,
    0x00,
    SDP_UNSIGNED_TWO_BYTE,
    0x08,
    0x06,
    SDP_UNSIGNED_TWO_BYTE,
    0x81,
    0x00,
    SDP_UNSIGNED_TWO_BYTE,
    0x86,
    0xdd,

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP),

    //Attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)(SDP_LANG_ENGLISH),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)(SDP_CHARACTER_UTF8),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)(SDP_BASE_LANG_OFFSET),

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_NAP >> 8),
    (uint8_t)(UUID_NAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0100 >> 8),
    (uint8_t)(0x0100),

    //Attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0B,
    'R', 'e', 'a', 'l', 't', 'e', 'k', ' ', 'N', 'A', 'P',

    //Attribute SDP_ATTR_SRV_DESC
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0B,
    'R', 'e', 'a', 'l', 't', 'e', 'k', ' ', 'N', 'A', 'P',

    //Attribute SDP_ATTR_SECURITY_DESC
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SECURITY_DESC >> 8),
    (uint8_t)SDP_ATTR_SECURITY_DESC,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,
    0x01,

    //Attribute SDP_ATTR_NET_ACCESS_TYPE
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_NET_ACCESS_TYPE >> 8),
    (uint8_t)SDP_ATTR_NET_ACCESS_TYPE,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,
    0x05,

    //Attribute SDP_ATTR_MAX_NET_ACCESS_RATE
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_MAX_NET_ACCESS_RATE >> 8),
    (uint8_t)SDP_ATTR_MAX_NET_ACCESS_RATE,
    SDP_UNSIGNED_FOUR_BYTE,
    0x00,
    0x13,
    0x12,
    0xd0
};
#endif

#if F_APP_PAN_GN_SUPPORT
static const uint8_t pan_gn_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x77,//0x59,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_GN >> 8),
    (uint8_t)(UUID_GN),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x1B,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_BNEP >> 8),
    (uint8_t)(PSM_BNEP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_BNEP >> 8),
    (uint8_t)(UUID_BNEP),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x00,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    0x08,
    0x00,
    SDP_UNSIGNED_TWO_BYTE,
    0x08,
    0x06,
    SDP_UNSIGNED_TWO_BYTE,
    0x81,
    0x00,
    SDP_UNSIGNED_TWO_BYTE,
    0x86,
    0xdd,

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP),

    //Attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)(SDP_LANG_ENGLISH),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)(SDP_CHARACTER_UTF8),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)(SDP_BASE_LANG_OFFSET),

    //Attribute SDP_ATTR_SRV_AVAILABILITY
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_AVAILABILITY >> 8),
    (uint8_t)SDP_ATTR_SRV_AVAILABILITY,
    SDP_UNSIGNED_ONE_BYTE,
    0xff,

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_GN >> 8),
    (uint8_t)(UUID_GN),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0100 >> 8),
    (uint8_t)(0x0100),

    //Attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0A,
    'R', 'e', 'a', 'l', 't', 'e', 'k', ' ', 'G', 'N',

    //Attribute SDP_ATTR_SRV_DESC
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_DESC + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0A,
    'R', 'e', 'a', 'l', 't', 'e', 'k', ' ', 'G', 'N',

    //Attribute SDP_ATTR_SECURITY_DESC
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SECURITY_DESC >> 8),
    (uint8_t)SDP_ATTR_SECURITY_DESC,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,
    0x01
};
#endif

static void app_bt_pan_demo_cback(T_BT_PAN_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_PAN_EVENT_PARAM *param = event_buf;
    T_APP_BT_PAN_DEMO_LINK *p_link;

    switch (event_type)
    {
    case BT_PAN_EVENT_CONN_IND:
        p_link = app_bt_pan_demo_find_link(param->pan_conn_ind.bd_addr);
        if (p_link != NULL)
        {
            bt_pan_connect_cfm(app_db.local_addr, p_link->bd_addr, true);
        }
        break;

    case BT_PAN_EVENT_SETUP_CONN_IND:
        p_link = app_bt_pan_demo_find_link(param->pan_conn_ind.bd_addr);
        if (p_link != NULL)
        {
            bt_pan_setup_connection_rsp(p_link->bd_addr, 0);
        }
        break;

    case BT_PAN_EVENT_CONN_CMPL:
        {
            char *temp_buff = "PAN Connected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
            p_link = app_bt_pan_demo_find_link(param->pan_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                memcpy(app_db.remote_addr, param->pan_conn_cmpl.bd_addr, 6);
            }

            bnepif_netif_up(app_db.remote_addr);
            bnepif_dhcp_start();
        }

        break;

    case BT_PAN_EVENT_DISCONN_CMPL:
        {
            char *temp_buff = "PAN Disconnected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
            bnepif_netif_down();
        }
        break;

    case BT_PAN_EVENT_ETHERNET_PACKET_IND:
        APP_PRINT_TRACE1("app_bt_pan_demo_cback: BT_PAN_EVENT_ETHERNET_PACKET_IND len 0x%x",
                         param->pan_ethernet_packet_ind.len);
        bnepif_low_level_input(param->pan_ethernet_packet_ind.buf, param->pan_ethernet_packet_ind.len);
        break;

    default:
        break;
    }
}

void app_bt_pan_demo_init(void)
{
    int32_t ret = 0;
#if F_APP_PAN_PANU_SUPPORT
    if (bt_sdp_record_add((void *)pan_panu_sdp_record) == false)
    {
        ret = -1;
        goto fail_sdp_add;
    }
#endif

#if F_APP_PAN_NAP_SUPPORT
    if (bt_sdp_record_add((void *)pan_nap_sdp_record) == false)
    {
        ret = -1;
        goto fail_sdp_add;
    }
#endif

#if F_APP_PAN_GN_SUPPORT
    if (bt_sdp_record_add((void *)pan_gn_sdp_record) == false)
    {
        ret = -1;
        goto fail_sdp_add;
    }
#endif

    if (bt_pan_init() == false)
    {
        ret = 2;
        goto fail_init;
    }

    bt_pan_cback_register(app_bt_pan_demo_cback);

    tcpip_init(NULL, NULL);
    bnepif_init(bd_addr_local, bt_pan_send);

    return;

fail_init:
fail_sdp_add:
    APP_PRINT_ERROR1("app_bt_pan_demo_init: failed %d", ret);
}

bool app_bt_pan_demo_connect(uint8_t *bd_addr)
{
    T_BT_SDP_UUID_DATA uuid;

    uuid.uuid_16 = UUID_NAP;

    return bt_sdp_discov_start(bd_addr, BT_SDP_UUID16, uuid);
}

bool app_bt_pan_demo_disconnect(uint8_t *bd_addr)
{
    T_APP_BT_PAN_DEMO_LINK *p_link;

    p_link = app_bt_pan_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_pan_disconnect_req(bd_addr);
    }

    return false;
}

bool app_bt_pan_demo_setup_onn_rsp(uint8_t *bd_addr)
{
    return bt_pan_setup_connection_rsp(bd_addr, 0x00);
}

bool app_bt_pan_demo_filter_net_type_set(uint8_t *bd_addr)
{
    uint16_t start_array[] = {0x86dd};
    uint16_t end_array[] = {0x86dd};

    return bt_pan_filter_net_type_set(bd_addr, 1, start_array, end_array);
}

bool app_bt_pan_demo_filter_multi_addr_set(uint8_t *bd_addr)
{
    uint8_t start_array[][6] = {{0x03, 0x00, 0x00, 0x20, 0x00, 0x00}};
    uint8_t end_array[][6] = {{0x03, 0x00, 0x00, 0x20, 0x00, 0x00}};

    return bt_pan_filter_multi_addr_set(bd_addr, 1, start_array, end_array);
}

bool app_bt_pan_demo_send(uint8_t *bd_addr, uint8_t type)
{
    uint8_t  buf[36] = {0x03, 0x00, 0x3c, 0x3b, 0x3a, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30, 0x2f, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 0x1f, 0x1e, 0x1d, 0x1c, 0x1b};
    uint8_t  addr[6] = {0};
    uint8_t *new_buf;

    new_buf = calloc(1, 48);
    if (new_buf == NULL)
    {
        return false;
    }

    if (type == 0)
    {
        memcpy(new_buf, addr, 6);
        memcpy(new_buf + 6, addr, 6);
        memcpy(new_buf + 12, buf, 36);
    }

    if (type == 2)
    {
        memcpy(new_buf, app_db.remote_addr, 6);
        memcpy(new_buf + 6, app_db.local_addr, 6);
        memcpy(new_buf + 12, buf, 36);
    }

    if (type == 3)
    {
        memcpy(new_buf, app_db.remote_addr, 6);
        memcpy(new_buf + 6, addr, 6);
        memcpy(new_buf + 12, buf, 36);
    }

    if (type == 4)
    {
        memcpy(new_buf, addr, 6);
        memcpy(new_buf + 6, app_db.local_addr, 6);
        memcpy(new_buf + 12, buf, 36);
    }

    bt_pan_send(bd_addr, new_buf, 46);
    free(new_buf);
    return true;
}

bool app_bt_pan_demo_arp(uint8_t *bd_addr)
{
    uint8_t  buf[42] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x13, 0x22, 0x56, 0xab, 0xbc, 0xdd, 0x08, 0x06, 0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x13, 0x22, 0x56, 0xab, 0xbc, 0xdd, 0xc0, 0xa8, 0xa7, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8, 0xa8, 0x64};

    bt_pan_send(bd_addr, buf, 42);
    return true;
}

bool app_bt_pan_demo_arp_v6(uint8_t *bd_addr)
{
    uint8_t  buf[86] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdd, 0xbc, 0xab, 0x56, 0x22, 0x13, 0x86, 0xdd, 0x60, 0x00, 0x00, 0x00, 0x00, 0x20, 0x3a, 0xff, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5d, 0x89, 0xed, 0xde, 0x11, 0x07, 0xee, 0x5d, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0xed, 0xde, 0x11, 0x07, 0x11, 0x5d, 0x87, 0x00, 0x73, 0x2a, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0xed, 0xde, 0x11, 0x07, 0x11, 0x5d, 0x01, 0x01, 0x13, 0x22, 0x56, 0xab, 0xbc, 0xdd};

    bt_pan_send(bd_addr, buf, 86);
    return true;
}

bool app_bt_pan_demo_ping(uint8_t *bd_addr)
{
    uint8_t  buf[54] = {0x00, 0x1b, 0xdc, 0x08, 0xe5, 0xa9, 0xdd, 0xbc, 0xab, 0x56, 0x22, 0x13, 0x08, 0x00, 0x45, 0x00, 0x00, 0x28, 0x00, 0x29, 0x00, 0x00, 0x80, 0x01, 0x69, 0x5e, 0xc0, 0xa8, 0xa7, 0x98, 0xc0, 0xa8, 0xa8, 0x64, 0x08, 0x00, 0x87, 0x85, 0x00, 0x01, 0x00, 0x09, 0x50, 0x74, 0x73, 0x50, 0x74, 0x73, 0x50, 0x74, 0x73, 0x50, 0x74, 0x73};

    bt_pan_send(bd_addr, buf, 54);
    return true;
}

bool app_bt_pan_demo_ping_v6(uint8_t *bd_addr)
{
    uint8_t  buf[62] = {0x00, 0x1b, 0xdc, 0x08, 0xe5, 0xa9, 0xdd, 0xbc, 0xab, 0x56, 0x22, 0x13, 0x86, 0xdd, 0x60, 0x00, 0x00, 0x00, 0x00, 0x30, 0x3a, 0x01, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5d, 0x89, 0xed, 0xde, 0x11, 0x07, 0xee, 0x5d, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0xed, 0xde, 0x11, 0x07, 0x11, 0x5d, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09};

    bt_pan_send(bd_addr, buf, 62);
    return true;
}

bool app_bt_pan_demo_ping_reply(uint8_t *bd_addr)
{
    uint8_t  buf[54] = {0x00, 0x1b, 0xdc, 0x08, 0xe5, 0xa9, 0xdd, 0xbc, 0xab, 0x56, 0x22, 0x13, 0x08, 0x00, 0x45, 0x00, 0x00, 0x28, 0x00, 0x29, 0x00, 0x00, 0x80, 0x01, 0x69, 0x5e, 0xc0, 0xa8, 0xa7, 0x98, 0xc0, 0xa8, 0xa8, 0x64, 0x00, 0x00, 0x8f, 0x85, 0x00, 0x01, 0x00, 0x09, 0x50, 0x74, 0x73, 0x50, 0x74, 0x73, 0x50, 0x74, 0x73, 0x50, 0x74, 0x73};

    bt_pan_send(bd_addr, buf, 54);
    return true;
}

bool app_bt_pan_demo_ping_reply_v6(uint8_t *bd_addr)
{
    uint8_t  buf[62] = {0x00, 0x1b, 0xdc, 0x08, 0xe5, 0xa9, 0xdd, 0xbc, 0xab, 0x56, 0x22, 0x13, 0x86, 0xdd, 0x60, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x3a, 0x01, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5d, 0x89, 0xed, 0xde, 0x11, 0x07, 0xee, 0x5d, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0xed, 0xde, 0x11, 0x07, 0x11, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09};

    bt_pan_send(bd_addr, buf, 62);
    return true;
}
