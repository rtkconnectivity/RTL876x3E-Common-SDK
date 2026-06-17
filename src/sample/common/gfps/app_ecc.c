/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_GFPS_COMMON_BASIC_FEATURE_SUPPORT
#include "trace.h"
#include "app_gfps_cfg.h"
#include "ecc_enhanced.h"
#include "gfps.h"

void app_gfps_ecc_handle_msg()
{
    if (!extend_app_cfg_const.gfps_support)
    {
        return;
    }

    T_ECC_CAUSE ecc_cause = ecc_sub_proc();

    if (ecc_cause == ECC_CAUSE_SUCCESS)
    {
        gfps_handle_pending_char_kbp();
    }
};
#endif
