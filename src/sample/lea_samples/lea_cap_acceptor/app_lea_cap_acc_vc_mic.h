/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_VC_MIC_H_
#define _APP_LEA_CAP_ACC_VC_MIC_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "gap.h"
#include "ble_audio.h"

#define MAX_VCS_OUTPUT_LEVEL              15
#define MAX_VCS_VOLUME_SETTING            255

#define APP_VCS_DEFAULT_VOL_SETTING       153
#define APP_VCS_DEFAULT_MUTE              0
#define APP_VCS_DEFAULT_CHG_CNT           0
#define APP_VCS_DEFAULT_VOL_FLAG          0
#define APP_VCS_DEFAULT_STEP_SIZE         MAX_VCS_VOLUME_SETTING/MAX_VCS_OUTPUT_LEVEL

void app_lea_acc_vc_mic_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
