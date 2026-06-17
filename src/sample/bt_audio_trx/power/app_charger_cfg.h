/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CHARGER_CFG_H_
#define _APP_CHARGER_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_CHARGER App Charger
  * @brief App Charger
  * @{
  */

/** @brief  Read only configurations for charger */
typedef struct
{
    uint8_t charger_support : 1;
    uint8_t discharger_support : 1;
    uint8_t battery_warning_percent;
    uint8_t timer_low_bat_warning;
    uint8_t low_bat_warning_count;
    uint8_t enable_bat_report_when_phone_conn : 1;
    uint8_t enable_new_low_bat_time_unit : 1; //0:sec, 1:min
    uint8_t enable_report_lower_battery_volume : 1;
    uint8_t enable_bat_report_when_level_drop : 1;
    uint8_t disable_bat_report_when_power_on : 1;

} T_APP_CHARGER_CFG;


extern T_APP_CHARGER_CFG app_charger_cfg;


/**
    * @brief  Charger config module init
    * @param  void
    * @return void
    */
void app_charger_cfg_init(void);


/** End of APP_CHARGER
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
