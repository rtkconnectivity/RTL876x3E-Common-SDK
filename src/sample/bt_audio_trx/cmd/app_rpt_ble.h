/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_RPT_BLE_H_
#define _APP_RPT_BLE_H_

#include "stdint.h"
#include "stdbool.h"
#include "app_link_util_cs.h"
#include "app_cmd.h"
#include "gap_callback_le.h"

void app_rpt_ble_conn_cmpl(T_APP_LE_LINK *p_link);

bool app_rpt_ble_mtu(uint8_t app_link_id);

bool app_rpt_ble_bond_cfm(uint8_t app_link_id);

void app_rpt_ble_conn_update(T_APP_LE_LINK *p_link, uint8_t res);

void app_rpt_ble_pair_status(uint8_t app_link_id, uint16_t cause, uint8_t *resolved_addr);

void app_rpt_ble_rand_addr(uint8_t rand_addr[6]);

void app_rpt_ble_set_data_len(T_LE_DATA_LEN_CHANGE_INFO *p_info);

void app_rpt_ble_link_key(uint8_t app_link_id, T_GAP_REMOTE_ADDR_TYPE addr_type,
                          uint8_t addr[6]);

void app_rpt_ble_phy_upd(T_LE_PHY_UPDATE_INFO *p_info);

void app_rpt_ble_ignore_slave_latency(T_LE_DISABLE_SLAVE_LATENCY_RSP *rsp);

void app_rpt_ble_scan_info(T_LE_EXT_ADV_REPORT_INFO *p_info);

#endif

