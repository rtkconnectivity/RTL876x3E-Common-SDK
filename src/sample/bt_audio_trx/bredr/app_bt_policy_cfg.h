/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_POLICY_CFG_H_
#define _APP_BT_POLICY_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_BT_POLICY App BT Policy
  * @brief App BT Policy
  * @{
  */
/** @brief  Read only configurations for bt policy */
typedef struct
{
    uint8_t pin_code_size;
    uint8_t pin_code[8];

    uint8_t enable_multi_sink_devices : 1;
    uint8_t max_legacy_multilink_devices;
    uint8_t link_scenario;  //linkback scenario

    uint8_t disable_multilink_preemptive : 1;
    uint8_t maximum_linkback_number : 4;
    uint8_t enable_multi_sco_disc_resume : 1;
    uint8_t enable_not_discoverable_when_linkback : 1;
    uint8_t enable_multi_outband_call_tone: 1;
    uint8_t enable_always_discoverable : 1; //used in multi-link app
    uint8_t enable_discoverable_in_standby_mode : 1;
    uint8_t enter_pairing_while_only_one_device_connected: 1;

    uint16_t timer_linkback_timeout;
    uint16_t timer_pairing_timeout;
    uint16_t timer_link_avrcp;
    uint16_t timer_link_back_loss;

} T_APP_BT_POLICY_CFG_CONST;

typedef struct
{
    uint8_t enable_multi_link : 1;
} T_APP_BT_POLICY_CFG;


extern const T_APP_BT_POLICY_CFG_CONST app_bt_policy_cfg;
extern T_APP_BT_POLICY_CFG app_bt_policy_cfg_nv;

/**
    * @brief  BT policy config module init
    * @param  void
    * @return void
    */
void app_bt_policy_cfg_init(void);

/** End of APP_BT_POLICY
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
