/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GAMING_DONGLE_SUPPORT || TARGET_LEGACY_GAMING_DONGLE || TARGET_LE_AUDIO_GAMING_DONGLE
#include <stdlib.h>
#include <string.h>
#include "os_queue.h"
#include "trace.h"
#include "audio_track.h"
#include "app_usb_vol_control.h"
#include "app_timer.h"
#include "app_cfg.h"
#if F_APP_GAMING_DONGLE_SUPPORT
#include "remote.h"
#include "app_main.h"
#include "app_audio_policy.h"
#endif

#define LOCAL_DSP_GAIN_MAX              0       //(db)
#define LOCAL_DSP_GAIN_MIN              -128    //(db)

#define CTRL_LEVEL                      30
#define GAIN_CHANGE_STEP(range)         (range / CTRL_LEVEL)

#define HID_MONITOR_TIMER_MS            (50) //ms

typedef enum
{
    APP_TIMER_USB_VOL_CTRL,
} T_USB_VOL_CTRL_TIMER;

typedef enum
{
    USB_VOL_CTRL_UP,
    USB_VOL_CTRL_DOWN,
    USB_VOL_CTRL_RELEASE,
} T_USB_VOL_CTRL_ACTION;

typedef struct usb_vol_ctrl_elem
{
    struct usb_vol_ctrl_elem *next;
    T_USB_VOL_CTRL_ACTION action;
} T_USB_VOL_CTRL_ELEM;

static T_OS_QUEUE usb_vol_ctrl_queue;
static uint8_t usb_vol_ctrl_timer_id = 0;
static uint8_t timer_idx_usb_vol_ctrl = 0;

static P_HOST_TYPE_SYNC_CB host_type_sync_cb;
T_USB_HOST_DETECT_HID_INFO hid_info_fun;

T_USB_SPK_INFO usb_spk_vol;

static bool app_usb_vol_action_enqueue(T_USB_VOL_CTRL_ACTION action)
{
    T_USB_VOL_CTRL_ELEM *vol_elem = calloc(1, sizeof(T_USB_VOL_CTRL_ELEM));
    bool ret = false;

    if (vol_elem != NULL)
    {
        vol_elem->action = action;

        os_queue_in(&usb_vol_ctrl_queue, vol_elem);

        ret = true;
    }

    return ret;
}

static void app_usb_vol_action_dequeue(void)
{
    if (usb_vol_ctrl_queue.count > 0)
    {
        T_USB_VOL_CTRL_ELEM *vol_elem = os_queue_out(&usb_vol_ctrl_queue);

        if (vol_elem != NULL)
        {
            switch (vol_elem->action)
            {
            case USB_VOL_CTRL_UP:
                {
                    hid_info_fun.volume_up();
                }
                break;

            case USB_VOL_CTRL_DOWN:
                {
                    hid_info_fun.volume_down();
                }
                break;

            case USB_VOL_CTRL_RELEASE:
                {
                    hid_info_fun.volume_release();
                }
                break;

            default:
                break;
            }

            free(vol_elem);
        }
    }
}

static void app_usb_vol_timer_cback(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_USB_VOL_CTRL:
        {
            app_usb_vol_action_dequeue();

            if (usb_vol_ctrl_queue.count == 0)
            {
                app_stop_timer(&timer_idx_usb_vol_ctrl);
            }
        }
        break;

    default:
        break;
    }
}

#if F_APP_GAMING_DONGLE_SUPPORT
void app_usb_play_vol_max_min_tone(T_APP_VOL_CTRL type)
{
    APP_PRINT_TRACE2("app_usb_play_vol_max_min_tone: max_or_min %d type %d",
                     usb_spk_vol.is_max_min_vol, usb_spk_vol.host_type);
    if (usb_spk_vol.host_type < OS_TYPE_ANDROID)
    {
        if ((usb_spk_vol.uac_spk_vol_gain == LOCAL_DSP_GAIN_MAX)
            && usb_spk_vol.is_max_min_vol && (type == SPK_VOL_UP))
        {
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                app_audio_tone_type_play(TONE_VOL_MAX, false, false);
            }
            else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_audio_tone_type_play(TONE_VOL_MAX, false, true);
            }
        }
        else if ((usb_spk_vol.uac_spk_vol_gain < LOCAL_DSP_GAIN_MAX)
                 && usb_spk_vol.is_max_min_vol && (type == SPK_VOL_DOWN))
        {
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                app_audio_tone_type_play(TONE_VOL_MIN, false, false);
            }
            else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_audio_tone_type_play(TONE_VOL_MIN, false, true);
            }
        }
    }
    else
    {
        if ((usb_spk_vol.uac_spk_vol_gain == LOCAL_DSP_GAIN_MAX) && (type == SPK_VOL_UP))
        {
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                app_audio_tone_type_play(TONE_VOL_MAX, false, false);
            }
            else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_audio_tone_type_play(TONE_VOL_MAX, false, true);
            }
        }
        else if ((usb_spk_vol.uac_spk_vol_gain == LOCAL_DSP_GAIN_MIN) && (type == SPK_VOL_DOWN))
        {
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                app_audio_tone_type_play(TONE_VOL_MIN, false, false);
            }
            else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_audio_tone_type_play(TONE_VOL_MIN, false, true);
            }
        }
    }
}

bool app_usb_volume_db_set(void *handle, int16_t gain, T_APP_CTRL_DIR dir)
{
    if (handle == NULL)
    {
        return false;
    }

    APP_PRINT_TRACE3("app_usb_volume_db_set: gain %d dir %d type %d", gain, dir, usb_spk_vol.host_type);

    if (usb_spk_vol.host_type == OS_TYPE_PS)
    {
        if (dir == CTRL_FROM_HOST)
        {
            if (app_cfg_nv.ps_vol_gain != VOL_GAIN_RESET)
            {
                gain = LOCAL_DSP_GAIN_MAX - app_cfg_nv.ps_vol_gain;
            }
        }
        else
        {
            app_cfg_nv.ps_vol_gain = LOCAL_DSP_GAIN_MAX - gain;
        }
    }

#if F_APP_ANDROID_VOL_SPECIAL_HANDLE
    if ((usb_spk_vol.host_type == OS_TYPE_ANDROID) ||
        (usb_spk_vol.host_type == OS_TYPE_SWITCH))
    {
        gain = LOCAL_DSP_GAIN_MAX;
    }
#else
    if (dir == CTRL_FROM_HOST)
    {
        if (usb_spk_vol.host_type == OS_TYPE_ANDROID)
        {
            if (app_cfg_nv.android_vol_gain != VOL_GAIN_RESET)
            {
                gain = LOCAL_DSP_GAIN_MAX - app_cfg_nv.android_vol_gain;
            }
        }
        else if (usb_spk_vol.host_type == OS_TYPE_SWITCH)
        {
            if (app_cfg_nv.switch_vol_gain != VOL_GAIN_RESET)
            {
                gain = LOCAL_DSP_GAIN_MAX - app_cfg_nv.switch_vol_gain;
            }
        }
    }
    else
    {
        if (usb_spk_vol.host_type == OS_TYPE_ANDROID)
        {
            app_cfg_nv.android_vol_gain = LOCAL_DSP_GAIN_MAX - gain;
        }
        else if (usb_spk_vol.host_type == OS_TYPE_SWITCH)
        {
            app_cfg_nv.switch_vol_gain = LOCAL_DSP_GAIN_MAX - gain;
        }
    }
#endif

    usb_spk_vol.uac_spk_vol_gain = gain;
    APP_PRINT_TRACE4("app_usb_volume_db_set: set_gain %d, android %d, swich %d, ps %d", gain,
                     app_cfg_nv.android_vol_gain, app_cfg_nv.switch_vol_gain, app_cfg_nv.ps_vol_gain);
    audio_track_out_db_set(handle, gain, gain);

    return true;
}

bool app_usb_audio_volume_control_handle(T_APP_VOL_CTRL type, void *handle)
{
    int16_t range = LOCAL_DSP_GAIN_MAX - LOCAL_DSP_GAIN_MIN;
    int16_t step = GAIN_CHANGE_STEP(range);

    if (type == SPK_VOL_UP)
    {
        /* 1. When teh host is PS or Android or Switch, headset change local gain here.
         * 2. When teh host is Windows or IOS, headset change local gain according host feedback.
         */
        if (usb_spk_vol.host_type >= OS_TYPE_ANDROID)
        {
            if (usb_spk_vol.uac_spk_vol_gain < LOCAL_DSP_GAIN_MAX)
            {
                usb_spk_vol.uac_spk_vol_gain += step;

                if (usb_spk_vol.uac_spk_vol_gain > LOCAL_DSP_GAIN_MAX)
                {
                    usb_spk_vol.uac_spk_vol_gain = LOCAL_DSP_GAIN_MAX;
                }
                app_usb_volume_db_set(handle, usb_spk_vol.uac_spk_vol_gain, CTRL_FROM_HEADSET);
            }
        }
    }
    else if (type == SPK_VOL_DOWN)
    {
        if (usb_spk_vol.host_type >= OS_TYPE_ANDROID)
        {
            if (usb_spk_vol.uac_spk_vol_gain > LOCAL_DSP_GAIN_MIN)
            {
                usb_spk_vol.uac_spk_vol_gain -= step;

                if (usb_spk_vol.uac_spk_vol_gain < LOCAL_DSP_GAIN_MIN)
                {
                    usb_spk_vol.uac_spk_vol_gain = LOCAL_DSP_GAIN_MIN;
                }
                app_usb_volume_db_set(handle, usb_spk_vol.uac_spk_vol_gain, CTRL_FROM_HEADSET);
            }
        }
    }
    APP_PRINT_TRACE3("app_usb_audio_volume_handle type %d cur_gain %d host type %d",
                     type, usb_spk_vol.uac_spk_vol_gain, usb_spk_vol.host_type);

    return true;
}
#endif

void app_usb_audio_volume_hid_ctrl(T_APP_VOL_CTRL type)
{
    APP_PRINT_INFO2("app_usb_audio_volume_hid_ctrl type %d host %d", type, usb_spk_vol.host_type);

    hid_info_fun.pipe_open();

    if (usb_spk_vol.host_type == OS_TYPE_SWITCH)
    {
        bool ret = false;

        if (type == SPK_VOL_UP)
        {
            ret = app_usb_vol_action_enqueue(USB_VOL_CTRL_UP);
        }
        else
        {
            ret = app_usb_vol_action_enqueue(USB_VOL_CTRL_DOWN);
        }

        if (ret)
        {
            app_usb_vol_action_enqueue(USB_VOL_CTRL_RELEASE);
        }

        if (timer_idx_usb_vol_ctrl == 0)
        {
            app_usb_vol_action_dequeue();

            app_start_timer(&timer_idx_usb_vol_ctrl,
                            "usb_vol_ctrl",
                            usb_vol_ctrl_timer_id, APP_TIMER_USB_VOL_CTRL, 0, true,
                            HID_MONITOR_TIMER_MS);
        }
    }
    else if (usb_spk_vol.host_type == OS_TYPE_PS)
    {
        APP_PRINT_INFO0("app_usb_audio_volume_hid_ctrl host is PS, no need send!");
    }
    else
    {
        if (type == SPK_VOL_UP)
        {
            hid_info_fun.volume_up();
        }
        else
        {
            hid_info_fun.volume_down();
        }

        hid_info_fun.volume_release();
    }
}

void app_usb_vol_host_type_sync_register(P_HOST_TYPE_SYNC_CB cb)
{
    host_type_sync_cb = cb;
}

static int app_usb_vol_cotrl_host_type_cback(T_OS_TYPE type)
{
    APP_PRINT_TRACE2("app_usb_vol_cotrl_host_type_cback %d %d", type, usb_spk_vol.host_type);

    if (usb_spk_vol.host_type != type)
    {
        usb_spk_vol.host_type = type;
        if (host_type_sync_cb)
        {
            host_type_sync_cb(type);
        }
    }
    return 0;
}

void app_vol_control_hid_info_register(T_USB_HOST_DETECT_HID_INFO info)
{
    memcpy(&hid_info_fun, &info, sizeof(T_USB_HOST_DETECT_HID_INFO));
}

void app_usb_vol_control_init(void)
{
#if F_APP_USB_HID_SUPPORT && F_APP_USB_AUDIO_SUPPORT
    usb_host_detect_init(app_usb_vol_cotrl_host_type_cback);
#endif

    app_timer_reg_cb(app_usb_vol_timer_cback, &usb_vol_ctrl_timer_id);

    os_queue_init(&usb_vol_ctrl_queue);
}

#endif
