/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include "trace.h"
#include "console.h"
#include "audio.h"
#include "btm.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "app_bt_audio_link.h"
#include "app_bt_audio_avrcp.h"

#if (BT_AUDIO_DEMO_ROLE == 1)
static const uint8_t avrcp_ct_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x3b,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Attribute length: 6 bytes
    //Service Class #0: A/V Remote Control
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER & 0xff),
    //Service Class #1: A/V Remote Control Controller

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 16 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0003 >> 8), //Category 1 Player / Recorder, Category 2
    (uint8_t)(0x0003 & 0xff),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
};

static const uint8_t avrcp_tg_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x38,//0x46,//0x5F,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03, //Attribute length: 3 bytes
    //Service Class #0: A/V Remote Control Target
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 16 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0001 >> 8), //Category 1
    (uint8_t)(0x0001 & 0xff),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
};
#else
static const uint8_t avrcp_ct_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x3B,//0x49,//0x62,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Attribute length: 6 bytes
    //Service Class #0: A/V Remote Control
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Service Class #1: A/V Remote Control Controller
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 12 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0001 >> 8), //Category 1 Player / Recorder
    (uint8_t)(0x0001),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
    /*
        //Attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST...it is used for SDP_ATTR_SRV_NAME
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

        //Attribute SDP_ATTR_PROVIDER_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x07, //Attribute length: 7 bytes
        0x52, 0x65, 0x61, 0x6C, 0x54, 0x65, 0x6B, //RealTek

        //Attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x08, //Attribute length: 8 bytes
        0x41, 0x56, 0x52, 0x43, 0x50, 0x20, 0x43, 0x54, //AVRCP CT
    */
};

static const uint8_t avrcp_tg_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x38,//0x46,//0x5F,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03, //Attribute length: 6 bytes
    //Service Class #0: A/V Remote Control Target
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 12 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0002 >> 8), //Category 2 Amplifier
    (uint8_t)(0x0002),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
    /*
        //Attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST...it is used for SDP_ATTR_SRV_NAME
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

        //Attribute SDP_ATTR_PROVIDER_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x07, //Attribute length: 7 bytes
        0x52, 0x65, 0x61, 0x6C, 0x54, 0x65, 0x6B, //RealTek

        //Attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x08, //Attribute length: 8 bytes
        0x41, 0x56, 0x52, 0x43, 0x50, 0x20, 0x54, 0x47, //AVRCP TG
    */
};
#endif

static void app_bt_audio_avrcp_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BT_AUDIO_LINK *p_link;

    switch (event_type)
    {
    case BT_EVENT_AVRCP_CONN_IND:
        p_link = app_bt_audio_find_link(param->avrcp_conn_ind.bd_addr);
        if (p_link != NULL)
        {
            bt_avrcp_connect_cfm(p_link->bd_addr, true);
        }
        break;

    case BT_EVENT_AVRCP_CONN_CMPL:
        {
            char *temp_buff = "AVRCP Connected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));

            memcpy(app_db.remote_addr, param->avrcp_conn_cmpl.bd_addr, 6);
        }
        break;

    case BT_EVENT_AVRCP_DISCONN_CMPL:
        break;

    case BT_EVENT_AVRCP_GET_CAPABILITIES_RSP:
        {
            p_link = app_bt_audio_find_link(param->avrcp_browsing_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                uint8_t  capability_count;
                uint8_t *capabilities;

                capability_count = param->avrcp_get_capabilities_rsp.capability_count;
                capabilities = param->avrcp_get_capabilities_rsp.capabilities;
                while (capability_count != 0)
                {
                    bt_avrcp_register_notification_req(p_link->bd_addr, *capabilities);
                    capability_count -= 1;
                    capabilities += 1;
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED_REG_REQ:
        p_link = app_bt_audio_find_link(param->avrcp_reg_play_status_changed.bd_addr);
        if (p_link != NULL)
        {
            if (p_link->is_streaming)
            {
                bt_avrcp_play_status_change_register_rsp(p_link->bd_addr, BT_AVRCP_PLAY_STATUS_PLAYING);
            }
            else
            {
                bt_avrcp_play_status_change_register_rsp(p_link->bd_addr, BT_AVRCP_PLAY_STATUS_PAUSED);
            }
        }
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED:
        p_link = app_bt_audio_find_link(param->avrcp_play_status_changed.bd_addr);
        if (p_link != NULL)
        {
            p_link->avrcp_play_status = param->avrcp_play_status_changed.play_status;
        }
        break;

    case BT_EVENT_AVRCP_PLAY:
        break;

    case BT_EVENT_AVRCP_PAUSE:
        break;

    case BT_EVENT_AVRCP_ABSOLUTE_VOLUME_SET:
        p_link = app_bt_audio_find_link(param->avrcp_absolute_volume_set.bd_addr);
        if (p_link != NULL)
        {
            p_link->a2dp_curr_volume = (param->avrcp_absolute_volume_set.volume *
                                        A2DP_VOLUME_MAX_LEVEL + 0x7F / 2) / 0x7F;

            if (p_link->a2dp_track_handle)
            {
                audio_track_volume_out_set(p_link->a2dp_track_handle, p_link->a2dp_curr_volume);
            }
        }
        break;

    case BT_EVENT_AVRCP_REG_VOLUME_CHANGED:
        p_link = app_bt_audio_find_link(param->avrcp_reg_volume_changed.bd_addr);
        if (p_link != NULL)
        {
            uint8_t vol = (p_link->a2dp_curr_volume * 0x7F + A2DP_VOLUME_MAX_LEVEL / 2) /
                          A2DP_VOLUME_MAX_LEVEL;

            bt_avrcp_volume_change_register_rsp(p_link->bd_addr, vol);
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            T_BT_SDP_ATTR_INFO *sdp_info = &param->sdp_attr_info.info;

            if (app_db.a2dp_role == BT_A2DP_ROLE_SNK)
            {
                if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_AV_REMOTE_CONTROL_TARGET)
                {
                    uint8_t *p_attr;

                    p_attr = bt_sdp_attr_find(sdp_info->p_attr,
                                              sdp_info->attr_len,
                                              SDP_ATTR_ADD_PROTO_DESC_LIST);
                    if (p_attr)
                    {
                        uint8_t *p_attr_param;
                        uint8_t *p_elem;
                        uint8_t  loop;
                        uint16_t uuid_1;
                        uint16_t uuid_2;
                        uint16_t l2c_psm;

                        loop = 1;
                        while ((p_elem = bt_sdp_elem_access(p_attr, sdp_info->attr_len, loop)) != NULL)
                        {
                            p_attr_param = bt_sdp_elem_access(p_elem, sdp_info->attr_len, 2);
                            if (p_attr_param)
                            {
                                p_attr_param = bt_sdp_elem_access(p_elem, sdp_info->attr_len, 1);

                                uuid_1 = p_attr_param[3] << 8 | p_attr_param[4];
                                uuid_2 = p_attr_param[11] << 8 | p_attr_param[12];
                                if (uuid_1 == UUID_L2CAP && uuid_2 == UUID_OBEX)
                                {
                                    l2c_psm = p_attr_param[6] << 8 | p_attr_param[7];
                                    bt_avrcp_cover_art_connect_req(param->sdp_attr_info.bd_addr, l2c_psm);
                                    break;
                                }
                            }
                            loop++;
                        }
                    }
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_COVER_ART_CONN_CMPL:
        {
            char *temp_buff = "AVRCP Cover Art Connected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case BT_EVENT_AVRCP_COVER_ART_DISCONN_CMPL:
        {
            char *temp_buff = "AVRCP Cover Art Disconnected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case BT_EVENT_AVRCP_ELEM_ATTR:
        p_link = app_bt_audio_find_link(param->avrcp_elem_attr.bd_addr);
        if (p_link != NULL)
        {
            if (param->avrcp_elem_attr.state == 0)
            {
                const char *attr[] = {"", "Title:", "Artist:", "Album:", "Track:",
                                      "TotalTrack:", "Genre:", "PlayingTime:"
                                     };
                uint8_t  temp_buff[50];
                uint16_t buf_len;
                uint8_t  attr_id;
                uint8_t  i, j;

                for (i = 0; i < param->avrcp_elem_attr.num_of_attr; i++)
                {
                    attr_id = param->avrcp_elem_attr.attr[i].attribute_id;
                    if (attr_id != BT_AVRCP_ELEM_ATTR_DEFAULT_COVER_ART)
                    {
                        snprintf((char *)temp_buff, 50, "%s%s\r\n", attr[attr_id],
                                 param->avrcp_elem_attr.attr[i].p_buf);

                        buf_len = param->avrcp_elem_attr.attr[i].length + sizeof(attr[attr_id]) + 2;
                        if (buf_len < 50)
                        {
                            console_write(temp_buff, buf_len);
                        }
                        else
                        {
                            console_write(temp_buff, 50);
                        }
                    }
                    else
                    {
                        uint8_t image_handle[16] = {0};

                        for (j = 0; j < param->avrcp_elem_attr.attr[i].length; j++)
                        {
                            image_handle[2 * j + 1] = param->avrcp_elem_attr.attr[i].p_buf[j];
                        }

                        bt_avrcp_cover_art_get(p_link->bd_addr, image_handle);
                    }
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_COVER_ART_DATA_IND:
        p_link = app_bt_audio_find_link(param->avrcp_cover_art_data_ind.bd_addr);
        if (p_link != NULL)
        {
            if (!param->avrcp_cover_art_data_ind.data_end)
            {
                bt_avrcp_cover_art_get(p_link->bd_addr, NULL);
            }
            else
            {
                char *temp_buff = "AVRCP Cover Art Get Success!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    default:
        break;
    }
}

void app_bt_audio_avrcp_init(void)
{
    int32_t ret = 0;

    bt_sdp_record_add((void *)avrcp_ct_sdp_record);
    bt_sdp_record_add((void *)avrcp_tg_sdp_record);

    //if absolute volume is supported
#if (BT_AUDIO_DEMO_ROLE == 1)
    if (bt_avrcp_init(BT_AVRCP_FEATURE_CATEGORY_1 | BT_AVRCP_FEATURE_CATEGORY_2,
                      BT_AVRCP_FEATURE_CATEGORY_1) == false)
    {
        ret = -1;
        goto fail_init;
    }
#else
    if (bt_avrcp_init(BT_AVRCP_FEATURE_CATEGORY_1, BT_AVRCP_FEATURE_CATEGORY_2) == false)
    {
        ret = -1;
        goto fail_init;
    }
#endif

    bt_mgr_cback_register(app_bt_audio_avrcp_cback);

    return;

fail_init:
    APP_PRINT_ERROR1("app_bt_audio_avrcp_init: failed %d", ret);
}

bool app_bt_audio_avrcp_element_attr_get(uint8_t *bd_addr, uint8_t attr_id)
{
    T_APP_BT_AUDIO_LINK *p_link;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_avrcp_get_element_attr_req(bd_addr, 1, &attr_id);
    }

    return false;
}

bool app_bt_audio_avrcp_cover_art_connect(uint8_t *bd_addr)
{
    T_BT_SDP_UUID_DATA uuid;

    uuid.uuid_16 = UUID_AV_REMOTE_CONTROL_TARGET;

    return bt_sdp_discov_start(bd_addr, BT_SDP_UUID16, uuid);
}

bool app_bt_audio_avrcp_cover_art_disconnect(uint8_t *bd_addr)
{
    T_APP_BT_AUDIO_LINK *p_link;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_avrcp_cover_art_disconnect_req(bd_addr);
    }

    return false;
}

bool app_bt_audio_avrcp_cover_art_get(uint8_t *bd_addr)
{
    T_APP_BT_AUDIO_LINK *p_link;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t attr = BT_AVRCP_ELEM_ATTR_DEFAULT_COVER_ART;

        return bt_avrcp_get_element_attr_req(bd_addr, 1, &attr);
    }

    return false;
}
