/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _HID_OVER_ISO_GATT_CLIENT_H_
#define _HID_OVER_ISO_GATT_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include <stdint.h>
#include <stdbool.h>
#include "bt_gatt_client.h"
#include "hid_over_iso_def.h"

/** @defgroup HID_OVER_ISO_GATT_CLIENT HID over ISO Service GATT Client
  * @brief HID over ISO client
  * @details
     Application shall register HID over ISO client when initialization through @ref hid_over_iso_client_init function.

  * \endcode
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup HID_OVER_ISO_GATT_CLIENT_Exported_Macros HID over ISO GATT Client Exported Macros
  * @brief
  * @{
  */

/** @defgroup HID_OVER_ISO_CLT_CB_MSG
  * @brief HID over ISO client callback messages
  * @{
  */
#define GATT_MSG_HID_OVER_ISO_CLIENT_DIS_DONE                           0x00
#define GATT_MSG_HID_OVER_ISO_CLIENT_READ_PROPERTIES_RESULT             0x01
#define GATT_MSG_HID_OVER_ISO_CLIENT_LE_HID_OPERATION_MODE_INDICATION   0x02
#define GATT_MSG_HID_OVER_ISO_CLIENT_WRITE_LE_HID_OPERATION_MODE_RESULT 0x03
#define GATT_MSG_HID_OVER_ISO_CLIENT_CCCD_CFG_RESULT                    0x04
/** @} End of HID_OVER_ISO_CLT_CB_MSG */
/** End of HID_OVER_ISO_GATT_CLIENT_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup HID_OVER_ISO_GATT_CLIENT_Exported_Types HID over ISO GATT Client Exported Types
  * @brief
  * @{
  */
/**
 * @brief P_FUN_HID_OVER_ISO_CLIENT_APP_CB HID over ISO Client Callback Function Point Definition Function
 *        pointer used in HID over ISO client module, to send events to specific client module.
 *        The events @ref HID_OVER_ISO_CLT_CB_MSG.
 */
typedef T_APP_RESULT(*P_FUN_HID_OVER_ISO_CLIENT_APP_CB)(uint16_t conn_handle, uint8_t type,
                                                        void *p_data);

/** @brief HID over ISO client discovery result
 *         The message data for GATT_MSG_HID_OVER_ISO_CLIENT_DIS_DONE.
*/
typedef struct
{
    bool     is_found;
    bool     load_from_ftl;
    bool     is_le_hid_operation_mode_indication_support;
} T_HID_OVER_ISO_CLIENT_DIS_DONE;

/** @brief HID over ISO client read result
 *         The message data for GATT_MSG_HID_OVER_ISO_CLIENT_READ_PROPERTIES_RESULT.
*/
typedef struct
{
    uint16_t  cause;
    T_HID_OVER_ISO_PROPERTIES_VALUE properties;
} T_HID_OVER_ISO_CLIENT_READ_PROPERTIES_RESULT;

/** @brief HID over ISO client write result
 *         The message data for GATT_MSG_HID_OVER_ISO_CLIENT_WRITE_LE_HID_OPERATION_MODE_RESULT.
*/
typedef struct
{
    uint16_t  cause;
} T_HID_OVER_ISO_CLIENT_WRITE_LE_HID_OPERATION_MODE_RESULT;

/** @brief HID over ISO client indication data
 *         The message data for GATT_MSG_HID_OVER_ISO_CLIENT_LE_HID_OPERATION_MODE_INDICATION.
*/
typedef struct
{
    T_LE_HID_OPERATION_MODE_VALUE  operation;
} T_HID_OVER_ISO_CLIENT_LE_HID_OPERATION_MODE_INDICATION;

/** @brief HID over ISO client configure CCCD data info
 *         The message data for GATT_MSG_HID_OVER_ISO_CLIENT_CCCD_CFG_RESULT.
*/
typedef struct
{
    uint16_t   cause;
    uint16_t   cccd_cfg;
} T_HID_OVER_ISO_CLIENT_CCCD_CFG_RESULT;
/** End of HID_OVER_ISO_GATT_CLIENT_Exported_Types
* @}
*/


/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup HID_OVER_ISO_GATT_CLIENT_Exported_Functions HID over ISO GATT Client Exported Functions
  * @brief
  * @{
  */

/**
  * @brief      Add HID over ISO client to application.
  * @param[in]  app_cb   Pointer of app callback function to handle specific client module data.
  * @return Result of add the specific client module.
  * @retval true  Add client module success.
  * @retval false Add client module failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        if(hid_over_iso_client_init(app_client_callback))
        {
            APP_PRINT_ERROR0("app_le_profile_init: add HID over ISO client fail!");
        }
    }
  * \endcode
  */
bool hid_over_iso_client_init(P_FUN_HID_OVER_ISO_CLIENT_APP_CB app_cb);

/**
  * @brief      Config the HID over ISO service CCCD data.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @param[in]  enable          Whether to enable CCCD.
  * \arg    true    Enable CCCD.
  * \arg    false   Disable CCCD.
  * @return Result of config the HID over ISO service CCCD data.
  * @retval true  Config the HID over ISO service CCCD data success.
  * @retval false Config the HID over ISO service CCCD data failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void test(uint16_t conn_handle)
    {
        hid_over_iso_client_cfg_cccd(conn_handle, true);
    }
  * \endcode
  */
bool hid_over_iso_client_cfg_cccd(uint16_t conn_handle, bool enable);

/**
  * @brief  Used by application, to read HID over ISO properties.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void test(uint16_t conn_handle)
    {
        bool ret = false;
        ret = hid_over_iso_client_read_properties(conn_handle);
        ......
    }
  * \endcode
  */
bool hid_over_iso_client_read_properties(uint16_t conn_handle);

/**
  * @brief  Used by application, to write LE HID Operation Mode characteristic.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @param[in]  p_operation     LE HID Operation Mode value.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool hid_over_iso_client_write_le_hid_operation_mode(uint16_t conn_handle,
                                                     T_LE_HID_OPERATION_MODE_VALUE *p_operation);

/** @} End of HID_OVER_ISO_GATT_CLIENT_Exported_Functions */

/** @} End of HID_OVER_ISO_GATT_CLIENT */


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* HID_OVER_ISO_GATT_CLIENT_H */
