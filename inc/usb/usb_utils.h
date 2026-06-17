/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_UTILS__
#define __USB_UTILS__
#include "os_queue.h"

#define USB_UTILS_MIN(a,b)            (((a) < (b)) ? (a) : (b))

#define T_USB_UTILS_LIST  T_OS_QUEUE

#define USB_UTILS_LIST_INIT(list)   os_queue_init(list)

#define USB_UTILS_LIST_INSERT_TAIL(list, item)  os_queue_in(list, item)

#define USB_UTILS_LIST_REMOVE_HEAD(list, item)  item = os_queue_out(list)

#define USB_UTILS_LIST_REMOVE(list, item)  os_queue_delete(list, item)

#define USB_UTILS_LIST_FIRST(list)  (list)->p_first

#define USB_UTILS_LIST_LAST(list)  (list)->p_last

#define USB_UTILS_LIST_EMPTY(list) ((list)->count == 0)

#define USB_UTILS_LIST_FOREACH(list, type, item)        \
    item = (type)(list)->p_first;                         \
    for(type item_next = (item != NULL)?item->p_next:NULL; item != NULL; item = item_next, item_next = (item_next != NULL)?item_next->p_next:NULL)\

#define USB_UTILS_CONTAINER_OF(ptr, type, member)   \
    ((type *)((char *)(ptr)-(size_t)(&((type *)0)->member)))

#define USB_UTILS_ALIGN_ROUNDUP(val, align)  (val + (align - ((val%align == 0)?align:(val%align))))
#define USB_UTILS_ALIGN_ROUNDDOWN(val, align)  (val - val%align)
#endif
