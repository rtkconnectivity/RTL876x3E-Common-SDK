/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "string.h"
#include "stdlib.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_adv_stop_cause.h"
#include "app_ble_gap.h"
#include "ble_conn.h"
#include "ble_scan.h"
#include "ble_ext_adv.h"
#include "gap_bond_le.h"
#include "gatt_builtin_services.h"
#include "bt_bond_le.h"
#include "gap_conn_le.h"
#include "profile_client.h"
#include "app_ble_rand_addr_mgr.h"
#include "app_cmd.h"
#include "app_cmd_ble.h"
#include "app_rpt_ble.h"
#include "app_main.h"
#include "app_transfer.h"
#include "gap_privacy.h"

static BLE_SCAN_HDL app_ble_scan_handle = NULL;

typedef struct ADV_Q_ELEM  ADV_Q_ELEM;

typedef struct ADV_Q_ELEM
{
    ADV_Q_ELEM *p_next;
    uint8_t handle;                 //
    T_BLE_EXT_ADV_MGR_STATE state; //used in adv_start/stop
    uint8_t adv_data[31]; //ble_ext_adv_mgr_init_adv_params does not copy adv_data, we should preserve one.
    uint32_t adv_data_len;
    uint8_t scan_rsp[31]; //ble_ext_adv_mgr_init_adv_params does not copy adv_data, we should preserve one.
    uint32_t scan_rsp_len;
} ADV_Q_ELEM;


typedef struct
{
    uint8_t adv_handle;
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_prop;
    uint32_t interval_min;
    uint32_t interval_max;
    T_GAP_LOCAL_ADDR_TYPE own_addr_type;
    uint8_t own_addr[6];
    T_GAP_REMOTE_ADDR_TYPE peer_addr_type;
    uint8_t peer_addr[6];
    uint8_t *adv_data;
    uint16_t adv_data_len;
    uint8_t *scan_rsp;
    uint16_t scan_rsp_len;
} ADV_PARAMS;

typedef struct
{
    uint16_t    cmd_id;
    uint8_t     adv_handle;
    uint8_t     adv_prop;
    uint32_t    interval_min;
    uint32_t    interval_max;
    uint8_t     own_addr_type;
    uint8_t     own_addr[6];
    uint8_t     peer_addr_type;
    uint8_t     peer_addr[6];
    uint16_t    adv_data_len;
    uint8_t     adv_data[];
} __attribute__((packed)) ADV_PARAMS_CMD;


typedef struct
{
    uint16_t    len;
    uint8_t     data[];
} __attribute__((packed)) SCAN_RSP;



static struct
{
    struct
    {
        T_OS_QUEUE q;
    } advs;

} ble =
{
    .advs = {.q = {}}
};


static ADV_Q_ELEM *find_adv_elem_by_hdl(uint8_t handle)
{
    ADV_Q_ELEM *tmp = (ADV_Q_ELEM *)ble.advs.q.p_first;
    for (; tmp != NULL; tmp = tmp->p_next)
    {
        if (tmp->handle == handle)
        {
            return tmp;
        }
    }

    return NULL;
}


static void adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));
    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            struct
            {
                uint8_t handle;
                uint8_t state;
                uint8_t stop_cause;
            } __attribute__((packed)) rpt = {};

            ADV_Q_ELEM *p_adv_elem = find_adv_elem_by_hdl(cb_data.p_ble_conn_info->adv_handle);
            rpt.handle = cb_data.p_ble_conn_info->adv_handle;

            if (p_adv_elem)
            {
                p_adv_elem->state = cb_data.p_ble_state_change->state;
                rpt.state = p_adv_elem->state;
                if (p_adv_elem->state == BLE_EXT_ADV_MGR_ADV_ENABLED)
                {
                    APP_PRINT_TRACE1("adv_callback: BLE_EXT_ADV_MGR_ADV_ENABLED, adv_handle %d",
                                     cb_data.p_ble_state_change->adv_handle);
                    rpt.stop_cause = 0;
                }
                else if (p_adv_elem->state == BLE_EXT_ADV_MGR_ADV_DISABLED)
                {
                    APP_PRINT_TRACE1("adv_callback: BLE_EXT_ADV_MGR_ADV_DISABLED, adv_handle %d",
                                     cb_data.p_ble_state_change->adv_handle);
                    switch (cb_data.p_ble_state_change->stop_cause)
                    {
                    case BLE_EXT_ADV_STOP_CAUSE_APP: // customer define cause
                        APP_PRINT_TRACE1("adv_callback: BLE_EXT_ADV_STOP_CAUSE_APP app_cause 0x%02x",
                                         cb_data.p_ble_state_change->app_cause);
                        break;

                    case BLE_EXT_ADV_STOP_CAUSE_CONN:
                        APP_PRINT_TRACE0("adv_callback: BLE_EXT_ADV_STOP_CAUSE_CONN");
                        break;

                    case BLE_EXT_ADV_STOP_CAUSE_TIMEOUT:
                        APP_PRINT_TRACE0("adv_callback: BLE_EXT_ADV_STOP_CAUSE_TIMEOUT");
                        break;

                    default:
                        APP_PRINT_TRACE1("adv_callback: stop_cause %d",
                                         cb_data.p_ble_state_change->stop_cause);
                        break;
                    }
                    rpt.stop_cause = cb_data.p_ble_state_change->stop_cause;
                }
                app_report_event(CMD_PATH_UART, EVENT_XM_LE_ADV_STATE, 0, (uint8_t *)&rpt, sizeof(rpt));
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        {
            ADV_Q_ELEM *p_adv_elem = find_adv_elem_by_hdl(cb_data.p_ble_conn_info->adv_handle);

            if (p_adv_elem)
            {
                APP_PRINT_TRACE1("adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x",
                                 cb_data.p_ble_conn_info->conn_id);
                p_adv_elem->state = ble_ext_adv_mgr_get_adv_state(p_adv_elem->handle);
            }
        }
        break;

    default:
        break;
    }
    return;
}

static bool adv_start(uint8_t handle, uint16_t duration_10ms)
{
    ADV_Q_ELEM *p_adv_elem = find_adv_elem_by_hdl(handle);

    if (p_adv_elem)
    {
        if (p_adv_elem->state == BLE_EXT_ADV_MGR_ADV_DISABLED)
        {
            if (ble_ext_adv_mgr_enable(handle, duration_10ms) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            APP_PRINT_TRACE0("customer_adv_start: Already started");
            return true;
        }
    }
    return false;
}


static bool adv_stop(uint8_t handle, uint8_t app_cause)
{
    if (ble_ext_adv_mgr_disable(handle, app_cause) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}


static bool adv_data_update(uint8_t handle, uint16_t adv_data_len, uint8_t *p_adv_data)
{
    ADV_Q_ELEM *p_adv_elem = find_adv_elem_by_hdl(handle);

    if (p_adv_data && p_adv_elem)
    {
        memcpy(p_adv_elem->adv_data, p_adv_data, adv_data_len);
        p_adv_elem->adv_data_len = adv_data_len;
    }
    else
    {
        return false;
    }

    if (ble_ext_adv_mgr_set_adv_data(p_adv_elem->handle, p_adv_elem->adv_data_len,
                                     p_adv_elem->adv_data) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}


static bool scan_rsp_data_update(uint8_t handle, uint16_t scan_rsp_data_len,
                                 uint8_t *p_scan_rsp_data)
{
    ADV_Q_ELEM *p_adv_elem = find_adv_elem_by_hdl(handle);

    if (p_scan_rsp_data && p_adv_elem)
    {
        memcpy(p_adv_elem->scan_rsp, p_scan_rsp_data, scan_rsp_data_len);
        p_adv_elem->scan_rsp_len = scan_rsp_data_len;
    }
    else
    {
        return false;
    }

    if (ble_ext_adv_mgr_set_scan_response_data(p_adv_elem->handle, p_adv_elem->scan_rsp_len,
                                               p_adv_elem->scan_rsp) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}


static bool change_adv_interval(uint8_t handle, uint16_t adv_interval)
{
    if (ble_ext_adv_mgr_change_adv_interval(handle, adv_interval) == GAP_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}


static uint8_t adv_create(ADV_PARAMS *p_params)
{
    ADV_Q_ELEM *p_elem = calloc(1, sizeof(*p_elem));

    if (p_params->adv_data)
    {
        memcpy(p_elem->adv_data, p_params->adv_data, p_params->adv_data_len);
        p_elem->adv_data_len = p_params->adv_data_len;
    }

    if (p_params->scan_rsp)
    {
        memcpy(p_elem->scan_rsp, p_params->scan_rsp, p_params->scan_rsp_len);
        p_elem->scan_rsp_len = p_params->scan_rsp_len;
    }

    p_elem->state = BLE_EXT_ADV_MGR_ADV_DISABLED;

    ble_ext_adv_mgr_init_adv_params(&p_elem->handle, p_params->adv_prop, p_params->interval_min,
                                    p_params->interval_max, p_params->own_addr_type, p_params->peer_addr_type, p_params->peer_addr,
                                    GAP_ADV_FILTER_ANY, p_elem->adv_data_len, p_elem->adv_data,
                                    p_elem->scan_rsp_len, p_elem->scan_rsp, p_params->own_addr);

    APP_PRINT_TRACE2("app_customer_ble adv_create: adv_data %b, le_single_random_addr %s",
                     TRACE_BINARY(sizeof(p_elem->adv_data), p_elem->adv_data),
                     TRACE_BDADDR(app_cfg_nv.le_single_random_addr));

    /* set adv event handle callback*/
    ble_ext_adv_mgr_register_callback(adv_callback, p_elem->handle);

    os_queue_in(&ble.advs.q, p_elem);

    return p_elem->handle;
}


static void adv_param_update(ADV_PARAMS *p_params)
{
    T_GAP_CAUSE ble_ext_adv_mgr_set_adv_param(uint8_t adv_handle,
                                              uint16_t adv_event_prop,
                                              uint32_t primary_adv_interval_min, uint32_t primary_adv_interval_max,
                                              uint8_t primary_adv_channel_map,
                                              T_GAP_LOCAL_ADDR_TYPE own_address_type,
                                              T_GAP_REMOTE_ADDR_TYPE peer_address_type, uint8_t *p_peer_address,
                                              T_GAP_ADV_FILTER_POLICY filter_policy, int8_t tx_power,
                                              T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy, uint8_t secondary_adv_max_skip,
                                              T_GAP_PHYS_TYPE secondary_adv_phy, uint8_t adv_sid,
                                              bool scan_req_notification_enable);

    ble_ext_adv_mgr_set_adv_param(p_params->adv_handle,
                                  p_params->adv_prop, p_params->interval_min, p_params->interval_max,
                                  GAP_ADVCHAN_ALL, p_params->own_addr_type, p_params->peer_addr_type,
                                  p_params->peer_addr, GAP_ADV_FILTER_ANY, 127, GAP_PHYS_PRIM_ADV_1M, 0,
                                  GAP_PHYS_1M, 0, false);

    adv_data_update(p_params->adv_handle, p_params->adv_data_len, p_params->adv_data);
    scan_rsp_data_update(p_params->adv_handle, p_params->scan_rsp_len, p_params->scan_rsp);
}

static void app_lea_scan_parse_name(uint8_t report_data_len, uint8_t *p_report_data,
                                    uint8_t *name_len, uint8_t *name)
{
    uint8_t *p_buffer;
    uint16_t pos = 0;

    while (pos < report_data_len)
    {
        uint16_t length = p_report_data[pos++];
        uint8_t type;

        if (length < 1)
        {
            return;
        }
        if ((length > 0x01) && ((pos + length) <= report_data_len))
        {
            /* Copy the AD Data to buffer. */
            p_buffer = p_report_data + pos + 1;
            /* AD Type, one octet. */
            type = p_report_data[pos];

            switch (type)
            {
            case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                {
                    *name_len = length - 1;

                    if (*name_len >= GAP_DEVICE_NAME_LEN)
                    {
                        *name_len = GAP_DEVICE_NAME_LEN - 1;
                    }
                    memcpy(name, p_buffer, *name_len);
                }
                break;

            default:
                break;
            }
        }
        pos += length;
    }
}

#define REPORT_DETAIL 0 // for external MCU
#define REPORT_SIMPLE 1 // for ACI Host Tool
static void app_ble_scan_report(T_LE_EXT_ADV_REPORT_INFO *p_report)
{
#if REPORT_DETAIL
    app_rpt_ble_scan_info(p_report);
#endif

#if REPORT_SIMPLE
    struct
    {
        uint16_t    event_type;
        uint8_t     addr_type;
        uint8_t     bd_addr[6];
        uint8_t     rssi;
        uint8_t     name_len;
        uint8_t     name[GAP_DEVICE_NAME_LEN];
    } __attribute__((packed)) rpt = {};

    rpt.event_type = p_report->event_type;
    rpt.addr_type = p_report->addr_type;
    memcpy(rpt.bd_addr, p_report->bd_addr, 6);
    rpt.rssi = p_report->rssi;
    app_lea_scan_parse_name(p_report->data_len, p_report->p_data, &rpt.name_len, rpt.name);
    app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_SCAN_INFO, 0, (uint8_t *)&rpt, sizeof(rpt));
#endif
}

static void app_ble_scan_cb(BLE_SCAN_EVT state, BLE_SCAN_EVT_DATA *data)
{
    switch (state)
    {
    case BLE_SCAN_REPORT:
        app_ble_scan_report(data->report);
        break;

    default:
        break;
    }
}

void app_cmd_ble_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                        uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE1("app_cmd_ble_handle: cmd_id 0x%04x", cmd_id);

    typedef struct
    {
        uint16_t cmd_id;
        uint8_t  status;
    } __attribute__((packed)) *ACK_PKT;

    switch (cmd_id)
    {
    case CMD_LE_DATA_TRANSFER:
        {
            app_transfer_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_START_ADVERTISING:
    case CMD_XM_LE_START_ADVERTISING:
        {

            struct
            {
                uint16_t cmd_id;
                uint8_t   handle;
                uint16_t duration;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;


            APP_PRINT_TRACE2("app_customer_ble_handle_cmd: handle 0x%02x, duration %x", cmd->handle,
                             cmd->duration);

            if (app_db.device_state != APP_DEVICE_STATE_OFF)
            {
                if (adv_start(cmd->handle, cmd->duration) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }

        break;

    case CMD_LE_STOP_ADVERTISING:
    case CMD_XM_LE_STOP_ADVERTISING:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t   handle;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            if (adv_stop(cmd->handle, APP_STOP_ADV_CAUSE_CONSOLE_UART) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

        }
        break;

    case CMD_LE_START_SCAN:
        {
            typedef struct
            {
                uint8_t scan_type;      /**< Scan type. Write only. Default is GAP_SCAN_MODE_ACTIVE, @ref T_GAP_SCAN_MODE. */
                uint16_t scan_interval;  /**< Scan Interval. Write only. In units of 0.625ms, range: 0x0004 to 0xFFFF. Default is 0x40. */
                uint16_t scan_window;    /**< Scan Window. Write only. In units of 0.625ms, range: 0x0004 to 0xFFFF. Default is 0x20. */
            } __attribute__((packed)) PHY_PARAMS;

            struct
            {
                uint16_t cmd_id;
                uint8_t local_addr_type;
                uint8_t phys;
                uint8_t filter_policy;
                uint8_t filter_duplicates;
                PHY_PARAMS phy_param_1m;
                PHY_PARAMS phy_param_coded;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            BLE_SCAN_PARAM param;

            param.own_addr_type = (T_GAP_LOCAL_ADDR_TYPE)cmd->local_addr_type;
            param.phys = cmd->phys;
            param.ext_filter_policy = (T_GAP_SCAN_FILTER_POLICY)cmd->filter_policy;
            param.ext_filter_duplicate = (T_GAP_SCAN_FILTER_DUPLICATE)cmd->filter_duplicates;

            if (cmd->phys & GAP_EXT_SCAN_PHYS_1M_BIT)
            {
                param.scan_param_1m.scan_type = (T_GAP_SCAN_MODE)cmd->phy_param_1m.scan_type;
                param.scan_param_1m.scan_interval = cmd->phy_param_1m.scan_interval;
                param.scan_param_1m.scan_window = cmd->phy_param_1m.scan_window;
            }
            if (cmd->phys & GAP_EXT_SCAN_PHYS_CODED_BIT)
            {

                param.scan_param_coded.scan_type = (T_GAP_SCAN_MODE)cmd->phy_param_coded.scan_type;
                param.scan_param_coded.scan_interval = cmd->phy_param_coded.scan_interval;
                param.scan_param_coded.scan_window = cmd->phy_param_coded.scan_window;
            }

            if (!ble_scan_start(&app_ble_scan_handle, app_ble_scan_cb, &param, NULL))
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_STOP_SCAN:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            ble_scan_stop(&app_ble_scan_handle);
        }
        break;

    case CMD_LE_GET_ADDR:
        {
            uint8_t rand_addr[6] = {0};
            app_ble_rand_addr_get(rand_addr);
            if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_IAP) ||
                (cmd_path == CMD_PATH_GATT_OVER_BREDR))
            {
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                app_report_event(cmd_path, EVENT_LE_PUBLIC_ADDR, app_idx, rand_addr, 6);
            }
        }
        break;

    case CMD_LE_SET_DATA_LEN:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t app_link_id;
                uint16_t tx_octets;
                uint16_t tx_time;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            le_set_data_len(app_db.le_link[cmd->app_link_id].conn_id, cmd->tx_octets, cmd->tx_time);
            APP_PRINT_TRACE3("app_cmd_ble_handle: app_link_id %d, tx_octets %d, tx_time %d",
                             cmd->app_link_id, cmd->tx_octets, cmd->tx_time);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_SET_PHY:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t app_link_id;
                uint8_t all_phys;
                uint8_t tx_phys;
                uint8_t rx_phys;
                uint8_t phy_options;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            le_set_phy(app_db.le_link[cmd->app_link_id].conn_id, cmd->all_phys, cmd->tx_phys,
                       cmd->rx_phys, (T_GAP_PHYS_OPTIONS)cmd->phy_options);
            APP_PRINT_TRACE4("app_cmd_ble_handle: app_link_id %d, all_phys %d, tx_phys %d, rx_phys %d, phy_options",
                             cmd->app_link_id, cmd->all_phys, cmd->tx_phys, cmd->rx_phys);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_GET_REMOTE_FEATURES:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            struct
            {
                uint16_t cmd_id;
                uint8_t app_link_id;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            uint8_t conn_id = app_db.le_link[cmd->app_link_id].conn_id;

            uint8_t features[GAP_LE_SUPPORTED_FEATURES_LEN] = {0};
            le_get_conn_param(GAP_PARAM_CONN_REMOTE_FEATURES, features, conn_id);

            struct
            {
                uint8_t app_link_id;
                uint8_t features[GAP_LE_SUPPORTED_FEATURES_LEN];
            } __attribute__((packed)) rpt = {};

            rpt.app_link_id = cmd->app_link_id;
            memcpy(&rpt.features, features, GAP_LE_SUPPORTED_FEATURES_LEN);
            app_report_event(CMD_PATH_UART, EVENT_LE_REMOTE_FEATURES, 0, (uint8_t *)&rpt, sizeof(rpt));
            APP_PRINT_TRACE2("app_cmd_ble_handle: app_link_id %d, features %b", rpt.app_link_id,
                             TRACE_BINARY(GAP_LE_SUPPORTED_FEATURES_LEN, features));
        }
        break;

    case CMD_LE_START_PAIR:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t  app_link_id;
            }
            __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            le_bond_pair(app_db.le_link[cmd->app_link_id].conn_id);
            APP_PRINT_TRACE1("app_cmd_ble_handle CMD_LE_START_PAIR: app_link_id %d", cmd->app_link_id);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_GET_ALL_BONDED_DEV:
        {
            typedef struct
            {
                uint8_t remote_identity_addr_type;
                uint8_t addr[6];
            }   __attribute__((packed)) BOND_INFO;

            struct
            {
                uint8_t bond_num;
                BOND_INFO bond_infos[];
            }   __attribute__((packed)) *rpt = NULL;

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            uint8_t bond_num = bt_le_get_bond_dev_num();
            uint32_t rpt_len = sizeof(*rpt) + sizeof(rpt->bond_infos[0]) * bond_num;
            rpt = calloc(1, rpt_len);
            rpt->bond_num = bond_num;
            for (uint8_t i = 0; i < bond_num; i++)
            {
                T_LE_BOND_ENTRY *p_entry = bt_le_find_key_entry_by_priority(i + 1);
                rpt->bond_infos[i].remote_identity_addr_type = p_entry->remote_identity_addr_type;
                memcpy(&rpt->bond_infos[i].addr, p_entry->remote_identity_addr, 6);
                APP_PRINT_TRACE4("app_cmd_ble_handle: prio %d, addr_type %d, addr %s, flags %d",
                                 i, rpt->bond_infos[i].remote_identity_addr_type, TRACE_BDADDR(rpt->bond_infos[i].addr),
                                 p_entry->flags);
            }

            app_report_event(CMD_PATH_UART, EVENT_LE_BONDED_DEV, 0, (uint8_t *)rpt, rpt_len);
            APP_PRINT_TRACE2("app_cmd_ble_handle: bond_num %d, bond_infos %b ", rpt->bond_num,
                             TRACE_BINARY(sizeof(rpt->bond_infos[0]) * bond_num, rpt->bond_infos));

            free(rpt);
        }
        break;

    case CMD_LE_GATT_SERV_CHG:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            struct
            {
                uint16_t cmd_id;
                uint16_t conn_handle;
                uint16_t cid;
                uint16_t start_handle;
                uint16_t end_handle;
            }   __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            APP_PRINT_TRACE4("app_cmd_ble_handle: conn_handle 0x%04x, cid 0x%04x, start_handle 0x%04x, end_handle 0x%04x",
                             cmd->conn_handle, cmd->cid, cmd->start_handle, cmd->end_handle);
            gatts_ext_service_changed_indicate(cmd->conn_handle, cmd->cid, cmd->start_handle, cmd->end_handle);
        }
        break;

    case CMD_LE_IGNORE_SLAVE_LATENCY:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            struct
            {
                uint16_t cmd_id;
                uint8_t app_link_id;
                uint8_t  ignore_latency;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            uint8_t conn_id = app_db.le_link[cmd->app_link_id].conn_id;
            APP_PRINT_TRACE2("app_cmd_ble_handle: app_link_id %d, need_ignore_latency %d", cmd->app_link_id,
                             cmd->ignore_latency);
            le_disable_slave_latency(conn_id, cmd->ignore_latency);
        }
        break;

    case CMD_LE_SET_RAND_ADDR:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t addr[6];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            APP_PRINT_TRACE1("app_cmd_ble_handle: cmd_id 0x%04x", cmd->cmd_id);

            T_GAP_CAUSE gap_cause = le_set_rand_addr(cmd->addr);
            ((ACK_PKT)ack_pkt)->status = (uint16_t)gap_cause;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_ATT_MTU_EXCHANGE:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            struct
            {
                uint16_t cmd_id;
                uint8_t  app_link_id;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            client_send_exchange_mtu_req(app_db.le_link[cmd->app_link_id].conn_id);

            APP_PRINT_TRACE1("app_cmd_ble_handle: CMD_LE_ATT_MTU_EXCHANGE conn_id %d",
                             app_db.le_link[cmd->app_link_id].conn_id);
        }
        break;

    case CMD_LE_CREATE_CONN:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t init_phys;
                uint8_t remote_bd_type;
                uint8_t remote_bd[6];
                uint8_t local_bd_type;
                uint16_t scan_timeout;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;
#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
            T_GAP_CAUSE gap_cause = app_lea_ini_create_conn(cmd->remote_bd,
                                                            (T_GAP_REMOTE_ADDR_TYPE)cmd->remote_bd_type);
#else
            T_GAP_CAUSE gap_cause = le_connect(cmd->init_phys, cmd->remote_bd,
                                               (T_GAP_REMOTE_ADDR_TYPE)cmd->remote_bd_type,
                                               (T_GAP_LOCAL_ADDR_TYPE)cmd->local_bd_type, cmd->scan_timeout);

            APP_PRINT_TRACE5("app_cmd_ble_handle: init_phys %d, remote_bd_type %d, remote_bd %s, local_bd_type %d, scan_timeout %d",
                             cmd->init_phys, cmd->remote_bd_type, TRACE_BDADDR(cmd->remote_bd), cmd->local_bd_type,
                             cmd->scan_timeout);
#endif
            ((ACK_PKT)ack_pkt)->status = (uint16_t)gap_cause;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_LE_DISC:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t app_link_id;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

#define ALL_LINK_ID     (0xff)

            if (cmd->app_link_id == ALL_LINK_ID)
            {
                app_ble_gap_disconnect_all(LE_LOCAL_DISC_CAUSE_CONSOLE_UART);
            }
            else
            {
                app_ble_gap_disconnect(&app_db.le_link[cmd->app_link_id], LE_LOCAL_DISC_CAUSE_CONSOLE_UART);
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_LE_CONN_PARAM_UPDATE:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t app_link_id;
                uint16_t conn_interval_min;
                uint16_t conn_interval_max;
                uint16_t conn_latency;
                uint16_t conn_supervision_timeout;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            if (ble_set_prefer_conn_param(app_db.le_link[cmd->app_link_id].conn_id, cmd->conn_interval_min,
                                          cmd->conn_interval_max,
                                          cmd->conn_latency, cmd->conn_supervision_timeout) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            APP_PRINT_TRACE5("app_customer_ble_handle_cmd: app_link_id %d, conn_interval_min %d, conn_interval_max %d, "
                             "conn_latency %d, conn_supervision_timeout %d", cmd->app_link_id, cmd->conn_interval_min,
                             cmd->conn_interval_max, cmd->conn_latency, cmd->conn_supervision_timeout);

            gaps_set_peripheral_preferred_conn_param(cmd->conn_interval_min, cmd->conn_interval_max,
                                                     cmd->conn_latency, cmd->conn_supervision_timeout);

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_LE_ADV_DATA_UPDATE:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     handle;
                uint16_t    adv_data_len;
                uint8_t    adv_data[];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            APP_PRINT_TRACE3("app_customer_ble_handle_cmd: handle %d, adv_data_len %d, adv_data %b",
                             cmd->handle, cmd->adv_data_len, TRACE_BINARY(cmd->adv_data_len, cmd->adv_data));

            if (cmd->adv_data_len <= 31)
            {
                if (adv_data_update(cmd->handle, cmd->adv_data_len, cmd->adv_data) != true)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_LE_SCAN_RSP_DATA_UPDATE:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     handle;
                uint16_t    scan_rsp_len;
                uint8_t     scan_rsp[];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            APP_PRINT_TRACE3("app_customer_ble_handle_cmd: handle %d, scan_rsp_len %d, scan_rsp %b",
                             cmd->handle, cmd->scan_rsp_len, TRACE_BINARY(cmd->scan_rsp_len, cmd->scan_rsp));

            if (cmd->scan_rsp_len <= 31)
            {
                if (scan_rsp_data_update(cmd->handle, cmd->scan_rsp_len, cmd->scan_rsp) != true)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }

        break;

    case CMD_XM_LE_ADV_INTVAL_UPDATE:
        {
            struct
            {
                uint16_t    cmd_id;
                uint8_t     handle;
                uint16_t    adv_interval;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            APP_PRINT_TRACE2("app_customer_ble_handle_cmd: handle %d, adv_interval %d",
                             cmd->handle, cmd->adv_interval);

            /*invalid adv interval between 20ms ~ 10.25s*/
            if (cmd->adv_interval >= 0x20 && cmd->adv_interval <= 0x4000)
            {
                if (change_adv_interval(cmd->handle, cmd->adv_interval) != true)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }

        break;

    case CMD_XM_LE_ATT_GET_MTU:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t  app_link_id;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            if (app_rpt_ble_mtu(cmd->app_link_id) == true)
            {

            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            }
        }
        break;

    case CMD_XM_LE_GET_ADDR:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            uint8_t random_addr[6] = {0};
            app_ble_rand_addr_get(random_addr);
            app_report_event(cmd_path, EVENT_XM_LE_ADDR, app_idx, random_addr, sizeof(random_addr));

        }
        break;

    case CMD_XM_LE_USER_CFM_REQ:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            struct
            {
                uint16_t cmd_id;
                uint8_t  app_link_id;
                uint8_t  cfm;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            if (cmd->cfm == 0x00)
            {
                le_bond_user_confirm(cmd->app_link_id, GAP_CFM_CAUSE_REJECT);
            }
            else if (cmd->cfm == 0x01)
            {
                le_bond_user_confirm(cmd->app_link_id, GAP_CFM_CAUSE_ACCEPT);
            }

            APP_PRINT_INFO2("app_customer_ble_handle_cmd: app_link_id %d, cfm 0x%x", cmd->app_link_id,
                            cmd->cfm);
        }
        break;

    case CMD_XM_LE_BOND_DEL:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            struct
            {
                uint16_t cmd_id;
                uint8_t  addr_type;
                uint8_t  addr[6];
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            APP_PRINT_TRACE2("app_customer_ble_handle_cmd: addr type %d, addr %s", cmd->addr_type,
                             TRACE_BDADDR(cmd->addr));
            T_LE_BOND_ENTRY *p_entry = bt_le_find_key_entry(cmd->addr, (T_GAP_REMOTE_ADDR_TYPE)cmd->addr_type,
                                                            NULL, LE_BOND_LOCAL_ADDRESS_ANY);
            bt_le_delete_bond(p_entry);
        }
        break;

    case CMD_XM_LE_BOND_DEL_ALL:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            bt_le_clear_all_keys();
        }
        break;

    case CMD_LE_ADV_CREATE:
    case CMD_XM_LE_ADV_CREATE:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            ADV_PARAMS_CMD *cmd = (typeof(cmd))cmd_ptr;
            SCAN_RSP *scan_rsp = (typeof(scan_rsp))&cmd->adv_data[cmd->adv_data_len];

            ADV_PARAMS params = {};
            params.adv_prop = (T_LE_EXT_ADV_LEGACY_ADV_PROPERTY)cmd->adv_prop;
            params.interval_min = cmd->interval_min;
            params.interval_max = cmd->interval_max;
            params.own_addr_type = (T_GAP_LOCAL_ADDR_TYPE)cmd->own_addr_type;
            params.peer_addr_type = (T_GAP_REMOTE_ADDR_TYPE)cmd->peer_addr_type;
            memcpy(params.own_addr, cmd->own_addr, 6);
            memcpy(params.peer_addr, cmd->peer_addr, 6);
            params.adv_data_len = cmd->adv_data_len;
            params.adv_data = cmd->adv_data;
            params.scan_rsp_len = scan_rsp->len;
            params.scan_rsp = scan_rsp->data;

            params.adv_handle = adv_create(&params);

            APP_PRINT_TRACE4("app_customer_ble_handle_cmd: adv_data_len %d, adv_data %b, scan_rsp_len %d, scan_rsp %b",
                             params.adv_data_len, TRACE_BINARY(params.adv_data_len, params.adv_data), params.scan_rsp_len,
                             TRACE_BINARY(params.scan_rsp_len, params.scan_rsp));

            APP_PRINT_TRACE8("app_customer_ble_handle_cmd: adv_handle %d, adv_prop 0x%02x, interval_min 0x%x"
                             "interval_max 0x%x, own_addr_type 0x%x, peer_addr_type 0x%x, own_addr %s, peer_addr %s",
                             params.adv_handle, params.adv_prop, params.interval_min, params.interval_max, params.own_addr_type,
                             params.peer_addr_type, TRACE_BDADDR(params.own_addr), TRACE_BDADDR(params.peer_addr));


            app_report_event(cmd_path, EVENT_XM_LE_ADV_CREATED, app_idx, &params.adv_handle,
                             sizeof(params.adv_handle));
            app_report_event(cmd_path, EVENT_LE_ADV_CREATED, app_idx, &params.adv_handle,
                             sizeof(params.adv_handle));
        }
        break;

    case CMD_XM_LE_ADV_PARAM_UPD:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);


            ADV_PARAMS_CMD *cmd = (typeof(cmd))cmd_ptr;

            SCAN_RSP *scan_rsp = (typeof(scan_rsp))&cmd->adv_data[cmd->adv_data_len];

            ADV_PARAMS params = {};
            params.adv_handle = cmd->adv_handle;
            params.adv_prop = (T_LE_EXT_ADV_LEGACY_ADV_PROPERTY)cmd->adv_prop;
            params.interval_min = cmd->interval_min;
            params.interval_max = cmd->interval_max;
            params.own_addr_type = (T_GAP_LOCAL_ADDR_TYPE)cmd->own_addr_type;
            params.peer_addr_type = (T_GAP_REMOTE_ADDR_TYPE)cmd->peer_addr_type;
            memcpy(params.own_addr, cmd->own_addr, 6);
            memcpy(params.peer_addr, cmd->peer_addr, 6);
            params.adv_data_len = cmd->adv_data_len;
            params.adv_data = cmd->adv_data;

            params.scan_rsp_len = scan_rsp->len;
            params.scan_rsp = scan_rsp->data;

            adv_param_update(&params);

            APP_PRINT_TRACE4("app_customer_ble_handle_cmd: adv_data_len %d, adv_data %b, scan_rsp_len %d, scan_rsp %b",
                             params.adv_data_len, TRACE_BINARY(params.adv_data_len, params.adv_data), params.scan_rsp_len,
                             TRACE_BINARY(params.scan_rsp_len, params.scan_rsp));

            APP_PRINT_TRACE8("app_customer_ble_handle_cmd: adv_handle %d, adv_prop 0x%02x, interval_min 0x%x"
                             "interval_max 0x%x, own_addr_type 0x%x, peer_addr_type 0x%x, own_addr %s, peer_addr %s",
                             params.adv_handle, params.adv_prop, params.interval_min, params.interval_max, params.own_addr_type,
                             params.peer_addr_type, TRACE_BDADDR(params.own_addr), TRACE_BDADDR(params.peer_addr));
        }
        break;

    case CMD_XM_LE_GET_PHY:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t  app_link_id;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            struct
            {
                uint8_t app_link_id;
                uint8_t rx_phy; // T_GAP_PHYS_TYPE
                uint8_t tx_phy;
            } __attribute__((packed)) rpt = {};

            rpt.app_link_id = cmd->app_link_id;
            le_get_conn_param(GAP_PARAM_CONN_RX_PHY_TYPE, &rpt.rx_phy, app_db.le_link[rpt.app_link_id].conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TX_PHY_TYPE, &rpt.tx_phy, app_db.le_link[rpt.app_link_id].conn_id);
            APP_PRINT_TRACE3("app_customer_ble_handle_cmd: app_link_id %d, rx_phy 0x%x, tx_phy 0x%x",
                             rpt.app_link_id, rpt.rx_phy, rpt.tx_phy);

            app_report_event(cmd_path, EVENT_XM_GET_PHY, app_idx, (uint8_t *)&rpt,
                             sizeof(rpt));
        }
        break;

    case CMD_LE_MODIFY_WHITELIST:
        {
            struct
            {
                uint16_t cmd_id;
                T_GAP_WHITE_LIST_OP  operation;
                uint8_t addr[6];
                T_GAP_REMOTE_ADDR_TYPE bd_type;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            T_GAP_CAUSE cause = app_ble_gap_modify_white_list(cmd->operation, cmd->addr, cmd->bd_type);
            if (cause != GAP_CAUSE_SUCCESS)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                APP_PRINT_ERROR1("app_customer_ble_handle_cmd: modify whitelist fail cause %d", cause);
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_MODIFY_WHITELIST_BY_IDX:
        {
            struct
            {
                uint16_t cmd_id;
                T_GAP_WHITE_LIST_OP operation;
                uint8_t bond_idx;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            uint8_t bond_num = bt_le_get_bond_dev_num();

            if (cmd->bond_idx >= bond_num && cmd->operation != GAP_WHITE_LIST_OP_CLEAR)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            else
            {
                T_LE_BOND_ENTRY *p_entry = bt_le_find_key_entry_by_priority(cmd->bond_idx + 1);
                if (app_ble_gap_modify_white_list(cmd->operation,
                                                  p_entry->remote_identity_addr,
                                                  (T_GAP_REMOTE_ADDR_TYPE)p_entry->remote_identity_addr_type) != GAP_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_MODIFY_RESOLVELIST_BY_IDX:
        {
            struct
            {
                uint16_t cmd_id;
                T_GAP_RESOLV_LIST_OP operation;
                uint8_t bond_idx;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            uint8_t bond_num = bt_le_get_bond_dev_num();

            if (cmd->bond_idx >= bond_num && cmd->operation != GAP_RESOLV_LIST_OP_CLEAR)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            else
            {
                T_LE_BOND_ENTRY *p_entry = bt_le_find_key_entry_by_priority(cmd->bond_idx + 1);
                uint8_t p_peer_irk[16] = {0};
                uint8_t p_local_irk[16] = {0};
                if (p_entry != NULL)
                {
                    bool result = bt_le_get_dev_irk(p_entry, false, (uint8_t *)&p_local_irk);
                    APP_PRINT_INFO2("bt_le_get_dev_irk result %d, %b", result, TRACE_BINARY(16, p_local_irk));
                    result = bt_le_get_dev_irk(p_entry, true, (uint8_t *)&p_peer_irk);
                    APP_PRINT_INFO2("bt_le_get_dev_irk result %d, %b", result, TRACE_BINARY(16, p_peer_irk));
                    if (result)
                    {
                        if (app_ble_gap_modify_resolve_list(cmd->operation,
                                                            p_entry->remote_identity_addr,
                                                            (T_GAP_IDENT_ADDR_TYPE)p_entry->remote_identity_addr_type,
                                                            p_peer_irk,
                                                            p_local_irk) != GAP_CAUSE_SUCCESS)
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                }
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_SET_RESOLUTION:
        {
            bool enable = cmd_ptr[2];
            T_GAP_CAUSE cause = le_privacy_set_addr_resolution(enable);
            if (cause != GAP_CAUSE_SUCCESS)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                APP_PRINT_ERROR1("app_customer_ble_handle_cmd: resolution set fail cause %d", cause);
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LE_USER_PASSKEY_INPUT:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t  app_link_id;
                uint8_t  key[6];
            } __attribute__((packed)) *cmd = (__typeof__(cmd))cmd_ptr;

            char passkey_str[7];
            for (int i = 0; i < 6; i++)
            {
                passkey_str[i] = cmd->key[i];
            }
            passkey_str[6] = '\0';
            uint32_t passkey = (uint32_t)atoi(passkey_str);
            uint8_t bd_addr[6];
            le_bond_passkey_input_confirm(app_db.le_link[cmd->app_link_id].conn_id, passkey,
                                          GAP_CFM_CAUSE_ACCEPT);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        break;
    }
}


void app_cmd_ble_init(void)
{
    os_queue_init(&ble.advs.q);
}

