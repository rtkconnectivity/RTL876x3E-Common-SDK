/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _IAP_DEMO_LINK_H_
#define _IAP_DEMO_LINK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup IAP_DEMO_LINK iAP Demo APP Link
  * @brief iAP Demo APP Link
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup IAP_DEMO_LINK_Exported_Macros iAP Demo Link Macros
  * @brief
  * @{
  */

/** @brief  Define links number. range: 0-4 */
#define IAP_DEMO_MAX_BR_LINK_NUM                 4

/** End of IAP_DEMO_LINK_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup IAP_DEMO_LINK_Exported_Types iAP Demo Link Types
  * @brief
  * @{
  */

/**  @brief  APP's Bluetooth BR/EDR link connection database */
typedef struct
{
    uint8_t         bd_addr[6];
    bool            used;
    uint8_t         id;

    uint16_t        rfc_iap_frame_size;//iap tx/rx mtu size.
    bool            authen_flag;
    bool            session_connected;
    uint16_t        session_id;
} T_IAP_DEMO_LINK;

/**  @brief  App define global app data structure */
typedef struct
{
    T_IAP_DEMO_LINK               iap_link[IAP_DEMO_MAX_BR_LINK_NUM];
} T_IAP_DEMO_APP_DB;
/** End of IAP_DEMO_LINK_Exported_Types
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup IAP_DEMO_LINK_Exported_Functions iAP Demo Link Exported Functions
  * @brief
  * @{
  */

/**
 * @brief     Find the APP iAP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP iAP data instance. If returned handle is NULL, the APP iAP
 *            data instance was failed to create.
 */
T_IAP_DEMO_LINK *iap_demo_find_link(uint8_t *bd_addr);

/**
 * @brief     Allocate the APP iAP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP iAP data instance. If returned handle is NULL, the APP iAP
 *            data instance was failed to create.
 */
T_IAP_DEMO_LINK *iap_demo_alloc_link(uint8_t *bd_addr);

/**
 * @brief     Free the APP iAP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP iAP data instance. If returned handle is NULL, the APP iAP
 *            data instance was failed to create.
 */
bool iap_demo_free_link(T_IAP_DEMO_LINK *p_link);

/** @} End of IAP_DEMO_LINK_Exported_Functions */

/** @} End of IAP_DEMO_LINK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IAP_DEMO_LINK_H_ */
