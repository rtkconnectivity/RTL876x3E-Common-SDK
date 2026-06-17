/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "gattc_tbl_storage.h"
#include "bt_gatt_client.h"
#include "app_lea_cap_com_main.h"
#include "app_lea_cap_com_cap.h"
#include "app_lea_cap_com_bap.h"

void app_lea_profile_init(void)
{
#if GATTC_TBL_STORAGE_SUPPORT
    gattc_tbl_storage_init();
#endif
    T_BLE_AUDIO_PARAMS ble_audio_param = {0};
    ble_audio_param.evt_queue_handle = app_evt_queue_handle;
    ble_audio_param.io_queue_handle = app_io_queue_handle;
    ble_audio_param.bt_gatt_client_init = (GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT |
                                           GATT_CLIENT_DISCOV_MODE_CCCD_STORAGE_BIT |
                                           GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT);
    ble_audio_param.acl_link_num = APP_MAX_BLE_LINK_NUM;
    ble_audio_param.io_event_type = IO_MSG_TYPE_LE_AUDIO;
    ble_audio_init(&ble_audio_param);
    app_lea_com_bap_init();
    app_lea_com_cap_init();
}
