/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_KEY_CFG_H_
#define _APP_KEY_CFG_H_

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

#define KEY_NULL                0
#define KEY0_MASK               0x01
#define KEY1_MASK               0x02
#define KEY2_MASK               0x04
#define KEY3_MASK               0x08
#define KEY4_MASK               0x10
#define KEY5_MASK               0x20
#define KEY6_MASK               0x40
#define KEY7_MASK               0x80

#define CALL_TYPES_NUM          9

typedef enum
{
    HYBRID_KEY_SHORT_PRESS              = 0x00,
    HYBRID_KEY_LONG_PRESS               = 0x01,
    HYBRID_KEY_ULTRA_LONG_PRESS         = 0x02,
    HYBRID_KEY_2_CLICK                  = 0x03,
    HYBRID_KEY_3_CLICK                  = 0x04,
    HYBRID_KEY_4_CLICK                  = 0x05,
    HYBRID_KEY_5_CLICK                  = 0x06,
    HYBRID_KEY_HALL_SWITCH_HIGH         = 0x07,
    HYBRID_KEY_HALL_SWITCH_LOW          = 0x08,
    HYBRID_KEY_COMBINEKEY_POWER_ONOFF   = 0x09,
    HYBRID_KEY_6_CLICK                  = 0x0A,
    HYBRID_KEY_7_CLICK                  = 0x0B,
    HYBRID_KEY_CLICK_AND_PRESS          = 0x0C,

    HYBRID_KEY_TOTAL
} T_HYBRID_KEY_SCENARIO;

/** @defgroup APP_KEY App Key
  * @brief App Key
  * @{
  */
/** @brief  Read only configurations for key */
typedef struct
{
    uint8_t key_gpio_support;

    uint8_t key_multi_click_interval;
    uint8_t key_long_press_interval;
    uint8_t key_long_press_repeat_interval;
    uint8_t key_power_on_interval;
    uint8_t key_power_off_interval;
    uint8_t key_enter_pairing_interval;
    uint8_t key_factory_reset_interval;
    uint8_t key_pinmux[8];
    uint8_t key_enable_mask;
    uint8_t key_long_press_repeat_mask;

    uint8_t key_high_active_mask;
    uint8_t key_disable_power_on_off;
    uint8_t mfb_replace_key0;

    uint8_t key_table[2][CALL_TYPES_NUM][MAX_KEY_NUM]; //Table 0: key short click, Table 1: key long press
    uint8_t hybrid_key_table[CALL_TYPES_NUM][HYBRID_KEY_NUM];
    uint8_t hybrid_key_mapping[HYBRID_KEY_NUM][2]; // [i][0]: Key Index. [i][1]: Hybrid Key click type


} T_APP_KEY_CFG;


extern T_APP_KEY_CFG app_key_cfg;


/**
    * @brief  KEY config module init
    * @param  void
    * @return void
    */
void app_key_cfg_init(void);


/** End of APP_KEY
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
