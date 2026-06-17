/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_ADV_H_
#define _APP_LEA_CAP_ACC_ADV_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/** @brief  Default minimum advertising interval when device is discoverable (units of 625us, 160=100ms) */
#define DEFAULT_ADVERTISING_INTERVAL_MIN            0x20
/** @brief  Default Maximum advertising interval */
#define DEFAULT_ADVERTISING_INTERVAL_MAX            0x30

#define LE_EXT_ADV_PSRI       0x0001
#define LE_EXT_ADV_ASCS       0x0002
#define LE_EXT_ADV_BASS       0x0004
#define LE_EXT_ADV_CAP_GEN    0x0008
#define LE_EXT_ADV_CAP_TARGET 0x0010

void app_ext_adv_adv_cfg(uint16_t flag);
bool app_ext_adv_params_init(void);

void app_ext_adv_update(void);
T_GAP_CAUSE app_ext_adv_start(void);
T_GAP_CAUSE app_ext_adv_stop(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
