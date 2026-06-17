/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_GFPS_FLAGS_CFG_H_
#define _APP_GFPS_FLAGS_CFG_H_

#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/**
 * @brief GFPS basic feature support
 *
 */
#define F_GFPS_COMMON_BASIC_FEATURE_SUPPORT (1 && CONFIG_REALTEK_GFPS_FEATURE_SUPPORT)

/**
 * @brief GFPS extend feature find hub support
 *
 */
#define F_GFPS_COMMON_FINDER_SUPPORT        (1 && CONFIG_REALTEK_GFPS_FINDER_SUPPORT)

/**
 * @brief GFPS extend feature ble support(include BLE audio)
 *
 */
#define F_GFPS_COMMON_LE_DEVICE_SUPPORT     (1 && CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT)

/**
 * @brief if device have two components
 *
 */
#define F_GFPS_COMMON_TWS_MODE              0

/**
 * @brief if device type is location tracker
 *
 */
#define F_GFPS_COMMON_LOCATION_TRACKER      1

/**
 * @brief if support PERIODIC_WAKEUP to start finder adv
 * if app want to use this feature F_APP_PERIODIC_WAKEUP in app_flags.h shall also set to 1
 */
#define F_GFPS_COMMON_PERIODIC_WAKEUP       0

/**
 * @brief if support battery info report, only TWS mode need
 *
 */
#define F_GFPS_COMMON_BATTERY_INFO_REPORT   (1 && F_GFPS_COMMON_TWS_MODE)

/**
 * @brief if support LE L2CAP channel, only lea support
 *
 */
#define F_GFPS_COMMON_LE_L2CAP_CHANNEL      (1 && F_GFPS_COMMON_LE_DEVICE_SUPPORT && !F_GFPS_COMMON_LOCATION_TRACKER)

/** End of _APP_GFPS_FLAGS_CFG_H_
* @}
*/

#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
