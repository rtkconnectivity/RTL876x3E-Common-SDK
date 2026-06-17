/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include "app_main.h"
#include "ble_audio.h"
#include "app_lea_ini_cap.h"
#include "app_lea_ini_bap.h"
#include "app_lea_ini_audio_data.h"
#include "app_lea_ini_mcp.h"
#include "app_lea_ini_ccp.h"
#include "app_flags.h"

void app_lea_profile_init(void)
{
    T_BLE_AUDIO_PARAMS ble_audio_param = {0};
    ble_audio_param.evt_queue_handle = audio_evt_queue_handle;
    ble_audio_param.io_queue_handle = audio_io_queue_handle;

    ble_audio_param.bt_gatt_client_init = 0; // Init gatt client in app_ble_client.c
    ble_audio_param.acl_link_num = MAX_BLE_LINK_NUM;
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
#endif
