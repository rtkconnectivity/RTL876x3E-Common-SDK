/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
#include "bt_gatt_client.h"
#endif
#include "bap.h"
#include "bass_mgr.h"
#include "cap.h"
#include "app_lea_acc_ascs.h"
#include "app_lea_acc_broadcast_audio.h"
#include "app_lea_acc_ccp.h"
#include "app_lea_acc_csis.h"
#include "app_lea_acc_mcp.h"
#include "app_lea_acc_pacs.h"
#include "app_lea_acc_profile.h"
#include "app_lea_acc_mgr.h"
#include "app_lea_acc_unicast_audio.h"
#include "app_lea_acc_vcs.h"
#include "app_lea_acc_vol_policy.h"
#include "app_main.h"

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
#define APP_SNK_ASE_NUM     4
#define APP_SRC_ASE_NUM     2

#define APP_ISOC_CIG_MAX_NUM                      2
#define APP_ISOC_CIS_MAX_NUM                      4
#endif

#if F_APP_TMAS_SUPPORT
static uint16_t app_lea_profile_tmas_role = 0;
#endif

static void app_lea_profile_bap_init(void)
{
    T_BAP_ROLE_INFO role_info = {0};

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    role_info.role_mask |= (BAP_UNICAST_SRV_SRC_ROLE | BAP_UNICAST_SRV_SNK_ROLE);
    role_info.isoc_cig_max_num = APP_ISOC_CIG_MAX_NUM;
    role_info.isoc_cis_max_num = APP_ISOC_CIS_MAX_NUM;
    role_info.snk_ase_num = APP_SNK_ASE_NUM;
    role_info.src_ase_num = APP_SRC_ASE_NUM;
#endif

#if F_APP_TMAP_BMR_SUPPORT
    role_info.role_mask |= (BAP_BROADCAST_SINK_ROLE | BAP_SCAN_DELEGATOR_ROLE);
    role_info.pa_adv_num = 0;
    role_info.isoc_big_broadcaster_num = 0;
    role_info.isoc_bis_broadcaster_num = 0;
    role_info.pa_sync_num = APP_MAX_SYNC_HANDLE_NUM;
    role_info.isoc_big_receiver_num = APP_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM;
    role_info.isoc_bis_receiver_num = APP_SYNC_RECEIVER_MAX_BIS_NUM;
    role_info.brs_num = APP_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM;
#endif
    role_info.init_gap = true;
    bap_role_init(&role_info);
}

static bool app_lea_profile_cap_init(T_CAP_INIT_PARAMS *p_cap_param)
{
    if (p_cap_param)
    {
        p_cap_param->cap_role = CAP_ACCEPTOR_ROLE;
#if F_APP_MCP_SUPPORT
        p_cap_param->mcp_media_control_client = true;
#endif
#if F_APP_CCP_SUPPORT
        p_cap_param->ccp_call_control_client = true;
#endif
        return cap_init(p_cap_param);
    }
    return false;
}

#if F_APP_TMAS_SUPPORT
static void app_lea_profile_tmas_init(void)
{
#if F_APP_TMAP_CT_SUPPORT
    app_lea_profile_tmas_role |= TMAS_CT_ROLE;
#endif
#if F_APP_TMAP_UMR_SUPPORT
    app_lea_profile_tmas_role |= TMAS_UMR_ROLE;
#endif
#if F_APP_TMAP_BMR_SUPPORT
    app_lea_profile_tmas_role |= TMAS_BMR_ROLE;
#endif
    if (app_lea_profile_tmas_role > 0)
    {
        tmas_init(app_lea_profile_tmas_role);
    }
}
#endif

static void app_lea_profile_le_audio_init(void)
{
    T_BLE_AUDIO_PARAMS ble_audio_param;

    ble_audio_param.evt_queue_handle = audio_evt_queue_handle;
    ble_audio_param.io_queue_handle = audio_io_queue_handle;

    ble_audio_param.bt_gatt_client_init = 0; // Init gatt client in app_ble_client.c
    ble_audio_param.acl_link_num = le_get_max_link_num();
    ble_audio_param.io_event_type = IO_MSG_TYPE_LE_AUDIO;

    ble_audio_init(&ble_audio_param);
}

void app_lea_acc_profile_init(void)
{
    T_CAP_INIT_PARAMS cap_init_param = {0};

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    app_lea_ascs_init();
#endif
#if F_APP_MCP_SUPPORT
    app_lea_mcp_init();
#endif
#if F_APP_CCP_SUPPORT
    app_lea_ccp_init();
#endif
    app_lea_pacs_init();
#if F_APP_VCS_SUPPORT
    app_lea_vcs_init();
    app_lea_vol_init();
#endif
    app_lea_profile_bap_init();
#if F_APP_CSIS_SUPPORT
    app_lea_csis_init(&cap_init_param);
#endif
    app_lea_profile_cap_init(&cap_init_param);
#if F_APP_TMAS_SUPPORT
    app_lea_profile_tmas_init();
#endif
    app_lea_profile_le_audio_init();
#if F_APP_TMAP_BMR_SUPPORT
    app_lea_bca_init();
#endif
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    app_lea_uca_init();
#endif
    app_lea_mgr_init();
}

uint16_t app_lea_profile_get_tmas_role(void)
{
#if F_APP_TMAS_SUPPORT
    return app_lea_profile_tmas_role;
#else
    return 0;
#endif
}

#endif
