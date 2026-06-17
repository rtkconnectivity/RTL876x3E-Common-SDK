/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _SPP_DEMO_CONSOLE_H_
#define _SPP_DEMO_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    GAP_ID = 0x0001,
    SPP_ID = 0x0002,
} T_SPP_DEMO_CMD_ID;

#define SPP_DEMO_ACTION_CONNECT             0x01
#define SPP_DEMO_ACTION_DISCONNECT          0x02
#define SPP_DEMO_ACTION_DISCONNECT_ALL      0x03
#define SPP_DEMO_ACTION_SEND_DATA           0x04

void app_spp_demo_cmd_register(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
