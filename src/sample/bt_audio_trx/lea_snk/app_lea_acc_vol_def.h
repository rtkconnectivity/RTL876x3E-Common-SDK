/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_ACC_VOL_DEF_H_
#define _APP_LEA_ACC_VOL_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "app_dsp_cfg.h"

#define MAX_VOLUME_SETTING              255
#define VOLUME_STEP_SIZE                MAX_VOLUME_SETTING / app_dsp_cfg_vol.playback_volume_max

#define VOLUME_SETTING(level)           (level * MAX_VOLUME_SETTING + \
                                         app_dsp_cfg_vol.playback_volume_max / 2) / app_dsp_cfg_vol.playback_volume_max

#define VOLUME_LEVEL(vol_setting)       (vol_setting * app_dsp_cfg_vol.playback_volume_max  + \
                                         MAX_VOLUME_SETTING / 2 ) / MAX_VOLUME_SETTING

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
