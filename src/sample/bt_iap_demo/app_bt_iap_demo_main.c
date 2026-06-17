/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "rtl876x_pinmux.h"
#include "gap.h"
#include "gap_br.h"
#include "os_mem.h"
#include "os_sched.h"
#include "btm.h"
#include "sysm.h"
#include "remote.h"
#include "console_uart.h"
#include "app_bt_iap_demo_task.h"
#include "app_bt_iap_demo_gap.h"
#include "app_bt_iap_demo_app.h"
#include "app_bt_iap_demo_sdp.h"
#include "app_bt_iap_demo_console.h"
#if F_DLPS_EN
#include "pm.h"
#endif

/** @defgroup  IAP_DEMO_MAIN iAP Demo Main
    * @brief Main file to initialize hardware and BT stack and start task scheduling
    * @{
    */

#define IAP_DEMO_DEFAULT_PAGESCAN_WINDOW             0x12
#define IAP_DEMO_DEFAULT_PAGESCAN_INTERVAL           0x800 //0x800
#define IAP_DEMO_DEFAULT_PAGE_TIMEOUT                0x4000
#define IAP_DEMO_DEFAULT_SUPVISIONTIMEOUT            0x1f40 //0x7D00
#define IAP_DEMO_DEFAULT_INQUIRYSCAN_WINDOW          0x12
#define IAP_DEMO_DEFAULT_INQUIRYSCAN_INTERVAL        0x800 //0x1000

#define PLAYBACK_POOL_SIZE                  (13*1024)
#define VOICE_POOL_SIZE                     (2*1024)
#define RECORD_POOL_SIZE                    (1*1024)
#define NOTIFICATION_POOL_SIZE              (8*1024)

#define IAP_DEMO_UART_TX          P3_1
#define IAP_DEMO_UART_RX          P3_0
#define IAP_DEMO_CP_DAT           P0_2
#define IAP_DEMO_CP_CLK           P0_3

#define CONSOLE_UART_RX_BUFFER_SIZE         300

const uint8_t local_bd_addr[6]  = {0x11, 0x22, 0x56, 0x34, 0x56, 0x77};
const char *local_name = "iAP demo app";

/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
  * @brief  Initialize gap bond manager related parameters
  * @return void
  */
static void app_bt_gap_init(void)
{
    uint32_t class_of_device = (uint32_t)(0x18 | (0x04 << 8) | (0x24 << 16));
    uint16_t supervision_timeout = IAP_DEMO_DEFAULT_SUPVISIONTIMEOUT;

    uint16_t link_policy = GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE;

    uint8_t radio_mode = GAP_RADIO_MODE_NONE_DISCOVERABLE;
    bool limited_discoverable = false;
    bool auto_accept_acl = false;

    uint8_t pagescan_type = GAP_PAGE_SCAN_TYPE_INTERLACED;
    uint16_t pagescan_interval = IAP_DEMO_DEFAULT_PAGESCAN_INTERVAL;
    uint16_t pagescan_window = IAP_DEMO_DEFAULT_PAGESCAN_WINDOW;
    uint16_t page_timeout = IAP_DEMO_DEFAULT_PAGE_TIMEOUT;

    uint8_t inquiryscan_type = GAP_INQUIRY_SCAN_TYPE_STANDARD;
    uint16_t inquiryscan_window = IAP_DEMO_DEFAULT_INQUIRYSCAN_WINDOW;
    uint16_t inquiryscan_interval = IAP_DEMO_DEFAULT_INQUIRYSCAN_INTERVAL;
    uint8_t inquiry_mode = GAP_INQUIRY_MODE_EXTENDED_RESULT;

    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_GENERAL_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t oob_enable = false;
    uint8_t bt_mode = GAP_BT_MODE_21ENABLED;

    gap_lib_init();

    gap_br_set_param(GAP_BR_PARAM_NAME, strlen((const char *)local_name) + 1, (void *)local_name);
    gap_set_bd_addr((uint8_t *)local_bd_addr);
    gap_br_cfg_accept_role(0);

    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);

    gap_br_set_param(GAP_BR_PARAM_BT_MODE, sizeof(uint8_t), &bt_mode);
    gap_br_set_param(GAP_BR_PARAM_COD, sizeof(uint32_t), &class_of_device);
    gap_br_set_param(GAP_BR_PARAM_LINK_POLICY, sizeof(uint16_t), &link_policy);
    gap_br_set_param(GAP_BR_PARAM_SUPV_TOUT, sizeof(uint16_t), &supervision_timeout);
    gap_br_set_param(GAP_BR_PARAM_AUTO_ACCEPT_ACL, sizeof(bool), &auto_accept_acl);


    gap_br_set_param(GAP_BR_PARAM_RADIO_MODE, sizeof(uint8_t), &radio_mode);
    gap_br_set_param(GAP_BR_PARAM_LIMIT_DISCOV, sizeof(bool), &limited_discoverable);

    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_TYPE, sizeof(uint8_t), &pagescan_type);
    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_INTERVAL, sizeof(uint16_t), &pagescan_interval);
    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_WINDOW, sizeof(uint16_t), &pagescan_window);
    gap_br_set_param(GAP_BR_PARAM_PAGE_TIMEOUT, sizeof(uint16_t), &page_timeout);

    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_TYPE, sizeof(uint8_t), &inquiryscan_type);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_INTERVAL, sizeof(uint16_t), &inquiryscan_interval);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_WINDOW, sizeof(uint16_t), &inquiryscan_window);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_MODE, sizeof(uint8_t), &inquiry_mode);
}

/**
 * @brief    Contains the initialization of framework
 * @return   void
 */
static void framework_init(void)
{
    /* System Manager */
    sys_mgr_init(iap_demo_evt_queue_handle);

    /* RemoteController Manager */
    remote_mgr_init(REMOTE_SESSION_ROLE_SINGLE);

    /* Bluetooth Manager */
    bt_mgr_init();
}

/**
 * @brief    Contains the initialization of pinmux settings and pad settings
 * @note     All the pinmux settings and pad settings shall be initiated in this function,
 *           but if legacy driver is used, the initialization of pinmux setting and pad setting
 *           should be peformed with the IO initializing.
 * @return   void
 */
void board_init(void)
{
    Pinmux_Config(IAP_DEMO_UART_TX, UART0_TX);
    Pad_Config(IAP_DEMO_UART_TX,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(IAP_DEMO_UART_RX, UART0_RX);
    Pad_Config(IAP_DEMO_UART_RX,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(IAP_DEMO_CP_DAT, I2C0_DAT);
    //Pad_Config(IAP_DEMO_CP_DAT,
    //PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(IAP_DEMO_CP_CLK, I2C0_CLK);
    //Pad_Config(IAP_DEMO_CP_CLK,
    //PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pad_Config(IAP_DEMO_CP_DAT,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(IAP_DEMO_CP_CLK,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_PullConfigValue(IAP_DEMO_CP_DAT, PAD_STRONG_PULL);
    Pad_PullConfigValue(IAP_DEMO_CP_CLK, PAD_STRONG_PULL);
}

/**
 * @brief    Contains the initialization of peripherals
 * @note     Both new architecture driver and legacy driver initialization method can be used
 * @return   void
 */
static void driver_init(void)
{
    T_CONSOLE_PARAM console_param;
    T_CONSOLE_OP    console_op;
    T_CONSOLE_UART_CONFIG console_uart_config;

    console_uart_config.one_wire_uart_support = 0;
    console_uart_config.uart_rx_pinmux = IAP_DEMO_UART_RX;
    console_uart_config.uart_tx_pinmux = IAP_DEMO_UART_TX;
    console_uart_config.rx_wake_up_pinmux = IAP_DEMO_UART_RX;
    console_uart_config.enable_rx_wake_up = 0;
    console_uart_config.data_uart_baud_rate = BAUD_RATE_115200;
    console_uart_config.callback = NULL;
    console_uart_config.uart_dma_rx_buffer_size = CONSOLE_UART_RX_BUFFER_SIZE;

#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
    console_uart_config.uart_rx_dma_enable = true;
#else
    console_uart_config.uart_rx_dma_enable = false;
#endif
    console_uart_config.uart_tx_dma_enable = false;

    console_uart_config_init(&console_uart_config);

    console_param.tx_buf_size   = 512;
    console_param.rx_buf_size   = 512;
    console_param.tx_wakeup_pin = IAP_DEMO_UART_TX;
    console_param.rx_wakeup_pin = IAP_DEMO_UART_RX;

    console_op.init = console_uart_init;
    console_op.tx_wakeup_enable = NULL; //console_uart_tx_wakeup_enable;
    console_op.rx_wakeup_enable = NULL; //console_uart_rx_wakeup_enable;
    console_op.write = console_uart_write;
    console_op.wakeup = console_uart_wakeup;
    console_op.tx_empty = NULL;

    console_init(&console_param, &console_op);
    console_set_mode(CONSOLE_MODE_STRING);

    app_iap_cp_hw_init();
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Peripheral APP, thus only one APP task is init here
 * @return   void
 */
void task_init(void)
{
    iap_demo_task_init();
}

/**
 * @brief    Contains the power mode settings
 * @return   void
 */
void pwr_mgr_init(void)
{
#if F_DLPS_EN
    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    power_mode_set(POWER_DLPS_MODE);
#endif
}


/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int main(void)
{
    board_init();
    driver_init();
    task_init();

    framework_init();
    app_bt_gap_init();
    iap_demo_gap_init();
    iap_demo_sdp_init();
    iap_demo_app_init();
    app_iap_cmd_register();

    os_sched_start();

    return 0;
}
/** @} */ /* End of group IAP_DEMO_MAIN */


