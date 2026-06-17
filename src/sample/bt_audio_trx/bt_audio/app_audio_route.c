/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include "os_mem.h"
#include "trace.h"
#include "app_dsp_cfg.h"
#include "app_audio_route.h"

#if F_APP_CUSTOMER_VOICE_LEVEL_SUPPORT
//-72*128,-64*128,-63*128,-63*128,-62*128,-61*128,-61*128,-60*128,-59*128,-59*128,
//-58*128,-57*128,-57*128,-56*128,-55*128,-55*128,-54*128,-53*128,-53*128,-52*128,
//-52*128,-51*128,-50*128,-50*128,-49*128,-48*128,-48*128,-47*128,-46*128,-46*128,
//-45*128,-44*128,-43*128,-42*128,-42*128,-41*128,-40*128,-40*128,-39*128,-39*128,
//-38*128,-37*128,-37*128,-36*128,-35*128,-35*128,-34*128,-33*128,-33*128,-32*128,
//-31*128,-31*128,-30*128,-29*128,-29*128,-28*128,-27*128,-27*128,-26*128,-25*128,
//-25*128,-24*128,-24*128,-23*128,-22*128,-22*128,-21*128,-20*128,-20*128,-19*128,
//-18*128,-18*128,-17*128,-16*128,-16*128,-15*128,-14*128,-14*128,-13*128,-12*128,
//-12*128,-11*128,-11*128,-10*128,-9*128,-9*128,-8*128,-7*128,-7*128,-6*128,
//-5*128,-5*128,-4*128,-3*128,-3*128,-2*128,-1*128,-1*128,-0*128,-0*128,
static uint16_t audio_dac_gain_table_100[100] =
{
    0xDC00, 0xE000, 0xE080, 0xE080, 0xE100, 0xE180, 0xE180, 0xE200, 0xE280, 0xE280,
    0xE300, 0xE380, 0xE380, 0xE400, 0xE480, 0xE480, 0xE500, 0xE580, 0xE580, 0xE600,
    0xE600, 0xE680, 0xE700, 0xE700, 0xE780, 0xE800, 0xE800, 0xE880, 0xE900, 0xE900,
    0xE980, 0xEA00, 0xEA80, 0xEB00, 0xEB00, 0xEB80, 0xEC00, 0xEC00, 0xEC80, 0xEC80,
    0xED00, 0xED80, 0xED80, 0xEE00, 0xEE80, 0xEE80, 0xEF00, 0xEF80, 0xEF80, 0xF000,
    0xF080, 0xF080, 0xF100, 0xF180, 0xF180, 0xF200, 0xF280, 0xF280, 0xF300, 0xF380,
    0xF380, 0xF400, 0xF400, 0xF480, 0xF500, 0xF500, 0xF580, 0xF600, 0xF600, 0xF680,
    0xF700, 0xF700, 0xF780, 0xF800, 0xF800, 0xF880, 0xF900, 0xF900, 0xF980, 0xFA00,
    0xFA00, 0xFA80, 0xFA80, 0xFB00, 0xFB80, 0xFB80, 0xFC00, 0xFC80, 0xFC80, 0xFD00,
    0xFD80, 0xFD80, 0xFE00, 0xFE80, 0xFE80, 0xFF00, 0xFF80, 0xFF80, 0x0000, 0x0000
};

static uint16_t voice_dac_gain_table_100[100] =
{
    0xDC00, 0xE000, 0xE080, 0xE080, 0xE100, 0xE180, 0xE180, 0xE200, 0xE280, 0xE280,
    0xE300, 0xE380, 0xE380, 0xE400, 0xE480, 0xE480, 0xE500, 0xE580, 0xE580, 0xE600,
    0xE600, 0xE680, 0xE700, 0xE700, 0xE780, 0xE800, 0xE800, 0xE880, 0xE900, 0xE900,
    0xE980, 0xEA00, 0xEA80, 0xEB00, 0xEB00, 0xEB80, 0xEC00, 0xEC00, 0xEC80, 0xEC80,
    0xED00, 0xED80, 0xED80, 0xEE00, 0xEE80, 0xEE80, 0xEF00, 0xEF80, 0xEF80, 0xF000,
    0xF080, 0xF080, 0xF100, 0xF180, 0xF180, 0xF200, 0xF280, 0xF280, 0xF300, 0xF380,
    0xF380, 0xF400, 0xF400, 0xF480, 0xF500, 0xF500, 0xF580, 0xF600, 0xF600, 0xF680,
    0xF700, 0xF700, 0xF780, 0xF800, 0xF800, 0xF880, 0xF900, 0xF900, 0xF980, 0xFA00,
    0xFA00, 0xFA80, 0xFA80, 0xFB00, 0xFB80, 0xFB80, 0xFC00, 0xFC80, 0xFC80, 0xFD00,
    0xFD80, 0xFD80, 0xFE00, 0xFE80, 0xFE80, 0xFF00, 0xFF80, 0xFF80, 0x0000, 0x0000
};
#endif

bool app_audio_route_physical_mic_get(T_AUDIO_CATEGORY category,
                                      T_AUDIO_ROUTE_LOGIC_IO_TYPE logical_mic,
                                      T_AUDIO_ROUTE_PHYSICAL_MIC *physical_mic)
{
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path;
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP physical_path_group;
    uint8_t physical_path_num;

    physical_path_group = audio_route_physical_path_take((T_AUDIO_CATEGORY)category);
    physical_path_num = physical_path_group.physical_path_num;
    physical_path = physical_path_group.physical_path;

    if (physical_path == NULL)
    {
        return false;
    }

    for (uint8_t i = 0; i < physical_path_num; i++)
    {
        if (physical_path->logic_io_type == logical_mic)
        {
            *physical_mic = physical_path->attr.mic_attr;
            break;
        }

        physical_path++;
    }

    return true;
}

bool app_audio_route_physical_mic_set(T_AUDIO_CATEGORY category,
                                      T_AUDIO_ROUTE_LOGIC_IO_TYPE logical_mic,
                                      T_AUDIO_ROUTE_PHYSICAL_MIC physical_mic)
{
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path, *physical_path_ptr;
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP physical_path_group;
    uint8_t physical_path_num;

    audio_route_category_path_unregister(category);

    physical_path_group = audio_route_physical_path_take(category);
    physical_path_num = physical_path_group.physical_path_num;
    physical_path = calloc(physical_path_num, sizeof(T_AUDIO_ROUTE_PHYSICAL_PATH));

    if (physical_path == NULL)
    {
        return false;
    }

    memcpy(physical_path, physical_path_group.physical_path,
           sizeof(T_AUDIO_ROUTE_PHYSICAL_PATH) * physical_path_num);
    audio_route_physical_path_give(&physical_path_group);

    physical_path_ptr = physical_path;

    for (uint8_t i = 0; i < physical_path_num; i++)
    {
        if (physical_path_ptr->logic_io_type == logical_mic)
        {
            physical_path_ptr->attr.mic_attr = physical_mic;
            break;
        }

        physical_path_ptr++;
    }

    audio_route_category_path_register(category, physical_path, physical_path_num);
    free(physical_path);

    return true;
}

bool app_audio_route_dac_gain_set(T_AUDIO_CATEGORY category, uint8_t level, uint16_t gain)
{
    bool ret = false;

    switch (category)
    {
#if F_APP_CUSTOMER_VOICE_LEVEL_SUPPORT
    case AUDIO_CATEGORY_AUDIO:
        if (level <= app_dsp_cfg_vol.playback_volume_max)
        {
            audio_dac_gain_table_100[level] = gain;
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_VOICE:
        if (level <= app_dsp_cfg_vol.voice_out_volume_max)
        {
            voice_dac_gain_table_100[level] = gain;
            ret = true;
        }
        break;
#else
    case AUDIO_CATEGORY_AUDIO:
        if (level <= app_dsp_cfg_vol.playback_volume_max)
        {
            app_dsp_cfg_data->audio_dac_gain_table[level] = gain;
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_VOICE:
        if (level <= app_dsp_cfg_vol.voice_out_volume_max)
        {
            app_dsp_cfg_data->voice_dac_gain_table[level] = gain;
            ret = true;
        }
        break;
#endif

    case AUDIO_CATEGORY_ANALOG:
        if (level <= app_dsp_cfg_vol.line_in_volume_out_max)
        {
            app_dsp_cfg_data->aux_dac_gain_table[level] = gain;
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_TONE:
        if (level <= app_dsp_cfg_vol.ringtone_volume_max)
        {
            app_dsp_cfg_data->ringtone_dac_gain_table[level] = gain;
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_VP:
        if (level <= app_dsp_cfg_vol.voice_prompt_volume_max)
        {
            app_dsp_cfg_data->vp_dac_gain_table[level] = gain;
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_APT:
        {

        }
        break;

    case AUDIO_CATEGORY_LLAPT:
        {

        }
        break;

    default:
        break;
    }

    return ret;
}

bool app_audio_route_adc_gain_set(T_AUDIO_CATEGORY category, uint8_t level, uint16_t gain)
{
    bool ret = false;

    switch (category)
    {
    case AUDIO_CATEGORY_VOICE:
        if (level <= app_dsp_cfg_vol.voice_volume_in_max)
        {
            app_dsp_cfg_data->voice_adc_gain_table[level] = gain;
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_RECORD:
        if (level <= app_dsp_cfg_vol.record_volume_max)
        {
            app_dsp_cfg_data->record_adc_gain_table[level] = gain;
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_ANALOG:
        if (level <= app_dsp_cfg_vol.line_in_volume_in_max)
        {
            app_dsp_cfg_data->aux_adc_gain_table[level] = gain;
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_APT:
        {

        }
        break;

    case AUDIO_CATEGORY_LLAPT:
        {

        }
        break;

    case AUDIO_CATEGORY_VAD:
        {

        }
        break;

    default:
        break;
    }

    return ret;
}

static bool app_audio_route_dac_gain_get_cback(T_AUDIO_CATEGORY category, uint32_t level,
                                               T_AUDIO_ROUTE_DAC_GAIN *gain)
{
    bool ret = false;

    switch (category)
    {
#if F_APP_CUSTOMER_VOICE_LEVEL_SUPPORT
    case AUDIO_CATEGORY_AUDIO:
        if (level <= app_dsp_cfg_vol.playback_volume_max)
        {
            gain->dac_gain = audio_dac_gain_table_100[level];
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_VOICE:
        if (level <= app_dsp_cfg_vol.voice_out_volume_max)
        {
            gain->dac_gain = voice_dac_gain_table_100[level];
            ret = true;
        }
        break;
#else
    case AUDIO_CATEGORY_AUDIO:
        if (level <= app_dsp_cfg_vol.playback_volume_max)
        {
            gain->dac_gain = app_dsp_cfg_data->audio_dac_gain_table[level];
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_VOICE:
        if (level <= app_dsp_cfg_vol.voice_out_volume_max)
        {
            gain->dac_gain = app_dsp_cfg_data->voice_dac_gain_table[level];
            ret = true;
        }
        break;
#endif

    case AUDIO_CATEGORY_ANALOG:
        if (level <= app_dsp_cfg_vol.line_in_volume_out_max)
        {
            gain->dac_gain = app_dsp_cfg_data->aux_dac_gain_table[level];
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_TONE:
        if (level <= app_dsp_cfg_vol.ringtone_volume_max)
        {
            gain->dac_gain = app_dsp_cfg_data->ringtone_dac_gain_table[level];
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_VP:
        if (level <= app_dsp_cfg_vol.voice_prompt_volume_max)
        {
            gain->dac_gain = app_dsp_cfg_data->vp_dac_gain_table[level];
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_APT:
        {

        }
        break;

    case AUDIO_CATEGORY_LLAPT:
        {

        }
        break;

    default:
        break;
    }

    return ret;
}

static bool app_audio_route_adc_gain_get_cback(T_AUDIO_CATEGORY category, uint32_t level,
                                               T_AUDIO_ROUTE_ADC_GAIN *gain)
{
    bool ret = false;

    switch (category)
    {
    case AUDIO_CATEGORY_VOICE:
        if (level <= app_dsp_cfg_vol.voice_volume_in_max)
        {
            gain->adc_gain = app_dsp_cfg_data->voice_adc_gain_table[level];
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_RECORD:
        if (level <= app_dsp_cfg_vol.record_volume_max)
        {
            gain->adc_gain = app_dsp_cfg_data->record_adc_gain_table[level];
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_ANALOG:
        if (level <= app_dsp_cfg_vol.line_in_volume_in_max)
        {
            gain->adc_gain = app_dsp_cfg_data->aux_adc_gain_table[level];
            ret = true;
        }
        break;

    case AUDIO_CATEGORY_APT:
        {

        }
        break;

    case AUDIO_CATEGORY_LLAPT:
        {

        }
        break;

    case AUDIO_CATEGORY_VAD:
        {

        }
        break;

    default:
        break;
    }

    return ret;
}

void app_audio_route_gain_init(void)
{
    uint8_t index;

    for (index = 0; index < AUDIO_CATEGORY_NUMBER; index++)
    {
        audio_route_gain_register((T_AUDIO_CATEGORY)index, app_audio_route_dac_gain_get_cback,
                                  app_audio_route_adc_gain_get_cback);
    }
}
