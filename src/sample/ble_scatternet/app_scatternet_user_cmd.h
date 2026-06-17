/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SCATTERNET_USER_CMD_H_
#define _APP_SCATTERNET_USER_CMD_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "data_uart.h"
#include "user_cmd_parse.h"

/** @defgroup SCATTERNET_CMD Scatternet User Command
  * @brief Scatternet User Command
  * @{
  */
extern const T_USER_CMD_TABLE_ENTRY user_cmd_table[];
extern T_USER_CMD_IF    user_cmd_if;

/** End of SCATTERNET_CMD
* @}
*/

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
