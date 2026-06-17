/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "bt_a2dp.h"
#include "app_a2dp_cfg.h"
#include "app_bt_policy_cfg.h"

T_APP_A2DP_CFG app_a2dp_cfg;

void app_a2dp_cfg_init(void)
{
    if (app_bt_policy_cfg_nv.enable_multi_link)
    {
        app_a2dp_cfg.a2dp_link_number = 2;
    }
    else
    {
        app_a2dp_cfg.a2dp_link_number = 1;
    }
    app_a2dp_cfg.a2dp_codec_type_sbc = true;

#if F_APP_A2DP_SOURCE_SUPPORT
    app_a2dp_cfg.a2dp_codec_type_aac = false;
#else
    app_a2dp_cfg.a2dp_codec_type_aac = true;
#endif
    app_a2dp_cfg.a2dp_codec_type_ldac = false;

    //SBC settings
    app_a2dp_cfg.sbc_sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ |
                                          BT_A2DP_SBC_SAMPLING_FREQUENCY_44_1KHZ |
                                          BT_A2DP_SBC_SAMPLING_FREQUENCY_32KHZ | BT_A2DP_SBC_SAMPLING_FREQUENCY_16KHZ;
#if F_APP_A2DP_SOURCE_SUPPORT
    if (app_bt_policy_cfg.enable_multi_sink_devices)
    {
        app_a2dp_cfg.sbc_sampling_frequency = BT_A2DP_SBC_SAMPLING_FREQUENCY_48KHZ;
    }
    else
    {

#if F_APP_INTEGRATED_TRANSCEIVER
        app_a2dp_cfg.sbc_sampling_frequency  &= BT_A2DP_SBC_SAMPLING_FREQUENCY_44_1KHZ;
#endif

    }

#endif

    if (app_bt_policy_cfg.enable_multi_sink_devices)
    {
        app_a2dp_cfg.sbc_channel_mode = BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO;

        app_a2dp_cfg.sbc_block_length = BT_A2DP_SBC_BLOCK_LENGTH_16;

        app_a2dp_cfg.sbc_subbands = BT_A2DP_SBC_SUBBANDS_8;

        app_a2dp_cfg.sbc_allocation_method = BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS;
    }
    else
    {
        app_a2dp_cfg.sbc_channel_mode = BT_A2DP_SBC_CHANNEL_MODE_MONO |
                                        BT_A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL |
                                        BT_A2DP_SBC_CHANNEL_MODE_STEREO | BT_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO;

        app_a2dp_cfg.sbc_block_length = BT_A2DP_SBC_BLOCK_LENGTH_4 | BT_A2DP_SBC_BLOCK_LENGTH_8 |
                                        BT_A2DP_SBC_BLOCK_LENGTH_12 | BT_A2DP_SBC_BLOCK_LENGTH_16;

        app_a2dp_cfg.sbc_subbands = BT_A2DP_SBC_SUBBANDS_4 | BT_A2DP_SBC_SUBBANDS_8;

        app_a2dp_cfg.sbc_allocation_method = BT_A2DP_SBC_ALLOCATION_METHOD_SNR |
                                             BT_A2DP_SBC_ALLOCATION_METHOD_LOUDNESS;
    }

#if F_APP_INTEGRATED_TRANSCEIVER
    app_a2dp_cfg.sbc_min_bitpool = 2;
    app_a2dp_cfg.sbc_max_bitpool = 37;
#else
    app_a2dp_cfg.sbc_min_bitpool = 2;
    app_a2dp_cfg.sbc_max_bitpool = 53;
#endif
    //AAC settings
    app_a2dp_cfg.aac_sampling_frequency = BT_A2DP_AAC_SAMPLING_FREQUENCY_44_1KHZ |
                                          BT_A2DP_AAC_SAMPLING_FREQUENCY_48KHZ;

    app_a2dp_cfg.aac_object_type = BT_A2DP_AAC_OBJECT_TYPE_MPEG_2_AAC_LC;
    app_a2dp_cfg.aac_channel_number = BT_A2DP_AAC_CHANNEL_NUMBER_1 | BT_A2DP_AAC_CHANNEL_NUMBER_2;
    app_a2dp_cfg.aac_vbr_supported = 1;
    app_a2dp_cfg.aac_bit_rate = 256000;

    //LC3 settings
    app_a2dp_cfg.lc3_sampling_frequency = BT_A2DP_LC3_SAMPLING_FREQUENCY_48KHZ |
                                          BT_A2DP_LC3_SAMPLING_FREQUENCY_44_1KHZ;
    app_a2dp_cfg.lc3_channel_num = BT_A2DP_LC3_CHANNEL_NUM_2 |
                                   BT_A2DP_LC3_CHANNEL_NUM_1;
    app_a2dp_cfg.lc3_frame_duration = BT_A2DP_LC3_FRAME_DURATION_10MS |
                                      BT_A2DP_LC3_FRAME_DURATION_7_5MS;
    app_a2dp_cfg.lc3_frame_length = 240;
}


