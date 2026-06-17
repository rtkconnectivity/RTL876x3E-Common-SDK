/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "contrib/apps/ping/ping.h"
#include "os_sched.h"

static struct
{
    char server_name[40];
} mgr =
{
    .server_name = {0}
};


void app_bt_pan_set_ping(char *server_name, uint32_t server_name_len)
{
    strncpy(mgr.server_name, server_name, sizeof(mgr.server_name) - 1);

}


static void ping_task(void *thread_param)
{
    struct hostent *host = NULL;
    host = gethostbyname(mgr.server_name);

    LWIP_PLATFORM_DIAG(("ping_task: get host by name %s",
                        ipaddr_ntoa((ip_addr_t *)host->h_addr_list[0])));

    ping_init((ip_addr_t *)host->h_addr_list[0]);
    ping_send_now();

    while (1)
    {
        os_delay(10);
    };
}


bool app_bt_pan_ping(uint8_t *bd_addr)
{
    sys_thread_new("sem", ping_task, NULL, 2048, 0);

    return true;
}




