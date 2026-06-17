/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_INI_CIS_CFG_H_
#define _APP_LEA_INI_CIS_CFG_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "bap_unicast_client.h"

#define LEA_MEDIA_TOPO_CFG_SUPPORT (UNICAST_AUDIO_CFG_4_BIT | UNICAST_AUDIO_CFG_6_I_BIT | \
                                    UNICAST_AUDIO_CFG_6_II_BIT)

#define LEA_CONVS_TOPO_CFG_SUPPORT (UNICAST_AUDIO_CFG_8_I_BIT| UNICAST_AUDIO_CFG_8_II_BIT| \
                                    UNICAST_AUDIO_CFG_5_BIT | UNICAST_AUDIO_CFG_11_II_BIT)

const T_UNICAST_AUDIO_CFG_TYPE lea_headset_media_topo[] =
{
    UNICAST_AUDIO_CFG_6_I,
    UNICAST_AUDIO_CFG_4,
};

const T_UNICAST_AUDIO_CFG_TYPE lea_headset_conv_topo[] =
{
    UNICAST_AUDIO_CFG_8_I,
    UNICAST_AUDIO_CFG_5,
};

const T_UNICAST_AUDIO_CFG_TYPE lea_rws_media_topo[] =
{
    UNICAST_AUDIO_CFG_6_II,
};

const T_UNICAST_AUDIO_CFG_TYPE lea_rws_conv_topo[] =
{
    UNICAST_AUDIO_CFG_11_II,
    UNICAST_AUDIO_CFG_8_II,
};

const T_CODEC_CFG_ITEM lea_media_codec[] =
{
    CODEC_CFG_ITEM_48_6,
    CODEC_CFG_ITEM_48_5,
    CODEC_CFG_ITEM_48_2,
    CODEC_CFG_ITEM_48_1,
};

const T_CODEC_CFG_ITEM lea_media_gaming_codec[] =
{
    CODEC_CFG_ITEM_48_1,
    CODEC_CFG_ITEM_48_3,
};

const T_CODEC_CFG_ITEM lea_conv_codec[] =
{
    CODEC_CFG_ITEM_16_1,
    CODEC_CFG_ITEM_16_2,
};

const T_CODEC_CFG_ITEM lea_nrec_codec[] =
{
    CODEC_CFG_ITEM_16_2,
    CODEC_CFG_ITEM_16_1,
};

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
