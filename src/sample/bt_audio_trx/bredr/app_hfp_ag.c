/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "btm.h"
#include "trace.h"
#include "gap_br.h"
#include "app_hfp_cfg.h"
#include "app_link_util_cs.h"
#include "app_hfp_ag.h"
#include "app_sdp.h"
#include "app_cmd.h"
#include "app_report.h"

#include "app_main.h"
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_cmd.h"
#endif
#if F_APP_HID_TELEPHONY_SUPPORT
#include "app_tri_dongle_telephony.h"
#endif
#define SCO_PKT_TYPES_HV3_EV3_2EV3          (GAP_PKT_TYPE_HV3 | GAP_PKT_TYPE_EV3 | GAP_PKT_TYPE_NO_3EV3 | GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)


static uint8_t active_link_id;
bool hfp_ag_in_call = false;
bool hfp_ag_incoming_call = false;
bool hfp_ag_outgoing_call = false;
bool hfp_ag_hold_call = false;
bool hfp_ag_hold_mute = false;

static void rpt_evt(T_EVENT_ID evt, uint8_t *data, uint16_t len)
{
    app_report_event(CMD_PATH_UART, evt, 0, data, len);
}


static void rpt_evt_with_link_id(T_EVENT_ID evt, uint8_t *addr)
{
    T_APP_BR_LINK *p_link = app_link_find_br_link(addr);
    if (p_link != NULL)
    {
        struct
        {
            uint8_t app_link_id;
        } __attribute__((packed)) rpt = {};
        rpt.app_link_id = p_link->id;
        rpt_evt(evt, (uint8_t *)&rpt, sizeof(rpt));
    }
}


static void app_hfp_ag_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;
    T_BT_HFP_AG_CALL_STATUS call_status = app_db.br_link[active_link_id].hfp_ag.call_status;

    switch (event_type)
    {
    case BT_EVENT_HFP_AG_CONN_IND:
        {
            rpt_evt_with_link_id(EVENT_HFP_AG_CONN_IND, param->hfp_ag_conn_ind.bd_addr);
            bt_hfp_ag_connect_cfm(param->hfp_ag_conn_ind.bd_addr, true);
        }
        break;

    case BT_EVENT_HFP_AG_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                p_link->hfp.call_id_type_chk = true;
                p_link->hfp.call_id_type_num = false;
                if (param->hfp_ag_conn_cmpl.is_hfp)
                {
                    p_link->connected_profile |= HFP_PROFILE_MASK;
                    rpt_evt(EVENT_HFP_AG_CONN_CMPL, (uint8_t *)&p_link->bd_addr, 6);
                }
                else
                {
                    p_link->connected_profile |= HSP_PROFILE_MASK;
                }
                active_link_id = p_link->id;
            }
        }
        break;

    case BT_EVENT_HFP_AG_DISCONN_CMPL:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (p_link->connected_profile & HFP_PROFILE_MASK)
                {
                    p_link->connected_profile &= ~HFP_PROFILE_MASK;
                    struct
                    {
                        uint8_t app_link_id;
                        uint16_t cause;
                    } __attribute__((packed)) rpt = {};

                    rpt.app_link_id = p_link->id;
                    rpt.cause = param->hfp_ag_disconn_cmpl.cause;
                    rpt_evt(EVENT_HFP_AG_DISCONN_CMPL, (uint8_t *)&rpt, sizeof(rpt));
                }
                else
                {
                    p_link->connected_profile &= ~HSP_PROFILE_MASK;
                }
            }
        }
        break;

    case BT_EVENT_HFP_AG_MIC_VOLUME_CHANGED:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_mic_volume_changed.bd_addr);
            if (p_link != NULL)
            {
                struct
                {
                    uint8_t app_link_id;
                    uint8_t volume;
                } __attribute__((packed)) rpt = {};

                rpt.app_link_id = p_link->id;
                rpt.volume = param->hfp_ag_mic_volume_changed.volume;
                rpt_evt(EVENT_HFP_AG_MIC_VOL_CHANGED, (uint8_t *)&rpt, sizeof(rpt));
            }

        }
        break;

    case BT_EVENT_HFP_AG_SPK_VOLUME_CHANGED:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_spk_volume_changed.bd_addr);
            if (p_link != NULL)
            {
                struct
                {
                    uint8_t app_link_id;
                    uint8_t volume;
                } __attribute__((packed)) rpt = {};

                rpt.app_link_id = p_link->id;
                rpt.volume = param->hfp_ag_spk_volume_changed.volume;
                rpt_evt(EVENT_HFP_AG_SPK_VOL_CHANGED, (uint8_t *)&rpt, sizeof(rpt));
            }
        }
        break;

    case BT_EVENT_HFP_AG_CALL_STATUS_CHANGED:
        {
            //update AG call_status.
            app_db.br_link[active_link_id].hfp_ag.call_status =
                (T_BT_HFP_AG_CALL_STATUS)param->hfp_ag_call_status_changed.curr_status;

            //notify app to start or stop audio stream.
            if (param->hfp_ag_call_status_changed.curr_status == BT_HFP_AG_CALL_ACTIVE ||
                param->hfp_ag_call_status_changed.curr_status == BT_HFP_AG_CALL_INCOMING ||
                param->hfp_ag_call_status_changed.curr_status == BT_HFP_AG_CALL_OUTGOING)
            {
                //notify app to start audio stream.
                //app should check codec type with BT_EVENT_HFP_AG_AG_HF_ACTIVE_CODEC_TYPE.
            }
            else if (param->hfp_ag_call_status_changed.curr_status == BT_HFP_AG_CALL_IDLE)
            {
                //notify app to stop audio stream.
            }

            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_call_status_changed.bd_addr);
            if (p_link != NULL)
            {
                struct
                {
                    uint8_t app_link_id;
                    uint8_t status;
                } __attribute__((packed)) rpt = {};

                rpt.app_link_id = p_link->id;
                rpt.status = param->hfp_ag_call_status_changed.curr_status;
                rpt_evt(EVENT_HFP_AG_CALL_STATUS_CHANGED, (uint8_t *)&rpt, sizeof(rpt));
            }
        }
        break;

    case BT_EVENT_HFP_AG_INDICATORS_STATUS_REQ:
        {
#if (F_APP_HID_TELEPHONY_SUPPORT == 0)
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_indicators_status_req.bd_addr);
            if (p_link != NULL)
            {
                //Td App provide current network status.
                T_BT_HFP_AG_SERVICE_INDICATOR service_indicator = BT_HFP_AG_SERVICE_STATUS_AVAILABLE;
                T_BT_HFP_AG_CALL_INDICATOR call_indicator;
                T_BT_HFP_AG_CALL_SETUP_INDICATOR call_setup_indicator;
                T_BT_HFP_AG_CALL_HELD_INDICATOR call_held_indicator;

                if (hfp_ag_in_call)
                {
                    call_setup_indicator = BT_HFP_AG_CALL_SETUP_STATUS_IDLE;
                    call_indicator = BT_HFP_AG_CALL_IN_PROGRESS;
                }
                else if (hfp_ag_incoming_call)
                {
                    call_setup_indicator = BT_HFP_AG_CALL_SETUP_STATUS_INCOMING_CALL;
                    call_indicator = BT_HFP_AG_NO_CALL_IN_PROGRESS;
                }
                else if (hfp_ag_outgoing_call)
                {
                    call_setup_indicator = BT_HFP_AG_CALL_SETUP_STATUS_IDLE;
                    call_indicator = BT_HFP_AG_NO_CALL_IN_PROGRESS;
                }
                else
                {
                    call_setup_indicator = BT_HFP_AG_CALL_SETUP_STATUS_IDLE;
                    call_indicator = BT_HFP_AG_NO_CALL_IN_PROGRESS;
                }

                if ((hfp_ag_hold_call) && (!hfp_ag_hold_mute))
                {
                    call_held_indicator = BT_HFP_AG_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL;
                }
                else if ((hfp_ag_hold_call) && (hfp_ag_hold_mute))
                {
                    call_held_indicator = BT_HFP_AG_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL;
                }
                else
                {
                    call_held_indicator = BT_HFP_AG_CALL_HELD_STATUS_IDLE;
                }

                uint8_t signal_indicator = 3;
                T_BT_HFP_AG_ROAMING_INDICATOR roaming_indicator = BT_HFP_AG_ROAMING_STATUS_INACTIVE;
                uint8_t batt_chg_indicator = 3;

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
#endif
            rpt_evt_with_link_id(EVENT_HFP_AG_INDI_STATUS_REQ,
                                 param->hfp_ag_indicators_status_req.bd_addr);
        }
        break;

    case BT_EVENT_HFP_AG_BATTERY_LEVEL:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_battery_level.bd_addr);
            if (p_link != NULL)
            {
                struct
                {
                    uint8_t app_link_id;
                    uint8_t batt_level;
                } __attribute__((packed)) rpt = {};

                rpt.app_link_id = p_link->id;
                rpt.batt_level = param->hfp_ag_battery_level.battery_level;

                rpt_evt(EVENT_HFP_AG_BATT_LEVEL, (uint8_t *)&rpt, sizeof(rpt));
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
                app_tri_dongle_notify_br_hf_bat_info(param->hfp_ag_battery_level.bd_addr,
                                                     param->hfp_ag_battery_level.battery_level);
#endif
            }
        }
        break;

    case BT_EVENT_HFP_AG_CODEC_TYPE_SELECTED:
        //update app hfp codec type.
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_codec_type_selected.bd_addr);
            if (p_link != NULL)
            {
                struct
                {
                    uint8_t app_link_id;
                    uint8_t codec_type;
                } __attribute__((packed)) rpt = {};

                rpt.app_link_id = p_link->id;
                rpt.codec_type = param->hfp_ag_codec_type_selected.codec_type;
                rpt_evt(EVENT_HFP_AG_HF_CODEC_TYPE_SELECTED, (uint8_t *)&rpt, sizeof(rpt));
            }
        }
        break;

    case BT_EVENT_HFP_AG_SUPPORTED_FEATURES:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_supported_features.bd_addr);
            if (p_link != NULL)
            {
                struct
                {
                    uint8_t app_link_id;
                    uint32_t feature_bitmap;
                } __attribute__((packed)) rpt = {};

                rpt.app_link_id = p_link->id;
                rpt.feature_bitmap = param->hfp_ag_supported_features.hf_bitmap;
                rpt_evt(EVENT_HFP_AG_SUPPORTED_FEATURES, (uint8_t *)&rpt, sizeof(rpt));
            }
        }
        break;

    case BT_EVENT_HFP_AG_INBAND_RINGING_REQ:
        {
            rpt_evt_with_link_id(EVENT_HFP_AG_INBAND_RINGING_REQ,
                                 param->hfp_ag_inband_ringing_req.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_CALL_ANSWER_REQ:
        {
#if (F_APP_HID_TELEPHONY_SUPPORT == 0)
            bt_hfp_ag_call_answer(param->hfp_ag_call_answer_req.bd_addr);
            bt_hfp_ag_audio_connect_req(param->hfp_ag_call_answer_req.bd_addr);
#endif
            rpt_evt_with_link_id(EVENT_HFP_AG_CALL_ANSWER_REQ,
                                 param->hfp_ag_call_answer_req.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_CALL_TERMINATE_REQ:
        {
#if (F_APP_HID_TELEPHONY_SUPPORT == 0)
            if ((call_status == BT_HFP_AG_CALL_INCOMING) ||
                (call_status == BT_HFP_AG_CALL_OUTGOING) ||
                (call_status == BT_HFP_AG_CALL_ACTIVE))
            {
                bt_hfp_ag_call_terminate(param->hfp_ag_call_terminate_req.bd_addr);
                bt_hfp_ag_audio_disconnect_req(param->hfp_ag_call_terminate_req.bd_addr);
            }

            if (call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
            {
                bt_hfp_ag_call_indicator_send(param->hfp_ag_call_terminate_req.bd_addr,
                                              BT_HFP_AG_NO_CALL_IN_PROGRESS);
            }

            if (call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                bt_hfp_ag_call_held_indicator_send(param->hfp_ag_call_terminate_req.bd_addr,
                                                   BT_HFP_AG_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL);
            }
#endif
            rpt_evt_with_link_id(EVENT_HFP_AG_CALL_TERM_REQ,
                                 param->hfp_ag_call_terminate_req.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_CURR_CALLS_LIST_QUERY:
        {
            //send list of current calls.
#if 0
            bt_hfp_ag_current_calls_list_send(uint8_t                         *bd_addr,
                                              uint8_t                          call_idx,
                                              uint8_t                          call_dir,
                                              T_BT_HFP_AG_CURRENT_CALL_STATUS  call_status,
                                              T_BT_HFP_AG_CURRENT_CALL_MODE    call_mode,
                                              uint8_t                          mpty,
                                              const char                      *call_num,
                                              uint8_t                          call_type);
            bt_hfp_ok_send(param->hfp_ag_curr_calls_list_query.bd_addr);
#endif

            rpt_evt_with_link_id(EVENT_HFP_AG_CURR_CALLS_LIST_QUERY,
                                 param->hfp_ag_curr_calls_list_query.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_DTMF_CODE:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_dtmf_code.bd_addr);
            if (p_link != NULL)
            {
                struct
                {
                    uint8_t app_link_id;
                    uint8_t code;
                } __attribute__((packed)) rpt = {};

                rpt.app_link_id = p_link->id;
                rpt.code = param->hfp_ag_dtmf_code.dtmf_code;
                rpt_evt(EVENT_HFP_AG_DTMF_CODE, (uint8_t *)&rpt, sizeof(rpt));
            }
        }
        break;

    case  BT_EVENT_HFP_AG_DIAL_WITH_NUMBER:
        {
            bt_hfp_ag_call_dial(param->hfp_ag_dial_with_number.bd_addr);

            struct
            {
                uint8_t app_link_id;
                uint8_t num_len;
                uint8_t number[sizeof(param->hfp_ag_dial_with_number.number)];
            } __attribute__((packed)) rpt = {};

            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_dial_with_number.bd_addr);
            if (p_link != NULL)
            {
                rpt.app_link_id = p_link->id;
                rpt.num_len = strlen(param->hfp_ag_dial_with_number.number) + 1;
                memcpy(rpt.number, param->hfp_ag_dial_with_number.number, sizeof(rpt.number));
                rpt_evt(EVENT_HFP_AG_DIAL_WITH_NUMBER, (uint8_t *)&rpt, sizeof(rpt));
            }
        }
        break;

    case  BT_EVENT_HFP_AG_DIAL_LAST_NUMBER:
        {
#if 0
            //dial last number when index of memory is valid,
            //otherwise, send error with BT_HFP_AG_ERR_INVALID_INDEX.

            if (invalid_mem_index)
            {
                bt_hfp_ag_ok_send(param->hfp_ag_dial_last_number.bd_addr);
                bt_hfp_ag_call_dial(param->hfp_ag_dial_last_number.bd_addr);
            }
            else
            {
                bt_hfp_ag_error_send(param->hfp_ag_dial_last_number.bd_addr, BT_HFP_AG_ERR_INVALID_INDEX);
            }
#endif

            rpt_evt_with_link_id(EVENT_HFP_AG_DIAL_LAST_NUMBER,
                                 param->hfp_ag_dial_last_number.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_3WAY_HELD_CALL_RELEASED:
        {

            //handle with AT cmd "AT+CHLD=0".
            //releases all held calls or sets User Determined User Busy (UDUB) for a waiting call.
            bt_hfp_ag_ok_send(param->hfp_ag_3way_held_call_released.bd_addr);
            if (call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
            {
                //sets User Determined User Busy (UDUB) for a waiting call.
                bt_hfp_ag_call_setup_indicator_send(param->hfp_ag_3way_held_call_released.bd_addr,
                                                    BT_HFP_AG_CALL_SETUP_STATUS_IDLE);
            }
            else if (call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                //releases all held calls
            }
            bt_hfp_ag_call_held_indicator_send(param->hfp_ag_3way_held_call_released.bd_addr,
                                               BT_HFP_AG_CALL_HELD_STATUS_IDLE);

            rpt_evt_with_link_id(EVENT_HFP_AG_3WAY_HELD_CALL_RELEASED,
                                 param->hfp_ag_3way_held_call_released.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_3WAY_ACTIVE_CALL_RELEASED:
        {
            //handle with AT cmd "AT+CHLD=1".
            //releases all active calls (if any exist) and accepts the other (held or waiting) call.
            if (call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
            {
                if (true)
                {
                    //if function supported.
                    bt_hfp_ag_ok_send(param->hfp_ag_3way_active_call_released.bd_addr);
                    bt_hfp_ag_call_setup_indicator_send(param->hfp_ag_3way_active_call_released.bd_addr,
                                                        BT_HFP_AG_CALL_SETUP_STATUS_IDLE);
                }
                else
                {
                    bt_hfp_ag_error_send(param->hfp_ag_3way_active_call_released.bd_addr,
                                         BT_HFP_AG_ERR_NO_NETWORK_SERVICE);
                }
            }
            else if (call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                bt_hfp_ag_ok_send(param->hfp_ag_3way_active_call_released.bd_addr);
                bt_hfp_ag_call_held_indicator_send(param->hfp_ag_3way_active_call_released.bd_addr,
                                                   BT_HFP_AG_CALL_HELD_STATUS_IDLE);

            }

            rpt_evt_with_link_id(EVENT_HFP_AG_3WAY_ACTIVE_CALL_RELEASED,
                                 param->hfp_ag_3way_held_call_released.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_3WAY_SWITCHED:
        {
            //handle with AT cmd "AT+CHLD=2".
            //places all active calls (if any exist) on hold and accepts the other (held or waiting) call.
            if (call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
            {
                if (true)
                {
                    //if function supported.
                    bt_hfp_ag_ok_send(param->hfp_ag_3way_switched.bd_addr);
                    bt_hfp_ag_call_setup_indicator_send(param->hfp_ag_3way_switched.bd_addr,
                                                        BT_HFP_AG_CALL_SETUP_STATUS_IDLE);
                    bt_hfp_ag_call_held_indicator_send(param->hfp_ag_3way_switched.bd_addr,
                                                       BT_HFP_AG_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL);
                }
                else
                {
                    bt_hfp_ag_error_send(param->hfp_ag_3way_switched.bd_addr,
                                         BT_HFP_AG_ERR_NO_NETWORK_SERVICE);
                }
            }
            else if (call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                bt_hfp_ag_ok_send(param->hfp_ag_3way_switched.bd_addr);
                bt_hfp_ag_call_held_indicator_send(param->hfp_ag_3way_switched.bd_addr,
                                                   BT_HFP_AG_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL);

            }

            rpt_evt_with_link_id(EVENT_HFP_AG_3WAY_SWITCHED,
                                 param->hfp_ag_3way_switched.bd_addr);
        }
        break;

    case BT_EVENT_HFP_AG_3WAY_MERGED:
        {
            //handle with AT cmd "AT+CHLD=3".
            //adds a held call to the conversation.
            if (call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                if (true)
                {
                    //if function supported.
                    bt_hfp_ag_ok_send(param->hfp_ag_3way_merged.bd_addr);
                    bt_hfp_ag_call_held_indicator_send(param->hfp_ag_3way_merged.bd_addr,
                                                       BT_HFP_AG_CALL_HELD_STATUS_IDLE);
                }
                else
                {
                    bt_hfp_ag_error_send(param->hfp_ag_3way_merged.bd_addr, BT_HFP_AG_ERR_NO_NETWORK_SERVICE);
                }
            }

            rpt_evt_with_link_id(EVENT_HFP_AG_3WAY_MERGED,
                                 param->hfp_ag_3way_merged.bd_addr);
        }
        break;

    case BT_EVENT_HFP_AG_SUBSCRIBER_NUMBER_QUERY:
        {
            rpt_evt_with_link_id(EVENT_HFP_AG_SUBSCRIBER_NUMBER_QUERY,
                                 param->hfp_ag_subscriber_number_query.bd_addr);
        }
        break;

    case   BT_EVENT_HFP_AG_NETWORK_NAME_FORMAT_SET:
        {
            rpt_evt_with_link_id(EVENT_HFP_AG_NETWORK_NAME_FORMAT_SET,
                                 param->hfp_ag_network_name_format_set.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_CURR_OPERATOR_QUERY:
        {
            rpt_evt_with_link_id(EVENT_HFP_AG_CURR_OPERATOR_QUERY,
                                 param->hfp_ag_curr_operator_query.bd_addr);
        }
        break;

    case  BT_EVENT_HFP_AG_ENHANCED_SAFETY_STATUS:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->hfp_ag_enhanced_safety_status.bd_addr);
            if (p_link != NULL)
            {
                struct
                {
                    uint8_t app_link_id;
                    uint8_t status;
                } __attribute__((packed)) rpt = {};
                rpt.app_link_id = p_link->id;
                rpt.status = param->hfp_ag_enhanced_safety_status.status;
                rpt_evt(EVENT_HFP_AG_ENHANCED_SAFETY_STATUS, (uint8_t *)&rpt, sizeof(rpt));
            }
        }
        break;

    case BT_EVENT_HFP_AG_NREC_STATUS:
        {
            bt_hfp_ag_ok_send(param->hfp_ag_nrec_status.bd_addr);
        }
        break;

    case BT_EVENT_HFP_AG_VENDOR_CMD:
        {
            bt_hfp_ag_error_send(param->hfp_ag_vendor_cmd.bd_addr, BT_HFP_AG_ERR_INVALID_CHAR_IN_TSTR);
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_hfp_ag_bt_cback: event_type 0x%04x", event_type);
    }
}

T_BT_HFP_AG_CALL_STATUS app_hfp_ag_get_call_status(void)
{
    return app_db.br_link[active_link_id].hfp_ag.call_status;
}

uint8_t app_hfp_ag_get_active_hfp_ag_index(void)
{
    return active_link_id;
}

bool app_hfp_ag_set_active_hfp_ag_index(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link)
    {
        active_link_id = p_link->id;
    }
    return false;
}

void *app_hfp_ag_get_active_hfp_ag_link(void)
{
    T_APP_BR_LINK *p_link = NULL;

    p_link = app_link_find_br_link_by_link_id(active_link_id);

    if (p_link != NULL)
    {
        if (!(p_link->connected_profile & HFP_PROFILE_MASK) &&
            !(p_link->connected_profile & HSP_PROFILE_MASK))
        {
            p_link = NULL;
        }
    }

    return p_link;

}

static void handle_cmd_with_link_id(uint8_t *cmd_ptr, bool (*handler)(uint8_t addr[6]))
{
    struct
    {
        uint16_t cmd_id;
        uint8_t app_link_id;
    } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

    if (handler) { handler(app_db.br_link[cmd->app_link_id].bd_addr); }
}


static void handle_cmd_with_gain(uint8_t *cmd_ptr, bool (*handler)(uint8_t addr[6],
                                                                   uint8_t gain_level))
{
    struct
    {
        uint16_t    cmd_id;
        uint8_t     app_link_id;
        uint8_t     gain_level;
    } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

    if (handler) { handler(app_db.br_link[cmd->app_link_id].bd_addr, cmd->gain_level); }
}


void app_hfp_ag_handle_cmd(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                           uint8_t rx_seqn, uint8_t app_idx)
{
    bool cmd_handled = true;
    uint16_t cmd_id;

    struct
    {
        uint16_t cmd_id;
        uint8_t status;
    } __attribute__((packed)) ack_pkt = {};

    cmd_id     = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    ack_pkt.cmd_id = cmd_id;
    ack_pkt.status = CMD_SET_STATUS_COMPLETE;

    switch (cmd_id)
    {
    case CMD_HFP_AG_CONNECT_SCO:
        {
            handle_cmd_with_link_id(cmd_ptr, bt_hfp_ag_audio_connect_req);
        }
        break;

    case CMD_HFP_AG_DISCONNECT_SCO:
        {
            handle_cmd_with_link_id(cmd_ptr, bt_hfp_ag_audio_disconnect_req);
        }
        break;

    case CMD_HFP_AG_CALL_INCOMING:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     call_num_type;
                uint8_t     call_num_len;
                uint8_t     call_num[];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_call_incoming(app_db.br_link[cmd->app_link_id].bd_addr,
                                    (char *)cmd->call_num, cmd->call_num_len, cmd->call_num_type);
        }
        break;

    case CMD_HFP_AG_CALL_ANSWER:
        {
            handle_cmd_with_link_id(cmd_ptr, bt_hfp_ag_call_answer);
        }
        break;

    case CMD_HFP_AG_CALL_TERMINATE:
        {
            handle_cmd_with_link_id(cmd_ptr, bt_hfp_ag_call_terminate);
        }
        break;

    case CMD_HFP_AG_CALL_DIAL:
        {
            handle_cmd_with_link_id(cmd_ptr, bt_hfp_ag_call_dial);
        }
        break;

    case CMD_HFP_AG_CALL_ALERTING:
        {
            handle_cmd_with_link_id(cmd_ptr, bt_hfp_ag_call_alert);
        }
        break;

    case CMD_HFP_AG_CALL_WAITING:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     call_num_type;
                uint8_t     call_num_len;
                uint8_t     call_num[];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_call_waiting_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                        (char *)cmd->call_num, cmd->call_num_len, cmd->call_num_type);
        }
        break;

    case CMD_HFP_AG_MIC_GAIN_LEVEL_SET:
        {
            handle_cmd_with_gain(cmd_ptr, bt_hfp_ag_microphone_gain_set);
        }
        break;

    case CMD_HFP_AG_SPEAKER_GAIN_LEVEL_SET:
        {
            handle_cmd_with_gain(cmd_ptr, bt_hfp_ag_speaker_gain_set);
        }
        break;

    case CMD_HFP_AG_RING_INTERVAL_SET:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint16_t    ring_interval;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_ring_interval_set(app_db.br_link[cmd->app_link_id].bd_addr, cmd->ring_interval);
        }
        break;

    case CMD_HFP_AG_INBAND_RINGING_SET:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     enabled;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_inband_ringing_set(app_db.br_link[cmd->app_link_id].bd_addr, (cmd->enabled == 1));
        }
        break;

    case CMD_HFP_AG_CURRENT_CALLS_LIST_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     clcc_call_idx;
                uint8_t     clcc_call_dir;
                uint8_t     clcc_call_status;
                uint8_t     clcc_call_mode;
                uint8_t     clcc_call_mpty;
                uint8_t     call_type;
                uint8_t     call_num_len;
                uint8_t     call_num[];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_current_calls_list_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                              cmd->clcc_call_idx, cmd->clcc_call_dir,
                                              (T_BT_HFP_AG_CURRENT_CALL_STATUS) cmd->clcc_call_status,
                                              (T_BT_HFP_AG_CURRENT_CALL_MODE) cmd->clcc_call_mode,
                                              cmd->clcc_call_mpty,
                                              (char *)cmd->call_num, cmd->call_type);
        }
        break;

    case CMD_HFP_AG_SEVICE_STATUS_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     status;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_service_indicator_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                             (T_BT_HFP_AG_SERVICE_INDICATOR) cmd->status);
        }
        break;

    case CMD_HFP_AG_CALL_STATUS_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     status;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_call_indicator_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                          (T_BT_HFP_AG_CALL_INDICATOR) cmd->status);
        }
        break;

    case CMD_HFP_AG_CALL_SETUP_STATUS_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     status;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_call_setup_indicator_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                                (T_BT_HFP_AG_CALL_SETUP_INDICATOR) cmd->status);
        }
        break;

    case CMD_HFP_AG_CALL_HELD_STATUS_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     status;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_call_held_indicator_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                               (T_BT_HFP_AG_CALL_HELD_INDICATOR) cmd->status);
        }
        break;

    case CMD_HFP_AG_SIGNAL_STRENGTH_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     value;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_signal_strength_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                           cmd->value);
        }
        break;

    case CMD_HFP_AG_ROAMING_STATUS_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     indicator;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_roaming_indicator_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                             (T_BT_HFP_AG_ROAMING_INDICATOR)cmd->indicator);
        }
        break;

    case CMD_HFP_AG_BATTERY_CHANGE_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     value;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_battery_charge_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                          cmd->value);
        }
        break;

    case CMD_HFP_AG_OK_SEND:
        {
            handle_cmd_with_link_id(cmd_ptr, bt_hfp_ag_ok_send);
        }
        break;

    case CMD_HFP_AG_ERROR_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     cme_error_code;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_error_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                 cmd->cme_error_code);
        }
        break;

    case CMD_HFP_AG_NETWORK_OPERATOR_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     op_len;
                uint8_t     op[];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_network_operator_name_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                                 (char *)cmd->op, cmd->op_len);
            bt_hfp_ag_ok_send(app_db.br_link[cmd->app_link_id].bd_addr);
        }
        break;

    case CMD_HFP_AG_SUBSCRIBER_NUMBER_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     service;
                uint8_t     call_type;
                uint8_t     call_num_len;
                uint8_t     *call_num;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_subscriber_number_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                             (char *)cmd->call_num, cmd->call_num_len,
                                             cmd->call_type, cmd->service);
        }
        break;

    case CMD_HFP_AG_INDICATORS_STATUS_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     service_indicator;
                uint8_t     call_indicator;
                uint8_t     call_setup_indicator;
                uint8_t     call_held_indicator ;
                uint8_t     signal_indicator;
                uint8_t     roaming_indicator;
                uint8_t     batt_chg_indicator;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_indicators_send(app_db.br_link[cmd->app_link_id].bd_addr,
                                      (T_BT_HFP_AG_SERVICE_INDICATOR)cmd->service_indicator,
                                      (T_BT_HFP_AG_CALL_INDICATOR)cmd->call_indicator,
                                      (T_BT_HFP_AG_CALL_SETUP_INDICATOR)cmd->call_setup_indicator,
                                      (T_BT_HFP_AG_CALL_HELD_INDICATOR)cmd->call_held_indicator,
                                      cmd->signal_indicator,
                                      (T_BT_HFP_AG_ROAMING_INDICATOR)cmd->roaming_indicator,
                                      cmd->batt_chg_indicator);
            bt_hfp_ag_ok_send(app_db.br_link[cmd->app_link_id].bd_addr);
        }
        break;

    case CMD_HFP_AG_VENDOR_CMD_SEND:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint8_t     at_cmd[];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            bt_hfp_ag_send_vnd_at_cmd_req(app_db.br_link[cmd->app_link_id].bd_addr,
                                          (char *)cmd->at_cmd);
        }
        break;

    case CMD_HFP_AG_SCO_CONN_REQ:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     app_link_id;
                uint16_t    max_latency;
                uint16_t    voice_setting;
                uint8_t     retrans_effort;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            gap_br_send_sco_conn_req(app_db.br_link[cmd->app_link_id].bd_addr,
                                     8000, 8000, cmd->max_latency, cmd->voice_setting,
                                     cmd->retrans_effort,  SCO_PKT_TYPES_HV3_EV3_2EV3);
        }
        break;

    case CMD_HFP_AG_INDICATORS_STATUS_SET:
        {
            struct
            {
                uint16_t    cmd_id;
                bool        status_in_call;
                bool        status_incoming_call;
                bool        status_outgoing_call;
                bool        status_hold_call;
                bool        status_hold_mute;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            hfp_ag_in_call = cmd->status_in_call;
            hfp_ag_incoming_call = cmd->status_incoming_call;
            hfp_ag_outgoing_call = cmd->status_outgoing_call;
            hfp_ag_hold_call = cmd->status_hold_call;
            hfp_ag_hold_mute = cmd->status_hold_mute;
        }
        break;

    default:
        cmd_handled = false;
        break;
    }

    if (cmd_handled)
    {
        app_cmd_set_event_ack(cmd_path, app_idx, (uint8_t *)&ack_pkt);
    }
}


void app_hfp_ag_init(void)
{
    if (app_cfg_const.supported_profile_mask & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
    {
        bt_hfp_ag_init(RFC_HFP_AG_CHANN_NUM, RFC_HSP_AG_CHANN_NUM,
                       app_hfp_cfg.hfp_ag_brsf_capability,
                       BT_HFP_AG_CODEC_TYPE_CVSD | BT_HFP_AG_CODEC_TYPE_MSBC);
        bt_mgr_cback_register(app_hfp_ag_bt_cback);
#if F_APP_HID_TELEPHONY_SUPPORT
        app_tri_dongle_telephony_init();
#endif
    }
}

