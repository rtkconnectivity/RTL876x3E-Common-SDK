/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_INI_LINK_H_
#define _APP_LEA_CAP_INI_LINK_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "app_msg.h"
#include "gap_conn_le.h"
#include "broadcast_source_sm.h"

/** @defgroup APP_LEA_CAP_INI_LINK_MGR CAP Initiator APP Link Mgr
  * @brief CAP Initiator APP Link Mgr
  * @{
  */

/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @addtogroup  APP_LEA_CAP_INI_GAP_MSG
    * @{
    */
#define LEA_LINK_DISC_PACS_DONE     0x0001
#define LEA_LINK_DISC_ASCS_DONE     0x0002
#define LEA_LINK_DISC_CSIS_DONE     0x0004
#define LEA_LINK_DISC_BASS_DONE     0x0008
#define LEA_LINK_DISC_CAS_DONE      0x0010

#define APP_LEA_CCID_MAX_NUM 4

#if BAP_BROADCAST_ASSISTANT
typedef struct
{
    bool    brs_is_used;
    uint8_t instance_id;
    uint8_t source_id;
    T_BROADCAST_SOURCE_HANDLE handle;
} T_BASS_BRS_CHAR_DB;
#endif

/*============================================================================*
 *                              Variables
 *============================================================================*/
/**  @brief  App define le link connection database */
typedef struct t_app_le_link
{
    bool                used;
    uint8_t             id;
    uint16_t            mtu_size;
    uint16_t            conn_handle;
    uint8_t             state;
    uint8_t             conn_id;
    uint8_t             bd_addr[6];
    uint8_t             bd_type;
    bool                mtu_received;
    bool                auth_cmpl;
    bool                start_discover;

    uint16_t            lea_disc_flags;
    uint16_t            lea_srv_found_flags;
#if BAP_BROADCAST_ASSISTANT
    uint8_t             brs_char_num;
    T_BASS_BRS_CHAR_DB  *brs_char_tbl;
#endif
} T_APP_LE_LINK;
/** @} */ /* End of group APP_LEA_CAP_INI_GAP_MSG */


/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
    * @brief  find the BLE link by connected id
    * @param  conn_id BLE link id(slot)
    * @return the BLE link
    */
T_APP_LE_LINK *app_link_find_le_link_by_conn_id(uint8_t conn_id);

/**
    * @brief  find the BLE link by connected handle
    * @param  conn_handle BLE connection handle
    * @return the BLE link
    */
T_APP_LE_LINK *app_link_find_le_link_by_conn_handle(uint16_t conn_handle);

/**
    * @brief  alloc the BLE link by link id(slot)
    * @param  conn_id BLE link id(slot)
    * @return the BLE link
    */
T_APP_LE_LINK *app_link_alloc_le_link_by_conn_id(uint8_t conn_id);

/**
    * @brief  free the BLE link
    * @param  p_link the BLE link
    * @return true: success; false: fail
    */
bool app_link_free_le_link(T_APP_LE_LINK *p_link);

/** End of _APP_LEA_CAP_INI_LINK_H_
* @}
*/
#endif
