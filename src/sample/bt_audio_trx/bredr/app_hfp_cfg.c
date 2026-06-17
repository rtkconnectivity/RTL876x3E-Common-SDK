/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "bt_hfp.h"
#include "bt_hfp_ag.h"
#include "app_hfp_cfg.h"
#include "app_bt_policy_cfg.h"

T_APP_HFP_CFG app_hfp_cfg;

void app_hfp_cfg_init(void)
{
    if (app_bt_policy_cfg_nv.enable_multi_link)
    {
        app_hfp_cfg.hfp_link_number = 2;
    }
    else
    {
        app_hfp_cfg.hfp_link_number = 1;
    }
    app_hfp_cfg.hfp_hf_brsf_capability = BT_HFP_HF_LOCAL_EC_NR_FUNCTION |
                                         BT_HFP_HF_LOCAL_THREE_WAY_CALLING |
                                         BT_HFP_HF_LOCAL_CLI_PRESENTATION_CAPABILITY | BT_HFP_HF_LOCAL_VOICE_RECOGNITION_ACTIVATION |
                                         BT_HFP_HF_LOCAL_REMOTE_VOLUME_CONTROL | BT_HFP_HF_LOCAL_ENHANCED_CALL_STATUS |
                                         BT_HFP_HF_LOCAL_CODEC_NEGOTIATION | BT_HFP_HF_LOCAL_ESCO_S4_SETTINGS;

    app_hfp_cfg.hfp_ag_brsf_capability = BT_HFP_AG_LOCAL_CAPABILITY_3WAY |
                                         BT_HFP_AG_LOCAL_CAPABILITY_EC_NR |
                                         BT_HFP_AG_LOCAL_CAPABILITY_INBAND_RINGING |
                                         BT_HFP_AG_LOCAL_CAPABILITY_CODEC_NEGOTIATION | BT_HFP_AG_LOCAL_CAPABILITY_ENHANCED_CALL_STATUS |
                                         BT_HFP_AG_LOCAL_CAPABILITY_HF_INDICATORS
#if F_APP_USB_HID_PC_TOOL
                                         | BT_HFP_AG_LOCAL_CAPABILITY_VOICE_RECOGNITION
#endif
                                         ;

    app_hfp_cfg.enable_auto_answer_incoming_call = false;
    app_hfp_cfg.enable_last_num_redial_always_by_first_phone = false;
    app_hfp_cfg.enable_last_num_redial_always_by_last_phone = false;
    app_hfp_cfg.always_play_hf_incoming_tone_when_incoming_call = false;

    app_hfp_cfg.fixed_inband_tone_gain = 0;
    app_hfp_cfg.inband_tone_gain_lv = 8;

    app_hfp_cfg.timer_no_service_warning = 0;
    app_hfp_cfg.timer_hfp_auto_answer_call = 0;
    app_hfp_cfg.timer_mic_mute_alarm = 0;
}


