/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_DULT_DEVICE_H_
#define _APP_DULT_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "app_dult_svc.h"
#include "app_gfps_cfg.h"

typedef enum
{
    DULT_RING_STATE_STOPPED,
    DULT_RING_STATE_STARTED,
} T_DULT_RING_STATE;

void app_dult_device_sound_start(T_SERVER_ID service_id, T_DULT_CALLBACK_DATA *p_callback);
void app_dult_device_sound_stop(T_SERVER_ID service_id, T_DULT_CALLBACK_DATA *p_callback);

bool app_dult_id_read_state_get(void);
void app_dult_id_read_state_enable(void);

T_DULT_RING_STATE app_dult_device_get_ring_state(void);
void app_dult_device_set_ring_state(T_DULT_RING_STATE ring_state);

void app_dult_device_init(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
