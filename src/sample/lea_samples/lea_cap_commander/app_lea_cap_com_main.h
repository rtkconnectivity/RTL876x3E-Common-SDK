/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_COM_MAIN_H_
#define _APP_LEA_CAP_COM_MAIN_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "csis_def.h"
#include "app_lea_cap_com_link.h"
#include "os_queue.h"

/** @defgroup APP_LEA_CAP_COM_MAIN BLE Audio CAP Commander Main
  * @brief Main file to initialize hardware and BT stack and start task scheduling
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_LEA_CAP_COM_MAIN_Exported_Macros App Main Macros
    * @{
    */

/** End of APP_LEA_CAP_COM_MAIN_Exported_Macros
    * @}
    */
/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_LEA_CAP_COM_MAIN_Exported_Types App Main Types
    * @{
    */

/**  @brief  App define global app data structure */
typedef struct
{
    T_APP_LE_LINK le_link[APP_MAX_BLE_LINK_NUM];
    T_OS_QUEUE    conn_dev_queue;
    T_OS_QUEUE    group_handle_queue;
    T_OS_QUEUE    scan_dev_queue;
#if BAP_BROADCAST_ASSISTANT
    T_OS_QUEUE    sync_handle_queue;
    T_OS_QUEUE    scan_baas_dev_queue;
#endif
} T_APP_DB;
/** End of APP_LEA_CAP_COM_MAIN_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_LEA_CAP_COM_MAIN_Exported_Variables App Main Variables
    * @{
    */
extern T_APP_DB app_db;

extern void *app_evt_queue_handle;  //!< Event queue handle
extern void *app_io_queue_handle;   //!< IO queue handle
/** End of APP_LEA_CAP_COM_MAIN_Exported_Variables
    * @}
    */

/** End of APP_LEA_CAP_COM_MAIN
* @}
*/

#endif

