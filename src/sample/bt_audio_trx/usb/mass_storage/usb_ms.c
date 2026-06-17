/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_MSC_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "usb_spec20.h"
#include "trace.h"
#include "usb_ms.h"

static T_USB_INTERFACE_DESC usb_msc_if_desc =
{
    .bLength = sizeof(T_USB_INTERFACE_DESC),
    .bDescriptorType = USB_DESC_TYPE_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_MSC,
    .bInterfaceSubClass = 0x06, //SCSI
    .bInterfaceProtocol = 0x50, //BBB
    .iInterface = 0
};

static const T_USB_ENDPOINT_DESC usb_ms_bi_ep_desc_hs =
{
    .bLength = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress = MSC_BULK_IN_EP,
    .bmAttributes = USB_EP_TYPE_BULK,
    .wMaxPacketSize = 512,
    .bInterval = 4,
};

static const T_USB_ENDPOINT_DESC usb_ms_bo_ep_desc_hs =
{
    .bLength = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress = MSC_BULK_OUT_EP,
    .bmAttributes = USB_EP_TYPE_BULK,
    .wMaxPacketSize = 512,
    .bInterval = 4,
};

static const T_USB_ENDPOINT_DESC usb_ms_bi_ep_desc_fs =
{
    .bLength = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress = 0x82,
    .bmAttributes = USB_EP_TYPE_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
};

static const T_USB_ENDPOINT_DESC usb_ms_bo_ep_desc_fs =
{
    .bLength = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress = 0x02,
    .bmAttributes = USB_EP_TYPE_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
};

static T_USB_DESC_HDR *const usb_ms_if_descs_fs[] =
{
    (T_USB_DESC_HDR *) &usb_msc_if_desc,
    (T_USB_DESC_HDR *) &usb_ms_bo_ep_desc_fs,
    (T_USB_DESC_HDR *) &usb_ms_bi_ep_desc_fs,
    NULL
};

static T_USB_DESC_HDR *const usb_ms_if_descs_hs[] =
{
    (T_USB_DESC_HDR *) &usb_msc_if_desc,
    (T_USB_DESC_HDR *) &usb_ms_bo_ep_desc_hs,
    (T_USB_DESC_HDR *) &usb_ms_bi_ep_desc_hs,
    NULL
};

void usb_msc_init(void)
{
    usb_ms_driver_if_desc_register((T_USB_DESC_HDR **)usb_ms_if_descs_fs,
                                   (T_USB_DESC_HDR **)usb_ms_if_descs_hs);

    usb_ms_driver_init();
}

void usb_msc_deinit(void)
{
    usb_ms_driver_deinit();

    usb_ms_driver_if_desc_unregister();
}
#endif
