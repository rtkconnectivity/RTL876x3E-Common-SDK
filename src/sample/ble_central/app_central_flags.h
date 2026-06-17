/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_CENTRAL_FLAGS_H_
#define _APP_CENTRAL_FLAGS_H_

/** @defgroup  CENTRAL_Config Central App Configuration
    * @brief This file is used to config app functions.
    * @{
    */
/*============================================================================*
 *                              Constants
 *============================================================================*/
/**
 * @brief Config DLPS
 * 0-Disable DLPS, 1-Enable DLPS
 */
#define F_DLPS_EN 1

/**
 * @brief Config APP LE link number
 *
 */
#define APP_MAX_LINKS 2

/** @brief  Config GATT services storage
 *  0-Not save, 1-Save to flash
 * If configure to 1, the GATT services discovery results will save to the flash.
 */
#define F_BT_GATT_SRV_HANDLE_STORAGE 0

/** @} */ /* End of group CENTRAL_Config */
#endif
