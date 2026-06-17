/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __WDG_H_
#define __WDG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/** @defgroup  HAL_WDG_API    WDG API
    * @brief This file introduces the watch dog timer (WDG) APIs.
    * @{
    */

/*============================================================================*
 *                                 Types
 *============================================================================*/

/** @defgroup HAL_WDG_API_Exported_Types WDG API Exported Types
  * @{
  */

/**
  * @brief  WDG mode definition.
  */
typedef enum _WDG_MODE
{
    INTERRUPT_CPU = 0,          /**< Interrupt CPU only. */
    RESET_ALL_EXCEPT_AON = 1,   /**< Reset all except RTC and some AON register. */
    RESET_CORE_DOMAIN = 2,      /**< Reset core domain. */
    RESET_ALL = 3               /**< Reset all. */
} T_WDG_MODE;


/** End of group HAL_WDG_API_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup HAL_WDG_API_Exported_Functions WDG API Exported Functions
 * @{
 */

/**
    * @brief  Get watch dog default mode.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @return Watch dog mode @ref T_WDG_MODE.
    */
T_WDG_MODE wdg_get_default_mode(void);

/**
    * @brief  Get watch dog current mode.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @return Watch dog mode @ref T_WDG_MODE.
    */
T_WDG_MODE wdg_get_mode(void);

/**
    * @brief  Get watch dog current timeout value.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @return The current timeout value, unit ms.
    */
uint32_t wdg_get_timeout_ms(void);

/**
    * @brief  Get watch dog default timeout value.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @return The default timeout value, unit ms.
    */
uint32_t wdg_get_default_timeout_ms(void);

/**
    * @brief  Change watch dog timeout period.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  timeout_ms: New timeout value.
    */
void wdg_change_timeout_period(uint32_t timeout_ms);

/**
    * @brief  Change watch dog mode.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  mode: Watch dog mode @ref T_WDG_MODE.
    */
void wdg_change_mode(T_WDG_MODE mode);

/**
    * @brief  Check if watch dog is enabled.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @retval true: Watch dog is enabled.
    * @retval false: Watch dog is disabled.
    */
bool wdg_is_enable(void);

/**
    * @brief  Set watch dog timeout period and mode.
    * @param  ms: New timeout value.
    * @param  wdg_mode: Watch dog mode @ref T_WDG_MODE.
    * @retval true: Set the period and mode success.
    * @retval false: Set the period and mode fail.
    */
extern bool (*wdg_start)(uint32_t ms, T_WDG_MODE  wdg_mode);
#define WDG_Start(ms, wdg_mode) wdg_start(ms, wdg_mode);

/**
    * @brief  Disable watch dog.
    */
extern void (*wdg_disable)(void);
#define WDG_Disable() wdg_disable();

/**
    * @brief  Kick watch dog.
    */
extern void (*wdg_kick)(void);
#define WDG_Kick() wdg_kick();

/**
    * @brief  Reset the MCU at specified mode.
    * @param  wdg_mode: Watch dog mode @ref T_WDG_MODE.
    */
extern void wdt_reset(T_WDG_MODE wdg_mode, const char *function, uint32_t line);
#define chip_reset(mode)    wdt_reset(mode,  __FUNCTION__, __LINE__)

/**
    * @brief  Reset the MCU at specified mode with reset reason.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  wdg_mode: Watch dog mode @ref T_WDG_MODE.
    * @param  reset_reason: Reset reason.
    * NOTE: The maximum value of RTL87x3E/RTL87x3EP is 0x3F, while the other ICs is 0xFF.
    */
void wdg_reset_with_reason(T_WDG_MODE wdg_mode, uint8_t reset_reason);

/**
    * @brief  Get watch dog reset reason.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @return The watch dog reset reason.
    * NOTE: The maximum value of RTL87x3E/RTL87x3EP is 0x3F, while the other ICs is 0xFF.
    */
uint8_t wdg_get_reset_reason(void);

#ifdef __cplusplus
}
#endif
/** @} */ /* End of group HAL_WDG_API_Exported_Functions */
/** @} */ /* End of group HAL_WDG_API */
#endif
