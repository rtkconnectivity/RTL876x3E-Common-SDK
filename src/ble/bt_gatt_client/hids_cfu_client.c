/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "stdlib.h"
#include <hids_cfu_client.h>
#include <trace.h>
#include <os_mem.h>

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

static T_HIDS_LINK hids_table[HIDS_MAX_LINKS];

/**<  Callback used to send data to app from HIDS client layer. */
static P_FUN_CLIENT_UUID16_GENERAL_APP_CB hids_client_cb = NULL;

T_HIDS_LINK *hids_find_link_by_conn_handle(uint8_t conn_handle)
{
    for (uint8_t i = 0; i < HIDS_MAX_LINKS; i++)
    {
        if (hids_table[i].used && (hids_table[i].conn_handle == conn_handle))
        {
            return &hids_table[i];
        }
    }

    return NULL;
}

T_HIDS_LINK *hids_alloc_link(uint8_t conn_handle)
{
    T_HIDS_LINK *p_hids_link = NULL;
    p_hids_link = hids_find_link_by_conn_handle(conn_handle);
    if (p_hids_link)
    {
        return p_hids_link;
    }

    for (uint8_t i = 0; i < HIDS_MAX_LINKS; i++)
    {
        if (hids_table[i].used == false)
        {
            hids_table[i].used = true;
            hids_table[i].conn_handle = conn_handle;
            return &hids_table[i];
        }
    }

    return NULL;
}

void hids_report_init(T_HIDS_SERVICE *p_service_info, uint8_t report_num)
{
    T_HIDS_REPORT  *p_report_info = NULL;
    PROTOCOL_PRINT_TRACE1("hids_report_init: report_num %d", report_num);

    if (!report_num)
    {
        return;
    }

    p_service_info->p_report_info = (T_HIDS_REPORT *)calloc(report_num, sizeof(T_HIDS_REPORT));
    if (!p_service_info->p_report_info)
    {
        return;
    }
    p_report_info = p_service_info->p_report_info;

    for (uint8_t i = 1; i < report_num; i++)
    {
        p_report_info->p_next = p_report_info + 1;
        p_report_info = p_report_info->p_next;
    }
}

void hids_service_init(T_HIDS_LINK *p_hids_link, uint8_t service_num)
{
    T_HIDS_SERVICE *p_service_info = NULL;
    PROTOCOL_PRINT_TRACE1("hids_service_init: serv num %d", service_num);

    if (!service_num)
    {
        return;
    }

    p_hids_link->hids_service_info = (T_HIDS_SERVICE *)calloc(1, sizeof(T_HIDS_SERVICE));

    if (!p_hids_link->hids_service_info)
    {
        return;
    }
    p_service_info = p_hids_link->hids_service_info;

    for (uint8_t i = 1; i < service_num; i++)
    {
        p_service_info->p_next = (T_HIDS_SERVICE *)calloc(1, sizeof(T_HIDS_SERVICE));
        if (!p_service_info->p_next)
        {
            return;
        }
        p_service_info = p_service_info->p_next;
        p_service_info->service_id = i;
    }
}

T_HIDS_SERVICE  *hids_find_service(uint8_t conn_handle, uint8_t service_id)
{
    T_HIDS_LINK *p_hids_link = NULL;
    T_HIDS_SERVICE *p_hids_service = NULL;
    for (uint8_t i = 0; i < HIDS_MAX_LINKS; i++)
    {
        if (hids_table[i].used && (hids_table[i].conn_handle == conn_handle))
        {
            p_hids_link = &hids_table[i];
            break;
        }
    }

    if (p_hids_link)
    {
        p_hids_service = p_hids_link->hids_service_info;
        while (p_hids_service)
        {
            if (p_hids_service->service_id == service_id)
            {
                return p_hids_service;
            }
            p_hids_service = p_hids_service->p_next;
        }
    }
    return NULL;
}

T_HIDS_REPORT  *hids_find_report(uint8_t conn_handle, uint8_t service_id,
                                 uint8_t report_instance_id)
{
    T_HIDS_REPORT  *p_hids_report = NULL;
    T_HIDS_SERVICE  *p_hids_service = NULL;
    uint8_t i = 0;

    PROTOCOL_PRINT_TRACE2("hids_find_report: srv_id %d, report id %d", service_id, report_instance_id);

    p_hids_service = hids_find_service(conn_handle, service_id);
    if (p_hids_service)
    {
        p_hids_report = p_hids_service->p_report_info;
        if (report_instance_id >= p_hids_service->hids_report_num || !p_hids_report)
        {
            return NULL;
        }

        while (i != report_instance_id)
        {
            p_hids_report = p_hids_report->p_next;
            i++;
            if (!p_hids_report)
            {
                return NULL;
            }
        }
        return p_hids_report;
    }
    else
    {
        return NULL;
    }
}

bool hids_write_report(uint8_t conn_handle, uint8_t service_id, uint8_t report_id,
                       T_PROFILE_HID_REPORT_TYPE report_type, uint16_t length,
                       uint8_t *p_data)
{
    T_HIDS_SERVICE *p_hids_service = NULL;
    T_HIDS_REPORT  *p_hids_report = NULL;
    uint16_t handle = 0;
    T_ATTR_UUID srv_uuid = {0};
    T_ATTR_UUID char_uuid = {0};
    T_GAP_CAUSE ret = GAP_CAUSE_ERROR_UNKNOWN;

    PROTOCOL_PRINT_INFO4("[HIDS] hids_write_report: service id %d, report id %d, report type %d, length %d",
                         service_id, report_id, report_type, length);

    p_hids_service = hids_find_service(conn_handle, service_id);
    if (p_hids_service)
    {
        for (uint8_t i = 0; i < p_hids_service->hids_report_num; i++)
        {
            p_hids_report = hids_find_report(conn_handle, service_id, i);
            if (p_hids_report && (p_hids_report->report_id == report_id) &&
                (p_hids_report->report_type == report_type))
            {
                srv_uuid.is_uuid16 = true;
                srv_uuid.instance_id = service_id;
                srv_uuid.p.uuid16 = GATT_UUID_HIDS;
                char_uuid.is_uuid16 = true;
                char_uuid.instance_id = i;
                char_uuid.p.uuid16 = GATT_UUID_CHAR_REPORT;
                if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
                {
                    ret = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle, length, p_data, NULL);
                    break;
                }
                else
                {
                    return false;
                }
            }
        }
    }

    if (ret != GAP_CAUSE_SUCCESS)
    {
        return false;
    }
    else
    {
        return true;
    }
}

T_APP_RESULT hids_client_read_by_uuid_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                                         void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;
    T_HIDS_REPORT  *p_hids_report = NULL;

    PROTOCOL_PRINT_INFO5("[HIDS] hids_client_read_by_uuid_cb: conn_handle %d, srv_id %d, char_id %d, value_size %d, data %b",
                         conn_handle,
                         p_client_cb_data->read_result.srv_instance_id,
                         p_client_cb_data->read_result.char_uuid.instance_id,
                         p_client_cb_data->read_result.value_size,
                         TRACE_BINARY(p_client_cb_data->read_result.value_size, p_client_cb_data->read_result.p_value));

    p_hids_report = hids_find_report(conn_handle, p_client_cb_data->read_result.srv_instance_id,
                                     p_client_cb_data->read_result.char_uuid.instance_id);
    if (p_hids_report && (p_client_cb_data->read_result.value_size == 2))
    {
        p_hids_report->report_id = *p_client_cb_data->read_result.p_value;
        p_hids_report->report_type = (T_PROFILE_HID_REPORT_TYPE) * (p_client_cb_data->read_result.p_value +
                                                                    1);
    }

    return result;
}

/**
 * @brief HIDS BLE Client Callbacks.
*/
T_APP_RESULT hids_client_cbs(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;
    T_HIDS_CLIENT_CB_DATA cb_data;
    memset(&cb_data, 0, sizeof(T_HIDS_CLIENT_CB_DATA));
    T_HIDS_LINK *p_hids_link = NULL;
    T_ATTR_UUID srv_uuid = {0};
    T_ATTR_UUID char_uuid = {0};
    T_HIDS_SERVICE *p_hids_service = NULL;
    T_HIDS_REPORT  *p_hids_report = NULL;
    uint16_t start_handle;
    uint16_t end_handle;
    uint16_t cccd_handle;
    bool cb_flag = false;

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            PROTOCOL_PRINT_INFO2("[HIDS] GATT_CLIENT_EVENT_DIS_DONE: is_found %d, conn_handle %d",
                                 p_client_cb_data->dis_done.is_found, conn_handle);
            cb_data.cb_type = HIDS_CLIENT_CB_TYPE_DISC_STATE;

            if (p_client_cb_data->dis_done.is_found)
            {
                cb_data.cb_content.disc_state = DISC_HIDS_DONE;
                p_hids_link = hids_alloc_link(conn_handle);
                if (p_hids_link)
                {
                    p_hids_link->hids_service_num = p_client_cb_data->dis_done.srv_instance_num;
                    hids_service_init(p_hids_link, p_hids_link->hids_service_num);
                }
                else
                {
                    PROTOCOL_PRINT_ERROR0("[HIDS] GATT_CLIENT_EVENT_DIS_DONE: Alloc hids mgr struct failed");
                    break;
                }

                srv_uuid.is_uuid16 = true;
                srv_uuid.p.uuid16 = GATT_UUID_HIDS;
                char_uuid.is_uuid16 = true;
                char_uuid.p.uuid16 = GATT_UUID_CHAR_REPORT;
                for (uint8_t i = 0; i < p_hids_link->hids_service_num; i++)
                {
                    srv_uuid.instance_id = i;
                    p_hids_service = hids_find_service(conn_handle, i);
                    if (p_hids_service)
                    {
                        p_hids_service->hids_report_num = gatt_client_get_char_num(conn_handle, &srv_uuid, &char_uuid);
                        hids_report_init(p_hids_service, p_hids_service->hids_report_num);

                        for (uint8_t j = 0; j < p_hids_service->hids_report_num; j++)
                        {
                            char_uuid.instance_id = j;
                            if (gatt_client_find_char_descriptor_range(conn_handle, &srv_uuid, &char_uuid, &start_handle,
                                                                       &end_handle, &cccd_handle))
                            {
                                for (uint16_t read_handle = start_handle; read_handle <= end_handle; read_handle++)
                                {
                                    if (read_handle != cccd_handle)
                                    {
                                        gatt_client_read(conn_handle, read_handle, hids_client_read_by_uuid_cb);
                                    }
                                }
                            }
                        }
                        gatt_client_enable_srv_cccd(conn_handle, &srv_uuid, GATT_CLIENT_CONFIG_ALL);
                    }
                }
                cb_flag = true;
            }
            else
            {
                cb_data.cb_content.disc_state = DISC_HIDS_FAILED;
            }
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:
        {
            PROTOCOL_PRINT_INFO3("[HIDS] GATT_CLIENT_EVENT_NOTIFY_IND: conn_handle 0x%x, uuid 0x%x, data length %d",
                                 conn_handle, p_client_cb_data->notify_ind.char_uuid.p.uuid16,
                                 p_client_cb_data->notify_ind.value_size);
            if (p_client_cb_data->notify_ind.char_uuid.p.uuid16 == GATT_UUID_CHAR_REPORT)
            {
                p_hids_report = hids_find_report(conn_handle, p_client_cb_data->notify_ind.srv_instance_id,
                                                 p_client_cb_data->notify_ind.char_uuid.instance_id);
                if (p_hids_report && p_hids_report->report_type == HID_INPUT_TYPE)
                {
                    cb_flag = true;
                    cb_data.cb_type = HIDS_CLIENT_CB_TYPE_NOTIF_IND_RESULT;
                    cb_data.cb_content.notify_data.report_id = p_hids_report->report_id;
                    cb_data.cb_content.notify_data.data_len = p_client_cb_data->notify_ind.value_size;
                    cb_data.cb_content.notify_data.p_data = p_client_cb_data->notify_ind.p_value;
                }
            }

        }
        break;

    default:
        break;
    }

    if (hids_client_cb && cb_flag)
    {
        (*hids_client_cb)(conn_handle, GATT_UUID_HIDS, &cb_data);
    }

    return result;
}

/**
  * @brief      Add hids client to application.
  * @param[in]  app_cb pointer of app callback function to handle specific client module data.
  * @return Client ID of the specific client module.
  * @retval 0xff failed.
  * @retval other success.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        client_init(1);
        hids_client_id = hids_add_client(app_client_callback);
    }
  * \endcode
  */
bool hids_add_client(P_FUN_CLIENT_UUID16_GENERAL_APP_CB app_cb)
{
    T_ATTR_UUID srv_uuid = {0};
    srv_uuid.is_uuid16 = true;
    srv_uuid.p.uuid16 = GATT_UUID_HIDS;

    if (gatt_client_spec_register(&srv_uuid, hids_client_cbs) == GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        hids_client_cb = app_cb;
        return true;
    }

    return false;
}

