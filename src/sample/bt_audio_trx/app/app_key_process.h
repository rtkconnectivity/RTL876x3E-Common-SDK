/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_KEY_PROCESS_H_
#define _APP_KEY_PROCESS_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_KEY_PROCESS App Key Process
  * @brief App Key Process
  * @{
  */

/**
    * @brief  key process initial.
    * @param  void
    * @return void
    */
void app_key_init(void);

/**
    * @brief  judge if now going to pairing mode by long press key.
    * @param  void
    * @return is pairing mode or other mode
    */
bool app_key_is_enter_pairing(void);

/**
    * @brief  reset enter pairing variable.
    * @param  void
    * @return void
    */
void app_key_reset_enter_pairing(void);

/**
    * @brief  key single click process.
    * @param  key:This parameter is from KEY0_MASK to KEY7_MASK
    * @return void
    */
void app_key_single_click(uint8_t key);

/**
    * @brief  key module handle message.
    *         when app_io_handle_msg recv msg IO_MSG_GPIO_KEY, it will be called.
    * @param  void
    * @return void
    */
void app_key_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  set long key power off key precessed or release.
    * @param  value: true pressed; flase released.
    * @return void
    */
void app_key_long_key_power_off_pressed_set(bool value);

/**
    * @brief  get long key power off key precessed or release.
    * @param  void
    * @return true pressed; flase released.
    */
bool app_key_long_key_power_off_pressed_get(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_KEY_PROCESS_H_ */
