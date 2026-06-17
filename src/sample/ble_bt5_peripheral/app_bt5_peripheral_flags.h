/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT5_PERIPHERAL_FLAGS_H_
#define _APP_BT5_PERIPHERAL_FLAGS_H_


/** @defgroup  BT5_PERIPH_Config BT5 Peripheral App Configuration
    * @brief This file is used to configure app functions.
    * @{
    */
/*============================================================================*
 *                              Constants
 *============================================================================*/

/**
 * @brief Configure Advertising PHY
 *
 */
#define ADVERTISING_PHY APP_PRIMARY_1M_SECONDARY_2M

/**
 * @brief Configure support coded phy or not
 *
 */
#define F_BT_LE_CODED_PHY_SUPPORT 0

/**
 * @brief Configure coding scheme of LE Coded PHY: 0 - S = 2, 1 - S = 8
 *
 */
#define LE_CODED_PHY_S8 (F_BT_LE_CODED_PHY_SUPPORT && 0)

/**
 * @brief Configure APP LE link number
 *
 */
#define APP_MAX_LINKS 1

/**
 * @brief  Configure DLPS: 0 - Disable DLPS, 1 - Enable DLPS
 *
 */
#define F_DLPS_EN 1

/**
 * @brief 1 - Enable LE Advertising Extensions
 *
 */
#define F_BT_LE_5_0_AE_ADV_SUPPORT 1

/** @} */ /* End of group BT5_PERIPH_Config */
#endif
