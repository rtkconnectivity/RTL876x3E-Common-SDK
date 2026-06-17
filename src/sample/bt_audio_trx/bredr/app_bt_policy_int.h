/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_POLICY_INT_H_
#define _APP_BT_POLICY_INT_H_

#include <stdint.h>
#include <stdbool.h>
#include "gap_br.h"
#include "btm.h"
#include "app_link_util_cs.h"
#include "app_linkback.h"
#include "app_bt_policy_api.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define APP_BT_POLICY_B2S_CONN_TIMER_MS          5000
typedef enum
{
    STATE_STARTUP                             = 0x00,
    STATE_AFE_LINKBACK                        = 0x22,
    STATE_AFE_CONNECTED                       = 0x23,
    STATE_AFE_PAIRING_MODE                    = 0x24,
    STATE_AFE_STANDBY                         = 0x25,
    STATE_DUT_TEST_MODE                       = 0x28,
    STATE_SHUTDOWN_STEP                       = 0x40,
    STATE_SHUTDOWN                            = 0x41,
    STATE_STOP                                = 0x42,
} T_STATE;

typedef enum
{
    EVENT_STARTUP                             = 0x00,
    EVENT_SRC_CONN_SUC                        = 0x30,
    EVENT_SRC_CONN_FAIL                       = 0x31,
    EVENT_SRC_CONN_FAIL_ACL_EXIST             = 0x32,
    EVENT_SRC_CONN_FAIL_CONTROLLER_BUSY       = 0x33,
    EVENT_SRC_CONN_TIMEOUT                    = 0x34,
    EVENT_SRC_AUTH_LINK_KEY_INFO              = 0x35,
    EVENT_SRC_AUTH_LINK_KEY_REQ               = 0x36,
    EVENT_SRC_AUTH_LINK_PIN_CODE_REQ          = 0x37,
    EVENT_SRC_AUTH_SUC                        = 0x38,
    EVENT_SRC_AUTH_FAIL                       = 0x39,
    EVENT_SRC_DISCONN_LOST                    = 0x3a,
    EVENT_SRC_DISCONN_NORMAL                  = 0x3b,
    EVENT_SRC_DISCONN_ROLESWAP                = 0x3d,
    EVENT_SCO_CONN_CMPL                       = 0x3e,
    EVENT_SCO_DISCONNECTED                    = 0x3f,
    EVENT_PROFILE_SDP_ATTR_INFO               = 0x40,
    EVENT_PROFILE_DID_ATTR_INFO               = 0x41,
    EVENT_PROFILE_SDP_DISCOV_CMPL             = 0x42,
    EVENT_PROFILE_CONN_SUC                    = 0x43,
    EVENT_PROFILE_DISCONN                     = 0x44,
    EVENT_PROFILE_SDP_DISCOV_STOP             = 0x45,
    EVENT_PROFILE_CONN_FAIL                   = 0x46,
    EVENT_ROLE_MASTER                         = 0x51,
    EVENT_ROLE_SLAVE                          = 0x52,
    EVENT_ROLESWITCH_FAIL                     = 0x53,
    EVENT_ROLESWITCH_TIMEOUT                  = 0x54,
    EVENT_CONN_SNIFF                          = 0x55,
    EVENT_CONN_ACTIVE                         = 0x56,
    EVENT_PAIRING_MODE_TIMEOUT                = 0x60,
    EVENT_DISCOVERABLE_TIMEOUT                = 0x61,
    EVENT_DEDICATED_ENTER_PAIRING_MODE        = 0x70,
    EVENT_DEDICATED_EXIT_PAIRING_MODE         = 0x71,
    EVENT_DEDICATED_CONNECT                   = 0x72,
    EVENT_DEDICATED_DISCONNECT                = 0x73,
    EVENT_DISCONNECT_ALL                      = 0x74,
    EVENT_LINKBACK_DELAY_TIMEOUT              = 0x76,
    EVENT_SRC_CONN_IND_TIMEOUT                = 0x78,
    EVENT_ENTER_DUT_TEST_MODE                 = 0x79,
    EVENT_RESTORE                             = 0x83,
    EVENT_SHUTDOWN_STEP_TIMEOUT               = 0x90,
    EVENT_SHUTDOWN                            = 0x91,
    EVENT_STOP                                = 0xa0,
} T_EVENT;

typedef enum
{
    STABLE_ENTER_MODE_NORMAL                  = 0x00,
    STABLE_ENTER_MODE_AGAIN                   = 0x01,
    STABLE_ENTER_MODE_DEDICATED_PAIRING       = 0x02,
    STABLE_ENTER_MODE_NOT_PAIRING             = 0x03,
    STABLE_ENTER_MODE_DIRECT_PAIRING          = 0x04,
} T_STABLE_ENTER_MODE;

typedef enum
{
    SHUTDOWN_STEP_BEGIN                       = 0x00,
    SHUTDOWN_STEP_START_DISCONN_B2S_PROFILE   = 0x10,
    SHUTDOWN_STEP_WAIT_DISCONN_B2S_PROFILE    = 0x11,
    SHUTDOWN_STEP_START_DISCONN_B2S_LINK      = 0x20,
    SHUTDOWN_STEP_WAIT_DISCONN_B2S_LINK       = 0x21,
    SHUTDOWN_STEP_END                         = 0x40,
} T_SHUTDOWN_STEP;

typedef struct
{
    uint8_t num_max;
    uint8_t num;
} T_B2S_CONNECTED;

typedef struct
{
    T_BP_IND_FUN ind_fun;
    bool at_once_trigger;
} T_STARTUP_PARAM;

typedef struct
{
    uint8_t *bd_addr;
    uint32_t prof;
    uint16_t cause;
    uint8_t bud_role;
    T_BT_LINK_KEY_TYPE key_type;
    uint8_t *link_key;
    T_BT_SDP_ATTR_INFO *sdp_info;
    bool is_special;
    bool check_bond_flag;
    bool not_check_addr_flag;
    bool is_visiable;
    bool is_connectable;
    bool is_force;

    bool is_later_avrcp;
    bool is_later_hid;
} T_BT_PARAM;



typedef enum
{
    ROLESWITCH_EVENT_LINK_CONNECTED      = 0x00,
    ROLESWITCH_EVENT_LINK_ACTIVE         = 0x01,
    ROLESWITCH_EVENT_LINK_DISCONNECTED   = 0x02,
    ROLESWITCH_EVENT_FAIL_RETRY          = 0x03,
    ROLESWITCH_EVENT_FAIL_RETRY_MAX      = 0x04,
    ROLESWITCH_EVENT_ROLE_CHANGED        = 0x05,
    ROLESWITCH_EVENT_SCO_CHANGED         = 0x06,
} T_ROLESWITCH_EVENT;

void app_bt_policy_state_machine(T_EVENT event, void *param);
uint8_t b2s_connected_get_num_max(void);
uint32_t get_profs_by_bond_flag(uint32_t bond_flag);
void b2s_connected_set_last_conn_index(uint8_t conn_idx);
uint8_t b2s_connected_get_last_conn_index(void);
void app_bt_policy_state_machine(T_EVENT event, void *param);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_POLICY_H_ */
