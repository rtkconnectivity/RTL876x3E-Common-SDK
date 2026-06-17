/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "section.h"
#include "fmc_api.h"
#include "gap_le.h"
#include "os_sched.h"
#include "app_task.h"
#include "dtm_app.h"

/** @defgroup  DTM_MAIN Direct Test Mode Main
  * @brief Main file to initialize parameters and register application message callback.
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
  * @brief  register gap message callback
  * @return void
  */
void app_le_gap_init(void)
{
    le_register_app_cb(app_gap_callback);
}

/**
  * @brief    Entry of APP code
  * @return   int (To avoid compile warning)
  */
int main(void)
{
    le_gap_init(0);
    gap_lib_init();
    app_le_gap_init();
    application_task_init();
    os_sched_start();

    return 0;
}

/** @} */ /* End of group DTM_MAIN */
