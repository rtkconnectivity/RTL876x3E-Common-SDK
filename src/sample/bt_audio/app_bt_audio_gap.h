/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _BT_AUDIO_GAP_H_
#define _BT_AUDIO_GAP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup BT_AUDIO_GAP BT Audio Gap
  * @brief BT Audio Gap
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
void app_bt_audio_gap_local_addr_set(uint8_t *bd_addr);

void app_bt_audio_gap_remote_addr_set(uint8_t *bd_addr);

bool app_bt_audio_gap_local_bt_name_set(uint8_t *p_name, uint8_t  len);

bool app_bt_audio_gap_inquiry_start(void);

bool app_bt_audio_gap_device_mode_set(uint8_t value);

void app_bt_audio_gap_init(void);

/** @} End of BT_AUDIO_GAP_Exported_Functions */

/** @} End of BT_AUDIO_GAP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_AUDIO_GAP_H_ */
