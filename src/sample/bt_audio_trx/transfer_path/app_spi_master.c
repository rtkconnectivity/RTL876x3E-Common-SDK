/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_SPI_ROLE_MASTER
#include "app_spi_api.h"
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "vector_table.h"
#include "dma_channel.h"
#include "trace.h"
#include "pm.h"
#include "io_debug.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"
#include "section.h"

#include "stdlib.h"

#include "app_dlps.h"
#include "app_pp_buffer.h"
#include "app_spi_common.h"
#include "app_util.h"
#include "app_cmd.h"
#include "app_msg.h"
#include "app_timer.h"
#include "board.h"

static uint8_t sendbuf[SPI_XMIT_SIZE];

static uint8_t spi_tx_seq = 0;
static uint8_t spi_tx_dma_ch_num = 0xa5;
static uint8_t spi_rx_dma_ch_num = 8;
static bool spi_master_init_flag = false;
static bool app_spi_master_idle = true;

#define SPI_TX_DMA_CHANNEL_NUM     spi_tx_dma_ch_num
#define SPI_TX_DMA_CHANNEL         DMA_CH_BASE(spi_tx_dma_ch_num)
#define SPI_TX_DMA_IRQ             DMA_CH_IRQ(spi_tx_dma_ch_num)
#define SPI_RX_DMA_CHANNEL_NUM     spi_rx_dma_ch_num
#define SPI_RX_DMA_CHANNEL         DMA_CH_BASE(spi_rx_dma_ch_num)
#define SPI_RX_DMA_IRQ             DMA_CH_IRQ(spi_rx_dma_ch_num)
#define SPI_RX_DMA_VECTOR          DMA_CH_VECTOR(spi_rx_dma_ch_num)

static T_PP_BUFFER ppbuf_mgr;

static void app_spi_master_tx_dma_handler(void);
static void app_spi_master_rx_dma_handler(void);

void app_spi_master_dma_trigger(void);

uint8_t app_spi_master_send_data(uint16_t cmd_id, uint8_t *p_data, uint16_t len)
{
    spi_tx_seq++;
    if (spi_tx_seq == 0)
    {
        spi_tx_seq = 1;
    }
    APP_PRINT_INFO1("app_spi_master_send_data: spi_tx_seq 0x%x", spi_tx_seq);

    if (!app_spi_master_idle)
    {
        return SPI_SEND_ERR_BUSY;
    }

    app_spi_master_idle = false;

    uint16_t total_len = len + 6;
    if (p_data != NULL)
    {
        if (len == 0 || total_len > SPI_XMIT_SIZE)
        {
            return SPI_SEND_ERR_LEN;
        }
    }

    memset(sendbuf, 0, SPI_XMIT_SIZE);

    sendbuf[0] = CMD_SYNC_BYTE;
    sendbuf[1] = spi_tx_seq;
    sendbuf[2] = (uint8_t)(len + 2);
    sendbuf[3] = (uint8_t)((len + 2) >> 8);
    sendbuf[4] = (uint8_t)cmd_id;
    sendbuf[5] = (uint8_t)(cmd_id >> 8);

    if (p_data != NULL)
    {
        memcpy(&sendbuf[6], p_data, len);
    }

    sendbuf[SPI_XMIT_SIZE - 1] = app_util_calc_checksum(&sendbuf[1], SPI_XMIT_SIZE - 2);

    app_spi_master_dma_trigger();

    return SPI_SEND_SUC;
}

void app_spi_master_cmd_set_parser(void)
{
    uint8_t err_code = 0;
    uint8_t check_sum = 0;
    uint16_t cmd_len = 0;
    uint8_t rx_seq = 0;
    uint8_t *p_data = NULL;
    void *rBuf = NULL;

    if (!app_pp_buffer_get_read_buf(&ppbuf_mgr, &rBuf))
    {
        err_code = 1;
        goto ERR;
    }

    p_data = malloc(SPI_XMIT_SIZE);
    if (p_data == NULL)
    {
        err_code = 2;
        goto ERR;
    }

    memcpy(p_data, rBuf, SPI_XMIT_SIZE);
    app_pp_buffer_set_read_done(&ppbuf_mgr);

    if (p_data[0] == SPI_CMD_SYNC_INVALID)
    {
        err_code = 5;
        goto ERR;
    }

    if (p_data[0] != CMD_SYNC_BYTE)
    {
        err_code = 3;
        goto ERR;
    }

    check_sum = app_util_calc_checksum(&p_data[1], SPI_XMIT_SIZE - 2);
    if (p_data[SPI_XMIT_SIZE - 1] != check_sum)
    {
        err_code = 4;
        goto ERR;
    }
    rx_seq = p_data[1];
    cmd_len = (uint16_t)(p_data[2] | (p_data[3] << 8));
    app_handle_cmd_set(&p_data[4], cmd_len, CMD_PATH_SPI, rx_seq, 0); //from cmd_id

    free(p_data);
    return;
ERR:
    APP_PRINT_ERROR1("app_spi_master_cmd_set_parser: err_code -%d", err_code);
    if (p_data != NULL)
    {
        free(p_data);
    }
}

static void app_spi_master_board_init(void)
{
    Pinmux_Config(PIN_MASTER_SPI0_SCK, SPI0_CLK_MASTER);
    Pinmux_Config(PIN_MASTER_SPI0_MOSI, SPI0_MO_MASTER);
    Pinmux_Config(PIN_MASTER_SPI0_MISO, SPI0_MI_MASTER);
    Pinmux_Config(PIN_MASTER_SPI0_CS, SPI0_SS_N_0_MASTER);
    Pad_Config(PIN_MASTER_SPI0_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_MASTER_SPI0_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_MASTER_SPI0_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_MASTER_SPI0_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
}

static void app_spi_master_pad_pull_up(void)
{
    Pad_Config(PIN_MASTER_SPI0_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_MASTER_SPI0_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_MASTER_SPI0_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_MASTER_SPI0_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
}

static void app_spi_master_driver_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 10;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_TxDmaEn             = ENABLE;
    SPI_InitStructure.SPI_RxDmaEn             = ENABLE;
    SPI_InitStructure.SPI_TxWaterlevel        = 46;
    SPI_InitStructure.SPI_RxWaterlevel        = 31;
    SPI_Init(SPI0, &SPI_InitStructure);
    SPI_Cmd(SPI0, ENABLE);
}

static void app_spi_master_dma_tx_init(void)
{
    sendbuf[0] = SPI_CMD_SYNC_INVALID;
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_TX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToPeripheral;//GDMA_DIR_MemoryToPeripheral
    GDMA_InitStruct.GDMA_BufferSize      = SPI_XMIT_SIZE;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_16;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_16;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)sendbuf;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)SPI0->DR;
    GDMA_InitStruct.GDMA_DestHandshake = GDMA_Handshake_SPI0_TX;
    GDMA_Init(SPI_TX_DMA_CHANNEL, &GDMA_InitStruct);
    GDMA_INTConfig(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_TX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

static void app_spi_master_dma_rx_init(void *readbuf)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;//GDMA_DIR_PeripheralToMemory
    GDMA_InitStruct.GDMA_BufferSize      = SPI_XMIT_SIZE;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)SPI0->DR;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(readbuf);
    GDMA_InitStruct.GDMA_SourceHandshake = GDMA_Handshake_SPI0_RX;
    GDMA_Init(SPI_RX_DMA_CHANNEL, &GDMA_InitStruct);
    RamVectorTableUpdate(SPI_RX_DMA_VECTOR, app_spi_master_rx_dma_handler);
    GDMA_INTConfig(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

void app_spi_master_dma_trigger(void)
{
    app_spi_disable_dlps(APP_SPI_RX_BIT);
    app_spi_disable_dlps(APP_SPI_TX_BIT);
    hal_gpio_set_level(PIN_M2S_GPIO, GPIO_LEVEL_HIGH);
}

ISR_TEXT_SECTION
static void app_spi_master_gpio_cb(uint32_t context)
{
    hal_gpio_irq_disable(PIN_S2M_GPIO);
    uint8_t pin_index = (uint32_t)context;
    T_GPIO_LEVEL gpio_level = hal_gpio_get_input_level(pin_index);

    if (gpio_level == GPIO_LEVEL_HIGH)
    {
        GDMA_Cmd(SPI_RX_DMA_CHANNEL_NUM, ENABLE);
        GDMA_Cmd(SPI_TX_DMA_CHANNEL_NUM, ENABLE); //SEND DATA
    }
    hal_gpio_irq_enable(PIN_S2M_GPIO);
}

static void app_spi_master_gpio_init(void)
{
    hal_gpio_init();
    hal_gpio_int_init();
    hal_gpio_init_pin(PIN_M2S_GPIO, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT, GPIO_PULL_DOWN);
    hal_gpio_set_level(PIN_M2S_GPIO, GPIO_LEVEL_LOW);

    hal_gpio_init_pin(PIN_S2M_GPIO, GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_DOWN);
    hal_gpio_set_up_irq(PIN_S2M_GPIO, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_HIGH, false);
    hal_gpio_register_isr_callback(PIN_S2M_GPIO, app_spi_master_gpio_cb, PIN_S2M_GPIO);
    hal_gpio_irq_enable(PIN_S2M_GPIO);
}

void app_spi_master_init(void)
{
    uint8_t freq_handle = 0;
    uint32_t actual_mhz = 0;
    pm_cpu_freq_req(&freq_handle, 80, &actual_mhz);

    uint8_t *buf0 = (uint8_t *)malloc(SPI_XMIT_SIZE);
    uint8_t *buf1 = (uint8_t *)malloc(SPI_XMIT_SIZE);
    app_pp_buffer_init(&ppbuf_mgr, buf0, buf1);
    void *wBuf;
    app_pp_buffer_get_write_buf(&ppbuf_mgr, &wBuf);

    app_spi_master_gpio_init();
    app_spi_master_board_init();
    app_spi_master_driver_init();

    if (!GDMA_channel_request(&spi_tx_dma_ch_num, app_spi_master_tx_dma_handler, true))
    {
        APP_PRINT_ERROR0("app_spi_master_init: tx channel request fail");
        return;
    }

    app_spi_master_dma_tx_init();
    app_spi_master_dma_rx_init(wBuf);
    spi_master_init_flag = true;

    APP_PRINT_INFO0("app_spi_master_init");
}

static void app_spi_master_tx_dma_handler(void)
{
    GDMA_SetSourceAddress(SPI_TX_DMA_CHANNEL, (uint32_t)sendbuf);
    GDMA_ClearINTPendingBit(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    app_spi_enable_dlps(APP_SPI_TX_BIT);
    if (app_spi_is_transmit_done())
    {
        app_spi_master_idle = true;
    }
}

static void app_spi_master_rx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);

    app_pp_buffer_set_write_done(&ppbuf_mgr);
#define DEBUG_TEST 0
#if DEBUG_TEST
    void *p_buf;
    if (app_pp_buffer_get_read_buf(&ppbuf_mgr, &p_buf))
    {
        APP_PRINT_INFO3("app_spi_master_rx_dma_handler: 0x%x 0x%x 0x%x", ((uint8_t *)p_buf)[0],
                        ((uint8_t *)p_buf)[1], ((uint8_t *)p_buf)[2]);
    }
    else
    {
        APP_PRINT_ERROR0("app_spi_master_rx_dma_handler: get read buf fail");
    }
#endif

    app_spi_msg_send(IO_SPI_MASTER_DATA_IN, NULL);
    void *wBuf;
    app_pp_buffer_get_write_buf(&ppbuf_mgr, &wBuf);
    GDMA_SetDestinationAddress(SPI_RX_DMA_CHANNEL, (uint32_t)wBuf);

    hal_gpio_set_level(PIN_M2S_GPIO, GPIO_LEVEL_LOW);
    app_spi_enable_dlps(APP_SPI_RX_BIT);
    if (app_spi_is_transmit_done())
    {
        app_spi_master_idle = true;
    }
}

void app_spi_master_enter_low_power(void)
{
    if (spi_master_init_flag)
    {
        APP_PRINT_INFO0("app_spi_master_enter_low_power");
        app_spi_master_pad_pull_up();
        app_dlps_set_pad_wake_up(PIN_S2M_GPIO, PAD_WAKEUP_POL_HIGH);
    }
}

void app_spi_master_exit_low_power(void)
{
    if (spi_master_init_flag)
    {
        APP_PRINT_INFO0("app_spi_master_exit_low_power");
        void *wBuf;
        app_spi_master_board_init();
        app_spi_master_driver_init();
        app_pp_buffer_get_write_buf(&ppbuf_mgr, &wBuf);
        app_spi_master_dma_tx_init();
        app_spi_master_dma_rx_init(wBuf);

        app_dlps_restore_pad(PIN_S2M_GPIO);
    }
}


#endif

