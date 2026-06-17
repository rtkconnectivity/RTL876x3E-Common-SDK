/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_LINK_H_
#define _APP_LEA_CAP_ACC_LINK_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "app_msg.h"
#include "gap_conn_le.h"
#include "os_queue.h"

/** @defgroup APP_LEA_CAP_ACC_LINK_MGR CAP Acceptor APP Link Mgr
  * @brief CAP Acceptor APP Link Mgr
  * @{
  */

/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @addtogroup  APP_LEA_CAP_ACC_GAP_MSG
    * @{
    */
#if BAP_UNICAST_SERVER
#define APP_LEA_CCID_MAX_NUM  4
#define APP_LEA_SINK_ASE_NUM  2
#define APP_LEA_SRC_ASE_NUM   1
#define APP_LEA_ASE_NUM (APP_LEA_SINK_ASE_NUM + APP_LEA_SINK_ASE_NUM)

typedef struct t_app_lea_ase
{
    bool     used;
    uint8_t  ase_id;
    uint8_t  ase_state;
    uint8_t  cig_id;
    uint8_t  direction;
    uint8_t  ccid_num;
    uint8_t  ccid[APP_LEA_CCID_MAX_NUM];
    uint16_t iso_conn_handle;
    uint16_t audio_contexts;
} T_APP_LEA_ASE;
#endif

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
#if BAP_UNICAST_SERVER
    T_APP_LEA_ASE       lea_ase_tbl[APP_LEA_ASE_NUM];
#endif
#if (MCP_MEDIA_CONTROL_CLIENT || CCP_CALL_CONTROL_CLIENT)
    T_OS_QUEUE          content_queue;
#endif
#if VCP_VOLUME_RENDERER
    uint8_t             volume_setting;
    uint8_t             mute;
#endif
} T_APP_LE_LINK;
/** @} */ /* End of group APP_LEA_CAP_ACC_GAP_MSG */


/*============================================================================*
 *                              Variables
 *============================================================================*/



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

/** End of _APP_LEA_CAP_ACC_LINK_H_
* @}
*/
#endif
