/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_AUDIO_TONE_CFG_H_
#define _APP_AUDIO_TONE_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_AUDIO App Audio
  * @brief App Audio
  * @{
  */

/* NOTICE: Make sure T_APP_AUDIO_TONE_TYPE is align to app_audio_tone_cfg.tone_xxxx offset */
typedef enum
{
    TONE_POWER_ON,                      //0x00
    TONE_POWER_OFF,                     //0x01
    TONE_PAIRING,                       //0x02
    TONE_PAIRING_COMPLETE,              //0x03
    TONE_PAIRING_FAIL,                  //0x04
    TONE_KEY_SHORT_PRESS,               //0x05
    TONE_KEY_LONG_PRESS,                //0x06
    TONE_VOL_CHANGED,                   //0x07
    TONE_KEY_HYBRID,                    //0x08
    TONE_HF_NO_SERVICE,                 //0x09
    TONE_HF_CALL_IN,                    //0x0A
    TONE_FIRST_HF_CALL_IN,              //0x0B
    TONE_LAST_HF_CALL_IN,               //0x0C
    TONE_HF_CALL_ACTIVE,                //0x0D
    TONE_HF_CALL_END,                   //0x0E
    TONE_HF_CALL_REJECT,                //0x0F

    TONE_HF_CALL_VOICE_DIAL,            //0x10
    TONE_HF_CALL_REDIAL,                //0x11
    TONE_IN_EAR_DETECTION,              //0x12
    TONE_GFPS_FINDME,                   //0x13
    TONE_LINK_CONNECTED,                //0x14
    TONE_VOL_MAX,                       //0x15
    TONE_VOL_MIN,                       //0x16
    TONE_ALARM,                         //0x17
    TONE_LINK_LOSS,                     //0x18
    TONE_LINK_DISCONNECT,               //0x19
    TONE_REMOTE_CONNECTED,              //0x1A
    TONE_REMOTE_ROLE_PRIMARY,           //0x1B
    TONE_REMOTE_ROLE_SECONDARY,         //0x1C
    TONE_LIGHT_SENSOR_ON,               //0x1D
    TONE_LIGHT_SENSOR_OFF,              //0x1E
    TONE_EXIT_GAMING_MODE,              //0x1F

    TONE_ENTER_AIRPLANE,                //0x20
    TONE_OTA_OVER_BLE_START,            //0x21
    TONE_LE_PAIR_COMPLETE,              //0x22
    TONE_PAIRING_TIMEOUT,               //0x23
    TONE_MIC_MUTE_ALARM,                //0x24
    TONE_KEY_ULTRA_LONG_PRESS,          //0x25
    TONE_DEV_MULTILINK_ON,              //0x26
    TONE_REMOTE_DISCONNECT,             //0x27
    TONE_FACTORY_RESET,                 //0x28
    TONE_REMOTE_LOSS,                   //0x29
    TONE_MIC_MUTE_ON,                   //0x2A
    TONE_MIC_MUTE_OFF,                  //0x2B
    TONE_LINK_LOSS2,                    //0x2C
    TONE_LINK_DISCONNECT2,              //0x2D
    TONE_BATTERY_PERCENTAGE_10,         //0x2E
    TONE_BATTERY_PERCENTAGE_20,         //0x2F

    TONE_BATTERY_PERCENTAGE_30,         //0x30
    TONE_BATTERY_PERCENTAGE_40,         //0x31
    TONE_BATTERY_PERCENTAGE_50,         //0x32
    TONE_BATTERY_PERCENTAGE_60,         //0x33
    TONE_BATTERY_PERCENTAGE_70,         //0x34
    TONE_BATTERY_PERCENTAGE_80,         //0x35
    TONE_BATTERY_PERCENTAGE_90,         //0x36
    TONE_BATTERY_PERCENTAGE_100,        //0x37
    TONE_AUDIO_PLAYING,                 //0x38
    TONE_AUDIO_PAUSED,                  //0x39
    TONE_AUDIO_FORWARD,                 //0x3A
    TONE_AUDIO_BACKWARD,                //0x3B
    TONE_APT_ON,                        //0x3C
    TONE_APT_OFF,                       //0x3D
    TONE_ENTER_GAMING_MODE,             //0x3E
    TONE_OUTPUT_INDICATION_1,           //0x3F

    TONE_SWITCH_VP_LANGUAGE,            //0x40
    TONE_DUT_TEST,                      //0x41
    TONE_HF_TRANSFER_TO_PHONE,          //0x42
    TONE_HF_OUTGOING_CALL,              //0x43
    TONE_AUDIO_EFFECT_0,                //0x44
    TONE_AUDIO_EFFECT_1,                //0x45
    TONE_AUDIO_EFFECT_2,                //0x46
    TONE_SOUND_PRESS_CALIBRATION,       //0x47
    TONE_EXIT_AIRPLANE,                 //0x48
    TONE_ANC_APT_OFF,                   //0x49
    TONE_ANC_SCENARIO_1,                //0x4A
    TONE_ANC_SCENARIO_2,                //0x4B
    TONE_ANC_SCENARIO_3,                //0x4C
    TONE_ANC_SCENARIO_4,                //0x4D
    TONE_ANC_SCENARIO_5,                //0x4E
    TONE_ENTER_DONGLE_MODE,             //0x4F

    TONE_APT_VOL_0,                     //0x50
    TONE_APT_VOL_1,                     //0x51
    TONE_APT_VOL_2,                     //0x52
    TONE_APT_VOLUME_UP,                 //0x53
    TONE_APT_VOLUME_DOWN,               //0x54
    TONE_AUDIO_EQ_0,                    //0x55
    TONE_AUDIO_EQ_1,                    //0x56
    TONE_AUDIO_EQ_2,                    //0x57
    TONE_AUDIO_EQ_3,                    //0x58
    TONE_AUDIO_EQ_4,                    //0x59
    TONE_AUDIO_EQ_5,                    //0x5A
    TONE_AUDIO_EQ_6,                    //0x5B
    TONE_AUDIO_EQ_7,                    //0x5C
    TONE_AUDIO_EQ_8,                    //0x5D
    TONE_AUDIO_EQ_9,                    //0x5E
    TONE_APT_VOL_MAX,                   //0x5F

    TONE_APT_VOL_MIN,                   //0x60
#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    TONE_FINDMY_SOUND,                  //0x61
#else
    TONE_APT_EQ_0,                      //0x61
#endif
    TONE_APT_EQ_1,                      //0x62
    TONE_APT_EQ_2,                      //0x63
    TONE_APT_EQ_3,                      //0x64
    TONE_APT_EQ_4,                      //0x65
    TONE_APT_EQ_5,                      //0x66
    TONE_APT_EQ_6,                      //0x67
    TONE_APT_EQ_7,                      //0x68
    TONE_APT_EQ_8,                      //0x69
    TONE_APT_EQ_9,                      //0x6A
    TONE_LLAPT_SCENARIO_1,              //0x6B
    TONE_LLAPT_SCENARIO_2,              //0x6C
    TONE_LLAPT_SCENARIO_3,              //0x6D
    TONE_LLAPT_SCENARIO_4,              //0x6E
    TONE_LLAPT_SCENARIO_5,              //0x6F

    TONE_AMA_START_SPEECH,              //0x70
    TONE_AMA_STOP_SPEECH,               //0x71
    TONE_AMA_ERROR_INDICATION,          //0x72

    TONE_APT_VOL_3,                     //0x73
    TONE_APT_VOL_4,                     //0x74
    TONE_APT_VOL_5,                     //0x75
    TONE_APT_VOL_6,                     //0x76
    TONE_APT_VOL_7,                     //0x77
    TONE_APT_VOL_8,                     //0x78
#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
    TONE_GFPS_PAIRING,                  //0x79
#else
    TONE_APT_VOL_9,                     //0x79
#endif
    TONE_BIS_START,                     //0x7A
    TONE_BIS_STOP,                      //0x7B

    TONE_CIS_START,                     //0x7C
    TONE_CIS_TIMEOUT,                   //0x7D
    TONE_CIS_SIMU,                      //0x7E
    TONE_BIS_LOSST,                     //0x7F

    // APP config bin offset 0x3B0
    TONE_CIS_LOST,                      //0x80
    TONE_CIS_CONNECTED,                 //0x81

    TONE_GFPS_DULT_FIND,                //0x82
    TONE_TYPE_MAX                       = 0xA0, // Tone1 128bytes + Tone2 32bytes
} T_APP_AUDIO_TONE_TYPE;

/** @brief  Read only configurations for audio tone */
typedef struct
{
    /* NOTICE: Make sure T_APP_AUDIO_TONE_TYPE is align to app_audio_tone_cfg.tone_xxxx offset */
    //Tone1: 128 bytes (offset 0x12c)
    uint8_t tone_power_on;
    uint8_t tone_power_off;
    uint8_t tone_pairing;
    uint8_t tone_pairing_complete;
    uint8_t tone_pairing_fail;
    uint8_t tone_key_short_press;
    uint8_t tone_key_long_press;
    uint8_t tone_vol_changed;
    uint8_t tone_key_hybrid;
    uint8_t tone_hf_no_service;
    uint8_t tone_hf_call_in;
    uint8_t tone_first_hf_call_in;
    uint8_t tone_last_hf_call_in;
    uint8_t tone_hf_call_active;
    uint8_t tone_hf_call_end;
    uint8_t tone_hf_call_reject;
    uint8_t tone_hf_call_voice_dial;
    uint8_t tone_hf_call_redial;
    uint8_t tone_in_ear_detection;
    uint8_t tone_gfps_findme;
    uint8_t tone_link_connected;
    uint8_t tone_vol_max;
    uint8_t tone_vol_min;
    uint8_t tone_alarm; // also used for tone_exit_dongle_mode
    uint8_t tone_link_loss;
    uint8_t tone_link_disconnect;
    uint8_t tone_remote_connected;
    uint8_t tone_remote_role_primary;
    uint8_t tone_remote_role_secondary;
    uint8_t tone_light_sensor_on;
    uint8_t tone_light_sensor_off;
    uint8_t tone_exit_gaming_mode;
    uint8_t tone_enter_airplane;
    uint8_t tone_ota_over_ble_start;
    uint8_t tone_le_pair_complete;
    uint8_t tone_pairing_timeout;
    uint8_t tone_mic_mute_alarm;
    uint8_t tone_key_ultra_long_press;
    uint8_t tone_dev_multilink_on;
    uint8_t tone_remote_disconnect;
    uint8_t tone_factory_reset;
    uint8_t tone_remote_loss;
    uint8_t tone_mic_mute_on;
    uint8_t tone_mic_mute_off;
    uint8_t tone_link_loss2;
    uint8_t tone_link_disconnect2;
    uint8_t tone_battery_percentage_10;
    uint8_t tone_battery_percentage_20;
    uint8_t tone_battery_percentage_30;
    uint8_t tone_battery_percentage_40;
    uint8_t tone_battery_percentage_50;
    uint8_t tone_battery_percentage_60;
    uint8_t tone_battery_percentage_70;
    uint8_t tone_battery_percentage_80;
    uint8_t tone_battery_percentage_90;
    uint8_t tone_battery_percentage_100;
    uint8_t tone_audio_playing;
    uint8_t tone_audio_paused;
    uint8_t tone_audio_forward;
    uint8_t tone_audio_backward;
    uint8_t tone_apt_on;
    uint8_t tone_apt_off;
    uint8_t tone_enter_gaming_mode;
    uint8_t tone_output_indication_1;

    uint8_t tone_switch_vp_language;
    uint8_t tone_dut_test;
    uint8_t tone_hf_transfer_to_phone;
    uint8_t tone_hf_outgoing_call;
    uint8_t tone_audio_effect_0;
    uint8_t tone_audio_effect_1;
    uint8_t tone_audio_effect_2;
    uint8_t tone_sound_press_calibration;

    uint8_t tone_exit_airplane;
    uint8_t tone_anc_apt_off;
    uint8_t tone_anc_scenario_1;
    uint8_t tone_anc_scenario_2;
    uint8_t tone_anc_scenario_3;
    uint8_t tone_anc_scenario_4;
    uint8_t tone_anc_scenario_5;
    uint8_t tone_enter_dongle_mode;

    uint8_t tone_apt_vol_0;
    uint8_t tone_apt_vol_1;
    uint8_t tone_apt_vol_2;
    uint8_t tone_apt_volume_up;
    uint8_t tone_apt_volume_down;
#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    uint8_t tone_find_my;
#else
    uint8_t tone_audio_eq_0;
#endif
    uint8_t tone_audio_eq_1;
    uint8_t tone_audio_eq_2;
    uint8_t tone_audio_eq_3;
    uint8_t tone_audio_eq_4;
    uint8_t tone_audio_eq_5;
    uint8_t tone_audio_eq_6;
    uint8_t tone_audio_eq_7;
    uint8_t tone_audio_eq_8;
    uint8_t tone_audio_eq_9;
    uint8_t tone_apt_vol_max;
    uint8_t tone_apt_vol_min;
    uint8_t tone_apt_eq_0;
    uint8_t tone_apt_eq_1;
    uint8_t tone_apt_eq_2;
    uint8_t tone_apt_eq_3;
    uint8_t tone_apt_eq_4;
    uint8_t tone_apt_eq_5;
    uint8_t tone_apt_eq_6;
    uint8_t tone_apt_eq_7;
    uint8_t tone_apt_eq_8;
    uint8_t tone_apt_eq_9;
    uint8_t tone_llapt_scenario_1;
    uint8_t tone_llapt_scenario_2;
    uint8_t tone_llapt_scenario_3;
    uint8_t tone_llapt_scenario_4;
    uint8_t tone_llapt_scenario_5;
    uint8_t tone_ama_start_speech;
    uint8_t tone_ama_stop_speech;
    uint8_t tone_ama_error_indication;
    uint8_t tone_apt_vol_3;
    uint8_t tone_apt_vol_4;
    uint8_t tone_apt_vol_5;
    uint8_t tone_apt_vol_6;
    uint8_t tone_apt_vol_7;
    uint8_t tone_apt_vol_8;
#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
    uint8_t gfps_pairing_tone;
#else
    uint8_t tone_apt_vol_9;
#endif
    uint8_t tone_bis_start;
    uint8_t tone_bis_stop;
    uint8_t tone_cis_start;
    uint8_t tone_cis_timeout;
    uint8_t tone_cis_simu ;
    uint8_t tone_bis_loss;

    //Tone2: 32 bytes (offset 0x3B0)
    uint8_t cis_loss;
    uint8_t cis_connected;
    uint8_t tone_gfps_dult_find;
    uint8_t tone_reserved[29];
} T_APP_AUDIO_TONE_CFG;

extern const T_APP_AUDIO_TONE_CFG app_audio_tone_cfg;



/** End of APP_AUDIO
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
