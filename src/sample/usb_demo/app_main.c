/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include "os_mem.h"
#include "os_sched.h"
#include "os_msg.h"
#include "os_task.h"
#include "os_sched.h"
#include "app_flags.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "trace.h"
#include "audio.h"
#include "sysm.h"
#include "remote.h"
#include "btm.h"
#include "app_timer.h"
#include "app_main.h"
#include "app_io_msg.h"
#include "board.h"
#include "app_usb.h"

/** @defgroup  USB_DEMO_APP_MAIN App Main
    * @brief Main file to initialize hardware and framework and start task scheduling
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/

#define MAX_NUMBER_OF_GAP_MESSAGE       0x20    //!< indicate BT stack message queue size
#define MAX_NUMBER_OF_IO_MESSAGE        0x40    //!< indicate io queue size, extra 0x20 for data uart
#define MAX_NUMBER_OF_GAP_TIMER         0x10    //!< indicate gap timer queue size
#define MAX_NUMBER_OF_DSP_MSG           0x20    //!< number of dsp message reserved for DSP message handling.
#define MAX_NUMBER_OF_CODEC_MSG         0x20    //!< number of codec message reserved for CODEC message handling.
#define MAX_NUMBER_OF_SYS_MSG           0x20    //!< indicate SYS timer queue size

#define MAX_NUMBER_OF_APP_TIMER_MODULE  0x30    //!< indicate app timer module size

/** indicate rx event queue size*/
#define MAX_NUMBER_OF_RX_EVENT      \
    (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE  +  MAX_NUMBER_OF_DSP_MSG + MAX_NUMBER_OF_CODEC_MSG + MAX_NUMBER_OF_GAP_TIMER + MAX_NUMBER_OF_SYS_MSG)

/*============================================================================*
 *                              Variables
 *============================================================================*/

void *audio_evt_queue_handle;
void *audio_io_queue_handle;

T_APP_DB app_db;

/*============================================================================*
 *                              Functions
 *============================================================================*/
#if GPIO_KEY_EN
extern void key_init(void);
#endif
/**
 * @brief    Contains the initialization of pinmux settings and pad settings
 * @note     All the pinmux settings and pad settings shall be initiated in this function,
 *           but if legacy driver is used, the initialization of pinmux setting and pad setting
 *           should be peformed with the IO initializing.
 * @return   void
 */
static void board_init(void)
{

}
/**
 * @brief    Contains the initialization of peripherals
 * @note     Both new architecture driver and legacy driver initialization method can be used
 * @return   void
 */
static void driver_init(void)
{

}
/**
* @brief framework_init() contains the initialization of audio manager.
* @return void
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
    audio_mgr_set_max_plc_count(3);
}
/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Peripheral APP, thus only one APP task is init here
 * @return   void
 */
static void app_task(void *pvParameters)
{
    uint8_t event;
    driver_init();

    DBG_DIRECT("app_task - unused memory: %d", mem_peek());

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
//                gap_handle_msg(event);
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
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int main(void)
{
    void *app_task_handle;

    APP_PRINT_INFO2("APP COMPILE TIME: [%s - %s]", TRACE_STRING(__DATE__), TRACE_STRING(__TIME__));

    memset(&app_db, 0, sizeof(T_APP_DB));

    APP_PRINT_INFO0("APP START FROM HW_RESET");

    os_msg_queue_create(&audio_io_queue_handle, "ioQ", MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&audio_evt_queue_handle, "evtQ", MAX_NUMBER_OF_RX_EVENT, sizeof(unsigned char));

    app_init_timer(audio_evt_queue_handle, MAX_NUMBER_OF_APP_TIMER_MODULE);

    board_init();
    framework_init();

#if F_APP_USB_HID_SUPPORT & GPIO_KEY_EN
    key_init();
#endif
    app_usb_init();

    os_task_create(&app_task_handle, "app_task", app_task, NULL, 1024 * 3, 1);

    os_sched_start();

    return 0;
}
/** @} */ /* End of group USB_DEMO_MAIN */
