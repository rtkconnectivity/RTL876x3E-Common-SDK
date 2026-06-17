/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "audio.h"
#include "audio_type.h"
#include "audio_passthrough.h"
#include "app_audio_demo_passthrough.h"

static T_APT_SM_STATE apt_state = APT_STOPPED;

void app_audio_passthrough_init(void)
{
    audio_passthrough_create(48000);
}

bool app_audio_normal_apt_enable(void)
{
    if (apt_state != APT_STARTED)
    {
        apt_state = APT_STARTING;
        return audio_passthrough_enable(AUDIO_PASSTHROUGH_MODE_NORMAL, 0);
    }
    return false;
}

bool app_audio_llapt_enable(uint8_t llapt_scenario_id)
{
    if (apt_state != APT_STARTED)
    {
        apt_state = APT_STARTING;
        return audio_passthrough_enable(AUDIO_PASSTHROUGH_MODE_LOW_LATENCY, llapt_scenario_id);
    }
    return false;
}

bool app_audio_normal_apt_disable(void)
{
    if (apt_state != APT_STOPPED)
    {
        apt_state = APT_STOPPING;
        return audio_passthrough_disable();
    }
    return false;
}

bool app_audio_llapt_disable(void)
{
    if (apt_state != APT_STOPPED)
    {
        apt_state = APT_STOPPING;
        return audio_passthrough_disable();
    }
    return false;
}
