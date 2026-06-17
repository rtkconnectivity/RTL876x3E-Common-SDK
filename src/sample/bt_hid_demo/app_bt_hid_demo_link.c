/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "app_bt_hid_demo_link.h"
#include "os_queue.h"

typedef struct t_app_bt_hid_demo_app_db
{
    T_OS_QUEUE link_list;
} T_APP_BT_HID_DEMO_APP_DB;

T_APP_BT_HID_DEMO_APP_DB *app_db;

T_APP_BT_HID_DEMO_LINK *app_bt_hid_demo_find_link(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *link;

    link = os_queue_peek(&app_db->link_list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

T_APP_BT_HID_DEMO_LINK *app_bt_hid_demo_alloc_link(uint8_t *bd_addr)
{
    T_APP_BT_HID_DEMO_LINK *link;

    link = calloc(1, sizeof(T_APP_BT_HID_DEMO_LINK));
    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);
        link->hid_conn_cmpl = false;
        os_queue_in(&app_db->link_list, link);
    }

    return link;
}

void app_bt_hid_demo_free_link(T_APP_BT_HID_DEMO_LINK *link)
{
    if (os_queue_delete(&app_db->link_list, link) == true)
    {
        free(link);
    }
}

void app_bt_hid_demo_db_init(void)
{
    app_db = calloc(1, sizeof(T_APP_BT_HID_DEMO_APP_DB));
}
