/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "gap_bond_le.h"
#include "sysm.h"
#include "btm.h"
#include "app_cfg_nv.h"

#include "app_ble_cfg.h"
#include "app_ble_gap.h"
#include "app_bt_policy_api.h"
#include "app_ble_common_adv.h"
#include "app_main.h"
#include "app_charger.h"
#include "ble_adv_data.h"
#include "app_key_process.h"

#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
#include "bt_bond_le.h"
#endif
#if F_APP_IAP_SUPPORT
#include "app_iap.h"
#endif

#if F_APP_IAP_RTK_SUPPORT
#include "app_iap_rtk.h"
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
#include "fmna_connection.h"
#include "app_findmy_ble.h"
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_gfps_cfg.h"
#include "app_gfps_device.h"
#include "app_gfps.h"
#endif

#include "platform_utils.h"
#include "string.h"

void app_ble_device_handle_factory_reset(void)
{
    APP_PRINT_INFO0("app_ble_device_handle_factory_reset");
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
    bt_le_clear_all_keys();
#else
    le_bond_clear_all_keys();
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    fmna_connection_set_is_fmna_paired(false);
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    app_gfps_device_handle_factory_reset();
#endif
}

void app_ble_device_handle_power_on_rtk_adv(void)
{
    app_ble_common_adv_start(app_ble_cfg.timer_ota_adv_timeout * 100);
}

static void app_ble_device_irk_handle_power_on(void)
{
    uint8_t temp_irk[16] = {0};

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        if (memcmp(app_cfg_nv.local_irk, temp_irk, 16) == 0)
        {
            uint8_t local_irk[16] = {0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33};
            uint8_t *p = local_irk;

            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));

            memcpy(app_cfg_nv.local_irk, local_irk, 16);
            app_cfg_store(app_cfg_nv.local_irk, 16);
        }

        bt_le_set_local_irk(GAP_IDENT_ADDR_PUBLIC, app_cfg_nv.bud_local_addr, app_cfg_nv.local_irk);
        /*patch need modify le_bond_set_param and call sm_setup_local_irk when set GAP_PARAM_BOND_SET_LOCAL_IRK so it can work after stack ready*/
        le_bond_set_param(GAP_PARAM_BOND_SET_LOCAL_IRK, GAP_KEY_LEN, app_cfg_nv.local_irk);

        ble_ext_adv_update_rpa_for_irk_changed();
    }

    APP_PRINT_INFO3("app_ble_device_irk_handle_power_on: bud_role %d, local_addr %b, local_irk %b",
                    app_cfg_nv.bud_role, TRACE_BDADDR(app_cfg_nv.bud_local_addr),
                    TRACE_BINARY(16, app_cfg_nv.local_irk));
}


void app_ble_device_handle_power_on(void)
{
    APP_PRINT_INFO0("app_ble_device_handle_power_on");
    bool is_pairing = app_key_is_enter_pairing();
    ble_ext_adv_print_info();
    ble_adv_data_enable();

    app_ble_device_irk_handle_power_on();

    if (app_ble_cfg.rtk_app_adv_support && (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        if (app_cfg_const.enable_power_on_adv_with_timeout)
        {
            app_ble_device_handle_power_on_rtk_adv();
        }
    }

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    app_gfps_device_handle_power_on(is_pairing);
#endif
}

void app_ble_device_handle_power_off(void)
{
    APP_PRINT_INFO0("app_ble_device_handle_power_off");

    ble_adv_data_disable();

    if (app_ble_cfg.rtk_app_adv_support)
    {
        if (app_ble_common_adv_get_state() != BLE_EXT_ADV_MGR_ADV_DISABLED)
        {
            app_ble_common_adv_stop(APP_STOP_ADV_CAUSE_POWER_OFF);
        }
    }

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    app_gfps_device_handle_power_off();
#endif

    app_ble_gap_disconnect_all(LE_LOCAL_DISC_CAUSE_POWER_OFF);
}


void app_ble_device_handle_enter_pair_mode(void)
{
    APP_PRINT_INFO0("app_ble_device_handle_enter_pair_mode");

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
    app_gfps_le_device_adv_start(REMOTE_SESSION_ROLE_SINGLE);
#endif
}


void app_ble_device_init(void)
{

}

