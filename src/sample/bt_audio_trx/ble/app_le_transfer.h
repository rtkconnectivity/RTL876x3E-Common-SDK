/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LE_TRANSFER_H_
#define _APP_LE_TRANSFER_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#if TRANSMIT_CLIENT_SUPPORT

typedef enum
{
    LE_TRANSFER_CONTROL_VOLUME_UP,
    LE_TRANSFER_CONTROL_VOLUME_DOWN,
} T_LE_TRANSFER_CONTROL_ACTION;

void app_le_transfer_volume_control(uint8_t action);

void app_le_transfer_control_cmd_handle(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                        uint16_t cmd_len, uint8_t *ack_pkt);
void app_le_transfer_send_battery(uint8_t level);

void app_le_transfer_send_ack(uint16_t event_id, uint8_t result);


/**
 * @brief app transmit_service client init
 *
 */
void app_le_transfer_init(void);
#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif

