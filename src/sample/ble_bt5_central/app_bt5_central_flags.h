/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT5_CENTRAL_FLAGS_H_
#define _APP_BT5_CENTRAL_FLAGS_H_


/** @defgroup  BT5_CENTRAL_Config BT5 Central App Configuration
    * @brief This file is used to config app functions.
    * @{
    */
/*============================================================================*
 *                              Constants
 *============================================================================*/
/**
 * @brief  Configure coding scheme of LE Coded PHY: 0 - S = 2, 1 - S = 8
 *
 */
#define LE_CODED_PHY_SUPPORT 0
#if LE_CODED_PHY_SUPPORT
#define LE_CODED_PHY_S8 0
#endif

/**
 * @brief  Configure APP LE link number
 *
 */
#define APP_MAX_LINKS 1

/**
 * @brief Configure APP to recombine advertising data:
 * 0 - Disable recombine advertising data feature, 1 - recombine advertising data
 *
 */
#define APP_RECOMBINE_ADV_DATA 1

/**
 * @brief 1 - Enable Extended Advertising Feature
 *
 */
#define F_BT_LE_5_0_AE_ADV_SUPPORT  1

/**
 * @brief  1 - Enable Extended scan Feature
 *
 */
#define F_BT_LE_5_0_AE_SCAN_SUPPORT 1

/**
 * @brief 1 - Enable set phy Feature
 *
 */
#define F_BT_LE_5_0_SET_PHY_SUPPORT 1
/** @} */ /* End of group BT5_CENTRAL_Config */
#endif
