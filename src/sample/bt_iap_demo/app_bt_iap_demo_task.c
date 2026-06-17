/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <os_msg.h>
#include <os_task.h>
#include "trace.h"
#include "gap.h"
#include "sysm.h"
#include "app_bt_iap_demo_task.h"
#include "app_msg.h"
#include "app_io_msg.h"

/**
 * @brief This file handles the implementation of application task related functions.
 *
 * Create App task and handle events & messages
 * @{
 */

/*============================================================================*
 *                              Variables
 *============================================================================*/
void *iap_demo_task_handle;   //!< APP Task handle
void *iap_demo_evt_queue_handle;  //!< Event queue handle
void *iap_demo_io_queue_handle;   //!< IO queue handle

/*============================================================================*
 *                              Functions
 *============================================================================*/
void iap_demo_main_task(void *p_param);

/**
 * @brief  Initialize App task
 * @return void
 */
void iap_demo_task_init()
{
    os_msg_queue_create(&iap_demo_io_queue_handle, "ioQ", IAP_DEMO_MAX_NUMBER_OF_IO_MESSAGE,
                        sizeof(T_IO_MSG));
    os_msg_queue_create(&iap_demo_evt_queue_handle, "evtQ", IAP_DEMO_MAX_NUMBER_OF_EVENT_MESSAGE,
                        sizeof(unsigned char));

    os_task_create(&iap_demo_task_handle, "bt_iap_demo_main_task", iap_demo_main_task, 0,
                   IAP_DEMO_TASK_STACK_SIZE,
                   IAP_DEMO_TASK_PRIORITY);
}

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_param    Parameters sending to the task
 * @return       void
 */
void iap_demo_main_task(void *p_param)
{
    uint8_t event;

    APP_PRINT_INFO2("iap_demo_main_task IAP_DEMO_TASK_STACK_SIZE:%d IAP_DEMO_TASK_PRIORITY:%d",
                    IAP_DEMO_TASK_STACK_SIZE,
                    IAP_DEMO_TASK_PRIORITY);
    gap_start_bt_stack(iap_demo_evt_queue_handle, iap_demo_io_queue_handle,
                       IAP_DEMO_MAX_NUMBER_OF_GAP_MESSAGE);

    while (true)
    {
        if (os_msg_recv(iap_demo_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (EVENT_GROUP(event) == EVENT_GROUP_IO)
            {
                T_IO_MSG io_msg;

                if (os_msg_recv(iap_demo_io_queue_handle, &io_msg, 0) == true)
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
