/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_CCP_SUPPORT
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "app_audio_policy.h"
#include "app_lea_acc_ascs.h"
#include "app_lea_acc_ccp.h"
#include "app_main.h"
#if F_APP_LEA_CMD_SUPPORT
#include "app_lea_acc_def.h"

#endif

static uint16_t app_lea_ccp_active_voice_conn_handle = 0;
static T_APP_CALL_STATUS app_lea_ccp_call_status = APP_CALL_IDLE;

static T_LEA_CALL_ENTRY *app_lea_ccp_alloc_call_entry(uint8_t conn_id, uint8_t call_index,
                                                      uint16_t call_state)
{
    T_APP_LE_LINK *p_link;
    T_LEA_CALL_ENTRY *p_call_entry = NULL;

    p_link = app_link_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        uint8_t i;

        for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
        {
            if (p_link->lea_call_entry[i].used == false)
            {
                p_call_entry = &p_link->lea_call_entry[i];

                p_call_entry->used = true;
                p_call_entry->call_index = call_index;
                p_call_entry->call_state = call_state;
                break;
            }
        }
    }

    return p_call_entry;
}

static bool app_lea_ccp_free_call_entry(T_LEA_CALL_ENTRY *p_call_entry)
{
    if (p_call_entry != NULL)
    {
        if (p_call_entry->used == true)
        {
            if (p_call_entry->call_uri != NULL)
            {
                free(p_call_entry->call_uri);
                p_call_entry->call_uri = NULL;
            }
            memset(p_call_entry, 0, sizeof(T_LEA_CALL_ENTRY));
            return true;
        }
    }

    return false;
}

T_LEA_CALL_ENTRY *app_lea_ccp_find_call_entry_by_idx(uint8_t conn_id, uint8_t call_index)
{
    T_APP_LE_LINK *p_link;
    T_LEA_CALL_ENTRY *p_call_entry = NULL;

    p_link = app_link_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        uint8_t i;

        for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
        {
            if (p_link->lea_call_entry[i].used == true &&
                p_link->lea_call_entry[i].call_index == call_index)
            {
                p_call_entry = &p_link->lea_call_entry[i];
                break;
            }
        }
    }

    return p_call_entry;
}

static uint16_t app_lea_ccp_ble_audio_cback(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    if ((msg & 0xFF00) == LE_AUDIO_MSG_GROUP_CCP_CLIENT)
    {
        APP_PRINT_INFO1("app_lea_ccp_ble_audio_cback: msg %x", msg);
    }

    switch (msg)
    {
    case LE_AUDIO_MSG_CCP_CLIENT_DIS_DONE:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_CLIENT_DIS_DONE *p_dis_done = (T_CCP_CLIENT_DIS_DONE *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_dis_done->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_dis_done->is_found)
            {
                if (p_dis_done->gtbs)
                {
                    APP_PRINT_INFO1("find GTBS: conn_handle 0x%x", p_dis_done->conn_handle);
                    uint16_t cfg_flags = CCP_CLIENT_CFG_CCCD_FLAG_BEARER_LIST_CURRENT_CALLS |
                                         CCP_CLIENT_CFG_CCCD_FLAG_STATUS_FLAGS |
                                         CCP_CLIENT_CFG_CCCD_FLAG_INCOMING_CALL_TARGET_BEARER_URI | CCP_CLIENT_CFG_CCCD_FLAG_CALL_STATE |
                                         CCP_CLIENT_CFG_CCCD_FLAG_CALL_CONTROL_POINT | CCP_CLIENT_CFG_CCCD_FLAG_TERMINATION_REASON |
                                         CCP_CLIENT_CFG_CCCD_FLAG_INCOMING_CALL;
                    ccp_client_cfg_cccd(p_dis_done->conn_handle, 0, cfg_flags, true, p_dis_done->gtbs);
                    ccp_client_read_char_value(p_dis_done->conn_handle, 0, TBS_UUID_CHAR_CALL_STATE, p_dis_done->gtbs);
                    p_link->gtbs = p_dis_done->gtbs;
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_READ_RESULT:
        {
            T_APP_LE_LINK *p_link;
            T_CCP_CLIENT_READ_RESULT *p_read_result = (T_CCP_CLIENT_READ_RESULT *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_read_result->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            if (p_read_result->cause == GAP_SUCCESS)
            {
                if (p_read_result->char_uuid == TBS_UUID_CHAR_CALL_STATE)
                {
                    if (p_read_result->data.call_state.call_state_len >= CCP_CALL_STATE_CHARA_LEN)
                    {
                        uint8_t call_index;
                        uint8_t call_state;
                        T_LEA_CALL_ENTRY *p_call_entry;
                        uint8_t call_list_num = 0;

                        call_list_num = p_read_result->data.call_state.call_state_len / CCP_CALL_STATE_CHARA_LEN;
                        // each le audio link could have several calls
                        // allocate call entry and update call state, active call index
                        for (uint8_t i = 0; i < call_list_num; i++)
                        {
                            call_index = p_read_result->data.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN];
                            call_state = p_read_result->data.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN + 1];

                            p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, call_index);
                            if (p_call_entry != NULL)
                            {
                                if (p_call_entry->call_state != call_state)
                                {
                                    p_call_entry->call_state = call_state;
                                }
                            }
                            else
                            {
                                app_lea_ccp_alloc_call_entry(p_link->conn_id, call_index, call_state);
                            }
                        }
                        app_lea_ccp_update_link_call_status(p_link);
                        app_lea_ccp_update_call_status();
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CCP_CLIENT_NOTIFY:
        {
            T_APP_LE_LINK *p_link;
            bool need_return = false;
            T_CCP_CLIENT_NOTIFY *p_notify_data = (T_CCP_CLIENT_NOTIFY *)buf;

            p_link = app_link_find_le_link_by_conn_handle(p_notify_data->conn_handle);
            if (p_link == NULL)
            {
                return BLE_AUDIO_CB_RESULT_APP_ERR;
            }

            switch (p_notify_data->char_uuid)
            {
            case TBS_UUID_CHAR_CALL_STATE:
            case TBS_UUID_CHAR_BEARER_LIST_CURRENT_CALLS:
            case TBS_UUID_CHAR_INCOMING_CALL:
                {
                    T_APP_CALL_STATUS pre_call_status = p_link->call_status;

                    app_lea_ccp_parse_and_update_call_state(p_link, p_notify_data);
                    app_lea_ccp_update_link_call_status(p_link);
                    app_lea_ccp_update_call_status();
                    app_lea_ccp_trigger_call_ringtone(p_link, pre_call_status);
                }
                break;

            case TBS_UUID_CHAR_TERMINATION_REASON:
                {
                    T_LEA_CALL_ENTRY *p_call_entry;
                    T_APP_CALL_STATUS pre_call_status = p_link->call_status;
                    T_CCP_CLIENT_NOTIFY *p_notify_data = (T_CCP_CLIENT_NOTIFY *)buf;
                    uint8_t call_index = p_notify_data->data.termination_reason.call_index;

                    p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, call_index);
                    if (p_call_entry != NULL)
                    {
                        app_lea_ccp_free_call_entry(p_call_entry);
                    }
                    app_lea_ccp_update_link_call_status(p_link);
                    app_lea_ccp_update_call_status();
                    app_lea_ccp_trigger_call_ringtone(p_link, pre_call_status);
                }
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

uint16_t app_lea_ccp_get_active_conn_handle(void)
{
    return app_lea_ccp_active_voice_conn_handle;
}

bool app_lea_ccp_set_active_conn_handle(uint16_t conn_handle)
{
    bool ret = false;
    T_APP_LE_LINK *p_link;

    p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    if (p_link != NULL)
    {
        app_lea_ccp_active_voice_conn_handle = conn_handle;
        ret = true;
    }
    APP_PRINT_INFO2("app_lea_ccp_set_active_conn_handle: active_voice_conn_handle %x, ret %x",
                    app_lea_ccp_active_voice_conn_handle, ret);
    return ret;
}

T_APP_CALL_STATUS app_lea_ccp_get_call_status(void)
{
    return app_lea_ccp_call_status;
}

static bool app_lea_ccp_set_active_call_uri(T_APP_LE_LINK *p_link, uint8_t *p_uri)
{
    int8_t error_code;
    uint8_t *p_buf = NULL;
    uint16_t uri_len;

    if (p_uri == NULL)
    {
        error_code = 1;
        goto error;
    }

    uri_len = strlen((char *)p_uri);
    if (uri_len == 0)
    {
        error_code = 2;
        goto error;
    }

    if (p_link->active_call_uri != NULL)
    {
        if ((memcmp(p_link->active_call_uri, p_uri, uri_len) == 0) &&
            (strlen((char *)p_link->active_call_uri) == uri_len))
        {
            error_code = 3;
            goto error;
        }
        free(p_link->active_call_uri);
        p_link->active_call_uri = NULL;
    }

    p_buf = malloc(uri_len + 1);
    if (p_buf == NULL)
    {
        error_code = 4;
        goto error;
    }

    memcpy(p_buf, p_uri, uri_len);
    p_link->active_call_uri = p_buf;
    p_link->active_call_uri[uri_len] = '\0';
    APP_PRINT_TRACE1("app_lea_ccp_set_active_call_uri: active call_uri %s",
                     TRACE_STRING(p_link->active_call_uri));
    return true;
error:
    APP_PRINT_ERROR1("app_lea_ccp_set_active_call_uri: error %d", -error_code);
    return false;
}

#if F_APP_LEA_CMD_SUPPORT
static void app_lea_ccp_report_call_status(T_APP_CALL_STATUS call_status)
{
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].lea_link_state != LEA_LINK_IDLE)
        {
            struct
            {
                uint8_t conn_id;
                uint8_t call_status;
            } __attribute__((packed)) rpt = {};

            rpt.conn_id = app_db.le_link[i].id;
            rpt.call_status = call_status;

            app_report_event(CMD_PATH_UART, EVENT_CCP_CALL_STATUS, 0, (uint8_t *)&rpt,
                             sizeof(rpt));
        }
    }
}
#endif

void app_lea_ccp_update_link_call_status(T_APP_LE_LINK *p_link)
{
    uint8_t i;
    uint8_t active_call_entry_exist = true;
    T_LEA_CALL_ENTRY *p_active_call_entry = NULL;
    T_LEA_CALL_ENTRY *p_inactive_call_entry = NULL;
    uint8_t active_call_state = TBS_CALL_STATE_RFU;
    uint8_t inactive_call_state = TBS_CALL_STATE_RFU;

    // find active call entry
    for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
    {
        if (p_link->lea_call_entry[i].used == true &&
            p_link->lea_call_entry[i].call_index == p_link->active_call_index)
        {
            p_active_call_entry = &p_link->lea_call_entry[i];
            active_call_state = p_active_call_entry->call_state;
            break;
        }
    }

    if (p_active_call_entry == NULL)
    {
        active_call_entry_exist = false;
    }

    // find inactive call entry
    for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
    {
        if (p_link->lea_call_entry[i].used == true &&
            p_link->lea_call_entry[i].call_index != p_link->active_call_index)
        {
            p_inactive_call_entry = &p_link->lea_call_entry[i];
            inactive_call_state = p_inactive_call_entry->call_state;
            break;
        }
    }

    // check active call entry need to exchange or not
    if (active_call_entry_exist == false)
    {
        if (inactive_call_state != TBS_CALL_STATE_RFU)
        {
            active_call_entry_exist = true;
            p_active_call_entry = p_inactive_call_entry;

            active_call_state = p_active_call_entry->call_state;
            inactive_call_state = TBS_CALL_STATE_RFU;
            p_link->active_call_index = p_inactive_call_entry->call_index;
        }
    }
    else
    {
        bool exchange_call_entry = false;

        if (active_call_state < TBS_CALL_STATE_ACTIVE)
        {
            if ((inactive_call_state >= TBS_CALL_STATE_ACTIVE) &&
                (inactive_call_state <= TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD))
            {
                exchange_call_entry = true;
            }
        }
        else if ((active_call_state >= TBS_CALL_STATE_LOCALLY_HELD) &&
                 (active_call_state <= TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD))
        {
            if (inactive_call_state == TBS_CALL_STATE_ACTIVE)
            {
                exchange_call_entry = true;
            }
        }

        if (exchange_call_entry == true)
        {
            T_LEA_CALL_ENTRY *p_temp_call_entry = NULL;

            p_temp_call_entry = p_inactive_call_entry;
            p_inactive_call_entry = p_active_call_entry;
            p_active_call_entry = p_temp_call_entry;

            active_call_state = p_active_call_entry->call_state;
            inactive_call_state = p_inactive_call_entry->call_state;
            p_link->active_call_index = p_active_call_entry->call_index;
        }
    }

    if (active_call_entry_exist == true)
    {
        switch (active_call_state)
        {
        case TBS_CALL_STATE_INCOMING:
            if ((inactive_call_state < TBS_CALL_STATE_ACTIVE) ||
                (inactive_call_state == TBS_CALL_STATE_RFU))
            {
                p_link->call_status = APP_CALL_INCOMING;
            }
            break;

        case TBS_CALL_STATE_DIALING:
        case TBS_CALL_STATE_ALERTING:
            {
                if ((inactive_call_state < TBS_CALL_STATE_ACTIVE) ||
                    (inactive_call_state == TBS_CALL_STATE_RFU))
                {
                    p_link->call_status = APP_CALL_OUTGOING;
                }
            }
            break;

        case TBS_CALL_STATE_ACTIVE:
            {
                if (inactive_call_state == TBS_CALL_STATE_INCOMING)
                {
                    p_link->call_status = APP_CALL_ACTIVE_WITH_CALL_WAITING;
                }
                else if (inactive_call_state == TBS_CALL_STATE_ACTIVE)
                {
                    p_link->call_status = APP_CALL_ACTIVE;
                }
                else if ((inactive_call_state == TBS_CALL_STATE_LOCALLY_HELD) ||
                         (inactive_call_state == TBS_CALL_STATE_REMOTELY_HELD))
                {
                    p_link->call_status = APP_CALL_ACTIVE_WITH_CALL_HELD;
                }
                else if (inactive_call_state == TBS_CALL_STATE_RFU)
                {
                    p_link->call_status = APP_CALL_ACTIVE;
                }
            }
            break;

        case TBS_CALL_STATE_LOCALLY_HELD:
        case TBS_CALL_STATE_REMOTELY_HELD:
            {
                if ((inactive_call_state < TBS_CALL_STATE_ACTIVE) ||
                    (inactive_call_state == TBS_CALL_STATE_RFU))
                {
                    p_link->call_status = APP_CALL_ACTIVE;
                }
            }
            break;

        default:
            break;
        }
        APP_PRINT_INFO2("app_lea_ccp_update_link_call_status: active call_state %d, inactive call_state %d",
                        active_call_state, inactive_call_state);
    }
    else
    {
        p_link->call_status = APP_CALL_IDLE;
    }
}

void app_lea_ccp_update_call_status(void)
{
    uint8_t i;
    uint16_t conn_handle;
    T_APP_LE_LINK *p_active_link;
    T_APP_LE_LINK *p_inactive_link;
    uint8_t active_link_exist = true;
    T_APP_CALL_STATUS active_call_status = APP_CALL_IDLE;
    T_APP_CALL_STATUS inactive_call_status = APP_CALL_IDLE;
#if F_APP_LEA_CMD_SUPPORT
    T_APP_CALL_STATUS pre_call_status = app_lea_ccp_call_status;
#endif

    conn_handle = app_lea_ccp_get_active_conn_handle();
    // find active call entry
    p_active_link = app_link_find_le_link_by_conn_handle(conn_handle);
    if (p_active_link == NULL)
    {
        active_link_exist = false;
    }
    else
    {
        active_call_status = p_active_link->call_status;
    }

    // find inactive call entry
    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (active_link_exist)
        {
            if ((app_db.le_link[i].used == true) && ((p_active_link != NULL) &&
                                                     (app_db.le_link[i].conn_handle != p_active_link->conn_handle)))
            {
                p_inactive_link = &app_db.le_link[i];
                inactive_call_status = p_inactive_link->call_status;
                break;
            }
        }
        else
        {
            if (app_db.le_link[i].used == true)
            {
                p_inactive_link = &app_db.le_link[i];
                inactive_call_status = p_inactive_link->call_status;
                break;
            }
        }
    }

    // check active call entry need to exchange or not
    if (active_link_exist == false)
    {
        if (inactive_call_status != APP_CALL_IDLE)
        {
            active_link_exist = true;
            p_active_link = p_inactive_link;
            inactive_call_status = APP_CALL_IDLE;
            app_lea_ccp_set_active_conn_handle(p_active_link->conn_handle);
        }
    }
    else
    {
        if (active_call_status < APP_CALL_ACTIVE)
        {
            if ((inactive_call_status >= APP_CALL_ACTIVE) &&
                (inactive_call_status <= APP_CALL_ACTIVE_WITH_CALL_HELD))
            {
                T_APP_LE_LINK *p_temp_link = NULL;

                p_temp_link = p_inactive_link;
                p_inactive_link = p_active_link;
                p_active_link = p_temp_link;

                active_call_status = p_active_link->call_status;
                inactive_call_status = p_inactive_link->call_status;
                app_lea_ccp_set_active_conn_handle(p_active_link->conn_handle);
            }
        }
    }

    if (active_link_exist == true)
    {
        switch (active_call_status)
        {
        case APP_CALL_INCOMING:
            if (inactive_call_status < APP_CALL_ACTIVE)
            {
                app_lea_ccp_call_status = APP_CALL_INCOMING;
            }
            break;

        case APP_CALL_ACTIVE:
            if (inactive_call_status == APP_CALL_INCOMING)
            {
                app_lea_ccp_call_status = APP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT;
            }
            else if (inactive_call_status > APP_CALL_ACTIVE)
            {
                app_lea_ccp_call_status = APP_MULTILINK_CALL_ACTIVE_WITH_CALL_HELD;
            }
            else
            {
                app_lea_ccp_call_status = APP_CALL_ACTIVE;
            }
            break;

        default:
            app_lea_ccp_call_status = active_call_status;
            break;
        }
        APP_PRINT_INFO2("app_lea_ccp_update_call_status: active call_status %d, inactive call_status %d",
                        active_call_status, inactive_call_status);
    }
    else
    {
        app_lea_ccp_call_status = APP_CALL_IDLE;
    }

#if F_APP_LEA_CMD_SUPPORT
    if (pre_call_status != app_lea_ccp_call_status)
    {
        app_lea_ccp_report_call_status(app_lea_ccp_call_status);
    }
#endif
}

void app_lea_ccp_parse_and_update_call_state(T_APP_LE_LINK *p_link,
                                             T_CCP_CLIENT_NOTIFY *p_notify_data)
{
    uint8_t call_index;
    uint8_t call_state;
    T_LEA_CALL_ENTRY *p_call_entry;

    switch (p_notify_data->char_uuid)
    {
    case TBS_UUID_CHAR_CALL_STATE:
        {
            uint8_t call_list_num = 0;
            call_list_num = p_notify_data->data.call_state.call_state_len / CCP_CALL_STATE_CHARA_LEN;
            // each le audio link could have several calls
            // allocate call entry and update call state, active call index
            for (uint8_t i = 0; i < call_list_num; i++)
            {
                call_index = p_notify_data->data.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN];
                call_state = p_notify_data->data.call_state.p_call_state[i * CCP_CALL_STATE_CHARA_LEN + 1];

                p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, call_index);
                if (p_call_entry == NULL)
                {
                    p_call_entry = app_lea_ccp_alloc_call_entry(p_link->conn_id, call_index, call_state);
                }

                if (p_call_entry)
                {
                    if (p_call_entry->call_state != call_state)
                    {
                        p_call_entry->call_state = call_state;
                    }

                    if ((call_state == TBS_CALL_STATE_ACTIVE) ||
                        (call_state == TBS_CALL_STATE_DIALING) ||
                        (call_state == TBS_CALL_STATE_ALERTING))
                    {
                        app_lea_ccp_set_active_call_uri(p_link, p_call_entry->call_uri);
                    }
                }
            }
        }
        break;

    case TBS_UUID_CHAR_BEARER_LIST_CURRENT_CALLS:
        {
            uint8_t  *p_uri;
            uint16_t uri_len;
            uint8_t item_len = 0;

            for (uint8_t i = 0; i < p_notify_data->data.bearer_list_current_calls.bearer_list_current_calls_len;
                )
            {
                item_len = p_notify_data->data.bearer_list_current_calls.p_bearer_list_current_calls[i];
                call_index = p_notify_data->data.bearer_list_current_calls.p_bearer_list_current_calls[i + 1];
                call_state = p_notify_data->data.bearer_list_current_calls.p_bearer_list_current_calls[i + 2];
                p_uri = &p_notify_data->data.bearer_list_current_calls.p_bearer_list_current_calls[i + 4];
                uri_len = item_len - 3;
                i += item_len + 1;

                p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, call_index);
                if (p_call_entry == NULL)
                {
                    p_call_entry = app_lea_ccp_alloc_call_entry(p_link->conn_id, call_index, call_state);
                }

                if (p_call_entry)
                {
                    if (p_call_entry->call_uri != NULL)
                    {
                        free(p_call_entry->call_uri);
                        p_call_entry->call_uri = NULL;
                    }

                    p_call_entry->call_uri = malloc(uri_len + 1);
                    if (p_call_entry->call_uri != NULL)
                    {
                        memcpy(p_call_entry->call_uri, p_uri, uri_len);
                        p_call_entry->call_uri[uri_len] = '\0';
                    }

                    if (p_call_entry->call_state != call_state)
                    {
                        p_call_entry->call_state = call_state;
                    }

                    if ((call_state == TBS_CALL_STATE_ACTIVE) ||
                        (call_state == TBS_CALL_STATE_DIALING) ||
                        (call_state == TBS_CALL_STATE_ALERTING))
                    {
                        app_lea_ccp_set_active_call_uri(p_link, p_call_entry->call_uri);
                    }
                }
            }
        }
        break;

    case TBS_UUID_CHAR_INCOMING_CALL:
        {
            uint8_t  *p_buf;
            uint8_t  *p_uri;
            uint16_t uri_len;

            call_index = p_notify_data->data.incoming_call.call_index;
            p_uri = p_notify_data->data.incoming_call.p_uri;
            uri_len = p_notify_data->data.incoming_call.uri_len;
            if (uri_len == 0)
            {
                return;
            }

            p_buf = malloc(uri_len + 1);
            if (p_buf != NULL)
            {
                p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, call_index);
                if (p_call_entry == NULL)
                {
                    p_call_entry = app_lea_ccp_alloc_call_entry(p_link->conn_id, call_index, TBS_CALL_STATE_INCOMING);
                }

                if (p_call_entry)
                {
                    if (p_call_entry->call_uri != NULL)
                    {
                        free(p_call_entry->call_uri);
                        p_call_entry->call_uri = NULL;
                    }
                    p_call_entry->call_uri = p_buf;
                    memcpy(p_call_entry->call_uri, p_uri, uri_len);
                    p_call_entry->call_uri[uri_len] = '\0';

                    APP_PRINT_INFO2("app_le_audio_call_uri: call_index %d, call_uri %s", call_index,
                                    TRACE_STRING(p_call_entry->call_uri));
                }
                else
                {
                    free(p_buf);
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_lea_ccp_trigger_call_ringtone(T_APP_LE_LINK *p_link, T_APP_CALL_STATUS call_status)
{
    T_APP_CALL_STATUS prev_status = call_status;
    T_APP_CALL_STATUS curr_status = p_link->call_status;

    APP_PRINT_INFO2("app_lea_ccp_trigger_call_ringtone: prev_status %x, curr_status %x",
                    prev_status, curr_status);
    switch (curr_status)
    {
    case APP_CALL_IDLE:
        {
            if (prev_status == APP_CALL_ACTIVE)
            {
                app_audio_tone_type_play(TONE_HF_CALL_END, false, true);
            }
            else if (prev_status == APP_CALL_INCOMING)
            {
                if (app_db.reject_call_by_key)
                {
                    app_db.reject_call_by_key = false;
                    app_audio_tone_type_play(TONE_HF_CALL_REJECT, false, true);
                }
            }
        }
        break;

    case APP_CALL_ACTIVE:
        {
            if (prev_status != APP_CALL_ACTIVE)
            {
                app_audio_tone_type_play(TONE_HF_CALL_ACTIVE, false, true);
            }
        }
        break;

    case APP_CALL_OUTGOING:
        {
            if (prev_status != APP_CALL_OUTGOING)
            {
                app_audio_tone_type_play(TONE_HF_OUTGOING_CALL, false, true);
            }
        }
        break;

    default:
        break;
    }

}

void app_lea_ccp_init(void)
{
    ble_audio_cback_register(app_lea_ccp_ble_audio_cback);
}
#endif
