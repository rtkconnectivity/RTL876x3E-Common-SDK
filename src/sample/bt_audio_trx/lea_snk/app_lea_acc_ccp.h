/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_ACC_CCP_H_
#define _APP_LEA_ACC_CCP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "ccp_client.h"
#include "app_link_util_cs.h"
#include "app_types.h"
#include "app_lea_acc_ccp_def.h"

/** @defgroup APP_LEA_CCP App LE Audio CCP
  * @brief this file handle CCP profile related process
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_LEA_CCP_Exported_Functions App LE Audio CCP Functions
    * @{
    */

/**
    * @brief  ccp module init. Register ble audio callback
    *         to handle ccp message
    * @param  void
    * @return void
    */
void app_lea_ccp_init(void);

/**
    * @brief  Get ccp active link connection handle
    * @param  void
    * @return LE link connection handle
    */
uint16_t app_lea_ccp_get_active_conn_handle(void);

/**
    * @brief  Set ccp active link connection handle
    * @param  conn_handle LE link connection handle
    * @return true: success; false: fail
    */
bool app_lea_ccp_set_active_conn_handle(uint16_t conn_handle);

/**
    * @brief  Get LE Audio device call status
    * @param  void
    * @return LE Audio device call status
    */
T_APP_CALL_STATUS app_lea_ccp_get_call_status(void);

/**
    * @brief  Update LE Audio link CCP call status
    * @param  p_link the LE link
    * @return void
    */
void app_lea_ccp_update_link_call_status(T_APP_LE_LINK *p_link);

/**
    * @brief  Update LE Audio device CCP call status
    * @param  void
    * @return void
    */
void app_lea_ccp_update_call_status(void);

/**
    * @brief  Parse CCP call state with CCP notify data, and update call state
    * @param  p_link the LE link
    * @param  p_notify_data CCP notify data
    * @return void
    */
void app_lea_ccp_parse_and_update_call_state(T_APP_LE_LINK *p_link,
                                             T_CCP_CLIENT_NOTIFY *p_notify_data);

/**
    * @brief  Trigger call ringtone when call state change
    * @param  p_link the LE link
    * @param  call_status \ref T_APP_CALL_STATUS
    * @return void
    */
void app_lea_ccp_trigger_call_ringtone(T_APP_LE_LINK *p_link, T_APP_CALL_STATUS call_status);

/**
    * @brief  Find call entry by call index
    * @param  conn_id LE link connection index
    * @param  call_index call index
    * @return call entry
    * @retval \ref T_LEA_CALL_ENTRY
    */
T_LEA_CALL_ENTRY *app_lea_ccp_find_call_entry_by_idx(uint8_t conn_id, uint8_t call_index);
/** @} */ /* End of group APP_LEA_CCP_Exported_Functions */
/** End of APP_LEA_CCP
* @}
*/

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
