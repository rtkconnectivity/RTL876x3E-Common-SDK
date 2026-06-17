/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "os_mem.h"
#include "os_sched.h"
#include "os_msg.h"
#include "os_task.h"
#include "os_ext.h"
#include "system_status_api.h"
#include "rtl876x_pinmux.h"
#include "hal_gpio_int.h"
#include "hal_gpio.h"
#include "dlps_util.h"
#include "trace.h"
#include "audio.h"
#include "audio_probe.h"
#include "sysm.h"
#include "gap_br.h"
#include "gap.h"
#include "console_uart.h"
#include "test_mode.h"
#include "single_tone.h"
#include "fmc_api.h"
#include "spp_stream.h"
#include "ble_stream.h"
#include "iap_stream.h"
#include "app_console.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "app_dsp_cfg.h"
#include "app_cfg_nv.h"
#include "app_audio_cfg.h"
#include "app_charger_cfg.h"
#include "app_main.h"
#include "app_gap.h"
#include "app_io_msg.h"
#include "app_ble_cfg.h"
#include "app_ble_gap.h"
#include "app_ble_client.h"
#include "app_ble_service.h"
#include "app_dlps.h"
#include "app_bt_policy_cfg.h"
#include "app_bt_policy_api.h"
#include "app_sdp.h"
#include "app_transfer_cfg.h"
#include "app_transfer.h"
#include "app_timer.h"
#include "app_audio_policy.h"
#include "app_a2dp_cfg.h"
#include "app_a2dp.h"
#include "app_hfp_cfg.h"
#include "app_hfp.h"
#include "app_avrcp_cfg.h"
#include "app_avrcp.h"
#include "app_iap_cfg.h"
#include "app_iap.h"
#include "app_spp_cfg.h"
#include "app_spp.h"
#include "app_cmd.h"
#include "app_ble_device.h"


#include "app_mmi.h"
#include "app_customer.h"
#include "app_bond.h"

#include "app_bond.h"
#include "app_adp.h"

#include "app_amp.h"
#include "app_line_in.h"

#include "app_key_process.h"
#include "app_key_gpio.h"
#include "app_key_cfg.h"

#if F_APP_GUI_SUPPORT
#include "app_panel_init.h"
#endif

#if F_APP_BT_PROFILE_PBAP_PCE_SUPPORT
#include "app_pbap_cfg.h"
#include "app_pbap.h"
#endif

#include "app_hfp_ag.h"

#if F_APP_BT_PROFILE_MAP_MCE_SUPPORT
#include "app_map.h"
#include "app_map_cfg.h"
#endif

#if F_APP_TEST_SUPPORT
#include "app_test.h"
#endif
#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif

#if F_APP_BT_HID_DEVICE_SUPPORT
#include "app_hid_cfg.h"
#include "app_hid.h"
#endif

#if F_APP_BT_HID_HOST_SUPPORT
#include "app_hid_cfg.h"
#include "app_hid_host.h"
#endif

#if F_APP_QDECODE_SUPPORT
#include "app_qdec.h"
#endif
#if F_APP_IAP_RTK_SUPPORT
#include "app_iap_rtk.h"
#endif

#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
#include "bt_bond_api.h"
#if (F_APP_LE_AUDIO_ACCEPTOR_SUPPORT || F_APP_LE_AUDIO_INITIATOR_SUPPORT)
#include "ble_audio_bond.h"
#endif
#endif

#if F_APP_HIFI4_SUPPORT
#include "app_buck_tps62860.h"
#include "app_buck_apw7564.h"
#include "ipc.h"
#endif
#ifdef F_APP_DEBUG_TASK_PROFILING
#include "hal_debug.h"
#endif

#if (F_APP_A2DP_XMIT_SRC_SUPPORT || F_APP_A2DP_XMIT_SRC_LEA_SUPPORT)
#include "app_a2dp_xmit_mgr.h"
#endif

#if (F_APP_SCO_XMIT_AG_SUPPORT || F_APP_SCO_XMIT_HF_SUPPORT)
#include "app_sco_xmit_mgr.h"
#endif

#if F_SOURCE_PLAY_SUPPORT
#include "app_src_play.h"
#endif

#if BAP_BROADCAST_SOURCE
#include "app_lea_ini_profile.h"
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
#include "app_findmy_task.h"
#include "app_findmy_ble.h"
#include "app_findmy.h"
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_gfps_cfg.h"
#include "app_gfps.h"
#include "app_gfps_msg.h"
#include "ecc_enhanced.h"
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
#include "app_dult.h"
#include "app_dult_device.h"
#endif
#endif

#if F_APP_DISABLE_NOTIFICATION_SUPPORT
#include "ringtone.h"
#include "voice_prompt.h"
#endif

#if F_APP_DATA_CAPTURE_SUPPORT
#include "app_data_capture_cs.h"
#endif

#if F_APP_AUTO_POWER_TEST_LOG
#include "app_power_test.h"
#endif

#if F_APP_DSP_SHM_80KB_TO_MCU_CHECK_SUPPORT
#include "os_heap.h"
#endif

#ifndef TARGET_RTL8773DO
#include "mem_config.h"
#endif

#include "bt_gatt_svc.h"
#include "app_ota.h"

#if F_APP_USB_AUDIO_SUPPORT | F_APP_USB_MSC_SUPPORT | F_APP_USB_HID_SUPPORT | F_APP_USB_CDC_SUPPORT
#include "app_usb.h"
#endif

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
#include "app_lea_acc_profile.h"
#endif


#if TRANSMIT_CLIENT_SUPPORT
#include "app_le_transfer.h"
#endif

#if F_APP_ONE_WIRE_UART_SUPPORT
#include "app_one_wire_uart.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_mgr.h"
#include "app_tri_dongle_cmd.h"
#if F_APP_UWB_SCENARIO_SUPPORT
#include "app_tri_dongle_uwb.h"
#endif
#if CONFIG_YYLX_DONGLE_FEATURE
#include "app_tri_dongle_yylx_multi_uhid.h"
#endif
#endif

#if F_APP_GUI_SUPPORT
#include "app_panel_msg.h"
#include "app_panel_init.h"
#endif

#if F_APP_CFU_FEATURE_SUPPORT
#include "app_common_cfu.h"
#endif

#if F_APP_CHARGING_CASE_CMD_SUPPORT
#include "app_charging_case_cmd.h"
#endif


#if F_APP_MULTILINK_ENABLE
#include "app_multilink.h"
#endif

#if F_APP_MALLEUS_SUPPORT
#include "app_malleus.h"
#endif

#if F_TRANS_UPDATE_FILE_SUPPORT
#include "app_trans_update_file.h"
#endif

#define MAX_NUMBER_OF_GAP_MESSAGE       0x20    //!< indicate BT stack message queue size
#define MAX_NUMBER_OF_IO_MESSAGE        0x20    //!< indicate io queue size
#define MAX_NUMBER_OF_DSP_MSG           0x20    //!< number of dsp message reserved for DSP message handling.
#define MAX_NUMBER_OF_CODEC_MSG         0x10    //!< number of codec message reserved for CODEC message handling.
#define MAX_NUMBER_OF_ANC_MSG           0x10    //!< number of anc message reserved for ANC message handling.
#define MAX_NUMBER_OF_SYS_MSG           0x20    //!< indicate SYS timer queue size
#define MAX_NUMBER_OF_LOADER_MSG        0x20    //!< indicate Bin Loader queue size
#define MAX_NUMBER_OF_APP_TIMER_MODULE  0x30    //!< indicate app timer module size
#define MAX_NUMBER_OF_GUI_MODULE        0x20    //!< indicate app timer module size
/** indicate rx event queue size*/
#define MAX_NUMBER_OF_RX_EVENT      \
    (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE + \
     MAX_NUMBER_OF_DSP_MSG + MAX_NUMBER_OF_CODEC_MSG + MAX_NUMBER_OF_ANC_MSG + \
     MAX_NUMBER_OF_SYS_MSG + MAX_NUMBER_OF_LOADER_MSG + MAX_NUMBER_OF_GUI_MODULE)

#define DEFAULT_PAGESCAN_WINDOW         0x12
#define DEFAULT_PAGESCAN_INTERVAL       0x800
#define DEFAULT_PAGE_TIMEOUT            0x2000
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#define DEFAULT_SUPVISIONTIMEOUT        0x1388
#else
#define DEFAULT_SUPVISIONTIMEOUT        0x1f40
#endif
#define DEFAULT_INQUIRYSCAN_WINDOW      0x12
#define DEFAULT_INQUIRYSCAN_INTERVAL    0x800

void *audio_evt_queue_handle;
void *audio_io_queue_handle;

T_APP_DB app_db = {};

/**
* @brief board_init() contains the initialization of pinmux settings and pad settings.
*
*   All the pinmux settings and pad settings shall be initiated in this function.
*   But if legacy driver is used, the initialization of pinmux setting and pad setting
*   should be peformed with the IO initializing.
*
* @return void
*/
static void board_init(void)
{
#if F_APP_GUI_SUPPORT
    uint32_t freq_actual;
    uint32_t gui_cpu_need = app_panel_get_cpu_freq();
    if (gui_cpu_need > pm_cpu_freq_get())
    {
        uint8_t freq_handle = 0;
        pm_cpu_freq_req(&freq_handle, gui_cpu_need, &freq_actual);
    }
#endif
    PAD_Pull_Mode pull_mode;

    if (app_cfg_const.enable_data_uart)
    {
        {
            Pinmux_Config(app_cfg_const.data_uart_tx_pinmux, UART0_TX);
            Pad_Config(app_cfg_const.data_uart_tx_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }

        Pinmux_Config(app_cfg_const.data_uart_rx_pinmux, UART0_RX);
        Pad_Config(app_cfg_const.data_uart_rx_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        if (app_cfg_const.enable_rx_wake_up)
        {
            Pinmux_Config(app_cfg_const.rx_wake_up_pinmux, DWGPIO);
            Pad_Config(app_cfg_const.rx_wake_up_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }

        if (app_cfg_const.enable_tx_wake_up)
        {
            Pinmux_Config(app_cfg_const.tx_wake_up_pinmux, DWGPIO);
            Pad_Config(app_cfg_const.tx_wake_up_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
        }
    }

    if (app_cfg_const.dsp_log_output_select == DSP_OUTPUT_LOG_BY_UART)
    {
        Pinmux_Config(app_cfg_const.dsp_log_pin, UART2_TX);
        Pad_Config(app_cfg_const.dsp_log_pin,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }

    //Config I2C0 pinmux. For CP control
    if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK ||
        app_cfg_const.ext_buck_support)
    {
        Pinmux_Config(app_cfg_const.i2c_0_dat_pinmux, I2C0_DAT);
        Pinmux_Config(app_cfg_const.i2c_0_clk_pinmux, I2C0_CLK);

        {
#if (TARGET_RTL8773DO || TARGET_RTL8773DFL)
            Pad_Config(app_cfg_const.i2c_0_dat_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
            Pad_Config(app_cfg_const.i2c_0_clk_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
#else
            Pad_Config(app_cfg_const.i2c_0_dat_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
            Pad_Config(app_cfg_const.i2c_0_clk_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
#endif
            Pad_PullConfigValue(app_cfg_const.i2c_0_dat_pinmux, PAD_STRONG_PULL);
            Pad_PullConfigValue(app_cfg_const.i2c_0_clk_pinmux, PAD_STRONG_PULL);
        }
    }

#if F_APP_LINEIN_SUPPORT
    if (app_cfg_const.line_in_support)
    {
        app_line_in_driver_init();
        app_dlps_pad_wake_up_polarity_invert(app_cfg_const.line_in_pinmux);
    }
#endif

    //Config amp pinmux
    if (app_cfg_const.enable_ctrl_ext_amp)
    {

#if F_APP_CUSTOMER_EXT_PA_SUPPORT_AW_CUSTOMER == 0
#if F_APP_CUSTOMER_EXT_PA_SUPPORT_AW87390 == 0
        Pinmux_Config(app_cfg_const.ext_amp_pinmux, DWGPIO);
        if (app_cfg_const.enable_ext_amp_high_active)
        {
            Pad_Config(app_cfg_const.ext_amp_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }
        else
        {
            Pad_Config(app_cfg_const.ext_amp_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
        }
#endif
#endif

    }

    //Config thermistor power pinmux
    if (app_cfg_const.thermistor_power_support && (app_cfg_const.thermistor_power_pinmux != 0xFF))
    {
        Pad_Config(app_cfg_const.thermistor_power_pinmux,
                   PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }

    //Config pcba shipping mode pinmux
    if (app_cfg_const.pcba_shipping_mode_pinmux != 0xFF)
    {
        Pad_Config(app_cfg_const.pcba_shipping_mode_pinmux,
                   PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }

#if F_APP_QDECODE_SUPPORT
    if (app_cfg_const.wheel_support)
    {
        app_qdec_pad_config();
    }
#endif


    if (app_cfg_const.dsp_gpio_num)
    {
        for (uint8_t i = 0; i < app_cfg_const.dsp_gpio_num; i++)
        {
            Pinmux_Config(app_cfg_const.dsp_gpio_pinmux[i], DSP_GPIO_OUT);
            Pad_Config(app_cfg_const.dsp_gpio_pinmux[i],
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }
    }

#if (TARGET_RTL8773DO || TARGET_RTL8773DFL )
    if (app_cfg_const.dsp2_gpio_num)
    {
        for (uint8_t i = 0; i < app_cfg_const.dsp2_gpio_num; i++)
        {
            Pinmux_Config(app_cfg_const.dsp2_gpio_pinmux[i], DSP2_GPIO_OUT);
            Pad_Config(app_cfg_const.dsp2_gpio_pinmux[i],
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }
    }

    if (app_cfg_const.adsp_gpio_num)
    {
        for (uint8_t i = 0; i < app_cfg_const.adsp_gpio_num; i++)
        {
            Pinmux_Config(app_cfg_const.adsp_gpio_pinmux[i], ANCDSP_GPIO_OUT);
            Pad_Config(app_cfg_const.adsp_gpio_pinmux[i],
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }
    }
#endif

    if (app_cfg_const.dsp_jtag_enable)
    {
        Pinmux_Config(app_cfg_const.dsp_jtck_pinmux, DSP_JTCK);
        Pad_Config(app_cfg_const.dsp_jtck_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.dsp_jtdi_pinmux, DSP_JTDI);
        Pad_Config(app_cfg_const.dsp_jtdi_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.dsp_jtdo_pinmux, DSP_JTDO);
        Pad_Config(app_cfg_const.dsp_jtdo_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.dsp_jtms_pinmux, DSP_JTMS);
        Pad_Config(app_cfg_const.dsp_jtms_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.dsp_jtrst_pinmux, DSP_JTRST);
        Pad_Config(app_cfg_const.dsp_jtrst_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    }

#if (TARGET_RTL8773DO || TARGET_RTL8773DFL )
    if (app_cfg_const.dsp2_jtag_enable)
    {
        Pinmux_Config(app_cfg_const.dsp2_jtck_pinmux, DSP2_JTCK);
        Pad_Config(app_cfg_const.dsp2_jtck_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.dsp2_jtdi_pinmux, DSP2_JTDI);
        Pad_Config(app_cfg_const.dsp2_jtdi_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.dsp2_jtdo_pinmux, DSP2_JTDO);
        Pad_Config(app_cfg_const.dsp2_jtdo_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.dsp2_jtms_pinmux, DSP2_JTMS);
        Pad_Config(app_cfg_const.dsp2_jtms_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.dsp2_jtrst_pinmux, DSP2_JTRST);
        Pad_Config(app_cfg_const.dsp2_jtrst_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    }

    if (app_cfg_const.adsp_jtag_enable)
    {
        Pinmux_Config(app_cfg_const.adsp_jtck_pinmux, ANCDSP_JTCK);
        Pad_Config(app_cfg_const.adsp_jtck_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.adsp_jtdi_pinmux, ANCDSP_JTDI);
        Pad_Config(app_cfg_const.adsp_jtdi_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.adsp_jtdo_pinmux, ANCDSP_JTDO);
        Pad_Config(app_cfg_const.adsp_jtdo_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.adsp_jtms_pinmux, ANCDSP_JTMS);
        Pad_Config(app_cfg_const.adsp_jtms_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.adsp_jtrst_pinmux, ANCDSP_JTRST);
        Pad_Config(app_cfg_const.adsp_jtrst_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    }

#if F_APP_EXT_PA_SUPPORT
    if (app_cfg_const.ext_pa_enable)
    {
        Pinmux_Config(app_cfg_const.ext_pa_pinmux, EN_EXPA);
        Pad_Config(app_cfg_const.ext_pa_pinmux, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
                   PAD_OUT_DISABLE, PAD_OUT_LOW);
    }
    if (app_cfg_const.ext_lna_enable)
    {
        Pinmux_Config(app_cfg_const.ext_lna_pinmux, EN_EXLNA);
        Pad_Config(app_cfg_const.ext_lna_pinmux, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
                   PAD_OUT_DISABLE, PAD_OUT_LOW);
    }
#endif
#endif
}

#if F_APP_EXT_FLASH_SUPPORT
static bool ext_flash_init(void)
{
    return fmc_flash_nor_init(FMC_SPIC_ID_3);
}
#endif

static void driver_init(void)
{
    app_adp_init();

    if (app_key_cfg.key_gpio_support)
    {
        hal_gpio_init();
        hal_gpio_int_init();
        hal_gpio_set_debounce_time(30);
        key_gpio_initial();
        app_key_init();
    }

    if (app_key_cfg.mfb_replace_key0)
    {
        key_gpio_mfb_init();
    }

#if F_APP_IAP_SUPPORT
    if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK)
    {
        app_iap_cp_hw_init(I2C0);
    }
#endif

#if F_APP_CONSOLE_SUPPORT
    if (app_cfg_const.enable_data_uart || app_cfg_const.one_wire_uart_support)
    {
        app_console_init();
    }
#endif

    app_charger_cfg_init();
    if (app_charger_cfg.charger_support || app_charger_cfg.discharger_support)
    {
        app_charger_init();
    }

#if F_APP_LINEIN_SUPPORT
    if (app_cfg_const.line_in_support)
    {
        app_line_in_driver_init();
        app_dlps_pad_wake_up_polarity_invert(app_cfg_const.line_in_pinmux);
    }
#endif

#if F_APP_AMP_SUPPORT
    if (app_cfg_const.enable_ctrl_ext_amp)
    {
        app_amp_cfg_init();
        app_amp_init();
    }
#endif

#if F_APP_QDECODE_SUPPORT
    if (app_cfg_const.wheel_support)
    {
        app_qdec_init_status_read();
        app_qdec_driver_init();
    }
#endif

#if F_APP_PERIODIC_WAKEUP_RECHARGE
    app_dlps_system_wakeup_clear_rtc_int();
#endif

#if F_APP_HIFI4_SUPPORT
    if (app_cfg_const.ext_buck_support)
    {
        app_buck_tps62860_init();
    }
    else
    {
        APP_PRINT_TRACE0("driver_init: not init buck support");
    }
#endif

#if F_APP_EXT_FLASH_SUPPORT
    ext_flash_init();
#endif

}

static void app_bt_gap_init(void)
{
    uint16_t supervision_timeout = DEFAULT_SUPVISIONTIMEOUT;

#if F_APP_INTEGRATED_TRANSCEIVER
    uint16_t link_policy = GAP_LINK_POLICY_SNIFF_MODE;
#else
    uint16_t link_policy = GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE;
#endif
    uint8_t radio_mode = GAP_RADIO_MODE_NONE_DISCOVERABLE;
    bool limited_discoverable = false;
    bool auto_accept_acl = false;

    uint8_t  pagescan_type = GAP_PAGE_SCAN_TYPE_INTERLACED;
    uint16_t pagescan_interval = DEFAULT_PAGESCAN_INTERVAL;
    uint16_t pagescan_window = DEFAULT_PAGESCAN_WINDOW;
    uint16_t page_timeout = DEFAULT_PAGE_TIMEOUT;
    uint8_t  page_scan_repetition_mode = GAP_PAGE_SCAN_REPETITION_R1;

    uint8_t inquiryscan_type = GAP_INQUIRY_SCAN_TYPE_INTERLACED;
    uint16_t inquiryscan_window = DEFAULT_INQUIRYSCAN_WINDOW;
    uint16_t inquiryscan_interval = DEFAULT_INQUIRYSCAN_INTERVAL;
    uint8_t inquiry_mode = GAP_INQUIRY_MODE_EXTENDED_RESULT;

    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_GENERAL_BONDING_FLAG |
                          GAP_AUTHEN_BIT_SC_FLAG | GAP_AUTHEN_BIT_FORCE_CENTRAL_ENCRYPT_FLAG;

#if F_APP_BREDR_SC_CTKD_SUPPORT
    auth_flags |= GAP_AUTHEN_BIT_SC_BR_FLAG;
#endif

#if F_APP_CHARGE_CASE_DEMO_SUPPORT | F_APP_DASHBOARD_DEMO_SUPPORT
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
#else
    uint8_t io_cap = GAP_IO_CAP_DISPLAY_YES_NO;
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    if (app_cfg_nv.trigle_dongle_support_passkey == EVENT_SET_PASSKEY_SUPPORT_ENABLE)
    {
        io_cap = GAP_IO_CAP_DISPLAY_YES_NO;
    }
    else
    {
        io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    }
#endif

    uint8_t oob_enable = false;
    uint8_t bt_mode = GAP_BT_MODE_21ENABLED;

#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
    bt_bond_init();
#if (F_APP_LE_AUDIO_ACCEPTOR_SUPPORT || F_APP_LE_AUDIO_INITIATOR_SUPPORT)
    ble_audio_bond_init();
#endif
#endif

    if ((app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE) && (app_cfg_nv.factory_reset_done == 0))
    {
        /* change the scan interval to 100ms(0xA0 * 0.625) before factory reset */
        pagescan_interval = 0xA0;
        inquiryscan_interval = 0xA0;
    }

    gap_lib_init();

    //0: to be master
    gap_br_cfg_accept_role(1);

    gap_br_set_param(GAP_BR_PARAM_NAME, GAP_DEVICE_NAME_LEN, app_cfg_nv.device_name_legacy);


    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);

    gap_br_set_param(GAP_BR_PARAM_BT_MODE, sizeof(uint8_t), &bt_mode);
    gap_br_set_param(GAP_BR_PARAM_COD, sizeof(uint32_t), &app_cfg_const.class_of_device);
    gap_br_set_param(GAP_BR_PARAM_LINK_POLICY, sizeof(uint16_t), &link_policy);
    gap_br_set_param(GAP_BR_PARAM_SUPV_TOUT, sizeof(uint16_t), &supervision_timeout);
    gap_br_set_param(GAP_BR_PARAM_AUTO_ACCEPT_ACL, sizeof(bool), &auto_accept_acl);

    gap_br_set_param(GAP_BR_PARAM_RADIO_MODE, sizeof(uint8_t), &radio_mode);
    gap_br_set_param(GAP_BR_PARAM_LIMIT_DISCOV, sizeof(bool), &limited_discoverable);

    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_TYPE, sizeof(uint8_t), &pagescan_type);
    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_INTERVAL, sizeof(uint16_t), &pagescan_interval);
    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_WINDOW, sizeof(uint16_t), &pagescan_window);
    gap_br_set_param(GAP_BR_PARAM_PAGE_TIMEOUT, sizeof(uint16_t), &page_timeout);
    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_REPETITION_MODE, sizeof(uint8_t),
                     &page_scan_repetition_mode);

    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_TYPE, sizeof(uint8_t), &inquiryscan_type);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_INTERVAL, sizeof(uint16_t), &inquiryscan_interval);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_WINDOW, sizeof(uint16_t), &inquiryscan_window);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_MODE, sizeof(uint8_t), &inquiry_mode);

    bt_pairing_tx_power_set(-2);

    app_ble_gap_param_init();
}

static void framework_init(void)
{
    /* System Manager */
    sys_mgr_init(audio_evt_queue_handle);

    /* RemoteController Manager */
    remote_mgr_init((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    remote_local_addr_set(app_cfg_nv.bud_local_addr);

    /* Bluetooth Manager */
    bt_mgr_init();

    /* Audio Manager */
    audio_mgr_init(PLAYBACK_POOL_SIZE, VOICE_POOL_SIZE, RECORD_POOL_SIZE, NOTIFICATION_POOL_SIZE);
    audio_mgr_set_max_plc_count(app_audio_cfg.maximum_packet_lost_compensation_count);

    if ((app_cfg_const.dsp_jtag_enable) || (app_cfg_const.dsp2_jtag_enable) ||
        (app_cfg_const.adsp_jtag_enable))
    {
        audio_probe_disable_dsp_powerdown();
    }

#if F_APP_DISABLE_NOTIFICATION_SUPPORT
    ringtone_mode_set(RINGTONE_MODE_SILENT);
    voice_prompt_mode_set(VOICE_PROMPT_MODE_SILENT);
#endif

#if F_APP_MULTI_CHANNEL_SUPPORT
    audio_track_policy_set(AUDIO_TRACK_POLICY_MULTI_STREAM);
#endif
}

static void app_task(void *pvParameters)
{
    uint8_t event;

    app_adp_detect();
    gap_start_bt_stack(audio_evt_queue_handle, audio_io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    app_gfps_msg_queue_init();
#endif

#if F_APP_ENABLE_TWO_ONE_WIRE_UART
    app_one_wire_uart_open(T_IDX_UART0);
    app_one_wire_uart_open(T_IDX_UART2);
#endif
#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    app_findmy_crypto_init();
#endif

    APP_PRINT_WARN3("app_task: unused mem ITCM %d, BUFFER_ON %d, cpu freq %d MHz",
                    mem_peek(),
                    os_mem_peek(RAM_TYPE_BUFFER_ON),
                    pm_cpu_freq_get());

    while (true)
    {
        if (os_msg_recv(audio_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (EVENT_GROUP(event) == EVENT_GROUP_IO)
            {
                T_IO_MSG io_msg;

                if (os_msg_recv(audio_io_queue_handle, &io_msg, 0) == true)
                {
                    if (event == EVENT_IO_TO_APP)
                    {
                        app_io_msg_handler(io_msg);
                    }
                }
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_STACK)
            {
                gap_handle_msg(event);
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_FRAMEWORK)
            {
                sys_mgr_event_handle(event);
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_APP)
            {
                app_timer_handle_msg(event);
            }
#if F_APP_GUI_SUPPORT
            else if (EVENT_GROUP(event) == EVENT_GROUP_GUI)
            {
                app_panel_msg_handle(event);
            }
#endif
        }
    }
}

#if F_APP_DSP_SHM_80KB_TO_MCU_CHECK_SUPPORT
void shm_data_copy(void)
{
#if defined (__CC_ARM)
    extern unsigned int Load$$SHARE_RAM_DATA$$RW$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RW$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RW$$Length;
    extern unsigned int Load$$SHARE_RAM_DATA$$ZI$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$ZI$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$ZI$$Length;

    uint32_t load_addr = (uint32_t)&Load$$SHARE_RAM_DATA$$RW$$Base;
    uint32_t dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$RW$$Base;
    uint32_t len = (uint32_t)&Image$$SHARE_RAM_DATA$$RW$$Length;
    memcpy((uint8_t *)dest_addr, (uint8_t *)load_addr, len);

    dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$ZI$$Base;
    len = (uint32_t)&Image$$SHARE_RAM_DATA$$ZI$$Length;
    memset((uint8_t *)dest_addr, 0, len);

    extern unsigned int Load$$SHARE_RAM_DATA$$RO$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RO$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RO$$Length;

    load_addr = (uint32_t)&Load$$SHARE_RAM_DATA$$RO$$Base;
    dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$RO$$Base;
    len = (uint32_t)&Image$$SHARE_RAM_DATA$$RO$$Length;
    memcpy((uint8_t *)dest_addr, (uint8_t *)load_addr, len);
#elif defined (__GNUC__)
    extern unsigned int *__share_ram_load_addr__;
    extern unsigned int *__share_ram_dst_addr__;
    extern unsigned int *__share_ram_code_length__;
    memcpy((void *)&__share_ram_dst_addr__,
           (void *)&__share_ram_load_addr__,
           (uint32_t)&__share_ram_code_length__);

#endif
}


void ram_config()
{
    /**
     *
        extern bool bin_loader_drv_get_Is80KShared2MCU(uint8_t *Is80KShared2MCU);
        API ret 1, Is80KShared2MCU = 0x01 ==> new dsp header, share 80K
        API ret 1, Is80KShared2MCU = 0x00 ==> new dsp header, snot share 80K
        API ret 0, Is80KShared2MCU = 0x01 ==> old dsp image
        API ret 0, Is80KShared2MCU = 0x00 ==> old dsp image
    */
    uint8_t is_dsp_share_80K_to_mcu = 0x00;
    extern bool bin_loader_drv_get_Is80KShared2MCU(uint8_t *Is80KShared2MCU);

    if (bin_loader_drv_get_Is80KShared2MCU(&is_dsp_share_80K_to_mcu))
    {
        if (is_dsp_share_80K_to_mcu == 0x01)
        {
            extern void sys_hall_set_dsp_share_memory_80k(bool is_off_ram);
            sys_hall_set_dsp_share_memory_80k(false);

            heap_shm_init(DSP_SHM_HEAP_ADDR, DSP_SHM_HEAP_SIZE);
            heap_shm_set(DSP_SHM_GLOBAL_ADDR, DSP_SHM_TOTAl_SIZE, 0);
            shm_data_copy();
            add_shm_to_continuous_tail();
        }
    }

}
#endif


int main(void)
{
#if F_APP_DSP_SHM_80KB_TO_MCU_CHECK_SUPPORT
    ram_config();
#endif

    uint32_t time_entry_app;
    void *app_task_handle;
    uint8_t wake_up_reason;
    uint16_t stack_size = 1024 * 3 - 256;

    log_module_trace_set(MODULE_HCI, LEVEL_TRACE, false);
    log_module_trace_set(MODULE_BTIF, LEVEL_TRACE, false);
    log_module_trace_set(MODULE_GAP, LEVEL_TRACE, false);

    time_entry_app = sys_timestamp_get();
    wake_up_reason = power_down_check_wake_up_reason();
    APP_PRINT_INFO2("TIME FROM PATCH TO APP: %d ms, wake_up_reason: 0x%x", time_entry_app,
                    wake_up_reason);
    APP_PRINT_INFO2("APP COMPILE TIME: [%s - %s]", TRACE_STRING(__DATE__), TRACE_STRING(__TIME__));

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    stack_size = 1024 * 6;
#endif

    {
        uint8_t freq_handle = 0;
        uint32_t actual_mhz = 0;

#if F_APP_MULTI_CHANNEL_SUPPORT
        pm_cpu_freq_set(100, &actual_mhz);
#else
        pm_cpu_freq_set(40, &actual_mhz);
#endif

    }

    if (sys_hall_get_reset_status())
    {
        APP_PRINT_INFO0("APP RESTART FROM WDT_SOFTWARE_RESET");
    }
    else
    {
        //APP power off reboot also in this case
        APP_PRINT_INFO0("APP START FROM HW_RESET");
    }

    os_msg_queue_create(&audio_io_queue_handle, "ioQ", MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&audio_evt_queue_handle, "evtQ", MAX_NUMBER_OF_RX_EVENT, sizeof(unsigned char));
#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    os_msg_queue_create(&app_findmy_queue_handle, "findmy_msg", MAX_NUMBER_OF_IO_MESSAGE,
                        sizeof(T_FINDMY_BLE_INDICATION));
#endif
#if F_APP_GUI_SUPPORT
    app_panel_msg_channel_register(audio_evt_queue_handle, audio_io_queue_handle,
                                   MAX_NUMBER_OF_GUI_MODULE);
#endif

    app_init_timer(audio_evt_queue_handle, MAX_NUMBER_OF_APP_TIMER_MODULE);
    pm_cpu_freq_init();
    app_ipc_init();

    app_mcu_cfg_init();

    app_key_cfg_init();

    app_dsp_cfg_init(app_audio_cfg.normal_apt_support);
    app_mcu_cfg_nv_load();

    app_transfer_cfg_init();
    board_init();
    app_bt_gap_init();
    framework_init();

    //Callback provider for other modules MUST init fisrt
    app_ota_init();

    //driver init MUST after callback provider init
    driver_init();

    //Other app module MUST init after here
    app_dlps_init();
    app_auto_power_off_init();
    app_audio_cfg_init();

    app_mmi_init();

    if (is_single_tone_test_mode()) //DUT test mode
    {
        reset_single_tone_test_mode();
        mp_hci_test_init(MP_HCI_TEST_DUT_MODE);
    }
    else //Normal mode
    {

#if F_APP_TEST_SUPPORT
        app_test_init();
#endif

        app_gap_init();
        app_ble_cfg_init();
        app_ble_gap_init();
        app_bt_policy_cfg_init();
        app_bt_policy_init();
        app_ble_client_init();

        gatt_svc_init(GATT_SVC_USE_EXT_SERVER, 0);
        T_GATT_SVC_PENDING_NUM num;
        num.notify_num = 20;
        num.ind_num = 10;
        gatt_svc_cfg_pending_num(num);

        app_hfp_cfg_init();

#if F_APP_HFP_AG_SUPPORT
        app_hfp_ag_init();
#endif

#if F_APP_HFP_HF_SUPPORT
        app_hfp_hf_init();
#endif

        app_avrcp_cfg_init();
        app_avrcp_init();
        app_a2dp_cfg_init();
        app_a2dp_init();
        app_sdp_init();
        app_spp_cfg_init();
        app_spp_init();

        app_audio_init();

#if F_APP_BT_PROFILE_PBAP_PCE_SUPPORT
        app_pbap_cfg_init();
        app_pbap_init();
#endif

#if F_APP_BT_HID_DEVICE_SUPPORT
        app_hid_cfg_init();
        app_hid_init();
#endif

#if F_APP_BT_HID_HOST_SUPPORT
        app_hid_cfg_init();
        app_hid_host_init();
#endif

#if F_APP_IAP_SUPPORT
        app_iap_cfg_init();
        app_iap_init();
#endif

#if F_APP_BT_PROFILE_MAP_MCE_SUPPORT
        app_map_cfg_init();
        app_map_init();
#endif

#if F_APP_MULTILINK_ENABLE
        app_multilink_init();
#endif
        app_ble_service_init();

        app_device_init();
        app_ble_device_init();
        app_transfer_init();

#if F_APP_LINEIN_SUPPORT
        app_line_in_init();
#endif

        app_cmd_init();
        app_bond_init();
#if F_TRANS_UPDATE_FILE_SUPPORT
        app_trans_update_file_init();
#endif

#if F_APP_DATA_CAPTURE_SUPPORT
        app_data_capture_init();
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
        app_findmy_ble_bond_sync_init();
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
        app_gfps_module_init();
#endif
        app_customer_init();

#if F_APP_IAP_RTK_SUPPORT
        spp_stream_init(0xff);
        ble_stream_init(0xff);
        iap_stream_init(0xff);
        app_iap_rtk_init();
#endif

#if TRANSMIT_CLIENT_SUPPORT
        app_le_transfer_init();
#endif

#if F_APP_CHARGING_CASE_CMD_SUPPORT
        app_charging_case_init();
#endif

#if F_APP_QDECODE_SUPPORT
        if (app_cfg_const.wheel_support)
        {
            app_qdec_init();
        }
#endif

#if (F_APP_A2DP_XMIT_SRC_SUPPORT || F_APP_A2DP_XMIT_SRC_LEA_SUPPORT)
        app_a2dp_xmit_mgr_init();
#endif

#if (F_APP_SCO_XMIT_AG_SUPPORT || F_APP_SCO_XMIT_HF_SUPPORT)
        app_sco_xmit_init();
#endif

#if BAP_BROADCAST_SOURCE
        app_lea_profile_init();
#endif

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
        app_lea_acc_profile_init();
#endif

#if F_APP_SD_CARD_SUPPORT
        app_sd_card_init();
#endif

#if F_SOURCE_PLAY_SUPPORT
        app_src_play_init();
#endif

#if F_APP_GUI_SUPPORT
        app_gui_init();
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
        uint32_t spic0_freq = 0;
        bool ret = fmc_flash_nor_clock_switch(FMC_SPIC_ID_0, 160, &spic0_freq);
        APP_PRINT_INFO1("fmc_flash_nor_clock_switch: set flash clock result %d", ret);
        app_tri_dongle_mgr_init();

#if F_APP_NXP_UWB_DRIVER_SUPPORT
        extern void uwb_task_init(void);
        uwb_task_init();
#endif
#if F_APP_UWB_SCENARIO_SUPPORT
        app_tri_dongle_uwb_init();
#endif
#if CONFIG_YYLX_DONGLE_FEATURE
        app_tri_dongle_yylx_multi_uhid_init();
#endif
#endif

#if F_APP_USB_AUDIO_SUPPORT | F_APP_USB_MSC_SUPPORT | F_APP_USB_HID_SUPPORT | F_APP_USB_CDC_SUPPORT
        app_usb_init();
#endif

#if F_APP_CFU_FEATURE_SUPPORT
        app_cfu_init();
#endif

#if F_APP_AUTO_POWER_TEST_LOG
        app_power_test_init();
#endif
        //increase app task priority to 2 so that user could implement some background daemon task
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
        os_task_create(&app_task_handle, "app_task", app_task, NULL, stack_size, 3);
#else
        os_task_create(&app_task_handle, "app_task", app_task, NULL, stack_size, 2);
#endif

#if F_APP_DEBUG_TASK_PROFILING
        hal_debug_init();
        hal_debug_task_time_proportion_init(5000);
#endif

#if F_APP_DEBUG_HIT_RATE_PRINT
        cache_hit_count_init(10000);
#endif
    }

#if F_APP_HIFI4_SUPPORT
    void *ipc_task_handle = NULL;
    os_task_create(&ipc_task_handle, "ipc_task", ipc_task, NULL, 1024 * 2, 2);
#endif

#if F_APP_MONITOR_MEMORY_AND_TIMER
    monitor_memory_and_timer(app_cfg_const.timer_monitor_heap_and_timer_timeout);
#endif

    APP_PRINT_INFO1("TIME FROM APP TO OS SCHED: %d ms", sys_timestamp_get() - time_entry_app);

    os_sched_start();

    return 0;
}

