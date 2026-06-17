/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_GUI_BT_POLICY__
#define __APP_GUI_BT_POLICY__



#ifdef __cplusplus
extern "C" {
#endif
#include "btm.h"
void app_gui_bt_policy_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len);

#ifdef __cplusplus
}
#endif

#endif
