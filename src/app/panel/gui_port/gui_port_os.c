/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "stdlib.h"
#include "guidef.h"
#include "gui_port.h"
#include "gui_api.h"
#include "wdg.h"

#include "os_timer.h"
#include <os_msg.h>
#include <os_task.h>
#include <os_mem.h>
#include "platform_utils.h"
#include "trace.h"
#include "stdarg.h"
#include "gui_api.h"
#include "trace.h"
#include "gui_server.h"

void *port_thread_create(const char *name, void (*entry)(void *param), void *param,
                         uint32_t stack_size, uint8_t priority)
{
    void *handle = NULL;
    if (os_task_create(&handle, name, entry, 0, stack_size, 1))
    {
        return handle;
    }
    else
    {
        return NULL;
    }
}
bool port_thread_delete(void *handle)
{
    return os_task_delete(handle);
}

bool port_thread_suspend(void *handle)
{
    return os_task_suspend(handle);
}

bool port_thread_resume(void *handle)
{
    return os_task_resume(handle);
}

bool port_thread_mdelay(uint32_t ms)
{
    platform_delay_ms(ms);
    // os_delay(ms);  //if open, when tab was slid, IDLE stack will overflow
    return true;
}

uint32_t port_thread_ms_get(void)
{
    return sys_timestamp_get();
}

uint32_t port_thread_us_get(void)
{
    return sys_timestamp_get_us();
}

bool port_mq_recv(void *handle, void *buffer, uint32_t size, uint32_t timeout)
{
    return os_msg_recv(handle, buffer, timeout);
}


bool port_mq_send(void *handle, void *buffer, uint32_t size, uint32_t timeout)
{
    if (os_msg_send(handle, (gui_msg_t *)buffer, timeout) == false)
    {
        APP_PRINT_ERROR2("port_mq_send:  send msg fail, line = %d, handle 0x%x", __LINE__, handle);
        return false;
    }
    return true;
}

bool port_mq_create(void *handle, const char *name, uint32_t msg_size, uint32_t max_msgs)
{
    if (os_msg_queue_create(handle, name, max_msgs, msg_size))
    {
        return true;
    }
    else
    {
        APP_PRINT_ERROR0("port_mq_create fail");
        return false;
    }
}

bool port_thread_ctx_malloc(uint32_t size)
{
#if 0 //renee removed for not support in RTL87x3E
    return os_alloc_secure_ctx(size);
#else
    return NULL;
#endif
}

void *port_malloc(uint32_t n)
{
    return malloc(n);
}

void port_free(void *rmem)
{
    free(rmem);
}

void *port_realloc(void *ptr, uint32_t n)
{
    void *p = malloc(n);
    if (ptr)
    {
        memcpy(p, ptr, n);
        free(ptr);
    }
    return p;
}

static void port_log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buf[256];
    vsnprintf(buf, sizeof(buf), format, args);

    APP_PRINT_INFO1("[GUI MODULE]%s", TRACE_STRING(buf));
    //DBG_DIRECT("[GUI MODULE]%s", buf);

    va_end(args);
}

extern void port_gui_lcd_power_off(void);
static void port_gui_task_sleep(void)
{
    port_gui_lcd_power_off();
}

static struct gui_os_api os_api =
{
    .name = "rtk_osif",
    .thread_create = port_thread_create,
    .thread_delete = port_thread_delete,
    .thread_suspend = port_thread_suspend,
    .thread_resume = port_thread_resume,
    .thread_mdelay = port_thread_mdelay,

    .thread_ms_get = port_thread_ms_get,
    .thread_us_get = port_thread_us_get,

    .mq_create = port_mq_create,
    .mq_send = port_mq_send,
    .mq_recv = port_mq_recv,

    .f_malloc = port_malloc,
    .f_free = port_free,
    .f_realloc = port_realloc,

    .gui_sleep_cb = port_gui_task_sleep,

    .log = port_log,
};

void gui_port_os_add_exe_to_gui_task()
{
    //APP_PRINT_INFO0("gui_port_os_add_exe_to_gui_task: feed watchdog");
    wdg_kick();
}

void gui_port_os_init(void)
{
    gui_os_api_register(&os_api);
    gui_task_ext_execution_sethook(gui_port_os_add_exe_to_gui_task);
}

