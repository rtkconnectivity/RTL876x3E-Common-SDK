/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_SPI_ROLE_SLAVE
#include "app_spi_api.h"
#include "rtl876x.h"
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "os_sync.h"
#include "vector_table.h"
#include "dma_channel.h"
#include "trace.h"
#include "pm.h"
#include "section.h"
#include "io_debug.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"

#include "stdlib.h"

#include "app_dlps.h"
#include "app_pp_buffer.h"
#include "app_spi_common.h"
#include "app_util.h"
#include "app_cmd.h"
#include "board.h"

#define FLASH_SPI                         SPI0
#define SPI_CLKDIV                        10

static uint8_t sendbuf[SPI_XMIT_SIZE];

static uint32_t spi_slow_clk = 0;
static uint8_t spi_tx_seq = 0;
static uint8_t spi_tx_dma_ch_num = 0xa5;
static uint8_t spi_rx_dma_ch_num = 8;
static bool spi_slave_init_flag = false;

#define SPI_TX_DMA_CHANNEL_NUM     spi_tx_dma_ch_num
#define SPI_TX_DMA_CHANNEL         DMA_CH_BASE(spi_tx_dma_ch_num)
#define SPI_TX_DMA_IRQ             DMA_CH_IRQ(spi_tx_dma_ch_num)
#define SPI_RX_DMA_CHANNEL_NUM     spi_rx_dma_ch_num
#define SPI_RX_DMA_CHANNEL         DMA_CH_BASE(spi_rx_dma_ch_num)
#define SPI_RX_DMA_IRQ             DMA_CH_IRQ(spi_rx_dma_ch_num)
#define SPI_RX_DMA_VECTOR          DMA_CH_VECTOR(spi_rx_dma_ch_num)

static T_PP_BUFFER ppbuf_recv_mgr;
static T_PP_BUFFER ppbuf_send_mgr;

static void app_spi_slave_tx_dma_handler(void);
static void app_spi_slave_rx_dma_handler(void);
static void app_spi_slave_tx_handler(void);
static void app_spi_slave_clear_rx_fifo(void);

uint8_t app_spi_slave_send_data(uint16_t cmd_id, uint8_t *p_data, uint16_t len)
{
    spi_tx_seq++;
    if (spi_tx_seq == 0)
    {
        spi_tx_seq = 1;
    }

    uint16_t total_len = len + 6;

    if (p_data != NULL)
    {
        if (len == 0 || total_len > SPI_XMIT_SIZE)
        {
            return SPI_SEND_ERR_LEN;
        }
    }
    void *wBuf;
    app_pp_buffer_get_write_buf(&ppbuf_send_mgr, &wBuf);

    uint8_t *p_ppbuf = (uint8_t *)wBuf;
    memset(p_ppbuf, 0, SPI_XMIT_SIZE);

    p_ppbuf[0] = CMD_SYNC_BYTE;
    p_ppbuf[1] = spi_tx_seq;
    p_ppbuf[2] = (uint8_t)(len + 2);
    p_ppbuf[3] = (uint8_t)((len + 2) >> 8);
    p_ppbuf[4] = (uint8_t)cmd_id;
    p_ppbuf[5] = (uint8_t)(cmd_id >> 8);

    if (p_data != NULL)
    {
        memcpy(&p_ppbuf[6], p_data, len);
    }

    p_ppbuf[SPI_XMIT_SIZE - 1] = app_util_calc_checksum(&p_ppbuf[1], SPI_XMIT_SIZE - 2);
    APP_PRINT_INFO1("app_spi_slave_send_data: seq 0x%x", spi_tx_seq);

    app_pp_buffer_set_write_done(&ppbuf_send_mgr);
    return SPI_SEND_SUC;
}

void app_spi_slave_cmd_set_parser(void)
{
    if (app_spi_is_transmit_done())
    {
        pm_cpu_slow_freq_set(1);
    }

    uint8_t err_code = 0;
    uint8_t check_sum = 0;
    uint16_t cmd_len = 0;
    uint8_t rx_seq = 0;
    uint8_t *p_data = NULL;
    void *rBuf = NULL;

    if (!app_pp_buffer_get_read_buf(&ppbuf_recv_mgr, &rBuf))
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
    app_pp_buffer_set_read_done(&ppbuf_recv_mgr);

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
    app_handle_cmd_set(&p_data[4], cmd_len, CMD_PATH_SPI, rx_seq, 0); //from cmd_id, rx seq always 0?

    free(p_data);
    return;

ERR:
    if (p_data != NULL)
    {
        free(p_data);
    }
    app_spi_slave_clear_rx_fifo();
    APP_PRINT_ERROR1("app_spi_slave_cmd_set_parser: err_code -%d", err_code);
}

static uint8_t app_spi_slave_get_slow_clock(void)
{
    uint8_t required_mhz = 0;
    if (SPI_CLKDIV < 8)
    {
#if (TARGET_RTL8773DO || TARGET_RTL8773DFL)
        required_mhz = 5; //5M
#else
        required_mhz = 4; //4M
#endif
    }
    else if (SPI_CLKDIV < 10)
    {
        required_mhz = 2; //2.5M
    }
#if (TARGET_RTL8773DO == 0 && TARGET_RTL8773DFL == 0)
    else if (SPI_CLKDIV < 32)
    {
        required_mhz = 1; //1.25M
    }
#endif
    return required_mhz;
}

static void app_spi_slave_board_spi_init(void)
{
    Pinmux_Config(PIN_SLAVE_SPI0_SCK, SPI0_CLK_SLAVE);
    Pinmux_Config(PIN_SLAVE_SPI0_MOSI, SPI0_SI_SLAVE);
    Pinmux_Config(PIN_SLAVE_SPI0_MISO, SPI0_SO_SLAVE);
    Pinmux_Config(PIN_SLAVE_SPI0_CS, SPI0_SS_N_0_SLAVE);
    Pad_Config(PIN_SLAVE_SPI0_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_SLAVE_SPI0_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_SLAVE_SPI0_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_SLAVE_SPI0_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
}

static void app_spi_slave_pad_pull_up(void)
{
    Pad_Config(PIN_SLAVE_SPI0_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_SLAVE_SPI0_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_SLAVE_SPI0_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(PIN_SLAVE_SPI0_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
}

static void app_spi_slave_driver_spi_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Slave;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = SPI_CLKDIV;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_TxDmaEn             = ENABLE;
    SPI_InitStructure.SPI_RxDmaEn             = ENABLE;
    SPI_InitStructure.SPI_TxThresholdLevel    = 0;
    SPI_InitStructure.SPI_TxWaterlevel        = 46;
    SPI_InitStructure.SPI_RxWaterlevel        = 31;
    SPI_Init(SPI0, &SPI_InitStructure);
    SPI_Cmd(SPI0, ENABLE);

    RamVectorTableUpdate(SPI0_VECTORn, (IRQ_Fun)app_spi_slave_tx_handler);
    SPI_INTConfig(SPI0, SPI_INT_TUF, ENABLE);

    /* Config SPI interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel         = SPI0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd      = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

static void app_spi_slave_dma_tx_init(void)
{
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

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_TX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
    GDMA_INTConfig(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
}

static void app_spi_slave_dma_rx_init(void *readbuf)
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

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
    GDMA_INTConfig(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
}

static void app_spi_slave_clear_tx_fifo(void)
{
    uint16_t send_len = 0;

    /* Check communication status */
    send_len = SPI_GetTxFIFOLen(FLASH_SPI);
    APP_PRINT_ERROR1("app_spi_slave_clear_tx_fifo: send_len %d", send_len);
    SPI_INTConfig(SPI0, SPI_INT_TUF, ENABLE);
}

static void app_spi_slave_clear_rx_fifo(void)
{
    uint16_t recv_len = 0;

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
    APP_PRINT_INFO1("app_spi_slave_clear_rx_fifo: %d", recv_len);
    while (recv_len)
    {
        SPI_ReceiveData(FLASH_SPI);
        recv_len--;
    }
}

void app_spi_slave_dma_trigger(void)
{
    pm_cpu_slow_freq_set(spi_slow_clk);
    uint32_t s = os_lock();
    app_spi_disable_dlps(APP_SPI_RX_BIT | APP_SPI_TX_BIT);

    uint8_t *temp_buf = NULL;
    if (!app_pp_buffer_get_read_buf(&ppbuf_send_mgr, (void *)&temp_buf))
    {
        sendbuf[0] = SPI_CMD_SYNC_INVALID;
        APP_PRINT_WARN0("app_spi_slave_dma_trigger: no data to be sent");
    }
    else
    {
        memcpy(sendbuf, temp_buf, SPI_XMIT_SIZE);
        app_pp_buffer_set_read_done(&ppbuf_send_mgr);
    }
    os_unlock(s);
    app_spi_slave_board_spi_init();
    app_spi_slave_driver_spi_init();
    app_spi_slave_dma_tx_init();
    void *wBuf;
    app_pp_buffer_get_write_buf(&ppbuf_recv_mgr, &wBuf);
    app_spi_slave_dma_rx_init(wBuf);

    GDMA_Cmd(SPI_RX_DMA_CHANNEL_NUM, ENABLE);
    GDMA_Cmd(SPI_TX_DMA_CHANNEL_NUM, ENABLE); //SEND DATA
    SPI_INTConfig(SPI0, SPI_INT_TXE, ENABLE);

    hal_gpio_set_level(PIN_S2M_GPIO, GPIO_LEVEL_HIGH);
}

ISR_TEXT_SECTION
static void app_spi_slave_gpio_cb(uint32_t context)
{
    hal_gpio_irq_disable(PIN_M2S_GPIO);
    uint8_t pin_index = (uint32_t)context;
    T_GPIO_LEVEL gpio_level = hal_gpio_get_input_level(pin_index);

    if (gpio_level == GPIO_LEVEL_HIGH)
    {
        app_spi_msg_send(IO_SPI_SLAVE_TRIGGER, NULL);
    }
    hal_gpio_irq_enable(PIN_M2S_GPIO);
}

static void app_spi_slave_gpio_init(void)
{
    hal_gpio_init();
    hal_gpio_int_init();
    hal_gpio_init_pin(PIN_M2S_GPIO, GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_DOWN);
    hal_gpio_set_up_irq(PIN_M2S_GPIO, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_HIGH, false);
    hal_gpio_register_isr_callback(PIN_M2S_GPIO, app_spi_slave_gpio_cb, PIN_M2S_GPIO);
    hal_gpio_irq_enable(PIN_M2S_GPIO);

    hal_gpio_init_pin(PIN_S2M_GPIO, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT, GPIO_PULL_DOWN);
    hal_gpio_set_level(PIN_S2M_GPIO, GPIO_LEVEL_LOW);
}

void app_spi_slave_init(void)
{
    uint8_t freq_handle = 0;
    uint32_t actual_mhz = 0;
    pm_cpu_freq_req(&freq_handle, 80, &actual_mhz);

    app_spi_slave_gpio_init();
    app_spi_slave_board_spi_init();
    app_spi_slave_driver_spi_init();

    if (!GDMA_channel_request(&spi_tx_dma_ch_num, app_spi_slave_tx_dma_handler, true))
    {
        APP_PRINT_ERROR0("app_spi_slave_init: tx channel request fail");
        return;
    }

    RamVectorTableUpdate(SPI_RX_DMA_VECTOR, app_spi_slave_rx_dma_handler);

    //pingpong buf init
    uint8_t *send_buf0 = (uint8_t *)calloc(1, SPI_XMIT_SIZE);
    uint8_t *send_buf1 = (uint8_t *)calloc(1, SPI_XMIT_SIZE);
    send_buf0[0] = SPI_CMD_SYNC_INVALID;
    send_buf1[0] = SPI_CMD_SYNC_INVALID;
    app_pp_buffer_init(&ppbuf_send_mgr, send_buf0, send_buf1);

    // dma tx
    app_spi_slave_dma_tx_init();

    // dma rx
    uint8_t *buf0 = (uint8_t *)malloc(SPI_XMIT_SIZE);
    uint8_t *buf1 = (uint8_t *)malloc(SPI_XMIT_SIZE);
    app_pp_buffer_init(&ppbuf_recv_mgr, buf0, buf1);
    void *wBuf;
    app_pp_buffer_get_write_buf(&ppbuf_recv_mgr, &wBuf);
    app_spi_slave_dma_rx_init(wBuf);

    spi_slave_init_flag = true;

    // spi_slow_clk = app_spi_slave_get_slow_clock();

    APP_PRINT_INFO0("app_spi_slave_init");
}

static void app_spi_slave_tx_dma_handler(void)
{
    GDMA_SetSourceAddress(SPI_TX_DMA_CHANNEL, (uint32_t)sendbuf);
    GDMA_ClearINTPendingBit(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);

    app_spi_enable_dlps(APP_SPI_TX_BIT);
}

static void app_spi_slave_rx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);

    hal_gpio_set_level(PIN_S2M_GPIO, GPIO_LEVEL_LOW);

    app_pp_buffer_set_write_done(&ppbuf_recv_mgr);
    app_spi_msg_send(IO_SPI_SLAVE_DATA_IN, NULL);
    void *wBuf;
    app_pp_buffer_get_write_buf(&ppbuf_recv_mgr, &wBuf);
    GDMA_SetDestinationAddress(SPI_RX_DMA_CHANNEL, (uint32_t)wBuf);

    app_spi_enable_dlps(APP_SPI_RX_BIT);
}

void app_spi_slave_enter_low_power(void)
{
    if (spi_slave_init_flag)
    {
        app_spi_slave_pad_pull_up();
    }
}

void app_spi_slave_exit_low_power(void)
{
    if (spi_slave_init_flag)
    {
        APP_PRINT_INFO0("app_spi_slave_exit_low_power");
        app_dlps_restore_pad(PIN_M2S_GPIO);
        app_dlps_set_pad_wake_up(PIN_M2S_GPIO, PAD_WAKEUP_POL_HIGH);
    }
}

static void app_spi_slave_tx_handler(void)
{
    if (SPI_GetINTStatus(SPI0, SPI_INT_TUF) == SET)
    {
        SPI_ClearINTPendingBit(SPI0, SPI_INT_TUF);
        SPI_INTConfig(SPI0, SPI_INT_TUF, DISABLE);
        APP_PRINT_ERROR0("app_spi_slave_tx_handler: SPI_INT_TUF!");
        app_spi_slave_clear_tx_fifo();
    }

    if (SPI_GetINTStatus(SPI0, SPI_INT_TXE) == SET)
    {
        SPI_ClearINTPendingBit(SPI0, SPI_INT_TXE);
        SPI_INTConfig(SPI0, SPI_INT_TXE, DISABLE);
        APP_PRINT_INFO0("app_spi_slave_tx_handler: SPI_INT_TXE!");
    }
}

#endif

