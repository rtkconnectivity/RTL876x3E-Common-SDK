/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "app_bt_pbap_demo_link.h"

static T_PBAP_DEMO_APP_DB app_db;//APP PBPP data base.

T_PBAP_DEMO_LINK *pbap_demo_find_link(uint8_t *bd_addr)
{
    T_PBAP_DEMO_LINK *p_link = NULL;
    uint8_t        i;

    if (bd_addr != NULL)
    {
        for (i = 0; i < PBAP_DEMO_MAX_BR_LINK_NUM; i++)
        {
            if (app_db.pbap_link[i].used == true &&
                !memcmp(app_db.pbap_link[i].bd_addr, bd_addr, 6))
            {
                p_link = &app_db.pbap_link[i];
                break;
            }
        }
    }

    return p_link;
}

T_PBAP_DEMO_LINK *pbap_demo_alloc_link(uint8_t *bd_addr)
{
    T_PBAP_DEMO_LINK *p_link = NULL;
    uint8_t        i;

    if (bd_addr != NULL)
    {
        for (i = 0; i < PBAP_DEMO_MAX_BR_LINK_NUM; i++)
        {
            if (app_db.pbap_link[i].used == false)
            {
                p_link = &app_db.pbap_link[i];

                p_link->used = true;
                p_link->id   = i;
                memcpy(p_link->bd_addr, bd_addr, 6);
                break;
            }
        }
    }

    return p_link;
}

bool pbap_demo_free_link(T_PBAP_DEMO_LINK *p_link)
{
    if (p_link != NULL)
    {
        if (p_link->used == true)
        {
            memset(p_link, 0, sizeof(T_PBAP_DEMO_LINK));
            return true;
        }
    }

    return false;
}
