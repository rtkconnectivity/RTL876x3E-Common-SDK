/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_TRANSFER_CFG_H_
#define _APP_TRANSFER_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** @defgroup APP_TRANSFER App Transfer
  * @brief App Transfer
  * @{
  */

/** @brief  Read only configurations for transfer */
typedef struct
{
    uint8_t data_uart_baud_rate;
    uint8_t dt_resend_num;
    uint8_t enable_embedded_cmd : 1;
    uint8_t report_uart_event_only_once : 1;

} T_APP_TRANSFER_CFG;


extern T_APP_TRANSFER_CFG app_transfer_cfg;


/**
    * @brief  Transfer config module init
    * @param  void
    * @return void
    */
void app_transfer_cfg_init(void);


/** End of APP_TRANSFER
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
