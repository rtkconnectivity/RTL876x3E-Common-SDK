/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CHARGING_CASE_CMD_H_
#define _APP_CHARGING_CASE_CMD_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief ADAPTOR DET to APP cmd
 */
typedef enum
{
    AD2B_CMD_NONE                   = 0x00,
    AD2B_CMD_FACTORY_RESET          = 0x01,
    AD2B_CMD_OPEN_CASE              = 0x03,
    AD2B_CMD_CLOSE_CASE             = 0x04,
    AD2B_CMD_ENTER_PAIRING          = 0x06,
    AD2B_CMD_ENTER_DUT_MODE         = 0x08,
    AD2B_CMD_CASE_CHARGER           = 0x0D,// case charger battery
} T_AD2B_CMD_ID;

typedef enum
{
    CHARGING_CASE_CMD_EXIT_OTA      = 0x00,
    CHARGING_CASE_CMD_ENTER_OTA     = 0x01,
} T_CHARGING_CASE_OTA_CMD;

typedef enum
{
    CHARGING_CASE_CMD_MUTE_STATUS   = 0x01,
    CHARGING_CASE_CMD_BUD_LOCATION  = 0x02,
    CHARGING_CASE_CMD_BATTERY_STATUS = 0x03,
    CHARGING_CASE_CMD_BUD_INFO      = 0x04,
    CHARGING_CASE_CMD_CONNECT_STATUS = 0x05,
} T_CHARGING_CASE_STATUS_CMD;

typedef enum
{
    CHARGING_CASE_ENABLE_LE_ADV      = 0x02,
    CHARGING_CASE_ENABLE_LEGACY_ADV  = 0x03,
} T_CHARGING_CASE_OTA_DEVICE_TYPE;

typedef enum
{
    CHARGING_CASE_BOND_STATE_NONE     = 0x00,
    CHARGING_CASE_BOND_STATE_BONED    = 0x01,
} T_CHARGING_CASE_BOND_STATE;

void app_charging_case_cmd_handle(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                  uint16_t cmd_len, uint8_t *ack_pkt);

void app_charging_case_headset_event_handler(uint16_t conn_handle, uint8_t *p_data,
                                             uint16_t data_len);
void app_charging_case_handle_le_disconn(uint8_t *bd_addr, uint8_t local_disc_cause);

void app_charging_case_send_cmd(uint16_t cmd_id);

void app_charging_case_init(void);

#ifdef __cplusplus
}
#endif /* __cplus */
#endif


