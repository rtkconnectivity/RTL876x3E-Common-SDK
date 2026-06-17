/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_HID_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "os_sync.h"
#include "usb_hid_desc.h"
#include "usb_spec20.h"
#include "trace.h"
#include "usb_hid_spec.h"
#include "usb_hid.h"
#include "usb_pipe.h"
#include "app_hid_report_desc.h"
#if F_APP_BT_AUDIO_TRI_DONGLE_2_4G
#include "section.h"
#endif
#include "errno.h"

void *inst = NULL;
void *inst1 = NULL;
void *inst2 = NULL;

static char report_descs[] =
{
    BUTTON_VOL_PLAY_CTRL_HID_DESC_ATTRIB_2,
    TELEPHONY_HID_DESC_ATTRIB,

//    HID_REPORT_DESC_KEYBOARD,
//    HID_REPORT_DESC_MOUSE_LOGIC,

    //vendor define:0xFF07
    VENDOR_0xFF07_HID_DESC_PART_1,
#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
    VENDOR_0xFF07_HID_DESC_PART_2,
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)
    VENDOR_0xFF07_HID_DESC_PART_3,
#endif
    VENDOR_0xFF07_HID_DESC_END_COLLECTION,
#if (USB_HID_CFU_USE_SINGLE_EP == 0)
#if F_APP_CFU_FEATURE_SUPPORT
    APP_CFU_HID_DESC_ATTRIB,
#endif
#endif
};
static char *g_report_desc = report_descs;
#if USB_HID_SEC_SUPPORT
static char report_descs2[] =
{
    BUTTON_VOL_PLAY_CTRL_HID_DESC_ATTRIB_2,
};
static char *g_report_desc1 = report_descs2;
#endif

#if USB_HID_CFU_USE_SINGLE_EP
static char report_descs3[] =
{
    //vendor define:0xFF07
    VENDOR_0xFF07_HID_DESC_PART_1,
#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
    VENDOR_0xFF07_HID_DESC_PART_2,
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)
    VENDOR_0xFF07_HID_DESC_PART_3,
#endif
    VENDOR_0xFF07_HID_DESC_END_COLLECTION,
    APP_CFU_HID_DESC_ATTRIB,
};
#endif

typedef struct _t_hid_ual
{
    struct _t_hid_ual *p_next;
    T_HID_CBS cbs;
} T_HID_UAL;

T_OS_QUEUE *ual_list;

static T_USB_INTERFACE_DESC hid_std_if_desc =
{
    .bLength            = sizeof(T_USB_INTERFACE_DESC),
    .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_CODE_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static T_HID_CS_IF_DESC  hid_cs_if_desc =
{
    .bLength            = sizeof(T_HID_CS_IF_DESC),
    .bDescriptorType    = DESC_TYPE_HID,
    .bcdHID             = 0x0110,
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .desc[0]            =
    {
        .bDescriptorType = DESC_TYPE_REPORT,
        .wDescriptorLength = sizeof(report_descs),
    },

};

static const T_USB_ENDPOINT_DESC int_in_ep_desc_fs =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
#if USB_HID_CFU_USE_SINGLE_EP
    .bEndpointAddress  = USB_DIR_IN | 0x04,
#else
    .bEndpointAddress  = USB_DIR_IN | 0x01,
#endif
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 1,
};

static const T_USB_ENDPOINT_DESC int_in_ep_desc_hs =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
#if USB_HID_CFU_USE_SINGLE_EP
    .bEndpointAddress  = USB_DIR_IN | 0x04,
#else
    .bEndpointAddress  = USB_DIR_IN | 0x01,
#endif
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x100,
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    .bInterval         = 1,
#else
    .bInterval         = 2,
#endif
};

#if USB_HID_SEC_SUPPORT
static T_USB_INTERFACE_DESC hid_std_if_desc1 =
{
    .bLength            = sizeof(T_USB_INTERFACE_DESC),
    .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_CODE_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static T_HID_CS_IF_DESC  hid_cs_if_desc1 =
{
    .bLength            = sizeof(T_HID_CS_IF_DESC),
    .bDescriptorType    = DESC_TYPE_HID,
    .bcdHID             = 0x0110,
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .desc[0]            =
    {
        .bDescriptorType = DESC_TYPE_REPORT,
        .wDescriptorLength = sizeof(report_descs2), // g_report_desc1
    },

};

static const T_USB_ENDPOINT_DESC int_in_ep_desc_fs1 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
#if USB_HID_CFU_USE_SINGLE_EP
    .bEndpointAddress  = USB_DIR_IN | 0x02,
#else
    .bEndpointAddress  = USB_DIR_IN | 0x04,
#endif
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 1,
};

static const T_USB_ENDPOINT_DESC int_in_ep_desc_hs1 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
#if USB_HID_CFU_USE_SINGLE_EP
    .bEndpointAddress  = USB_DIR_IN | 0x02,
#else
    .bEndpointAddress  = USB_DIR_IN | 0x04,
#endif
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 2,
};
#endif

#if USB_HID_CFU_USE_SINGLE_EP
static T_USB_INTERFACE_DESC hid_std_if_desc2 =
{
    .bLength            = sizeof(T_USB_INTERFACE_DESC),
    .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_CODE_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static T_HID_CS_IF_DESC  hid_cs_if_desc2 =
{
    .bLength            = sizeof(T_HID_CS_IF_DESC),
    .bDescriptorType    = DESC_TYPE_HID,
    .bcdHID             = 0x0110,
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .desc[0]            =
    {
        .bDescriptorType = DESC_TYPE_REPORT,
        .wDescriptorLength = sizeof(report_descs3),
    },

};

static const T_USB_ENDPOINT_DESC int_in_ep_desc_fs2 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = USB_DIR_IN | 0x01,
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 1,
};

static const T_USB_ENDPOINT_DESC int_in_ep_desc_hs2 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = USB_DIR_IN | 0x01,
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 2,
};
#endif

static void *const hid_if_descs_fs[] =
{
    (void *) &hid_std_if_desc,
    (void *) &hid_cs_if_desc,
    (void *) &int_in_ep_desc_fs,
    NULL,
};

static void *const hid_if_descs_hs[] =
{
    (void *) &hid_std_if_desc,
    (void *) &hid_cs_if_desc,
    (void *) &int_in_ep_desc_hs,
    NULL,
};

#if USB_HID_SEC_SUPPORT
static void *const hid_if_descs_fs1[] =
{
    (void *) &hid_std_if_desc1,
    (void *) &hid_cs_if_desc1,
    (void *) &int_in_ep_desc_fs1,
    NULL,
};

static void *const hid_if_descs_hs1[] =
{
    (void *) &hid_std_if_desc1,
    (void *) &hid_cs_if_desc1,
    (void *) &int_in_ep_desc_hs1,
    NULL,
};
#endif

#if USB_HID_CFU_USE_SINGLE_EP
static const void *hid_if_descs_fs2[] =
{
    (void *) &hid_std_if_desc2,
    (void *) &hid_cs_if_desc2,
    (void *) &int_in_ep_desc_fs2,
    NULL,
};

static const void *hid_if_descs_hs2[] =
{
    (void *) &hid_std_if_desc2,
    (void *) &hid_cs_if_desc2,
    (void *) &int_in_ep_desc_hs2,
    NULL,
};
#endif

void *usb_hid_data_pipe_open(uint8_t ep_addr, T_USB_HID_ATTR attr, uint8_t pending_req_num,
                             USB_HID_DATA_PIPE_CB cb)
{
    T_USB_HID_DRIVER_ATTR driver_attr;
    memcpy(&driver_attr, &attr, sizeof(T_USB_HID_DRIVER_ATTR));
    if (driver_attr.mtu < int_in_ep_desc_fs.wMaxPacketSize)
    {
        driver_attr.mtu = int_in_ep_desc_fs.wMaxPacketSize;
    }
    if (pending_req_num < USB_HID_MAX_PENDING_REQ_NUM)
    {
        pending_req_num = USB_HID_MAX_PENDING_REQ_NUM;
    }

    return usb_hid_driver_data_pipe_open(ep_addr, driver_attr, pending_req_num, cb);
}

#if F_APP_BT_AUDIO_TRI_DONGLE_2_4G
RAM_TEXT_SECTION
#endif
int usb_hid_data_pipe_send(void *handle, void *buf, uint32_t len)
{
    usb_hid_driver_data_pipe_send(handle, buf, len);
    return ESUCCESS;
}

int usb_hid_data_pipe_close(void *handle)
{
    return usb_hid_driver_data_pipe_close(handle);
}

int usb_hid_ual_register(T_HID_CBS cbs)
{
    T_HID_UAL *ual_node = malloc(sizeof(T_HID_UAL));
    memcpy(&ual_node->cbs, &cbs, sizeof(T_USB_HID_DRIVER_CBS));
    os_queue_in(ual_list, ual_node);
    return 0;
}

int usb_hid_ual_unregister(void)
{
    T_HID_UAL *ual = (T_HID_UAL *)ual_list->p_first;
    T_HID_UAL *ual_tmp;
    while (ual)
    {
        ual_tmp = ual;
        ual = ual->p_next;
        free(ual_tmp);
    }

    return 0;
}

int32_t usb_hid_get_report(T_HID_DRIVER_REPORT_REQ_VAL req_value, void *buf, uint16_t *len)
{
    uint32_t ret = 0;
    uint8_t s = os_lock();
    T_HID_UAL *ual = (T_HID_UAL *)ual_list->p_first;
    os_unlock(s);
    while (ual)
    {
        if (ual->cbs.get_report)
        {
            ret += ual->cbs.get_report(req_value, buf, len);
        }
        ual = ual->p_next;
    }

    return ret;
}

int32_t usb_hid_set_report(T_HID_DRIVER_REPORT_REQ_VAL req_value, void *buf, uint16_t len)
{
    uint32_t ret = 0;
    uint8_t s = os_lock();
    T_HID_UAL *ual = (T_HID_UAL *)ual_list->p_first;
    os_unlock(s);
    while (ual)
    {
        if (ual->cbs.set_report)
        {
            ret += ual->cbs.set_report(req_value, buf, len);
        }
        ual = ual->p_next;
    }
    return ret;
}

void usb_hid_recfg_report_descs(uint8_t ep_addr, void *report_desc, uint16_t desc_len)
{
    APP_PRINT_TRACE2("usb_hid_recfg_report_descs, ep_addr 0x%x, desc_len 0x%x)", ep_addr, desc_len);
    if (report_desc && (desc_len != 0))
    {
#if (USB_HID_CFU_USE_SINGLE_EP == 0)
        if (ep_addr == HID_INT_IN_EP_1)
#else
        if (ep_addr == HID_INT_IN_EP_4)
#endif
        {
            hid_cs_if_desc.desc[0].wDescriptorLength = desc_len;
            g_report_desc = report_desc;
        }
#if USB_HID_SEC_SUPPORT
        else if (ep_addr == HID_INT_IN_EP_2)
        {
            hid_cs_if_desc1.desc[0].wDescriptorLength = desc_len;
            g_report_desc1 = report_desc;
        }
#endif
    }
}

void usb_hid_init(void)
{
    inst = usb_hid_driver_inst_alloc();
    ual_list = malloc(sizeof(T_OS_QUEUE));
    os_queue_init(ual_list);
    usb_hid_driver_if_desc_register(inst, (void *)hid_if_descs_hs, (void *)hid_if_descs_fs,
                                    (void *)g_report_desc);
#if (USB_HID_SEC_SUPPORT == 1)
    if (g_report_desc1)
    {
        inst1 = usb_hid_driver_inst_alloc();
        usb_hid_driver_if_desc_register(inst1, (void *)hid_if_descs_hs1, (void *)hid_if_descs_fs1,
                                        (void *)g_report_desc1);
    }
#endif

#if USB_HID_CFU_USE_SINGLE_EP
    inst2 = usb_hid_driver_inst_alloc();
    usb_hid_driver_if_desc_register(inst2, (void *)hid_if_descs_hs2, (void *)hid_if_descs_fs2,
                                    (void *)report_descs3);
#endif
    T_USB_HID_DRIVER_CBS cbs;
    memset(&cbs, 0, sizeof(T_USB_HID_DRIVER_CBS));
    cbs.get_report = usb_hid_get_report;
    cbs.set_report = usb_hid_set_report;
    usb_hid_driver_cbs_register(inst, &cbs);
#if (USB_HID_SEC_SUPPORT == 1)
    if (inst1)
    {
        usb_hid_driver_cbs_register(inst1, &cbs);
    }
#endif

#if USB_HID_CFU_USE_SINGLE_EP
    if (inst2)
    {
        usb_hid_driver_cbs_register(inst2, &cbs);
    }
#endif
    usb_hid_driver_init();
}

void usb_hid_deinit(void)
{
    usb_hid_driver_deinit();
    usb_hid_driver_cbs_unregister(inst);
    usb_hid_driver_if_desc_unregister(inst);
    usb_hid_driver_inst_free(inst);
#if USB_HID_SEC_SUPPORT
    if (g_report_desc1)
    {
        usb_hid_driver_cbs_unregister(inst1);
        usb_hid_driver_if_desc_unregister(inst1);
        usb_hid_driver_inst_free(inst1);
        inst1 = NULL;
        g_report_desc1 = NULL;
    }
#endif
#if USB_HID_CFU_USE_SINGLE_EP
    usb_hid_driver_cbs_unregister(inst2);
    usb_hid_driver_if_desc_unregister(inst2);
    usb_hid_driver_inst_free(inst2);
    inst2 = NULL;
#endif
}
#endif
