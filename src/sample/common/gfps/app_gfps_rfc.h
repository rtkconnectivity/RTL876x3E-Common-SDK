/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_GFPS_RFC_H_
#define _APP_GFPS_RFC_H_

#include "bt_rfc.h"
#include "stdint.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_RWS_GFPS App Gfps
  * @brief App Gfps
  * @{
  */

void app_gfps_rfc_cback(uint8_t *bd_addr, T_BT_RFC_MSG_TYPE msg_type, void *msg_buf);

void app_gfps_rfc_sdp_record_add(void);
/** End of APP_GFPS_RFC
* @}
*/

/** End of APP_RWS_GFPS
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_GFPS_RFC_H_ */


