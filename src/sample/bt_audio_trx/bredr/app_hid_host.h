/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_HID_HOST_H_
#define _APP_HID_HOST_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_HID_HOST App Hid
  * @brief App Hid HOST
  * @{
  */

void app_hid_host_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                 uint16_t cmd_len, uint8_t *ack_pkt);
/**
    * @brief  hid host module init
    * @param  void
    * @return void
    */
void app_hid_host_init(void);

/** End of APP_HID_HOST
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_HID_HOST_H_ */
