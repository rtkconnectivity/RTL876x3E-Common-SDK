/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "lwip/sys.h"

static sys_sem_t sem = NULL;

void sem_create(void *thread_param)
{
    uint32_t timeout = 0x5a;

    err_t err = sys_sem_new(&sem, 0);
    if (err != ERR_OK)
    {
        return;
    }

//    LWIP_PLATFORM_DIAG(("app_bt_pan_sem: sem %p, *sem %p", sem, *(uint32_t *)sem));

    timeout = sys_arch_sem_wait(&sem, 300);

    LWIP_PLATFORM_DIAG(("app_bt_pan_sem: sem timeout 1 %d", timeout));

    timeout = sys_arch_sem_wait(&sem, 0);
    LWIP_PLATFORM_DIAG(("app_bt_pan_sem: sem timeout 2 %d", timeout));


    while (1);
}


bool app_bt_pan_sem(void)
{
    sys_thread_new("sem", sem_create, NULL, 2048, 0);
    return true;
}
