/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "stdlib.h"
#include "trace.h"
#include "sysm.h"
#include "ringtone.h"
#include "gap_conn_le.h"
#include "gap_bond_le.h"
#include "gap_ext_scan.h"
#include "dfu_api.h"
#include "app_ota.h"
#include "app_ble_cfg.h"
#include "app_ble_gap.h"
#include "app_ble_service.h"
#include "app_main.h"
#include "app_transfer.h"
#include "app_report.h"

#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "remote.h"
#include "app_cmd.h"
#include "app_audio_policy.h"

#include "app_auto_power_off.h"
#include "gap_privacy.h"

#if F_APP_GATT_OVER_BREDR_SUPPORT
#include "app_att.h"
#endif

#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
#include "gap_bond_manager.h"
#include "bt_bond_api.h"
#include "bt_bond_le.h"
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_gfps_cfg.h"
#include "app_gfps_device.h"
#include "app_gfps.h"
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
#include "app_gfps_finder.h"
#endif
#endif

#include "ble_scan.h"
#include "ble_mgr.h"
#include "app_ble_rand_addr_mgr.h"
#include "gap_vendor.h"
#if F_APP_SC_KEY_DERIVE_SUPPORT
#include "app_ble_sc_key_derive.h"
#endif

#include "app_rpt_ble.h"

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include "ble_audio.h"
#include "app_lea_ini_conn_mgr.h"
#endif

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
#include "bt_gatt_client.h"
#endif

#include "app_ble_common_adv.h"

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
#include "ble_audio.h"
#include "app_lea_acc_pacs.h"
#endif

#if GATTC_TBL_STORAGE_SUPPORT
#include "gattc_tbl_storage.h"
#endif

#include "app_timer.h"

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_bond_manager.h"
#include "app_tri_dongle_usb_hid_ctrl.h"
#include "app_tri_dongle_cmd.h"
#endif

#if F_APP_GUI_SUPPORT
#include "app_panel_le_db.h"
#endif

#if F_APP_CHARGE_CASE_DEMO_SUPPORT
#include "app_charging_case_cmd.h"
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
#include "fmna_gatt_platform.h"
#include "gap_adv.h"
#include "tps.h"
#endif

#if CONFIG_REALTEK_FINDMY_SUPPORT_CUSTOMIZED_APP
#include "custom_app.h"
#endif

static uint8_t timer_idx_gap_delay_disconn = 0;
static uint8_t timer_idx_gap_wait_for_authen = 0;
static uint8_t app_ble_gap_timer_id = 0;
static uint8_t authen_fail_conn_id = 0xFF;
static uint8_t wait_for_authen_conn_id = 0xFF;

uint8_t scan_rsp_data_len;
uint8_t scan_rsp_data[GAP_MAX_LEGACY_ADV_LEN];

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_mgr.h"
#endif

typedef struct t_ble_whitelist_op_elem T_BLE_WHITELIST_OP_ELEM;

typedef struct t_ble_whitelist_op_elem
{
    T_BLE_WHITELIST_OP_ELEM *p_next;
    T_GAP_WHITE_LIST_OP operation;
    T_GAP_REMOTE_ADDR_TYPE bd_type;
    uint8_t bd_addr[6];
} T_BLE_WHITELIST_OP_ELEM;

typedef struct t_ble_resolvelist_op_elem T_BLE_RESOLVELIST_OP_ELEM;

typedef struct t_ble_resolvelist_op_elem
{
    T_BLE_RESOLVELIST_OP_ELEM *p_next;
    T_GAP_RESOLV_LIST_OP operation;
    T_GAP_IDENT_ADDR_TYPE bd_type;
    uint8_t bd_addr[6];
    uint8_t local_irk[16];
    uint8_t peer_irk[16];
} T_BLE_RESOLVELIST_OP_ELEM;

static struct
{
    struct
    {
        T_OS_QUEUE op_q;
        bool       op_busy;
    } white_list;

    struct
    {
        T_OS_QUEUE op_q;
        bool       op_busy;
    } resolve_list;

    T_GAP_DEV_STATE state;

    struct
    {
        uint8_t len;
        uint8_t data[GAP_MAX_LEGACY_ADV_LEN];
    } scan_rsp;

} mgr =
{
    .white_list =
    {
        .op_q = {},
        .op_busy = false,
    },

    .resolve_list =
    {
        .op_q = {},
        .op_busy = false,
    },

    .state = 0,

    .scan_rsp =
    {
        .len = 0,
        .data = {},
    }
};

typedef enum
{
    APP_TIMER_GAP_DISCONNECT_DELAY,
    APP_TIMER_GAP_WAIT_FOR_AUTHEN,
} T_APP_GAP_TIMER;

bool app_ble_gap_gen_scan_rsp_data(uint8_t *p_scan_len, uint8_t *p_scan_data)
{
    uint8_t device_name_len;

    if (p_scan_len == NULL || p_scan_data == NULL)
    {
        return false;
    }

    device_name_len = strlen((const char *)app_cfg_nv.device_name_le);

    if (device_name_len > GAP_MAX_LEGACY_ADV_LEN - 2)
    {
        device_name_len = GAP_MAX_LEGACY_ADV_LEN - 2;
    }

    p_scan_data[0] = device_name_len + 1;
    p_scan_data[1] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&p_scan_data[2], app_cfg_nv.device_name_le, device_name_len);
    *p_scan_len = device_name_len + 2;
    return true;
}

LE_ENCRYPT_STATE app_ble_gap_get_link_encryption_status(T_APP_LE_LINK *p_link)
{
    if (p_link)
    {
        return p_link->encryption_status;
    }
    else
    {
        APP_PRINT_ERROR0("app_ble_gap_get_link_encryption_status: link not exist");
        return LE_LINK_ERROR;
    }
}

static void app_ble_gap_handle_dev_state_change_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_TRACE5("app_ble_gap_handle_dev_state_change_evt: le_state.gap_adv_state %d, new_state.gap_adv_state %d, "
                     "le_state.gap_scan_state %d, new_state.gap_scan_state %d, cause 0x%04x",
                     mgr.state.gap_adv_state,
                     new_state.gap_adv_state,
                     mgr.state.gap_scan_state,
                     new_state.gap_scan_state,
                     cause);

    if (mgr.state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {

#if F_APP_GATT_OVER_BREDR_SUPPORT
            app_att_init();
#endif
        }
    }
#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
    if (mgr.state.gap_conn_state != new_state.gap_conn_state)
    {
        if (new_state.gap_conn_state == GAP_CONN_DEV_STATE_IDLE)
        {
            if (app_db.conn_dev_queue.count)
            {
                T_CONN_DEV_INFO *p_conn_dev;
                p_conn_dev = os_queue_peek(&app_db.conn_dev_queue, 0);
                if (p_conn_dev)
                {
                    if (app_lea_ini_create_conn(p_conn_dev->bd_addr, p_conn_dev->addr_type) == GAP_CAUSE_SUCCESS)
                    {
                        os_queue_delete(&app_db.conn_dev_queue, p_conn_dev);
                        free(p_conn_dev);
                    }
                }
            }
        }
    }
#endif
    mgr.state = new_state;
}


static void rpt_disc_evt(uint8_t app_link_id, T_LE_LOCAL_DISC_CAUSE disc_cause)
{
    struct
    {
        uint8_t app_link_id;
        uint16_t disc_cause;
    } __attribute__((packed)) rpt = {};

    if (app_cfg_const.enable_data_uart)
    {
        rpt.app_link_id = app_link_id;
        rpt.disc_cause = disc_cause;
        app_report_event(CMD_PATH_UART, EVENT_LE_DISCONNECTED, 0, (uint8_t *)&rpt, sizeof(rpt));
    }

}

static bool app_ble_is_static_rand_addr(uint8_t *p_addr)
{
    return (p_addr[5] & 0xC0) == 0xC0;
}

static bool app_ble_is_rpa_addr(uint8_t *p_addr)
{
    return (p_addr[5] & 0xC0) == 0x40;
}

/**
 * @brief get remote bd address and address type and store them into p_link.
 *
 * @param p_link  @ref T_APP_LE_LINK
 *
 * remote use RPA(resolvable private address) and resolved address type is public address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_PUBLIC;
 * p_link->bd_addr is resloved public address;
 *
 * remote use RPA(resolvable private address) and resolved address type is static random address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_STATIC;
 * p_link->bd_addr is resloved static random address;
 *
 * remote use RPA(resolvable private address) and RPA can not be success resolved:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_UNRESOLVED;
 * p_link->bd_addr is RPA;
 * Reasons why RPA cannot be successfully resolved may be:
 * 1.Pairing failed or pairing was not performed, resulting in stack unable to obtain the remote identity address.
 * 2.Pairing is successful and stack can obtain the remote identity address, but RPA resolution fails.
 *
 * remote use static random address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_STATIC_RANDOM;
 * p_link->bd_addr is static random address;
 *
 * remote use public address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_PUBLIC;
 * p_link->bd_addr is public address;
 */
void app_ble_gap_get_remote_bd_addr_and_type(T_APP_LE_LINK *p_link)
{
    uint8_t remote_bd_addr[6] = {0};
    uint8_t resolved_addr[6] = {0};
    T_GAP_IDENT_ADDR_TYPE resolved_bd_type = (T_GAP_IDENT_ADDR_TYPE)0xFF;
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = (T_GAP_REMOTE_ADDR_TYPE)0xFF;
    if (p_link == NULL)
    {
        return;
    }

    le_get_conn_addr(p_link->conn_id, remote_bd_addr, &remote_bd_type);

    if (remote_bd_type == GAP_REMOTE_ADDR_LE_RANDOM)
    {
        if (app_ble_is_rpa_addr(remote_bd_addr))
        {
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
            if (bt_le_resolve_random_address(remote_bd_addr, resolved_addr, &resolved_bd_type) == true)
#else
            if (le_resolve_random_address(remote_bd_addr, resolved_addr, &resolved_bd_type) == true)
#endif
            {
                if (resolved_bd_type == GAP_IDENT_ADDR_PUBLIC)
                {
                    memcpy(p_link->bd_addr, resolved_addr, 6);
                    p_link->bd_type = APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_PUBLIC;
                }
                else if (resolved_bd_type == GAP_IDENT_ADDR_RAND)
                {
                    memcpy(p_link->bd_addr, resolved_addr, 6);
                    p_link->bd_type = APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_STATIC;
                }
            }
            else
            {
                memcpy(p_link->bd_addr, remote_bd_addr, 6);
                p_link->bd_type = APP_LE_REMOTE_BD_TYPE_RPA_AND_UNRESOLVED;
            }
        }
        else if (app_ble_is_static_rand_addr(remote_bd_addr))
        {
            memcpy(p_link->bd_addr, remote_bd_addr, 6);
            p_link->bd_type = APP_LE_REMOTE_BD_TYPE_STATIC_RANDOM;
        }
    }
    else if (remote_bd_type == GAP_REMOTE_ADDR_LE_PUBLIC)
    {
        memcpy(p_link->bd_addr, remote_bd_addr, 6);
        p_link->bd_type = APP_LE_REMOTE_BD_TYPE_PUBLIC;
    }
}

static void app_ble_gap_handle_new_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                                  uint16_t disc_cause)
{
    T_APP_LE_LINK *p_link;

    APP_PRINT_TRACE3("app_ble_gap_handle_new_conn_state_evt: conn_id %d, new_state %d, cause 0x%04x",
                     conn_id, new_state, disc_cause);
    p_link = app_link_find_le_link_by_conn_id(conn_id);

    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTING:
        if (p_link != NULL)
        {
            p_link->state = LE_LINK_STATE_DISCONNECTING;
        }
        break;

    case GAP_CONN_STATE_DISCONNECTED:
        if (p_link != NULL)
        {
            uint8_t local_disc_cause = p_link->local_disc_cause;
            rpt_disc_evt(p_link->id, p_link->local_disc_cause);

            for (uint8_t i = 0; i < p_link->disc_cb_list.count; i++)
            {
                T_LE_DISC_CB_ENTRY *p_entry;
                p_entry = os_queue_peek(&p_link->disc_cb_list, i);
                if (p_entry != NULL && p_entry->disc_callback != NULL)
                {
                    p_entry->disc_callback(conn_id, p_link->local_disc_cause, disc_cause);
                }
            }
#if F_APP_DASHBOARD_DEMO_SUPPORT
            app_ble_common_adv_start(0);
#endif

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
            app_lea_ini_handle_disconnected(conn_id);
#endif

#if F_APP_GUI_SUPPORT
            app_panel_le_free_link(p_link->bd_addr);
#endif

#if F_APP_CHARGE_CASE_DEMO_SUPPORT
            app_charging_case_handle_le_disconn(p_link->bd_addr, local_disc_cause);
#endif

            app_transfer_queue_reset(CMD_PATH_LE);
            if (authen_fail_conn_id == conn_id)
            {
                app_stop_timer(&timer_idx_gap_delay_disconn);
                authen_fail_conn_id = 0xFF;
            }

            if (wait_for_authen_conn_id == conn_id)
            {
                app_stop_timer(&timer_idx_gap_wait_for_authen);
                wait_for_authen_conn_id = 0xFF;
            }

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            struct
            {
                uint8_t     bd_addr[6];
            } tri_dongle_ble_disc = {0};
            memcpy(tri_dongle_ble_disc.bd_addr, p_link->bd_addr, 6);
#if F_APP_1_EP_HID_MULTI_DEV_CDC_SUPPORT
            uint16_t record_conn_handle = p_link->conn_handle;
#endif
#endif
            app_link_free_le_link(p_link);

            if (app_link_get_le_link_num() == 0)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BLE_LINK_EXIST, app_cfg_const.timer_auto_power_off);

                //app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_BLE_DISC);
            }

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            app_tri_dongle_mgr_ble_disc_report(tri_dongle_ble_disc.bd_addr);
#if F_APP_1_EP_HID_MULTI_DEV_CDC_SUPPORT
            app_tri_dongle_offset_ci_update(APP_TRI_DONGLE_UPDATE_OFFSET_CLEAN,
                                            record_conn_handle,
                                            NULL);
#endif
#endif
        }
        break;

    case GAP_CONN_STATE_CONNECTING:
        if (p_link == NULL)
        {
            p_link = app_link_alloc_le_link_by_conn_id(conn_id);
            if (p_link != NULL)
            {
                p_link->state = LE_LINK_STATE_CONNECTING;
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        if (p_link != NULL)
        {
            app_ble_gap_get_remote_bd_addr_and_type(p_link);

            p_link->conn_handle = le_get_conn_handle(conn_id);

            if (p_link->state == LE_LINK_STATE_CONNECTING)
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_LINK_EXIST);

                p_link->state = LE_LINK_STATE_CONNECTED;

                le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &p_link->mtu_size, conn_id);
                if (app_cfg_const.enable_data_uart)
                {
                    app_report_event(CMD_PATH_UART, EVENT_LE_CONNECTED, 0, &p_link->id, 1);

                    app_rpt_ble_conn_cmpl(p_link);
#if F_APP_BT_AUDIO_TRI_DONGLE
                    bool res = false;
                    for (uint8_t i = 0; i < p_link->bas_client_instance_num; i ++)
                    {
                        res = bas_client_read_battery_level(p_link->conn_handle, i);
                    }
                    if (res == false)
                    {
                        APP_PRINT_ERROR0("F_APP_BT_AUDIO_TRI_DONGLE BAS not support!!!");
                    }
#endif
                }

#if TRANSMIT_CLIENT_SUPPORT
                p_link->tx_event_seqn = 0;
#endif
            }

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
            app_lea_ini_handle_connected(conn_id);
#endif

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
            app_lea_pacs_init_available_context(p_link->conn_handle);
#endif
            app_ota_send_srv_change_indication_info(conn_id);
        }
        break;

    default:
        break;
    }
}


#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
T_APP_RESULT app_ble_gap_client_discov_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                                          void *p_data)
{
    if (type == GATT_CLIENT_EVENT_DIS_ALL_STATE)
    {
        T_GATT_CLIENT_DIS_ALL_DONE *p_disc = (T_GATT_CLIENT_DIS_ALL_DONE *)p_data;
        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
        APP_PRINT_INFO3("app_ble_gap_client_discov_cb:is_success %d, load_from_ftl %d, conn_handle %d",
                        p_disc->state, p_disc->load_from_ftl, conn_handle);
#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
        if (app_cfg_nv.triple_feature.audio_device_lea_supported)
        {
            app_ini_disable_conn_update(p_link->bd_addr);
        }
#else
        if (p_disc->state == GATT_CLIENT_STATE_DISCOVERY)
        {
            app_lea_ini_handle_dev_state_change_event(p_link->bd_addr, CP_EVENT_GATT_DISCOVERY);
            app_ini_disable_conn_update(p_link->bd_addr);
        }
        else if (p_disc->state == GATT_CLIENT_STATE_DONE &&
                 !p_disc->load_from_ftl)
        {
            app_lea_ini_handle_dev_state_change_event(p_link->bd_addr, CP_EVENT_GATT_DONE);
            app_ini_enable_conn_update(p_link->bd_addr);
        }
#endif
#endif
    }
    else if (type == GATT_CLIENT_EVENT_SERVICE_CHANGED_INDICATION)
    {
        //delete old service table in ftl and restart service discovery
        APP_PRINT_INFO0("app_ble_gap_client_discov_cb: receive service change indication");
#if GATTC_TBL_STORAGE_SUPPORT
        gattc_tbl_storage_del(conn_handle);
#endif
        gatt_client_start_discovery_all(conn_handle, app_ble_gap_client_discov_cb);
    }
    return APP_RESULT_SUCCESS;
}
#endif

void app_ble_gap_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    T_GAP_CONN_INFO conn_info;

    if (le_get_conn_info(conn_id, &conn_info) == false)
    {
        return;
    }

    APP_PRINT_INFO4("app_ble_gap_handle_authen_state_evt:conn_id %d, state %d, cause 0x%x, role %d",
                    conn_id, new_state, cause, conn_info.role);

    if (new_state == GAP_AUTHEN_STATE_COMPLETE)
    {
        T_APP_LE_LINK *p_link;
        p_link = app_link_find_le_link_by_conn_id(conn_id);

        if (p_link != NULL)
        {
            if (cause == GAP_SUCCESS)
            {
                p_link->cmd.tx_mask |= TX_ENABLE_AUTHEN_BIT;
                p_link->encryption_status = LE_LINK_ENCRYPTIONED;
                app_audio_tone_type_play(TONE_LE_PAIR_COMPLETE, false, false);

                app_ble_gap_get_remote_bd_addr_and_type(p_link);

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
//                app_gfps_handle_b2s_ble_bonded(conn_id, p_link->bd_addr);
#endif

                if (wait_for_authen_conn_id == conn_id)
                {
                    app_stop_timer(&timer_idx_gap_wait_for_authen);
                    wait_for_authen_conn_id = 0xFF;
                }
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
                app_tri_dongle_mgr_ble_authen_state_evt(conn_id);
#endif

#if F_APP_GUI_SUPPORT
                app_panel_le_alloc_link(p_link->bd_addr);
#endif
            }
            else
            {
                if (conn_info.role == GAP_LINK_ROLE_MASTER)
                {
                    //master authen fail, directly disconnect this ble link
                    authen_fail_conn_id = conn_id;

                    if (cause != (SM_ERR | SMP_ERR_PAIRING_NOT_SUPPORTED))
                    {
                        app_start_timer(&timer_idx_gap_delay_disconn, "gap_disconnect_delay",
                                        app_ble_gap_timer_id, APP_TIMER_GAP_DISCONNECT_DELAY, 0, false,
                                        100);
                    }

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
                    if (cause == (HCI_ERR | HCI_ERR_KEY_MISSING))
                    {
                        APP_PRINT_INFO1("key missing 1 %s", TRACE_BDADDR(p_link->bd_addr));
                        app_tri_dongle_usb_hid_delete_one_desc(p_link->bd_addr);
                    }
#endif
                }
                else if (conn_info.role == GAP_LINK_ROLE_SLAVE)
                {
                    if (cause == (SM_ERR | SM_ERR_LINK_KEY_MISSING))
                    {
                        //slave keep ble link to wait master do SMP again, or let master disconnect link
                        wait_for_authen_conn_id = conn_id;
                        app_start_timer(&timer_idx_gap_wait_for_authen, "gap_wait_for_authen",
                                        app_ble_gap_timer_id, APP_TIMER_GAP_WAIT_FOR_AUTHEN, 0, false,
                                        10000);
                    }
                    else
                    {
                        //authen fail, directly disconnect this ble link
                        authen_fail_conn_id = conn_id;
                        app_start_timer(&timer_idx_gap_delay_disconn, "gap_disconnect_delay",
                                        app_ble_gap_timer_id, APP_TIMER_GAP_DISCONNECT_DELAY, 0, false,
                                        100);
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
                        if (cause == (HCI_ERR | HCI_ERR_KEY_MISSING))
                        {
                            APP_PRINT_INFO1("key missing 2 %s", TRACE_BDADDR(p_link->bd_addr));
                            app_tri_dongle_usb_hid_delete_one_desc(p_link->bd_addr);
                        }
#endif
                    }
                }
            }

#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 0)
            app_rpt_ble_pair_status(p_link->id, cause, p_link->bd_addr);
#endif

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
            if (cause == GAP_SUCCESS)
            {
                p_link->auth_cmpl = true;

                if (p_link->mtu_received)
                {
#if F_APP_RESET_DISCOVERY_BY_BLE_CONN
#if GATTC_TBL_STORAGE_SUPPORT
                    gattc_tbl_storage_del(p_link->conn_handle);
#endif
#endif
                    if (gatt_client_start_discovery_all(p_link->conn_handle,
                                                        app_ble_gap_client_discov_cb) == GAP_CAUSE_SUCCESS)
                    {
                        p_link->start_discover = true;
                    }
                }
            }
            else
            {
                p_link->auth_cmpl = false;
            }
#endif
        }
    }
    else if (new_state == GAP_AUTHEN_STATE_STARTED)
    {
        if (wait_for_authen_conn_id == conn_id)
        {
            app_stop_timer(&timer_idx_gap_wait_for_authen);
            wait_for_authen_conn_id = 0xFF;
        }
    }
}

T_GAP_CAUSE app_ble_gap_modify_white_list(T_GAP_WHITE_LIST_OP  operation, uint8_t *bd_addr,
                                          T_GAP_REMOTE_ADDR_TYPE bd_type)
{
    APP_PRINT_INFO3("app_ble_gap_modify_white_list: opration %d, addr_type %d, addr %s",
                    operation, bd_type, TRACE_BDADDR(bd_addr));
    T_GAP_CAUSE ret;
    if (mgr.white_list.op_busy == false)
    {
        mgr.white_list.op_busy = true;
        ble_scan_pause();
        ret = le_modify_white_list(operation, bd_addr, bd_type);
        ble_scan_resume();
    }
    else
    {
        T_BLE_WHITELIST_OP_ELEM *op = malloc(sizeof(T_BLE_WHITELIST_OP_ELEM));
        if (!op)
        {
            ret = GAP_CAUSE_NO_RESOURCE;
            return ret;
        }
        op->operation = operation;
        op->bd_type = bd_type;
        memcpy(op->bd_addr, bd_addr, 6);
        os_queue_in(&mgr.white_list.op_q, op);
        ret = GAP_CAUSE_SUCCESS;
    }
    return ret;
}

T_GAP_CAUSE app_ble_gap_modify_resolve_list(T_GAP_RESOLV_LIST_OP  operation, uint8_t *bd_addr,
                                            T_GAP_IDENT_ADDR_TYPE bd_type,
                                            uint8_t *p_peer_irk, uint8_t *p_local_irk)
{
    APP_PRINT_INFO3("app_ble_gap_modify_resolve_list: opration %d, bd_type %d, addr %s",
                    operation, bd_type, TRACE_BDADDR(bd_addr));
    T_GAP_CAUSE ret;
    if (mgr.resolve_list.op_busy == false)
    {
        mgr.resolve_list.op_busy = true;
        ble_scan_pause();
        if (operation == GAP_RESOLV_LIST_OP_ADD)
        {
            ret = le_privacy_add_resolv_list(bd_type, bd_addr, p_peer_irk, true);
        }
        else
        {
            ret = le_privacy_modify_resolv_list(operation, bd_type, bd_addr);
        }

        ble_scan_resume();
    }
    else
    {
        T_BLE_RESOLVELIST_OP_ELEM *op = malloc(sizeof(T_BLE_RESOLVELIST_OP_ELEM));
        if (!op)
        {
            ret = GAP_CAUSE_NO_RESOURCE;
            return ret;
        }
        op->operation = operation;
        op->bd_type = bd_type;
        memcpy(op->bd_addr, bd_addr, 6);
        memcpy(op->local_irk, p_local_irk, 16);
        memcpy(op->peer_irk, p_peer_irk, 16);
        os_queue_in(&mgr.resolve_list.op_q, op);
        ret = GAP_CAUSE_SUCCESS;
    }
    APP_PRINT_INFO1("app_ble_gap_modify_resolve_list: ret %d", ret);
    return ret;
}

void app_ble_gap_handle_gap_msg(T_IO_MSG *p_io_msg)
{
    APP_PRINT_TRACE1("app_ble_gap_handle_gap_msg: subtype %d", p_io_msg->subtype);
    T_LE_GAP_MSG stack_msg;
    T_APP_LE_LINK *p_link;

    memcpy(&stack_msg, &p_io_msg->u.param, sizeof(p_io_msg->u.param));

    ble_mgr_handle_gap_msg(p_io_msg->subtype, &stack_msg);

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_tri_dongle_mgr_handle_gap_msg(p_io_msg->subtype, stack_msg);
#endif

    switch (p_io_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            app_ble_gap_handle_dev_state_change_evt(stack_msg.msg_data.gap_dev_state_change.new_state,
                                                    stack_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_ble_gap_handle_new_conn_state_evt(stack_msg.msg_data.gap_conn_state_change.conn_id,
                                                  (T_GAP_CONN_STATE)stack_msg.msg_data.gap_conn_state_change.new_state,
                                                  stack_msg.msg_data.gap_conn_state_change.disc_cause);
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
            app_gfps_link_handle_new_conn_state_evt(stack_msg.msg_data.gap_conn_state_change.conn_id,
                                                    (T_GAP_CONN_STATE)stack_msg.msg_data.gap_conn_state_change.new_state,
                                                    stack_msg.msg_data.gap_conn_state_change.disc_cause);
#endif
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            uint8_t conn_id = stack_msg.msg_data.gap_conn_mtu_info.conn_id;
            p_link = app_link_find_le_link_by_conn_id(conn_id);

            if (p_link != NULL)
            {
#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
                p_link->mtu_received = true;

                if ((p_link->start_discover == false)
#if (F_APP_ADD_NON_YYLX_BLE_SCENARIO == 0)
                    && p_link->auth_cmpl
#endif
                   )
                {
#if F_APP_RESET_DISCOVERY_BY_BLE_CONN
#if GATTC_TBL_STORAGE_SUPPORT
                    gattc_tbl_storage_del(le_get_conn_handle(conn_id));
#endif
#endif
                    if (gatt_client_start_discovery_all(le_get_conn_handle(conn_id),
                                                        app_ble_gap_client_discov_cb) == GAP_CAUSE_SUCCESS)
                    {
                        p_link->start_discover = true;
                    }
                }
#endif
                p_link->mtu_size = stack_msg.msg_data.gap_conn_mtu_info.mtu_size;

                app_rpt_ble_mtu(p_link->id);

                if (p_link->encryption_status == LE_LINK_UNENCRYPTIONED)
                {
                    T_GAP_CONN_INFO conn_info;
                    bool ret = le_get_conn_info(conn_id, &conn_info);

                    /*slave using le_bond_pair() will affect findmy and gfps function*/
                    if ((ret == true) && (conn_info.role == GAP_LINK_ROLE_MASTER))
                    {
#if F_APP_ADD_NON_YYLX_BLE_SCENARIO
                        if (app_tri_dongle_is_yylx_hid_device(p_link->bd_addr))
#endif
                        {
                            le_bond_pair(p_link->conn_id);
                        }
                    }
                }
            }
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            app_ble_gap_handle_authen_state_evt(stack_msg.msg_data.gap_authen_state.conn_id,
                                                stack_msg.msg_data.gap_authen_state.new_state,
                                                stack_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            uint8_t conn_id = stack_msg.msg_data.gap_bond_just_work_conf.conn_id;
            le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("app_ble_gap_handle_gap_msg: GAP_MSG_LE_BOND_JUST_WORK");
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:
        {
            uint32_t display_value = 0;
            uint8_t conn_id = stack_msg.msg_data.gap_bond_passkey_display.conn_id;
            le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO2("app_ble_gap_handle_gap_msg: GAP_MSG_LE_BOND_PASSKEY_DISPLAY conn_id %d, passkey %d",
                            conn_id, display_value);
            le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {
            uint8_t conn_id = stack_msg.msg_data.gap_bond_user_conf.conn_id;
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(conn_id);

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
            if (!app_gfps_handle_ble_user_confirm(conn_id))
            {
                if (p_link)
                {
                    app_rpt_ble_bond_cfm(p_link->id);
                }
            }
#else
            if (p_link)
            {
                app_rpt_ble_bond_cfm(p_link->id);
            }
#endif
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
            app_lea_ini_handle_conn_update_status(stack_msg.msg_data.gap_conn_param_update);
#endif
#if CONFIG_DONGLE_RTL8763ESH
            app_ble_only_handle_conn_update_status(stack_msg.msg_data.gap_conn_param_update);
#endif
            p_link = app_link_find_le_link_by_conn_id(stack_msg.msg_data.gap_conn_param_update.conn_id);
            if (p_link && p_link->used && (stack_msg.msg_data.gap_conn_param_update.status == 0))
            {
                app_rpt_ble_conn_update(p_link, stack_msg.msg_data.gap_conn_param_update.status);
            }
        }
        break;

    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE:
        {

        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
        {
            uint8_t conn_id = stack_msg.msg_data.gap_bond_passkey_input.conn_id;
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(conn_id);
            app_report_event(CMD_PATH_UART, EVENT_LE_USER_PASSKEY_INPUT_REQ, 0, &p_link->id,
                             sizeof(p_link->id));
            // le_bond_passkey_input_confirm(conn_id, passkey, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    default:
        break;
    }
#if (F_APP_LE_AUDIO_INITIATOR_SUPPORT || F_APP_LE_AUDIO_ACCEPTOR_SUPPORT)
    ble_audio_handle_gap_msg(p_io_msg->subtype, stack_msg);
#endif
}

static T_APP_RESULT app_ble_gap_cb(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA cb_data;

    memcpy(&cb_data, p_cb_data, sizeof(T_LE_CB_DATA));

#if (F_APP_LE_AUDIO_INITIATOR_SUPPORT || F_APP_LE_AUDIO_ACCEPTOR_SUPPORT)
    //ble audio lib handle gap msg
    ble_audio_handle_gap_cb(cb_type, p_cb_data);
#endif

    ble_mgr_handle_gap_cb(cb_type, p_cb_data);

    switch (cb_type)
    {
    case GAP_MSG_LE_REMOTE_FEATS_INFO:
        {
        }
        break;

    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        {
            app_rpt_ble_set_data_len(cb_data.p_le_data_len_change_info);
        }
        break;

    case GAP_MSG_LE_DISABLE_SLAVE_LATENCY:
        {
            app_rpt_ble_ignore_slave_latency(cb_data.p_le_disable_slave_latency_rsp);
        }
        break;

    case GAP_MSG_LE_PHY_UPDATE_INFO:
        {
            app_rpt_ble_phy_upd(cb_data.p_le_phy_update_info);
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            T_LE_PHY_UPDATE_INFO *p_info = cb_data.p_le_phy_update_info;
            app_tri_dongle_vendor_update_procedure(APP_TRI_DONGLE_VENDOR_UPDATE_PHY_CMPL, p_info->conn_id);
#endif
        }
        break;
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
    case GAP_MSG_APP_BOND_MANAGER_INFO:
        {
            result = bt_bond_mgr_handle_gap_msg(cb_data.p_le_cb_data);
        }
        break;
#endif
    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE_INFO:
        {

        }
        break;
    // case GAP_MSG_LE_EXT_ADV_REPORT_INFO:
    //     APP_PRINT_INFO6("GAP_MSG_LE_EXT_ADV_REPORT_INFO:event_type 0x%x, bd_addr %s, addr_type %d, rssi %d, data_len %d,data %b",
    //                     cb_data.p_le_ext_adv_report_info->event_type,
    //                     TRACE_BDADDR(cb_data.p_le_ext_adv_report_info->bd_addr),
    //                     cb_data.p_le_ext_adv_report_info->addr_type,
    //                     cb_data.p_le_ext_adv_report_info->rssi,
    //                     cb_data.p_le_ext_adv_report_info->data_len,
    //                     TRACE_BINARY(cb_data.p_le_ext_adv_report_info->data_len, cb_data.p_le_ext_adv_report_info->p_data));


    //     break;

    case GAP_MSG_LE_CONN_UPDATE_IND:
#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
        result = app_lea_ini_handle_conn_update_ind(cb_data.p_le_conn_update_ind);
#else
#if CONFIG_DONGLE_RTL8763ESH
        result = app_ble_only_handle_conn_update_ind(cb_data.p_le_conn_update_ind);
#else
        result = APP_RESULT_ACCEPT;
#endif
#endif
        break;

    case GAP_MSG_LE_EXT_ADV_START_SETTING:
        break;

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        {
            APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation = 0x%x cause=0x%x",
                            cb_data.p_le_modify_white_list_rsp->operation,
                            cb_data.p_le_modify_white_list_rsp->cause);

            mgr.white_list.op_busy = false;
            if (mgr.white_list.op_q.count != 0)
            {
                T_BLE_WHITELIST_OP_ELEM *op = os_queue_out(&mgr.white_list.op_q);
                if (op != NULL)
                {
                    app_ble_gap_modify_white_list(op->operation, op->bd_addr, op->bd_type);
                    free(op);
                    op = NULL;
                }
            }
        }
        break;

#if !CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
    case GAP_MSG_LE_BOND_MODIFY_INFO:
        {
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_MODIFY_INFO: type 0x%x",
                            cb_data.p_le_bond_modify_info->type);
#if GATTC_TBL_STORAGE_SUPPORT
            gattc_tbl_storage_handle_bond_modify(p_data->p_le_bond_modify_info);
#endif
        }
        break;
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    case GAP_MSG_LE_ADV_READ_TX_POWER:
        APP_PRINT_INFO2("GAP_MSG_LE_ADV_READ_TX_POWER: cause 0x%x, tx_power_level 0x%x",
                        cb_data.p_le_adv_read_tx_power_rsp->cause,
                        cb_data.p_le_adv_read_tx_power_rsp->tx_power_level);
        tps_set_parameter(TPS_PARAM_TX_POWER, 1, &(cb_data.p_le_adv_read_tx_power_rsp->tx_power_level));
        break;
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    case GAP_MSG_LE_READ_RSSI:
        {
            if (cb_data.p_le_read_rssi_rsp->cause == 0)
            {
                app_tri_dongle_notify_ble_rssi_info(cb_data.p_le_read_rssi_rsp->conn_id,
                                                    cb_data.p_le_read_rssi_rsp->rssi);
            }
        }
        break;
#endif

    default:
        break;
    }

    return result;
}


static T_APP_RESULT app_ble_privacy_cb(uint8_t msg_type, T_LE_PRIVACY_CB_DATA msg_data)
{
    switch (msg_type)
    {
    case GAP_MSG_LE_PRIVACY_MODIFY_RESOLV_LIST:
        {
            APP_PRINT_INFO2("GAP_MSG_LE_PRIVACY_MODIFY_RESOLV_LIST: operation = 0x%x cause=0x%x",
                            msg_data.p_le_privacy_modify_resolv_list_rsp->operation,
                            msg_data.p_le_privacy_modify_resolv_list_rsp->cause);
            mgr.resolve_list.op_busy = false;
            if (mgr.resolve_list.op_q.count != 0)
            {
                T_BLE_RESOLVELIST_OP_ELEM *op = os_queue_out(&mgr.resolve_list.op_q);
                if (op != NULL)
                {
                    app_ble_gap_modify_resolve_list(op->operation, op->bd_addr, op->bd_type,
                                                    op->peer_irk, op->local_irk);
                    free(op);
                    op = NULL;
                }
            }
        }
        break;

    case GAP_MSG_LE_PRIVACY_RESOLUTION_STATUS_INFO:
        {
            APP_PRINT_INFO2("GAP_MSG_LE_PRIVACY_RESOLUTION_STATUS_INFO: status = 0x%x cause=0x%x",
                            msg_data.le_privacy_resolution_status_info.status,
                            msg_data.le_privacy_resolution_status_info.cause);
        }

    default:
        break;
    }
    return APP_RESULT_SUCCESS;
}

bool app_ble_gap_disconnect(T_APP_LE_LINK *p_link, T_LE_LOCAL_DISC_CAUSE disc_cause)
{
    if (p_link != NULL)
    {
        APP_PRINT_TRACE2("app_ble_gap_disconnect: conn_id %d, disc_cause %d",
                         p_link->conn_id, disc_cause);
        if (le_disconnect(p_link->conn_id) == GAP_CAUSE_SUCCESS)
        {
            p_link->local_disc_cause = disc_cause;
            return true;
        }
    }
    return false;
}

void app_ble_gap_disconnect_all(T_LE_LOCAL_DISC_CAUSE disc_cause)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        p_link = app_link_find_le_link_by_conn_id(i);
        if (p_link != NULL && p_link->used)
        {
            if (p_link->state == LE_LINK_STATE_CONNECTING
                || p_link->state == LE_LINK_STATE_CONNECTED)
            {
                app_ble_gap_disconnect(p_link, disc_cause);
            }
        }
    }
}

static void app_ble_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            APP_PRINT_INFO0("BT_EVENT_READY2");
            memcpy(app_db.factory_addr, param->ready.bd_addr, 6);
        }
        break;

    default:
        break;
    }
}

static uint32_t app_ble_gap_get_ext_adv_num(void)
{
    uint8_t adv_handle_number = 0;

    if (app_ble_cfg.rtk_app_adv_support)
    {
        adv_handle_number++;//for common adv
    }


    adv_handle_number++;

#if BAP_BROADCAST_SOURCE
    adv_handle_number++;
#endif

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    adv_handle_number++;
#endif

#if F_APP_CSIS_SUPPORT
    adv_handle_number++;
#endif

#if F_APP_BASS_SUPPORT
    adv_handle_number++;
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    adv_handle_number++;//for findmy adv
#if CONFIG_REALTEK_FINDMY_SUPPORT_CUSTOMIZED_APP
    adv_handle_number++;//for findmy cust adv
#endif
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    adv_handle_number += 2;//for gfps adv
#endif

    return adv_handle_number;
}


void app_ble_gap_ble_mgr_init(void)
{
    BLE_MGR_PARAMS param = {0};

    param.ble_ext_adv.enable = true;
    param.ble_ext_adv.adv_num = app_ble_gap_get_ext_adv_num();

    param.ble_conn.enable = true;
    uint8_t supported_max_le_link_num = le_get_max_link_num();
    param.ble_conn.link_num = ((MAX_BLE_LINK_NUM <= supported_max_le_link_num) ? MAX_BLE_LINK_NUM :
                               supported_max_le_link_num);

    param.ble_scan.enable = true;

    ble_mgr_init(&param);
    gap_vendor_le_set_host_feature(0, 1);
}

void app_ble_gap_vendor_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_VENDOR_CB_DATA cb_data;

    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_VENDOR_CB_DATA));
    APP_PRINT_INFO2("app_ble_gap_vendor_callback:cb_type %d, cause 0x%x", cb_type,
                    cb_data.gap_vendor_cause.cause);
    switch (cb_type)
    {
    case GAP_MSG_EXT_ADV_HANDLE_PRIORITY_SET:
        break;
    default:
        break;
    }

    return;
}
static void app_ble_gap_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_ble_gap_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_GAP_DISCONNECT_DELAY:
        {
            app_stop_timer(&timer_idx_gap_delay_disconn);
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(authen_fail_conn_id);

            if (p_link != NULL)
            {
                authen_fail_conn_id = 0xFF;
                p_link->cmd.tx_mask &= ~TX_ENABLE_AUTHEN_BIT;
                p_link->encryption_status = LE_LINK_UNENCRYPTIONED;
                app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED);
            }
        }
        break;

    case APP_TIMER_GAP_WAIT_FOR_AUTHEN:
        {
            app_stop_timer(&timer_idx_gap_wait_for_authen);
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(wait_for_authen_conn_id);

            if (p_link != NULL && p_link->encryption_status != LE_LINK_ENCRYPTIONED)
            {
                wait_for_authen_conn_id = 0xFF;
                p_link->cmd.tx_mask &= ~TX_ENABLE_AUTHEN_BIT;
                p_link->encryption_status = LE_LINK_UNENCRYPTIONED;
                app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED);
            }
        }
        break;

    default:
        break;
    }
}
void app_ble_gap_init(void)
{
    bt_mgr_cback_register(app_ble_gap_bt_cback);
    le_register_app_cb(app_ble_gap_cb);
    le_privacy_register_cb(app_ble_privacy_cb);
    app_ble_rand_addr_init();
    app_ble_gap_ble_mgr_init();
#if F_APP_SC_KEY_DERIVE_SUPPORT
    app_ble_key_derive_init();
#endif

    os_queue_init(&mgr.white_list.op_q);
    os_queue_init(&mgr.resolve_list.op_q);
    gap_vendor_register_cb(app_ble_gap_vendor_callback);
#if GATTC_TBL_STORAGE_SUPPORT
    gattc_tbl_storage_init();
#endif
    app_timer_reg_cb(app_ble_gap_timeout_cb, &app_ble_gap_timer_id);
}

void app_ble_gap_param_init(void)
{
    //device name and device appearance
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
    T_GAP_SCAN_MODE  scan_mode = GAP_SCAN_MODE_PASSIVE;
    uint8_t scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
    //GAP Bond Manager parameters
    uint32_t passkey = 0; // passkey "000000"
    uint8_t use_fixed_passkey = false;
    uint16_t sec_req_requirement = GAP_AUTHEN_BIT_SC_BR_FLAG | GAP_AUTHEN_BIT_GENERAL_BONDING_FLAG |
                                   GAP_AUTHEN_BIT_SC_FLAG | GAP_AUTHEN_BIT_MITM_FLAG |
                                   GAP_AUTHEN_BIT_FORCE_CENTRAL_ENCRYPT_FLAG;

    /* Initialize extended scan parameters */
    T_GAP_LOCAL_ADDR_TYPE  own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    T_GAP_LE_EXT_SCAN_PARAM extended_scan_param[GAP_EXT_SCAN_MAX_PHYS_NUM];
    extended_scan_param[0].scan_type = scan_mode;
    extended_scan_param[0].scan_interval = 0x10;
    extended_scan_param[0].scan_window = 0x10;
    uint8_t  scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period = 0;

    uint8_t supported_max_le_link_num = le_get_max_link_num();
    uint8_t link_num = ((MAX_BLE_LINK_NUM <= supported_max_le_link_num) ? MAX_BLE_LINK_NUM :
                        supported_max_le_link_num);
    le_gap_init(link_num);
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, app_cfg_nv.device_name_le);
    //Set device appearance
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);


    uint8_t  slave_init_mtu_req = true;
    le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_mtu_req),
                     &slave_init_mtu_req);

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
    uint8_t use_extended = true;
    le_set_gap_param(GAP_PARAM_USE_EXTENDED_ADV, sizeof(use_extended), &use_extended);
#endif
#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
    bool cis_flag = true;
    le_set_gap_param(GAP_PARAM_CIS_HOST_SUPPORT, sizeof(cis_flag), &cis_flag);
#endif

    le_set_gap_param(GAP_PARAM_USE_EXTENDED_ADV, sizeof(uint8_t), &(uint8_t) {true});


    // Setup the GAP Bond Manager
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(uint32_t), &passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(uint8_t), &use_fixed_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &sec_req_requirement);

    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_LOCAL_ADDR_TYPE, sizeof(own_address_type),
                          &own_address_type);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                          &scan_filter_policy);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                          &scan_filter_duplicate);
    le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_1M, &extended_scan_param[0]);
}


