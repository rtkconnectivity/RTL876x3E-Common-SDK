/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_ACC_USR_H_
#define _APP_LEA_ACC_USR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "bt_direct_msg.h"
#include "app_dsp_cfg.h"
#include "app_link_util_cs.h"

/** @defgroup APP_LEA_UMR App LE Audio UMR
  * @brief this file handle Unicast Audio related process
  * @{
  */


/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_LEA_UMR_Exported_Functions App LE Audio Unicast Audio Functions
    * @{
    */

void app_lea_uca_init(void);
void app_lea_uca_link_sm(uint16_t conn_handle, uint8_t event, void *p_data);
void app_lea_uca_set_sidetone_instance(T_APP_DSP_CFG_SIDETONE info);
T_AUDIO_EFFECT_INSTANCE *app_lea_uca_p_eq_instance(void);
T_AUDIO_EFFECT_INSTANCE app_lea_uca_get_eq_instance(void);
bool *app_lea_uca_get_eq_abled(void);
bool app_lea_uca_get_diff_path(void);
void app_lea_uca_handle_iso_data(T_BT_DIRECT_CB_DATA *p_data);
T_APP_LE_LINK *app_lea_uca_get_stream_link(void);
/** @} */ /* End of group APP_LEA_UMR_Exported_Functions */
/** End of APP_LEA_UMR
* @}
*/

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
