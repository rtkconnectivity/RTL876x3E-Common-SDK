/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_SCATTERNET_FLAGS_H_
#define _APP_SCATTERNET_FLAGS_H_

/** @defgroup  SCATTERNET_Config Scatternet App Configuration
    * @brief This file is used to config app functions.
    * @{
    */
/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @brief  Config APP LE link number */
#define APP_MAX_LINKS 2

/** @brief  Config airplane mode support: 0-Not built in, 1-built in, use user command to set*/
#define F_BT_AIRPLANE_MODE_SUPPORT 0

/** @brief  Config device name characteristic and appearance characteristic property: 0-Not writeable, 1-writeable, save to flash*/
#define F_BT_GAPS_CHAR_WRITEABLE 0

/** @brief  Config set physical: 0-Not built in, 1-built in, use user command to set*/
#define F_BT_LE_5_0_SET_PHY_SUPPORT 1

/** @brief  Config local address type: 0-pulic address, 1-static random address */
#define F_BT_LE_USE_STATIC_RANDOM_ADDR 0

#define F_APP_GATT_SERVER_EXT_API_SUPPORT 1
/** @} */ /* End of group SCATTERNET_Config */
#endif
