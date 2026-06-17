/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_AUDIO_SUPPORT
#include <stdlib.h>
#include "os_sync.h"
#include "trace.h"
#include "audio.h"
#include "usb_audio.h"
#include "usb_audio_stream.h"
#include "usb_msg.h"
#include "app_usb_audio.h"
#include "app_main.h"
#include "section.h"
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_uac.h"
#endif
#include "app_src_play.h"

typedef enum
{
    USB_AUDIO_UAL_STATE_IDLE,
    USB_AUDIO_UAL_STATE_INITED,
    USB_AUDIO_UAL_STATE_STARTING,
    USB_AUDIO_UAL_STATE_STARTED,
    USB_AUDIO_UAL_STATE_STOPPING,
    USB_AUDIO_UAL_STATE_STOPED,
    USB_AUDIO_UAL_STATE_RUNNING,
} T_USB_AUDIO_UAL_STATE;

typedef struct _usb_audio
{
    uint32_t id;
    T_USB_AUDIO_UAL_STATE state;
//    void *handle;
} T_USB_AUDIO;

typedef struct app_usb_audio_db
{
    T_USB_AUDIO *playback;
    T_USB_AUDIO *capture;
} T_APP_USB_AUDIO_DB;

T_APP_USB_AUDIO_DB app_usb_audio_db;

const uint32_t sample_freq[8] = {8000, 16000, 32000, 44100, 48000, 88000, 96000, 192000};

#define UAVOL2DSPVOL(vol, range) (0x10*(vol)/range)
#define BUF_UPPER_THR_MS(sr, bw, ch_num) ((PLAYBACK_POOL_SIZE)*1000* 8 /sr/bw/ch_num * 80/100) /*80 percent of playback pool total size*/
#define BUF_LOWER_THR_MS(sr, bw, ch_num)  ((PLAYBACK_POOL_SIZE)*1000* 8/sr/bw/ch_num * 10/100) /*10 percent of playback pool total size*/
#define PKT_NUM_IN_UT(pkt_size) (0)
#define PKT_NUM_IN_LT(pkt_size) ((PLAYBACK_POOL_SIZE)*60/100/pkt_size)

#define UAC_DAC_MAX_LEVEL      100
#define UAC_ADC_MAX_LEVEL      100
static bool is_ios_system = false;
static const uint16_t usb_audio_avrcp_gain_table[] =
{
    0x0000, 0x0001, 0x0002, 0x0003, 0x0005, 0x0006, 0x0007, 0x0008, 0x000A, 0x000B,
    0x000C, 0x000D, 0x000F, 0x0010, 0x0011, 0x0012, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0019, 0x001A, 0x001B, 0x001C, 0x001E, 0x001F, 0x0020, 0x0021, 0x0023, 0x0024,
    0x0025, 0x0026, 0x0028, 0x0029, 0x002A, 0x002B, 0x002D, 0x002E, 0x002F, 0x0031,
    0x0032, 0x0033, 0x0034, 0x0036, 0x0037, 0x0038, 0x0039, 0x003B, 0x003C, 0x003D,
    0x003E, 0x0040, 0x0041, 0x0042, 0x0044, 0x0045, 0x0046, 0x0047, 0x0049, 0x004A,
    0x004B, 0x004C, 0x004E, 0x004F, 0x0050, 0x0052, 0x0053, 0x0054, 0x0055, 0x0057,
    0x0058, 0x0059, 0x005B, 0x005C, 0x005D, 0x005E, 0x0060, 0x0061, 0x0062, 0x0063,
    0x0065, 0x0066, 0x0067, 0x0069, 0x006A, 0x006B, 0x006C, 0x006E, 0x006F, 0x0070,
    0x0072, 0x0073, 0x0074, 0x0075, 0x0077, 0x0078, 0x0079, 0x007B, 0x007C, 0x007D,
    0x007F
};
static const uint16_t usb_audio_dac_dig_gain_table[] =
{
    0x0000, 0x0e61, 0x15a2, 0x1a86, 0x1e38, 0x2130, 0x23ac, 0x25ce, 0x27ad, 0x2957,
    0x2ad7, 0x2c35, 0x2d76, 0x2e9e, 0x2fb2, 0x30b3, 0x31a4, 0x3287, 0x335e, 0x3429,
    0x34eb, 0x35a3, 0x3653, 0x36fb, 0x379c, 0x3837, 0x38cc, 0x395b, 0x39e6, 0x3a6b,
    0x3aec, 0x3b69, 0x3be2, 0x3c58, 0x3cca, 0x3d39, 0x3da4, 0x3e0d, 0x3e73, 0x3ed7,
    0x3f38, 0x3f96, 0x3ff3, 0x404d, 0x40a5, 0x40fb, 0x4150, 0x41a2, 0x41f3, 0x4242,
    0x4290, 0x42dc, 0x4326, 0x4370, 0x43b8, 0x43fe, 0x4444, 0x4488, 0x44cb, 0x450c,
    0x454d, 0x458d, 0x45cb, 0x4609, 0x4646, 0x4681, 0x46bc, 0x46f6, 0x472f, 0x4768,
    0x479f, 0x47d6, 0x480c, 0x4841, 0x4875, 0x48a9, 0x48dc, 0x490f, 0x4941, 0x4972,
    0x49a2, 0x49d2, 0x4a02, 0x4a30, 0x4a5f, 0x4a8c, 0x4aba, 0x4ae6, 0x4b12, 0x4b3e,
    0x4b69, 0x4b94, 0x4bbe, 0x4be8, 0x4c11, 0x4c3a, 0x4c63, 0x4c8b, 0x4cb2, 0x4cda,
    0x4d00
};
static const int16_t usb_audio_mic_gain_table[] =
{
    0x0000, 0x1258, 0x0a6a, 0x1fa9, 0x238d, 0x26a6, 0x2938, 0x2b6a, 0x2d56, 0x2f0a,
    0x3093, 0x31f7, 0x333d, 0x346b, 0x3582, 0x3687, 0x377b, 0x3861, 0x393a, 0x3a08,
    0x3acb, 0x3b85, 0x3c37, 0x3ce1, 0x3d83, 0x3e1f, 0x3eb5, 0x3f46, 0x3fd1, 0x4058,
    0x40da, 0x4158, 0x41d2, 0x4248, 0x42bb, 0x432a, 0x4396, 0x4400, 0x4466, 0x44ca,
    0x452c, 0x458b, 0x45e8, 0x4642, 0x469b, 0x46f1, 0x4746, 0x4799, 0x47ea, 0x483a,
    0x4888, 0x48d4, 0x491f, 0x4969, 0x49b1, 0x49f8, 0x4a3d, 0x4a82, 0x4ac5, 0x4b07,
    0x4b48, 0x4b88, 0x4bc6, 0x4c04, 0x4c41, 0x4c7d, 0x4cb8, 0x4cf2, 0x4d2c, 0x4d64,
    0x4d9c, 0x4dd3, 0x4e09, 0x4e3e, 0x4e73, 0x4ea7, 0x4eda, 0x4f0c, 0x4f3e, 0x4f70,
    0x4fa0, 0x4fd0, 0x5000, 0x502f, 0x505d, 0x508b, 0x50b8, 0x50e5, 0x5111, 0x513d,
    0x5168, 0x5193, 0x51bd, 0x51e7, 0x5211, 0x523a, 0x5262, 0x528a, 0x52b2, 0x52da,
    0x5300
};
//--------------------------------------------------------------------------//

static uint16_t app_usb_audio_get_volume_level(const int16_t gain_table[], uint16_t vol_param)
{
    uint16_t gain_value = vol_param;
    uint16_t gain_table_val = gain_table[0];
    uint16_t l_level = 0;
    uint16_t r_level = UAC_DAC_MAX_LEVEL;
    uint16_t mid_level = l_level + r_level / 2;
    uint8_t cnt = 0;
    while (l_level <= r_level)
    {
        cnt++;
        mid_level = (r_level + l_level + 1) / 2;
        gain_table_val = gain_table[mid_level];

        if ((mid_level == l_level) || (mid_level == r_level))
        {
            if (gain_value == gain_table[l_level])
            {
                mid_level = l_level;
            }
            else if (gain_value == gain_table[r_level])
            {
                mid_level = r_level;
            }

            break;
        }
        if (gain_value == gain_table_val)
        {
            break;
        }
        else if (gain_table_val > gain_value)
        {
            r_level = mid_level - 1;
        }
        else
        {
            l_level = mid_level + 1;
        }
    }
    return mid_level;
}
static uint16_t app_usb_audio_spk_vol_transform(uint16_t vol_param)
{
    uint16_t volume;
    uint16_t level = 66;
    if (vol_param == usb_audio_dac_dig_gain_table[UAC_DAC_MAX_LEVEL]) //MAX_VOL
    {
        level = UAC_DAC_MAX_LEVEL;
    }
    else if (vol_param == usb_audio_dac_dig_gain_table[0]) //MIN_VOL
    {
        level = 0;
    }
    else
    {
        if (is_ios_system == false)
        {
            level = app_usb_audio_get_volume_level((const int16_t *)usb_audio_dac_dig_gain_table, vol_param);
        }
    }
    if ((vol_param != usb_audio_dac_dig_gain_table[level]) && (is_ios_system == false))
    {
        APP_PRINT_INFO2("app_usb_audio_spk_vol_transform vol_param 0x%x ,table_vol 0x%x", vol_param,
                        usb_audio_dac_dig_gain_table[level]);
        is_ios_system = true;
    }
    if (is_ios_system == true)
    {
        uint16_t j = usb_audio_dac_dig_gain_table[UAC_DAC_MAX_LEVEL] - usb_audio_dac_dig_gain_table[0];
        uint16_t i = vol_param - usb_audio_dac_dig_gain_table[0];
        level = i * UAC_DAC_MAX_LEVEL / j;
    }
    volume = usb_audio_avrcp_gain_table[level];
    APP_PRINT_INFO4("app_usb_audio_spk_vol_transform, vol 0x%x, level = %d, volume 0x%x is_ios_system %d",
                    vol_param,
                    level, volume, is_ios_system);
    return volume;
}

static uint8_t app_usb_audio_mic_vol_transform(uint16_t vol_param)
{
    uint8_t volume = 80;
    uint16_t level;
    if (vol_param == usb_audio_mic_gain_table[UAC_ADC_MAX_LEVEL]) //MAX_VOL
    {
        level = UAC_ADC_MAX_LEVEL;
    }
    else if (vol_param == usb_audio_mic_gain_table[0]) //MIN_VOL
    {
        level = 0;
    }
    else
    {
        level = app_usb_audio_get_volume_level((const int16_t *)usb_audio_mic_gain_table, vol_param);
        if (level == 0)
        {
            level++;
        }
        if (level == UAC_ADC_MAX_LEVEL)
        {
            level--;
        }
    }
    volume = usb_audio_avrcp_gain_table[level];
    APP_PRINT_INFO3("app_usb_audio_mic_vol_transform, vol 0x%x, level = %d, volume 0x%x", vol_param,
                    level, volume);
    return volume;
}

bool app_usb_audio_active(uint32_t label, T_USB_AUDIO_PIPE_ATTR attr)
{
    APP_PRINT_INFO5("app_usb_audio_active: label %d, dir %d, sample_rate %d, bit_width %d, channel %d",
                    label, attr.dir, attr.content.audio.sample_rate,
                    attr.content.audio.bit_width, attr.content.audio.channels);
#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE && !F_APP_UAC_SDK_SUPPORT)
    app_src_play_handle_usb_active(attr);
    return app_tri_dongle_uac_active(label, attr);
#else
    if (label == USB_AUDIO_STREAM_LABEL_1)
    {
#if F_SOURCE_PLAY_SUPPORT
        app_src_play_handle_usb_active(attr);
#endif
    }

    return true;
#endif
}

bool app_usb_audio_deactive(uint32_t label, uint8_t dir)
{
    APP_PRINT_INFO2("app_usb_audio_deactive: label %d, dir %d", label, dir);
#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE && !F_APP_UAC_SDK_SUPPORT)
    return app_tri_dongle_uac_deactive(label, dir);
#else
    if (label == USB_AUDIO_STREAM_LABEL_1)
    {
#if F_SOURCE_PLAY_SUPPORT
        app_src_play_handle_usb_inactive(dir);
#endif
    }

    return true;
#endif
}

RAM_TEXT_SECTION
bool app_usb_audio_data_xmit_out(uint8_t *data, uint16_t length, uint32_t label)
{
    bool ret = false;

    if (label == USB_AUDIO_STREAM_LABEL_1)
    {
#if F_SOURCE_PLAY_SUPPORT
        extern bool app_usb_audio_downstream_fill(uint8_t *data, uint16_t len);
        app_usb_audio_downstream_fill(data, length);
#endif
        usb_audio_stream_data_trans_msg(label);
    }

    return ret;
}

bool app_usb_audio_data_xmit_out_handle(uint32_t label)
{
#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE && !F_APP_UAC_SDK_SUPPORT)
    return app_tri_dongle_uac_data_xmit_out_handle(label);
#else
    if ((label == USB_AUDIO_STREAM_LABEL_1) || (label == USB_AUDIO_STREAM_LABEL_2))
    {
        // downstream data msg
    }
    return true;
#endif
}

bool app_usb_audio_us_data_write(uint8_t *data, uint16_t length)
{
    if (app_usb_audio_us_get_remaining_pool_size() >= length)
    {
        return (usb_audio_stream_data_write(app_usb_audio_db.capture->id, data, length));
    }
    else
    {
        usb_audio_stream_data_flush(app_usb_audio_db.capture->id, length);
        return (usb_audio_stream_data_write(app_usb_audio_db.capture->id, data, length));
    }
}

RAM_TEXT_SECTION
uint16_t app_usb_audio_us_get_remaining_pool_size(void)
{
    return (usb_audio_stream_get_remaining_pool_size(app_usb_audio_db.capture->id));
}

RAM_TEXT_SECTION
uint16_t app_usb_audio_us_get_data_len(void)
{
    return (usb_audio_stream_get_data_len(app_usb_audio_db.capture->id));
}

void app_usb_audio_us_clear_up(void)
{
    usb_audio_stream_buf_clear(app_usb_audio_db.capture->id);
}

RAM_TEXT_SECTION
void app_usb_audio_us_size_change(uint16_t data_space, uint16_t free_space,
                                  uint16_t byte_out)
{

}

RAM_TEXT_SECTION
bool app_usb_audio_us_data_empty(void)
{
#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE && !F_APP_UAC_SDK_SUPPORT)
    app_tri_dongle_uac_msg_post(UAC_UP_EMPTY);
#endif
    return true;
}

bool app_usb_audio_set_spk_vol(uint32_t label, uint16_t vol)
{
#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE && !F_APP_UAC_SDK_SUPPORT)
    uint16_t volume = app_usb_audio_spk_vol_transform(vol);
    return app_tri_dongle_uac_set_spk_vol(label, volume);
#else
    if (label == USB_AUDIO_STREAM_LABEL_1)
    {

    }

    return true;
#endif
}

bool app_usb_audio_set_spk_mute(uint32_t label, bool is_mute)
{
#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE && !F_APP_UAC_SDK_SUPPORT)
    return app_tri_dongle_uac_set_spk_mute(is_mute);
#else
    if (label == USB_AUDIO_STREAM_LABEL_1)
    {

    }

    return true;
#endif
}

bool app_usb_audio_set_mic_vol(uint32_t label, uint16_t vol)
{
#if (CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE && !F_APP_UAC_SDK_SUPPORT)
    uint8_t volume = app_usb_audio_mic_vol_transform(vol);
    return app_tri_dongle_uac_set_mic_vol(label, volume);
#else
    if (label == USB_AUDIO_STREAM_LABEL_1)
    {

    }

    return true;
#endif
}

bool app_usb_audio_set_mic_mute(uint32_t label, bool is_mute)
{
    if (label == USB_AUDIO_STREAM_LABEL_1)
    {

    }

    return true;
}

void app_usb_audio_init(void)
{
    app_usb_audio_db.playback = malloc(sizeof(T_USB_AUDIO));
    app_usb_audio_db.playback->id = usb_audio_stream_ual_bind(USB_AUDIO_STREAM_TYPE_OUT,
                                                              USB_AUDIO_STREAM_LABEL_1);

    app_usb_audio_db.capture = malloc(sizeof(T_USB_AUDIO));
    app_usb_audio_db.capture->id = usb_audio_stream_ual_bind(USB_AUDIO_STREAM_TYPE_IN,
                                                             USB_AUDIO_STREAM_LABEL_1);
}
#endif
