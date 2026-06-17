/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "console.h"
#include "btm.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "bt_map.h"
#include "app_bt_map_demo_link.h"
#include "app_bt_map_demo.h"

#define RFC_MAP_MNS_CHANN_NUM           20
#define L2CAP_MAP_MNS_PSM               0x1001

static const uint8_t map_mce_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x49,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_MSG_NOTIFICATION_SERVER >> 8),
    (uint8_t)(UUID_MSG_NOTIFICATION_SERVER),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x05,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_RFCOMM >> 8),
    (uint8_t)(UUID_RFCOMM),
    SDP_UNSIGNED_ONE_BYTE,
    RFC_MAP_MNS_CHANN_NUM,  //channel number
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_OBEX >> 8),
    (uint8_t)(UUID_OBEX),

    //Attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0B,
    'R', 'e', 'a', 'l', 't', 'e', 'k', '-', 'M', 'N', 'S',

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_MSG_ACCESS_PROFILE >> 8),
    (uint8_t)UUID_MSG_ACCESS_PROFILE,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x04,   //version 1.4

    //attribute SDP_ATTR_L2C_PSM
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_L2C_PSM) >> 8),
    (uint8_t)(SDP_ATTR_L2C_PSM),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(L2CAP_MAP_MNS_PSM >> 8),
    (uint8_t)(L2CAP_MAP_MNS_PSM),

    //attribute SDP_ATTR_MAP_SUPPERTED_FEATS
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_MAP_SUPPERTED_FEATS) >> 8),
    (uint8_t)(SDP_ATTR_MAP_SUPPERTED_FEATS),
    SDP_UNSIGNED_FOUR_BYTE,
    (uint8_t)(0x0000024F >> 24),
    (uint8_t)(0x0000024F >> 16),
    (uint8_t)(0x0000024F >> 8),
    (uint8_t)(0x0000024F)
};

static void app_map_demo_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    //T_APP_MAP_DEMO_LINK *p_link;

    switch (event_type)
    {
    case BT_EVENT_SDP_ATTR_INFO:
        {
            T_BT_SDP_ATTR_INFO *sdp_info = &param->sdp_attr_info.info;

            if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_MSG_ACCESS_SERVER)
            {
                bt_map_mas_connect_over_rfc_req(param->sdp_attr_info.bd_addr, sdp_info->server_channel);
            }
        }
        break;

    case BT_EVENT_MAP_MAS_CONN_CMPL:
        {
            char *temp_buff = "MAP Connected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case BT_EVENT_MAP_MAS_DISCONN_CMPL:
        {
            char *temp_buff = "MAP Disconnected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case BT_EVENT_MAP_MNS_CONN_IND:
        {
            bt_map_mns_connect_cfm(param->map_mns_conn_ind.bd_addr, true);
        }
        break;

    case BT_EVENT_MAP_MNS_CONN_CMPL:
        {
            char *temp_buff = "MAP Message Notification ON!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case BT_EVENT_MAP_MNS_DISCONN_CMPL:
        {
            char *temp_buff = "MAP Message Notification OFF!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case BT_EVENT_MAP_GET_FOLDER_LISTING_CMPL:
        {
            T_BT_EVENT_PARAM_MAP_GET_FOLDER_LISTING_CMPL *event;

            event = &(param->map_get_folder_listing_cmpl);
            if (!event->data_end)
            {
                bt_map_mas_get_continue(event->bd_addr);
            }
            else
            {
                char *temp_buff = "MAP Get Folder Listing Success!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    case BT_EVENT_MAP_GET_MSG_LISTING_CMPL:
        {
            T_BT_EVENT_PARAM_MAP_GET_MSG_LISTING_CMPL *event;

            event = &(param->map_get_msg_listing_cmpl);
            if (!event->data_end)
            {
                bt_map_mas_get_continue(event->bd_addr);
            }
            else
            {
                char *temp_buff = "MAP Get Message Listing Success!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    case BT_EVENT_MAP_GET_MSG_CMPL:
        {
            T_BT_EVENT_PARAM_MAP_GET_MSG_CMPL *event;

            event = &(param->map_get_msg_cmpl);
            if (!event->data_end)
            {
                bt_map_mas_get_continue(event->bd_addr);
            }
            else
            {
                char *temp_buff = "MAP Get Message Success!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    case BT_EVENT_MAP_MSG_NOTIFICATION:
        {
            T_BT_EVENT_PARAM_MAP_MSG_NOTIFICATION *event;

            event = &(param->map_msg_notification);
            if (!event->data_end)
            {
                bt_map_mns_send_event_rsp(event->bd_addr, BT_MAP_RSP_CONTINUE);
            }
            else
            {
                bt_map_mns_send_event_rsp(event->bd_addr, BT_MAP_RSP_SUCCESS);
            }
        }
        break;

    default:
        break;
    }
}

void app_map_demo_init(void)
{
    int32_t ret = 0;

    bt_sdp_record_add((void *)map_mce_sdp_record);

    if (bt_map_init(RFC_MAP_MNS_CHANN_NUM, L2CAP_MAP_MNS_PSM, 0x0000024F) == false)
    {
        ret = -1;
        goto fail_init;
    }

    bt_mgr_cback_register(app_map_demo_cback);

    return;

fail_init:
    APP_PRINT_ERROR1("app_map_demo_init: failed %d", ret);
}

bool app_map_demo_connect(uint8_t *bd_addr)
{
    T_BT_SDP_UUID_DATA uuid;

    uuid.uuid_16 = UUID_MSG_ACCESS_SERVER;

    return bt_sdp_discov_start(bd_addr, BT_SDP_UUID16, uuid);
}

bool app_map_demo_disconnect(uint8_t *bd_addr)
{
    T_APP_MAP_DEMO_LINK *p_link;

    p_link = app_map_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_map_mas_disconnect_req(bd_addr);
    }

    return false;
}

bool app_map_demo_msg_notification_set(uint8_t *bd_addr, bool enable)
{
    T_APP_MAP_DEMO_LINK *p_link;

    p_link = app_map_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_map_mas_msg_notification_set(bd_addr, enable);
    }

    return false;
}

bool app_map_demo_folder_set(uint8_t *bd_addr, uint8_t folder)
{
    T_APP_MAP_DEMO_LINK *p_link;

    p_link = app_map_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_map_mas_folder_set(bd_addr, (T_BT_MAP_FOLDER)folder);
    }

    return false;
}

bool app_map_demo_folder_listing_get(uint8_t *bd_addr)
{
    T_APP_MAP_DEMO_LINK *p_link;

    p_link = app_map_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_map_mas_folder_listing_get(bd_addr, 10, 0);
    }

    return false;
}

bool app_map_demo_msg_listing_get(uint8_t *bd_addr)
{
    T_APP_MAP_DEMO_LINK *p_link;

    p_link = app_map_demo_find_link(bd_addr);
    if (p_link != NULL)
    {
        //NULL terminated UNICODE : inbox
        uint8_t map_path_inbox[12] =
        {
            0x00, 0x69, 0x00, 0x6e, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x78, 0x00, 0x00
        };

        return bt_map_mas_msg_listing_get(bd_addr, map_path_inbox, sizeof(map_path_inbox), 20, 0);
    }

    return false;
}
