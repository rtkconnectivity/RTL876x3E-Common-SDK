/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "app_link_util_cs.h"
#include "app_main.h"
#include "bt_bond_le.h"
#include "gap_bond_le.h"
#include "gap_conn_le.h"
#include "trace.h"


void app_rpt_ble_conn_cmpl(T_APP_LE_LINK *p_link)
{
    struct
    {
        uint8_t state;
        uint8_t app_link_id;
        uint16_t conn_handle;
        uint8_t role;
        uint8_t addr_type;
        uint8_t addr[6];
        uint16_t conn_interval;
        uint16_t conn_latency;
        uint16_t sup_tout;
    } __attribute__((packed)) rpt;

    T_GAP_CONN_INFO conn_info = {};
    rpt.state = LE_LINK_STATE_CONNECTED;
    rpt.app_link_id = p_link->id;
    rpt.conn_handle = p_link->conn_handle;
    if (le_get_conn_info(p_link->conn_id, &conn_info))
    {
        rpt.role = conn_info.role;
        rpt.addr_type = conn_info.remote_bd_type;
        memcpy(rpt.addr, conn_info.remote_bd, 6);
    }
    else
    {
        return;
    }

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &rpt.conn_interval, p_link->conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &rpt.conn_latency, p_link->conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &rpt.sup_tout, p_link->conn_id);

    app_report_event(CMD_PATH_UART, EVENT_XM_LE_CON_STATE, 0, (uint8_t *)&rpt, sizeof(rpt));
}


void app_rpt_ble_conn_update(T_APP_LE_LINK *p_link, uint8_t res)
{
    struct
    {
        uint8_t app_link_id;
        uint8_t update_res;
        uint16_t conn_interval;
        uint16_t conn_latency;
        uint16_t sup_tout;
    } __attribute__((packed)) rpt;

    rpt.app_link_id = p_link->id;
    uint8_t conn_id = p_link->conn_id;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &rpt.conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &rpt.conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &rpt.sup_tout, conn_id);
    rpt.update_res = res;
    app_report_event(CMD_PATH_UART, EVENT_XM_LE_CON_PARAM, 0, (uint8_t *)&rpt, sizeof(rpt));
}

bool app_rpt_ble_mtu(uint8_t app_link_id)
{
    if (app_db.le_link[app_link_id].state == LE_LINK_STATE_CONNECTED)
    {
        struct
        {
            uint8_t app_link_id;
            uint16_t mtu;
        } __attribute__((packed)) rpt;

        rpt.app_link_id = app_link_id;

        enum hdr_sz {att_hdr_sz = 3};
        rpt.mtu = app_db.le_link[app_link_id].mtu_size - att_hdr_sz;

        app_report_event(CMD_PATH_UART, EVENT_XM_LE_ATT_MTU, 0, (uint8_t *)&rpt, sizeof(rpt));
        return true;
    }

    return false;
}

bool app_rpt_ble_bond_cfm(uint8_t app_link_id)
{
    struct
    {
        uint8_t app_link_id;
        uint32_t display_val;
    } __attribute__((packed)) rpt;

    rpt.app_link_id = app_link_id;
    le_bond_get_display_key(app_db.le_link[app_link_id].conn_id, &rpt.display_val);
    APP_PRINT_INFO1("GAP_MSG_LE_BOND_USER_CONFIRMATION display_value = %d", rpt.display_val);
    app_report_event(CMD_PATH_UART, EVENT_XM_LE_USER_CONFIRMATION_REQ, 0, (uint8_t *)&rpt, sizeof(rpt));

    return true;
}


void app_rpt_ble_pair_status(uint8_t app_link_id, uint16_t cause, uint8_t *resolved_addr)
{
    struct
    {
        uint8_t  app_link_id;
        uint16_t cause;
        uint8_t  addr[6];
    } __attribute__((packed)) rpt;

    rpt.app_link_id = app_link_id;
    rpt.cause = cause;
    if (cause == GAP_SUCCESS)
    {
        memcpy(rpt.addr, resolved_addr, 6);
    }
    else
    {
        return;
    }
    app_report_event(CMD_PATH_UART, EVENT_LE_PAIR_STATUS, 0, (uint8_t *)&rpt, sizeof(rpt));
}


void app_rpt_ble_rand_addr(uint8_t rand_addr[6])
{
    APP_PRINT_TRACE1("app_rpt_ble_rand_addr: rand_addr %s", TRACE_BDADDR(rand_addr));

    app_report_event(CMD_PATH_UART, EVENT_XM_LE_ADDR, 0, rand_addr, 6);
}

void app_rpt_ble_link_key(uint8_t app_link_id, T_GAP_REMOTE_ADDR_TYPE addr_type,
                          uint8_t addr[6])
{
    T_LE_BOND_ENTRY *p_entry = NULL;

    struct
    {
        uint8_t  app_link_id;
        uint8_t  addr_type;
        uint8_t  addr[6];
        uint8_t  ltk_len;
        uint8_t  ltk[16];
    } __attribute__((packed)) rpt;

    rpt.app_link_id = app_link_id;
    rpt.addr_type = addr_type;
    memcpy(rpt.addr, addr, 6);

    uint8_t local_bd_addr[6] = {0};
    uint8_t local_bd_type = LE_BOND_LOCAL_ADDRESS_ANY;
    extern bool le_get_conn_local_addr(uint16_t conn_handle, uint8_t *bd_addr, uint8_t *bd_type);
    le_get_conn_local_addr(app_db.le_link[app_link_id].conn_handle, local_bd_addr, &local_bd_type);
    p_entry = bt_le_find_key_entry(addr, addr_type, local_bd_addr, local_bd_type);
    bool ret = bt_le_get_dev_ltk(p_entry, false, &rpt.ltk_len, rpt.ltk);

    APP_PRINT_TRACE6("app_rpt_ble_link_key: p_entry %p, ret %d, addr_type %d, addr %s, link_key_len %d, link_key %b",
                     p_entry, ret, addr_type, TRACE_BDADDR(rpt.addr), rpt.ltk_len,
                     TRACE_BINARY(rpt.ltk_len, rpt.ltk));

    app_report_event(CMD_PATH_UART, EVENT_XM_LE_LINK_KEY, 0, (uint8_t *)&rpt, sizeof(rpt));
}

void app_rpt_ble_set_data_len(T_LE_DATA_LEN_CHANGE_INFO *p_info)
{
    struct
    {
        uint8_t   app_link_id;
        uint16_t  max_tx_octets;
        uint16_t  max_tx_time;
        uint16_t  max_rx_octets;
        uint16_t  max_rx_time;
    } __attribute__((packed)) rpt;

    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(p_info->conn_id);

    if (p_link)
    {
        rpt.app_link_id = p_link->id;
        rpt.max_tx_octets = p_info->max_tx_octets;
        rpt.max_tx_time = p_info->max_tx_time;
        rpt.max_rx_octets = p_info->max_rx_octets;
        rpt.max_rx_time = p_info->max_rx_time;

        APP_PRINT_TRACE5("app_rpt_ble_set_data_len: app_link_id %d, max_tx_octets %d, max_tx_time %d, max_rx_octets %d, max_rx_time %d",
                         rpt.app_link_id, rpt.max_tx_octets, rpt.max_tx_time, rpt.max_rx_octets, rpt.max_rx_time);
        app_report_event(CMD_PATH_UART, EVENT_LE_SET_DATA_LEN, 0, (uint8_t *)&rpt, sizeof(rpt));
    }
}

void app_rpt_ble_phy_upd(T_LE_PHY_UPDATE_INFO *p_info)
{
    struct
    {
        uint8_t   app_link_id;
        uint8_t  tx_phy;
        uint8_t  rx_phy;
    } __attribute__((packed)) rpt;

    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(p_info->conn_id);

    if (p_link)
    {
        rpt.app_link_id = p_link->id;
        rpt.tx_phy = p_info->tx_phy;
        rpt.rx_phy = p_info->rx_phy;

        APP_PRINT_TRACE3("app_rpt_ble_phy_upd: app_link_id %d, tx_phy %d, rx_phy %d",
                         rpt.app_link_id, rpt.tx_phy, rpt.rx_phy);

        app_report_event(CMD_PATH_UART, EVENT_LE_PHY_UPD, 0, (uint8_t *)&rpt, sizeof(rpt));
    }
}

void app_rpt_ble_scan_info(T_LE_EXT_ADV_REPORT_INFO *p_info)
{
    /** @brief  Information of le extended advertising report. */
    struct
    {
        uint16_t        event_type;
        uint8_t         addr_type;
        uint8_t         bd_addr[6];
        int8_t          rssi;
        uint8_t         data_len;
        uint8_t         data[]; //max 229 bytes
    } __attribute__((packed)) * rpt = NULL;

    uint16_t rpt_len = sizeof(*rpt) + p_info->data_len;
    rpt = malloc(rpt_len);
    if (rpt)
    {
        rpt->event_type = p_info->event_type;
        rpt->addr_type = p_info->addr_type;
        memcpy(rpt->bd_addr, p_info->bd_addr, 6);
        rpt->rssi = p_info->rssi;
        rpt->data_len = p_info->data_len;
        memcpy(rpt->data, p_info->p_data, rpt->data_len);
        app_report_event(CMD_PATH_UART, EVENT_LE_SCAN_INFO, 0, (uint8_t *)rpt, rpt_len);
        APP_PRINT_TRACE5("app_rpt_ble_scan_info: event_type %d, addr_type %d, bd_addr %s, rssi %d, data_len %d",
                         rpt->event_type, rpt->addr_type, TRACE_BDADDR(rpt->bd_addr), rpt->rssi, rpt->data_len);
        free(rpt);
    }
}


void app_rpt_ble_ignore_slave_latency(T_LE_DISABLE_SLAVE_LATENCY_RSP *rsp)
{
    enum PARAM_POS
    {
        RES_POS     = 0,
        END_POS     = RES_POS + 1,
    };

    struct
    {
        uint8_t   res;
    } __attribute__((packed)) rpt;

    rpt.res = (rsp->cause == HCI_SUCCESS);
    APP_PRINT_TRACE1("app_rpt_ble_ignore_slave_latency: cause %d", rsp->cause);
    app_report_event(CMD_PATH_UART, EVENT_LE_IGNORE_SLAVE_LATENCY, 0, (uint8_t *)&rpt, sizeof(rpt));
}

