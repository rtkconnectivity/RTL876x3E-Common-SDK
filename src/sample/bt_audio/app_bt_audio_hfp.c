/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "console.h"
#include "audio.h"
#include "btm.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "bt_hfp.h"
#include "bt_hfp_ag.h"
#include "app_flags.h"
#include "app_bt_audio_link.h"
#include "app_bt_audio_hfp.h"

#define RFC_HFP_CHANN_NUM               1
#define RFC_HSP_CHANN_NUM               2
#define RFC_HSP_AG_CHANN_NUM            22
#define RFC_HFP_AG_CHANN_NUM            23

uint16_t sco_seq_num = 0;

static const uint8_t hfp_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x4B,//0x37,//0x59,

    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x06,                                   //6bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HANDSFREE >> 8), //0x111E
    (uint8_t)(UUID_HANDSFREE),
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_GENERIC_AUDIO >> 8),  //0x1203
    (uint8_t)(UUID_GENERIC_AUDIO),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x0C,                                   //12bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x03,                               //3bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_L2CAP >> 8),     //0x0100
    (uint8_t)(UUID_L2CAP),
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x05,                               //5bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_RFCOMM >> 8),   //0x0003
    (uint8_t)(UUID_RFCOMM),
    SDP_UNSIGNED_ONE_BYTE,           //0x08
    RFC_HFP_CHANN_NUM,  //0x02

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,
    /*
        //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST...it is used for SDP_ATTR_SRV_NAME
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
        SDP_STRING_HDR,                             //0x25 text string
        0x0F,                                   //15 bytes
        0x48, 0x61, 0x6e, 0x64, 0x73, 0x2d, 0x66, 0x72, 0x65, 0x65,
        0x20, 0x75, 0x6e, 0x69, 0x74, //"Hands-free unit"
    */

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x08,                                   //8 bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x06,                               //6 bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_HANDSFREE >> 8), //0x111E
    (uint8_t)(UUID_HANDSFREE),
    SDP_UNSIGNED_TWO_BYTE,           //0x09
    (uint8_t)(0x0109 >> 8),     //version number default hf1.9
    (uint8_t)(0x0109),

    //Attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0F,
    'H', 'a', 'n', 'd', 's', '-', 'F', 'r', 'e', 'e', ' ', 'u', 'n', 'i', 't',

    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x013F >> 8),
    (uint8_t)(0x013F)
};

static const uint8_t hfp_ag_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x34,//52 bytes belowed.

    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x06,                                   //6bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HANDSFREE_AUDIO_GATEWAY >> 8), //0x111F
    (uint8_t)(UUID_HANDSFREE_AUDIO_GATEWAY),
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_GENERIC_AUDIO >> 8),  //0x1203
    (uint8_t)(UUID_GENERIC_AUDIO),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x0C,                                   //12bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x03,                               //3bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_L2CAP >> 8),     //0x0100
    (uint8_t)(UUID_L2CAP),
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x05,                               //5bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_RFCOMM >> 8),   //0x0003
    (uint8_t)(UUID_RFCOMM),
    SDP_UNSIGNED_ONE_BYTE,           //0x08
    RFC_HFP_AG_CHANN_NUM,  //0x11

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x08,                                   //8 bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x06,                               //6 bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_HANDSFREE >> 8), //0x111E
    (uint8_t)(UUID_HANDSFREE),
    SDP_UNSIGNED_TWO_BYTE,           //0x09
    (uint8_t)(0x0109 >> 8),     //version number default hf1.9
    (uint8_t)(0x0109),
    /*
        //attribute SDP_ATTR_BROWSE_GROUP_LIST
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
        (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x03,
        SDP_UUID16_HDR,
        (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
        (uint8_t)UUID_PUBLIC_BROWSE_GROUP,
    */
    /*
        //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST...it is used for SDP_ATTR_SRV_NAME
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
        SDP_STRING_HDR,                             //0x25 text string
        0x0F,                                   //15 bytes
        0x48, 0x61, 0x6e, 0x64, 0x73, 0x2d, 0x66, 0x72, 0x65, 0x65,
        0x20, 0x75, 0x6e, 0x69, 0x74, //"Hands-free unit"
    */
    //attribute SDP_ATTR_EXT_NETWORK
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_EXT_NETWORK) >> 8),
    (uint8_t)(SDP_ATTR_EXT_NETWORK),
    SDP_UNSIGNED_ONE_BYTE,
    (uint8_t)(0x01),

    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x012F >> 8),
    (uint8_t)(0x012F)
};

static void app_bt_audio_hfp_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BT_AUDIO_LINK *p_link;

    switch (event_type)
    {
    case BT_EVENT_SDP_ATTR_INFO:
        {
            T_BT_SDP_ATTR_INFO *sdp_info = &param->sdp_attr_info.info;
            if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
            {
                if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_HANDSFREE)
                {
                    bt_hfp_ag_connect_req(param->sdp_attr_info.bd_addr, sdp_info->server_channel, true);
                }
            }
            else if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_HF)
            {
                if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_HANDSFREE_AUDIO_GATEWAY)
                {
                    bt_hfp_connect_req(param->sdp_attr_info.bd_addr, sdp_info->server_channel, true);
                }
            }
        }
        break;

    case BT_EVENT_HFP_CONN_IND:
        p_link = app_bt_audio_find_link(param->hfp_conn_ind.bd_addr);
        if (p_link != NULL)
        {
            bt_hfp_connect_cfm(p_link->bd_addr, true);
        }
        break;

    case BT_EVENT_HFP_CONN_CMPL:
        {
            char *temp_buff = "HFP Connected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
            switch (param->hfp_call_status.curr_status)
            {
            case BT_HFP_CALL_IDLE:
                {
                    char *temp_buff = "call idle!\r\n";
                    console_write((uint8_t *)temp_buff, strlen(temp_buff));
                }
                break;

            case BT_HFP_CALL_INCOMING:
                {
                    char *temp_buff = "call incoming!\r\n";
                    console_write((uint8_t *)temp_buff, strlen(temp_buff));
                }
                break;

            case BT_HFP_CALL_ACTIVE:
                {
                    char *temp_buff = "call active!\r\n";
                    console_write((uint8_t *)temp_buff, strlen(temp_buff));
                }
                break;

            default:
                break;
            }
        }
        break;

    case BT_EVENT_HFP_SUPPORTED_FEATURES_IND:
        {
            if (param->hfp_supported_features_ind.ag_bitmap & BT_HFP_HF_REMOTE_CAPABILITY_INBAND_RINGING)
            {
                APP_PRINT_INFO0("AG support in-band ringtone");
            }
        }
        break;

    case BT_EVENT_HFP_SERVICE_STATUS:
        {
            if (!param->hfp_service_status.status)
            {
                APP_PRINT_INFO0("No Home/Roam network available");
            }
        }
        break;

    case BT_EVENT_HFP_CURRENT_CALL_LIST_IND:
        {
            /* - status=
                0 = Active
                1 = Held
                2 = Dialing (outgoing calls only)
                3 = Alerting (outgoing calls only)
                4 = Incoming (incoming calls only)
                5 = Waiting (incoming calls only)
                6 = Call held by Response and Hold
               - mode= 0 (Voice), 1 (Data), 2 (FAX)
               - mpty=
                0 - this call is NOT a member of a multi-party (conference) call
                1 - this call IS a member of a multi-party (conference) call*/
        }
        break;

    case BT_EVENT_HFP_ROAM_IND:
        {
            /* roam: Roaming status indicator, where: <value>=0 means roaming is not active
               <value>=1 means a roaming is active */
        }
        break;

    case BT_EVENT_HFP_SIGNAL_IND:
        {
            /* signal: Signal Strength indicator, where: <value>= ranges from 0 to 5 */
        }
        break;

    case BT_EVENT_HFP_BATTERY_IND:
        {
            /* battchg: Battery Charge indicator of AG, where: <value>=ranges from 0 to 5 */
        }
        break;

    case BT_EVENT_HFP_VENDOR_CMD:
        {
            /* If vendor at cmd +RTK: is defined */
            if ((param->hfp_vendor_cmd.len >= strlen("+RTK:")) &&
                (!strncmp("+RTK:", (char *)param->hfp_vendor_cmd.cmd, 5)))
            {
                console_write(param->hfp_vendor_cmd.cmd, param->hfp_vendor_cmd.len);
            }
        }
        break;

    case BT_EVENT_HFP_DISCONN_CMPL:
        {
            char *temp_buff = "HFP Disconnected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case BT_EVENT_SCO_CONN_IND:
        {
            p_link = app_bt_audio_find_link(param->sco_conn_ind.bd_addr);

            if (p_link != NULL)
            {
                if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
                {
                    bt_hfp_ag_audio_connect_cfm(param->sco_conn_ind.bd_addr, true);
                }
                else if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_HF)
                {
                    bt_hfp_audio_connect_cfm(param->sco_conn_ind.bd_addr, true);
                }
            }
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            char *temp_buff = "SCO Connected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
            T_AUDIO_FORMAT_INFO format_info;

            p_link = app_bt_audio_find_link(param->sco_conn_cmpl.bd_addr);

            if (p_link != NULL)
            {
                if (param->sco_conn_cmpl.air_mode == 3)
                {
                    format_info.type = AUDIO_FORMAT_TYPE_MSBC;
                    format_info.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
                    format_info.attr.msbc.sample_rate = 16000;
                    format_info.attr.msbc.bitpool = 26;
                    format_info.attr.msbc.allocation_method = 0;
                    format_info.attr.msbc.subband_num = 8;
                    format_info.attr.msbc.block_length = 15;
                }
                else if (param->sco_conn_cmpl.air_mode == 2)
                {
                    format_info.type = AUDIO_FORMAT_TYPE_CVSD;
                    format_info.attr.cvsd.chann_num = AUDIO_SBC_CHANNEL_MODE_MONO;
                    format_info.attr.cvsd.sample_rate = 8000;
                    if (param->sco_conn_cmpl.rx_pkt_len == 30)
                    {
                        format_info.attr.cvsd.frame_duration = AUDIO_CVSD_FRAME_DURATION_3_75_MS;
                    }
                    else
                    {
                        format_info.attr.cvsd.frame_duration = AUDIO_CVSD_FRAME_DURATION_7_5_MS;
                    }
                }
                else
                {
                    break;
                }

                if (p_link->sco_track_handle != NULL)
                {
                    audio_track_release(p_link->sco_track_handle);
                    p_link->sco_track_handle = NULL;
                }

                p_link->sco_track_handle = audio_track_create(AUDIO_STREAM_TYPE_VOICE,
                                                              AUDIO_STREAM_MODE_NORMAL,
                                                              AUDIO_STREAM_USAGE_SNOOP,
                                                              format_info,
                                                              10,
                                                              8,
                                                              AUDIO_DEVICE_OUT_DEFAULT | AUDIO_DEVICE_IN_DEFAULT,
                                                              NULL,
                                                              NULL);
                if (p_link->sco_track_handle != NULL)
                {
                    APP_PRINT_INFO0("voice audio track start");
                    audio_track_latency_set(p_link->sco_track_handle, 15, false);
                    audio_track_start(p_link->sco_track_handle);
                }
            }
        }
        break;

    case BT_EVENT_SCO_DATA_IND:
        {
            uint16_t written_len;
            T_AUDIO_STREAM_STATUS  status;

            p_link = app_bt_audio_find_link(param->sco_data_ind.bd_addr);
            if (p_link != NULL)
            {
                sco_seq_num++;

                if (param->sco_data_ind.status == BT_SCO_PKT_STATUS_OK)
                {
                    status = AUDIO_STREAM_STATUS_CORRECT;
                }
                else
                {
                    status = AUDIO_STREAM_STATUS_LOST;
                }

                audio_track_write(p_link->sco_track_handle, param->sco_data_ind.bt_clk,
                                  sco_seq_num,
                                  status,
                                  1,
                                  param->sco_data_ind.p_data,
                                  param->sco_data_ind.length,
                                  &written_len);
            }
        }
        break;

    case BT_EVENT_SCO_DISCONNECTED:
        {
            char *temp_buff = "SCO Disconnected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));

            p_link = app_bt_audio_find_link(param->sco_disconnected.bd_addr);
            if (p_link != NULL)
            {
                audio_track_release(p_link->sco_track_handle);
                sco_seq_num = 0;
            }
        }
        break;

    case BT_EVENT_HFP_AG_CONN_IND:
        p_link = app_bt_audio_find_link(param->hfp_ag_conn_ind.bd_addr);
        if (p_link != NULL)
        {
            bt_hfp_ag_connect_cfm(p_link->bd_addr, true);
        }
        break;

    case BT_EVENT_HFP_AG_CONN_CMPL:
        {
            char *temp_buff = "HFP Connected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));

#if (BT_AUDIO_DEMO_ROLE == 1)
            //bt_hfp_ag_call_incoming(param->hfp_ag_conn_cmpl.bd_addr, "10086", 6, 129);
            //app_bt_audio_hfp_sco_connect(param->hfp_ag_conn_cmpl.bd_addr);
#endif
        }
        break;

    case BT_EVENT_HFP_AG_DISCONN_CMPL:
        {
            char *temp_buff = "HFP Disconnected!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));
        }
        break;

    case BT_EVENT_HFP_AG_INDICATORS_STATUS_REQ:
        {
            p_link = app_bt_audio_find_link(param->hfp_ag_indicators_status_req.bd_addr);
            if (p_link != NULL)
            {
                //Td App provide current network status.
                T_BT_HFP_AG_SERVICE_INDICATOR service_indicator = BT_HFP_AG_SERVICE_STATUS_AVAILABLE;
                T_BT_HFP_AG_CALL_INDICATOR call_indicator = BT_HFP_AG_NO_CALL_IN_PROGRESS;
                T_BT_HFP_AG_CALL_SETUP_INDICATOR call_setup_indicator = BT_HFP_AG_CALL_SETUP_STATUS_IDLE;
                T_BT_HFP_AG_CALL_HELD_INDICATOR call_held_indicator = BT_HFP_AG_CALL_HELD_STATUS_IDLE;

                //Td App provide current signal status.
                uint8_t signal_indicator = 5;
                //Td App provide current roaming status.
                T_BT_HFP_AG_ROAMING_INDICATOR roaming_indicator = BT_HFP_AG_ROAMING_STATUS_ACTIVE;
                //Td App provide current battery status.
                uint8_t batt_chg_indicator = 5;
                bt_hfp_ag_indicators_send(p_link->bd_addr,
                                          service_indicator,
                                          call_indicator,
                                          call_setup_indicator,
                                          call_held_indicator,
                                          signal_indicator,
                                          roaming_indicator,
                                          batt_chg_indicator);
                bt_hfp_ag_ok_send(param->hfp_ag_indicators_status_req.bd_addr);
            }
        }
        break;

    case BT_EVENT_HFP_AG_CURR_CALLS_LIST_QUERY:
        {
            p_link = app_bt_audio_find_link(param->hfp_ag_curr_calls_list_query.bd_addr);
            if (p_link != NULL)
            {
                bt_hfp_ag_ok_send(param->hfp_ag_curr_calls_list_query.bd_addr);
            }
        }
        break;

    case  BT_EVENT_HFP_AG_DIAL_LAST_NUMBER:
        {
            bt_hfp_ag_ok_send(param->hfp_ag_dial_last_number.bd_addr);
            bt_hfp_ag_call_dial(param->hfp_ag_dial_last_number.bd_addr);
            bt_hfp_ag_audio_connect_req(param->hfp_ag_dial_last_number.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_CALL_ANSWER_REQ:
        {
            bt_hfp_ag_call_answer(param->hfp_ag_call_answer_req.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_CALL_TERMINATE_REQ:
        {
            bt_hfp_ag_call_terminate(param->hfp_ag_call_terminate_req.bd_addr);
        }
        break;

    case BT_EVENT_HFP_AG_CALL_STATUS_CHANGED:
        {
            if (param->hfp_ag_call_status_changed.curr_status == BT_HFP_AG_CALL_INCOMING)
            {
                char *temp_buff = "call incoming!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
            else if (param->hfp_ag_call_status_changed.curr_status == BT_HFP_AG_CALL_ACTIVE)
            {
                char *temp_buff = "call active!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
            else if (param->hfp_ag_call_status_changed.curr_status == BT_HFP_AG_CALL_IDLE)
            {
                char *temp_buff = "call idle!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
                bt_hfp_ag_audio_disconnect_req(param->hfp_ag_call_status_changed.bd_addr);
            }
        }
        break;

    case BT_EVENT_HFP_AG_VENDOR_CMD:
        {
            /* If vendor at cmd AT+RTK= is defined */
            if ((param->hfp_ag_vendor_cmd.len >= strlen("AT+RTK=")) &&
                (!strncmp("AT+RTK=", (char *)param->hfp_ag_vendor_cmd.cmd, 7)))
            {
                console_write(param->hfp_ag_vendor_cmd.cmd, param->hfp_ag_vendor_cmd.len);
                /* After process vendor at cmd, ok rsp must be sent */
                bt_hfp_ag_ok_send(param->hfp_ag_vendor_cmd.bd_addr);
            }
            else
            {
                /* If vendor at cmd is undefined, error rsp must be sent */
                bt_hfp_ag_error_send(param->hfp_ag_vendor_cmd.bd_addr, BT_HFP_AG_ERR_INVALID_CHAR_IN_TSTR);
            }
        }
        break;

    default:
        break;
    }
}

static void app_audio_policy_hfp_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_DATA_IND:
        {
            T_AUDIO_STREAM_STATUS status;
            T_APP_BT_AUDIO_LINK *p_link = NULL;
            uint8_t  i;
            uint32_t timestamp;
            uint16_t seq_num;
            uint8_t  frame_num;
            uint16_t read_len;
            uint8_t *buf;


            for (i = 0; i < APP_BT_AUDIO_MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].sco_track_handle != NULL)
                {
                    p_link = &app_db.br_link[i];
                    break;
                }
            }

            if (p_link != NULL)
            {
                buf = os_mem_alloc2(param->track_data_ind.len);

                if (buf != NULL)
                {
                    if (audio_track_read(param->track_data_ind.handle,
                                         &timestamp,
                                         &seq_num,
                                         &status,
                                         &frame_num,
                                         buf,
                                         param->track_data_ind.len,
                                         &read_len) == true)
                    {
                        bt_sco_data_send(p_link->bd_addr, seq_num, buf, read_len);
                    }

                    os_mem_free(buf);
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_bt_audio_hfp_init(uint8_t role)
{
    int32_t ret = 0;

    app_db.hfp_role = role;

    if (role == BT_AUDIO_HFP_ROLE_AG)
    {
        if (bt_sdp_record_add((void *)hfp_ag_sdp_record) == false)
        {
            ret = 1;
            goto fail_sdp_add;
        }

        if (bt_hfp_ag_init(RFC_HFP_AG_CHANN_NUM, RFC_HSP_AG_CHANN_NUM,
                           BT_HFP_AG_LOCAL_CAPABILITY_3WAY |
                           BT_HFP_AG_LOCAL_CAPABILITY_VOICE_RECOGNITION |
                           BT_HFP_AG_LOCAL_CAPABILITY_INBAND_RINGING |
                           BT_HFP_AG_LOCAL_CAPABILITY_CODEC_NEGOTIATION |
                           BT_HFP_AG_LOCAL_CAPABILITY_HF_INDICATORS |
                           BT_HFP_AG_LOCAL_CAPABILITY_ESCO_S4_T2_SUPPORTED,
                           BT_HFP_AG_CODEC_TYPE_CVSD | BT_HFP_AG_CODEC_TYPE_MSBC) == false)
        {
            ret = 2;
            goto fail_init;
        }
    }
    else if (role == BT_AUDIO_HFP_ROLE_HF)
    {
        if (bt_sdp_record_add((void *)hfp_sdp_record) == false)
        {
            ret = 1;
            goto fail_sdp_add;
        }

        if (bt_hfp_init(RFC_HFP_CHANN_NUM, RFC_HSP_CHANN_NUM,
                        BT_HFP_HF_LOCAL_THREE_WAY_CALLING |
                        BT_HFP_HF_LOCAL_CLI_PRESENTATION_CAPABILITY |
                        BT_HFP_HF_LOCAL_VOICE_RECOGNITION_ACTIVATION |
                        BT_HFP_HF_LOCAL_CODEC_NEGOTIATION |
                        BT_HFP_HF_LOCAL_ESCO_S4_SETTINGS |
                        BT_HFP_HF_LOCAL_REMOTE_VOLUME_CONTROL,
                        BT_HFP_HF_CODEC_TYPE_CVSD | BT_HFP_HF_CODEC_TYPE_MSBC) == false)
        {
            ret = 2;
            goto fail_init;
        }
    }

    bt_mgr_cback_register(app_bt_audio_hfp_cback);
    audio_mgr_cback_register(app_audio_policy_hfp_cback);

fail_init:
fail_sdp_add:
    APP_PRINT_ERROR1("app_bt_audio_hfp_init: failed %d", -ret);
}

bool app_bt_audio_hfp_vendor_at_cmd(uint8_t *bd_addr)
{
    if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
    {
        /* The vendor at cmd which AG sent must end with \r\n*/
        char *vendor_cmd = "\r\n+RTK:TEST\r\n";
        return bt_hfp_ag_send_vnd_at_cmd_req(bd_addr, vendor_cmd);
    }
    else
    {
        /* The vendor at cmd which HF sent must end with \r*/
        char *vendor_cmd = "AT+RTK=TEST\r";
        return bt_hfp_send_vnd_at_cmd_req(bd_addr, vendor_cmd);
    }
}

bool app_bt_audio_hfp_call_incoming(uint8_t *bd_addr)
{
    if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
    {
        return bt_hfp_ag_call_incoming(bd_addr, "10086", 6, 129);
    }
    else
    {
        return bt_hfp_dial_last_number_req(bd_addr);
    }
}

bool app_bt_audio_hfp_call_answer(uint8_t *bd_addr)
{
    if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
    {
        return bt_hfp_ag_call_answer(bd_addr);
    }
    else
    {
        return bt_hfp_call_answer_req(bd_addr);
    }
}

bool app_bt_audio_hfp_call_terminate(uint8_t *bd_addr)
{
    if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
    {
        return bt_hfp_ag_call_terminate(bd_addr);
    }
    else
    {
        return bt_hfp_call_terminate_req(bd_addr);
    }
}

bool app_bt_audio_hfp_sco_connect(uint8_t *bd_addr)
{
    if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
    {
        return bt_hfp_ag_audio_connect_req(bd_addr);
    }
    else
    {
        return bt_hfp_audio_connect_req(bd_addr);
    }
}

bool app_bt_audio_hfp_sco_disconnect(uint8_t *bd_addr)
{
    T_APP_BT_AUDIO_LINK *p_link;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
        {
            return bt_hfp_ag_audio_disconnect_req(bd_addr);
        }
        else
        {
            return bt_hfp_audio_disconnect_req(bd_addr);
        }
    }

    return false;
}

bool app_bt_audio_hfp_connect(uint8_t *bd_addr)
{
    T_BT_SDP_UUID_DATA uuid;

    if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
    {
        uuid.uuid_16 = UUID_HANDSFREE;
    }
    else
    {
        uuid.uuid_16 = UUID_HANDSFREE_AUDIO_GATEWAY;
    }

    return bt_sdp_discov_start(bd_addr, BT_SDP_UUID16, uuid);
}

bool app_bt_audio_hfp_disconnect(uint8_t *bd_addr)
{
    T_APP_BT_AUDIO_LINK *p_link;

    p_link = app_bt_audio_find_link(bd_addr);
    if (p_link != NULL)
    {
        if (app_db.hfp_role == BT_AUDIO_HFP_ROLE_AG)
        {
            return bt_hfp_ag_disconnect_req(bd_addr);
        }
        else
        {
            return bt_hfp_disconnect_req(bd_addr);
        }
    }

    return false;
}
