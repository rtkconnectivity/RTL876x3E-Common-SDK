/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_UART_H_
#define _APP_UART_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_UART APP UART.
  * @brief app uart event handle and implementation
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_UART_Exported_Macros App UART Macros
    * @{
    */

/** End of APP_UART_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_UART_Exported_Types App Console UART Types
    * @{
    */
typedef struct
{
    uint8_t             id;
    uint16_t            rx_count;
    uint16_t            rx_process_offset;
    uint8_t             *rx_buffer;
    uint32_t            rx_w_idx;
    uint32_t            rx_buf_size;
} T_UART_CMD;

/**  @brief  App define global app data structure */

/** End of APP_UART_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_UART_Exported_Variables App Console UART Variables
    * @{
    */


/** End of APP_UART_Exported_Variables
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_UART_Exported_Variables App UART Functions
    * @{
    */
void app_uart_init(void);
void app_uart_parser(uint8_t index);
void app_uart_handle_rx_msg(T_IO_MSG *msg);

/** End of APP_UART_Exported_Variables
    * @}
    */

/** End of APP_UART
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_UART_H_ */
