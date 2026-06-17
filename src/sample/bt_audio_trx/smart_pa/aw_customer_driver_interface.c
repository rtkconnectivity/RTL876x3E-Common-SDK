/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "trace.h"
#include "board.h"
#include "aw_customer_driver_interface.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_ir.h"


#define IR_TX_FIFO_THR_LEVEL             2
#define BIT(_n)     (uint32_t)(1U << (_n))

static uint8_t customer_aw_mdoe_type = 0;


void set_customer_aw_mdoe_type(uint8_t type)
{
    customer_aw_mdoe_type = type;
}

uint8_t get_customer_aw_mdoe_type(void)
{
    return customer_aw_mdoe_type;
}

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_ir_init(void)
{
    Pad_Config(SMART_PA_POWER_EN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pinmux_Config(SMART_PA_POWER_EN, IRDA_TX);
}

/**
  * @brief  Initialize IR peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_ir_init(void)
{
    /* Enable IR clock */
    RCC_PeriphClockCmd(APBPeriph_IR, APBPeriph_IR_CLOCK, ENABLE);

    /* Initialize IR */
    IR_InitTypeDef IR_InitStruct;
    IR_StructInit(&IR_InitStruct);
    IR_InitStruct.IR_Freq           = 150;
    IR_InitStruct.IR_DutyCycle      = 2; /* !< 1/2 duty cycle */
    IR_InitStruct.IR_Mode           = IR_MODE_TX;
    IR_InitStruct.IR_TxIdleLevel    = IR_IDLE_OUTPUT_LOW;
    IR_InitStruct.IR_TxInverse      = IR_TX_DATA_NORMAL;
    IR_InitStruct.IR_TxFIFOThrLevel = IR_TX_FIFO_THR_LEVEL;
    IR_Init(&IR_InitStruct);
    IR_TxOutputInverse(ENABLE);
}

bool aw_customer_start(uint8_t sample_rate)
{
    board_ir_init();
    driver_ir_init();
    uint32_t  irBuf[2] = {0x80000001, 0x000000ff};
    uint8_t aw_mode = get_customer_aw_mdoe_type();
    APP_PRINT_INFO1("aw_customer_start aw_mode: %d", aw_mode);

    if (aw_mode > aw_customer_mode_10)
    {
        return false;
    }
    else
    {
        if (aw_mode == aw_customer_mode_1)
        {
            Pad_Config(SMART_PA_POWER_EN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            return true;
        }
        switch (aw_mode)
        {
        case aw_customer_mode_2:
            {
                irBuf[0] = 0x80000001;
            }
            break;
        case aw_customer_mode_3:
            {
                irBuf[0] = 0x80000002;
            }
            break;
        case aw_customer_mode_4:
            {
                irBuf[0] = 0x80000003;
            }
            break;
        case aw_customer_mode_5:
            {
                irBuf[0] = 0x80000004;
            }
            break;
        case aw_customer_mode_6:
            {
                irBuf[0] = 0x80000005;
            }
            break;
        case aw_customer_mode_7:
            {
                irBuf[0] = 0x80000006;
            }
            break;
        case aw_customer_mode_8:
            {
                irBuf[0] = 0x80000007;
            }
            break;
        case aw_customer_mode_9:
            {
                irBuf[0] = 0x80000008;
            }
            break;
        case aw_customer_mode_10:
            {
                irBuf[0] = 0x80000009;
            }
            break;
        }
        IR_SendBuf(irBuf, 2, ENABLE);
        IR_Cmd(IR_MODE_TX, ENABLE);
        while (!(IR->TX_SR & BIT(15)));
        Pad_Config(SMART_PA_POWER_EN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
    return true;
}

bool aw_customer_stop(void)
{
    APP_PRINT_INFO0("[aw_customer_stop] aw_customer_stop");
    IR_Cmd(IR_MODE_TX, DISABLE);
    Pad_Config(SMART_PA_POWER_EN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE,
               PAD_OUT_LOW);

    return true;
}

void customer_aw_pin_init(void)
{
    APP_PRINT_INFO0("[AW] customer_aw_pin_init");
    Pad_Config(SMART_PA_POWER_EN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE,
               PAD_OUT_LOW);
}



