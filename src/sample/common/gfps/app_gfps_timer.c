/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_GFPS_COMMON_BASIC_FEATURE_SUPPORT
#include "app_timer.h"
#include "trace.h"
#include "string.h"
#include "app_gfps.h"
#include "app_gfps_timer.h"
#include "app_gfps_cfg.h"
#include "app_gfps_device.h"
#include "app_gfps_finder.h"

uint8_t app_gfps_timer_id;
uint8_t timer_idx_update_rpa;
uint8_t timer_idx_reset_failure_count;
uint8_t timer_idx_check_receive_pair_req;
uint8_t timer_idx_check_receive_passkey;
uint8_t timer_idx_retroactively_check_accountkey;
uint8_t timer_idx_tag_auto_reset;
uint8_t timer_idx_clear_reject_pair_flag;

uint32_t app_gfps_timer_timeout_ms = 0xDBBA0;//15min
uint32_t app_gfps_timer_timeout_ms_reset_failure_count = 300 * 1000; //5min
uint32_t app_gfps_timer_timeout_ms_check_receive_pair_req = 10 * 1000; //10s
uint32_t app_gfps_timer_timeout_ms_check_receive_passkey = 10 * 1000;
uint32_t app_gfps_timer_timeout_ms_check_accountkey = 60 * 1000;
uint32_t app_gfps_timer_timeout_ms_tag_auto_reset = 300 * 1000; //5min
uint32_t app_gfps_timer_timeout_ms_clear_reject_pair_flag = 30 * 1000; //30s

typedef enum
{
    APP_GFPS_TIMER_UPDATE_RPA                     = 0x00,
    APP_GFPS_TIMER_RESET_FAILURE_COUNT            = 0x01,
    APP_GFPS_TIMER_CHECK_RECEIVE_PAIR_REQ         = 0x02,
    APP_GFPS_TIMER_CHECK_RECEIVE_PASSKEY          = 0x03,
    APP_GFPS_TIMER_RETROACTIVELY_CHECK_ACCOUNTKEY = 0x04,
    APP_GFPS_TIMER_TAG_AUTO_RESET                 = 0x05,
    APP_GFPS_TIMER_CLEAR_REJECT_PAIR_FLAG         = 0x06,

    APP_GFPS_TIMER_TOTAL,
} T_APP_GFPS_TIMER;

void app_gfps_timer_start_check_reject_pair_flag(void)
{
    app_start_timer(&timer_idx_clear_reject_pair_flag,
                    "app_gfps_timer_clear_reject_pair_flag",
                    app_gfps_timer_id, APP_GFPS_TIMER_CLEAR_REJECT_PAIR_FLAG, 0,
                    false, app_gfps_timer_timeout_ms_clear_reject_pair_flag);
}

void app_gfps_timer_start_tag_auto_reset(void)
{
    app_start_timer(&timer_idx_tag_auto_reset,
                    "app_gfps_timer_tag_auto_reset",
                    app_gfps_timer_id, APP_GFPS_TIMER_TAG_AUTO_RESET, 0,
                    false, app_gfps_timer_timeout_ms_tag_auto_reset);
}

void app_gfps_timer_retroactively_start_check_accountkey(void)
{
    app_start_timer(&timer_idx_retroactively_check_accountkey,
                    "app_gfps_timer_retroactively_check_accountkey",
                    app_gfps_timer_id, APP_GFPS_TIMER_RETROACTIVELY_CHECK_ACCOUNTKEY, 0,
                    false, app_gfps_timer_timeout_ms_check_accountkey);
}

void app_gfps_timer_start_check_receive_passkey(void)
{
    app_start_timer(&timer_idx_check_receive_passkey, "app_gfps_timer_check_receive_passkey",
                    app_gfps_timer_id, APP_GFPS_TIMER_CHECK_RECEIVE_PASSKEY, 0,
                    false, app_gfps_timer_timeout_ms_check_receive_passkey);
}

void app_gfps_timer_start_reset_failure_count(void)
{
    app_start_timer(&timer_idx_reset_failure_count, "app_gfps_timer_reset_failure_count",
                    app_gfps_timer_id, APP_GFPS_TIMER_RESET_FAILURE_COUNT, 0,
                    false, app_gfps_timer_timeout_ms_reset_failure_count);
}

void app_gfps_timer_start_check_receive_pair_req(void)
{
    app_start_timer(&timer_idx_check_receive_pair_req, "app_gfps_timer_check_receive_pair_req",
                    app_gfps_timer_id, APP_GFPS_TIMER_CHECK_RECEIVE_PAIR_REQ, 0,
                    false, app_gfps_timer_timeout_ms_check_receive_pair_req);
}

void app_gfps_timer_start_update_rpa(void)
{
    app_start_timer(&timer_idx_update_rpa, "app_gfps_timer_update_rpa",
                    app_gfps_timer_id, APP_GFPS_TIMER_UPDATE_RPA, 0,
                    false, app_gfps_timer_timeout_ms);
}

/**
 * @brief stop update adv ei timer
 *
 */
void app_gfps_timer_stop_update_rpa(void)
{
    app_stop_timer(&timer_idx_update_rpa);
}

void app_gfps_timer_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_gfps_timer_timeout_cb: timer_id %d, timer_chann %d",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case APP_GFPS_TIMER_UPDATE_RPA:
        {
            bool gfps_generate_rpa = true;
            app_gfps_update_rpa(gfps_generate_rpa);
            app_gfps_timer_start_update_rpa();
        }
        break;

    case APP_GFPS_TIMER_RESET_FAILURE_COUNT:
        {
            app_gfps_reset_failure_count();
        }
        break;

    case APP_GFPS_TIMER_CHECK_RECEIVE_PAIR_REQ:
        {
            app_gfps_check_receive_pair_req();
        }
        break;

    case APP_GFPS_TIMER_CHECK_RECEIVE_PASSKEY:
        {
            app_gfps_check_receive_passkey();
        }
        break;

    case APP_GFPS_TIMER_RETROACTIVELY_CHECK_ACCOUNTKEY:
        {
            app_gfps_reset_account_key_write_counts();
            app_gfps_set_allow_write_account_key(false);
        }
        break;

    case APP_GFPS_TIMER_TAG_AUTO_RESET:
        {
#if F_GFPS_COMMON_FINDER_SUPPORT
            /*If the Provider was paired(seeker already write account key), but FHN wasn't provisioned within 5 minutes
            the Provider should revert to its factory configuration and clear the stored account keys.*/
            if (extend_app_cfg_const.gfps_finder_support)
            {
                if (!app_gfps_finder_provisioned())
                {
                    APP_PRINT_TRACE0("app_gfps_timer_timeout_cb: not provisioned within 5 minutes, shall do factory reset");
                    app_gfps_device_handle_factory_reset();

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
                    if (extend_app_cfg_const.gfps_le_device_support)
                    {
                        app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
                    }
                    else
#endif
                    {
                        app_gfps_device_check_state((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
                    }
                }
            }
#endif
        }
        break;

    case APP_GFPS_TIMER_CLEAR_REJECT_PAIR_FLAG:
        {
            app_gfps_clear_reject_pair_flag();
            APP_PRINT_TRACE0("app_gfps_timer_timeout_cb: clear reject_pair flag");
        }
        break;

    default:
        break;
    }
}

void app_gfps_timer_init(void)
{
    app_timer_reg_cb(app_gfps_timer_timeout_cb, &app_gfps_timer_id);
}
#endif
