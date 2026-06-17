/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "stdlib.h"
#include "asp_gatt_client.h"
#include <trace.h>
#include <os_mem.h>

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

uint8_t asp_service_uuid[16] = {0xFD, 0x63, 0x1C, 0x02, 0xC6, 0xAE, 0x27, 0x93, 0x76, 0x4E, 0x21, 0xAB, 0x00, 0x00, 0x46, 0x7A};
uint8_t asp_service_char_9a_uuid[16] = {0xFD, 0x63, 0x1C, 0x02, 0xC6, 0xAE, 0x27, 0x93, 0x76, 0x4E, 0x21, 0xAB, 0x01, 0x00, 0x46, 0x7A};
uint8_t asp_service_char_9b_uuid[16] = {0xFD, 0x63, 0x1C, 0x02, 0xC6, 0xAE, 0x27, 0x93, 0x76, 0x4E, 0x21, 0xAB, 0x02, 0x00, 0x46, 0x7A};

static T_ASP_LINK asp_client_table[ASP_MAX_LINKS];

/**<  Callback used to send data to app from ASP client layer. */
static P_FUN_CLIENT_UUID128_GENERAL_APP_CB asp_client_cb = NULL;

T_ASP_LINK *asp_find_link_by_conn_handle(uint8_t conn_handle)
{
    for (uint8_t i = 0; i < ASP_MAX_LINKS; i++)
    {
        if (asp_client_table[i].used && (asp_client_table[i].conn_handle == conn_handle))
        {
            return &asp_client_table[i];
        }
    }

    return NULL;
}

T_ASP_LINK *asp_alloc_link(uint8_t conn_handle)
{
    T_ASP_LINK *p_asp_link = NULL;
    p_asp_link = asp_find_link_by_conn_handle(conn_handle);
    if (p_asp_link)
    {
        return p_asp_link;
    }

    for (uint8_t i = 0; i < ASP_MAX_LINKS; i++)
    {
        if (asp_client_table[i].used == false)
        {
            asp_client_table[i].used = true;
            asp_client_table[i].conn_handle = conn_handle;
            return &asp_client_table[i];
        }
    }

    return NULL;
}

void asp_service_init(T_ASP_LINK *p_asp_link, uint8_t service_num)
{
    T_ASP_SERVICE *p_service_info = NULL;

    if (!service_num)
    {
        return;
    }

    PROTOCOL_PRINT_TRACE1("[ASP] asp_service_init: service num %d", service_num);

    p_asp_link->asp_service_info = (T_ASP_SERVICE *)calloc(1, sizeof(T_ASP_SERVICE));
    if (!p_asp_link->asp_service_info)
    {
        return;
    }
    p_service_info = p_asp_link->asp_service_info;

    for (uint8_t i = 1; i < service_num; i++)
    {
        p_service_info->p_next = (T_ASP_SERVICE *)calloc(1, sizeof(T_ASP_SERVICE));
        if (!p_service_info->p_next)
        {
            return;
        }
        p_service_info = p_service_info->p_next;
        p_service_info->service_id = i;
    }
}

T_ASP_SERVICE  *asp_find_service(uint8_t conn_handle, uint8_t service_id)
{
    T_ASP_LINK *p_asp_link = NULL;
    T_ASP_SERVICE *p_asp_service = NULL;
    for (uint8_t i = 0; i < ASP_MAX_LINKS; i++)
    {
        if (asp_client_table[i].used && (asp_client_table[i].conn_handle == conn_handle))
        {
            p_asp_link = &asp_client_table[i];
            break;
        }
    }

    if (p_asp_link)
    {
        p_asp_service = p_asp_link->asp_service_info;
        while (p_asp_service)
        {
            if (p_asp_service->service_id == service_id)
            {
                return p_asp_service;
            }
            p_asp_service = p_asp_service->p_next;
        }
    }
    return NULL;
}

bool asp_write_data(uint16_t conn_handle, uint8_t service_id, uint16_t length, uint8_t *p_data)
{
    T_ASP_SERVICE *p_asp_service = NULL;
    uint16_t handle = 0;
    T_ATTR_UUID srv_uuid = {0};
    T_ATTR_UUID char_uuid = {0};
    T_GAP_CAUSE ret = GAP_CAUSE_ERROR_UNKNOWN;

    p_asp_service = asp_find_service(conn_handle, service_id);
    if (p_asp_service)
    {
        srv_uuid.is_uuid16 = false;
        srv_uuid.instance_id = service_id;
        memcpy(srv_uuid.p.uuid128, asp_service_uuid, 16);
        char_uuid.is_uuid16 = false;
        char_uuid.instance_id = 0;  //every asp service only has one 9A char
        memcpy(char_uuid.p.uuid128, asp_service_char_9a_uuid, 16);
        if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
        {
            ret = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle, length, p_data, NULL);
        }
        else
        {
            return false;
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

/**
 * @brief ASP BLE Client Callbacks.
*/
T_APP_RESULT asp_client_cbs(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;
    T_ASP_CLIENT_CB_DATA cb_data;
    memset(&cb_data, 0, sizeof(T_ASP_CLIENT_CB_DATA));
    T_ASP_LINK *p_asp_link = NULL;
    bool cb_flag = false;
    T_ATTR_UUID srv_uuid = {0};
    uint8_t service_uuid[16] = {0xFD, 0x63, 0x1C, 0x02, 0xC6, 0xAE, 0x27, 0x93, 0x76, 0x4E, 0x21, 0xAB, 0x00, 0x00, 0x46, 0x7A};

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            PROTOCOL_PRINT_INFO2("[ASP] GATT_CLIENT_EVENT_DIS_DONE: is_found %d, service num %d",
                                 p_client_cb_data->dis_done.is_found, p_client_cb_data->dis_done.srv_instance_num);
            cb_data.cb_type = ASP_CLIENT_CB_TYPE_DISC_STATE;

            if (p_client_cb_data->dis_done.is_found)
            {
                cb_data.cb_content.disc_state = DISC_ASP_DONE;
                p_asp_link = asp_alloc_link(conn_handle);
                if (p_asp_link)
                {
                    p_asp_link->asp_service_num = p_client_cb_data->dis_done.srv_instance_num;
                    asp_service_init(p_asp_link, p_asp_link->asp_service_num);
                }
                srv_uuid.is_uuid16 = false;
                srv_uuid.instance_id = 0;
                memcpy(srv_uuid.p.uuid128, service_uuid, 16);
                gatt_client_enable_srv_cccd(conn_handle, &srv_uuid, GATT_CLIENT_CONFIG_ALL);
                cb_flag = true;
            }
            else
            {
                cb_data.cb_content.disc_state = DISC_ASP_FAILED;
            }
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:
        {
            PROTOCOL_PRINT_INFO3("[ASP] GATT_CLIENT_EVENT_NOTIFY_IND: conn_handle 0x%x, uuid 0x%x, data length %d",
                                 conn_handle, p_client_cb_data->notify_ind.char_uuid.p.uuid128,
                                 p_client_cb_data->notify_ind.value_size);
            cb_data.cb_type = ASP_CLIENT_CB_TYPE_NOTIF_IND_RESULT;
            cb_data.cb_content.notify_data.data_len = p_client_cb_data->notify_ind.value_size;
            cb_data.cb_content.notify_data.p_data = p_client_cb_data->notify_ind.p_value;
            if (memcmp(p_client_cb_data->notify_ind.char_uuid.p.uuid128, asp_service_char_9b_uuid, 16) == 0)
            {
                cb_flag = true;
                cb_data.cb_content.notify_data.notify_type = ASP_NOTIFY_9B;
            }
            else if (memcmp(p_client_cb_data->notify_ind.char_uuid.p.uuid128, asp_service_char_9a_uuid,
                            16) == 0)
            {
                cb_flag = true;
                cb_data.cb_content.notify_data.notify_type = ASP_NOTIFY_9A;
            }
            else
            {
                break;
            }
        }
        break;

    default:
        break;
    }

    if (asp_client_cb && cb_flag)
    {
        (*asp_client_cb)(conn_handle, service_uuid, &cb_data);
    }

    return result;
}

/**
  * @brief      Add asp client to application.
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
        asp_client_id = asp_add_client(app_client_callback);
    }
  * \endcode
  */
bool asp_add_client(P_FUN_CLIENT_UUID128_GENERAL_APP_CB app_cb)
{
    T_ATTR_UUID srv_uuid = {0};
    srv_uuid.is_uuid16 = false;
    memcpy(srv_uuid.p.uuid128, asp_service_uuid, 16);

    if (gatt_client_spec_register(&srv_uuid, asp_client_cbs) == GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        asp_client_cb = app_cb;
        return true;
    }

    return false;
}

