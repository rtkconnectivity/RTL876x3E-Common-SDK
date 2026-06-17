/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include "stdlib_corecrt.h"
#include "bt_types.h"
#include "trace.h"
#include "stdlib.h"
#include "bqb.h"
#include "app_mmi.h"
#include "app_main.h"
#include "gap_br.h"
#include "cli_power.h"
#include "app_msg.h"
#include "app_console_msg.h"
#include "app_cmd.h"
#include "app_bt_policy_api.h"
#include "app_device.h"
#include "app_link_util_cs.h"
#include "bt_a2dp.h"
#include "bt_hfp.h"
#include "bt_avrcp.h"
#include "bt_spp.h"
#include "btm.h"
#include "bt_pbap.h"
#include "bt_hfp_ag.h"
#include "os_sched.h"
#include "bt_map.h"

#define SCO_PKT_TYPES_HV3_EV3_2EV3          (GAP_PKT_TYPE_HV3 | GAP_PKT_TYPE_EV3 | GAP_PKT_TYPE_NO_3EV3 | GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define AVDTP_VERSON_V1_3                   0x0103

void app_console_handle_msg(T_IO_MSG console_msg)
{
    uint16_t  subtype;
    uint16_t  id;
    uint16_t  cmd_len;
    uint8_t   rx_seqn;
    uint8_t   action;
    uint8_t  *p;

    p       = console_msg.u.buf;
    subtype = console_msg.subtype;
    switch (subtype)
    {
    case IO_MSG_CONSOLE_STRING_RX:
        LE_STREAM_TO_UINT16(id, p);

        if (id == POWER_CMD)
        {
            LE_STREAM_TO_UINT8(action, p);

            if (action == POWER_ACTION_POWER_ON)
            {
                app_device_handle_power_on_cmd();
            }
            else if (action == POWER_ACTION_POWER_OFF)
            {
                app_db.power_off_cause = POWER_OFF_CAUSE_CMD_SET;
                app_mmi_handle_action(MMI_DEV_POWER_OFF);
            }
        }
        else if (id == BQB_CMD_SDP)
        {
            T_GAP_UUID_DATA data;
            uint16_t uuid;

            LE_STREAM_TO_UINT8(action, p);
            LE_STREAM_TO_UINT16(uuid, p);
            data.uuid_16 = uuid;

            if (action == BQB_ACTION_SDP_SEARCH)
            {
                gap_br_start_sdp_discov(p, GAP_UUID16, data);
            }
        }
        else if (id == BQB_CMD_AVDTP)
        {
            LE_STREAM_TO_UINT8(action, p);

            if (action == BQB_ACTION_AVDTP_OPEN)
            {
                bt_a2dp_stream_open_req(p, BT_A2DP_ROLE_SNK);
            }
            else if (action == BQB_ACTION_AVDTP_START)
            {
                bt_a2dp_stream_start_req(p);
            }
            else if (action == BQB_ACTION_AVDTP_CLOSE)
            {
                bt_a2dp_stream_close_req(p);
            }
            else if (action == BQB_ACTION_AVDTP_ABORT)
            {
                bt_a2dp_stream_abort_req(p);
            }
            else if (action == BQB_ACTION_AVDTP_CONNECT_SIGNAL_SNK)
            {
                bt_a2dp_connect_req(p, AVDTP_VERSON_V1_3, BT_A2DP_ROLE_SNK, 0);
            }
            else if (action == BQB_ACTION_AVDTP_CONNECT_SIGNAL_SRC)
            {
                bt_a2dp_connect_req(p, AVDTP_VERSON_V1_3, BT_A2DP_ROLE_SRC, 0);
            }
            else if (action == BQB_ACTION_AVDTP_CONNECT_STREAM)
            {
                //api removed, use bt_a2dp_stream_open_req() to connect_stream_chann
            }
            else if (action == BQB_ACTION_AVDTP_DISCONNECT)
            {
                bt_a2dp_disconnect_req(p);
            }
        }
        else if (id == BQB_CMD_AVRCP)
        {
            LE_STREAM_TO_UINT8(action, p);

            if (action == BQB_ACTION_AVRCP_CONNECT)
            {
                bt_avrcp_connect_req(p);
            }
            else if (action == BQB_ACTION_AVRCP_CONNECT_CONTROLLER)
            {

            }
            else if (action == BQB_ACTION_AVRCP_CONNECT_TARGET)
            {

            }
            else if (action == BQB_ACTION_AVRCP_DISCONNECT)
            {
                bt_avrcp_disconnect_req(p);
            }
            else if (action == BQB_ACTION_AVRCP_GET_PLAY_STATUS)
            {
                bt_avrcp_get_play_status_req(p);
            }
            else if (action == BQB_ACTION_AVRCP_GET_ELEMENT_ATTR)
            {
                uint8_t attr = BT_AVRCP_ELEM_ATTR_TITLE;
                bt_avrcp_get_element_attr_req(p, 1, &attr);
            }
            else if (action == BQB_ACTION_AVRCP_PLAY)
            {
                bt_avrcp_play(p);
            }
            else if (action == BQB_ACTION_AVRCP_PAUSE)
            {
                bt_avrcp_pause(p);
            }
            else if (action == BQB_ACTION_AVRCP_STOP)
            {
                bt_avrcp_stop(p);
            }
            else if (action == BQB_ACTION_AVRCP_REWIND)
            {
                bt_avrcp_rewind_start(p);
                bt_avrcp_rewind_stop(p);
            }
            else if (action == BQB_ACTION_AVRCP_FASTFORWARD)
            {
                bt_avrcp_fast_forward_start(p);
                bt_avrcp_fast_forward_stop(p);
            }
            else if (action == BQB_ACTION_AVRCP_FORWARD)
            {
                bt_avrcp_forward(p);
            }
            else if (action == BQB_ACTION_AVRCP_BACKWARD)
            {
                bt_avrcp_backward(p);
            }
            else if (action == BQB_ACTION_AVRCP_NOTIFY_VOLUME)
            {
                uint8_t   vol;
                LE_STREAM_TO_UINT8(vol, p);
                bt_avrcp_volume_change_req(p, vol);
            }
        }
        else if (id == BQB_CMD_RFCOMM)
        {
            LE_STREAM_TO_UINT8(action, p);

            switch (action)
            {
            case BQB_ACTION_RFCOMM_CONNECT_SPP:
                {
                    uint8_t   server_chann;
                    LE_STREAM_TO_UINT8(server_chann, p);
                    bt_spp_connect_req(p, server_chann, 1012, 7, 6);
                }
                break;

            case BQB_ACTION_RFCOMM_CONNECT_HFP:
                {
                    uint8_t   server_chann;
                    LE_STREAM_TO_UINT8(server_chann, p);
                    bt_hfp_connect_req(p, server_chann, true);
                }
                break;

            case BQB_ACTION_RFCOMM_CONNECT_HSP:
                {
                    uint8_t   server_chann;
                    LE_STREAM_TO_UINT8(server_chann, p);
                    bt_hfp_connect_req(p, server_chann, false);
                }
                break;

            case BQB_ACTION_RFCOMM_CONNECT_PBAP:
                {
                    uint8_t   server_chann;
                    LE_STREAM_TO_UINT8(server_chann, p);
                    bt_pbap_connect_over_rfc_req(p, server_chann, true);
                }
                break;

            case BQB_ACTION_RFCOMM_DISCONNECT_SPP:
                bt_spp_disconnect_all_req(p);
                break;

            case BQB_ACTION_RFCOMM_DISCONNECT_HFP:
                bt_hfp_disconnect_req(p);
                break;

            case BQB_ACTION_RFCOMM_DISCONNECT_HSP:
                bt_hfp_disconnect_req(p);
                break;

            case BQB_ACTION_RFCOMM_DISCONNECT_PBAP:
                bt_pbap_disconnect_req(p);
                break;

            case BQB_ACTION_RFCOMM_DISCONNECT_ALL:
                break;

            case BQB_ACTION_RFCOMM_CONNECT_HFP_AG:
                {
                    uint8_t   server_chann;
                    LE_STREAM_TO_UINT8(server_chann, p);
                    bt_hfp_ag_connect_req(p, server_chann, true);
                }
                break;

            case BQB_ACTION_RFCOMM_CONNECT_HSP_AG:
                {
                    uint8_t   server_chann;
                    LE_STREAM_TO_UINT8(server_chann, p);
                    bt_hfp_ag_connect_req(p, server_chann, false);
                }
                break;

            case BQB_ACTION_RFCOMM_DISCONNECT_HFP_AG:
                bt_hfp_ag_disconnect_req(p);
                break;

            case BQB_ACTION_RFCOMM_DISCONNECT_HSP_AG:
                bt_hfp_ag_disconnect_req(p);
                break;

            default:
                break;
            }
        }
        else if (id == BQB_CMD_HFHS)
        {
            LE_STREAM_TO_UINT8(action, p);

            switch (action)
            {
            case BQB_ACTION_HFHS_CONNECT_SCO:
                bt_hfp_audio_connect_req(p);
                break;
            case BQB_ACTION_HFHS_DISCONNECT_SCO:
                bt_hfp_audio_disconnect_req(p);
                break;
            case BQB_ACTION_HFHS_CALL_ANSWER:
                bt_hfp_call_answer_req(p);
                break;
            case BQB_ACTION_HFHS_CALL_REDIAL:
                bt_hfp_dial_last_number_req(p);
                break;
            case BQB_ACTION_HFHS_CALL_ACTIVE:
                bt_hfp_release_active_call_accept_held_or_waiting_call_req(p);
                break;
            case BQB_ACTION_HFHS_CALL_END:
                bt_hfp_call_terminate_req(p);
                break;
            case BQB_ACTION_HFHS_CALL_REJECT:
                bt_hfp_call_terminate_req(p);
                break;
            case BQB_ACTION_HFHS_VOICE_RECOGNITION_ACTIVATE:
                bt_hfp_voice_recognition_enable_req(p);
                break;
            case BQB_ACTION_HFHS_VOICE_RECOGNITION_DEACTIVATE:
                bt_hfp_voice_recognition_disable_req(p);
                break;
            case BQB_ACTION_HFHS_SPK_GAIN_LEVEL_REPORT:
                {
                    uint8_t   level;
                    LE_STREAM_TO_UINT8(level, p);
                    bt_hfp_speaker_gain_level_report(p, level);
                }
                break;

            case BQB_ACTION_HFHS_MIC_GAIN_LEVEL_REPORT:
                {
                    uint8_t   level;
                    LE_STREAM_TO_UINT8(level, p);
                    bt_hfp_microphone_gain_level_report(p, level);
                }
                break;

            case BQB_ACTION_HFHS_SCO_CONN_REQ:
                {
                    uint16_t   voice_setting;
                    uint8_t    retrans_effort;
                    LE_STREAM_TO_UINT16(voice_setting, p);
                    LE_STREAM_TO_UINT8(retrans_effort, p);
                    gap_br_send_sco_conn_req(p, 8000, 8000, 12, voice_setting, retrans_effort,
                                             SCO_PKT_TYPES_HV3_EV3_2EV3);
                }
                break;

            default:
                break;
            }
        }
        else if (id == BQB_CMD_HFHS_AG)
        {
            LE_STREAM_TO_UINT8(action, p);

            switch (action)
            {
            case BQB_ACTION_HFHS_AG_CONNECT_SCO:
                {
                    bt_hfp_ag_audio_connect_req(p);
                }
                break;
            case BQB_ACTION_HFHS_AG_DISCONNECT_SCO:
                bt_hfp_ag_audio_disconnect_req(p);
                break;
            case BQB_ACTION_HFHS_AG_CALL_INCOMING:
                {
                    char      call_num[20];
                    uint8_t   call_num_len;
                    uint8_t   call_type;
                    call_num_len = strlen((char *)p) + 1;
                    memcpy_s(call_num, sizeof(call_num), p, call_num_len);
                    p += call_num_len;
                    LE_STREAM_TO_UINT8(call_type, p);

                    bt_hfp_ag_call_incoming(p, call_num, call_num_len, call_type);
                }
                break;
            case BQB_ACTION_HFHS_AG_CALL_ANSWER:
                bt_hfp_ag_call_answer(p);
                break;
            case BQB_ACTION_HFHS_AG_CALL_TERMINATE:
                bt_hfp_ag_call_terminate(p);
                break;
            case  BQB_ACTION_HFHS_AG_CALL_DIAL:
                bt_hfp_ag_call_dial(p);
                break;
            case  BQB_ACTION_HFHS_AG_CALL_ALERTING:
                bt_hfp_ag_call_alert(p);
                break;
            case  BQB_ACTION_HFHS_AG_CALL_WAITING:
                {
                    char      call_num[20];
                    uint8_t   call_num_len;
                    uint8_t   call_type;

                    call_num_len = strlen((char *)p) + 1;
                    memcpy_s(call_num, sizeof(call_num), p, call_num_len);
                    p += call_num_len;
                    LE_STREAM_TO_UINT8(call_type, p);

                    bt_hfp_ag_call_waiting_send(p, call_num, call_num_len, call_type);
                }
                break;

            case BQB_ACTION_HFHS_AG_MIC_GAIN_LEVEL_REPORT:
                {
                    uint8_t   level;
                    LE_STREAM_TO_UINT8(level, p);
                    bt_hfp_ag_microphone_gain_set(p, level);
                }
                break;

            case BQB_ACTION_HFHS_AG_SPEAKER_GAIN_LEVEL_REPORT:
                {
                    uint8_t   level;
                    LE_STREAM_TO_UINT8(level, p);
                    bt_hfp_ag_speaker_gain_set(p, level);
                }
                break;

            case BQB_ACTION_HFHS_AG_RING_INTERVAL_SET:
                {
                    uint16_t   ring_interval;
                    LE_STREAM_TO_UINT16(ring_interval, p);
                    bt_hfp_ag_ring_interval_set(p, ring_interval);
                }
                break;

            case BQB_ACTION_HFHS_AG_INBAND_RINGING_SET:
                {
                    uint8_t   ring_enable;
                    LE_STREAM_TO_UINT8(ring_enable, p);
                    bt_hfp_ag_inband_ringing_set(p, (ring_enable == 1));
                }
                break;

            case BQB_ACTION_HFHS_AG_CURRENT_CALLS_LIST_SEND:
                {
                    uint8_t     clcc_call_idx;
                    uint8_t     clcc_call_dir;
                    uint8_t     clcc_call_status;
                    uint8_t     clcc_call_mode;
                    uint8_t     clcc_call_mpty;
                    char        call_num[20];
                    uint8_t     call_type;
                    uint8_t   call_num_len;

                    LE_STREAM_TO_UINT8(clcc_call_idx, p);
                    LE_STREAM_TO_UINT8(clcc_call_dir, p);
                    LE_STREAM_TO_UINT8(clcc_call_status, p);
                    LE_STREAM_TO_UINT8(clcc_call_mode, p);
                    LE_STREAM_TO_UINT8(clcc_call_mpty, p);

                    call_num_len = strlen((char *)p) + 1;
                    memcpy_s(call_num, sizeof(call_num), p, call_num_len);
                    p += call_num_len;
                    LE_STREAM_TO_UINT8(call_type, p);

                    bt_hfp_ag_current_calls_list_send(p,
                                                      clcc_call_idx,
                                                      clcc_call_dir,
                                                      (T_BT_HFP_AG_CURRENT_CALL_STATUS) clcc_call_status,
                                                      (T_BT_HFP_AG_CURRENT_CALL_MODE) clcc_call_mode,
                                                      clcc_call_mpty,
                                                      call_num, call_type);
                }
                break;

            case BQB_ACTION_HFHS_AG_SEVICE_STATUS_SEND:
                {
                    uint8_t   status;
                    LE_STREAM_TO_UINT8(status, p);
                    bt_hfp_ag_service_indicator_send(p, (T_BT_HFP_AG_SERVICE_INDICATOR) status);
                }
                break;

            case  BQB_ACTION_HFHS_AG_CALL_STATUS_SEND:
                {
                    uint8_t   status;
                    LE_STREAM_TO_UINT8(status, p);
                    bt_hfp_ag_call_indicator_send(p, (T_BT_HFP_AG_CALL_INDICATOR) status);
                }
                break;

            case BQB_ACTION_HFHS_AG_CALL_SETUP_STATUS_SEND:
                {
                    uint8_t   status;
                    LE_STREAM_TO_UINT8(status, p);
                    bt_hfp_ag_call_setup_indicator_send(p, (T_BT_HFP_AG_CALL_SETUP_INDICATOR) status);
                }
                break;

            case BQB_ACTION_HFHS_AG_CALL_HELD_STATUS_SEND:
                {
                    uint8_t   status;
                    LE_STREAM_TO_UINT8(status, p);
                    bt_hfp_ag_call_held_indicator_send(p, (T_BT_HFP_AG_CALL_HELD_INDICATOR) status);
                }
                break;

            case BQB_ACTION_HFHS_AG_SIGNAL_STRENGTH_SEND:
                {
                    uint8_t   value;
                    LE_STREAM_TO_UINT8(value, p);
                    bt_hfp_ag_signal_strength_send(p, value);
                }
                break;

            case BQB_ACTION_HFHS_AG_ROAMING_STATUS_SEND:
                {
                    uint8_t   value;
                    LE_STREAM_TO_UINT8(value, p);
                    bt_hfp_ag_roaming_indicator_send(p, (T_BT_HFP_AG_ROAMING_INDICATOR) value);
                }
                break;

            case BQB_ACTION_HFHS_AG_BATTERY_CHANGE_SEND:
                {
                    uint8_t   value;
                    LE_STREAM_TO_UINT8(value, p);
                    bt_hfp_ag_battery_charge_send(p, value);
                }
                break;

            case BQB_ACTION_HFHS_AG_OK_SEND:
                {
                    bt_hfp_ag_ok_send(p);
                }
                break;

            case BQB_ACTION_HFHS_AG_ERROR_SEND:
                {
                    uint8_t   cme_error_code;
                    LE_STREAM_TO_UINT8(cme_error_code, p);
                    bt_hfp_ag_error_send(p, cme_error_code);
                }
                break;

            case  BQB_ACTION_HFHS_AG_NETWORK_OPERATOR_SEND:
                {
                    char      operator[17];
                    uint8_t   operator_len;

                    operator_len = strlen((char *)p) + 1;
                    memcpy_s(operator, sizeof(operator), p, operator_len);
                    p += operator_len;

                    bt_hfp_ag_network_operator_name_send(p, operator, operator_len);
                    bt_hfp_ag_ok_send(p);
                }
                break;

            case  BQB_ACTION_HFHS_AG_SUBSCRIBER_NUMBER_SEND:
                {
                    char      call_num[20];
                    uint8_t   call_num_len;
                    uint8_t   call_type;
                    uint8_t   service;

                    call_num_len = strlen((char *)p) + 1;
                    memcpy_s(call_num, sizeof(call_num), p, call_num_len);
                    p += call_num_len;
                    LE_STREAM_TO_UINT8(call_type, p);
                    LE_STREAM_TO_UINT8(service, p);

                    bt_hfp_ag_subscriber_number_send(p, call_num, call_num_len, call_type, service);
                }
                break;

            case  BQB_ACTION_HFHS_AG_INDICATORS_STATUS_SEND:
                {
                    uint8_t service_indicator;
                    uint8_t call_indicator;
                    uint8_t call_setup_indicator;
                    uint8_t call_held_indicator;
                    uint8_t signal_indicator;
                    uint8_t roaming_indicator;
                    uint8_t batt_chg_indicator;

                    LE_STREAM_TO_UINT8(service_indicator, p);
                    LE_STREAM_TO_UINT8(call_indicator, p);
                    LE_STREAM_TO_UINT8(call_setup_indicator, p);
                    LE_STREAM_TO_UINT8(call_held_indicator, p);
                    LE_STREAM_TO_UINT8(signal_indicator, p);
                    LE_STREAM_TO_UINT8(roaming_indicator, p);
                    LE_STREAM_TO_UINT8(batt_chg_indicator, p);
                    bt_hfp_ag_indicators_send(p,
                                              (T_BT_HFP_AG_SERVICE_INDICATOR)service_indicator,
                                              (T_BT_HFP_AG_CALL_INDICATOR)call_indicator,
                                              (T_BT_HFP_AG_CALL_SETUP_INDICATOR)call_setup_indicator,
                                              (T_BT_HFP_AG_CALL_HELD_INDICATOR)call_held_indicator,
                                              signal_indicator,
                                              (T_BT_HFP_AG_ROAMING_INDICATOR)roaming_indicator,
                                              batt_chg_indicator);
                    bt_hfp_ag_ok_send(p);

                }
                break;

            case  BQB_ACTION_HFHS_AG_VENDOR_CMD_SEND:
                {
                    char      vnd_cmd[50];
                    uint8_t   vnd_cmd_len;

                    vnd_cmd_len = strlen((char *)p) + 1;
                    memcpy_s(vnd_cmd, sizeof(vnd_cmd), p, vnd_cmd_len);
                    p += vnd_cmd_len;

                    bt_hfp_ag_send_vnd_at_cmd_req(p, vnd_cmd);
                }
                break;

            case BQB_ACTION_HFHS_AG_SCO_CONN_REQ:
                {
                    uint16_t max_latency;
                    uint16_t   voice_setting;
                    uint8_t    retrans_effort;
                    LE_STREAM_TO_UINT16(max_latency, p);
                    LE_STREAM_TO_UINT16(voice_setting, p);
                    LE_STREAM_TO_UINT8(retrans_effort, p);
                    gap_br_send_sco_conn_req(p, 8000, 8000, max_latency, voice_setting, retrans_effort,
                                             SCO_PKT_TYPES_HV3_EV3_2EV3);
                }
                break;

            default:
                break;
            }
        }
        else if (id == BQB_CMD_PBAP)
        {
            LE_STREAM_TO_UINT8(action, p);
            switch (action)
            {
            case BQB_ACTION_PBAP_CONN_RFC:
                {
                    uint8_t   server_chann;
                    uint8_t   feature_flag;

                    LE_STREAM_TO_UINT8(server_chann, p);
                    LE_STREAM_TO_UINT8(feature_flag, p);

                    if (feature_flag)
                    {
                        bt_pbap_connect_over_rfc_req(p, server_chann, true);
                    }
                    else
                    {
                        bt_pbap_connect_over_rfc_req(p, server_chann, false);
                    }
                }
                break;

            case BQB_ACTION_PBAP_CONN_L2C:
                {
                    uint16_t   psm;
                    uint8_t    feature_flag;

                    LE_STREAM_TO_UINT16(psm, p);
                    LE_STREAM_TO_UINT8(feature_flag, p);

                    if (feature_flag)
                    {
                        bt_pbap_connect_over_l2c_req(p, psm, true);
                    }
                    else
                    {
                        bt_pbap_connect_over_l2c_req(p, psm, false);
                    }
                }
                break;

            case BQB_ACTION_PBAP_VCARD_SRM:
            case BQB_ACTION_PBAP_VCARD_NOSRM:
                {
                    char *phone_number;
                    phone_number = "10010";
                    bt_pbap_vcard_listing_by_number_pull(p, phone_number);
                }
                break;

            case BQB_ACTION_PBAP_VCARD_ENTRY:
                {
                    uint8_t p_name[12] = {0x00, 0x30, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00};
                    bt_pbap_pull_vcard_entry(p, p_name, sizeof(p_name), 0xffffffff);
                }
                break;

            case BQB_ACTION_PBAP_SET_PHONE_BOOK:
                {
                    bt_pbap_phone_book_set(p, BT_PBAP_REPOSITORY_LOCAL, BT_PBAP_PATH_PB);
                }
                break;

            case BQB_ACTION_PBAP_PULL_PHONE_BOOK:
                {
                    bt_pbap_phone_book_pull(p, BT_PBAP_REPOSITORY_LOCAL, BT_PBAP_PHONE_BOOK_PB, 0, 0x20, 0);
                }
                break;

            default:
                break;
            }
        }
        else if (id == BQB_CMD_MAP)
        {
            LE_STREAM_TO_UINT8(action, p);
            switch (action)
            {
            case BQB_ACTION_MAP_CONNECT_MAS_RFC:
                {
                    uint8_t   server_chann;
                    LE_STREAM_TO_UINT8(server_chann, p);
                    bt_map_mas_connect_over_rfc_req(p, server_chann);
                }
                break;

            case BQB_ACTION_MAP_CONNECT_MAS_L2C:
                {
                    uint16_t   psm;
                    LE_STREAM_TO_UINT16(psm, p);
                    bt_map_mas_connect_over_l2c_req(p, psm);
                }
                break;

            case BQB_ACTION_MAP_DISCONNECT_MAS:
                {
                    bt_map_mas_disconnect_req(p);
                }
                break;

            case BQB_ACTION_MAP_MNS_ON:
                {
                    bt_map_mas_msg_notification_set(p, true);
                }
                break;

            case BQB_ACTION_MAP_MNS_OFF:
                {
                    bt_map_mas_msg_notification_set(p, false);
                }
                break;

            case BQB_ACTION_MAP_FOLDER_LISTING:
                {
                    bt_map_mas_folder_listing_get(p, 10, 0);
                }
                break;

            case BQB_ACTION_MAP_SET_FOLDER_INBOX:
                {
                    bt_map_mas_folder_set(p, BT_MAP_FOLDER_INBOX);
                }
                break;

            case BQB_ACTION_MAP_SET_FOLDER_OUTBOX:
                {
                    bt_map_mas_folder_set(p, BT_MAP_FOLDER_OUTBOX);
                }
                break;

            case BQB_ACTION_MAP_MSG_LISTING:
                {
                    //NULL terminated UNICODE : inbox
                    uint8_t map_path_inbox[12] =
                    {
                        0x00, 0x69, 0x00, 0x6e, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x78, 0x00, 0x00
                    };

                    bt_map_mas_msg_listing_get(p, map_path_inbox, 12, 10, 0);
                }
                break;

            case BQB_ACTION_MAP_GET_MSG:
                {
                    //msg handle = "0123456789000002"
                    uint8_t handle[34] =
                    {
                        0x00, 0x30, 0x00, 0x31, 0x00, 0x32, 0x00, 0x33, 0x00, 0x34, 0x00, 0x35, 0x00, 0x36,
                        0x00, 0x37, 0x00, 0x38, 0x00, 0x39, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30,
                        0x00, 0x30, 0x00, 0x32, 0x00, 0x00
                    };

                    bt_map_mas_msg_get(p, handle, 34, false);
                }
                break;

            case BQB_ACTION_MAP_UPLOAD:
            case BQB_ACTION_MAP_UPLOAD_SRM:
                {
                    //NULL terminated UNICODE : outbox
                    uint8_t map_path_outbox[14] =
                    {
                        0x00, 0x6f, 0x00, 0x75, 0x00, 0x74, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x78, 0x00, 0x00
                    };
                    uint8_t msg[248] =
                    {
                        0x42, 0x45, 0x47, 0x49, 0x4E, 0x3A, 0x42, 0x4D, 0x53, 0x47, 0xD, 0xA, //BEGIN:BMSG
                        0x56, 0x45, 0x52, 0x53, 0x49, 0x4F, 0x4E, 0x3A, 0x31, 0x2E, 0x30, 0xD, 0xA, //VERSION:1.0
                        0x53, 0x54, 0x41, 0x54, 0x55, 0x53, 0x3A, 0x52, 0x45, 0x41, 0x44, 0xD, 0xA, //STATUS:READ
                        0x54, 0x59, 0x50, 0x45, 0x3A, 0x53, 0x4D, 0x53, 0x5F, 0x43, 0x44, 0x4D, 0x41, 0xD, 0xA, //TYPE:SMS_CDMA
                        0x46, 0x4F, 0x4C, 0x44, 0x45, 0x52, 0x3A, 0x74, 0x65, 0x6C, 0x65, 0x63, 0x6F, 0x6D, 0x2F,
                        0x6D, 0x73, 0x67, 0x2F, 0x4F, 0x55, 0x54, 0x42, 0x4F, 0x58, 0xD, 0xA, //FOLDER:telecom/msg/outbox
                        0x42, 0x45, 0x47, 0x49, 0x4E, 0x3A, 0x20, 0x42, 0x45, 0x4E, 0x56, 0xD, 0xA, //BEGIN:BENV
                        0x42, 0x45, 0x47, 0x49, 0x4E, 0x3A, 0x20, 0x56, 0x43, 0x41, 0x52, 0x44, 0xD, 0xA, //BEGIN:VCARD
                        0x56, 0x45, 0x52, 0x53, 0x49, 0x4F, 0x4E, 0x3A, 0x20, 0x33, 0x2E, 0x30, 0xD, 0xA, //VERSION:3.0
                        0x46, 0x4E, 0x3A, 0xD, 0xA, //FN:
                        0x4E, 0x3A, 0xD, 0xA, //N:
                        0x54, 0x45, 0x4C, 0x3A, 0x31, 0x30, 0x30, 0x38, 0x36, 0xD, 0xA, //TEL:10086
                        0x45, 0x4E, 0x44, 0x3A, 0x20, 0x56, 0x43, 0x41, 0x52, 0x44, 0xD, 0xA, //END:VCARD
                        0x42, 0x45, 0x47, 0x49, 0x4E, 0x3A, 0x42, 0x42, 0x4F, 0x44, 0x59, 0xD, 0xA, //BEGIN:BBODY
                        0x4C, 0x45, 0x4E, 0x47, 0x54, 0x48, 0x3A, 0x32, 0x35, 0xD, 0xA, //LENGTH:25
                        0x43, 0x48, 0x41, 0x52, 0x53, 0x45, 0x54, 0x3A, 0x55, 0x54, 0x46, 0x2D, 0x38, 0xD, 0xA, //CHARSET:UTF-8
                        0x42, 0x45, 0x47, 0x49, 0x4E, 0x3A, 0x4D, 0x53, 0x47, 0xD, 0xA, //BEGIN:MSG
                        0x31, 0x32, 0x33, 0xD, 0xA, //123
                        0x45, 0x4E, 0x44, 0x3A, 0x4D, 0x53, 0x47, 0xD, 0xA, //END:MSG
                        0x45, 0x4E, 0x44, 0x3A, 0x42, 0x42, 0x4F, 0x44, 0x59, 0xD, 0xA, //END:BBODY
                        0x45, 0x4E, 0x44, 0x3A, 0x42, 0x45, 0x4E, 0x56, 0xD, 0xA, //END:BENV
                        0x45, 0x4E, 0x44, 0x3A, 0x42, 0x4D, 0x53, 0x47, 0xD, 0xA  //END:BMSG
                    };

                    if (action == BQB_ACTION_MAP_UPLOAD)
                    {
                        uint16_t repeat_count = 0;
                        uint16_t i;

                        LE_STREAM_TO_UINT16(repeat_count, p);
                        for (i = 0; i < repeat_count; i++)
                        {
                            bt_map_mas_msg_push(p, NULL, 0, false, true, msg, 248);
                            os_delay(20);
                        }

                        bt_map_mas_msg_push(p, NULL, 0, false, false, msg, 248);
                    }
                    else
                    {
                        bt_map_mas_msg_push(p, map_path_outbox, 14, false, true, msg, 248);
                    }
                }
                break;

            default:
                break;
            }
        }

        free(console_msg.u.buf);
        break;

    case  IO_MSG_CONSOLE_BINARY_RX:
        {
            p += 1;
            LE_STREAM_TO_UINT8(rx_seqn, p);
            LE_STREAM_TO_UINT16(cmd_len, p);

            app_handle_cmd_set(p, cmd_len, CMD_PATH_UART, rx_seqn, 0);
        }
        free(console_msg.u.buf);
        break;

    default:
        break;
    }
}


