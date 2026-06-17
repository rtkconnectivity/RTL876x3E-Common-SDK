/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <os_mem.h>

//#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
//__asm(".global __use_no_semihosting\n\t");
//#else
//#pragma import(__use_no_semihosting_swi)
//#endif

/* Standard IO device handles. */
#define STDIN       0
#define STDOUT      1
#define STDERR      2

typedef int FILEHANDLE;

/*
 * These names are used during library initialization as the
 * file names opened for stdin, stdout, and stderr.
 * As we define _sys_open() to always return the same file handle,
 * these can be left as their default values.
 */
const char __stdin_name[] =  ":tt";
const char __stdout_name[] =  ":tt";
const char __stderr_name[] =  ":tt";

FILEHANDLE _sys_open(const char *name, int openmode)
{
    return 1; /* everything goes to the same output */
}
int _sys_close(FILEHANDLE fh)
{
    return 0;
}
int _sys_write(FILEHANDLE fh, const unsigned char *buf,
               unsigned len, int mode)
{
//  your_device_write(buf, len);
    return 0;
}
int _sys_read(FILEHANDLE fh, unsigned char *buf,
              unsigned len, int mode)
{
    return -1; /* not supported */
}
void _ttywrch(int ch)
{
    char c = ch;
//  your_device_write(&c, 1);
}
int _sys_istty(FILEHANDLE fh)
{
    return 0; /* buffered output */
}
int _sys_seek(FILEHANDLE fh, long pos)
{
    return -1; /* not supported */
}
long _sys_flen(FILEHANDLE fh)
{
    return -1; /* not supported */
}

void _sys_exit(int return_code)
{
    /* TODO: perhaps exit the thread which is invoking this function */
    while (1);
}

/**
 * used by tmpnam() or tmpfile()
 */
int _sys_tmpnam(char *name, int fileno, unsigned maxlength)
{
    return -1;
}

char *_sys_command_string(char *cmd, int len)
{
    /* no support */
    return NULL;
}

#include <rt_sys.h>
#include <stdio.h>
#include <trace.h>
#include <os_sched.h>
int fputc(int ch, FILE *f)
{
    DBG_DIRECT("[STUBS] %c", ch);
    return ch;
}

/*retarget to weak function to ensure that the application has freedom of redefinition*/
void *malloc(size_t size)
{
    void *ptr = os_mem_alloc(RAM_TYPE_ITCM1, size);
    if (ptr == NULL)
    {
        DBG_DIRECT("[malloc]!!!!!!!!!!MALLOC FAIL!!!!!!!!!");
    }

    return ptr;
}

void *calloc(size_t n, size_t size)
{
    return os_mem_zalloc(RAM_TYPE_ITCM1, n * size);
}


#define FreeRTOS_portBYTE_ALIGNMENT 8
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK *pxNextFreeBlock;   /*<< The next free block in the list. */
    size_t xBlockSize : 28;                 /*<< The size of the free block. */
    size_t xRamType : 3;
    size_t xAllocateBit : 1;
} BlockLink_t;
void *realloc(void *ptr, size_t n)
{
    void *new_ptr;
    BlockLink_t *pxLink;
    size_t old_size, new_size = 0;

    if (ptr == NULL)
    {
        return os_mem_alloc(RAM_TYPE_ITCM1, n);
    }

    if (n == 0)
    {
        os_mem_free(ptr);
        ptr = NULL;
        return NULL;
    }

    pxLink = (BlockLink_t *)((uint8_t *)ptr - sizeof(BlockLink_t));
    old_size = pxLink->xBlockSize;

    if ((old_size - sizeof(BlockLink_t)) ==
        (n + (FreeRTOS_portBYTE_ALIGNMENT - (n & (FreeRTOS_portBYTE_ALIGNMENT - 1)))))
    {
        return ptr;
    }

    os_sched_suspend();
    os_mem_free(ptr);
    new_ptr = os_mem_alloc(RAM_TYPE_ITCM1, n);
    if (ptr != new_ptr)
    {
        pxLink = (BlockLink_t *)((uint8_t *)new_ptr - sizeof(BlockLink_t));
        new_size = pxLink->xBlockSize;
        memcpy(new_ptr, ptr, (old_size < new_size) ? (old_size - sizeof(BlockLink_t)) : (new_size - sizeof(
                    BlockLink_t)));
    }
    os_sched_resume();

    return new_ptr;
}

void free(void *ptr)
{
    os_mem_free(ptr);
}


