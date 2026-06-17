/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_INI_MAIN_H_
#define _APP_LEA_CAP_INI_MAIN_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "csis_def.h"
#include "app_lea_cap_ini_link.h"
#include "os_queue.h"
#include "app_lea_cap_ini_bap_bsrc.h"
#include "bt_gatt_svc.h"
#include "app_lea_cap_ini_mcp.h"
#include "app_lea_cap_ini_ccp.h"

/** @defgroup APP_LEA_CAP_INI_MAIN BLE Audio CAP Initiator Main
  * @brief Main file to initialize hardware and BT stack and start task scheduling
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_LEA_CAP_INI_MAIN_Exported_Macros App Main Macros
    * @{
    */

/** End of APP_LEA_CAP_INI_MAIN_Exported_Macros
    * @}
    */
/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_LEA_CAP_INI_MAIN_Exported_Types App Main Types
    * @{
    */

/**  @brief  App define global app data structure */
typedef struct
{
    T_APP_LE_LINK le_link[APP_MAX_BLE_LINK_NUM];
    uint8_t       bap_role;
    uint8_t       cap_role;
    T_OS_QUEUE    conn_dev_queue;
    T_OS_QUEUE    group_handle_queue;
    T_OS_QUEUE    scan_dev_queue;
#if BAP_BROADCAST_SOURCE
    T_BROADCAST_SOURCE_CB bsrc_db;
#endif
    T_OS_QUEUE    iso_input_queue;        //host to controller
    T_OS_QUEUE    iso_output_queue;       //controller to host
#if MCP_MEDIA_CONTROL_SERVER
    T_APP_LEA_MCP_DB mcp_db;
#endif
#if CCP_CALL_CONTROL_SERVER
    T_APP_LEA_CCP_DB ccp_db;
#endif
} T_APP_DB;
/** End of APP_LEA_CAP_INI_MAIN_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_LEA_CAP_INI_MAIN_Exported_Variables App Main Variables
    * @{
    */
extern T_APP_DB app_db;

extern void *app_evt_queue_handle;  //!< Event queue handle
extern void *app_io_queue_handle;   //!< IO queue handle
extern const char device_name_le[];
/** End of APP_LEA_CAP_INI_MAIN_Exported_Variables
    * @}
    */

/** End of APP_LEA_CAP_INI_MAIN
* @}
*/

#endif

