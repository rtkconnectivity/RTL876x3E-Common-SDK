/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_HFP_TYPES_H_
#define _APP_HFP_TYPES_H_

#include "bt_hfp.h"
#include "app_cmd.h"
#include "app_hfp_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_HFP App Hfp
  * @brief this file handle hfp profile related process
  * @{
  */

typedef enum
{
    APP_REMOTE_HFP_SYNC_SCO_GAIN_WHEN_CALL_ACTIVE,
    APP_REMOTE_HFP_AUTO_ANSWER_CALL_TIMER,

    APP_REMOTE_HFP_TOTAL,
} T_APP_HFP_REMOTE_MSG;

/**  @brief  caller id type
  */
typedef enum
{
    CALLER_ID_NUMBER = 0x00,
    CALLER_ID_NAME = 0x01
} T_CALLER_ID_TYPE;

typedef enum
{
    APP_SCO_ADJUST_ACL_CONN_IND_EVT = 0x00,
    APP_SCO_ADJUST_ACL_CONN_END_EVT = 0x01,
    APP_SCO_ADJUST_LINKBACK_EVT = 0x02,
    APP_SCO_ADJUST_LINKBACK_END_EVT = 0x03,
    APP_SCO_ADJUST_SCO_CONN_IND_EVT = 0x04,
    APP_SCO_ADJUST_SCO_CONN_CMPL_EVT = 0x05,

    APP_SCO_ADJUST_EVT_TOTAL,
} T_APP_HFP_SCO_ADJUST_EVT;

typedef enum t_app_hfp_call_status
{
    APP_HFP_CALL_IDLE                              = 0x00,
    APP_HFP_VOICE_ACTIVATION_ONGOING               = 0x01,
    APP_HFP_CALL_INCOMING                          = 0x02,
    APP_HFP_CALL_OUTGOING                          = 0x03,
    APP_HFP_CALL_ACTIVE                            = 0x04,
    APP_HFP_CALL_ACTIVE_WITH_CALL_WAITING          = 0x05,
    APP_HFP_CALL_ACTIVE_WITH_CALL_HELD             = 0x06,
    APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT   = 0x07,
    APP_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD   = 0x08,
} T_APP_HFP_CALL_STATUS;


typedef struct T_APP_BR_LINK T_APP_BR_LINK;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_HFP_H_ */
