/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include <stdint.h>
#include <stdlib.h>
#include "trace.h"
#include "stdlib.h"
#include "board.h"
#include "rtl876x_i2c.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "aw87390_driver_interface.h"


#define AW87390_I2C_ADDR                 0x58


static struct
{
    bool bypassed;

} aw87390 =
{
    .bypassed = false,
};


const uint8_t aw87390_init_param[33 * 2] =
{
    0x67, 0x03,
    0x02, 0x07,
    0x02, 0x00,
    0x02, 0x0C,
    0x03, 0x08,
    0x04, 0x05,
    0x05, 0x0C,
    0x06, 0x07,
    0x07, 0x4E,
    0x08, 0x06,
    0x09, 0x08,
    0x0A, 0x3A,
    0x61, 0xB3,
    0x62, 0x24,
    0x63, 0x05,
    0x64, 0x48,
    0x65, 0x17,
    0x79, 0x7A,
    0x7A, 0x6C,
    0x78, 0x80,
    0x66, 0x38,
    0x76, 0x00,
    0x78, 0x00,
    0x68, 0x1B,
    0x69, 0x5B,
    0x70, 0x1D,
    0x71, 0x10,
    0x72, 0xB4,
    0x73, 0x4F,
    0x74, 0x24,
    0x75, 0x02,
    0x01, 0x07,
    0xFF, 0x00,
};

#if 0
const uint8_t aw87390_spk_param[33 * 2] =
{
    0x67, 0x03,
    0x02, 0x07,
    0x02, 0x00,
    0x02, 0x0C,
    0x03, 0x08,
    0x04, 0x05,
    0x05, 0x0C,
    0x06, 0x07,
    0x07, 0x4E,
    0x08, 0x06,
    0x09, 0x08,
    0x0A, 0x3A,
    0x61, 0xB3,
    0x62, 0x24,
    0x63, 0x05,
    0x64, 0x48,
    0x65, 0x17,
    0x79, 0x7A,
    0x7A, 0x6C,
    0x78, 0x80,
    0x66, 0x38,
    0x76, 0x00,
    0x78, 0x00,
    0x68, 0x1B,
    0x69, 0x5B,
    0x70, 0x1D,
    0x71, 0x10,
    0x72, 0xB4,
    0x73, 0x4F,
    0x74, 0x24,
    0x75, 0x02,
    0x01, 0x07,
    0xFF, 0x00,
};
#endif

const uint8_t aw87390_music_param[32 * 2] =
{
    0x67, 0x02,
    0x02, 0x07,
    0x02, 0x00,
    0x02, 0x0C,
    0x03, 0x00,
    0x04, 0x05,
    0x05, 0x0C,
    0x06, 0x04,
    0x07, 0x4E,
    0x08, 0x04,
    0x09, 0x08,
    0x0A, 0x3A,
    0x61, 0xB3,
    0x62, 0x24,
    0x63, 0x05,
    0x64, 0x48,
    0x65, 0x17,
    0x79, 0x7A,
    0x7A, 0x6C,
    0x78, 0x80,
    0x66, 0x30,
    0x78, 0x00,
    0x68, 0x1B,
    0x69, 0x5B,
    0x70, 0x1D,
    0x71, 0x00,
    0x72, 0xB4,
    0x73, 0x4F,
    0x74, 0x24,
    0x75, 0x02,
    0x01, 0x07,
    0xFF, 0x00,
};

/**
  * @brief  set Ipeak from 2.5A to 2.0A.
  */
const uint8_t aw87390_ipeak_param[32 * 2] =
{
    0x67, 0x03,
    0x02, 0x07,
    0x02, 0x00,
    0x02, 0x0C,
    0x03, 0x00,
    0x04, 0x00,
    0x05, 0x0C,
    0x06, 0x04,
    0x07, 0x4E,
    0x08, 0x04,
    0x09, 0x08,
    0x0A, 0x12,
    0x61, 0xB3,
    0x62, 0x24,
    0x63, 0x05,
    0x64, 0x48,
    0x65, 0x16,
    0x79, 0x7A,
    0x7A, 0x6C,
    0x78, 0x80,
    0x66, 0x30,
    0x78, 0x00,
    0x68, 0x1B,
    0x69, 0x5B,
    0x70, 0x1D,
    0x71, 0x30,
    0x72, 0xB4,
    0x73, 0x4F,
    0x74, 0x24,
    0x75, 0x02,
    0x01, 0x07,
    0xFF, 0x00,
};

const uint8_t aw87390_bypass_param[32 * 2] =
{
    0x67, 0x02,
    0x02, 0x07,
    0x02, 0x00,
    0x02, 0x0C,
    0x03, 0x00,
    0x04, 0x05,
    0x05, 0x0C,
    0x06, 0x0F,
    0x07, 0x4E,
    0x08, 0x09,
    0x09, 0x08,
    0x0A, 0x3B,
    0x61, 0xB3,
    0x62, 0x24,
    0x63, 0x05,
    0x64, 0x48,
    0x65, 0x17,
    0x79, 0x7A,
    0x7A, 0x6C,
    0x78, 0x80,
    0x66, 0x30,
    0x78, 0x00,
    0x68, 0x1B,
    0x69, 0x5B,
    0x70, 0x1D,
    0x71, 0x00,
    0x72, 0xB4,
    0x73, 0x4F,
    0x74, 0x24,
    0x75, 0x02,
    0x01, 0x07,
    0xFF, 0x00,
};

const uint8_t aw87390_off_param[33 * 2] =
{
    0x67, 0x03,
    0x02, 0x07,
    0x02, 0x00,
    0x02, 0x0C,
    0x03, 0x08,
    0x04, 0x05,
    0x05, 0x0C,
    0x06, 0x07,
    0x07, 0x4E,
    0x08, 0x06,
    0x09, 0x08,
    0x0A, 0x3B,
    0x61, 0xB3,
    0x62, 0x24,
    0x63, 0x05,
    0x64, 0x48,
    0x65, 0x17,
    0x79, 0x7A,
    0x7A, 0x6C,
    0x78, 0x80,
    0x66, 0x38,
    0x76, 0x00,
    0x78, 0x00,
    0x68, 0x1B,
    0x69, 0x5B,
    0x70, 0x1D,
    0x71, 0x10,
    0x72, 0xB4,
    0x73, 0x4F,
    0x74, 0x24,
    0x75, 0x02,
    0x01, 0x00,
    0xFF, 0x00,
};

const uint8_t aw87390_rcv_param[33 * 2] =
{
    0x67, 0x03,
    0x02, 0x07,
    0x02, 0x00,
    0x02, 0x00,
    0x03, 0x08,
    0x04, 0x05,
    0x05, 0x13,
    0x06, 0x0F,
    0x07, 0x4E,
    0x08, 0x09,
    0x09, 0x08,
    0x0A, 0x4B,
    0x61, 0xB3,
    0x62, 0x24,
    0x63, 0x05,
    0x64, 0x28,
    0x65, 0x15,
    0x79, 0x7A,
    0x7A, 0x6C,
    0x78, 0x80,
    0x66, 0xB8,
    0x76, 0x00,
    0x78, 0x00,
    0x68, 0x1B,
    0x69, 0x5B,
    0x70, 0x1C,
    0x71, 0x10,
    0x72, 0xB4,
    0x73, 0x4F,
    0x74, 0x24,
    0x75, 0x02,
    0x01, 0x07,
    0xFF, 0x10,
};


/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_aw87390_init(void)
{
    Pinmux_Config(AW87390_SDA, I2C1_DAT);
    Pinmux_Config(AW87390_SCL, I2C1_CLK);
    Pad_Config(AW87390_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(AW87390_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
}

void aw87390_i2c_enter_dlps(void)
{
    Pad_Config(AW87390_SDA, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(AW87390_SCL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SMART_PA_POWER_EN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
}

void aw87390_i2c_exit_dlps(void)
{
    Pad_Config(AW87390_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(AW87390_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(SMART_PA_POWER_EN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
}

static int aw_dev0_i2c_write_func(uint16_t dev_addr, uint8_t reg_addr, uint8_t *pdata, uint16_t len)
{
    uint8_t *wbuf = NULL;

    wbuf = malloc(len + 1);
    if (wbuf == NULL)
    {
        return -1;
    }
    wbuf[0] = reg_addr;
    memcpy(wbuf + 1, pdata, len);

    I2C_SetSlaveAddress(AW87390_I2C, AW87390_I2C_ADDR);
    if (I2C_MasterWrite(AW87390_I2C, wbuf, len + 1) != I2C_Success)
    {
        free(wbuf);
        return -1;
    }
    else
    {
        free(wbuf);
        return 0;
    }
}

static int aw_dev0_i2c_read_func(uint16_t dev_addr, uint8_t reg_addr, uint8_t *pdata, uint16_t len)
{
    I2C_Status ret;

    I2C_SetSlaveAddress(AW87390_I2C, AW87390_I2C_ADDR);
    ret = I2C_RepeatRead(AW87390_I2C, &reg_addr, 1, pdata, len);
    if (ret != I2C_Success)
    {
        APP_PRINT_ERROR1("i2c_read error: %d", ret);
        return -1;
    }

    return 0;
}

bool aw87390_get_chip_id(uint8_t *p_chip_id)
{
    if (aw_dev0_i2c_read_func(AW87390_I2C_ADDR, 0x00, p_chip_id, 1) != 0)
    {
        return false;
    }
    APP_PRINT_INFO1("[AW87390] aw87390_get_chip_id: %x", *p_chip_id);

    return true;
}

void aw87390_set_bypass(bool bypassed)
{
    aw87390.bypassed = bypassed;
}

bool aw87390_start(uint8_t sample_rate)
{
    APP_PRINT_INFO0("[AW87390] AUDIO_PROBE_DSP_MGR_EVT_SMART_PA_START");
    //os_delay(10);

    if (aw87390.bypassed == false)
    {
        for (uint8_t cnt = 0; cnt < sizeof(aw87390_ipeak_param) / 2; cnt ++)
        {
            aw_dev0_i2c_write_func(AW87390_I2C_ADDR, aw87390_ipeak_param[cnt * 2],
                                   (uint8_t *)(&aw87390_ipeak_param[cnt * 2 + 1]), 1);
        }
#if 0
        for (uint8_t cnt = 0; cnt < sizeof(aw87390_music_param) / 2; cnt ++)
        {
            aw_dev0_i2c_write_func(AW87390_I2C_ADDR, aw87390_music_param[cnt * 2],
                                   (uint8_t *)(&aw87390_music_param[cnt * 2 + 1]), 1);
            APP_PRINT_INFO3("[AW87390] music [%d] %02x %02x", cnt, aw87390_music_param[cnt * 2],
                            aw87390_music_param[cnt * 2 + 1]);
        }
#endif
    }
    else
    {
        for (uint8_t cnt = 0; cnt < sizeof(aw87390_bypass_param) / 2; cnt ++)
        {
            aw_dev0_i2c_write_func(AW87390_I2C_ADDR, aw87390_bypass_param[cnt * 2],
                                   (uint8_t *)(&aw87390_bypass_param[cnt * 2 + 1]), 1);
            APP_PRINT_INFO3("[AW87390] bypass [%d] %02x %02x", cnt, aw87390_bypass_param[cnt * 2],
                            aw87390_bypass_param[cnt * 2 + 1]);
        }
    }

    return true;
}

bool aw87390_stop(void)
{
    APP_PRINT_INFO0("[AW87390] AUDIO_PROBE_DSP_MGR_EVT_SMART_PA_STOP");
    for (uint8_t cnt = 0; cnt < 33; cnt ++)
    {
        aw_dev0_i2c_write_func(AW87390_I2C_ADDR, aw87390_off_param[cnt * 2],
                               (uint8_t *)(&aw87390_off_param[cnt * 2 + 1]), 1);
        //APP_PRINT_INFO3("aw87390_off_param[%d] %02x %02x", cnt, aw87390_off_param[cnt*2], aw87390_off_param[cnt*2+1]);
    }
    return true;
}

static void aw87390_init(void)
{
    for (uint8_t cnt = 0; cnt < 33; cnt ++)
    {
        aw_dev0_i2c_write_func(AW87390_I2C_ADDR, aw87390_init_param[cnt * 2],
                               (uint8_t *)(&aw87390_init_param[cnt * 2 + 1]), 1);
        //APP_PRINT_INFO3("aw87390_init_param[%d] %02x %02x", cnt, aw87390_init_param[cnt*2], aw87390_init_param[cnt*2+1]);
    }
}


void aw87390_driver_init(void)
{
    APP_PRINT_INFO0("[AW87390] aw87390_driver_init");

    board_aw87390_init();

    RCC_PeriphClockCmd(APBPeriph_I2C1, APBPeriph_I2C1_CLOCK, ENABLE);

    I2C_InitTypeDef  I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_InitStructure.I2C_DeviveMode = I2C_DeviveMode_Master;
    I2C_InitStructure.I2C_AddressMode = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_SlaveAddress = AW87390_I2C_ADDR;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;

    I2C_Init(AW87390_I2C, &I2C_InitStructure);
    I2C_Cmd(AW87390_I2C, ENABLE);

    aw87390_init();
}



