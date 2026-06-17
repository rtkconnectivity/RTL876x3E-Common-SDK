/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _PBAP_DEMO_APP_H_
#define _PBAP_DEMO_APP_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup PBAP_DEMO_APP PBAP Demo APP
  * @brief PBAP Demo APP
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup PBAP_DEMO_APP_Exported_Macros PBAP Demo APP Exported Macros
  * @brief
  * @{
  */

/**
* @brief Define PBAP ExternalAccessoryProtocol identifuer.
* @note  All ExternalAccessoryProtocol identifuer must be unique.
*/
#define EA_PROTOCOL_ID_RTK      10

/** End of PBAP_DEMO_APP_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup PBAP_DEMO_APP_Exported_Functions PBAP Demo APP Exported Functions
  * @brief
  * @{
  */

/**
    * @brief  PBAP profile init
    * @param  void
    * @return void
    */
void pbap_demo_app_init(void);

/**
    * @brief  Start PBAP connect procedure
    * \param[in] bd_addr    Remote BT address.
    * @return void
    */
bool pbap_demo_conn_start(uint8_t *bd_addr);

/**
    * @brief  Start PBAP disconnect procedure
    * @param  void
    * @return void
    */
bool pbap_demo_disconnect_start(void);

/**
    * @brief  Send PBAP data
    * @param  p_param    phone book string
    * @return void
    */
bool pbap_demo_app_set_phone_book(char *p_param);

/**
    * @brief  Send PBAP data
    * @param  p_number    call number
    * @return void
    */
bool pbap_demo_app_pull_caller_id_name(char *p_number);

/** @} End of PBAP_DEMO_APP_Exported_Functions */

/** @} End of PBAP_DEMO_APP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PBAP_DEMO_APP_H_ */
