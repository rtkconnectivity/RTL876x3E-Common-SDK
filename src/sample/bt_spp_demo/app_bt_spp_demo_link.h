/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _SPP_DEMO_LINK_H_
#define _SPP_DEMO_LINK_H_

#include <stdint.h>
#include <stdbool.h>
#include "os_queue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_spp_service
{
    struct t_spp_service   *next;
    uint16_t                frame_size;
    uint8_t                 credit;
    uint8_t                 local_server_chann;
} T_SPP_SERVICE;

typedef struct t_spp_demo_link
{
    struct t_spp_demo_link *next;
    uint8_t                 bd_addr[6];
    T_OS_QUEUE              service_list;
} T_SPP_DEMO_LINK;

T_SPP_SERVICE *spp_demo_find_service(uint8_t bd_addr[6],
                                     uint8_t local_server_chann);

T_SPP_SERVICE *spp_demo_alloc_service(uint8_t *bd_addr, uint8_t local_server_chann);

void spp_demo_free_service(uint8_t *bd_addr, T_SPP_SERVICE *service);

T_SPP_DEMO_LINK *spp_demo_find_link(uint8_t *bd_addr);

T_SPP_DEMO_LINK *spp_demo_alloc_link(uint8_t *bd_addr);

void spp_demo_free_link(T_SPP_DEMO_LINK *link);

void spp_demo_int(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SPP_DEMO_LINK_H_ */
