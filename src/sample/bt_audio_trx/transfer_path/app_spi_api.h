/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SPI_API_H_
#define _APP_SPI_API_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "string.h"
#include "rtl876x.h"
#include "btm.h"

#define SPI_XMIT_SIZE                   700
#define SPI_CMD_SYNC_INVALID            0xFF
typedef enum
{
    SPI_M2S_CMD,
    SPI_M2S_AUDIO,
    SPI_S2M_CMD,
    SPI_S2M_AUDIO,
} T_SPI_DATA_TYPE;

typedef enum
{
    SPI_M2S_AUDIO_A2DP,
    SPI_M2S_AUDIO_SCO,
    SPI_S2M_AUDIO_SCO,
} T_SPI_AUDIO_TYPE;

typedef enum
{
    SPI_M2S_CMD_A2DP_CONFIG,
    SPI_M2S_CMD_SCO_CONFIG,
    SPI_S2M_CMD_SCO_CONFIG,
} T_SPI_CMD_TYPE;

typedef struct t_a2dp_sbc_pkt
{
    uint8_t frame_num;
    uint16_t pkt_len;
} T_A2DP_SBC_PKT;

typedef enum
{
    SPI_SEND_SUC,
    SPI_SEND_ERR_LEN,
    SPI_SEND_ERR_BUSY,
} T_SPI_SEND_ERR_CODE;

void app_spi_master_init(void);
void app_spi_master_cmd_set_parser(void);
uint8_t app_spi_master_send_data(uint16_t cmd_id, uint8_t *p_data, uint16_t len);

void app_spi_slave_init(void);
void app_spi_slave_cmd_set_parser(void);
uint8_t app_spi_slave_send_data(uint16_t cmd_id, uint8_t *p_data, uint16_t len);

void app_spi_master_enter_low_power(void);
void app_spi_master_exit_low_power(void);
void app_spi_slave_enter_low_power(void);
void app_spi_slave_exit_low_power(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SPI_API_H_ */
