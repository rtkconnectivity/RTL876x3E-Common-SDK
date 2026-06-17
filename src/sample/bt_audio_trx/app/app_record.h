/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CMD_RECORD_H_
#define _APP_CMD_RECORD_H_


#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
  *                           Header Files
  *============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include "app_cmd.h"

/** @defgroup _XIAOMI_MIC_RECORD_H_
  * @brief
  * @{
  */

bool app_record_play_volume_set(uint8_t volume);

uint8_t app_record_play_volume_get(void);

void app_record_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr,
                           uint16_t cmd_len, uint8_t *ack_pkt);

void app_record_init(void);


/** @} End of _XIAOMI_MIC_RECORD_H_ */

#ifdef __cplusplus
}
#endif

#endif //_XIAOMI_MIC_RECORD_H_
