/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_IAP_RTK_H_
#define _APP_IAP_RTK_H_

#include <stdbool.h>
#include <stdint.h>
#include "bt_iap.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    IAP_SEND_OK             = 0x00,
    IAP_PROFILE_NOT_CONN    = 0x01,
    IAP_SESSION_NOT_CONN    = 0x02,
    IAP_RTK_LAUNCHED        = 0x03,
    IAP_SEND_FAIL           = 0x04,
} IAP_SEND_RESULT;

void app_iap_rtk_init(void);

bool app_iap_rtk_create(uint8_t *bd_addr);

bool app_iap_rtk_delete(uint8_t *bd_addr);

bool app_iap_rtk_launch(uint8_t *bd_addr, T_BT_IAP_APP_LAUNCH_METHOD method);

void app_iap_rtk_handle_remote_conn_cmpl(void);

bool app_iap_rtk_connected(uint8_t *bd_addr);

IAP_SEND_RESULT app_iap_rtk_transfer(uint8_t app_idx, uint8_t *data, uint32_t len);

bool app_iap_rtk_send(uint8_t *bd_addr, uint8_t *data, uint32_t len);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_IAP_RTK_H_ */
