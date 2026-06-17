/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _MAP_DEMO_CONSOLE_H_
#define _MAP_DEMO_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_MAP_DEMO_CONSOLE MAP Demo Console
* @brief MAP Demo command
* @{
*/

typedef enum
{
    GAP_LEGACY_ID         = 0x0001,
    MAP_ID               = 0x0002,
} T_MAP_DEMO_CMD_ID;

#define MAP_DEMO_ACTION_CONNECT             0x01
#define MAP_DEMO_ACTION_DISCONNECT          0x02
#define MAP_DEMO_ACTION_MNS_ON              0x03
#define MAP_DEMO_ACTION_MNS_OFF             0x04
#define MAP_DEMO_ACTION_SET_FOLDER          0x05
#define MAP_DEMO_ACTION_GET_FOLDER_LISTING  0x06
#define MAP_DEMO_ACTION_GET_MSG_LISTING     0x07
#define MAP_DEMO_ACTION_GET_MSG             0x08

void app_map_demo_cmd_register(void);

/** @} End of APP_MAP_DEMO_CONSOLE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MAP_DEMO_CONSOLE_H_ */
