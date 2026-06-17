/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BLE_HID_DEVICE_H_
#define _APP_BLE_HID_DEVICE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define HOGP_KB_REPORT_ID                       0x04

#if F_APP_BLE_HID_DEVICE_SUPPORT
void app_ble_hid_device_cmd_handle(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                   uint16_t cmd_len, uint8_t *ack_pkt);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
