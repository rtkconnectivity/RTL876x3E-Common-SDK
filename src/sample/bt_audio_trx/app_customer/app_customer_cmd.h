/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CUSTOMER_CMD_H_
#define _APP_CUSTOMER_CMD_H_

#include "stdint.h"

typedef enum
{
    XM_MMI_NULL = 0x00,

    //Play Mode
    XM_MMI_MODE_LOCAL_PLAY                  = 0x01,
    XM_MMI_MODE_A2DP_SOURCE                 = 0x02,
    XM_MMI_SEAMLESS_SWITCH_LOCAL_PLAY       = 0x04,
    XM_MMI_SEAMLESS_SWITCH_A2DP_SRC         = 0x05,
    XM_MMI_ENABLE_REPORT_PLAY_TIME          = 0x06,
    XM_MMI_DISABLE_REPORT_PLAY_TIME         = 0x07,

    XM_MMI_BT_BOND_CLEAR_ALL                = 0x03,

    XM_MMI_TOTAL
} T_XM_MMI_ACTION;



typedef enum
{
    XM_TEST_START_REQ_RRAME = 0x00,
    XM_TEST_STOP_REQ_RRAME  = 0x01,
    XM_TEST_TONE            = 0x02,
    XM_TEST_MEM             = 0x03,

    XM_TEST
} T_XM_TEST_CMD;

void app_customer_handle_cmd_set(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                                 uint8_t rx_seqn, uint8_t app_idx);

void app_customer_cmd_init(void);

#endif
