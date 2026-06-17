/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_GFPS_H_
#define _APP_GFPS_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "btm.h"
#include "gfps.h"
#include "remote.h"
#include "os_queue.h"
#include "ble_ext_adv.h"
#include "gfps_sass_conn_status.h"
#include "app_msg.h"

/** @defgroup APP_RWS_GFPS App Gfps
  * @brief App Gfps
  * @{
  */

typedef enum
{
    GFPS_LOCATOR_TRACKER      = 1,
    GFPS_WATCH                = 146,
    GFPS_HEADPHONES           = 149,
    GFPS_EARPHONES            = 150,
} T_GFPS_DEVICE_TYPE;

typedef struct
{
    bool is_gfps_pairing;
    bool gfps_raw_passkey_received;
    bool le_bond_confirm_pending;
    bool edr_bond_confirm_pending;
    bool auth_param_change;

    uint8_t  io_cap;
    uint16_t auth_flags;
    uint32_t gfps_raw_passkey;
    uint32_t le_bond_passkey;
    uint32_t edr_bond_passkey;
    uint8_t  edr_bond_bd_addr[6];

    uint8_t          gfps_conn_id;
    uint16_t         gfps_msg_cid;
    T_GAP_CONN_STATE gfps_conn_state;

    uint8_t  *p_gfps_cmd;
    uint16_t gfps_cmd_len;
} T_GFPS_LINK;

typedef enum
{
    GFPS_ACTION_IDLE,
    GFPS_ACTION_ADV_DISCOVERABLE_MODE_WITH_MODEL_ID,
    GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE,
    GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI,
} T_GFPS_ACTION;

typedef struct
{
    uint8_t                    gfps_adv_handle;
    T_BLE_EXT_ADV_MGR_STATE    gfps_adv_state;
    T_GFPS_ACTION              gfps_curr_action;

    T_SERVER_ID                gfps_id;
    uint8_t                    current_conn_id;
    T_GFPS_BATTERY_INFO        gfps_battery_info;
    uint8_t                    random_address[6];
    bool                       gfps_linkback_init;
    bool                       force_enter_pair_mode;

    bool                       is_gfps_additional_pairing;
    uint8_t                    additional_default_io_cap;
    uint8_t                    additional_default_auth_flags;
    uint8_t                    gfps_additional_conn_id;
    bool                       gfps_additional_passkey_from_gfps_received;
    bool                       gfps_additional_passkey_from_gfps_pending;
    uint8_t                    gfps_additional_bond_addr[6];
    uint32_t                   gfps_additional_passkey_from_stack;
    uint32_t                   gfps_additional_passkey_from_gfps;
    T_GFPS_PASSKEY_STATUS_CODE status_code;
    uint8_t                    failure_count;
    bool                       receive_pair_req;
    bool                       receive_passkey;
    bool                       allow_write_account_key;
    bool                       is_gfps_retroactive_pairing;
    uint8_t                    account_key_write_counts;
    bool                       app_stop_gfps_adv;
    bool                       reject_pair;
    //T_GFPS_ADDITIONAL_BOND_TYPE gfps_additional_bond_type;
} T_GFPS_DB;

typedef enum
{
    GFPS_KEY_FORCE_ENTER_PAIR_MODE        = 0x00,
    GFPS_LE_DISCONN_FORCE_ENTER_PAIR_MODE = 0x01,
    GFPS_EXIT_PAIR_MODE                   = 0x02,
    GFPS_BT_POLICY_FORCE_EXIT_PAIR_MODE   = 0x03,
    GFPS_BT_POLICY_FORCE_ENTER_PAIR_MODE  = 0x04,

} T_GFPS_FORCE_ENTER_PAIR_MODE;

/**
 * @brief google Fast pair initialize
 *
 * @param void
 * @return void
 */
void app_gfps_init(void);

/**
 * @brief gfps adv initialize
 *
 */
void app_gfps_adv_init(void);

/**
 * @brief start gfps adv according to expected gfps action
 *
 * @param gfps_next_action @ref T_GFPS_ACTION
 * @return true  success
 * @return false fail
 */
bool app_gfps_next_action(T_GFPS_ACTION gfps_next_action);

/**
 * @brief get resolvable private address used by gfps adv
 *
 * @param random_bd random reslovable privacy address
 * @return void
 */
void app_gfps_get_random_addr(uint8_t *random_bd);

/**
 * @brief update gfps adv interval
 *
 * default interval is:
 * in pairing mdoe : extend_app_cfg_const.gfps_discov_adv_interval;
 * not in pairing mode: extend_app_cfg_const.gfps_not_discov_adv_interval;
 *
 * @param adv_interval Range:[32,8000], Unit:0.625ms [20ms, 5s]
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_gfps_adv_update_adv_interval(uint32_t adv_interval);

/**
 * @brief handle user confirmation for legacy pairing.
 * if not gfps pair, gfps module will not call gap_br_user_cfm_req_cfm to confirm legacy pairiing.
 * maybe app need call gap_br_user_cfm_req_cfm by themself.
 * @param confirm_req  bt user confirm request
 * @return void
 */
bool app_gfps_handle_bt_user_confirm(T_BT_EVENT_PARAM_LINK_USER_CONFIRMATION_REQ confirm_req);

/**
 * @brief handle user confirmation for le pairing.
 * if not gfps pair, gfps module will not call le_bond_user_confirm to confirm le pairiing.
 * maybe app need call le_bond_user_confirm by themself.
 * @param conn_id BLE connection id
 * @return void
 */
bool app_gfps_handle_ble_user_confirm(uint8_t conn_id);

/**
 * @brief enter non discoverable mode
 *
 * @param void
 * @return void
 */
void app_gfps_enter_nondiscoverable_mode(void);

/**
 * @brief gfps handle case status
 *
 * @param open 1:case open, 0: case close
 */
void app_gfps_handle_case_status(bool open);

/**
 * @brief gfps handle b2s connected
 *
 * @param bd_addr
 */
void app_gfps_handle_b2s_connected(uint8_t *bd_addr);

/**
 * @brief app_gfps_device_handle_b2s_ble_bonded
 *
 * @param conn_id                 le connection id
 * @param p_remote_identity_addr  remote identity address
 */
void app_gfps_handle_b2s_ble_bonded(uint8_t conn_id, uint8_t *p_remote_identity_addr);

/**
 * @brief gfps set ble connection parameter
 *
 * @param conn_id ble connection id
 */
void app_gfps_set_ble_conn_param(uint8_t conn_id);

/**
 * @brief GFPS gen sass connection status
 *
 * @param conn_status @ref T_SASS_CONN_STATUS_FIELD
 * @return true  Success
 * @return false Fail
 */
bool app_gfps_gen_conn_status(T_SASS_CONN_STATUS_FIELD *conn_status);

/**
 * @brief GFPS notify sass connection status
 *
 */
void app_gfps_notify_conn_status(void);

/**
 * @brief GFPS update sass connection status
 *
 */
void app_gfps_adv_update_conn_status(void);

/**
 * @brief GFPS check sass connection status
 *
 */
void app_gfps_adv_check_sass_conn_status(void);

/**
 * @brief start gfps adv and le audio adv for gfps pairing
 *
 * @param device_role    bud role @ref T_REMOTE_SESSION_ROLE
 * @return void
 */
void app_gfps_le_device_adv_start(T_REMOTE_SESSION_ROLE device_role);

/**
 * @brief gfps handle ble connection information
 *
 * @param remote_bd_addr   remote bd address
 * @param remote_addr_type remote bd address type
 * @return true  success
 * @return false fail
 */
bool app_gfps_ble_conn_info_handle(uint8_t *remote_bd_addr, uint8_t remote_addr_type);

/**
 * @brief gfps le linkback information init
 *
 * @param conn_id le connection id
 */
void app_gfps_linkback_info_init(uint8_t conn_id);

/**
 * @brief gfps force enter pairing mode
 *
 * @param status @ref T_GFPS_FORCE_ENTER_PAIR_MODE
 */
void app_gfps_force_enter_pairing_mode(uint8_t status);

/**
 * @brief secondary bud handle addition ble user confirm
 *
 * @param conn_id gfps addition connection id
 */
bool app_gfps_handle_additional_ble_user_confirm(uint8_t conn_id);

/**
 * @brief secondary bud handle addition ble bond info
 *
 * @param buf @ref T_GFPS_ADDITIONAL_BOND_DATA
 */
void app_gfps_sec_handle_addition_bond_info(uint8_t *buf);

/**
 * @brief primary bud handle addition ble bond info
 *
 * @param buf @ref T_GFPS_ADDITIONAL_BOND_DATA
 */
void app_gfps_pri_handle_additional_bond_info(uint8_t *buf);

/**
 * @brief secondary bud handle addition change IO CAP info
 *
 * @param change_io_cap true change to GAP_IO_CAP_DISPLAY_YES_NO
 */
void app_gfps_sec_handle_additional_io_cap(bool change_io_cap);

/**
 * @brief gfps update resolvable private address
 *
 * @param gfps_generate_rpa if gfps_generate_rpa is true
 *                         gfps will generate a valid rpa and set this rpa into stack, and update this rpa in gfps.
 *                         otherwise:
 *                         directly get rpa from stack, and update this rpa in gfps.
 *
 */
void app_gfps_update_rpa(bool gfps_generate_rpa);

/**
 * @brief gfps update announcement
 *
 */
void app_gfps_update_announcement(uint8_t announcement);

/**
 * @brief Rest failure count
 *
 */
void app_gfps_reset_failure_count(void);

/**
 * @brief Check receive pairing request
 *
 */
void app_gfps_check_receive_pair_req(void);

/**
 * @brief Check receive passkey
 *
 */
void app_gfps_check_receive_passkey(void);

/**
 * @brief Set allow write account key or not
 *
 * @param allow  true allow; false not allow
 */
void app_gfps_set_allow_write_account_key(bool allow);

/**
 * @brief Reset account key write counts
 *
 */
void app_gfps_reset_account_key_write_counts(void);

/**
 * @brief Get ble address
 *
 * @param ble_addr  ble address
 */
void app_gfps_get_ble_addr(uint8_t *ble_addr);

/**
 * @brief Read firmware version
 *
 * @return T_APP_RESULT  Result
 */
T_APP_RESULT app_gfps_read_firmware_version(void);

/**
 * @brief GFPS message queue init
 *
 */
void app_gfps_msg_queue_init(void);

/**
 * @brief GFPS module init
 *
 */
void app_gfps_module_init(void);

/**
 * @brief GFPS adv module init
 *
 */
void app_gfps_module_adv_init(void);

/**
 * @brief GFPS set profile mask
 *
 */
void app_gfps_set_profile_mask(void);

/**
 * @brief GFPS module set CTKD flags
 * if not set, some phone will not connect legacy profile(A2DP/HFP)
 * @param p_key_convert_flag flags
 */
void app_gfps_set_key_convert_flag(uint8_t *p_key_convert_flag);

/**
 * @brief GFPS handle le numerical comparison bond cfm
 *
 * @param conn_id connection id
 * @return true  success GFPS will call le_bond_user_confirm
 * @return false fail    GFPS will not call le_bond_user_confirm, maybe need app call le_bond_user_confirm
 */
bool app_gfps_handle_le_numerical_comparison_bond_cfm(uint8_t conn_id);

/**
 * @brief GFPS LEA enter pair mode by MMI action
 *
 */
void app_gfps_handle_mmi_lea_enter_pair_mode(void);

/**
 * @brief GFPS module decide to accept or reject this BLE link
 *
 * @param remote_bd_addr  remote bd address
 * @param remote_addr_type remote bd address type
 * @return T_APP_RESULT  APP_RESULT_ACCEPT or APP_RESULT_REJECT
 */
T_APP_RESULT app_gfps_ble_connection_accept_or_reject(uint8_t *remote_bd_addr,
                                                      uint8_t remote_addr_type);

void app_gfps_in_case_out_case_report_battery(uint8_t remote_loc, uint8_t local_loc);

/**
 * @brief GFPS module call this API to set extend_app_cfg_const.gfps_le_device_support true or false.
 * Normally extend_app_cfg_const.gfps_le_device_support is hardcoded by the MCU cfg tool or FTL.
 * Some customers want to dynamically enable or disable it, So we provide this this API.
 * Note that:
 * If the LE audio module is not enabled(app_lea_mgr_set_lea_mode(false)), gfps_le_device_support cannot be set to 1.
 * Also, if gfps_le_device_support is modified, a restart/reboot is required for the changes to take effect.
 * @param enable  true  Set extend_app_cfg_const.gfps_le_device_support to 1
 *                false Set extend_app_cfg_const.gfps_le_device_support to 0
 */
void app_gfps_set_lea_support(bool enable);

/**
 * @brief Get reject pair flag
 *
 * @return true  reject pair flag is true
 * @return false reject pair flag is false
 */
bool app_gfps_get_reject_pair_flag(void);

void app_gfps_clear_reject_pair_flag(void);

/**
 * @brief if GFPS pairing fail, do not allow normal pair directly for MITM protection.
 * GFPS module will set reject pair flag and start a timer to clear this flag after 30s.
 * During this period, if receive a pairing request,
 * Normally app shall reject this pairing request to avoid potential security risk.
 * After 30s, the reject pair flag will be cleared by timer, then app can accept pairing request again.
 * @param conn_id connection id
 */
void app_gfps_handle_le_justwork_bond_cfm(uint8_t conn_id);
/** End of APP_RWS_GFPS
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
