/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_BLE_HID_DEVICE_SUPPORT
#include <string.h>
#include "trace.h"
#include "hids_kb.h"
#include "app_ble_hid_device.h"
#include "app_cmd.h"
#include "os_sched.h"
#include "gap_conn_le.h"

void app_ble_hid_device_cmd_handle(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                   uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t conn_handle;
    conn_handle = le_get_conn_handle(0);
    APP_PRINT_TRACE2("app_ble_hid_device_cmd_handle: conn_handle %d, data %b",
                     conn_handle, TRACE_BINARY(cmd_len, cmd_ptr));

    if (!hids_send_report(conn_handle, HOGP_KB_REPORT_ID, &cmd_ptr[2], cmd_len - 2))
    {
        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
    }

    os_delay(200);

    memset(cmd_ptr, 0, cmd_len);
    if (!hids_send_report(conn_handle, HOGP_KB_REPORT_ID, &cmd_ptr[2], cmd_len - 2))
    {
        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
    }
    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
}
#endif
