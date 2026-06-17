/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _IAP_DEMO_TASK_H_
#define _IAP_DEMO_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup IAP_DEMO_TASK IAP DEMO Task
  * @brief IAP DEMO Task
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
#define IAP_DEMO_TASK_PRIORITY             1             //!< Task priorities
#define IAP_DEMO_TASK_STACK_SIZE           1024*2        //!<  Task stack size
#define IAP_DEMO_MAX_NUMBER_OF_GAP_MESSAGE     0x20      //!<  GAP message queue size
#define IAP_DEMO_MAX_NUMBER_OF_IO_MESSAGE      0x20      //!<  IO message queue size
#define IAP_DEMO_MAX_NUMBER_OF_EVENT_MESSAGE   (IAP_DEMO_MAX_NUMBER_OF_GAP_MESSAGE + IAP_DEMO_MAX_NUMBER_OF_IO_MESSAGE)    //!< Event message queue size

/*============================================================================*
 *                              Variables
*============================================================================*/
extern void *iap_demo_evt_queue_handle;  //!< Event queue handle
extern void *iap_demo_io_queue_handle;   //!< IO queue handle

/**
 * @brief  Initialize App task
 * @return void
 */
void iap_demo_task_init(void);

/** @} End of IAP_DEMO_TASK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IAP_DEMO_TASK_H_ */

