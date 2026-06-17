/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/

#include "trace.h"
#include "os_sched.h"
#include "app_panel_init.h"
#include "app_io_resource_init.h"
#include <string.h>

#if (SUPPORT_ACCESS_SHM == 1)
void shm_data_copy(void)
{
    extern unsigned int Load$$SHARE_RAM_DATA$$RW$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RW$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RW$$Length;
    extern unsigned int Load$$SHARE_RAM_DATA$$ZI$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$ZI$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$ZI$$Length;

    uint32_t load_addr = (uint32_t)&Load$$SHARE_RAM_DATA$$RW$$Base;
    uint32_t dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$RW$$Base;
    uint32_t len = (uint32_t)&Image$$SHARE_RAM_DATA$$RW$$Length;
    memcpy((uint8_t *)dest_addr, (uint8_t *)load_addr, len);

    dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$ZI$$Base;
    len = (uint32_t)&Image$$SHARE_RAM_DATA$$ZI$$Length;
    memset((uint8_t *)dest_addr, 0, len);

    extern unsigned int Load$$SHARE_RAM_DATA$$RO$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RO$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RO$$Length;

    load_addr = (uint32_t)&Load$$SHARE_RAM_DATA$$RO$$Base;
    dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$RO$$Base;
    len = (uint32_t)&Image$$SHARE_RAM_DATA$$RO$$Length;
    memcpy((uint8_t *)dest_addr, (uint8_t *)load_addr, len);
}
#endif

int main(void)
{
    DBG_DIRECT("Dashboard demo");
    DBG_DIRECT("APP COMPILE TIME: [%s - %s]", __DATE__, __TIME__);

    app_io_resource_request_init();

    app_system_clock_init();
    app_components_init();
    app_task_init();

    /* Start scheduler. */
    os_sched_start();
}
/*-----------------------------------------------------------*/
