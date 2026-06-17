/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_HID_DEMO_LINK_H_
#define _APP_BT_HID_DEMO_LINK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_HID_DEMO_LINK BT HID Demo APP Link
  * @brief BT HID Demo APP Link
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_BT_HID_DEMO_LINK_Exported_Macros BT HID Demo Link Macros
  * @brief
  * @{
  */

/** @brief  Define links number. range: 0-4 */
#define APP_BT_HID_DEMO_MAX_BR_LINK_NUM                 4

/** End of APP_BT_HID_DEMO_LINK_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup APP_BT_HID_DEMO_LINK_Exported_Types BT HID Demo Link Types
  * @brief
  * @{
  */

/**  @brief  APP's Bluetooth BR/EDR link connection database */
typedef struct t_app_bt_hid_demo_link
{
    struct t_app_bt_hid_demo_link *next;
    uint8_t                        bd_addr[6];
    bool                           hid_conn_cmpl;
} T_APP_BT_HID_DEMO_LINK;

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BT_HID_DEMO_LINK_Exported_Functions BT HID Demo Link Exported Functions
  * @brief
  * @{
  */

/**
 * @brief     Find the APP BT HID data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP BT HID data instance. If returned handle is NULL, the APP BT HID
 *            data instance was failed to create.
 */
T_APP_BT_HID_DEMO_LINK *app_bt_hid_demo_find_link(uint8_t *bd_addr);

/**
 * @brief     Allocate the APP BT HID data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP BT HID data instance. If returned handle is NULL, the APP BT HID
 *            data instance was failed to create.
 */
T_APP_BT_HID_DEMO_LINK *app_bt_hid_demo_alloc_link(uint8_t *bd_addr);

/**
 * @brief     Free the APP BT HID data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP BT HID data instance. If returned handle is NULL, the APP BT HID
 *            data instance was failed to create.
 */
void app_bt_hid_demo_free_link(T_APP_BT_HID_DEMO_LINK *p_link);

void app_bt_hid_demo_db_init(void);

/** @} End of APP_BT_HID_DEMO_LINK_Exported_Functions */

/** @} End of APP_BT_HID_DEMO_LINK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_HID_DEMO_LINK_H_ */
