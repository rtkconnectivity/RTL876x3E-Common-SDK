/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_customer.h"
#include "app_customer_bt.h"
#include "app_customer_audio_policy.h"


void app_customer_init(void)
{
    app_customer_bt_init();

#if F_APP_CUSTOMER_AUDIO_POLICY_SUPPORT
    app_customer_audio_init();
#endif

}























