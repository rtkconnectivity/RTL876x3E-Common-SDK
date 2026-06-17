/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "os_task.h"
#include "os_mem.h"
#include "os_sched.h"
#include "os_msg.h"
#include "trace.h"
#include "rtl876x_pinmux.h"
#include "gap.h"
#include "gap_br.h"
#include "btm.h"
#include "sysm.h"
#include "remote.h"
#include "app_msg.h"
#include "app_io_msg.h"
#include "console_uart.h"
#include "app_bt_spp_demo_console.h"
#include "app_bt_spp_demo_gap.h"
#include "app_bt_spp_demo_app.h"
#include "app_bt_spp_demo_sdp.h"

#define SPP_DEMO_UART_TX          P3_1
#define SPP_DEMO_UART_RX          P3_0

#define SPP_DEMO_DEFAULT_PAGESCAN_WINDOW             0x12
#define SPP_DEMO_DEFAULT_PAGESCAN_INTERVAL           0x800
#define SPP_DEMO_DEFAULT_PAGE_TIMEOUT                0x4000
#define SPP_DEMO_DEFAULT_SUPVISIONTIMEOUT            0x1f40
#define SPP_DEMO_DEFAULT_INQUIRYSCAN_WINDOW          0x12
#define SPP_DEMO_DEFAULT_INQUIRYSCAN_INTERVAL        0x800

#define SPP_DEMO_MAX_NUMBER_OF_GAP_MESSAGE     0x20
#define SPP_DEMO_MAX_NUMBER_OF_IO_MESSAGE      0x20
#define SPP_DEMO_MAX_NUMBER_OF_EVENT_MESSAGE   (SPP_DEMO_MAX_NUMBER_OF_GAP_MESSAGE + SPP_DEMO_MAX_NUMBER_OF_IO_MESSAGE)

#define CONSOLE_UART_RX_BUFFER_SIZE         300

#if SPP_DEMO_SERVER_ROLE
static uint8_t bd_addr_spp_server[6]  = {0x11, 0x22, 0x66, 0x77, 0x77, 0x88};
#else
static uint8_t bd_addr_spp_client[6]  = {0x11, 0x22, 0x55, 0x77, 0x88, 0x99};
#endif

void *spp_demo_evt_queue_handle;
void *spp_demo_io_queue_handle;


static void app_bt_gap_init(void)
{
    uint32_t class_of_device = (uint32_t)(0x18 | (0x04 << 8) | (0x24 << 16));
    uint16_t supervision_timeout = SPP_DEMO_DEFAULT_SUPVISIONTIMEOUT;

    uint16_t link_policy = GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE;

    uint8_t radio_mode = GAP_RADIO_MODE_VISIBLE_CONNECTABLE;
    bool limited_discoverable = false;
    bool auto_accept_acl = false;

    uint8_t pagescan_type = GAP_RADIO_MODE_VISIBLE_CONNECTABLE;
    uint16_t pagescan_interval = SPP_DEMO_DEFAULT_PAGESCAN_INTERVAL;
    uint16_t pagescan_window = SPP_DEMO_DEFAULT_PAGESCAN_WINDOW;
    uint16_t page_timeout = SPP_DEMO_DEFAULT_PAGE_TIMEOUT;

    uint8_t inquiryscan_type = GAP_INQUIRY_SCAN_TYPE_STANDARD;
    uint16_t inquiryscan_window = SPP_DEMO_DEFAULT_INQUIRYSCAN_WINDOW;
    uint16_t inquiryscan_interval = SPP_DEMO_DEFAULT_INQUIRYSCAN_INTERVAL;
    uint8_t inquiry_mode = GAP_INQUIRY_MODE_EXTENDED_RESULT;

    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_GENERAL_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t oob_enable = false;
    uint8_t bt_mode = GAP_BT_MODE_21ENABLED;

    gap_lib_init();

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

#if SPP_DEMO_SERVER_ROLE
    gap_set_bd_addr(bd_addr_spp_server);
    gap_br_set_param(GAP_BR_PARAM_NAME, GAP_DEVICE_NAME_LEN, "spp server");
    gap_br_cfg_accept_role(0);
#else
    gap_set_bd_addr(bd_addr_spp_client);
    gap_br_set_param(GAP_BR_PARAM_NAME, GAP_DEVICE_NAME_LEN, "spp client");
    gap_br_cfg_accept_role(1);
#endif
}

static void framework_init(void)
{
    sys_mgr_init(spp_demo_evt_queue_handle);
    remote_mgr_init(REMOTE_SESSION_ROLE_SINGLE);
    bt_mgr_init();
}

void board_init(void)
{
    Pinmux_Config(SPP_DEMO_UART_TX, UART0_TX);
    Pad_Config(SPP_DEMO_UART_TX,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(SPP_DEMO_UART_RX, UART0_RX);
    Pad_Config(SPP_DEMO_UART_RX,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

static void driver_init(void)
{
    T_CONSOLE_PARAM console_param;
    T_CONSOLE_OP    console_op;
    T_CONSOLE_UART_CONFIG console_uart_config;

    console_uart_config.one_wire_uart_support = 0;
    console_uart_config.uart_rx_pinmux = SPP_DEMO_UART_RX;
    console_uart_config.uart_tx_pinmux = SPP_DEMO_UART_TX;
    console_uart_config.rx_wake_up_pinmux = SPP_DEMO_UART_RX;
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
    console_param.tx_wakeup_pin = SPP_DEMO_UART_TX;
    console_param.rx_wakeup_pin = SPP_DEMO_UART_RX;

    console_op.init = console_uart_init;
    console_op.tx_wakeup_enable = NULL;
    console_op.rx_wakeup_enable = NULL;
    console_op.write = console_uart_write;
    console_op.wakeup = console_uart_wakeup;
    console_op.tx_empty = NULL;

    console_init(&console_param, &console_op);
    console_set_mode(CONSOLE_MODE_STRING);
}

void spp_demo_main_task(void *p_param)
{
    uint8_t event;

    gap_start_bt_stack(spp_demo_evt_queue_handle, spp_demo_io_queue_handle,
                       SPP_DEMO_MAX_NUMBER_OF_GAP_MESSAGE);

    while (true)
    {
        if (os_msg_recv(spp_demo_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (EVENT_GROUP(event) == EVENT_GROUP_IO)
            {
                T_IO_MSG io_msg;

                if (os_msg_recv(spp_demo_io_queue_handle, &io_msg, 0) == true)
                {
                    if (event == EVENT_IO_TO_APP)
                    {
                        app_io_handle_msg(io_msg);
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
        }
    }
}

void task_init(void)
{
    void *app_task_handle;

    os_msg_queue_create(&spp_demo_io_queue_handle, "ioQ", SPP_DEMO_MAX_NUMBER_OF_IO_MESSAGE,
                        sizeof(T_IO_MSG));
    os_msg_queue_create(&spp_demo_evt_queue_handle, "evtQ", SPP_DEMO_MAX_NUMBER_OF_EVENT_MESSAGE,
                        sizeof(unsigned char));

    os_task_create(&app_task_handle, "spp_demo_main_task", spp_demo_main_task, NULL, 1024 * 3, 1);
}

int main(void)
{
    board_init();
    driver_init();
    task_init();
    framework_init();
    app_bt_gap_init();
    spp_demo_gap_init();
    spp_demo_sdp_init();
    spp_demo_app_init();
    app_spp_demo_cmd_register();

    os_sched_start();

    return 0;
}
