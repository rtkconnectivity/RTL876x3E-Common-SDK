/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_MSC_SUPPORT
#include "errno.h"
#include "usb_ms_driver.h"
#include "sd.h"
#include "os_mem.h"
#include <string.h>

#define USB_MS_DISK_BLK_SIZE        (0x200)

static int usb_ms_disk_format(void)
{
    return ESUCCESS;
}

void *usb_ms_disk_buffer_alloc(uint32_t blk_num)
{
    return os_mem_zalloc(RAM_TYPE_BUFFER_ON, blk_num * USB_MS_DISK_BLK_SIZE);
}

int usb_ms_disk_buffer_free(void *buf)
{
    os_mem_free(buf);

    return ESUCCESS;
}

static int usb_ms_disk_read(uint32_t lba, uint32_t blk_num, uint8_t *data)
{
    int ret = ESUCCESS;
    if (sd_read(lba, (uint32_t)data, USB_MS_DISK_BLK_SIZE, blk_num) != SD_OK)
    {
        ret = -EIO;
    }
    return ret;
}

static int usb_ms_disk_write(uint32_t lba, uint32_t blk_num, uint8_t *data)
{
    int ret = ESUCCESS;

    if (sd_write(lba, (uint32_t)data, USB_MS_DISK_BLK_SIZE, blk_num) != SD_OK)
    {
        ret = -EIO;
    }

    return ret;
}

static bool usb_ms_disk_is_ready(void)
{
    return true;
}

static int usb_ms_disk_capacity_get(uint32_t *max_lba, uint32_t *blk_len)
{
//    *max_lba =  sd_if_get_dev_block_num();
    *blk_len = USB_MS_DISK_BLK_SIZE;

    *max_lba =  sd_get_dev_capacity() / (*blk_len) - 1;

    return ESUCCESS;
}

static const T_DISK_DRIVER usb_ms_disk_driver =
{
    .type = 0,
    .blk_size = USB_MS_DISK_BLK_SIZE,
    .max_blk_per_access = 1,
    .format = usb_ms_disk_format,
    .read = usb_ms_disk_read,
    .write = usb_ms_disk_write,
    .is_ready = usb_ms_disk_is_ready,
    .remove = NULL,
    .capacity_get = usb_ms_disk_capacity_get,
    .buffer_alloc = usb_ms_disk_buffer_alloc,
    .buffer_free = usb_ms_disk_buffer_free,
};


int usb_ms_disk_init(void)
{
    usb_ms_driver_disk_register((T_DISK_DRIVER *)&usb_ms_disk_driver);
    return 0;
}

int usb_ms_disk_deinit(void)
{
    usb_ms_driver_disk_unregister((T_DISK_DRIVER *)&usb_ms_disk_driver);
    // usb_dm_cb_unregister(usb_ms_disk_dm_cb);
    return 0;
}
#endif
