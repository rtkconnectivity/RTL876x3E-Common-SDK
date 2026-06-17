/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_INI_CONN_MGR_H_
#define _APP_LEA_INI_CONN_MGR_H_

#include <stdbool.h>
#include <stdint.h>
#include "gap.h"
#include "gap_callback_le.h"
#include "gap_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* connection parameter boundary values */
#define BLE_SCAN_INT_MIN            0x0004
#define BLE_SCAN_INT_MAX            0x4000
#define BLE_SCAN_WIN_MIN            0x0004
#define BLE_SCAN_WIN_MAX            0x4000
#define BLE_EXT_SCAN_INT_MAX        0x00FFFFFF
#define BLE_EXT_SCAN_WIN_MAX        0xFFFF
#define BLE_CONN_INT_MIN            0x0006
#define BLE_CONN_INT_MAX            0x0C80
#define BLE_CONN_LATENCY_MAX        500
#define BLE_CONN_SUP_TOUT_MIN       0x000A
#define BLE_CONN_SUP_TOUT_MAX       0x0C80
#define CONN_CE_MIN                 0x0002

#define BLE_CONN_FAST_CI_DEF    (0x0006)
#define BLE_CONN_NORMAL_CI_DEF  (0x0018)
/* default slave latency */
#define BLE_CONN_SLAVE_LATENCY_DEF 0 /* 0 */
/* default supervision timeout */
#define BLE_CONN_TIMEOUT_DEF 500

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#define BLE_CONN_TIMEOUT_SCO_END_DEF 450
#endif

typedef struct ble_conn_param
{
    uint16_t ci_min;
    uint16_t ci_max;
    uint16_t latency;
    uint16_t timeout;
    uint16_t ce_min;
    uint16_t ce_max;
    uint16_t ci_min_pend;
    uint16_t ci_max_pend;
    uint16_t latency_pend;
    uint16_t timeout_pend;
    uint16_t ce_min_pend;
    uint16_t ce_max_pend;
    bool     ce_2slot_used;
    bool     param_update_pend;
    uint8_t conn_mask;
} T_LEA_CONN_PARAM;

typedef enum
{
    BLE_UPDATE_WAIT_RSP         = 0x01, /* waiting for connection update finished */
    BLE_UPDATE_DISABLE          = 0x02, /* conn update disabled */
} T_BLE_CONN_UPDATE;

typedef enum
{
    CP_EVENT_GATT_DISCOVERY,
    CP_EVENT_GATT_DONE,
    CP_EVENT_DISCV_ALL_DONE,
    CP_EVENT_CIS_ESTABLISH,
    CP_EVENT_CIS_DISCONNECT,
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    CP_EVENT_SCO_ENABLE,
    CP_EVENT_SCO_END_ENABLE,
    CP_EVENT_BIS_ITV_10_ENABLE,
    CP_EVENT_DEFAULT = 0xFF,
#endif
} T_UPDATE_CONN_PARAM_EVENT;

typedef struct t_conn_dev_info
{
    struct t_conn_dev_info *p_next;
    uint8_t addr_type;
    uint8_t bd_addr[6];
} T_CONN_DEV_INFO;

T_GAP_CAUSE app_lea_ini_create_conn(uint8_t *p_bd_addr, uint8_t addr_type);
void app_lea_ini_add_conn_dev(uint8_t addr_type, uint8_t *bd_addr);

void app_lea_ini_handle_connected(uint8_t conn_id);
void app_lea_ini_handle_disconnected(uint8_t conn_id);

bool app_ini_enable_conn_update(uint8_t *addr);
bool app_ini_disable_conn_update(uint8_t *addr);

bool app_lea_ini_handle_dev_state_change_event(uint8_t *bd_addr, T_UPDATE_CONN_PARAM_EVENT event);

T_APP_RESULT app_lea_ini_handle_conn_update_ind(T_LE_CONN_UPDATE_IND *p_conn_ind);
void app_lea_ini_handle_conn_update_status(T_GAP_CONN_PARAM_UPDATE update_info);

#if F_APP_FIX_RAPOO_M300G_IOP_ISSUE
uint8_t app_tri_dongle_get_rapoo_m300g_status(void);
void app_tri_dongle_set_rapoo_m300g_status(bool evt);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_LEA_INI_CONN_MGR_H_ */
