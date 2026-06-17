/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_AUDIO_SUPPORT | F_APP_USB_HID_SUPPORT | F_APP_USB_MSC_SUPPORT | F_APP_USB_CDC_SUPPORT
#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "os_msg.h"
#include "os_task.h"
#include "section.h"
#include "sysm.h"
#include "hal_adp.h"
#include "pm.h"
#include "system_status_api.h"
#include "usb_audio_stream.h"
#include "usb_dev.h"
#include "usb_dm.h"
#include "usb_msg.h"
#include "app_auto_power_off.h"
#include "app_cfg.h"
#include "app_io_msg.h"
#include "app_usb.h"
#include "app_usb_audio.h"
#include "app_main.h"
#include "app_mmi.h"
#include "app_ipc.h"
#include "app_device.h"
#include "app_dlps.h"
#include "pm.h"
#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
#include "pmu_api.h"
#endif

#if F_APP_USB_HID_SUPPORT
#include "usb_hid.h"
#include "app_usb_hid.h"
#endif

#if F_APP_USB_MSC_SUPPORT
#include "usb_ms.h"
#include "rtl876x_sdio.h"
#include "sd.h"
#endif

#if F_APP_USB_CDC_SUPPORT
#include "usb_cdc.h"
#include "app_usb_cdc.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_cmd.h"
#include "app_tri_dongle_usb_hid_ctrl.h"
#include "app_tri_dongle_mgr.h"
#endif

typedef enum
{
    USB_EVT_PLUG,
    USB_EVT_UNPLUG,
    USB_EVT_PWR_STATUS_CHG,
    USB_EVT_BC12_DETECT,
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
} T_APP_USB_DB;

static T_APP_USB_DB app_usb_db;
static uint8_t app_usb_freq_handle = 0;
static bool app_usb_dev_trigger_evt(T_USB_EVT evt,  uint32_t param);

static BtPowerMode bt_pwr_state = BTPOWER_ACTIVE;
static POWERMode s_pwr_state = POWER_DLPS_MODE;

static void app_usb_set_hp_mode(void)
{
    uint32_t actual_mhz = 0;
    pm_cpu_set_auto_slow_enable(false);
    app_usb_db.store.pre_cpu_clk = pm_cpu_freq_get();
    pm_cpu_freq_req(&app_usb_freq_handle, pm_cpu_max_freq_get(), &actual_mhz);
    pm_dvfs_set_supreme();
    APP_PRINT_INFO2("app_usb_set_hp_mode, pre clk:%d, cur clk:%d", app_usb_db.store.pre_cpu_clk,
                    actual_mhz);
}

static void app_usb_set_lp_mode(void)
{
    uint32_t pre_cpu_clk, actual_mhz = 0;
    pm_cpu_set_auto_slow_enable(true);
    pre_cpu_clk = pm_cpu_freq_get();
#if F_APP_USB_SUSPEND_SUPPORT
    app_usb_db.store.pre_cpu_clk = 40; // later need to use pm_cpu_freq_req
#endif
    pm_cpu_freq_clear(&app_usb_freq_handle, &actual_mhz);
    APP_PRINT_INFO2("app_usb_set_lp_mode, pre clk:%d, cur clk:%d", pre_cpu_clk, actual_mhz);
}

#if F_APP_USB_SUSPEND_SUPPORT
static void app_usb_resume_dsp_clk(void)
{
    uint32_t actual_mhz_dsp = 0;
    pm_dsp1_freq_set(160, &actual_mhz_dsp);
}

static void app_usb_suspend_dsp_clk(void)
{
    uint32_t actual_mhz_dsp = 0;
    pm_dsp1_freq_set(40, &actual_mhz_dsp);
    pm_dvfs_set_by_clock();
}

static void app_usb_suspend_src(void)
{
    //Customers can handle bt actions according to their needs, such as disconneting bt link
}

static void app_usb_resume_src(void)
{
    //Customers can handle bt actions according to their needs, such as conneting bt link
}
#endif

void app_usb_start(void)
{
    if (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        app_usb_set_hp_mode();
        app_auto_power_off_disable(AUTO_POWER_OFF_MASK_USB_AUDIO_MODE);
        app_dlps_disable(APP_DLPS_ENTER_CHECK_USB);
        s_pwr_state = power_mode_get();
        power_mode_set(POWER_LPS_MODE);
        bt_pwr_state = bt_power_mode_get();
        bt_power_mode_set(BTPOWER_ACTIVE);
#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
        pmu_vcore3_pon_domain_enable(PMU_USB);
        usb_dm_start(false);
#else
        usb_dm_start(false);
#endif
        app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_PLUG, NULL);
    }
}

void app_usb_stop(void)
{
    if (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        app_auto_power_off_enable(AUTO_POWER_OFF_MASK_USB_AUDIO_MODE, app_cfg_const.timer_auto_power_off);
        usb_dm_stop();
        app_dlps_enable(APP_DLPS_ENTER_CHECK_USB);
#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
        pmu_vcore3_pon_domain_disable(PMU_USB);
#endif

        app_usb_set_lp_mode();
        app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_UNPLUG, NULL);
        bt_power_mode_set(bt_pwr_state);
        power_mode_set(s_pwr_state);
    }
}

RAM_TEXT_SECTION
static bool app_usb_dev_trigger_evt(T_USB_EVT evt,  uint32_t param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB;
    gpio_msg.subtype = USB_MSG(USB_MSG_GROUP_DEV, evt);
    gpio_msg.u.param = param;

    return app_io_msg_send(&gpio_msg);
}

static bool app_usb_dm_cb(T_USB_DM_EVT evt, T_USB_DM_EVT_PARAM *param)
{
    if (evt == USB_DM_EVT_STATUS_IND)
    {
        T_USB_DM_EVT_PARAM_STATUS_IND *status_ind = (T_USB_DM_EVT_PARAM_STATUS_IND *)param;
        T_USB_POWER_STATE usb_pwr_state = status_ind->state;
        if (usb_pwr_state == USB_RESUMING)
        {
            //resume interrupt handle to do
#if F_APP_USB_SUSPEND_SUPPORT
#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
            // bbpro3 don't need set cpu to xtal and set clk when resume
#else
            app_usb_resume_dsp_clk();
#endif
#endif
            app_usb_set_hp_mode();
        }

        app_usb_dev_trigger_evt(USB_EVT_PWR_STATUS_CHG, usb_pwr_state);
    }
    else if (evt == USB_DM_EVT_BC12_DETECT)
    {
        T_USB_DM_EVT_PARAM_BC12_DET *bc12_det = (T_USB_DM_EVT_PARAM_BC12_DET *)param;
        uint8_t bc12_type = bc12_det->type;
        app_usb_dev_trigger_evt(USB_EVT_BC12_DETECT, bc12_type);
    }
    return true;
}

#if F_APP_USB_SUSPEND_SUPPORT
static void app_usb_dm_evt_suspend_handle(void)
{
    if (app_device_is_power_on() == true)
    {
        app_usb_suspend_src();
    }
    app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_SUSPEND, NULL);
    app_usb_set_lp_mode();
#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
    // bbpro3 don't need to set clk when resume because it can enter dlps
#else
    app_usb_suspend_dsp_clk();
#endif
}

static void app_usb_dm_evt_resume_handle(void)
{
    app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_RESUME, NULL);
    if (app_device_is_power_on() == true)
    {
        app_usb_resume_src();
    }
}
#endif

static void app_usb_dm_evt_handle(uint8_t evt, uint32_t data)
{
    switch (evt)
    {
    case USB_EVT_PLUG:
        {
            app_usb_start();
        }
        break;

    case USB_EVT_UNPLUG:
        {
            app_usb_stop();

        }
        break;

    case USB_EVT_BC12_DETECT:
        {
            APP_PRINT_INFO1("app_usb_dev_trigger_evt, bc12_type 0x%x", data);
        }
        break;

    case USB_EVT_PWR_STATUS_CHG:
        {
            T_USB_POWER_STATE usb_pwr_state = (T_USB_POWER_STATE)data;
            APP_PRINT_INFO2("app_usb_dev_trigger_evt, old:0x%x, usb_pwr_state:0x%x", app_usb_db.usb_pwr_state,
                            usb_pwr_state);
#if F_APP_USB_SUSPEND_SUPPORT
            switch (usb_pwr_state)
            {
            case USB_SUSPENDED:
                {
                    app_usb_dm_evt_suspend_handle();
                }
                break;

            case USB_POWERED:
            case USB_DEFAULT:
            case USB_ADDRESSED:
            case USB_CONFIGURED:
                {
                    if (app_usb_db.usb_pwr_state == USB_SUSPENDED)
                    {
                        app_usb_dm_evt_resume_handle();
                    }
                    if (usb_pwr_state == USB_CONFIGURED)
                    {
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
                        app_tri_dongle_notify_usb_state_delay_info();
#endif
                    }
                }
                break;

            default:
                break;
            }
#endif

#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
            if (usb_pwr_state == USB_SUSPENDED)
            {
                app_dlps_enable(APP_DLPS_ENTER_CHECK_USB);
            }
            else if (usb_pwr_state > USB_PDN)
            {
                app_dlps_disable(APP_DLPS_ENTER_CHECK_USB);
            }
#endif
            app_usb_db.usb_pwr_state = usb_pwr_state;
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            app_tri_dongle_if_sys_evt(REDEVELOP_MSG_SYS_USB_STATUS, (uint8_t *)&app_usb_db.usb_pwr_state, NULL);
#endif
        }
        break;

    default:
        break;
    }
}

bool app_usb_connected(void)
{
    return (app_usb_db.usb_pwr_state > USB_PDN);
}

T_USB_POWER_STATE app_usb_power_state(void)
{
    return app_usb_db.usb_pwr_state;
}

static void app_usb_adp_state_change_cb(T_ADP_PLUG_EVENT event, void *user_data)
{
    if (event == ADP_EVENT_PLUG_IN)
    {
        app_usb_dev_trigger_evt(USB_EVT_PLUG, 0);
    }
    else if (event == ADP_EVENT_PLUG_OUT)
    {
        app_usb_dev_trigger_evt(USB_EVT_UNPLUG, 0);
    }
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
            app_usb_dm_evt_handle(submsg, param);
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

void app_usb_init(void)
{
    memset(&app_usb_db, 0, sizeof(T_APP_USB_DB));

    T_USB_DM_EVT_MSK evt_msk = {0};
    evt_msk.b.status_ind = 1;
    evt_msk.b.bc12_det = 1;
    usb_dm_cb_register(evt_msk, app_usb_dm_cb);
    adp_register_state_change_cb(ADP_DETECT_5V, (P_ADP_PLUG_CBACK)app_usb_adp_state_change_cb, NULL);

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_tri_dongle_usb_hid_report_init();
#endif

    usb_dev_init();

    T_USB_CORE_CONFIG config = {.speed = USB_SPEED_FULL, .class_set = {.hid_enable = 0, .uac_enable = 0}};
    config.speed = USB_SPEED_HIGH;
#if F_APP_USB_AUDIO_SUPPORT
    config.class_set.uac_enable = 1;//app_cfg2_const.usb_config.audio_class_en;
#endif
#if F_APP_USB_HID_SUPPORT
    config.class_set.hid_enable = 1;
#endif
    usb_dm_core_init(config);

#if F_APP_USB_AUDIO_SUPPORT
//    if (app_cfg2_const.usb_config.audio_class_en)
    {
        usb_audio_stream_init();
        app_usb_audio_init();
    }
#endif

#if F_APP_USB_CDC_SUPPORT
    usb_cdc_init();
    app_usb_cdc_open_pipe();
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_tri_dongle_usb_hid_load_desc_when_power_on();
#endif

#if F_APP_USB_HID_SUPPORT
    usb_hid_init();
    app_usb_hid_init();
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_usb_hid_open_pipe();
#endif

#if F_APP_USB_MSC_SUPPORT
    extern int usb_ms_scsi_init(void);
    usb_ms_scsi_init();
    extern int usb_ms_disk_init(void);
    usb_ms_disk_init();
    usb_msc_init();
#endif
}
#endif
