/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#if F_GFPS_COMMON_BASIC_FEATURE_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "os_mem.h"
#include "trace.h"
#include "app_main.h"
#include "app_gfps_cfg.h"
#include "gap_conn_le.h"
#include "app_gfps_link_util.h"

T_APP_GFPS_LINK_DB app_gfps_link_db;

bool app_gfps_ble_disconnect(T_APP_GFPS_LE_LINK *p_link, T_GFPS_LE_LOCAL_DISC_CAUSE disc_cause)
{
    if (p_link != NULL)
    {
        APP_PRINT_TRACE2("app_gfps_ble_disconnect: conn_id %d, disc_cause %d",
                         p_link->conn_id, disc_cause);
        if (le_disconnect(p_link->conn_id) == GAP_CAUSE_SUCCESS)
        {
            p_link->local_disc_cause = disc_cause;
            return true;
        }
    }
    return false;
}


T_APP_GFPS_LE_LINK *app_gfps_link_find_le_link_by_conn_id(uint8_t conn_id)
{
    T_APP_GFPS_LE_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < APP_GFPS_MAX_LINKS; i++)
    {
        if (app_gfps_link_db.le_link[i].used == true &&
            app_gfps_link_db.le_link[i].conn_id == conn_id)
        {
            p_link = &app_gfps_link_db.le_link[i];
            break;
        }
    }

    return p_link;
}

T_APP_GFPS_LE_LINK *app_gfps_link_find_le_link_by_addr(uint8_t *bd_addr)
{
    T_APP_GFPS_LE_LINK *p_link = NULL;
    uint8_t i;

    if (bd_addr != NULL)
    {
        for (i = 0; i < APP_GFPS_MAX_LINKS; i++)
        {
            if (app_gfps_link_db.le_link[i].used == true &&
                !memcmp(app_gfps_link_db.le_link[i].bd_addr, bd_addr, 6))
            {
                p_link = &app_gfps_link_db.le_link[i];
                break;
            }
        }
    }

    return p_link;
}

T_APP_GFPS_LE_LINK *app_gfps_link_alloc_le_link_by_conn_id(uint8_t conn_id)
{
    T_APP_GFPS_LE_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < APP_GFPS_MAX_LINKS; i++)
    {
        if (app_gfps_link_db.le_link[i].used == false)
        {
            p_link = &app_gfps_link_db.le_link[i];

            p_link->used    = true;
            p_link->id      = i;
            p_link->conn_id = conn_id;
            break;
        }
    }

    return p_link;
}

bool app_gfps_link_free_le_link(T_APP_GFPS_LE_LINK *p_link)
{
    if (p_link != NULL)
    {
        if (p_link->used == true)
        {
            memset(p_link, 0, sizeof(T_APP_GFPS_LE_LINK));
            p_link->conn_id = 0xFF;
#if F_GFPS_COMMON_BASIC_FEATURE_SUPPORT
            p_link->gfps_link.gfps_conn_id = 0xFF;
#endif
            return true;
        }
    }

    return false;
}

void app_gfps_link_handle_new_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                             uint16_t disc_cause)
{
    APP_PRINT_TRACE4("app_gfps_link_handle_new_conn_state_evt: conn_id %d, new_state %d, cause 0x%04x, gfps support %d",
                     conn_id, new_state, disc_cause, extend_app_cfg_const.gfps_support);
    if (extend_app_cfg_const.gfps_support)
    {
        T_APP_GFPS_LE_LINK *p_gfps_link;
        p_gfps_link = app_gfps_link_find_le_link_by_conn_id(conn_id);

        switch (new_state)
        {
        case GAP_CONN_STATE_DISCONNECTING:
            if (p_gfps_link != NULL)
            {
                p_gfps_link->state = GFPS_LE_LINK_STATE_DISCONNECTING;
            }
            break;

        case GAP_CONN_STATE_DISCONNECTED:
            if (p_gfps_link != NULL)
            {
                uint8_t local_disc_cause = p_gfps_link->local_disc_cause;
                app_gfps_link_free_le_link(p_gfps_link);
            }
            break;

        case GAP_CONN_STATE_CONNECTING:
            if (p_gfps_link == NULL)
            {
                p_gfps_link = app_gfps_link_alloc_le_link_by_conn_id(conn_id);
                if (p_gfps_link != NULL)
                {
                    p_gfps_link->state = GFPS_LE_LINK_STATE_CONNECTING;
                }
            }
            break;

        case GAP_CONN_STATE_CONNECTED:
            if (p_gfps_link != NULL)
            {
                p_gfps_link->conn_handle = le_get_conn_handle(conn_id);

                if (p_gfps_link->state == GFPS_LE_LINK_STATE_CONNECTING)
                {
                    p_gfps_link->state = GFPS_LE_LINK_STATE_CONNECTED;
                }

#if F_GFPS_COMMON_LE_DEVICE_SUPPORT
                if (extend_app_cfg_const.gfps_le_device_support)
                {
                    app_gfps_linkback_info_init(conn_id);
                }
#endif
            }
            break;

        default:
            break;
        }
    }
}
#endif
