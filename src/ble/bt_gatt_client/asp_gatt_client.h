/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _ASP_GATT_CLIENT_H_
#define _ASP_GATT_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include <stdint.h>
#include <stdbool.h>
#include "profile_client.h"
#include "bt_gatt_client.h"



/** @defgroup ASP_GATT_CLIENT Asp Service Client
  * @brief ASP Gatt client
  * @details
     Application shall register hids client when initialization through @ref asp_add_client function.

     Application shall handle callback function registered by asp_add_client.
  * \code{.c}
    T_APP_RESULT app_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
    {
        T_APP_RESULT  result = APP_RESULT_SUCCESS;
        if (client_id == asp_client_id)
        {
            T_ASP_CLIENT_CB_DATA *p_asp_cb_data = (T_ASP_CLIENT_CB_DATA *)p_data;
            switch (p_asp_cb_data->cb_type)
            {
            case ASP_CLIENT_CB_TYPE_DISC_STATE:
                switch (p_asp_cb_data->cb_content.disc_state)
                {
                case DISC_ASP_DONE:
                ......
        }
    }
  * \endcode
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup ASP_GATT_CLIENT_Exported_Macros Asp Gatt Client Exported Macros
  * @brief
  * @{
  */

/** @brief  Define links number. range: 0-4 */
#define ASP_MAX_LINKS  1

extern uint8_t asp_service_uuid[16];

/** End of ASP_GATT_CLIENT_Exported_Macros
* @}
*/

typedef T_APP_RESULT(*P_FUN_CLIENT_UUID128_GENERAL_APP_CB)(uint16_t conn_handle, uint8_t *srv_uuid,
                                                           void *p_data);


/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup ASP_CFU_CLIENT_Exported_Types Asp Gatt Client Exported Types
  * @brief
  * @{
  */

/** @brief ASP client discovery state*/
typedef enum
{
    DISC_ASP_IDLE,
    DISC_ASP_START,
    DISC_ASP_DONE,
    DISC_ASP_FAILED
} T_ASP_DISC_STATE;

/** @brief ASP client notify type*/
typedef enum
{
    ASP_NOTIFY_9A,
    ASP_NOTIFY_9B
} T_ASP_NOTIFY_TYPE;

/** @brief ASP client notify type*/
typedef struct
{
    T_ASP_NOTIFY_TYPE  notify_type;
    uint16_t data_len;
    uint8_t *p_data;
} T_ASP_NOTIFY_DATA;

/** @brief ASP client write type*/
typedef enum
{
    ASP_WRITE_9A,
} T_ASP_WRTIE_TYPE;

/** @brief ASP client write result*/
typedef struct
{
    T_ASP_WRTIE_TYPE type;
    uint16_t cause;
} T_ASP_WRITE_RESULT;

/** @brief ASP client read type*/
typedef enum
{
    ASP_READ_9A,
    ASP_READ_9B,
} T_ASP_READ_TYPE;

/** @brief ASP client read result*/
typedef struct
{
    T_ASP_READ_TYPE type;
    uint16_t data_len;
    uint8_t *p_data;
    uint16_t cause;
} T_ASP_READ_RESULT;

/** @brief ASP client callback type*/
typedef enum
{
    ASP_CLIENT_CB_TYPE_DISC_STATE,          //!< Discovery procedure state, done or pending.
    ASP_CLIENT_CB_TYPE_READ_RESULT,         //!< Read request's result data, responsed from server.
    ASP_CLIENT_CB_TYPE_WRITE_RESULT,        //!< Write request result, success or fail.
    ASP_CLIENT_CB_TYPE_NOTIF_IND_RESULT,    //!< Notification or indication data received from server.
} T_ASP_CLIENT_CB_TYPE;

/** @brief ASP client callback content*/
typedef union
{
    T_ASP_DISC_STATE disc_state;
    T_ASP_READ_RESULT read_result;
    T_ASP_NOTIFY_DATA notify_data;
    T_ASP_WRITE_RESULT write_result;
} T_ASP_CLIENT_CB_CONTENT;

/** @brief ASP client callback data*/
typedef struct
{
    T_ASP_CLIENT_CB_TYPE     cb_type;
    T_ASP_CLIENT_CB_CONTENT  cb_content;
} T_ASP_CLIENT_CB_DATA;

/**
 * @brief hids service def
 *
 */
typedef struct asp_service
{
    uint8_t service_id;
    struct asp_service *p_next;
} T_ASP_SERVICE;

/**
 * @brief  ASP client Link control block defination.
 */
typedef struct
{
    bool used;
    uint8_t conn_handle;
    uint8_t asp_service_num;
    T_ASP_SERVICE *asp_service_info;
} T_ASP_LINK;
/** End of ASP_CLIENT_Exported_Types
* @}
*/


/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup ASP_CLIENT_Exported_Functions ASP Client Exported Functions
  * @brief
  * @{
  */

/**
  * @brief      Add asp client to application.
  * @param[in]  app_cb pointer of app callback function to handle specific client module data.
  * @return Client ID of the specific client module.
  * @retval 0xff failed.
  * @retval other success.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        client_init(1);
        asp_client_id = asp_add_client(app_client_callback);
    }
  * \endcode
  */
bool asp_add_client(P_FUN_CLIENT_UUID128_GENERAL_APP_CB app_cb);

/**
 * @brief api of asp write report
 *
 * @param conn_handle le link connect handle
 * @param service_id service instance id
 * @param length data length
 * @param p_data data pointer
 * @return true
 * @return false
 */
bool asp_write_data(uint16_t conn_handle, uint8_t service_id, uint16_t length,
                    uint8_t *p_data);

/** @} End of ASP_CLIENT_Exported_Functions */

/** @} End of ASP_CLIENT */




#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _HIDS_CFU_CLIENT_H_ */
