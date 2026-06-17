/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_USB_CDC_H__
#define __APP_USB_CDC_H__

#define CDC_MAX_TRANSMISSION_UNIT                   0x40
#define CDC_MAX_PENDING_REQ_NUM                     0x08

extern void *cdc_bulk_in_handle;

bool app_usb_cdc_bulk_in_pipe_send(void *data, uint16_t length);
void app_usb_cdc_open_pipe(void);
void app_usb_cdc_close_pipe(void);

#endif
