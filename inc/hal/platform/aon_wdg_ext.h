/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __AON_WDG_EXT_H_
#define __AON_WDG_EXT_H_


/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "rtl876x.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AON_WDG1_REG   ((BTAON_FAST_RTC_AON_WDT *)AON_WDG_REG_BASE)
#define AON_WDG2_REG   ((BTAON_FAST_RTC_AON_WDT *)(AON_WDG_REG_BASE + 0x0c))

#define AON_WDG1_REG_OFFSET   ((uint32_t)AON_WDG1_REG - RTC_REG_BASE)
#define AON_WDG2_REG_OFFSET   ((uint32_t)AON_WDG2_REG - RTC_REG_BASE)
/*AON_WDG1 is the same as charger aon wdg, and can not be used for the project with using charger*/
#define AON_WDG1 (BTAON_FAST_RTC_AON_WDT *)AON_WDG1_REG_OFFSET
#define AON_WDG2 (BTAON_FAST_RTC_AON_WDT *)AON_WDG2_REG_OFFSET

/**
    * @brief  Enable aon wdg.
    * \xrefitem Added_API_2_14_0_0 "Added Since 2.14.0.0" "Added API"
    * @param  wdg: @ref BTAON_FAST_RTC_AON_WDT
    */
void aon_wdg_enable(BTAON_FAST_RTC_AON_WDT *wdg);

/**
    * @brief  Disable aon wdg.
    * \xrefitem Added_API_2_14_0_0 "Added Since 2.14.0.0" "Added API"
    * @param  wdg: @ref BTAON_FAST_RTC_AON_WDT
    */
void aon_wdg_disable(BTAON_FAST_RTC_AON_WDT *wdg);

/**
    * @brief  Restart aon wdg.
    * \xrefitem Added_API_2_14_0_0 "Added Since 2.14.0.0" "Added API"
    * @param  wdg: @ref BTAON_FAST_RTC_AON_WDT
    */
void aon_wdg_kick(BTAON_FAST_RTC_AON_WDT *wdg);

/**
    * @brief  Aon wdg is enabled or not.
    * \xrefitem Added_API_2_14_0_0 "Added Since 2.14.0.0" "Added API"
    * @param  wdg: @ref BTAON_FAST_RTC_AON_WDT
    * @return True: enabled; false: diabled
    */
bool aon_wdg_is_enable(BTAON_FAST_RTC_AON_WDT *wdg);

#ifdef __cplusplus
}
#endif

#endif
