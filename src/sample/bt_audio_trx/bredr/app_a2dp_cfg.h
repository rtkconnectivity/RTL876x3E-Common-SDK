/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_A2DP_CFG_H_
#define _APP_A2DP_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_A2DP App A2dp
  * @brief App A2dp
  * @{
  */
/** @brief  Read only configurations for a2dp */
typedef struct
{
    uint8_t a2dp_link_number;
    uint16_t a2dp_codec_type_sbc : 1;
    uint16_t a2dp_codec_type_aac : 1;
    uint16_t a2dp_codec_type_ldac : 1;
    uint16_t a2dp_codec_type_lc3 : 1;


    //Profile A2DP codec settings
    //SBC settings
    uint8_t sbc_sampling_frequency;
    uint8_t sbc_channel_mode;
    uint8_t sbc_block_length;
    uint8_t sbc_subbands;
    uint8_t sbc_allocation_method;
    uint8_t sbc_min_bitpool;
    uint8_t sbc_max_bitpool;

    //AAC settings
    uint16_t aac_sampling_frequency;
    uint8_t  aac_object_type;
    uint8_t  aac_channel_number;
    uint8_t  aac_vbr_supported;
    uint32_t aac_bit_rate;

    //LDAC settings
    uint8_t ldac_sampling_frequency;
    uint8_t ldac_channel_mode;

    //LC3 settings
    uint16_t lc3_sampling_frequency;
    uint8_t lc3_channel_num;
    uint8_t lc3_frame_duration;
    uint8_t lc3_frame_length;

} T_APP_A2DP_CFG;


extern T_APP_A2DP_CFG app_a2dp_cfg;


/**
    * @brief  A2DP config module init
    * @param  void
    * @return void
    */
void app_a2dp_cfg_init(void);


/** End of APP_A2DP
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
