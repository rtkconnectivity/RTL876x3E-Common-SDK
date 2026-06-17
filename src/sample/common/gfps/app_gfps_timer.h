/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_GFPS_TIMER_H_
#define _APP_GFPS_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief app_gfps_timer_init
 *
 */
void app_gfps_timer_init(void);

/**
 * @brief start update rpa timer
 *
 */
void app_gfps_timer_start_update_rpa(void);

/**
 * @brief stop update rpa timer
 *
 */
void app_gfps_timer_stop_update_rpa(void);

void app_gfps_timer_start_reset_failure_count(void);
void app_gfps_timer_start_check_receive_pair_req(void);
void app_gfps_timer_start_check_receive_passkey(void);
void app_gfps_timer_retroactively_start_check_accountkey(void);
void app_gfps_timer_start_tag_auto_reset(void);
void app_gfps_timer_start_check_reject_pair_flag(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
