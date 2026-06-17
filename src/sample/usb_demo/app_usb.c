/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_AUDIO_SUPPORT|F_APP_USB_HID_SUPPORT
#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "section.h"
#include "sysm.h"
#include "hal_adp.h"
#include "pm.h"
#include "system_status_api.h"
#include "usb_hid.h"
#include "usb_audio_stream.h"
#include "usb_dev.h"
#include "usb_dm.h"
#include "usb_msg.h"
#include "app_io_msg.h"
#include "app_ipc.h"
#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio.h"
#if F_APP_USB_HID_SUPPORT
#include "app_usb_audio_mmi.h"
#endif
#elif F_APP_USB_HID_SUPPORT
#include "app_usb_hid.h"
#endif

#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
#define     CPU_FREQ_MHZ_MAX    160
#else
#define     CPU_FREQ_MHZ_MAX    100
#endif
typedef enum
{
    USB_EVT_PLUG,
    USB_EVT_UNPLUG,
    USB_EVT_PWR_STATUS_CHG,
} T_USB_EVT;

#define USB_DM_EVT_Q_NUM     0x20

#define USB_DM_EVT_USB_START 0x01
#define USB_DM_EVT_USB_STOP  0x02

#define READY_BIT_INIT       (0x01 << 0)
#define READY_BIT_PLUGGED    (0x01 << 1)
#define READY_BIT_POWER_ON   (0x01 << 2)
#define READY_BIT_ALL        (READY_BIT_POWER_ON|READY_BIT_PLUGGED|READY_BIT_INIT)

typedef struct _store_param
{
    uint32_t pre_cpu_clk;
    POWERMode pre_pwr_mode;
} T_STORE_PARAM;

typedef struct _app_usb_db
{
    T_STORE_PARAM store;
    uint8_t ready_set_to_start;
    T_USB_POWER_STATE usb_pwr_state;
    void *usb_dm_task;
    void *usb_dm_evt_q_handle;
    bool power_on_when_usb_plug;
    bool ds_playing;
} T_APP_USB_DB;

static T_APP_USB_DB app_usb_db;
static uint8_t app_usb_freq_handle = 0;

static void app_usb_start(void)
{
    uint32_t actual_mhz = 0;
    sys_hall_auto_sleep_in_idle(false);
    app_usb_db.store.pre_cpu_clk = pm_cpu_freq_get();
    pm_cpu_freq_req(&app_usb_freq_handle, pm_cpu_max_freq_get(), &actual_mhz);
    pm_dvfs_set_supreme();
    usb_dm_start(false);

    APP_PRINT_INFO2("app_usb_start: pre clk %d, cur clk %d", app_usb_db.store.pre_cpu_clk,  actual_mhz);
    app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_PLUG, NULL);
}

static void app_usb_stop(void)
{
    uint32_t actual_mhz = 0;
    usb_dm_stop();

    sys_hall_auto_sleep_in_idle(true);
    pm_cpu_freq_clear(&app_usb_freq_handle, &actual_mhz);
    app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_UNPLUG, NULL);
}

RAM_TEXT_SECTION
static bool usb_dev_trigger_evt(T_USB_EVT evt,  uint32_t param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB;
    gpio_msg.subtype = USB_MSG(USB_MSG_GROUP_DEV, evt);
    gpio_msg.u.param = param;

    return app_io_msg_send(&gpio_msg);
}

static void usb_dm_evt_handle(uint8_t evt, uint32_t data)
{

}

bool app_usb_connected(void)
{
    return (app_usb_db.usb_pwr_state > USB_PDN);
}

void app_usb_msg_handle(T_IO_MSG *msg)
{
    uint16_t subtype = msg->subtype;
    uint8_t group = USB_MSG_GROUP(subtype);
    uint8_t submsg = USB_MSG_SUBTYPE(subtype);
    uint32_t param = msg->u.param;

    switch (group)
    {
    case USB_MSG_GROUP_DEV:
        {
            usb_dm_evt_handle(submsg, param);
        }
        break;
#if F_APP_USB_AUDIO_SUPPORT
    case USB_MSG_GROUP_IF_AUDIO:
        {
            usb_audio_stream_evt_handle(submsg, param);
        }
        break;
#endif
    default:
        break;
    }
}

#if F_APP_USB_AUDIO_SUPPORT
static void app_usb_ipc_cback(uint32_t id, void *msg)
{
    switch (id)
    {
    case USB_IPC_EVT_AUDIO_DS_START:
        {
            if (app_usb_db.ds_playing == false)
            {
                app_usb_audio_start(USB_AUDIO_SCENARIO_PLAYBACK);
                app_usb_db.ds_playing = true;
            }
        }
        break;

    case USB_IPC_EVT_AUDIO_DS_STOP:
        {
            app_usb_db.ds_playing = false;
        }
        break;

    case USB_IPC_EVT_AUDIO_US_START:
        {
            app_usb_audio_start(USB_AUDIO_SCENARIO_CAPTURE);
        }
        break;

    case USB_IPC_EVT_AUDIO_US_STOP:
        {
            app_usb_audio_stop(USB_AUDIO_SCENARIO_CAPTURE);
        }
        break;

    case USB_IPC_EVT_AUDIO_DS_XMIT:
        {
            if (app_usb_db.ds_playing == false)
            {
                app_usb_audio_start(USB_AUDIO_SCENARIO_PLAYBACK);
                app_usb_db.ds_playing = true;
            }
        }
        break;

    default:
        break;
    }
}
#endif

void app_usb_init(void)
{
    memset(&app_usb_db, 0, sizeof(T_APP_USB_DB));

    usb_dev_init();
#if F_APP_USB_AUDIO_SUPPORT
    usb_audio_stream_init();
    app_usb_audio_init();
    app_ipc_subscribe(USB_IPC_TOPIC, app_usb_ipc_cback);
#endif
#if F_APP_USB_HID_SUPPORT
    usb_hid_init();
#endif
#if F_APP_USB_AUDIO_SUPPORT & F_APP_USB_HID_SUPPORT
    app_usb_audio_mmi_init();
#elif F_APP_USB_HID_SUPPORT
    app_usb_hid_init();
#endif

    T_USB_CORE_CONFIG config = {.speed = USB_SPEED_FULL, .class_set = {.hid_enable = 0, .uac_enable = 0}};
#if F_APP_USB_AUDIO_SUPPORT
    config.class_set.uac_enable = 1;
#endif
#if F_APP_USB_HID_SUPPORT
    config.class_set.hid_enable = 1;
#endif
    usb_dm_core_init(config);

    app_usb_start();
}
#endif
