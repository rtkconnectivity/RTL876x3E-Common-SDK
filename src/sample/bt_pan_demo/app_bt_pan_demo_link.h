/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_PAN_DEMO_LINK_H_
#define _APP_BT_PAN_DEMO_LINK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_PAN_DEMO_LINK BT PAN Demo APP Link
  * @brief BT PAN Demo APP Link
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_BT_PAN_DEMO_LINK_Exported_Macros BT PAN Demo Link Macros
  * @brief
  * @{
  */

/** @brief  Define links number. range: 0-4 */
#define APP_BT_PAN_DEMO_MAX_BR_LINK_NUM                 4

/** End of APP_BT_PAN_DEMO_LINK_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup APP_BT_PAN_DEMO_LINK_Exported_Types BT PAN Demo Link Types
  * @brief
  * @{
  */

/**  @brief  APP's Bluetooth BR/EDR link connection database */
typedef struct
{
    uint8_t             bd_addr[6];
    bool                used;
    uint8_t             id;
} T_APP_BT_PAN_DEMO_LINK;

/**  @brief  App define global app data structure */
typedef struct
{
    T_APP_BT_PAN_DEMO_LINK     br_link[APP_BT_PAN_DEMO_MAX_BR_LINK_NUM];
    uint8_t                    local_addr[6];
    uint8_t                    remote_addr[6];
} T_APP_BT_PAN_DEMO_APP_DB;
/** End of APP_BT_PAN_DEMO_LINK_Exported_Types
* @}
*/

extern T_APP_BT_PAN_DEMO_APP_DB app_db;

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BT_PAN_DEMO_LINK_Exported_Functions BT PAN Demo Link Exported Functions
  * @brief
  * @{
  */

/**
 * @brief     Find the APP BT PAN data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP BT PAN data instance. If returned handle is NULL, the APP BT PAN
 *            data instance was failed to create.
 */
T_APP_BT_PAN_DEMO_LINK *app_bt_pan_demo_find_link(uint8_t *bd_addr);

/**
 * @brief     Allocate the APP BT PAN data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP BT PAN data instance. If returned handle is NULL, the APP BT PAN
 *            data instance was failed to create.
 */
T_APP_BT_PAN_DEMO_LINK *app_bt_pan_demo_alloc_link(uint8_t *bd_addr);

/**
 * @brief     Free the APP BT PAN data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP BT PAN data instance. If returned handle is NULL, the APP BT PAN
 *            data instance was failed to create.
 */
bool app_bt_pan_demo_free_link(T_APP_BT_PAN_DEMO_LINK *p_link);

/** @} End of APP_BT_PAN_DEMO_LINK_Exported_Functions */

/** @} End of APP_BT_PAN_DEMO_LINK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_PAN_DEMO_LINK_H_ */
