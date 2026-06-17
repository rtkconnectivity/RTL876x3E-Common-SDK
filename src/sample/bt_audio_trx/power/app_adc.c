/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#if F_APP_ADC_SUPPORT
#include "trace.h"
#include "app_adc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_adc.h"
#include "adc_manager.h"
#include "app_report.h"

uint8_t application_key_adc_channel_index = 0x0;
bool is_adc_mgr_ever_registered = false;

static uint8_t target_io_pin;
static uint8_t cmd_path;
static uint8_t app_idx;

/***********************************************************
AON must use system_status_api
************************************************************/
/** @defgroup  APP_ADC_API Audio ADC
    * @brief app process implementation for audio sample project
    * @{
    */

/*============================================================================*
 *                              Public Functions
 *============================================================================*/
/** @defgroup APP_ADC_Exported_Functions Audio ADC Functions
    * @brief
    * @{
    */

void app_adc_set_cmd_info(uint8_t path, uint8_t idx)
{
    cmd_path = path;
    app_idx = idx;
}

bool app_adc_enable_read_adc_io(uint8_t input_pin)
{
    if (!IS_ADC_CHANNEL(input_pin))
    {
        APP_PRINT_ERROR0("app_adc_enable_read_adc_io: invalid ADC IO");
        return false;
    }

    if (!is_adc_mgr_ever_registered)
    {
        target_io_pin = input_pin;
        Pad_Config(input_pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
        Pad_PowerOrShutDownValue(input_pin, 0);

        ADC_HighBypassCmd(input_pin, ENABLE);

        ADC_InitTypeDef ADC_InitStruct;
        ADC_StructInit(&ADC_InitStruct);
        ADC_InitStruct.adcClock = ADC_CLK_39K;
        ADC_InitStruct.bitmap = 0x1;
        ADC_InitStruct.schIndex[0] = EXT_SINGLE_ENDED(input_pin);

        if (!adc_mgr_register_req(&ADC_InitStruct,
                                  (adc_callback_function_t)app_adc_interrupt_handler,
                                  &application_key_adc_channel_index))
        {
            APP_PRINT_ERROR0("app_adc_enable_read_adc_io: ADC Register Request Fail");
            return false;
        }
        else
        {
            is_adc_mgr_ever_registered = true;
        }
    }

    adc_mgr_enable_req(application_key_adc_channel_index);
    return true;
}

void app_adc_interrupt_handler(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data;
    uint16_t final_value[2];
    uint8_t evt_param[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    adc_mgr_read_data_req(application_key_adc_channel_index, &adc_data, 0x0001);

    final_value[0]  = ADC_GetHighBypassRes(adc_data,
                                           EXT_SINGLE_ENDED(target_io_pin));

    evt_param[0] = (uint8_t) final_value[0];
    evt_param[1] = (uint8_t)(final_value[0] >> 8);
    APP_PRINT_TRACE4("app_adc_interrupt_handler: int_status: %d, adc_data = 0x%x, Final result = 0x%02X %02X",
                     int_status, adc_data,
                     evt_param[1], evt_param[0]);

    app_report_event(cmd_path, EVENT_REPORT_PAD_VOLTAGE, app_idx, evt_param, sizeof(evt_param));
}
#endif
/** @} */ /* End of group APP_ADC_Exported_Functions */
/** @} */ /* End of group APP_ADC_API */
