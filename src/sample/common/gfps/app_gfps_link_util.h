/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_GFPS_LINK_UTIL_H_
#define _APP_GFPS_LINK_UTIL_H_

#include <stdint.h>
#include <stdbool.h>
#include "os_queue.h"
#include "app_gfps.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define APP_GFPS_MAX_LINKS 4

#define APP_STOP_ADV_CAUSE_GFPS_ACTION_IDLE       (0xF1)
#define APP_STOP_ADV_CAUSE_GFPS_EXIT_PAIRING_MODE (0xF2)
#define APP_STOP_ADV_CAUSE_GFPS_FINDER            (0xF3)
#define APP_STOP_ADV_CAUSE_GFPS_DULT_PAUSE        (0xF4)
#define APP_STOP_ADV_CAUSE_GFPS_FINDER_KEY_PRESS  (0xF5)
#define APP_STOP_ADV_CAUSE_GFPS_FINDER_UPDATE_EIK (0xF6)
// you can add new cause in here

typedef enum
{
    GFPS_LE_LOCAL_DISC_CAUSE_GFPS_STOP                     = 0x01,
    GFPS_LE_LOCAL_DISC_CAUSE_GFPS_FAILED                   = 0x02,
    GFPS_LE_LOCAL_DISC_CAUSE_GFPS_LINK_FULL                = 0x03,
    GFPS_LE_LOCAL_DISC_CAUSE_GFPS_FINDER                   = 0x04,
    GFPS_LE_LOCAL_DISC_CAUSE_GFPS_PAIR_NOT_STARTED         = 0x05,
    GFPS_LE_LOCAL_DISC_CAUSE_GFPS_PAIR_NOT_RECEIVE_PASSKEY = 0x06,
} T_GFPS_LE_LOCAL_DISC_CAUSE;

typedef enum
{
    GFPS_LE_LINK_STATE_DISCONNECTED,
    GFPS_LE_LINK_STATE_CONNECTING,
    GFPS_LE_LINK_STATE_CONNECTED,
    GFPS_LE_LINK_STATE_DISCONNECTING,
} T_GFPS_LE_LINK_STATE;

typedef struct t_app_gfps_le_link
{
    uint8_t                 bd_addr[6];
    bool                    used;
    uint16_t                conn_handle;
    uint8_t                 conn_id;
    uint8_t                 id;
    uint8_t                 state;
    uint8_t                 local_disc_cause;
    T_GFPS_LINK             gfps_link;
} T_APP_GFPS_LE_LINK;

typedef struct
{
    T_APP_GFPS_LE_LINK le_link[APP_GFPS_MAX_LINKS];
} T_APP_GFPS_LINK_DB;

extern T_APP_GFPS_LINK_DB app_gfps_link_db;
/**
    * @brief  find the BLE link by connected id
    * @param  conn_id BLE link id(slot)
    * @return the BLE link
    */
T_APP_GFPS_LE_LINK *app_gfps_link_find_le_link_by_conn_id(uint8_t conn_id);

/**
    * @brief  find the BLE link by bluetooth address
    * @param  bd_addr bluetooth address
    * @return the BLE link
    */
T_APP_GFPS_LE_LINK *app_gfps_link_find_le_link_by_addr(uint8_t *bd_addr);

/**
    * @brief  alloc the BLE link by link id(slot)
    * @param  conn_id BLE link id(slot)
    * @return the BLE link
    */
T_APP_GFPS_LE_LINK *app_gfps_link_alloc_le_link_by_conn_id(uint8_t conn_id);

/**
    * @brief  free the BLE link
    * @param  p_link the BLE link
    * @return true: success; false: fail
    */
bool app_gfps_link_free_le_link(T_APP_GFPS_LE_LINK *p_link);

/**
 * @brief disconnect BLe link
 *
 * @param p_link  current ble link
 * @param disc_cause  local disconnect cause
 * @return true
 * @return false
 */
bool app_gfps_ble_disconnect(T_APP_GFPS_LE_LINK *p_link, T_GFPS_LE_LOCAL_DISC_CAUSE disc_cause);

/**
 * @brief GFPS handle connection state
 *
 * @param conn_id connection id
 * @param new_state new connection state
 * @param disc_cause disconnect cause
 */
void app_gfps_link_handle_new_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                             uint16_t disc_cause);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_GFPS_LINK_UTIL_H_ */
