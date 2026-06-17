/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "audio.h"
#include "audio_type.h"
#include "line_in.h"
#include "app_audio_demo_line_in.h"

void app_audio_line_in_init(void)
{
    line_in_create(AUDIO_DEVICE_IN_AUX | AUDIO_DEVICE_OUT_SPK, 48000);
}

bool app_audio_line_in_start(void)
{
    bool ret = false;

    if (line_in_start())
    {
        ret = true;
    }

    return ret;
}

bool app_audio_line_in_stop(void)
{
    bool ret = false;

    if (line_in_stop())
    {
        ret = true;
    }

    return ret;
}
