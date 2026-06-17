/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_flags.h"
#include "app_cfg_nv.h"

#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

#define MAX_BR_LINK_NUM                     2

#if F_APP_MULTILINK_ENABLE
#undef MAX_BR_LINK_NUM
#define MAX_BR_LINK_NUM                     2
#endif

#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO && F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
#define MAX_BLE_LINK_NUM                    4
#else
#if F_APP_1_EP_HID_MULTI_DEV_CDC_SUPPORT
#define MAX_BLE_LINK_NUM                    6
#else
#define MAX_BLE_LINK_NUM                    3
#endif
#endif
#define PLAYBACK_POOL_SIZE                  (15*1024)
#define VOICE_POOL_SIZE                     (2*1024)
#define RECORD_POOL_SIZE                    (1*1024)
#define NOTIFICATION_POOL_SIZE              (3*1024)

#if (F_APP_BT_AUDIO_TRANSMITTER_DEMO_SUPPORT || F_APP_CHARGE_CASE_DEMO_SUPPORT)
#undef VOICE_POOL_SIZE
#undef NOTIFICATION_POOL_SIZE
#define VOICE_POOL_SIZE                     (2*1024) //test for hfp
#define NOTIFICATION_POOL_SIZE              (0)
#if (F_APP_SD_CARD_LOCALPLAY == 0 && F_APP_ATTACH_LOCAL_PLAY_SUPPORT == 0)
#undef PLAYBACK_POOL_SIZE
#define PLAYBACK_POOL_SIZE                  (0*1024)
#endif

#endif

#if F_APP_BT_AUDIO_TRANSCEIVER_DEMO_SUPPORT
#undef VOICE_POOL_SIZE
#undef NOTIFICATION_POOL_SIZE
#define VOICE_POOL_SIZE                     (0)
#define NOTIFICATION_POOL_SIZE              (0)
#if (F_APP_ATTACH_LOCAL_PLAY_SUPPORT == 0)
#undef PLAYBACK_POOL_SIZE
#define PLAYBACK_POOL_SIZE                  (0*1024)
#endif
#endif

typedef enum
{
    DSP_OUTPUT_LOG_NONE = 0x0,          //!< no DSP log.
    DSP_OUTPUT_LOG_BY_UART = 0x1,       //!< DSP log by uart directly.
    DSP_OUTPUT_LOG_BY_MCU = 0x2,        //!< DSP log by MCU.
} T_DSP_OUTPUT_LOG;

typedef enum
{
    DEVICE_BUD_SIDE_LEFT      = 0,
    DEVICE_BUD_SIDE_RIGHT     = 1,
} T_DEVICE_BUD_SIDE;

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

#define DEVICE_NAME_MAX_LEN     40

/** @brief  Read only configurations for inbox audio application */
typedef struct
{
    uint32_t sync_word;

    //device feature
    uint8_t device_name_legacy_default[DEVICE_NAME_MAX_LEN]; //this field offset is fixed, SHALL NOT modify
    uint8_t device_name_le_default[DEVICE_NAME_MAX_LEN];     //this field offset is fixed, SHALL NOT modify
    uint8_t ext_pa_enable: 1;
    uint8_t ext_lna_enable: 1;
    uint8_t ext_rsv: 6;
    uint8_t ext_pa_pinmux;
    uint8_t ext_lna_pinmux;
    //Timer: 32 bytes(offset 0x04)
    uint8_t timer_hfp_ring_period;

    uint8_t bud_role;
    uint8_t bud_side : 1; // This field is invalid
    uint8_t le_name_sync_to_legacy_name : 1; // This field is invalid
    uint8_t class_of_device[3];
    uint8_t enable_dlps : 1;

    uint32_t supported_profile_mask;

    //power on/off
    uint8_t reset_eq_when_power_on : 1;
    uint8_t auto_power_on_and_enter_pairing_mode_before_factory_reset : 1;
    uint8_t enable_power_on_adv_with_timeout : 1;
    uint8_t auto_power_on_after_factory_reset : 1;
    uint8_t enable_power_on_linkback : 1;
    uint8_t enable_power_on_linkback_fail_enter_pairing : 1;

    uint16_t timer_auto_power_off;
    uint16_t timer_auto_power_off_while_phone_connected_and_anc_apt_off;
    uint8_t enable_power_off_to_dlps_mode : 1;
    uint8_t enable_pairing_timeout_to_power_off : 1;
    uint8_t disable_power_off_wdt_reset : 1;
    uint8_t disallow_auto_power_off_when_airplane_mode: 1;

    //debug
    uint16_t timer_monitor_heap_and_timer_timeout;
    uint16_t timer_pairing_while_one_conn_timeout;

    //peripheral and pinnmux
    uint8_t enable_data_uart : 1;
    uint8_t enable_tx_wake_up : 1;
    uint8_t enable_rx_wake_up : 1;
    uint8_t one_wire_uart_support : 1;
    uint8_t resved : 1;

    uint8_t line_in_support : 1;
    uint8_t ext_buck_support: 1;
    uint8_t ext_buck_vendor: 3;
    uint8_t thermistor_power_support : 1;

    //DUT
    uint8_t enable_5_mins_auto_power_off_under_dut_test_mode: 1;
    uint8_t enable_repeat_dut_test_tone : 1;

    //factory reset
    uint8_t enable_factory_reset_whitout_limit : 1;
    uint8_t enable_not_reset_device_name : 1;

//Peripheral: 48 bytes (offset 0x1c4)
    uint8_t data_uart_tx_pinmux;
    uint8_t data_uart_rx_pinmux;
    uint8_t tx_wake_up_pinmux;
    uint8_t rx_wake_up_pinmux;
    uint8_t i2c_0_dat_pinmux;
    uint8_t i2c_0_clk_pinmux;
    uint8_t line_in_pinmux;

    uint8_t thermistor_power_pinmux;
    uint8_t enable_ctrl_ext_amp : 1;
    uint8_t ext_amp_pinmux;
    uint8_t enable_ext_amp_high_active : 1;

    uint8_t one_wire_uart_data_pinmux;
    uint8_t recved;
    uint8_t pcba_shipping_mode_pinmux;
    uint8_t qdec_y_pha_pinmux;
    uint8_t qdec_y_phb_pinmux;
    uint8_t buck_enable_pinmux;

    //dsp_option1
    uint8_t enable_dsp_capture_data_by_spp : 1;
    uint8_t dsp_log_pin : 6;
    uint8_t dsp_log_output_select : 2;

    uint8_t dsp_gpio_num;
    uint8_t dsp_gpio_pinmux[7];
    uint8_t dsp_jtag_enable : 1;
    uint8_t dsp2_jtag_enable : 1;
    uint8_t dsp2_log_output_select : 2;
    uint8_t adsp_jtag_enable : 1;
    uint8_t dsp_rsv0 : 3;

    uint8_t dsp_jtck_pinmux;
    uint8_t dsp_jtdi_pinmux;
    uint8_t dsp_jtdo_pinmux;
    uint8_t dsp_jtms_pinmux;
    uint8_t dsp_jtrst_pinmux;

    uint8_t dsp2_gpio_num;
    uint8_t dsp2_gpio_pinmux[7];
    uint8_t dsp2_jtck_pinmux;
    uint8_t dsp2_jtdi_pinmux;
    uint8_t dsp2_jtdo_pinmux;
    uint8_t dsp2_jtms_pinmux;
    uint8_t dsp2_jtrst_pinmux;

    uint8_t adsp_gpio_num;
    uint8_t adsp_gpio_pinmux[7];
    uint8_t adsp_jtck_pinmux;
    uint8_t adsp_jtdi_pinmux;
    uint8_t adsp_jtdo_pinmux;
    uint8_t adsp_jtms_pinmux;
    uint8_t adsp_jtrst_pinmux;

} T_APP_CFG_CONST;


/** End of APP_CFG_Exported_Types
    * @}
    */

extern T_APP_CFG_CONST app_cfg_const;


/**
    * @brief  load config parameters from ftl
    * @param  void
    * @return void
    */
void app_mcu_cfg_const_load(void);

/**
    * @brief  config module init
    * @param  void
    * @return void
    */
void app_mcu_cfg_init(void);

/** @} */ /* End of group APP_CFG_Exported_Functions */

/** End of APP_CFG
* @}
*/
#if F_APP_SD_CARD_SUPPORT
void app_sd_card_init(void);
#endif

#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif /*_AU_CFG_H_*/
