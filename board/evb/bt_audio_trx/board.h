/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "app_flags.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_RELEASE             0
#define KEY_PRESS               1

#define MAX_KEY_NUM             8
#define HYBRID_KEY_NUM          8

#define PIN_KEY0                0xFF
#define PIN_KEY1                P2_2
#define PIN_KEY2                0xFF
#define PIN_KEY3                0xFF
#define PIN_KEY4                0xFF
#define PIN_KEY5                0xFF
#define PIN_KEY6                0xFF
#define PIN_KEY7                0xFF

#define RX_GDMA_BUFFER_SIZE             256

#if F_APP_BT_AUDIO_TRANSMITTER_MP3_DEMO_SUPPORT
#undef RX_GDMA_BUFFER_SIZE
#define RX_GDMA_BUFFER_SIZE             1088
#endif

#define AW87390_I2C                  I2C1
#define AW87390_SCL                  ADC_0
#define AW87390_SDA                  ADC_1
#define ONE_WIRE_UART0_PINMUX        P3_0
#define ONE_WIRE_UART2_PINMUX        P3_1
#define DATA_UART_TX_PINMUX          P3_1
#define DATA_UART_RX_PINMUX          P3_0

#if F_APP_NXP_UWB_DRIVER_SUPPORT
#define NXP_UWB_FEATURE_ENABLED             1

#define NXP_UWB_BOARD_MALLOC_RAM_TYPE                   RAM_TYPE_DATA_ON

#define NXP_UWB_GPIO_IRQ                                P2_1
#define NXP_UWB_GPIO_RDY                                P2_2
#define NXP_UWB_GPIO_CHIP_EN                            P0_3

#define NXP_UWB_SPI_CLK_PIN                             P0_0
#define NXP_UWB_SPI_SEL_PIN                             P2_3
#define NXP_UWB_SPI_MISO_PIN                            P0_2
#define NXP_UWB_SPI_MOSI_PIN                            P0_1

#define UWB_SPI_INDEX                   SPI1
#define UWB_SPI_FUNC_CLK                SPI1_CLK_MASTER
#define UWB_SPI_FUNC_MO                 SPI1_MO_MASTER
#define UWB_SPI_FUNC_MI                 SPI1_MI_MASTER
#define UWB_SPI_FUNC_SS                 SPI1_SS_N_0_MASTER
#define UWB_SPI_APBPERIPH               APBPeriph_SPI1
#define UWB_SPI_APBPERIPH_CLOCK         APBPeriph_SPI1_CLOCK

#define UWB_SPI_CS_SOFT                 1
#if UWB_SPI_CS_SOFT
#define UWB_SPI_CS_APBPERIPH            APBPeriph_GPIO
#define UWB_SPI_CS_APBPERIPH_CLOCK      APBPeriph_GPIO_CLOCK
#endif

#define UWB_IRQ_IRQ_PRI     3
#define UWB_IRQ_IRQ_CH      GPIO5_IRQn
#define UWB_IRQ_IRQ_FUNC    GPIO5_Handler
#endif

#if F_APP_SPI_ROLE_SLAVE
#if (TARGET_RTL8773DO || TARGET_RTL8773DFL)
#define PIN_SLAVE_SPI0_SCK          P1_3
#define PIN_SLAVE_SPI0_MOSI         P1_4
#define PIN_SLAVE_SPI0_MISO         P1_5
#define PIN_SLAVE_SPI0_CS           P1_2
#define PIN_S2M_GPIO                P2_1
#define PIN_M2S_GPIO                P0_2
#elif TARGET_RTL87X3EP
#define PIN_SLAVE_SPI0_SCK          P0_0
#define PIN_SLAVE_SPI0_MOSI         P1_0
#define PIN_SLAVE_SPI0_MISO         P0_1
#define PIN_SLAVE_SPI0_CS           P2_4
#define PIN_S2M_GPIO                P2_3
#define PIN_M2S_GPIO                P0_2
#else
#define PIN_SLAVE_SPI0_SCK          P0_0
#define PIN_SLAVE_SPI0_MOSI         P1_0
#define PIN_SLAVE_SPI0_MISO         P0_1
#define PIN_SLAVE_SPI0_CS           P1_1
#define PIN_S2M_GPIO                P0_3
#define PIN_M2S_GPIO                P0_2
#endif
#endif

#if F_APP_SPI_ROLE_MASTER
#if (TARGET_RTL8773DO || TARGET_RTL8773DFL)
#define PIN_MASTER_SPI0_SCK         P1_3
#define PIN_MASTER_SPI0_MOSI        P1_4
#define PIN_MASTER_SPI0_MISO        P1_5
#define PIN_MASTER_SPI0_CS          P1_2
#define PIN_S2M_GPIO                P2_1
#define PIN_M2S_GPIO                P0_2
#elif TARGET_RTL87X3EP
#define PIN_MASTER_SPI0_SCK         P0_0
#define PIN_MASTER_SPI0_MOSI        P1_0
#define PIN_MASTER_SPI0_MISO        P0_1
#define PIN_MASTER_SPI0_CS          P2_4
#define PIN_S2M_GPIO                P2_3
#define PIN_M2S_GPIO                P0_2
#else
#define PIN_MASTER_SPI0_SCK         P0_0
#define PIN_MASTER_SPI0_MOSI        P1_0
#define PIN_MASTER_SPI0_MISO        P0_1
#define PIN_MASTER_SPI0_CS          P1_1
#define PIN_S2M_GPIO                P0_3
#define PIN_M2S_GPIO                P0_2
#endif
#endif

#define SMART_PA_POWER_EN           P1_1

#define I2C_0_DAT_PINMUX            P0_2
#define I2C_0_CLK_PINMUX            P0_3

#define PIN_DSP_LOG_OUTPUT          P0_0

#if F_APP_GUI_SUPPORT
/*Lcd pin definition*/
/*qspi bus*/
#if (LCD_INTERFACE == LCD_INTERFACE_QSPI)
#define LCD_RST                                   P4_4
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
#define LCD_BL                                    P2_3
#endif
/*
These qspi pin set cannot be changed, just recording here by code comments
#define SIO2                                      P9_0
#define SIO1                                      P9_1
#define CS                                        P9_2
#define SIO0                                      P9_3
#define CLK                                       P9_4
#define SIO3                                      P9_5
*/

/*i8080 bus*/
#elif (LCD_INTERFACE == LCD_INTERFACE_8080)
#define LCD_RST                                   P9_5
#define LCD_POWER_EN                              P0_1
#define LCD_8080_BL                               P9_4
/*
These 8080 pin set cannot be changed, just recording here by code comments
#define LCD_8080_D0                               P2_6
#define LCD_8080_D1                               P2_7
#define LCD_8080_D2                               P4_0
#define LCD_8080_D3                               P4_1
#define LCD_8080_D4                               P4_2
#define LCD_8080_D5                               P4_3
#define LCD_8080_D6                               P4_4
#define LCD_8080_D7                               P4_5
#define LCD_8080_CS                               P2_3
#define LCD_8080_DCX                              P2_4
#if IC_TYPE_RTL8763EWE == 1
#define LCD_8080_RD                               P9_0
#else
#define LCD_8080_RD                               P2_1
#endif
#define LCD_8080_WR                               P2_5
*/
#elif (LCD_INTERFACE == LCD_INTERFACE_LCDC_QSPI)
#define LCD_RST                                   P4_4
#endif

/*SDIO pin definition*/
/*These sdio pin set cannot be changed, just recording here by code comments
typedef enum
{
    SDH_CLK,
    SDH_CMD,
    SDH_D0,
    SDH_D1,
    SDH_D2,
    SDH_D3,
    SDH_WT_PROT,
    SDH_CD,
} T_SD_PIN_NUM;
The use of group0 or 1 depends on hardware mount, the default is group0, which is configured in sd_card_cfg.sdh_group
const uint8_t   sdio_pin_group0[8] = {P5_0, P5_1, P5_2, P5_3, P5_4, P5_5, P5_7, P5_6};
const uint8_t   sdio_pin_group1[8] = {P6_0, P6_1, P6_2, P6_3, P6_4, P6_5, P2_0, P2_3};
*/

#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916) || (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801) || (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#define LCD_TE                                    P2_2
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z_QSPI_1_BIT) || (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z_SPI)
#define LCD_TE                                    P4_5
#endif

#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
#if (TARGET_RTL8773DO == 1)
#define VCI_EN                                    P5_5
#else
#define VCI_EN                                    P4_3
#endif
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#define LCD_AVDD_EN                               P2_3
#endif

/*Touch pin definition*/
#if (TARGET_RTL8773DO == 1)
#define TOUCH_I2C_SCL                             P4_5
#define TOUCH_I2C_SDA                             P4_6
#elif ((((CONFIG_REALTEK_TARGET_RTL8763EWE_VP == 1) || (CONFIG_REALTEK_TARGET_RTL8763EWE == 1)) && (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CS816T)) || (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CST816D) || (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CHSC6417) || (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_CHSC5816))
#define TOUCH_I2C_SCL                             P0_0
#define TOUCH_I2C_SDA                             P0_1
#elif (((CONFIG_REALTEK_TARGET_RTL8763EWE_VP == 1) || (CONFIG_REALTEK_TARGET_RTL8763EWE == 1)) && (TARGET_TOUCH_DEVICE == TOUCH_DEVICE_GT9147))
#define TOUCH_I2C_SCL                             P1_1
#define TOUCH_I2C_SDA                             P1_0
#else
#define TOUCH_I2C_SCL                             P4_7
#define TOUCH_I2C_SDA                             P4_6
#endif
#define TOUCH_INT                                 P0_2
#define TOUCH_RST                                 P0_3
#endif

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_H_ */

