/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/
#ifndef _AUDIO_ADC_API_H_
#define _AUDIO_ADC_API_H_

#include "app_charger.h"

/** @defgroup  APP_AUDIO_ADC_API App Audio Adc Api
    * @brief AUDIO ADC module implementation for audio sample project
    * @{
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup AUDIO_ADC_Exported_Types Audio ADC Types
    * @{
    */


/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup AUDIO_ADC_Exported_Functions Audio ADC Functions
    * @{
    */
bool app_adc_enable_read_adc_io(uint8_t target_io_pin);
void app_adc_interrupt_handler(void *pvPara, uint32_t int_status);
void app_adc_set_cmd_info(uint8_t cmd_path, uint8_t app_idx);

/** @} */ /* End of group AUDIO_ADC_Exported_Functions */

/** @} */ /* End of group AUDIO_ADC_API */

#endif
