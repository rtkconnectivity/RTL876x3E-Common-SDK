/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_MAIN_H_
#define _APP_LEA_CAP_ACC_MAIN_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "csis_def.h"
#include "app_lea_cap_acc_link.h"
#include "os_queue.h"

/** @defgroup APP_LEA_CAP_ACC_MAIN BLE Audio CAP Acceptor Main
  * @brief Main file to initialize hardware and BT stack and start task scheduling
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_LEA_CAP_ACC_MAIN_Exported_Macros App Main Macros
    * @{
    */
typedef enum
{
    APP_LEA_CSIS_CFG_NOT_EXIST     = 0,
    APP_LEA_CSIS_CFG_RANK_1        = 1,
    APP_LEA_CSIS_CFG_RANK_2        = 2,
} T_APP_LEA_CSIS_CFG;

/** End of APP_LEA_CAP_ACC_MAIN_Exported_Macros
    * @}
    */
/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_LEA_CAP_ACC_MAIN_Exported_Types App Main Types
    * @{
    */

/**  @brief  App define global app data structure */
typedef struct
{
    T_APP_LEA_CSIS_CFG csis_cfg;
    T_APP_LE_LINK le_link[APP_MAX_BLE_LINK_NUM];
    uint16_t lea_sink_available_contexts;
    uint16_t lea_source_available_contexts;
    uint16_t lea_sink_supported_contexts;
    uint16_t lea_source_supported_contexts;
    uint32_t lea_sink_audio_location;
    uint32_t lea_source_audio_location;
    uint8_t  lea_csis_sirk[CSIS_SIRK_LEN];
    T_OS_QUEUE iso_input_queue;        //host to controller
    T_OS_QUEUE iso_output_queue;       //controller to host
#if (BAP_BROADCAST_SINK || BAP_SCAN_DELEGATOR)
    T_OS_QUEUE sync_handle_queue;
    T_OS_QUEUE scan_baas_dev_queue;
#endif
} T_APP_DB;
/** End of APP_LEA_CAP_ACC_MAIN_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_LEA_CAP_ACC_MAIN_Exported_Variables App Main Variables
    * @{
    */
extern T_APP_DB app_db;

extern void *app_evt_queue_handle;  //!< Event queue handle
extern void *app_io_queue_handle;   //!< IO queue handle
/** End of APP_LEA_CAP_ACC_MAIN_Exported_Variables
    * @}
    */

bool bt_stack_start(void);

/** End of APP_LEA_CAP_ACC_MAIN
* @}
*/

#endif

