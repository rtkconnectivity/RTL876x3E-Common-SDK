/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_COM_GAP_H_
#define _APP_LEA_CAP_COM_GAP_H_

#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "app_msg.h"

/** @defgroup APP_LEA_CAP_COM CAP Commander APP
  * @brief CAP Commander APP
  * @{
  */
/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief    All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void app_handle_io_msg(T_IO_MSG io_msg);

/**
 * @brief app_gap_init
 *        register gap message callback and initialize ble manager module.
 */
void app_gap_init(void);

void app_lea_add_conn_dev(uint8_t addr_type, uint8_t *bd_addr);
/** End of _APP_LEA_CAP_COM_GAP_H_
* @}
*/
#ifdef __cplusplus
}
#endif
#endif
