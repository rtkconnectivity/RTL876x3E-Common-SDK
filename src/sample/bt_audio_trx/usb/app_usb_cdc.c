/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_CDC_SUPPORT
#include "usb_cdc.h"
#include "string.h"
#include "app_usb.h"
#include "app_usb_cdc.h"
#include "trace.h"
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_mgr.h"
#endif

void *cdc_bulk_in_handle = NULL;
void *cdc_bulk_out_handle = NULL;

bool app_usb_cdc_bulk_in_pipe_send(void *data, uint16_t length)
{
    bool ret = false;

    if ((app_usb_power_state() == USB_CONFIGURED) & (cdc_bulk_in_handle != NULL))
    {
        ret = usb_cdc_data_pipe_send(cdc_bulk_in_handle, data, length);
    }

    return ret;
}

static uint32_t app_usb_cdc_in_pipe_xfer_done(void *handle, void *buf, uint32_t result,
                                              int status)
{
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    struct
    {
        uint32_t cdc_tx_len;
        int cdc_status;
        void *cdc_buf;
    } rpt = {0};
    rpt.cdc_tx_len = result;
    rpt.cdc_status = status;
    rpt.cdc_buf = buf;
    app_tri_dongle_if_sys_evt(REDEVELOP_MSG_SYS_USB_CDC_IN_DONE, (uint8_t *)&rpt, NULL);
#endif
    return 0;
}

uint32_t app_usb_cdc_out_pipe_xfer_done(void *handle, void *buf, uint32_t len, int status)
{
    //Receive data from the CDC here
    //USB_PRINT_INFO0("app_usb_cdc_out_pipe_xfer_done");
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    struct
    {
        uint32_t cdc_len;
        int cdc_status;
        void *cdc_buf;
    } rpt = {0};
    rpt.cdc_len = len;
    rpt.cdc_status = status;
    rpt.cdc_buf = buf;
    app_tri_dongle_if_sys_evt(REDEVELOP_MSG_SYS_USB_CDC_OUT_REPORT, (uint8_t *)&rpt, NULL);
#endif
    //Use this function send data to host
    //Sample code loopback behavior
    app_usb_cdc_bulk_in_pipe_send(buf, len);

    return 0;
}

void app_usb_cdc_open_pipe(void)
{
    if (cdc_bulk_in_handle == NULL)
    {
        T_USB_CDC_ATTR attr =
        {
            .zlp = 1,
            .high_throughput = 0,
            .congestion_ctrl = CDC_CONGESTION_CTRL_DROP_CUR,
            .rsv = 0,
            .mtu = CDC_MAX_TRANSMISSION_UNIT
        };
        cdc_bulk_in_handle = usb_cdc_data_pipe_open(CDC_BULK_IN_EP, attr, CDC_MAX_PENDING_REQ_NUM,
                                                    app_usb_cdc_in_pipe_xfer_done);
    }

    if (cdc_bulk_out_handle == NULL)
    {
        T_USB_CDC_ATTR attr =
        {
            .zlp = 0,
            .high_throughput = 0,
            .congestion_ctrl = CDC_CONGESTION_CTRL_DROP_CUR,
            .rsv = 0,
            .mtu = CDC_MAX_TRANSMISSION_UNIT
        };
        cdc_bulk_out_handle = usb_cdc_data_pipe_open(CDC_BULK_OUT_EP, attr, 0,
                                                     app_usb_cdc_out_pipe_xfer_done);
    }

}

void app_usb_cdc_close_pipe(void)
{
    if (cdc_bulk_in_handle != NULL)
    {
        usb_cdc_data_pipe_close(cdc_bulk_in_handle);
        cdc_bulk_in_handle = NULL;
    }

    if (cdc_bulk_out_handle != NULL)
    {
        usb_cdc_data_pipe_close(cdc_bulk_out_handle);
        cdc_bulk_out_handle = NULL;
    }
}

#endif
