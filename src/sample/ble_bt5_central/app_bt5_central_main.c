/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "os_mem.h"
#include "os_sched.h"
#include "trace.h"
#include "gap_msg.h"
#include "app_bt5_central_task.h"
#include "app_bt5_central_gap.h"

/** @defgroup  BT5_CENTRAL_DEMO_MAIN BT5 Central Main
    * @brief Main file to initialize hardware and BT stack and start task scheduling
    * @{
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief board_init
          Contains the initialization of pinmux settings and pad settings
 * @note  All the pinmux settings and pad settings shall be initiated in this function,
 *        but if legacy driver is used, the initialization of pinmux setting and pad setting
 *        should be peformed with the IO initializing.
 * @return void
 */
void board_init(void)
{

}

/**
 * @brief driver_init
          Contains the initialization of peripherals
 * @note  Both new architecture driver and legacy driver initialization method can be used
 * @return void
 */
void driver_init(void)
{

}

/**
 * @brief pwr_mgr_init
          Contains the power mode settings
 * @return void
 */
void pwr_mgr_init(void)
{
}

/**
 * @brief task_init
          Contains the initialization of all tasks
 * @note  There is only one task in BT5 BLE Central APP, thus only one APP task is init here
 * @return void
 */
void task_init(void)
{
    app_task_init();
}

/**
 * @brief Entry of APP code
 * @return int (To avoid compile warning)
 */
int main(void)
{
    board_init();
    le_gap_init(APP_MAX_LINKS);
    gap_lib_init();
    app_gap_init();
    pwr_mgr_init();
    task_init();
    os_sched_start();

    return 0;
}
/** @} */ /* End of group BT5_CENTRAL_DEMO_MAIN */
