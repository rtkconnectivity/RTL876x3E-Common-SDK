/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */


#include "gui_fsal.h"
#include "fsal_posix.h"

static struct fsal_fdtable fdtab;

static int fd_alloc(struct fsal_fdtable *fdt, int startfd)
{
    int idx;
    /* find an empty fd entry */
    for (idx = startfd; idx < (int)fdt->maxfd; idx++)
    {
        if (fdt->fds[idx] == RT_NULL)
        {
            break;
        }
        if (fdt->fds[idx]->ref_count == 0)
        {
            break;
        }
    }

    /* allocate a larger FD container */
    if (idx == fdt->maxfd && fdt->maxfd < FSAL_FD_MAX)
    {
        int cnt, index;
        struct gui_file_fd **fds;

        /* increase the number of FD with 4 step length */
        cnt = fdt->maxfd + 4;
        cnt = cnt > FSAL_FD_MAX ? FSAL_FD_MAX : cnt;

        fds = (struct gui_file_fd **)GUI_REALLOC(fdt->fds, cnt * sizeof(struct gui_file_fd *));
        if (fds == NULL) { goto __exit; } /* return fdt->maxfd */

        /* clean the new allocated fds */
        for (index = fdt->maxfd; index < cnt; index ++)
        {
            fds[index] = NULL;
        }

        fdt->fds   = fds;
        fdt->maxfd = cnt;
    }

    /* allocate  'struct gui_file_fd' */
    if (idx < (int)fdt->maxfd && fdt->fds[idx] == RT_NULL)
    {
        fdt->fds[idx] = (struct gui_file_fd *)GUI_CALLOC(1, sizeof(struct gui_file_fd));
        if (fdt->fds[idx] == RT_NULL)
        {
            idx = fdt->maxfd;
        }
    }

__exit:
    return idx;
}

static int fd_new(void)
{
    struct gui_file_fd *d;
    int idx;
    struct fsal_fdtable *fdt;
    fdt = &fdtab;
    idx = fd_alloc(fdt, 0);
    /* can't find an empty fd entry */
    if (idx == fdt->maxfd)
    {
        idx = -(1 + FSAL_FD_OFFSET);
        GUI_PRINTF("DFS fd new is failed! Could not found an empty fd entry.");
        goto __result;
    }

    d = fdt->fds[idx];
    d->ref_count = 1;
    d->magic = FSAL_FD_MAGIC;

__result:
    return idx + FSAL_FD_OFFSET;

}

static struct gui_file_fd *fd_get(int fd)
{
    struct gui_file_fd *d;
    struct fsal_fdtable *fdt;
    fdt = &fdtab;
    fd = fd - FSAL_FD_OFFSET;
    if (fd < 0 || fd >= (int)fdt->maxfd)
    {
        return NULL;
    }
    d = fdt->fds[fd];
    GUI_PRINTF("fd = %d\n", fd);
    GUI_PRINTF("d = 0x%x\n", d);
    /* check gui_file_fd valid or not */
    if ((d == NULL) || (d->magic != FSAL_FD_MAGIC))
    {
        return NULL;
    }

    /* increase the reference count */
    d->ref_count ++;


    return d;
}
static void fd_put(struct gui_file_fd *fd)
{
    RT_ASSERT(fd != NULL);

    fd->ref_count --;

    /* clear this fd entry */
    if (fd->ref_count == 0)
    {
        int index;
        struct fsal_fdtable *fdt;

        fdt = &fdtab;
        for (index = 0; index < (int)fdt->maxfd; index ++)
        {
            if (fdt->fds[index] == fd)
            {
                GUI_FREE(fd);
                fdt->fds[index] = 0;
                break;
            }
        }
    }
}
/* file api*/
int gui_open(const char *file, int flags, ...)
{
    int fd;
    int result;
    struct gui_file_fd *d;
    fd = fd_new();
    if (fd < 0)
    {
        return -1;
    }
    d = fd_get(fd);

    result = fsal_open(d, file, flags);
    if (result < 0)
    {
        /* release the ref-count of fd */
        fd_put(d);
        fd_put(d);

        rt_set_errno(result);

        return -1;
    }

    /* release the ref-count of fd */
    fd_put(d);

    return fd;

}
int gui_read(int fd, void *buf, size_t len)
{
    int result;
    struct gui_file_fd *d;

    /* get the fd */
    d = fd_get(fd);
    if (d == NULL)
    {

        return -1;
    }

    result = fsal_read(d, buf, len);
    if (result < 0)
    {
        fd_put(d);

        return -1;
    }

    /* release the ref-count of fd */
    fd_put(d);

    return result;
}
int gui_write(int fd, const void *buf, size_t len)
{
    int result;
    struct gui_file_fd *d;

    /* get the fd */
    d = fd_get(fd);
    if (d == NULL)
    {

        return -1;
    }

    result = fsal_write(d, (void *)buf, len);
    if (result < 0)
    {
        fd_put(d);

        return -1;
    }

    /* release the ref-count of fd */
    fd_put(d);

    return result;
}
int gui_close(int fd)
{
    int result;
    struct gui_file_fd *d;

    d = fd_get(fd);
    if (d == NULL)
    {

        return -1;
    }

    result = fsal_close(d);
    fd_put(d);

    if (result < 0)
    {

        return -1;
    }

    fd_put(d);

    return 0;
}
int gui_lseek(int fd, int offset, int whence)
{
    int result;
    struct gui_file_fd *d;

    d = fd_get(fd);

    if (d == NULL)
    {

        return -1;
    }

    result = fsal_seek(d, offset, whence);
    fd_put(d);

    return 0;
}
