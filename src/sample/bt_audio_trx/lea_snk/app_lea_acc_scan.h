/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LE_AUDIO_SCAN_H_
#define _APP_LE_AUDIO_SCAN_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "gap_callback_le.h"

/** @defgroup APP_LEA_SCAN App LE Audio SCAN
  * @brief this file handle lea scan related process
  * @{
  */

typedef struct
{
    uint8_t adv_addr[6];
    uint8_t adv_addr_type;
    uint8_t advertiser_sid;
    uint8_t broadcast_id[3];
} T_LEA_BRS_INFO;

void app_lea_acc_scan_start(void);
void app_lea_acc_scan_stop(void);
void app_lea_scan_init(void);

/** @} */ /* End of group APP_LEA_SCAN_Exported_Functions */
/** End of APP_LEA_SCAN
* @}
*/
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
