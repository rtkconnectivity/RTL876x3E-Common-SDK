/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _BT_AUDIO_CONSOLE_H_
#define _BT_AUDIO_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_AUDIO_CONSOLE BT Audio Console
* @brief BT Audio command
* @{
*/

typedef enum
{
    GAP_LEGACY_ID         = 0x0001,
    PBAP_ID               = 0x0002,
} T_BT_AUDIO_CMD_ID;

#define APP_ACTION_PBAP_CONNECT              0x01
#define APP_ACTION_PBAP_DISCONNECT           0x02
#define APP_ACTION_PBAP_SET_PHONE_BOOK       0x03
#define APP_ACTION_PBAP_PULL_CALLER_ID_NAME  0x04

void app_pbap_cmd_register(void);

/** @} End of APP_BT_AUDIO_CONSOLE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_AUDIO_CONSOLE_H_ */
