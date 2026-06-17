/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if (F_APP_SPI_ROLE_MASTER || F_APP_SPI_ROLE_SLAVE)
#include "app_spi_cmd.h"
#include "trace.h"
#include "app_spi_api.h"
#include "app_cmd.h"

void app_spi_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                            uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));;

    APP_PRINT_TRACE3("app_spi_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);

    switch (cmd_id)
    {
    case CMD_SPI_INIT:
        {
            uint8_t role = cmd_ptr[2];
            if (role != SPI_ROLE_MASTER && role != SPI_ROLE_SLAVE)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

#if F_APP_SPI_ROLE_MASTER
            if (role == SPI_ROLE_MASTER)
            {
                app_spi_master_init();
            }
#endif
#if F_APP_SPI_ROLE_SLAVE
            if (role == SPI_ROLE_SLAVE)
            {
                app_spi_slave_init();
            }
#endif
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        break;
    }
}
#endif

