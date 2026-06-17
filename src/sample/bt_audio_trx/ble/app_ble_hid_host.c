/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_BLE_HID_HOST_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "hids_kb.h"
#include "app_ble_hid_host.h"
#include "app_cmd.h"
#include "os_sched.h"
#include "gap_conn_le.h"

void app_ble_hid_host_read_report_map_handle(uint16_t conn_handle, uint8_t *report_buf,
                                             uint16_t report_len)
{
    T_APP_LE_LINK *p_link;
    p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link != NULL)
    {
        APP_PRINT_INFO3("app_ble_hid_host_read_report_map_handle: le_addr %s, conn_handle 0x%x, len %d",
                        TRACE_BDADDR(p_link->bd_addr), conn_handle, report_len);

        if ((report_buf == NULL) || (report_len == 0))
        {
            return;
        }

        struct
        {
            uint16_t    conn_handle;
            uint16_t    pkt_len;
            uint8_t     payload[];
        } __attribute__((packed)) *rpt = NULL;

        rpt = calloc(1, report_len + sizeof(*rpt));
        rpt->conn_handle = conn_handle;
        rpt->pkt_len = report_len;

        memcpy(&rpt->payload, report_buf, report_len);
        app_report_event(CMD_PATH_UART, EVENT_BLE_HID_READ_REPORT_MAP, 0, (uint8_t *)rpt,
                         report_len + sizeof(*rpt));
        free(rpt);

    }
}

void app_ble_hid_host_read_feature_report_handle(uint16_t conn_handle, uint8_t *p_value,
                                                 uint16_t length, uint8_t report_id)
{
    T_APP_LE_LINK *p_link;
    p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link != NULL)
    {
        APP_PRINT_INFO3("app_ble_hid_host_read_feature_report_handle: le_addr %s, conn_handle 0x%x, len %d",
                        TRACE_BDADDR(p_link->bd_addr), conn_handle, length);
        struct
        {
            uint16_t    conn_handle;
            uint8_t     report_id;
            uint16_t    pkt_len;
            uint8_t     payload[];
        } __attribute__((packed)) *rpt = NULL;

        rpt = calloc(1, length + sizeof(*rpt));
        rpt->conn_handle = conn_handle;
        rpt->report_id = report_id;
        rpt->pkt_len = length;

        memcpy(&rpt->payload, p_value, length);
        app_report_event(CMD_PATH_UART, EVENT_BLE_HID_READ_REPORT, 0, (uint8_t *)rpt,
                         length + sizeof(*rpt));
        free(rpt);
    }
}

void app_ble_hid_host_inreport_handle(uint16_t conn_handle, uint8_t *p_value,
                                      uint16_t length, uint8_t report_id)
{
    T_APP_LE_LINK *p_link;
    p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link != NULL)
    {
        APP_PRINT_INFO3("app_ble_hid_host_inreport_handle: le_addr %s, conn_handle 0x%x, len %d",
                        TRACE_BDADDR(p_link->bd_addr), conn_handle, length);
        struct
        {
            uint16_t    conn_handle;
            uint8_t     report_id;
            uint16_t    pkt_len;
            uint8_t     payload[];
        } __attribute__((packed)) *rpt = NULL;

        rpt = calloc(1, length + sizeof(*rpt));
        rpt->conn_handle = conn_handle;
        rpt->report_id = report_id;
        rpt->pkt_len = length;

        memcpy(&rpt->payload, p_value, length);
        app_report_event(CMD_PATH_UART, EVENT_BLE_HID_IN_REPORT, 0, (uint8_t *)rpt,
                         length + sizeof(*rpt));
        free(rpt);
    }
}
#endif

