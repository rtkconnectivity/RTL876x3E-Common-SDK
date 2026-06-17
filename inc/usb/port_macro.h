/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _PORT_MACRO_H_
#define _PORT_MACRO_H_
#include "os_mem.h"

#define USB_USER_SPEC_SECTION    __attribute__((section(".text.usb.user_spec")))
#define USB_ISOC_ISR_ENTRY_SECTION    __attribute__((section(".text.usb.isoc_isr_entry")))

#ifdef CONFIG_SOC_SERIES_RTL8763E
#define USB_PKT_RAM_ALLOC(size)   os_mem_alloc(RAM_TYPE_BUFFER_ON, size)
#else
#define USB_PKT_RAM_ALLOC(size)   os_mem_alloc(RAM_TYPE_DATA_ON, size)
#endif
#define USB_PKT_RAM_FREE(ptr)   os_mem_free(ptr)
#endif
