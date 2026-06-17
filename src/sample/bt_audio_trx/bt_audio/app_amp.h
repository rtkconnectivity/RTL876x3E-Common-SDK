/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AMP_H_
#define _APP_AMP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool app_amp_is_off_state(void);
void app_amp_cfg_init(void);
bool app_amp_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_AMP_H_ */
