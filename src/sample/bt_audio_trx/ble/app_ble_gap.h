/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BLE_GAP_H_
#define _APP_BLE_GAP_H_

#include <stdint.h>
#include <stdbool.h>

#include "app_msg.h"
#include "gap.h"
#include "ble_ext_adv.h"
#include "app_link_util_cs.h"
#include "app_adv_stop_cause.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_BLE_GAP App Ble Gap
  * @brief App Ble Gap
  * @{
  */
#define RWS_LE_DEFAULT_MIN_CONN_INTERVAL   (12)//12*1.25 = 15ms
#define RWS_LE_DEFAULT_MAX_CONN_INTERVAL   (24)//24*1.25 = 30ms
#define RWS_LE_DEFAULT_SLAVE_LATENCY       (0)
#define RWS_LE_DEFAULT_SUPERVISION_TIMEOUT (500)//500*10 = 5000ms


extern uint8_t scan_rsp_data_len;
extern uint8_t scan_rsp_data[GAP_MAX_LEGACY_ADV_LEN];

void app_ble_gap_param_init(void);
void app_ble_gap_init(void);
bool app_ble_gap_gen_scan_rsp_data(uint8_t *p_scan_len, uint8_t *p_scan_data);
bool app_ble_gap_disconnect(T_APP_LE_LINK *p_link, T_LE_LOCAL_DISC_CAUSE disc_cause);
void app_ble_gap_disconnect_all(T_LE_LOCAL_DISC_CAUSE disc_cause);
T_GAP_CAUSE app_ble_gap_modify_white_list(T_GAP_WHITE_LIST_OP  operation, uint8_t *bd_addr,
                                          T_GAP_REMOTE_ADDR_TYPE bd_type);
T_GAP_CAUSE app_ble_gap_modify_resolve_list(T_GAP_RESOLV_LIST_OP  operation, uint8_t *bd_addr,
                                            T_GAP_IDENT_ADDR_TYPE bd_type,
                                            uint8_t *p_peer_irk, uint8_t *p_local_irk);
/**
    * @brief  All the bt gap msg events are pre-handled in this function.
    *         Then the event handling function shall be called according to the subType of T_IO_MSG.
    * @param  p_io_msg  pointer to io msg
    * @return void
    */
void app_ble_gap_handle_gap_msg(T_IO_MSG *p_io_msg);

/**
    * @brief  app ble get link encryption status
    * @param  T_APP_LE_LINK *
    * @return @ref T_LE_LINK_ENCRYPTION_STATE
    */
LE_ENCRYPT_STATE app_ble_gap_get_link_encryption_status(T_APP_LE_LINK *p_link);

/**
 * @brief get remote bd address and address type and store them into p_link.
 *
 * @param p_link  @ref T_APP_LE_LINK
 *
 * remote use RPA(resolvable private address) and resolved address type is public address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_PUBLIC;
 * p_link->bd_addr is resloved public address;
 *
 * remote use RPA(resolvable private address) and resolved address type is static random address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_STATIC;
 * p_link->bd_addr is resloved static random address;
 *
 * remote use RPA(resolvable private address) and RPA can not be success resolved:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_UNRESOLVED;
 * p_link->bd_addr is RPA;
 * Reasons why RPA cannot be successfully resolved may be:
 * 1.Pairing failed or pairing was not performed, resulting in stack unable to obtain the remote identity address.
 * 2.Pairing is successful and stack can obtain the remote identity address, but RPA resolution fails.
 *
 * remote use static random address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_STATIC_RANDOM;
 * p_link->bd_addr is static random address;
 *
 * remote use public address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_PUBLIC;
 * p_link->bd_addr is public address;
 */
void app_ble_gap_get_remote_bd_addr_and_type(T_APP_LE_LINK *p_link);
/** End of APP_BLE_GAP
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BLE_GAP_H_ */
