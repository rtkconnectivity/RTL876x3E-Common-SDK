/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_GFPS_COMMON_BASIC_FEATURE_SUPPORT
#include "string.h"
#include "gfps.h"
#include "trace.h"
#include "app_cmd.h"
#include "app_gfps.h"
#include "bt_gfps.h"
#include "app_gfps_cfg.h"
#include "app_gfps_finder.h"
#include "app_gfps_finder_adv.h"
#include "app_gfps_msg.h"
#include "app_dult_device.h"

T_APP_GFPS_CFG extend_app_cfg_const;

static uint8_t gfps_tone_idx = 0xFF;
static uint8_t gfps_dult_tone_idx = 0xFF;
static T_APP_AUDIO_TONE_TYPE dult_tone_type = TONE_GFPS_DULT_FIND;
static T_APP_AUDIO_TONE_TYPE gfps_tone_type = TONE_GFPS_FINDME;

void app_gfps_cfg_set_tone_idx(uint8_t tone_idx)
{
    gfps_tone_idx = tone_idx;
}

uint8_t app_gfps_cfg_get_tone_idx(void)
{
    return gfps_tone_idx;
}

void app_gfps_cfg_set_dult_tone_idx(uint8_t tone_idx)
{
    gfps_dult_tone_idx = tone_idx;
}

uint8_t app_gfps_cfg_get_dult_tone_idx(void)
{
    return gfps_dult_tone_idx;
}

T_APP_AUDIO_TONE_TYPE app_gfps_cfg_dult_get_tone_type(void)
{
    return dult_tone_type;
}

void app_gfps_cfg_dult_set_tone_type(T_APP_AUDIO_TONE_TYPE tone_type)
{
    dult_tone_type = tone_type;
}

T_APP_AUDIO_TONE_TYPE app_gfps_cfg_get_tone_type(void)
{
    return gfps_tone_type;
}

void app_gfps_cfg_set_tone_type(T_APP_AUDIO_TONE_TYPE tone_type)
{
    gfps_tone_type = tone_type;
}

void app_gfps_cfg_init(void)
{
    APP_PRINT_TRACE3("app_gfps_cfg_init: F_GFPS_COMMON_BASIC_FEATURE_SUPPORT %d,"
                     "F_GFPS_COMMON_FINDER_SUPPORT %d, F_GFPS_COMMON_LE_DEVICE_SUPPORT %d,",
                     F_GFPS_COMMON_BASIC_FEATURE_SUPPORT, F_GFPS_COMMON_FINDER_SUPPORT,
                     F_GFPS_COMMON_LE_DEVICE_SUPPORT);

    APP_PRINT_TRACE5("app_gfps_cfg_init: F_GFPS_COMMON_PERIODIC_WAKEUP %d,"
                     "F_GFPS_COMMON_TWS_MODE %d, F_GFPS_COMMON_LOCATION_TRACKER %d,"
                     "F_GFPS_COMMON_BATTERY_INFO_REPORT %d, F_GFPS_COMMON_LE_L2CAP_CHANNEL %d",
                     F_GFPS_COMMON_PERIODIC_WAKEUP, F_GFPS_COMMON_TWS_MODE,
                     F_GFPS_COMMON_LOCATION_TRACKER, F_GFPS_COMMON_BATTERY_INFO_REPORT,
                     F_GFPS_COMMON_LE_L2CAP_CHANNEL);

    extend_app_cfg_const.gfps_model_id[0] = 0x00;
    extend_app_cfg_const.gfps_model_id[1] = 0x00;
    extend_app_cfg_const.gfps_model_id[2] = 0x00;

    extend_app_cfg_const.gfps_support = 1;
    extend_app_cfg_const.gfps_finder_support = 1;
    extend_app_cfg_const.gfps_le_device_support = 1;
    extend_app_cfg_const.gfps_enable_tx_power = 1;
    extend_app_cfg_const.gfps_tx_power = -6;
    extend_app_cfg_const.tone_gfps_findme = 0x9C;//power_on.wav
    extend_app_cfg_const.gfps_account_key_num = 5;
    extend_app_cfg_const.gfps_discov_adv_interval = 32;
    extend_app_cfg_const.gfps_not_discov_adv_interval = 100;

    extend_app_cfg_const.gfps_battery_info_enable = 0;
    extend_app_cfg_const.gfps_le_disconn_force_enter_pairing_mode = 0;
    extend_app_cfg_const.gfps_le_device_mode = GFPS_LE_DEVICE_MODE_LE_MODE_WITHOUT_LEA;

    uint8_t gfps_public_key[64] = {0};
    uint8_t  gfps_private_key[32] = {0};
    memcpy(extend_app_cfg_const.gfps_public_key, gfps_public_key, 64);
    memcpy(extend_app_cfg_const.gfps_private_key, gfps_private_key, 32);

    extend_app_cfg_const.tone_gfps_dult_find = 0x9C;//power_on.wav

    uint8_t  device_name[64] = {0};
    uint8_t  company_name[64] = {0};
    memcpy(extend_app_cfg_const.gfps_company_name, company_name, sizeof(company_name));
    memcpy(extend_app_cfg_const.gfps_device_name, device_name, sizeof(device_name));

    extend_app_cfg_const.gfps_device_type = GFPS_LOCATOR_TRACKER;
    uint8_t gfps_version[5] = {'3', '.', '3', '.', '0'};
    memcpy(extend_app_cfg_const.gfps_version, gfps_version, 5) ;
    extend_app_cfg_const.gfps_power_on_finder_adv_interval = 800;
    extend_app_cfg_const.disable_finder_adv_when_power_off = 1;
    extend_app_cfg_const.gfps_power_off_finder_adv_interval = 1600;
    extend_app_cfg_const.gfps_power_off_finder_adv_duration = 10;//10s

    app_gfps_cfg_set_tone_idx(extend_app_cfg_const.tone_gfps_findme);
    app_gfps_cfg_set_dult_tone_idx(extend_app_cfg_const.tone_gfps_dult_find);
    app_gfps_cfg_dult_set_tone_type(TONE_POWER_ON);
    app_gfps_cfg_set_tone_type(TONE_POWER_ON);
}

void app_gfps_finder_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                    uint16_t cmd_len, uint8_t *ack_pkt)
{
    if (!extend_app_cfg_const.gfps_finder_support)
    {
        APP_PRINT_ERROR0("app_gfps_finder_handle_cmd_set: finder not support");
        return;
    }

    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE3("app_gfps_finder_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);

    if (cmd_id == CMD_GFPS_FINDER_FEATURE)
    {
        uint8_t action = cmd_ptr[2];
        switch (action)
        {
        case GFPS_FINDER_ENTER_PAIRING_MODE:
            {
                if (extend_app_cfg_const.gfps_support)
                {
                    app_gfps_force_enter_pairing_mode(GFPS_KEY_FORCE_ENTER_PAIR_MODE);
                }
            }
            break;

        case GFPS_FINDER_EXIT_PAIRING_MODE:
            {
                if (extend_app_cfg_const.gfps_support)
                {
                    app_gfps_force_enter_pairing_mode(GFPS_EXIT_PAIR_MODE);
                }
            }
            break;

        case GFPS_FINDER_STOP_RING:
            {
                if (extend_app_cfg_const.gfps_finder_support)
                {
                    if (extend_app_cfg_const.gfps_support)
                    {
                        app_gfps_msg_handle_ring_event(GFPS_ALL_STOP);

                        uint8_t ring_state = GFPS_FINDER_RING_BUTTON_STOP;
                        app_gfps_finder_send_ring_rsp(ring_state);
                    }
                }
            }
            break;

        case GFPS_FINDER_STOP_ADV:
            {
                if (extend_app_cfg_const.gfps_support)
                {
                    app_gfps_finder_adv_handle_mmi_action();
                }
            }
            break;

        case GFPS_FINDER_ENTER_ID_READ_STATE:
            {
                if (extend_app_cfg_const.gfps_finder_support)
                {
                    app_dult_id_read_state_enable();
                }
            }
            break;

        case GFPS_FINDER_SET_TX_POWER:
            {
                if (cmd_len >= 4)
                {
                    // cmd_ptr[3] contains the TX power value (signed byte)
                    int8_t tx_power = (int8_t)cmd_ptr[3];
                    extend_app_cfg_const.gfps_tx_power = tx_power;
                    gfps_set_tx_power(extend_app_cfg_const.gfps_enable_tx_power, extend_app_cfg_const.gfps_tx_power);
                    APP_PRINT_TRACE1("app_gfps_finder_handle_cmd_set: set TX power to %d dBm", tx_power);
                }
            }
            break;

        default:
            break;
        }

        ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
    }
}

void app_gfps_lea_adv_start(void)
{

}
#endif
