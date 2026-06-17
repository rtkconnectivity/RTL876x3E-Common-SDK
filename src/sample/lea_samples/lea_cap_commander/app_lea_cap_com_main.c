/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "os_sched.h"
#include "os_msg.h"
#include "os_task.h"
#include "trace.h"
#include "gap.h"
#include "sysm.h"
#include "data_uart.h"
#include "user_cmd_parse.h"
#include "app_msg.h"
#include "app_lea_cap_com_main.h"
#include "app_lea_cap_com_user_cmd.h"
#include "app_lea_cap_com_gap.h"
#include "app_lea_cap_com_profile.h"

/** @defgroup  APP_LEA_CAP_COM_MAIN BLE Audio CAP Commander Main
    * @brief Main file to initialize hardware and BT stack and start task scheduling
    * @{
    */
/*============================================================================*
 *                              Macros
 *============================================================================*/
#define APP_TASK_PRIORITY             2         //!< Task priorities
#define APP_TASK_STACK_SIZE           256 * 8   //!<  Task stack size
#define MAX_NUMBER_OF_GAP_MESSAGE     0x20      //!<  GAP message queue size
#define MAX_NUMBER_OF_IO_MESSAGE      0x40      //!<  IO message queue size
#define MAX_NUMBER_OF_EVENT_MESSAGE   (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE)    //!< Event message queue size

/*============================================================================*
 *                              Variables
 *============================================================================*/
void *app_task_handle;   //!< APP Task handle
void *app_evt_queue_handle;  //!< Event queue handle
void *app_io_queue_handle;   //!< IO queue handle

T_APP_DB app_db;

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief    Contains the initialization of pinmux settings and pad settings
 * @note     All the pinmux settings and pad settings shall be initiated in this function,
 *           but if legacy driver is used, the initialization of pinmux setting and pad setting
 *           should be peformed with the IO initializing.
 * @return   void
 */
void board_init(void)
{

}

/**
 * @brief    Contains the initialization of peripherals
 * @note     Both new architecture driver and legacy driver initialization method can be used
 * @return   void
 */
void driver_init(void)
{
    data_uart_init(app_evt_queue_handle, app_io_queue_handle);
    user_cmd_init(&user_cmd_if, "CAP Commander");
}

/**
 * @brief    Contains the power mode settings
 * @return   void
 */
void pwr_mgr_init(void)
{

}

static void framework_init(void)
{
    /* System Manager */
    sys_mgr_init(app_evt_queue_handle);
}

/**
 * @brief    Contains the initialization of pinmux settings and pad settings
 * @note     All the pinmux settings and pad settings shall be initiated in this function,
 *           but if legacy driver is used, the initialization of pinmux setting and pad setting
 *           should be peformed with the IO initializing.
 * @return   void
 */
void app_msg_init(void)
{
    os_msg_queue_create(&app_io_queue_handle, "ioQ", MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&app_evt_queue_handle, "evtQ", MAX_NUMBER_OF_EVENT_MESSAGE, sizeof(uint8_t));
}

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_param    Parameters sending to the task
 * @return       void
 */
void app_main_task(void *p_param)
{
    uint8_t event;

    gap_start_bt_stack(app_evt_queue_handle, app_io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

    driver_init();
    while (true)
    {
        if (os_msg_recv(app_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (EVENT_GROUP(event) == EVENT_GROUP_IO)
            {
                T_IO_MSG io_msg;

                if (os_msg_recv(app_io_queue_handle, &io_msg, 0) == true)
                {
                    if (event == EVENT_IO_TO_APP)
                    {
                        app_handle_io_msg(io_msg);
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

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Audio CAP Commander APP, thus only one APP task is init here
 * @return   void
 */
void task_init(void)
{
    os_task_create(&app_task_handle, "app", app_main_task, 0, APP_TASK_STACK_SIZE,
                   APP_TASK_PRIORITY);
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int main(void)
{
    app_msg_init();
    board_init();
    app_gap_init();
    framework_init();
    app_lea_profile_init();
    pwr_mgr_init();
    task_init();
    os_sched_start();

    return 0;
}
/** @} */ /* End of group APP_LEA_CAP_COM_MAIN */


