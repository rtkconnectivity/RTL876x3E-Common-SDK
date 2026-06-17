/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "trace.h"
#include "bt_types.h"
#include "os_queue.h"
#include "app_bt_spp_demo_link.h"

typedef struct t_spp_demo_app_db
{
    T_OS_QUEUE link_list;
} T_SPP_DEMO_APP_DB;

static T_SPP_DEMO_APP_DB *app_db;

T_SPP_SERVICE *spp_demo_find_service(uint8_t bd_addr[6],
                                     uint8_t local_server_chann)
{
    T_SPP_DEMO_LINK *link;
    T_SPP_SERVICE   *service = NULL;

    link = os_queue_peek(&app_db->link_list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6))
        {
            service = os_queue_peek(&link->service_list, 0);
            while (service != NULL)
            {
                if (service->local_server_chann == local_server_chann)
                {
                    return service;
                }

                service = service->next;
            }
        }

        link = link->next;
    }

    return service;
}

T_SPP_SERVICE *spp_demo_alloc_service(uint8_t *bd_addr, uint8_t local_server_chann)
{
    T_SPP_DEMO_LINK *link;
    T_SPP_SERVICE   *service = NULL;

    link = spp_demo_find_link(bd_addr);
    if (link != NULL)
    {
        service = calloc(1, (sizeof(T_SPP_SERVICE)));
        if (service != NULL)
        {
            service->local_server_chann = local_server_chann;
            os_queue_in(&link->service_list, service);
        }
    }

    return service;
}

void spp_demo_free_service(uint8_t *bd_addr, T_SPP_SERVICE *service)
{
    T_SPP_DEMO_LINK *link;

    link = spp_demo_find_link(bd_addr);
    if (link != NULL)
    {
        if (os_queue_delete(&link->service_list, service) == true)
        {
            free(service);
        }
    }
}

T_SPP_DEMO_LINK *spp_demo_find_link(uint8_t *bd_addr)
{
    T_SPP_DEMO_LINK *link;

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

T_SPP_DEMO_LINK *spp_demo_alloc_link(uint8_t *bd_addr)
{
    T_SPP_DEMO_LINK *link;

    link = calloc(1, sizeof(T_SPP_DEMO_LINK));
    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_in(&app_db->link_list, link);
    }

    return link;
}

void spp_demo_free_link(T_SPP_DEMO_LINK *link)
{
    T_SPP_SERVICE *service;

    service = os_queue_out(&link->service_list);
    while (service != NULL)
    {
        spp_demo_free_service(link->bd_addr, service);
        service = os_queue_out(&link->service_list);
    }

    if (os_queue_delete(&app_db->link_list, link) == true)
    {
        free(link);
    }
}

void spp_demo_int(void)
{
    app_db = calloc(1, sizeof(T_SPP_DEMO_APP_DB));
}
