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
#include "app_bt_pbap_demo_task.h"
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
void *pbap_demo_task_handle;   //!< APP Task handle
void *pbap_demo_evt_queue_handle;  //!< Event queue handle
void *pbap_demo_io_queue_handle;   //!< IO queue handle

/*============================================================================*
 *                              Functions
 *============================================================================*/
void pbap_demo_main_task(void *p_param);

/**
 * @brief  Initialize App task
 * @return void
 */
void pbap_demo_task_init()
{
    os_msg_queue_create(&pbap_demo_io_queue_handle, "ioQ", PBAP_DEMO_MAX_NUMBER_OF_IO_MESSAGE,
                        sizeof(T_IO_MSG));
    os_msg_queue_create(&pbap_demo_evt_queue_handle, "evtQ", PBAP_DEMO_MAX_NUMBER_OF_EVENT_MESSAGE,
                        sizeof(unsigned char));

    os_task_create(&pbap_demo_task_handle, "pbap_demo_main_task", pbap_demo_main_task, 0,
                   PBAP_DEMO_TASK_STACK_SIZE,
                   PBAP_DEMO_TASK_PRIORITY);
}

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_param    Parameters sending to the task
 * @return       void
 */
void pbap_demo_main_task(void *p_param)
{
    uint8_t event;

    APP_PRINT_INFO2("pbap_demo_main_task PBAP_DEMO_TASK_STACK_SIZE:%d PBAP_DEMO_TASK_PRIORITY:%d",
                    PBAP_DEMO_TASK_STACK_SIZE,
                    PBAP_DEMO_TASK_PRIORITY);
    gap_start_bt_stack(pbap_demo_evt_queue_handle, pbap_demo_io_queue_handle,
                       PBAP_DEMO_MAX_NUMBER_OF_GAP_MESSAGE);

    while (true)
    {
        if (os_msg_recv(pbap_demo_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (EVENT_GROUP(event) == EVENT_GROUP_IO)
            {
                T_IO_MSG io_msg;

                if (os_msg_recv(pbap_demo_io_queue_handle, &io_msg, 0) == true)
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
