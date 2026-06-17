/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_ACC_CSIS_H_
#define _APP_LEA_ACC_CSIS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "csis_def.h"
#include "cap.h"
/** @defgroup APP_LEA_CSIS App LE Audio CSIS
  * @brief this file handle CSIS profile related process
  * @{
  */

extern uint8_t csis_sirk[CSIS_SIRK_LEN];

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_LEA_CSIS_Exported_Functions App CSIS Functions
    * @{
    */

/**
     * @brief csis init Paramer assigned by lea profile init.
     * @param p_param \ref T_CAP_INIT_PARAMS
     * @return  void
     */
void app_lea_csis_init(T_CAP_INIT_PARAMS *p_param);

/** @} */ /* End of group APP_LEA_CSIS_Exported_Functions */
/** End of APP_LEA_CSIS
* @}
*/
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
