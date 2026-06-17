/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CFG_NV_H_
#define _APP_CFG_NV_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

//App config data
#define APP_DATA_SYNC_WORD                  0xAA55

#define APP_MCU_CFG_SYNC_WORD               0xAA55AA55
#define APP_MCU_CFG_SYNC_WORD_LEN           4

#define APP_CONFIG_OFFSET                   0
#define APP_CONFIG_SIZE                     1024

#define APP_LED_OFFSET                      (APP_CONFIG_OFFSET + APP_CONFIG_SIZE)
#define APP_LED_SIZE                        512

#define SYS_CONFIG_OFFSET                   (APP_LED_OFFSET + APP_LED_SIZE)
#define SYS_CONFIG_SIZE                     512

#define APP_CONFIG2_OFFSET                  (SYS_CONFIG_OFFSET + SYS_CONFIG_SIZE)
#define APP_CONFIG2_SIZE                    512

#define TONE_DATA_OFFSET        4096 //Rsv 4K for APP parameter for better flash control
#define TONE_DATA_SIZE          3072


//FTL start
#define APP_RW_DATA_ADDR        3072
#define APP_RW_DATA_SIZE        360

#define FACTORY_RESET_OFFSET    124         //this value is fixed, SHALL NOT modify

#define GCSS_ATT_TBL_INFO_ADDR              (APP_RW_DATA_ADDR + APP_RW_DATA_SIZE)
#define GCSS_ATT_TBL_INFO_SIZE              (30 * 16)

#define APP_FINDMY_INFO_ADDR                (GCSS_ATT_TBL_INFO_ADDR + GCSS_ATT_TBL_INFO_SIZE)
#define APP_FINDMY_INFO_SIZE                320

#define APP_RW_OTA_SRV_CHANGE_INFO_ADDR     (APP_FINDMY_INFO_ADDR + APP_FINDMY_INFO_SIZE)
#define APP_RW_OTA_SRV_CHANGE_INFO_SIZE     64
//FTL end


/** @defgroup APP_CFG App Cfg
  * @brief App Cfg
  * @{
  */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_CFG_Exported_Types App Cfg Types
    * @{
    */
/** @brief  Read/Write configurations for inbox audio application  */
typedef struct
{
    uint32_t sync_word;
    uint32_t length;
} T_APP_CFG_NV_HDR;

/*T_APP_CFG_NV usage:
The offsets of existing items MUST NOT be changed:
1. If deleting existing items, positions of these items shall be reserved
2. If adding new items, the new items shall be appended in the end
*/
typedef struct
{
    T_APP_CFG_NV_HDR hdr;
    //offset: 8
    uint8_t device_name_legacy[40];
    uint8_t device_name_le[40];

    uint8_t le_single_random_addr[6];

    uint8_t bud_role;

    uint8_t bud_local_addr[6];
    uint8_t reserved1[6];
    uint8_t reserved2[6];
    uint16_t reserved3;
    //offset: 116
    uint32_t single_tone_timeout_val;
    //offset: 120
    uint8_t single_tone_tx_gain;
    uint8_t single_tone_channel_num;
    uint8_t reserved4;
    uint8_t eq_idx_anc_mode_record;

    //offset: 124
    union
    {
        uint8_t offset_app_is_power_on;
        struct
        {
            uint8_t is_app_reboot : 1;
            uint8_t enable_multi_link : 1;
            uint8_t app_is_power_on : 1;
            uint8_t factory_reset_done : 1;
            uint8_t adp_factory_reset_power_on : 1;
            uint8_t reserved7 : 1;
            uint8_t gpio_out_detected : 1;
            uint8_t adaptor_is_plugged : 1;
        };
    };

    uint8_t eq_idx;

    uint8_t reserved5;
    uint8_t local_level;
    uint8_t reserved_local_loc;
    union
    {
        uint8_t offset_smart_chargerbox;
        struct
        {
            uint8_t power_off_cause_cmd : 1;
            uint8_t power_on_cause_cmd: 1;
        };
    };

    uint8_t audio_gain_level[8];
    uint8_t voice_gain_level[8];
    uint8_t line_in_gain_level;

    uint8_t voice_prompt_language;

    uint8_t spk_channel;
    uint8_t reserved;
    uint8_t pin_code_size;
    uint8_t pin_code[8];

    union
    {
        uint8_t tone_volume_out_level;
        struct
        {
            uint8_t voice_prompt_volume_out_level : 4;
            uint8_t ringtone_volume_out_level : 4;
        };
    };

    uint8_t eq_idx_gaming_mode_record;
    uint8_t eq_idx_normal_mode_record;
    uint8_t apt_eq_idx;

    union
    {
        uint8_t offset_is_dut_test_mode;
        struct
        {
            uint8_t is_dut_test_mode : 1;
            uint8_t adaptor_changed : 1;

            uint8_t auto_power_on_after_factory_reset : 1;
            uint8_t trigger_dut_mode_from_power_off : 1;
        };
    };

    union
    {
        uint8_t offset_jig_dongle_id;
        struct
        {
            uint8_t jig_dongle_id : 5;
            uint8_t jig_subcmd : 3;
        };
    };

    union
    {
        uint8_t offset_listening_mode_cycle;
        struct
        {

            uint8_t rws_low_latency_level_record : 3;
            uint8_t disallow_power_on_when_vbat_in: 1;
        };
    };

    uint8_t abs_vol[8];
    uint8_t reserved6[6];
    uint8_t eq_idx_line_in_mode_record;
    uint8_t app_pair_idx_mapping[8];
    uint8_t either_bud_has_vol_ctrl_key : 1;


    union
    {
        uint8_t offset_one_wire_aging_test_done;

        struct
        {
            uint8_t one_wire_aging_test_done : 1;
            uint8_t one_wire_disable_power_on : 1;
            uint8_t one_wire_reset_to_uninitial_state : 1;
            uint8_t one_wire_rsv : 4;
        };
    };

    uint8_t lea_vol_setting;
    uint8_t lea_vol_out_mute;
    uint8_t lea_vcs_change_cnt;
    uint8_t lea_reserve[6];
    uint8_t lea_srcaddr[6];
    uint8_t lea_srctype;
    uint8_t lea_srcsid;
    uint8_t lea_srcch;
    uint8_t lea_valid;
    uint8_t lea_encryp;
    uint8_t lea_code[16];
    uint8_t bis_audio_gain_level;

    uint8_t trigle_dongle_support_passkey;
    struct
    {
        uint8_t hid_device_2p4_supported : 1;
        uint8_t hid_device_ble_active : 1;
        uint8_t hid_device_2p4_active : 1;
        uint8_t hid_device_2p4_reserved : 5;
        uint8_t audio_device_lea_supported : 1;
        uint8_t audio_device_unicast_supported : 1;
        uint8_t audio_device_broadcast_supported : 1;
        uint8_t audio_device_assistant_supported : 1;
        uint8_t audio_device_policy : 1;
        uint8_t audio_device_reserved : 3;
        uint8_t office_1_br_support : 1;
        uint8_t office_1_reserved : 7;
    } triple_feature;

    uint8_t local_irk[16];
    uint8_t chargecase_remote_bd[6];
} T_APP_CFG_NV;

/** End of APP_CFG_Exported_Types
    * @}
    */

extern T_APP_CFG_NV app_cfg_nv;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_CFG_Exported_Functions App Cfg Functions
    * @{
    */

void app_mcu_cfg_nv_load(void);

/**
    * @brief  store config parameters to ftl
    * @param  void
    * @return ftl result
    */
uint32_t app_cfg_store_all(void);

/**
    * @brief    Save FTL value defined in @ref T_APP_CFG_NV
    * @note
    * @param    pdata  specify data buffer
    * @param    size   size to store
    * @return   status
    * @return   0  status successful
    * @return   otherwise fail
    */
uint32_t app_cfg_store(void *pdata, uint16_t size);

/**
    * @brief  reset config parameters
    * @param  void
    * @return ftl result
    */
uint32_t app_cfg_reset(void);

/** @} */ /* End of group APP_CFG_Exported_Functions */

/** End of APP_CFG
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif /*_AU_CFG_H_*/
