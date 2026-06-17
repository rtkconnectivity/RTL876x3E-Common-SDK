/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt
 *
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "lwip/opt.h"
#include "lwip/arch.h"
#if !NO_SYS
#include "sys_arch.h"
#endif
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/debug.h"
#include "lwip/sys.h"
#include <string.h>
#include "trace.h"

#include "os_sched.h"
#include "os_msg.h"
#include "os_task.h"
#include "os_sync.h"

/*============================================================================*
 *                              Local Macros
 *============================================================================*/
#define SYS_THREAD_MAX  4
#define TIMEOUT_MAX     0xFFFFFFFFUL

/*============================================================================*
 *                              Local Types
 *============================================================================*/
struct sys_timeouts
{
    struct sys_timeo *next;
};

struct timeoutlist
{
    struct sys_timeouts timeouts;
    sys_thread_t tsk_hdl;
};
/*============================================================================*
 *                              Local Variables
 *============================================================================*/
int errno;
uint32_t lwip_sys_now;
//static struct timeoutlist s_timeoutlist[SYS_THREAD_MAX] = {};
//static uint16_t s_nextthread = 0;

/*============================================================================*
 *                              Functions Declaration
 *============================================================================*/

/*============================================================================*
 *                              Local Functions
 *============================================================================*/

/*============================================================================*
*                              Global Functions
*============================================================================*/
uint32_t sys_jiffies(void)
{
    lwip_sys_now = os_sys_time_get();// * 10;
    return lwip_sys_now;
}

uint32_t sys_now(void)
{
    lwip_sys_now = os_sys_time_get();// * 10;
    return lwip_sys_now;
}

void sys_init(void)
{
//    int i;

//    // Initialize the the per-thread sys_timeouts structures
//    // make sure there are no valid pids in the list
//    for (i = 0; i < SYS_THREAD_MAX; i++)
//    {
//        s_timeoutlist[i].tsk_hdl = 0;
//        s_timeoutlist[i].timeouts.next = NULL;
//    }
//    // keep track of how many threads have been created
//    s_nextthread = 0;
}

//struct sys_timeouts *sys_arch_timeouts(void)
//{
//    int i;
//    xTaskHandle tsk_hdl;
//    struct timeoutlist *tl;

//    tsk_hdl = xTaskGetCurrentTaskHandle();
//    for (i = 0; i < s_nextthread; i++)
//    {
//        tl = &(s_timeoutlist[i]);
//        if (tl->tsk_hdl == tsk_hdl)
//        {
//            return &(tl->timeouts);
//        }
//    }

//    return NULL;
//}

sys_prot_t sys_arch_protect(void)
{
    return os_lock();
}

void sys_arch_unprotect(sys_prot_t pval)
{
    os_unlock(pval);
}

#if !NO_SYS
err_t sys_sem_new(sys_sem_t *sem, uint8_t count)
{
    if (count <= 1)
    {
        os_sem_create(sem, "lwip", 0, 1);
//        if (count == 1)
//        {
//            sys_sem_signal(sem);
//        }
    }
    else
    {
        os_sem_create(sem, "lwip", count, count);
    }

#if SYS_STATS
    ++lwip_stats.sys.sem.used;
    if (lwip_stats.sys.sem.max < lwip_stats.sys.sem.used)
    {
        lwip_stats.sys.sem.max = lwip_stats.sys.sem.used;
    }
#endif

    if (*sem != SYS_SEM_NULL)
    {
        return ERR_OK;
    }
    else
    {
#if SYS_STATS
        ++lwip_stats.sys.sem.err;
#endif

        LWIP_PLATFORM_DIAG(("sys_sem_new() fail"));
        return ERR_MEM;
    }
}

void sys_sem_free(sys_sem_t *sem)
{
#if SYS_STATS
    --lwip_stats.sys.sem.used;
#endif

    os_sem_delete(*sem);
    *sem = SYS_SEM_NULL;
}

int sys_sem_valid(sys_sem_t *sem)
{
    return (*sem != SYS_SEM_NULL);
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
    *sem = SYS_SEM_NULL;
}

uint32_t sys_arch_sem_wait(sys_sem_t *sem, uint32_t timeout)
{
    uint32_t wait_timeout = 0;
    uint32_t start_timestamp = 0 ;

    if (*sem == SYS_SEM_NULL)
    {
        return SYS_ARCH_TIMEOUT;
    }

    start_timestamp = os_sys_time_get();

    if (timeout != 0)
    {
        wait_timeout = timeout;
    }
    else
    {
        wait_timeout = TIMEOUT_MAX;
    }

    if (os_sem_take(*sem, wait_timeout) == true)
    {
        return ((os_sys_time_get() - start_timestamp));
    }
    else
    {
        LWIP_PLATFORM_DIAG(("sys_arch_sem_wait() timeout"));
        return SYS_ARCH_TIMEOUT;
    }
}

void sys_sem_signal(sys_sem_t *sem)
{
    if (os_sem_give(*sem) != true)
    {
        LWIP_PLATFORM_DIAG(("sys_sem_signal() fail!\n"));
    }
}

err_t sys_mutex_new(sys_mutex_t *mutex)
{
    os_mutex_create(mutex);

    if (*mutex != SYS_MRTEX_NULL)
    {
        return ERR_OK;
    }
    else
    {
        LWIP_PLATFORM_DIAG(("sys_mutex_new() fail"));
        return ERR_MEM;
    }
}

void sys_mutex_free(sys_mutex_t *mutex)
{
    os_sem_delete(*mutex);
}

void sys_mutex_set_invalid(sys_mutex_t *mutex)
{
    *mutex = SYS_MRTEX_NULL;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
    os_sem_take(*mutex, TIMEOUT_MAX);
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
    os_sem_give(*mutex);
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize,
                            int prio)
{
    sys_thread_t handle = NULL;
    bool xReturn = true;
    xReturn = os_task_create(&handle, name, function, arg, stacksize, prio);
    if (xReturn != true)
    {
        return NULL;
    }

    return handle;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    os_msg_queue_create(mbox, "lwip", (uint32_t)size, (uint32_t)sizeof(void *));

#if SYS_STATS
    ++lwip_stats.sys.mbox.used;
    if (lwip_stats.sys.mbox.max < lwip_stats.sys.mbox.used)
    {
        lwip_stats.sys.mbox.max = lwip_stats.sys.mbox.used;
    }
#endif
    if (NULL == *mbox)
    {
        return ERR_MEM;
    }

    return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
    uint32_t msg_num = 0;
    os_msg_queue_peek(*mbox, &msg_num);

    if (msg_num)
    {
        /* Line for breakpoint.  Should never break here! */
//        portNOP();
#if SYS_STATS
        lwip_stats.sys.mbox.err++;
#endif

        // TODO notify the user of failure.
    }

    os_msg_queue_delete(*mbox);

#if SYS_STATS
    --lwip_stats.sys.mbox.used;
#endif
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
    if (*mbox == SYS_MBOX_NULL)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
    *mbox = SYS_MBOX_NULL;
}

void sys_mbox_post(sys_mbox_t *q, void *msg)
{
    while (os_msg_send(*q, &msg, TIMEOUT_MAX) != true);
}

err_t sys_mbox_trypost(sys_mbox_t *q, void *msg)
{
    if (os_msg_send(*q, &msg, 0) == true)
    {
        return ERR_OK;
    }
    else
    {
        LWIP_PLATFORM_DIAG(("sys_mbox_trypost() fail! mbox address:0x%x", q));
        return ERR_MEM;
    }
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg)
{
    if (os_msg_send(*q, &msg, 0) == true)
    {
        return ERR_OK;
    }
    else
    {
        LWIP_PLATFORM_DIAG(("sys_mbox_trypost_fromisr() fail"));
        return ERR_MEM;
    }
//  uint32_t ulReturn;
//  err_t err = ERR_MEM;
//  BaseType_t pxHigherPriorityTaskWoken;

//  ulReturn = taskENTER_CRITICAL_FROM_ISR();

//  if (xQueueSendFromISR(*q,&msg,&pxHigherPriorityTaskWoken)==pdPASS)
//  {
//      err = ERR_OK;
//  }

//  portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
//  taskEXIT_CRITICAL_FROM_ISR( ulReturn );

//  return err;
}

uint32_t sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, uint32_t timeout)
{
    void *dummyptr;
    uint32_t wait_timeout = 0;
    uint32_t start_stamp = 0 ;

    if (msg == NULL)
    {
        msg = &dummyptr;
    }

    start_stamp = sys_now();

    if (timeout != 0)
    {
        wait_timeout = timeout;
    }
    else
    {
        wait_timeout = TIMEOUT_MAX;
    }

    if (os_msg_recv(*q, &(*msg), wait_timeout) == true)
    {
        return ((sys_now() - start_stamp));
    }
    else
    {
        *msg = NULL;
//      LWIP_PLATFORM_DIAG(("[sys_arch_mbox_fetch] timeout"));
        return SYS_ARCH_TIMEOUT;
    }
}

uint32_t sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg)
{
    void *dummyptr;

    if (msg == NULL)
    {
        msg = &dummyptr;
    }

    if (os_msg_recv(*q, &(*msg), 0) == true)
    {
        return ERR_OK;
    }
    else
    {
        return SYS_MBOX_EMPTY;
    }
}

#if LWIP_NETCONN_SEM_PER_THREAD
#error LWIP_NETCONN_SEM_PER_THREAD==1 not supported
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#include <stdarg.h>
#include <stdio.h>
int lwip_printf(const char *format, ...)
{
    char tx_buffer[256];
    va_list args;
    va_start(args, format);
    int n = vsnprintf((char *)tx_buffer, 256, format, args);
    va_end(args);

#ifdef TARGET_RTL8773E
    DBG_BUFFER_INTERNAL(SUBTYPE_FORMAT, MODULE_APP, LEVEL_ERROR, "[LwIP] %s", 1,
                        TRACE_STRING(tx_buffer));
#elif defined TARGET_RTL8763E
    DBG_BUFFER_INTERNAL(LOG_TYPE, SUBTYPE_FORMAT, MODULE_APP, LEVEL_ERROR, "[LwIP] %s", 1,
                        TRACE_STRING(tx_buffer));

#else
    DBG_BUFFER_INTERNAL(LOG_TYPE, SUBTYPE_FORMAT, MODULE_APP, LEVEL_ERROR, "[LwIP] %s", 1,
                        TRACE_STRING(tx_buffer));
#endif
    return n;
}

#endif /* !NO_SYS */
