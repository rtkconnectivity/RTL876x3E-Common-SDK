/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "rtl876x_pinmux.h"
#include "trace.h"
#include "os_task.h"
#include "os_mem.h"
#include "os_sched.h"
#include "os_msg.h"
#include "gap.h"
#include "gap_br.h"
#include "btm.h"
#include "sysm.h"
#include "remote.h"
#include "audio.h"
#include "app_timer.h"
#include "app_msg.h"
#include "app_audio_io_msg.h"
#include "console_uart.h"
#include "app_audio_demo_console.h"
#include "app_audio_demo_policy.h"
#include "app_audio_demo_passthrough.h"
#include "app_audio_demo_line_in.h"


/** @defgroup  AUDIO_DEMO_MAIN Audio demo Main
    * @brief Main file to initialize hardware and start task scheduling
    * @{
    */

#define AUDIO_DEMO_UART_TX          P3_1
#define AUDIO_DEMO_UART_RX          P3_0

#define BT_AUDIO_DEFAULT_PAGESCAN_WINDOW             0x12
#define BT_AUDIO_DEFAULT_PAGESCAN_INTERVAL           0x800 //0x800
#define BT_AUDIO_DEFAULT_PAGE_TIMEOUT                0x4000
#define BT_AUDIO_DEFAULT_SUPVISIONTIMEOUT            0x1f40 //0x7D00
#define BT_AUDIO_DEFAULT_INQUIRYSCAN_WINDOW          0x12
#define BT_AUDIO_DEFAULT_INQUIRYSCAN_INTERVAL        0x800 //0x1000

#define PLAYBACK_POOL_SIZE                  (13*1024)
#define VOICE_POOL_SIZE                     (2*1024)
#define RECORD_POOL_SIZE                    (1*1024)
#define NOTIFICATION_POOL_SIZE              (8*1024)

#define BT_AUDIO_MAX_NUMBER_OF_GAP_MESSAGE     0x20      //!<  GAP message queue size
#define BT_AUDIO_MAX_NUMBER_OF_IO_MESSAGE      0x20      //!<  IO message queue size
#define BT_AUDIO_MAX_NUMBER_OF_EVENT_MESSAGE   (BT_AUDIO_MAX_NUMBER_OF_GAP_MESSAGE + BT_AUDIO_MAX_NUMBER_OF_IO_MESSAGE)    //!< Event message queue size
#define AUDIO_MAX_NUMBER_OF_APP_TIMER_MODULE   0x10    //!< indicate app timer module size

#define CONSOLE_UART_RX_BUFFER_SIZE         300

void *audio_evt_queue_handle;  //!< Event queue handle
void *audio_io_queue_handle;   //!< IO queue handle

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief    Contains the initialization of framework
 * @return   void
 */
static void framework_init(void)
{
    /* System Manager */
    sys_mgr_init(audio_evt_queue_handle);

    /* RemoteController Manager */
    remote_mgr_init(REMOTE_SESSION_ROLE_SINGLE);

    /* Bluetooth Manager */
    bt_mgr_init();

    /* Audio Manager */
    audio_mgr_init(PLAYBACK_POOL_SIZE, VOICE_POOL_SIZE, RECORD_POOL_SIZE, NOTIFICATION_POOL_SIZE);
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
    Pinmux_Config(AUDIO_DEMO_UART_TX, UART0_TX);
    Pad_Config(AUDIO_DEMO_UART_TX,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(AUDIO_DEMO_UART_RX, UART0_RX);
    Pad_Config(AUDIO_DEMO_UART_RX,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
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
    console_uart_config.uart_rx_pinmux = AUDIO_DEMO_UART_RX;
    console_uart_config.uart_tx_pinmux = AUDIO_DEMO_UART_TX;
    console_uart_config.rx_wake_up_pinmux = AUDIO_DEMO_UART_RX;
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
    console_param.tx_wakeup_pin = AUDIO_DEMO_UART_TX;
    console_param.rx_wakeup_pin = AUDIO_DEMO_UART_RX;

    console_op.init = console_uart_init;
    console_op.tx_wakeup_enable = NULL; //console_uart_tx_wakeup_enable;
    console_op.rx_wakeup_enable = NULL; //console_uart_rx_wakeup_enable;
    console_op.write = console_uart_write;
    console_op.wakeup = console_uart_wakeup;
    console_op.tx_empty = NULL;

    console_init(&console_param, &console_op);
    console_set_mode(CONSOLE_MODE_STRING);
}

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_param    Parameters sending to the task
 * @return       void
 */
void audio_demo_main_task(void *p_param)
{
    uint8_t event;

    gap_start_bt_stack(audio_evt_queue_handle, audio_io_queue_handle,
                       BT_AUDIO_MAX_NUMBER_OF_GAP_MESSAGE);

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
            else if (EVENT_GROUP(event) == EVENT_GROUP_APP)
            {
                app_timer_handle_msg(event);
            }
        }
    }
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in Audio APP, thus only one APP task is init here
 * @return   void
 */
void task_init(void)
{
    void *app_task_handle;

    os_msg_queue_create(&audio_io_queue_handle, "ioQ", BT_AUDIO_MAX_NUMBER_OF_IO_MESSAGE,
                        sizeof(T_IO_MSG));
    os_msg_queue_create(&audio_evt_queue_handle, "evtQ", BT_AUDIO_MAX_NUMBER_OF_EVENT_MESSAGE,
                        sizeof(unsigned char));

    app_init_timer(audio_evt_queue_handle, AUDIO_MAX_NUMBER_OF_APP_TIMER_MODULE);

    os_task_create(&app_task_handle, "audio_demo_main_task", audio_demo_main_task, NULL, 1024 * 3, 1);
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
    app_audio_init();
    app_audio_passthrough_init();
    app_audio_line_in_init();
    app_audio_demo_cmd_register();
    os_sched_start();

    return 0;
}
/** @} */ /* End of group AUDIO_MAIN */


