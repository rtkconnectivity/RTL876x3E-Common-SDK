/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "gap.h"
#include "gap_br.h"
#include "app_gap.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "remote.h"
#include "btm.h"
#include "ble_mgr.h"
#include "app_customer_bt.h"
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_mgr.h"
#endif

static void app_gap_common_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_CB_DATA));
    APP_PRINT_INFO1("app_gap_common_callback: cb_type = %d", cb_type);

    ble_mgr_handle_gap_common_cb(cb_type, p_cb_data);

    switch (cb_type)
    {
    case GAP_MSG_WRITE_AIRPLAN_MODE:
        APP_PRINT_INFO1("app_gap_common_callback: GAP_MSG_WRITE_AIRPLAN_MODE cause 0x%04x",
                        cb_data.p_gap_write_airplan_mode_rsp->cause);
        break;
    case GAP_MSG_READ_AIRPLAN_MODE:
        APP_PRINT_INFO2("app_gap_common_callback: GAP_MSG_READ_AIRPLAN_MODE cause 0x%04x, mode %d",
                        cb_data.p_gap_read_airplan_mode_rsp->cause,
                        cb_data.p_gap_read_airplan_mode_rsp->mode);
        break;
    case GAP_MSG_VENDOR_CMD_CMPL_EVENT:
        {
        }
        break;

    case GAP_MSG_SET_LOCAL_BD_ADDR:
        {
            APP_PRINT_INFO1("app_gap_common_callback: GAP_MSG_SET_LOCAL_BD_ADDR: cause 0x%04x",
                            cb_data.p_gap_set_bd_addr_rsp->cause);
        }
        break;

    case GAP_MSG_VENDOR_CMD_RSP:
        {
            APP_PRINT_INFO4("app_ble_gap_common_callback: GAP_MSG_VENDOR_CMD_RSP command 0x%x, cause 0x%x, is_cmpl_evt %d, param_len %d",
                            cb_data.p_gap_vendor_cmd_rsp->command,
                            cb_data.p_gap_vendor_cmd_rsp->cause,
                            cb_data.p_gap_vendor_cmd_rsp->is_cmpl_evt,
                            cb_data.p_gap_vendor_cmd_rsp->param_len);
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            if (app_tri_dongle_if_plugin_prog(REDEVELOP_MSG_PLUGIN_BLE_VENDOR_CMD_RSP,
                                              &cb_data, NULL) == REDEVELOP_RESULT_OFF_HOOK_PROG)
            {
                return;
            }

            if (cb_data.p_gap_vendor_cmd_rsp->param_len >= 1)
            {
                APP_PRINT_INFO2("app_ble_gap_common_callback: ble link_nume %d, %b",
                                app_link_get_le_link_num(),
                                TRACE_BINARY(cb_data.p_gap_vendor_cmd_rsp->param_len, cb_data.p_gap_vendor_cmd_rsp->param));
                uint8_t vendor_subcode = cb_data.p_gap_vendor_cmd_rsp->param[0];
                if (cb_data.p_gap_vendor_cmd_rsp->param_len >= 3)
                {
                    if ((cb_data.p_gap_vendor_cmd_rsp->command == HCI_EXT_SUB_PROPRIETARY_CTRL) &&
                        (vendor_subcode == HCI_EXT_SUB_CONN_UPDT_PROPRIETARY_SUBCODE))
                    {
                        uint8_t *p = &cb_data.p_gap_vendor_cmd_rsp->param[1];
                        uint16_t conn_handle = 0xFFFF;
                        LE_STREAM_TO_UINT16(conn_handle, p);
                        if (conn_handle != 0xFFFF)
                        {
                            T_APP_LE_LINK *p_link;
                            p_link = app_link_find_le_link_by_conn_handle(conn_handle);

                            if (p_link != NULL)
                            {
#if F_APP_UWB_SCENARIO_SUPPORT
                                app_tri_dongle_uwb_client_notify_result(conn_handle, cb_data);
#endif
                                if (cb_data.p_gap_vendor_cmd_rsp->cause)
                                {
                                    app_tri_dongle_retry_2_5ms_update(p_link->conn_id);
                                }
                                else
                                {
#if F_APP_UWB_SCENARIO_SUPPORT
                                    app_tri_dongle_set_uwb_cmd_ci_procedure(p_link->conn_handle,
                                                                            APP_TRI_DONGLE_UWB_UPDTAE_CMPL,
                                                                            NULL);
#endif
                                }
                            }
                        }
                    }
#if F_APP_1_EP_HID_MULTI_DEV_CDC_SUPPORT
                    else if ((cb_data.p_gap_vendor_cmd_rsp->command == HCI_EXT_SUB_PROPRIETARY_CTRL) &&
                             (vendor_subcode == HCI_EXT_SUB_CONN_UPDT_YYLX_SET_OFFSET))
                    {
                        uint8_t *p = &cb_data.p_gap_vendor_cmd_rsp->param[1];
                        uint16_t conn_handle = 0xFFFF;
                        LE_STREAM_TO_UINT16(conn_handle, p);
                        if (cb_data.p_gap_vendor_cmd_rsp->cause)
                        {
                            APP_PRINT_INFO0("app_ble_gap_common_callback fail");
                        }
                        else
                        {
                            app_tri_dongle_offset_ci_update(APP_TRI_DONGLE_UPDATE_OFFSET_RSP,
                                                            conn_handle,
                                                            NULL);
                        }

                        app_tri_dongle_offset_ci_update(APP_TRI_DONGLE_UPDATE_OFFSET_CHECK,
                                                        NULL,
                                                        NULL);
                    }
#endif
                }
            }
#endif
        }
        break;

    case GAP_MSG_VENDOR_EVT_INFO:
        {
            APP_PRINT_INFO2("app_ble_gap_common_callback: GAP_MSG_VENDOR_EVT_INFO param_len %d, %b",
                            cb_data.p_gap_vendor_evt_info->param_len,
                            TRACE_BINARY(cb_data.p_gap_vendor_evt_info->param_len, cb_data.p_gap_vendor_evt_info->param));
#if F_APP_1_EP_HID_MULTI_DEV_CDC_SUPPORT
            uint16_t conn_handle = (cb_data.p_gap_vendor_evt_info->param[1] |
                                    (cb_data.p_gap_vendor_evt_info->param[2] << 8));
            uint16_t offset = (cb_data.p_gap_vendor_evt_info->param[3] |
                               (cb_data.p_gap_vendor_evt_info->param[4] << 8));

            app_tri_dongle_offset_ci_update(APP_TRI_DONGLE_OFFSET_DEFAULT_SETTING,
                                            conn_handle,
                                            offset);
#endif
        }
        break;

    default:
        break;
    }
    return;
}

static void app_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            static const uint8_t null_addr[6] = {0};

            memcpy(app_db.factory_addr, param->ready.bd_addr, 6);

            if (!memcmp(app_cfg_nv.bud_local_addr, null_addr, 6))
            {
                memcpy(app_cfg_nv.bud_local_addr, app_db.factory_addr, 6);
                remote_local_addr_set(app_cfg_nv.bud_local_addr);
            }

            gap_set_bd_addr(app_cfg_nv.bud_local_addr);
        }
        break;

    case BT_EVENT_LINK_USER_CONFIRMATION_REQ:
        {
            uint8_t io_cap;

            gap_get_param(GAP_PARAM_BOND_IO_CAPABILITIES, &io_cap);

            if ((io_cap == GAP_IO_CAP_DISPLAY_ONLY ||
                 io_cap == GAP_IO_CAP_KEYBOARD_ONLY ||
                 io_cap == GAP_IO_CAP_NO_INPUT_NO_OUTPUT) &&
                param->link_user_confirmation_req.just_works == true)
            {
                gap_br_user_cfm_req_cfm(param->link_user_confirmation_req.bd_addr, GAP_CFM_CAUSE_ACCEPT);
            }
            else
            {
                uint8_t temp_buff[10];
                T_APP_BR_LINK *p_link = NULL;

                p_link = app_link_find_br_link(param->link_user_confirmation_req.bd_addr);

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
                gap_br_user_cfm_req_cfm(param->link_user_confirmation_req.bd_addr, GAP_CFM_CAUSE_ACCEPT);
#else
                app_customer_bt_user_cfm_rpt(param->link_user_confirmation_req.bd_addr,
                                             param->link_user_confirmation_req.just_works,
                                             param->link_user_confirmation_req.display_value);
#endif

                temp_buff[0] = (uint8_t)param->link_user_confirmation_req.display_value;
                temp_buff[1] = (uint8_t)(param->link_user_confirmation_req.display_value >> 8);
                temp_buff[2] = (uint8_t)(param->link_user_confirmation_req.display_value >> 16);
                temp_buff[3] = (uint8_t)(param->link_user_confirmation_req.display_value >> 24);
                memcpy(&temp_buff[4], &param->link_user_confirmation_req.bd_addr, 6);

                if (p_link)
                {
                    app_report_event(CMD_PATH_UART, EVENT_REPORT_SSP_NUMERIC_VALUE, p_link->id, temp_buff,
                                     sizeof(temp_buff));
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_gap_init(void)
{
    gap_register_app_cb(app_gap_common_callback);

    bt_mgr_cback_register(app_gap_bt_cback);
}
