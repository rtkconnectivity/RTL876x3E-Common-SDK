/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "bt_gatt_svc.h"
#include "ble_audio.h"
#include "gattc_tbl_storage.h"
#include "bt_gatt_client.h"
#include "app_lea_cap_acc_main.h"
#include "app_lea_cap_acc_bap.h"
#include "app_lea_cap_acc_vc_mic.h"
#include "app_lea_cap_acc_csis.h"
#include "app_lea_cap_acc_mcp.h"
#include "app_lea_cap_acc_audio_data.h"
#include "app_lea_cap_acc_ccp.h"

#define MAX_BLE_SRV_NUM                           16

void app_lea_acc_cap_init(void)
{
    T_CAP_INIT_PARAMS cap_init_param = {0};

    cap_init_param.cap_role = CAP_ROLE;
    cap_init_param.cas.enable = true;
#if CSIP_SET_MEMBER
    if (app_db.csis_cfg != APP_LEA_CSIS_CFG_NOT_EXIST)
    {
        app_lea_acc_csis_init(&cap_init_param);
    }
#endif
#if MCP_MEDIA_CONTROL_CLIENT
    app_lea_acc_mcp_init_cap(&cap_init_param);
#endif
#if CCP_CALL_CONTROL_CLIENT
    app_lea_acc_ccp_init_cap(&cap_init_param);
#endif
    cap_init(&cap_init_param);
}

void app_lea_profile_init(void)
{
    gatt_svc_init(GATT_SVC_USE_EXT_SERVER, MAX_BLE_SRV_NUM);
    T_GATT_SVC_PENDING_NUM num;
    num.notify_num = 20;
    num.ind_num = 10;
    gatt_svc_cfg_pending_num(num);

#if GATTC_TBL_STORAGE_SUPPORT
    gattc_tbl_storage_init();
#endif
    T_BLE_AUDIO_PARAMS ble_audio_param = {0};

    ble_audio_param.evt_queue_handle = app_evt_queue_handle;
    ble_audio_param.io_queue_handle = app_io_queue_handle;
#if APP_LEA_EATT_SUPPORT
    ble_audio_param.bt_gatt_client_init = (GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT |
                                           GATT_CLIENT_DISCOV_MODE_CCCD_STORAGE_BIT |
                                           GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT |
                                           GATT_CLIENT_DISCOV_MODE_GATT_SVC);
    gatt_client_cfg_client_supported_feature(GATT_SVC_CLIENT_SUPPORTED_FEATURES_EATT_BEARER_BIT);
#else
    ble_audio_param.bt_gatt_client_init = (GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT |
                                           GATT_CLIENT_DISCOV_MODE_CCCD_STORAGE_BIT |
                                           GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT);
#endif
    ble_audio_param.acl_link_num = APP_MAX_BLE_LINK_NUM;
    ble_audio_param.io_event_type = IO_MSG_TYPE_LE_AUDIO;
    ble_audio_init(&ble_audio_param);
    app_lea_acc_vc_mic_init();
    app_lea_acc_bap_init();
    app_lea_acc_cap_init();
    app_lea_audio_data_init();
}
