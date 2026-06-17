/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_GFPS_COMMON_FINDER_SUPPORT

#include "stdlib.h"
#include "trace.h"
#include "string.h"

#include "ble_ext_adv.h"

#include "audio.h"

#include "app_timer.h"
#include "app_dult_svc.h"
#include "app_dult.h"
#include "app_gfps_cfg.h"
#include "app_audio_policy.h"
#include "app_adv_stop_cause.h"
#include "app_dult_device.h"

/**
 * @brief DULT timer related
 */
static uint8_t           app_dult_timer_id = 0;
static uint8_t           timer_idx_dult_addr_update = 0;
static uint8_t           timer_idx_dult_exit_id_read_state = 0;
static uint8_t           timer_idx_dult_ring_duration = 0;

static bool              dult_id_read_enable = false;
static bool              keep_ring = false;
static T_DULT_RING_STATE dult_ring_state = DULT_RING_STATE_STOPPED;
typedef enum
{
    APP_DULT_DEVICE_TIMER_UPDATE_ADDR,
    APP_DULT_DEVICE_TIMER_EXIT_ID_READ_STATE,
    APP_DULT_DEVICE_TIMER_RING_DURATION,

} T_APP_DULT_DEVICE_TIMER;

#define DULT_TIMER_INTERVAL_24H       60 * 60 * 1000 * 24
#define DULT_TIMER_INTERVAL_15MIN     60 * 15 * 1000
#define DULT_TIMER_INTERVAL_5MIN      60 * 5 *1000
#define DULT_TIMER_RING_DURATION      12 * 1000

// Sound
void app_dult_ring_duration_timer_start(void)
{
    keep_ring = true;
    app_start_timer(&timer_idx_dult_ring_duration, "dult_ring_duration",
                    app_dult_timer_id,
                    APP_DULT_DEVICE_TIMER_RING_DURATION, 0, false,
                    DULT_TIMER_RING_DURATION);

    APP_PRINT_TRACE0("app_dult_ring_duration_timer_start");
}

void app_dult_ring_duration_timer_stop(void)
{
    keep_ring = false;
    app_stop_timer(&timer_idx_dult_ring_duration);
    APP_PRINT_TRACE0("app_dult_ring_duration_timer_stop");
}

void app_dult_device_sound_start(T_SERVER_ID service_id, T_DULT_CALLBACK_DATA *p_callback)
{
    if (app_audio_tone_type_play(app_gfps_cfg_dult_get_tone_type(), false, false))
    {
        dult_ring_state = DULT_RING_STATE_STARTED;
        app_dult_ring_duration_timer_start();
        app_dult_report_cmd_rsp(p_callback->conn_handle, p_callback->cid, service_id,
                                DULT_OPCODE_CTRL_SOUND_START, DULT_CMD_RSP_STATE_SUCCESS);
    }
    else
    {
        app_dult_report_cmd_rsp(p_callback->conn_handle, p_callback->cid, service_id,
                                DULT_OPCODE_CTRL_SOUND_START, DULT_CMD_RSP_STATE_INVALID_STATE);
    }
}

void app_dult_device_sound_stop(T_SERVER_ID service_id, T_DULT_CALLBACK_DATA *p_callback)
{
    app_dult_ring_duration_timer_stop();
    app_audio_tone_type_cancel(app_gfps_cfg_dult_get_tone_type(), false);
    app_dult_sound_complete_handle();
}

static void app_dult_audio_cback(T_AUDIO_EVENT event_type, void *event_buf,
                                 uint16_t buf_len)
{
    bool handle = false;
    T_AUDIO_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case AUDIO_EVENT_RINGTONE_STOPPED:
    case AUDIO_EVENT_VOICE_PROMPT_STOPPED:
        {
            uint8_t tone_stopped_idx;

            if (event_type == AUDIO_EVENT_VOICE_PROMPT_STOPPED)
            {
                tone_stopped_idx = param->voice_prompt_stopped.index + VOICE_PROMPT_INDEX;
            }
            else
            {
                tone_stopped_idx = param->ringtone_stopped.index;
            }

            if (tone_stopped_idx == app_gfps_cfg_get_dult_tone_idx())
            {
                if (keep_ring)
                {
                    app_audio_tone_type_play(app_gfps_cfg_dult_get_tone_type(), false, false);
                }

                APP_PRINT_TRACE1("app_dult_audio_cback: keep_ring %d", keep_ring);
                handle = true;
            }
        }
        break;

    default:
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_dult_audio_cback: event_type 0x%04x", event_type);
    }
}

//identify read state
bool app_dult_id_read_state_get(void)
{
    return dult_id_read_enable;
}

void app_dult_id_read_state_enable(void)
{
    if (!extend_app_cfg_const.gfps_finder_support)
    {
        return;
    }

    APP_PRINT_TRACE0("app_dult_id_read_state_enable");
    dult_id_read_enable = true;
    app_start_timer(&timer_idx_dult_exit_id_read_state, "exit_id_read_state",
                    app_dult_timer_id,
                    APP_DULT_DEVICE_TIMER_EXIT_ID_READ_STATE, 0, false,
                    DULT_TIMER_INTERVAL_5MIN);
}

static void app_dult_timer_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_dult_timer_cb: timer_id 0x%02x, timer_chann %d",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case APP_DULT_DEVICE_TIMER_EXIT_ID_READ_STATE:
        {
            //APP_PRINT_TRACE0("app_dult_timer_cb: id_read_state_disable");
            dult_id_read_enable = false;
        }
        break;

    case APP_DULT_DEVICE_TIMER_RING_DURATION:
        {
            //service_id and p_callback will be called internal
            //APP_PRINT_TRACE0("app_dult_timer_cb: ring duration timeout");
            app_dult_device_sound_stop(0xFF, NULL);
        }

    default:
        break;
    }
}

T_DULT_RING_STATE app_dult_device_get_ring_state(void)
{
    return dult_ring_state;
}

void app_dult_device_set_ring_state(T_DULT_RING_STATE ring_state)
{
    dult_ring_state = ring_state;
}

void app_dult_device_init(void)
{
    audio_mgr_cback_register(app_dult_audio_cback);
    app_timer_reg_cb(app_dult_timer_cb, &app_dult_timer_id);
}

#endif
