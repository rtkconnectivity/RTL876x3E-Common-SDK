/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "app_ble_client.h"
#include "ancs_gatt_client.h"
#include "app_main.h"
#include "app_report.h"
#include "app_cmd.h"
#include "bt_gatt_client.h"

#if F_APP_BLE_AMS_CLIENT_SUPPORT
#include "ams.h"
#endif

#if F_APP_BT_ANCS_CLIENT_SUPPORT
#include "ancs.h"
#endif

#if BLE_HID_CLIENT_SUPPORT
#include "hids_gatt_client.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include <string.h>
#include "ftl.h"
#include "bas_gatt_client.h"
#include "dis_gatt_client.h"
#include "app_tri_dongle_mgr.h"
#include "app_tri_dongle_usb_hid_ctrl.h"
#include "app_tri_dongle_bond_manager.h"
#include "app_tri_dongle_cmd.h"
#include "ota_gatt_client.h"
#include "dfu_gatt_client.h"
#include "app_dfu_ota_client.h"
#include "gaps_gatt_client.h"
#include "uwb_vendor_client.h"
#include "app_report.h"
#endif

#if F_APP_BLE_HID_HOST_SUPPORT
#include "app_ble_hid_host.h"
#endif

#if BLE_HID_CLIENT_SUPPORT
static uint16_t hid_conn_handle;

T_APP_RESULT app_ble_hid_service_cback(uint16_t conn_handle, uint8_t type,
                                       void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    if (type == GATT_MSG_HIDS_CLIENT_DIS_DONE)
    {
        T_HIDS_CLIENT_DIS_DONE *p_dis_done = (T_HIDS_CLIENT_DIS_DONE *)p_data;
        APP_PRINT_TRACE1("app_ble_hid_service_cback: discover state %d",
                         p_dis_done->is_found);
        if (p_dis_done->is_found)
        {
            hid_conn_handle = conn_handle;

            for (uint8_t i = 0; i < p_dis_done->srv_instance_num; i++)
            {
                hids_client_read_report_map_value(conn_handle, i);
            }
        }
    }
    else if (type == GATT_MSG_HIDS_CLIENT_READ_RESULT)
    {
        T_HIDS_CLIENT_READ_RESULT *p_read_result = (T_HIDS_CLIENT_READ_RESULT *)p_data;
        APP_PRINT_TRACE1("app_ble_hid_service_cback: char_uuid 0x%x",
                         p_read_result->char_uuid);

        if (p_read_result->char_uuid == GATT_UUID_CHAR_REPORT_MAP)
        {
#if F_APP_BLE_HID_HOST_SUPPORT
            app_ble_hid_host_read_report_map_handle(conn_handle,
                                                    p_read_result->data.hids_report_map.p_hids_report_map,
                                                    p_read_result->data.hids_report_map.hids_report_map_len);
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            app_tri_dongle_usb_hid_recfg_ble_report_desc(conn_handle,
                                                         p_read_result->data.hids_report_map.p_hids_report_map,
                                                         p_read_result->data.hids_report_map.hids_report_map_len);
#endif
        }
        else if (p_read_result->char_uuid == GATT_UUID_CHAR_REPORT)
        {
#if F_APP_BLE_HID_HOST_SUPPORT
            app_ble_hid_host_read_feature_report_handle(conn_handle,
                                                        p_read_result->data.hids_report.p_hids_report_value,
                                                        p_read_result->data.hids_report.hids_report_value_len,
                                                        p_read_result->data.hids_report.report_id);
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            app_tri_dongle_usb_hid_read_ble_feature_report_handle(conn_handle,
                                                                  p_read_result->data.hids_report.p_hids_report_value,
                                                                  p_read_result->data.hids_report.hids_report_value_len,
                                                                  p_read_result->data.hids_report.report_id);
#endif
        }
    }
    else if (type == GATT_MSG_HIDS_CLIENT_NOTIFY_IND)
    {
        T_HIDS_CLIENT_NOTIFY_DATA *p_notify_data = (T_HIDS_CLIENT_NOTIFY_DATA *)p_data;
#if F_APP_BLE_HID_HOST_SUPPORT
        app_ble_hid_host_inreport_handle(conn_handle, p_notify_data->p_data,
                                         p_notify_data->data_len, p_notify_data->report_id);
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
        app_tri_dongle_usb_ble_hid_inreport_handle(conn_handle, p_notify_data->p_data,
                                                   p_notify_data->data_len, p_notify_data->report_id);
#endif
    }
    return app_result;
}

uint16_t app_ble_hid_get_conn_handle(void)
{
    return hid_conn_handle;
}
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
T_APP_RESULT app_bas_client_callback(uint16_t conn_handle, uint8_t type, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    if (type == GATT_MSG_BAS_CLIENT_DIS_DONE)
    {
        T_BAS_CLIENT_DIS_DONE *p_bas_done = (T_BAS_CLIENT_DIS_DONE *)p_data;
        APP_PRINT_INFO4("app_bas_client_callback DIS_DONE: conn_handle 0x%x, is_found %d, load_from_ftl %d, srv_instance_num %d",
                        conn_handle, p_bas_done->is_found, p_bas_done->load_from_ftl, p_bas_done->srv_instance_num);

        T_APP_LE_LINK *p_link;
        uint8_t conn_id;
        le_get_conn_id_by_handle(conn_handle, &conn_id);
        p_link = app_link_find_le_link_by_conn_id(conn_id);
        if (p_link)
        {
            p_link->bas_client_instance_num = p_bas_done->srv_instance_num;
        }
    }
    else if (type == GATT_MSG_BAS_CLIENT_READ_BATTERY_LEVEL_RESULT)
    {
        T_BAS_CLIENT_READ_BATTERY_LEVEL_RESULT *p_write_result = (T_BAS_CLIENT_READ_BATTERY_LEVEL_RESULT *)
                                                                 p_data;
        app_tri_dongle_notify_bas_info(conn_handle, p_write_result->battery_level);
        app_report_event(CMD_PATH_UART, EVENT_LE_BAS_INFO_REPORT, 0, &p_write_result->battery_level,
                         sizeof(p_write_result->battery_level));
    }
    else if (type == GATT_MSG_BAS_CLIENT_BATTERY_LEVEL_NOTIFY)
    {
        T_BAS_CLIENT_BATTERY_LEVEL_NOTIFY *p_notify_result = (T_BAS_CLIENT_BATTERY_LEVEL_NOTIFY *)
                                                             p_data;
        app_tri_dongle_notify_bas_info(conn_handle, p_notify_result->battery_level);
    }

    return app_result;
}

T_APP_RESULT app_dis_client_callback(uint16_t conn_handle, uint8_t type, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    if (type == GATT_MSG_DIS_CLIENT_DIS_DONE)
    {
        T_DIS_CLIENT_DIS_DONE *p_dis_done = (T_DIS_CLIENT_DIS_DONE *)p_data;
        APP_PRINT_INFO4("app_dis_client_callback DIS_DONE: conn_handle 0x%x, is_found %d, load_from_ftl %d, srv_instance_num %d",
                        conn_handle, p_dis_done->is_found, p_dis_done->load_from_ftl, p_dis_done->srv_instance_num);

        T_APP_LE_LINK *p_link;
        uint8_t conn_id;
        le_get_conn_id_by_handle(conn_handle, &conn_id);
        p_link = app_link_find_le_link_by_conn_id(conn_id);
        if (p_link)
        {
            p_link->dis_client_instance_num = p_dis_done->srv_instance_num;
        }
    }
    else if (type == GATT_MSG_DIS_CLIENT_READ_RESULT)
    {
        T_DIS_CLIENT_READ_RESULT *p_read_result = (T_DIS_CLIENT_READ_RESULT *)p_data;

        if (p_read_result->cause != ATT_SUCCESS)
        {
            uint8_t conn_id = CONN_ID_DEFAULT_AND_GET_FAIL;
            le_get_conn_id_by_handle(conn_handle, &conn_id);

            if (conn_id != CONN_ID_DEFAULT_AND_GET_FAIL)
            {
                uint8_t bd_addr[6];
                uint8_t bd_type;
                le_get_conn_addr(conn_id, bd_addr, &bd_type);

                app_tri_dongle_notify_ble_dis_not_support(bd_addr, p_read_result->char_uuid);
            }
        }
        else
        {
            switch (p_read_result->char_uuid)
            {
            case GATT_UUID_CHAR_MODEL_NUMBER:
            case GATT_UUID_CHAR_SERIAL_NUMBER:
            case GATT_UUID_CHAR_FIRMWARE_REVISION:
            case GATT_UUID_CHAR_HARDWARE_REVISION:
            case GATT_UUID_CHAR_SOFTWARE_REVISION:
            case GATT_UUID_CHAR_MANUFACTURER_NAME:
                {
                    app_tri_dongle_notify_ble_dis_info_report(conn_handle,
                                                              p_read_result->char_uuid,
                                                              p_read_result->value_size,
                                                              p_read_result->data.char_utf8_string);
                }
                break;

            case GATT_UUID_CHAR_SYSTEM_ID:
                {
                    uint8_t sys_id_data[p_read_result->value_size];
                    memset(sys_id_data, 0, p_read_result->value_size);
                    memcpy(&sys_id_data[0], &p_read_result->data.system_id.mid[0], 5);
                    memcpy(&sys_id_data[5], &p_read_result->data.system_id.oui[0], 3);
                    app_tri_dongle_notify_ble_dis_info_report(conn_handle,
                                                              p_read_result->char_uuid,
                                                              p_read_result->value_size,
                                                              (uint8_t *)&sys_id_data);
                }
                break;

            case GATT_UUID_CHAR_IEEE_CERTIF_DATA_LIST:
                {
                    app_tri_dongle_notify_ble_dis_info_report(conn_handle,
                                                              p_read_result->char_uuid,
                                                              p_read_result->value_size,
                                                              p_read_result->data.ieee_certif_data_list);
                }
                break;

            case GATT_UUID_CHAR_PNP_ID:
                {
                    struct
                    {
                        uint16_t    vendor_id;
                        uint16_t    product_id;
                        uint16_t    product_version;
                        uint8_t     vendor_id_source;
                    } pnp_id_data = {};

                    pnp_id_data.vendor_id = p_read_result->data.pnp_id.vendor_id;
                    pnp_id_data.product_id = p_read_result->data.pnp_id.product_id;
                    pnp_id_data.product_version = p_read_result->data.pnp_id.product_version;
                    pnp_id_data.vendor_id_source = p_read_result->data.pnp_id.vendor_id_source;

                    app_tri_dongle_notify_ble_dis_info_report(conn_handle,
                                                              p_read_result->char_uuid,
                                                              p_read_result->value_size,
                                                              (uint8_t *)&pnp_id_data);
                }
                break;

            case GATT_UUID_CHAR_UDI_FOR_MEDICAL_DEVICES:
                {
                    app_tri_dongle_notify_ble_dis_info_report(conn_handle,
                                                              p_read_result->char_uuid,
                                                              p_read_result->value_size,
                                                              p_read_result->data.udi_for_medical_dev);
                }
                break;

            default:
                break;
            }
        }
    }

    return app_result;
}

T_APP_RESULT app_gaps_gatt_client_callback(uint16_t conn_handle, uint8_t type, void *p_data)
{
    if (type == GATT_MSG_GAPS_CLIENT_DIS_DONE)
    {
        T_GAPS_CLIENT_DIS_DONE *p_dis = (T_GAPS_CLIENT_DIS_DONE *)p_data;
        APP_PRINT_INFO3("GATT_MSG_GAPS_CLIENT_DIS_DONE: conn_handle 0x%x, is_found %d, load_from_ftl %d",
                        conn_handle, p_dis->is_found, p_dis->load_from_ftl);
        gaps_client_read_char(conn_handle, GATT_UUID_CHAR_DEVICE_NAME);
        gaps_client_read_char(conn_handle, GATT_UUID_CHAR_APPEARANCE);
    }
    else if (type == GATT_MSG_GAPS_CLIENT_READ_RESULT)
    {
        T_GAPS_CLIENT_READ_RESULT *p_read = (T_GAPS_CLIENT_READ_RESULT *)p_data;

        if (p_read->cause == APP_RESULT_SUCCESS)
        {
            switch (p_read->char_uuid)
            {
            case GATT_UUID_CHAR_DEVICE_NAME:
                {
                    app_bond_mgr_store_refresh_lea_dev_name(conn_handle,
                                                            p_read->value_len,
                                                            p_read->data.p_device_name);
                }
                break;

            case GATT_UUID_CHAR_APPEARANCE:
                {
                    app_bond_mgr_store_refresh_lea_dev_type(conn_handle, p_read->data.device_appearance);
                }
                break;

            case GATT_UUID_CHAR_PER_PREF_CONN_PARAM:
                {
                }
                break;

            case GATT_UUID_CHAR_CENTRAL_ADDRESS_RESOLUTION:
                {
                }
                break;

            case GATT_UUID_CHAR_RESOLVABLE_PRIVATE_ADDRESS_ONLY:
                {
                }
                break;

            default:
                break;
            }
        }
        else
        {
            switch (p_read->char_uuid)
            {
            case GATT_UUID_CHAR_APPEARANCE:
                {
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
                    app_bond_mgr_store_unsupport_lea_group_info(conn_handle, APP_TRI_DONGLE_LEA_UNSUPPORT_DEV_TYPE);
#endif
                }
                break;

            default:
                break;
            }
        }
    }
    else if (type == GATT_MSG_GAPS_CLIENT_WRITE_DEVICE_NAME_RESULT)
    {
        T_GAPS_CLIENT_WRITE_DEVICE_NAME_RESULT *p_write = (T_GAPS_CLIENT_WRITE_DEVICE_NAME_RESULT *)p_data;
    }
    else if (type == GATT_MSG_GAPS_CLIENT_WRITE_APPEARANCE_RESULT)
    {
        T_GAPS_CLIENT_WRITE_APPEARANCE_RESULT *p_write = (T_GAPS_CLIENT_WRITE_APPEARANCE_RESULT *)p_data;
    }

    return APP_RESULT_SUCCESS;
}

T_APP_RESULT app_gaps_uwb_client_callback(uint16_t conn_handle, uint8_t type, void *p_data)
{
    if (type == GATT_MSG_UWB_CLIENT_DIS_DONE)
    {
        T_UWB_VENODR_CLIENT_UWB_DONE *p_dis = (T_UWB_VENODR_CLIENT_UWB_DONE *)p_data;
        APP_PRINT_INFO4("GATT_MSG_GAPS_CLIENT_UWB_DONE: conn_handle 0x%x, is_found %d, load_from_ftl %d, srv_instance_num %d",
                        conn_handle, p_dis->is_found, p_dis->load_from_ftl, p_dis->srv_instance_num);
    }
    else if (type == GATT_MSG_UWB_CLIENT_READ_RESULT)
    {
        T_UWB_CLIENT_READ_RESULT *p_read = (T_UWB_CLIENT_READ_RESULT *)p_data;
    }
    else if (type == GATT_MSG_UWB_CLIENT_WRITE_RESULT)
    {
        T_UWB_CLIENT_WRITE_RESULT *write_result = (T_UWB_CLIENT_WRITE_RESULT *)p_data;
    }
    else if (type == GATT_MSG_UWB_CLIENT_NOTIFY_IND)
    {
        T_UWB_CLIENT_NOTIFY_DATA *p_notify = (T_UWB_CLIENT_NOTIFY_DATA *)p_data;
#if F_APP_UWB_SCENARIO_SUPPORT
        app_tri_dongle_uwb_client_vendor_cmd(conn_handle,
                                             p_notify->value_size,
                                             p_notify->p_value);
#endif
    }

    return APP_RESULT_SUCCESS;
}
#endif

void app_ble_client_init(void)
{
    gatt_client_init(GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT |
                     GATT_CLIENT_DISCOV_MODE_CCCD_STORAGE_BIT |
                     GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT |
                     GATT_CLIENT_DISCOV_MODE_GATT_SVC);

    gatt_client_cfg_client_supported_feature(GATT_SVC_CLIENT_SUPPORTED_FEATURES_ROBUST_CACHING_BIT);

    T_GATT_CLIENT_PENDING_NUM num;
    num.write_cmd_num = 20;
    gatt_client_cfg_pending_num(num);

#if F_APP_BLE_AMS_CLIENT_SUPPORT
    ams_init();
#endif

#if F_APP_BT_ANCS_CLIENT_SUPPORT
    app_ancs_client_init();
#endif

#if BLE_HID_CLIENT_SUPPORT
    hids_add_client(app_ble_hid_service_cback, MAX_BLE_LINK_NUM);
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    bas_client_init(app_bas_client_callback);
    dis_client_init(app_dis_client_callback);
    ota_client_init(app_ota_client_callback);
    dfu_client_init(app_dfu_client_callback);
    gaps_client_init(app_gaps_gatt_client_callback);
#if F_APP_UWB_SCENARIO_SUPPORT
    uwb_vendor_client_init(app_gaps_uwb_client_callback);
#endif
#endif
}
