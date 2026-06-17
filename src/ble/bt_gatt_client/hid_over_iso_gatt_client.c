/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include <trace.h>
#include "gatt.h"
#include "hid_over_iso_gatt_client.h"

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

const T_ATTR_UUID hid_over_iso_srv_uuid =
{
    .is_uuid16 = true,
    .instance_id = 0,
    .p.uuid16 = GATT_UUID_HID_OVER_ISO
};

const T_ATTR_UUID hid_over_iso_char_properties =
{
    .is_uuid16 = true,
    .instance_id = 0,
    .p.uuid16 = GATT_UUID_CHAR_HID_OVER_ISO_PROPERTIES
};

const T_ATTR_UUID hid_over_iso_char_mode =
{
    .is_uuid16 = true,
    .instance_id = 0,
    .p.uuid16 = GATT_UUID_CHAR_LE_HID_OPERATION_MODE
};

/**<  Callback used to send data to app from HID over ISO client layer. */
static P_FUN_HID_OVER_ISO_CLIENT_APP_CB hid_over_iso_client_cb = NULL;


bool hid_over_iso_client_cfg_cccd(uint16_t conn_handle, bool enable)
{
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint8_t cccd_cfg;

    if (enable)
    {
        cccd_cfg = GATT_CLIENT_CONFIG_ALL;
    }
    else
    {
        cccd_cfg = GATT_CLIENT_CONFIG_DISABLE;
    }

    if (gatt_client_check_cccd_enabled(conn_handle, (T_ATTR_UUID *)&hid_over_iso_srv_uuid,
                                       (T_ATTR_UUID *)&hid_over_iso_char_mode) != enable)
    {
        cause = gatt_client_enable_char_cccd(conn_handle, (T_ATTR_UUID *)&hid_over_iso_srv_uuid,
                                             (T_ATTR_UUID *)&hid_over_iso_char_mode, cccd_cfg);
    }

    return (cause == GAP_CAUSE_SUCCESS) ? true : false;
}


bool hid_over_iso_client_read_properties(uint16_t conn_handle)
{
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint16_t handle = 0;

    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&hid_over_iso_srv_uuid,
                                     (T_ATTR_UUID *)&hid_over_iso_char_properties, &handle))
    {
        cause = gatt_client_read(conn_handle, handle, NULL);
    }
    return (cause == GAP_CAUSE_SUCCESS) ? true : false;
}

bool hid_over_iso_client_write_le_hid_operation_mode(uint16_t conn_handle,
                                                     T_LE_HID_OPERATION_MODE_VALUE *p_operation)
{
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint16_t handle = 0;
    uint8_t buf[sizeof(T_LE_HID_OPERATION_MODE_VALUE)];
    uint8_t *p = buf;

    if (p_operation == NULL)
    {
        return false;
    }

    LE_UINT8_TO_STREAM(p, p_operation->opcode);

    if (p_operation->opcode == LE_HID_OPERATION_MODE_CHAR_OPCODE_SELECT_HYBRID_OPERATION_MODE)
    {
        LE_UINT8_TO_STREAM(p, p_operation->param.hybrid_param.cig_id);
        LE_UINT8_TO_STREAM(p, p_operation->param.hybrid_param.cis_id);
        LE_UINT16_TO_STREAM(p, p_operation->param.hybrid_param.report_interval);
        memcpy(p, &p_operation->param.hybrid_param.hybrid_mode_iso_reports_enable,
               p_operation->param.hybrid_param.reports_num);
        p += p_operation->param.hybrid_param.reports_num;
    }

    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&hid_over_iso_srv_uuid,
                                     (T_ATTR_UUID *)&hid_over_iso_char_mode, &handle))
    {
        cause = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle, p - buf, buf, NULL);
    }
    return (cause == GAP_CAUSE_SUCCESS) ? true : false;
}

T_APP_RESULT hid_over_iso_client_cbs(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            T_HID_OVER_ISO_CLIENT_DIS_DONE dis_done = {0};
            uint16_t char_properties;
            dis_done.is_found = p_client_cb_data->dis_done.is_found;
            dis_done.load_from_ftl = p_client_cb_data->dis_done.load_from_ftl;

            if (p_client_cb_data->dis_done.is_found)
            {
                if (gatt_client_get_char_prop(conn_handle, (T_ATTR_UUID *)&hid_over_iso_srv_uuid,
                                              (T_ATTR_UUID *)&hid_over_iso_char_mode, &char_properties))
                {
                    if (char_properties & GATT_CHAR_PROP_INDICATE)
                    {
                        dis_done.is_le_hid_operation_mode_indication_support = true;
                    }
                }
            }

            if (hid_over_iso_client_cb)
            {
                result = (* hid_over_iso_client_cb)(conn_handle, GATT_MSG_HID_OVER_ISO_CLIENT_DIS_DONE, &dis_done);
            }
        }
        break;

    case GATT_CLIENT_EVENT_READ_RESULT:
        {
            T_HID_OVER_ISO_CLIENT_READ_PROPERTIES_RESULT read_result = {0};
            read_result.cause = p_client_cb_data->read_result.cause;

            if (read_result.cause == ATT_SUCCESS)
            {
                if (p_client_cb_data->read_result.char_uuid.p.uuid16 == GATT_UUID_CHAR_HID_OVER_ISO_PROPERTIES)
                {
                    if (p_client_cb_data->read_result.value_size >= 7 && p_client_cb_data->read_result.value_size <= 21)
                    {
                        read_result.properties.features = p_client_cb_data->read_result.p_value[0];
                        LE_ARRAY_TO_UINT16(read_result.properties.supported_report_intervals,
                                           p_client_cb_data->read_result.p_value + 1);
                        read_result.properties.max_sdu_size_device_to_host = p_client_cb_data->read_result.p_value[3];
                        read_result.properties.max_sdu_size_host_to_device = p_client_cb_data->read_result.p_value[4];
                        read_result.properties.reports_num = (p_client_cb_data->read_result.value_size - 5) / 2;
                        memcpy(read_result.properties.reports, p_client_cb_data->read_result.p_value + 5,
                               p_client_cb_data->read_result.value_size - 5);
                    }
                    else
                    {
                        result = APP_RESULT_INVALID_VALUE_SIZE;
                        read_result.cause = ATT_ERR_INVALID_VALUE_SIZE;
                    }
                }
            }

            if (hid_over_iso_client_cb)
            {
                (* hid_over_iso_client_cb)(conn_handle, GATT_MSG_HID_OVER_ISO_CLIENT_READ_PROPERTIES_RESULT,
                                           &read_result);
            }
        }
        break;

    case GATT_CLIENT_EVENT_WRITE_RESULT:
        {
            T_HID_OVER_ISO_CLIENT_WRITE_LE_HID_OPERATION_MODE_RESULT write_result = {0};
            write_result.cause = p_client_cb_data->write_result.cause;

            if (hid_over_iso_client_cb)
            {
                (* hid_over_iso_client_cb)(conn_handle,
                                           GATT_MSG_HID_OVER_ISO_CLIENT_WRITE_LE_HID_OPERATION_MODE_RESULT,
                                           &write_result);
            }
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:
        {
            T_HID_OVER_ISO_CLIENT_LE_HID_OPERATION_MODE_INDICATION ind_data = {0};

            if (p_client_cb_data->notify_ind.char_uuid.p.uuid16 == GATT_UUID_CHAR_LE_HID_OPERATION_MODE)
            {
                if (p_client_cb_data->notify_ind.value_size == 0)
                {
                    result = APP_RESULT_INVALID_VALUE_SIZE;
                }
                else
                {
                    ind_data.operation.opcode = p_client_cb_data->notify_ind.p_value[0];
                    if (ind_data.operation.opcode == LE_HID_OPERATION_MODE_CHAR_OPCODE_SELECT_HYBRID_OPERATION_MODE)
                    {
                        if (p_client_cb_data->notify_ind.value_size < 6 || p_client_cb_data->notify_ind.value_size > 13)
                        {
                            result = APP_RESULT_INVALID_VALUE_SIZE;
                        }
                        else
                        {
                            ind_data.operation.param.hybrid_param.cig_id = p_client_cb_data->notify_ind.p_value[1];
                            ind_data.operation.param.hybrid_param.cis_id = p_client_cb_data->notify_ind.p_value[2];
                            LE_ARRAY_TO_UINT16(ind_data.operation.param.hybrid_param.report_interval,
                                               p_client_cb_data->notify_ind.p_value + 3);
                            ind_data.operation.param.hybrid_param.reports_num = p_client_cb_data->notify_ind.value_size - 5;
                            memcpy(ind_data.operation.param.hybrid_param.hybrid_mode_iso_reports_enable,
                                   p_client_cb_data->notify_ind.p_value + 5,
                                   ind_data.operation.param.hybrid_param.reports_num);
                        }
                    }
                    else if (ind_data.operation.opcode ==
                             LE_HID_OPERATION_MODE_CHAR_OPCODE_SELECT_DEFAULT_OPERATION_MODE)
                    {

                    }
                    else
                    {
                        result = (T_APP_RESULT)ATT_ERR_HID_OVER_ISO_OPCODE_OUTSIDE_RANGE;
                    }
                }
            }

            if (hid_over_iso_client_cb && (result == APP_RESULT_SUCCESS))
            {
                result = (* hid_over_iso_client_cb)(conn_handle,
                                                    GATT_MSG_HID_OVER_ISO_CLIENT_LE_HID_OPERATION_MODE_INDICATION, &ind_data);
            }
        }
        break;

    case GATT_CLIENT_EVENT_CCCD_CFG:
        {
            T_HID_OVER_ISO_CLIENT_CCCD_CFG_RESULT cccd_info = {0};

            cccd_info.cause = p_client_cb_data->cccd_cfg.cause;
            cccd_info.cccd_cfg = p_client_cb_data->cccd_cfg.cccd_cfg;

            if (hid_over_iso_client_cb)
            {
                result = (* hid_over_iso_client_cb)(conn_handle, GATT_MSG_HID_OVER_ISO_CLIENT_CCCD_CFG_RESULT,
                                                    &cccd_info);
            }
        }
        break;

    default:
        break;
    }

    return result;
}


bool hid_over_iso_client_init(P_FUN_HID_OVER_ISO_CLIENT_APP_CB app_cb)
{
    if (gatt_client_spec_register((T_ATTR_UUID *)&hid_over_iso_srv_uuid,
                                  hid_over_iso_client_cbs) == GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        hid_over_iso_client_cb = app_cb;
        return true;
    }

    return false;
}

