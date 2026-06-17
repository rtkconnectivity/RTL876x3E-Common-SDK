/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "anc.h"
#include "app_audio_demo_anc.h"

static T_ANC_SM_STATE anc_state = ANC_STOPPED;

bool app_audio_anc_enable(uint8_t scenario_id)
{
    if (anc_state != ANC_STARTED)
    {
        anc_state = ANC_STARTING;
        return anc_enable(scenario_id);
    }
    return false;
}

bool app_audio_anc_disable(void)
{
    if (anc_state != ANC_STOPPED)
    {
        anc_state = ANC_STOPPING;
        return anc_disable();
    }
    return false;
}
