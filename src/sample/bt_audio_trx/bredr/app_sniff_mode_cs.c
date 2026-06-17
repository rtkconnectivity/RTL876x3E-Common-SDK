/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"

#include "app_link_util_cs.h"
#include "app_sniff_mode_cs.h"
#include "app_main.h"

#if F_APP_DATA_CAPTURE_SUPPORT
#include "app_data_capture_cs.h"
#endif

typedef enum
{
    SNIFF_MODE_REMOTE_MSG_ROLESWAP_INFO = 0,
} T_SNIFF_MODE_REMOTE_MSG;

typedef struct
{
    uint16_t b2b_link_disable_flag;
    uint16_t b2s_disable_all_flag;
    uint16_t b2s_link_disable_flag;
} T_ROLESWAP_INFO;

static uint16_t g_b2s_disable_all_flag;
static bool g_have_disable_all;


void app_sniff_mode_startup(void)
{
    g_b2s_disable_all_flag = 0;
    g_have_disable_all = false;
}

void app_sniff_mode_b2s_disable(uint8_t *bd_addr, uint16_t flag)
{
    T_APP_BR_LINK *p_link;

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        p_link = app_link_find_br_link(bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_TRACE3("app_sniff_mode_b2s_disable: p_link 0x%p, curr flag 0x%04x, set flag 0x%04x",
                             p_link, p_link->sniff_mode_disable_flag, flag);

            p_link->sniff_mode_disable_flag |= flag;

            bt_sniff_mode_disable(bd_addr);
        }
    }
}

void app_sniff_mode_b2s_enable(uint8_t *bd_addr, uint16_t flag)
{
    T_APP_BR_LINK *p_link;

#if F_APP_DATA_CAPTURE_SUPPORT
    if (app_data_capture_executing_check())
    {
        return;
    }
#endif

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        p_link = app_link_find_br_link(bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_TRACE3("app_sniff_mode_b2s_enable: p_link 0x%p, curr flag 0x%04x, clear flag 0x%04x",
                             p_link, p_link->sniff_mode_disable_flag, flag);

            p_link->sniff_mode_disable_flag &= ~flag;

            if (0 == g_b2s_disable_all_flag &&
                0 == p_link->sniff_mode_disable_flag)
            {
                APP_PRINT_TRACE0("app_sniff_mode_b2s_enable: bt_sniff_mode_enable");

                bt_sniff_mode_enable(bd_addr, 0, 0, 0, 0);
            }
        }
    }
}

void app_sniff_mode_b2s_disable_all(uint16_t flag)
{
    uint8_t app_idx;

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        APP_PRINT_TRACE2("app_sniff_mode_b2s_disable_all: curr flag 0x%04x, set flag 0x%04x",
                         g_b2s_disable_all_flag, flag);

        g_b2s_disable_all_flag |= flag;

        for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
        {
            if (app_db.br_link[app_idx].used)
            {
                bt_sniff_mode_disable(app_db.br_link[app_idx].bd_addr);
            }
        }
    }
}

void app_sniff_mode_b2s_enable_all(uint16_t flag)
{
    T_APP_BR_LINK *p_link;
    uint8_t app_idx;

#if F_APP_DATA_CAPTURE_SUPPORT
    if (app_data_capture_executing_check())
    {
        return;
    }
#endif

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        APP_PRINT_TRACE2("app_sniff_mode_b2s_enable_all: curr flag 0x%04x, set flag 0x%04x",
                         g_b2s_disable_all_flag, flag);

        g_b2s_disable_all_flag &= ~flag;

        if (0 == g_b2s_disable_all_flag)
        {
            for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
            {
                if (app_link_check_b2s_link_by_id(app_idx))
                {
                    p_link = &app_db.br_link[app_idx];

                    if (0 == p_link->sniff_mode_disable_flag)
                    {
                        APP_PRINT_TRACE0("app_sniff_mode_b2s_enable_all: bt_sniff_mode_enable");

                        bt_sniff_mode_enable(p_link->bd_addr, 0, 0, 0, 0);
                    }
                }
            }
        }
    }
}

void app_sniff_mode_b2s_check_left_flag_when_disconnect(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    if (!g_have_disable_all)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                if (0 != p_link->sniff_mode_disable_flag)
                {
                    APP_PRINT_TRACE2("app_sniff_mode_b2s_check_left_flag_when_disconnect: p_link 0x%p, curr flag 0x%04x",
                                     p_link, p_link->sniff_mode_disable_flag);

                    p_link->sniff_mode_disable_flag = 0;
                }
            }
        }
    }
}

void app_sniff_mode_b2b_disable(uint8_t *bd_addr, uint16_t flag)
{
    T_APP_BR_LINK *p_link;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        p_link = app_link_find_br_link(bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_TRACE3("app_sniff_mode_b2b_disable: p_link 0x%p, curr flag 0x%04x, set flag 0x%04x",
                             p_link, p_link->sniff_mode_disable_flag, flag);

            p_link->sniff_mode_disable_flag |= flag;

            bt_sniff_mode_disable(bd_addr);
        }
    }
}

void app_sniff_mode_b2b_enable(uint8_t *bd_addr, uint16_t flag)
{
    T_APP_BR_LINK *p_link;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        p_link = app_link_find_br_link(bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_TRACE3("app_sniff_mode_b2b_enable: p_link 0x%p, curr flag 0x%04x, clear flag 0x%04x",
                             p_link, p_link->sniff_mode_disable_flag, flag);

            p_link->sniff_mode_disable_flag &= ~flag;
        }
    }
}

void app_sniff_mode_disable_all(void)
{
    uint8_t app_idx;

    APP_PRINT_TRACE0("app_sniff_mode_disable_all");

    g_have_disable_all = true;

    for (app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
    {
        if (app_db.br_link[app_idx].used)
        {
            bt_sniff_mode_disable(app_db.br_link[app_idx].bd_addr);
        }
    }
}

