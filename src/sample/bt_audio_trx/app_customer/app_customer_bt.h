/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CUSTOMER_BT_H_
#define _APP_CUSTOMER_BT_H_

#include <stdbool.h>
#include <string.h>
#include "bt_att.h"
#include "app_cmd.h"
#include "app_att.h"

void app_customer_bt_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                                uint16_t cmd_len, uint8_t *ack_pkt);

void app_customer_bt_init(void);


void app_customer_bt_att_cb(uint8_t *bd_addr, T_BT_ATT_MSG_TYPE msg_type, void *p_msg);

void app_customer_bt_user_cfm_rpt(uint8_t bd_addr[6], bool just_works, uint32_t disp_val);
#endif
