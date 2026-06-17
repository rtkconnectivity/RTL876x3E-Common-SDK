/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_ble_rand_addr_mgr.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "gap_le.h"
#include "trace.h"
#include "os_queue.h"
#include <string.h>
#include <stdlib.h>


typedef enum
{
    SYNC_STATIC_ADDR = 0,
    REMOTE_MSG_MAX = 1,
} RELAY_MSG;


typedef struct CB_ELEM CB_ELEM;


typedef struct CB_ELEM
{
    CB_ELEM *next;
    RANDOM_ADDR_MGR_CB cb;
} CB_ELEM;


struct
{
    T_OS_QUEUE cb_queue;
} addr_mgr
=
{
    .cb_queue = {0}
};

#if 0
static void go_through_cbs(RANDOM_ADDR_MGR_EVT_DATA *data)
{
    CB_ELEM *tmp = (CB_ELEM *)addr_mgr.cb_queue.p_first;

    for (; tmp != NULL; tmp = tmp->next)
    {
        tmp->cb(data);
    }
}
#endif

void app_ble_rand_addr_handle_role_decieded(void)
{

}

void app_ble_rand_addr_get(uint8_t random_addr[6])
{
    memcpy(random_addr, app_cfg_nv.le_single_random_addr, 6);
}
void app_ble_rand_addr_set(uint8_t random_addr[6])
{
    memcpy(app_cfg_nv.le_single_random_addr, random_addr, 6);
    app_cfg_store(app_cfg_nv.le_single_random_addr, 6);
    APP_PRINT_TRACE1("app_ble_rand_addr_set: addr %s", TRACE_BDADDR(random_addr));
}

void app_ble_rand_addr_cb_reg(RANDOM_ADDR_MGR_CB cb)
{
    CB_ELEM *p_ele = NULL;
    p_ele = calloc(1, sizeof(*p_ele));
    if (p_ele)
    {
        p_ele->cb = cb;
        os_queue_in(&addr_mgr.cb_queue, p_ele);
    }
}

void app_ble_rand_addr_init(void)
{
    if ((app_cfg_nv.le_single_random_addr[5] & 0xC0) != 0xC0)
    {
        le_gen_rand_addr(GAP_RAND_ADDR_STATIC, app_cfg_nv.le_single_random_addr);
        app_cfg_store(&app_cfg_nv.le_single_random_addr, 6);
        APP_PRINT_TRACE1("app_ble_rand_addr_init: le_single_random_addr %s",
                         TRACE_BDADDR(app_cfg_nv.le_single_random_addr));
    }

    os_queue_init(&addr_mgr.cb_queue);

}


