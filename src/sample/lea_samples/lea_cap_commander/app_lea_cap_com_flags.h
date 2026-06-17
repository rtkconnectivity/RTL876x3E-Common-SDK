/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_COM_FLAGS_H_
#define _APP_LEA_CAP_COM_FLAGS_H_

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "ble_audio_flags.h"

/** @defgroup  APP_LEA_CAP_COM_Config CAP Commander APP Configuration
    * @brief This file is used to config app functions.
    * @{
    */
/*============================================================================*
 *                              Constants
 *============================================================================*/

/**
 * @brief Config APP LE link number
 *
 */
#define APP_MAX_BLE_LINK_NUM 2

#define GATTC_TBL_STORAGE_SUPPORT 1

#define CAP_ROLE                 CAP_COMMANDER_ROLE

//CAP role component
#define BAP_UNICAST_CLIENT       (LE_AUDIO_UNICAST_CLIENT_ROLE && 0)
#define BAP_UNICAST_SERVER       (LE_AUDIO_UNICAST_SERVER_ROLE && 0)
#define BAP_BROADCAST_SOURCE     (LE_AUDIO_BROADCAST_SOURCE_ROLE && 0)
#define BAP_BROADCAST_SINK       (LE_AUDIO_BROADCAST_SINK_ROLE && 0)
#define BAP_BROADCAST_ASSISTANT  (LE_AUDIO_BROADCAST_ASSISTANT_ROLE && 1)
#define BAP_SCAN_DELEGATOR       (LE_AUDIO_SCAN_DELEGATOR_ROLE && 0)
#define VCP_VOLUME_CONTROLLER    (LE_AUDIO_VCS_CLIENT_SUPPORT && 1)
#define VCP_VOLUME_RENDERER      (LE_AUDIO_VCS_SUPPORT && 0)
#define MICP_MIC_CONTROLLER      (LE_AUDIO_MICS_CLIENT_SUPPORT && 1)
#define MICP_MIC_DEVICE          (LE_AUDIO_MICS_SUPPORT && 0)
#define CCP_CALL_CONTROL_SERVER  (LE_AUDIO_CCP_SERVER_SUPPORT && 0)
#define CCP_CALL_CONTROL_CLIENT  (LE_AUDIO_CCP_CLIENT_SUPPORT && 0)
#define MCP_MEDIA_CONTROL_SERVER (LE_AUDIO_MCP_SERVER_SUPPORT && 0)
#define MCP_MEDIA_CONTROL_CLIENT (LE_AUDIO_MCP_CLIENT_SUPPORT && 0)
#define CSIP_SET_COORDINATOR     (LE_AUDIO_CSIS_CLIENT_SUPPORT && 1)
#define CSIP_SET_MEMBER          (LE_AUDIO_CSIS_SUPPORT && 0)

//Optional profile role supported
#define APP_LEA_VOCS_CLIENT_SUPPORT  (VCP_VOLUME_CONTROLLER && LE_AUDIO_VOCS_CLIENT_SUPPORT && 1)
#define APP_LEA_AICS_CLIENT_SUPPORT  ((VCP_VOLUME_CONTROLLER|| MICP_MIC_CONTROLLER) && LE_AUDIO_AICS_CLIENT_SUPPORT && 1)

#define APP_PA_PRINT_INFO            0
/** @} */ /* End of group APP_LEA_CAP_COM_Config */
#endif
