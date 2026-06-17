/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_GFPS_CFG_H_
#define _APP_GFPS_CFG_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_cfg.h"
#include "remote.h"
#include "app_gfps_link_util.h"
#include "app_link_util_cs.h"
#include "app_audio_cfg.h"
#include "app_audio_tone_cfg.h"

#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @brief  Read only configurations for gfps */
typedef struct
{
    uint8_t gfps_model_id[3];
    uint8_t gfps_support : 1;
    uint8_t gfps_enable_tx_power : 1;
    uint8_t gfps_battery_info_enable : 1;
    uint8_t gfps_battery_remain_time_enable : 1;
    uint8_t gfps_battery_show_ui : 1;
    uint8_t gfps_left_ear_batetry_support : 1;
    uint8_t gfps_right_ear_batetry_support : 1;
    uint8_t gfps_case_battery_support : 1;
    int8_t  gfps_tx_power;
    uint8_t gfps_account_key_num;
    uint8_t gfps_sass_support : 1;
    uint8_t gfps_sass_support_dyamic_multilink_switch : 1;
    uint8_t gfps_sass_rsv : 5;
    uint8_t gfps_finder_support : 1;
    uint8_t gfps_le_device_support : 1;
    uint8_t gfps_le_disconn_force_enter_pairing_mode: 1;
    uint8_t gfps_le_device_mode: 2;
    uint8_t gfps_rsv : 4;
    uint16_t gfps_discov_adv_interval;
    uint16_t gfps_not_discov_adv_interval;
    uint8_t  gfps_public_key[64];
    uint8_t  gfps_private_key[32];
    uint8_t tone_gfps_findme;
    uint8_t tone_gfps_dult_find;
    uint8_t gfps_company_name[64];
    uint8_t gfps_device_name[64];
    uint8_t gfps_device_type;
    uint8_t gfps_version[10];
    uint32_t gfps_power_on_finder_adv_interval;
    uint32_t gfps_power_off_finder_adv_interval;
    uint16_t gfps_power_off_finder_adv_duration;
    uint8_t disable_finder_adv_when_power_off;
} T_APP_GFPS_CFG;

extern T_APP_GFPS_CFG extend_app_cfg_const;

typedef enum
{
    GFPS_FINDER_ENTER_PAIRING_MODE  = 0x01,
    GFPS_FINDER_EXIT_PAIRING_MODE   = 0x02,
    GFPS_FINDER_STOP_RING           = 0x03,
    GFPS_FINDER_STOP_ADV            = 0x04,
    GFPS_FINDER_ENTER_ID_READ_STATE = 0x05,
    GFPS_FINDER_SET_TX_POWER        = 0x06,
} T_GFPS_FINDER_ACTION;

/**
    * @brief  GFPS config module init
    * @param  void
    * @return void
    */
void app_gfps_cfg_init(void);

/**
 * @brief Handle finder command
 *
 * @param app_idx  Reporter app index
 * @param cmd_path Reporter command path
 * @param cmd_ptr  command data
 * @param cmd_len  command length
 * @param ack_pkt  ACK
 */
void app_gfps_finder_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                    uint16_t cmd_len, uint8_t *ack_pkt);

/**
 * @brief Start LE audio adv
 *
 */
void app_gfps_lea_adv_start(void);

/**
 * @brief Get gfps find me tone index
 *
 * @return uint8_t find me tone index
 */
uint8_t app_gfps_cfg_get_tone_idx(void);

/**
 * @brief Get dult tone index
 *
 * @return uint8_t  dult tone index
 */
uint8_t app_gfps_cfg_get_dult_tone_idx(void);

/**
 * @brief Get dult tone type
 *
 * @return T_APP_AUDIO_TONE_TYPE  tone type
 */
T_APP_AUDIO_TONE_TYPE app_gfps_cfg_dult_get_tone_type(void);

/**
 * @brief Get gfps find me tone type
 *
 * @return T_APP_AUDIO_TONE_TYPE tone type
 */
T_APP_AUDIO_TONE_TYPE app_gfps_cfg_get_tone_type(void);

#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
