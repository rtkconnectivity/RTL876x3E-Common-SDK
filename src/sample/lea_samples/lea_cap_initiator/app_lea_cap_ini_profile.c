/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "ble_audio.h"
#include "gattc_tbl_storage.h"
#include "bt_gatt_client.h"
#include "app_lea_cap_ini_main.h"
#include "app_lea_cap_ini_cap.h"
#include "app_lea_cap_ini_bap.h"
#include "app_lea_cap_ini_audio_data.h"
#include "app_lea_cap_ini_mcp.h"
#include "app_lea_cap_ini_ccp.h"

#define MAX_BLE_SRV_NUM                           2

void app_lea_profile_init(void)
{
#if (CCP_CALL_CONTROL_SERVER || MCP_MEDIA_CONTROL_SERVER)
    gatt_svc_init(GATT_SVC_USE_EXT_SERVER, MAX_BLE_SRV_NUM);
    T_GATT_SVC_PENDING_NUM num;
    num.notify_num = 20;
    num.ind_num = 10;
    gatt_svc_cfg_pending_num(num);
#endif

#if GATTC_TBL_STORAGE_SUPPORT
    gattc_tbl_storage_init();
#endif
    T_BLE_AUDIO_PARAMS ble_audio_param = {0};
    ble_audio_param.evt_queue_handle = app_evt_queue_handle;
    ble_audio_param.io_queue_handle = app_io_queue_handle;
    ble_audio_param.bt_gatt_client_init = (GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT |
                                           GATT_CLIENT_DISCOV_MODE_CCCD_STORAGE_BIT |
                                           GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT |
                                           GATT_CLIENT_DISCOV_MODE_GATT_SVC);
#if APP_LEA_EATT_SUPPORT
    gatt_client_cfg_client_supported_feature(GATT_SVC_CLIENT_SUPPORTED_FEATURES_EATT_BEARER_BIT);
#endif
    ble_audio_param.acl_link_num = APP_MAX_BLE_LINK_NUM;
    ble_audio_param.io_event_type = IO_MSG_TYPE_LE_AUDIO;
    ble_audio_init(&ble_audio_param);
    app_lea_ini_bap_init();
    app_lea_ini_cap_init();
#if MCP_MEDIA_CONTROL_SERVER
    app_lea_ini_mcp_init();
#endif
#if CCP_CALL_CONTROL_SERVER
    app_lea_ini_ccp_init();
#endif
    app_lea_audio_data_init();
}
