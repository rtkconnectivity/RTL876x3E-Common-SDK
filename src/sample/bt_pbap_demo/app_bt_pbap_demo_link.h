/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _PBAP_DEMO_LINK_H_
#define _PBAP_DEMO_LINK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup PBAP_DEMO_LINK PBAP Demo APP Link
  * @brief PBAP Demo APP Link
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup PBAP_DEMO_LINK_Exported_Macros PBAP Demo Link Macros
  * @brief
  * @{
  */

/** @brief  Define links number. range: 0-4 */
#define PBAP_DEMO_MAX_BR_LINK_NUM                 4

/** End of PBAP_DEMO_LINK_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup PBAP_DEMO_LINK_Exported_Types PBAP Demo Link Types
  * @brief
  * @{
  */

/**  @brief  APP's Bluetooth BR/EDR link connection database */
typedef struct
{
    uint8_t         bd_addr[6];
    bool            used;
    uint8_t         id;
} T_PBAP_DEMO_LINK;

/**  @brief  App define global app data structure */
typedef struct
{
    T_PBAP_DEMO_LINK               pbap_link[PBAP_DEMO_MAX_BR_LINK_NUM];
} T_PBAP_DEMO_APP_DB;
/** End of PBAP_DEMO_LINK_Exported_Types
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup PBAP_DEMO_LINK_Exported_Functions PBAP Demo Link Exported Functions
  * @brief
  * @{
  */

/**
 * @brief     Find the APP PBAP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP PBAP data instance. If returned handle is NULL, the APP PBAP
 *            data instance was failed to create.
 */
T_PBAP_DEMO_LINK *pbap_demo_find_link(uint8_t *bd_addr);

/**
 * @brief     Allocate the APP PBAP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP PBAP data instance. If returned handle is NULL, the APP PBAP
 *            data instance was failed to create.
 */
T_PBAP_DEMO_LINK *pbap_demo_alloc_link(uint8_t *bd_addr);

/**
 * @brief     Free the APP PBAP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP PBAP data instance. If returned handle is NULL, the APP PBAP
 *            data instance was failed to create.
 */
bool pbap_demo_free_link(T_PBAP_DEMO_LINK *p_link);

/** @} End of PBAP_DEMO_LINK_Exported_Functions */

/** @} End of PBAP_DEMO_LINK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PBAP_DEMO_LINK_H_ */
