/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __FSAL_POSIX_H__
#define __FSAL_POSIX_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FSAL_FD_MAX 100
#define FSAL_FD_OFFSET 3
#define FSAL_FD_MAGIC 0xfdfd
struct fsal_fdtable
{
    uint32_t maxfd;
    struct gui_file_fd **fds;
};

/* file api*/
int gui_open(const char *file, int flags, ...);
int gui_read(int fd, void *buf, size_t len);
int gui_write(int fd, const void *buf, size_t len);
int gui_close(int d);
int gui_lseek(int fd, int offset, int whence);

#ifdef __cplusplus
}
#endif

#endif
