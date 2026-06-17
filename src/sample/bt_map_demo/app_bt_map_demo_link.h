/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_MAP_DEMO_LINK_H_
#define _APP_MAP_DEMO_LINK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_MAP_DEMO_LINK BT Audio APP Link
  * @brief BT Audio APP Link
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_MAP_DEMO_LINK_Exported_Macros BT Audio Link Macros
  * @brief
  * @{
  */

/** @brief  Define links number. range: 0-4 */
#define APP_MAP_DEMO_MAX_BR_LINK_NUM                 4

/** End of APP_MAP_DEMO_LINK_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup APP_MAP_DEMO_LINK_Exported_Types BT Audio Link Types
  * @brief
  * @{
  */

/**  @brief  APP's Bluetooth BR/EDR link connection database */
typedef struct
{
    uint8_t             bd_addr[6];
    bool                used;
    uint8_t             id;
} T_APP_MAP_DEMO_LINK;

/**  @brief  App define global app data structure */
typedef struct
{
    T_APP_MAP_DEMO_LINK  br_link[APP_MAP_DEMO_MAX_BR_LINK_NUM];
    uint8_t                 a2dp_role;
    uint8_t                 hfp_role;
    uint8_t                 remote_addr[6];
} T_APP_MAP_DEMO_APP_DB;
/** End of APP_MAP_DEMO_LINK_Exported_Types
* @}
*/

extern T_APP_MAP_DEMO_APP_DB app_db;

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_MAP_DEMO_LINK_Exported_Functions BT Audio Link Exported Functions
  * @brief
  * @{
  */

/**
 * @brief     Find the APP SPP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP SPP data instance. If returned handle is NULL, the APP SPP
 *            data instance was failed to create.
 */
T_APP_MAP_DEMO_LINK *app_map_demo_find_link(uint8_t *bd_addr);

/**
 * @brief     Allocate the APP SPP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP SPP data instance. If returned handle is NULL, the APP SPP
 *            data instance was failed to create.
 */
T_APP_MAP_DEMO_LINK *app_map_demo_alloc_link(uint8_t *bd_addr);

/**
 * @brief     Free the APP SPP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP SPP data instance. If returned handle is NULL, the APP SPP
 *            data instance was failed to create.
 */
bool app_map_demo_free_link(T_APP_MAP_DEMO_LINK *p_link);

/** @} End of APP_MAP_DEMO_LINK_Exported_Functions */

/** @} End of APP_MAP_DEMO_LINK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_MAP_DEMO_LINK_H_ */
