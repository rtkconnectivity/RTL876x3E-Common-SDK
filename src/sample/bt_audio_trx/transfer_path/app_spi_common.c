/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if (F_APP_SPI_ROLE_MASTER || F_APP_SPI_ROLE_SLAVE)
#include "app_spi_common.h"
#include "app_spi_api.h"
#include "app_dlps.h"
#include "trace.h"

static uint8_t app_spi_xmit_status_bitmask = 0;

void app_spi_disable_dlps(uint8_t bit)
{
    app_spi_xmit_status_bitmask |= bit;
    app_dlps_disable(APP_DLPS_ENTER_CHECK_SPI);
}

void app_spi_enable_dlps(uint8_t bit)
{
    app_spi_xmit_status_bitmask &= ~bit;
    if (app_spi_xmit_status_bitmask == 0)
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_SPI);
    }
}

bool app_spi_is_transmit_done(void)
{
    return (app_spi_xmit_status_bitmask == 0) ? true : false;
}

bool app_spi_msg_send(IO_SPI_MSG_TYPE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type = IO_MSG_TYPE_AUDIO_TRANS_SPI;
    msg.subtype = subtype;
    msg.u.buf = param_buf;

    return app_io_msg_send(&msg);
}

void app_spi_msg_handle(T_IO_MSG io_msg)
{
    IO_SPI_MSG_TYPE sub_type = (IO_SPI_MSG_TYPE)io_msg.subtype;
    APP_PRINT_INFO1("app_spi_msg_handle: subtype %d", sub_type);
    switch (sub_type)
    {
    case IO_SPI_MASTER_DATA_IN:
        {
#if F_APP_SPI_ROLE_MASTER
            app_spi_master_cmd_set_parser();
#endif
        }
        break;

    case IO_SPI_SLAVE_DATA_IN:
        {
#if F_APP_SPI_ROLE_SLAVE
            app_spi_slave_cmd_set_parser();
#endif
        }
        break;

    case IO_SPI_SLAVE_TRIGGER:
        {
#if F_APP_SPI_ROLE_SLAVE
            extern void app_spi_slave_dma_trigger(void);
            app_spi_slave_dma_trigger();
#endif
        }
        break;

    default:
        break;
    }
}

#endif
