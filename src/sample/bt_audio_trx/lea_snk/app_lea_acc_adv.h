/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LE_AUDIO_ACC_ADV_H_
#define _APP_LE_AUDIO_ACC_ADV_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "ble_ext_adv.h"

/** @defgroup APP_LEA_ADV App LE Audio ADV
  * @brief this file handle lea adv related process
  * @{
  */
typedef enum
{
    LEA_ADV_TIMEOUT_CIS               = 0x00,
    LEA_ADV_TIMEOUT_DELEGATOR         = 0x01,
} T_LEA_ADV_TIMEOUT;

typedef enum
{
    LEA_ADV_MODE_PAIRING              = 0x00,
    LEA_ADV_MODE_LINK_LOSS_LINK_BACK  = 0x01,
} T_LEA_ADV_MODE;

typedef enum
{
    LEA_POWER_OFF                = 0x01,
    LEA_ENTER_PAIRING            = 0x02,
    LEA_EXIT_PAIRING             = 0x03,
    LEA_FE_SHAKING_DONE          = 0x04,
    LEA_AFE_SHAKING_DONE         = 0x05,
    LEA_MMI                      = 0x06,
    LEA_ADV_START                = 0x07,
    LEA_ADV_STOP                 = 0x08,
    LEA_ADV_TIMEOUT              = 0x09,
    LEA_CIG_START                = 0x0A,
} T_LEA_ADV_EVENT;

/**
 * @brief  Initialize parameter of advertising data
 *         with TMAP role, csis,...etc. Also setting
 *         adv interval and bt address type.
 * @param  No parameter.
 * @return void
 */
void app_lea_adv_init(void);

/**@brief Start BLE advertising
 *
 * @param[in] mode
 *            using scenario, link le audio pairing mode,
 *            scan delegator.
 * @return void
 */
void app_lea_adv_start(uint8_t mode);

/**@brief Stop BLE advertising
 *
 * @param  No parameter.
 * @return void
 */
void app_lea_adv_stop(void);

/**@brief Update BLE advertising data

 * @param  No parameter.
 * @return void
 */
void app_lea_adv_update(void);

/**@brief Get state of BLE advertising in lea scenario

 * @param  No parameter.
 * @return T_BLE_EXT_ADV_MGR_STATE state of BLE advertising
 */
T_BLE_EXT_ADV_MGR_STATE app_lea_adv_get_state(void);

/**
 * @brief BLE Audio advertising state machine
 *
 * @param[in] event
 *            state machine event T_LEA_ADV_EVENT.
 * @param[in] p_data
 *            pointer to event data.
 * @return void
 */
bool app_lea_mgr_adv_sm(uint8_t event, void *p_data);

/** @} */ /* End of group APP_LEA_ADV_Exported_Functions */
/** End of APP_LEA_ADV
* @}
*/

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
