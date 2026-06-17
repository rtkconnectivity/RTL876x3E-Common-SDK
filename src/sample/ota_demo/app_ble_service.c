/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "os_mem.h"
#include "dfu_api.h"
#include "ota_service.h"
#include "app_ble_service.h"
#include "app_main.h"
#include "app_ble_gap.h"
#include "bt_gatt_svc.h"

static T_APP_RESULT app_ble_service_ota_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_OTA_CALLBACK_DATA *p_ota_cb_data = (T_OTA_CALLBACK_DATA *)p_data;
    APP_PRINT_INFO2("app_ble_service_ota_srv_cb: service_id %d, msg_type %d",
                    service_id, p_ota_cb_data->msg_type);
    switch (p_ota_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        break;
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        if (OTA_WRITE_CHAR_VAL == p_ota_cb_data->msg_data.write.opcode &&
            OTA_VALUE_ENTER == p_ota_cb_data->msg_data.write.value)
        {
            /* Check battery level first */
            if (app_db.local_batt_level >= 30)
            {
                T_APP_LE_LINK *p_link;
                p_link = app_link_find_le_link_by_conn_id(p_ota_cb_data->conn_id);
                /* Battery level is greater than or equal to 30 percent */
                if (p_link != NULL)
                {
                    app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_SWITCH_TO_OTA);
                }
                APP_PRINT_INFO1("app_ble_service_ota_srv_cb: Preparing switch into OTA mode conn_id %d",
                                p_ota_cb_data->conn_id);
            }
            else
            {
                /* Battery level is less than 30 percent */
                APP_PRINT_WARN1("app_ble_service_ota_srv_cb: Battery level is not enough to support OTA, local_batt_level %d",
                                app_db.local_batt_level);
            }
        }
        break;

    default:

        break;
    }

    return app_result;
}

void app_ble_service_init(void)
{
    /** NOTES: 1 includes ota service.
     *  if more ble service are added, you need to modify this value.
     * */
#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    server_cfg_use_ext_api(true);
    APP_PRINT_INFO0("app_ble_service_init: server_cfg_use_ext_api true");
    server_ext_register_app_cb(app_ble_service_ota_srv_cb);
#else
    server_register_app_cb(app_ble_service_ota_srv_cb);
#endif

    server_init(1);

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    gatt_svc_init(GATT_SVC_USE_EXT_SERVER, 0);
#else
    gatt_svc_init(GATT_SVC_USE_NORMAL_SERVER, 0);
#endif

    ota_add_service(app_ble_service_ota_srv_cb);
}
