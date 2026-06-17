/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BLE_HID_HOST_H_
#define _APP_BLE_HID_HOST_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#if F_APP_BLE_HID_HOST_SUPPORT
void app_ble_hid_host_read_report_map_handle(uint16_t conn_handle, uint8_t *report_buf,
                                             uint16_t report_len);

void app_ble_hid_host_read_feature_report_handle(uint16_t conn_handle, uint8_t *p_value,
                                                 uint16_t length, uint8_t report_id);

void app_ble_hid_host_inreport_handle(uint16_t conn_handle, uint8_t *p_value,
                                      uint16_t length, uint8_t report_id);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
