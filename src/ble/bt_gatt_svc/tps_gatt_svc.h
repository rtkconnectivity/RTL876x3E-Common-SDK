/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _TPS_GATT_SVC_H_
#define _TPS_GATT_SVC_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include <stdint.h>
#include "bt_gatt_svc.h"


/** @defgroup TPS Tx Power Service
  * @brief Tx power service
  * @details

    The Tx Power service uses the Tx Power Level characteristic to expose the current transmit power level
    of a device when in a connection. The Tx Power service contains only a Tx Power Level characteristic.
    The Tx Power Service generally makes up a profile with some other services, such as Proximity, and its
    role is to indicate a device's transmit power level when in a connection.

    Applications shall register Tx Power service during initialization through @ref tps_reg_svc function.

  * @{
  */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup TPS_Exported_Macros TPS Exported Macros
  * @brief
  * @{
  */

#define GATT_UUID_TX_POWER_SERVICE        0x1804
#define GATT_UUID_CHAR_TX_LEVEL           0x2A07

/** @defgroup TPS_Read_Info TPS Read Info
  * @brief  Read characteristic value.
  * @{
  */
#define TPS_READ_TX_POWER_VALUE            1
#define GATT_MSG_TPS_SERVER_READ_TX_POWER  0x01

/** @} */
/** @} End of TPS_Exported_Macros */
/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup TPS_Exported_Types TPS Exported Types
  * @brief
  * @{
  */
/* Add all public types here */

/** @brief BAS server read ind
 *         The message data for GATT_MSG_BAS_SERVER_READ_BATTERY_LEVEL_IND.
*/

/**
 * @brief P_FUN_TPS_SERVER_APP_CB BAS Server Callback Function Point Definition Function
 *        pointer used in BAS server module, to send events to specific server module.
 *        The events @ref BAS_SVC_CB_MSG.
 */
typedef T_APP_RESULT(*P_FUN_TPS_SERVER_APP_CB)(uint16_t conn_handle, uint16_t cid,
                                               uint8_t type, void *p_data);

/** @defgroup TPS_Upstream_Message TPS Upstream Message
  * @brief TPS data struct for notification data to application.
  * @{
  */
/** Message content: @ref TPS_Upstream_Message. */
typedef struct
{
    T_SERVER_ID service_id;
    uint16_t    char_uuid;
    uint16_t    offset;
} T_TPS_SERVER_READ_TX_POWER;

/** @} */

/** @} End of TPS_Exported_Types */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup TPS_Exported_Functions TPS Exported Functions
  * @{
  */

/**
 * @brief       Confirm for read tps characteristic value request.
 *              When application receives GATT_MSG_DIS_SERVER_READ_CHAR_IND, if the read request is accepted,
 *              application shall return APP_RESULT_PENDING or APP_RESULT_SUCCESS, and call this API later.
 *
 * @param[in]   conn_handle    Connection handle of the ACL link.
 * @param[in]   cid            Local CID assigned by Bluetooth stack.
 * @param[in]   service_id     Service id.
 * @param[in]   char_uuid      Characteristic UUID.
 * @param[in]   offset         Offset.
 * @param[in]   value_len      Characteristic value length.
 * @param[in]   p_value        Characteristic value.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    T_APP_RESULT app_tps_service_callback(uint16_t conn_handle, uint16_t cid, uint8_t type,
                                           void *p_data)
    {
        switch (type)
        {
        case GATT_MSG_TPS_SERVER_READ_TX_POWER:
        {
          T_TPS_SERVER_READ_TX_POWER *p_tps_read_tx_power;
          p_tps_read_tx_power = (T_TPS_SERVER_READ_TX_POWER *)p_data;

          if(p_tps_read_tx_power->char_uuid == GATT_UUID_CHAR_TX_LEVEL)
          {
              uint8_t tx_power_value;
              tps_char_read_confirm(conn_handle, cid, p_tps_read_tx_power->service_id,
                                   p_tps_read_tx_power->char_uuid, p_tps_read_tx_power->offset,
                                   sizeof(tx_power_value), &tx_power_value);

          }
        }
        break;

        default:
          break;
            }
        }

        return APP_RESULT_SUCCESS;
    }
 * \endcode
 */
bool tps_char_read_confirm(uint16_t conn_handle, uint16_t cid, uint8_t service_id,
                           uint16_t char_uuid, uint16_t offset,
                           uint16_t value_len, uint8_t *p_value);

/**
  * @brief Add tx power service to the Bluetooth Host.
  *
  * @param[in]   app_cb  Callback when service attribute was read, write or CCCD update.
  * @return Service ID generated by the Bluetooth Host: @ref T_SERVER_ID.
  * @retval 0xFF Operation failure.
  * @retval others Service ID assigned by Bluetooth Host.
  *
  * <b>Example usage</b>
  * \code{.c}
    T_APP_RESULT app_tps_service_callback(uint16_t conn_handle, uint16_t cid,
                                          uint8_t type, void *p_data)
    {
      APP_PRINT_INFO3("app_tps_service_callback: conn_handle 0x%x, cid 0x%x, type 0x%x",
                      conn_handle, cid, type);

        switch (type)
        {
        case GATT_MSG_TPS_SERVER_READ_TX_POWER:
        {
          T_TPS_SERVER_READ_TX_POWER *p_tps_read_tx_power;
          p_tps_read_tx_power = (T_TPS_SERVER_READ_TX_POWER *)p_data;

          if(p_tps_read_tx_power->char_uuid == GATT_UUID_CHAR_TX_LEVEL)
          {
              uint8_t tx_power_value;
              tps_char_read_confirm(conn_handle, cid, p_tps_read_tx_power->service_id,
                                   p_tps_read_tx_power->char_uuid, p_tps_read_tx_power->offset,
                                   sizeof(tx_power_value), &tx_power_value);

          }
        }
        break;

        return APP_RESULT_SUCCESS;
    }

    void profile_init()
    {
        server_init(service_num);
        tps_id = tps_add_service(app_handle_profile_message);
    }
  * \endcode
  */
T_SERVER_ID tps_reg_srv(P_FUN_TPS_SERVER_APP_CB app_cb);

/** @} End of TPS_Exported_Functions */

/** @} End of TPS */


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif /* _TPS_H_ */
