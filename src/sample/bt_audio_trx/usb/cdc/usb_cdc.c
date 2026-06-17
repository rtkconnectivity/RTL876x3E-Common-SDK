/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_CDC_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "os_sync.h"
#include "usb_spec20.h"
#include "trace.h"
#include "usb_cdc_spec.h"
#include "usb_cdc.h"
#include "usb_pipe.h"

static void *cdc_inst0 = NULL;
static void *cdc_inst1 = NULL;

#if IAD_SUPPORT
static T_USB_INTERFACE_ASSOC_DESC cdc_iad_desc =
{
    .bLength = sizeof(T_USB_INTERFACE_ASSOC_DESC),
    .bDescriptorType = USB_DESC_TYPE_IAD,
    .bFirstInterface = 0,
    .bInterfaceCount = 2,
    .bFunctionClass = 2,
    .bFunctionSubClass = 2,
    .bFunctionProtocol = 0,
    .iFunction = 0
};
#endif

static T_USB_INTERFACE_DESC cdc_std_if_desc =
{
    .bLength            = sizeof(T_USB_INTERFACE_DESC),
    .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = USB_CLASS_CODE_COMM,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static const T_CDC_HEADER_FUNC_DESC cdc_header_desc =
{
    .bFunctionLength = sizeof(T_CDC_HEADER_FUNC_DESC),
    .bDescriptorType = USB_DESCTYPE_CLASS_INTERFACE,
    .bDescriptorSubtype = 0,
    .bcdCDC = 0x0110
};

static T_CM_FUNC_DESC cm_func_desc =
{
    .bFunctionLength = sizeof(T_CM_FUNC_DESC),
    .bDescriptorType = USB_DESCTYPE_CLASS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_SUBCLASS_CM,
    .bmCapabilities = 0x03,
    .bDataInterface = 1
};

static const T_CDC_ACM_FUNC_DESC acm_func_desc =
{
    .bFunctionLength = sizeof(T_CDC_ACM_FUNC_DESC),
    .bDescriptorType = USB_DESCTYPE_CLASS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_SUBCLASS_ACM,
    .bmCapabilities = 0x00,
};

static T_CDC_UNION_FUNC_DESC cdc_union_desc =
{
    .bFunctionLength = sizeof(T_CDC_UNION_FUNC_DESC),
    .bDescriptorType = USB_DESCTYPE_CLASS_INTERFACE,
    .bDescriptorSubtype = 0x06,
    .bControlInterface = 0,   // intf num of cdc_std_if_desc
    .bSubordinateInterface = 1   // intf num of cdc_std_data_if_desc
};

static T_USB_INTERFACE_DESC cdc_std_data_if_desc =
{
    .bLength = sizeof(T_USB_INTERFACE_DESC),
    .bDescriptorType = USB_DESC_TYPE_INTERFACE,
    .bInterfaceNumber = 1,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_CDC_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0, // Common AT commands
    .iInterface = 0
};

static const T_USB_ENDPOINT_DESC bulk_in_ep_desc_fs =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = CDC_BULK_IN_EP,
    .bmAttributes      = USB_EP_TYPE_BULK,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 0,
};

static const T_USB_ENDPOINT_DESC bulk_in_ep_desc_hs =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = CDC_BULK_IN_EP,
    .bmAttributes      = USB_EP_TYPE_BULK,
    .wMaxPacketSize    = 512,
    .bInterval         = 0,
};

static const T_USB_ENDPOINT_DESC bulk_out_ep_desc_fs =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = CDC_BULK_OUT_EP,
    .bmAttributes      = USB_EP_TYPE_BULK,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 0,
};

static const T_USB_ENDPOINT_DESC bulk_out_ep_desc_hs =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = CDC_BULK_OUT_EP,
    .bmAttributes      = USB_EP_TYPE_BULK,
    .wMaxPacketSize    = 512,
    .bInterval         = 0,
};

static void *const cdc_if0_descs_fs[] =
{
#if IAD_SUPPORT
    (void *) &cdc_iad_desc,
#endif
    (void *) &cdc_std_if_desc,
    (void *) &cdc_header_desc,
    (void *) &cm_func_desc,
    (void *) &acm_func_desc,
    (void *) &cdc_union_desc,
    NULL,
};

static void *const cdc_if0_descs_hs[] =
{
#if IAD_SUPPORT
    (void *) &cdc_iad_desc,
#endif
    (void *) &cdc_std_if_desc,
    (void *) &cdc_header_desc,
    (void *) &cm_func_desc,
    (void *) &acm_func_desc,
    (void *) &cdc_union_desc,
    NULL,
};

static void *const cdc_if1_descs_fs[] =
{
    (void *) &cdc_std_data_if_desc,
    (void *) &bulk_in_ep_desc_fs,
    (void *) &bulk_out_ep_desc_fs,
    NULL,
};

static void *const cdc_if1_descs_hs[] =
{
    (void *) &cdc_std_data_if_desc,
    (void *) &bulk_in_ep_desc_hs,
    (void *) &bulk_out_ep_desc_hs,
    NULL,
};

void *usb_cdc_data_pipe_open(uint8_t ep_addr, T_USB_CDC_ATTR attr, uint8_t pending_req_num,
                             USB_CDC_DATA_PIPE_CB cb)
{
    T_USB_CDC_DRIVER_ATTR driver_attr;
    memcpy(&driver_attr, &attr, sizeof(T_USB_CDC_DRIVER_ATTR));
    return usb_cdc_driver_data_pipe_open(ep_addr, driver_attr, pending_req_num, cb);
}

bool usb_cdc_data_pipe_send(void *handle, void *buf, uint32_t len)
{
    usb_cdc_driver_data_pipe_send(handle, buf, len);
    return true;
}

int usb_cdc_data_pipe_close(void *handle)
{
    return usb_cdc_driver_data_pipe_close(handle);
}

void usb_cdc_init(void)
{
    cdc_inst0 = usb_cdc_driver_inst_alloc();
    usb_cdc_driver_if_desc_register(cdc_inst0, (void *)cdc_if0_descs_hs, (void *)cdc_if0_descs_fs);
    cdc_inst1 = usb_cdc_driver_inst_alloc();
    usb_cdc_driver_if_desc_register(cdc_inst1, (void *)cdc_if1_descs_hs, (void *)cdc_if1_descs_fs);

    usb_cdc_driver_init();
#if IAD_SUPPORT
    cdc_iad_desc.bFirstInterface = cdc_std_if_desc.bInterfaceNumber;
#endif
}

void usb_cdc_deinit(void)
{
    usb_cdc_driver_deinit();

    usb_cdc_driver_if_desc_unregister(cdc_inst0);

    usb_cdc_driver_if_desc_unregister(cdc_inst1);

    usb_cdc_driver_inst_free(cdc_inst0);
    cdc_inst0 = NULL;

    usb_cdc_driver_inst_free(cdc_inst1);
    cdc_inst1 = NULL;
}
#endif
