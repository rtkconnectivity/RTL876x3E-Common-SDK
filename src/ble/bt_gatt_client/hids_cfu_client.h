/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _HIDS_CFU_CLIENT_H_
#define _HIDS_CFU_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include <profile_client.h>
#include <stdint.h>
#include <stdbool.h>
#include "bt_gatt_client.h"



/** @defgroup HIDS_CFU_CLIENT Hids CFU Service Client
  * @brief HIDS CFU client
  * @details
     Application shall register hids client when initialization through @ref hids_add_client function.

     Application can start discovery battery service through @ref hids_start_discovery function.

     Application shall handle callback function registered by hids_add_client.
  * \code{.c}
    T_APP_RESULT app_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
    {
        T_APP_RESULT  result = APP_RESULT_SUCCESS;
        if (client_id == hids_client_id)
        {
            T_BAS_CLIENT_CB_DATA *p_hids_cb_data = (T_HIDS_CLIENT_CB_DATA *)p_data;
            switch (p_hids_cb_data->cb_type)
            {
            case HIDS_CLIENT_CB_TYPE_DISC_STATE:
                switch (p_hids_cb_data->cb_content.disc_state)
                {
                case DISC_HIDS_DONE:
                ......
        }
    }
  * \endcode
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup HIDS_CFU_CLIENT_Exported_Macros Hids CFU Client Exported Macros
  * @brief
  * @{
  */

#define GATT_UUID_HIDS                          0x1812
#define GATT_UUID_CHAR_PROTOCOL_MODE            0x2A4E
#define GATT_UUID_CHAR_REPORT                   0x2A4D
#define GATT_UUID_CHAR_REPORT_MAP               0x2A4B
#define GATT_UUID_CHAR_HID_INFO                 0x2A4A
#define GATT_UUID_CHAR_HID_CONTROL_POINT        0x2A4C

/** @brief  Define links number. range: 0-4 */
#define HIDS_MAX_LINKS  1

/** End of HIDS_CFU_CLIENT_Exported_Macros
* @}
*/

typedef T_APP_RESULT(*P_FUN_CLIENT_UUID16_GENERAL_APP_CB)(uint16_t conn_handle, uint16_t srv_uuid,
                                                          void *p_data);


/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup HIDS_CFU_CLIENT_Exported_Types Hids CFU Client Exported Types
  * @brief
  * @{
  */

/**
 * @brief hid report type
 *
 */
typedef enum
{
    HID_INPUT_TYPE   = 1,
    HID_OUTPUT_TYPE  = 2,
    HID_FEATURE_TYPE = 3
} T_PROFILE_HID_REPORT_TYPE;

typedef enum
{
    BOOT_PROTOCOL_MODE = 0,
    REPORT_PROCOCOL_MODE = 1
} T_HID_PROTOCOL_MODE;

/** @brief HIDS client handle type*/
typedef enum
{
    HDL_HIDS_SRV_START,                 //!< start handle of hids service
    HDL_HIDS_SRV_END,                   //!< end handle of battery service
    HDL_HIDS_PROTOCOL_MODE,             //!< protocol mode characteristic value handle
    HDL_HIDS_REPORT_2A_INPUT,           //!< CFU 2A report characteristic handle
    HDL_HIDS_REPORT_2A_INPUT_CCCD,      //!< CFU 2A report characteristic cccd handle
    HDL_HIDS_REPORT_2A_OUTPUT,          //!< CFU 2A report characteristic handle
    HDL_HIDS_REPORT_2A_FEATURE,         //!< CFU 2A report characteristic handle
    HDL_HIDS_REPORT_2C_INPUT,           //!< CFU 2C report characteristic handle
    HDL_HIDS_REPORT_2C_INPUT_CCCD,      //!< CFU 2C report characteristic cccd handle
    HDL_HIDS_REPORT_2D_INPUT,           //!< CFU 2D report characteristic handle
    HDL_HIDS_REPORT_2D_INPUT_CCCD,      //!< CFU 2D report characteristic cccd handle
    HDL_HIDS_REPORT_2D_OUTPUT,          //!< CFU 2D report characteristic handle
    HDL_HIDS_REPORT_MAP,                //!< report map report characteristic handle
    HDL_HIDS_REPORT_MAP_EXTERNAL_REPORT_REFER, //!< report pefer report characteristic handle
    HDL_HIDS_HID_INFO,                  //!< hid info characteristic handle
    HDL_HIDS_HID_CONTROL_POINT,         //!< control point characteristic handle
    HDL_HIDS_CACHE_LEN                  //!< handle cache length
} T_HIDS_HANDLE_TYPE;

/** @brief HIDS client discovery state*/
typedef enum
{
    DISC_HIDS_IDLE,
    DISC_HIDS_START,
    DISC_HIDS_DONE,
    DISC_HIDS_FAILED
} T_HIDS_DISC_STATE;

/** @brief HIDS client notify type*/
typedef struct
{
    uint8_t  report_id;
    uint16_t data_len;
    uint8_t *p_data;
} T_HIDS_NOTIFY_DATA;

/** @brief HIDS client write type*/
typedef enum
{
    HIDS_REPORT_WRITE_2A_INPUT,
    HIDS_REPORT_WRITE_2A_OUTPUT,
    HIDS_REPORT_WRITE_2A_FEATURE,
    HIDS_REPORT_WRITE_2C_INPUT,
    HIDS_REPORT_WRITE_2D_INPUT,
    HIDS_REPORT_WRITE_2D_OUTPUT,
} T_HIDS_WRTIE_TYPE;

/** @brief HIDS client write result*/
typedef struct
{
    T_HIDS_WRTIE_TYPE type;
    uint16_t cause;
} T_HIDS_WRITE_RESULT;

/** @brief HIDS client read type*/
typedef enum
{
    HIDS_REPORT_READ_2A_INPUT,
    HIDS_REPORT_READ_2A_OUTPUT,
    HIDS_REPORT_READ_2A_FEATURE,
    HIDS_REPORT_READ_2C_INPUT,
    HIDS_REPORT_READ_2D_INPUT,
    HIDS_REPORT_READ_2D_OUTPUT,
    HIDS_READ_PROTOCOL_MODE,
    HIDS_READ_REPORT_MAP,
    HIDS_READ_REPORT_PEFER_REPORT,
    HIDS_READ_HID_INFO,
    HIDS_READ_HID_CONTROL_POINT,
} T_HIDS_READ_TYPE;

/** @brief HIDS client read result*/
typedef struct
{
    T_HIDS_READ_TYPE type;
    uint16_t data_len;
    uint8_t *p_data;
    uint16_t cause;
} T_HIDS_READ_RESULT;

/** @brief HIDS client callback type*/
typedef enum
{
    HIDS_CLIENT_CB_TYPE_DISC_STATE,          //!< Discovery procedure state, done or pending.
    HIDS_CLIENT_CB_TYPE_READ_RESULT,         //!< Read request's result data, responsed from server.
    HIDS_CLIENT_CB_TYPE_WRITE_RESULT,        //!< Write request result, success or fail.
    HIDS_CLIENT_CB_TYPE_NOTIF_IND_RESULT,    //!< Notification or indication data received from server.
} T_HIDS_CLIENT_CB_TYPE;

/** @brief HIDS client callback content*/
typedef union
{
    T_HIDS_DISC_STATE disc_state;
    T_HIDS_READ_RESULT read_result;
    T_HIDS_NOTIFY_DATA notify_data;
    T_HIDS_WRITE_RESULT write_result;
} T_HIDS_CLIENT_CB_CONTENT;

/** @brief HIDS client callback data*/
typedef struct
{
    T_HIDS_CLIENT_CB_TYPE     cb_type;
    T_HIDS_CLIENT_CB_CONTENT  cb_content;
} T_HIDS_CLIENT_CB_DATA;

typedef struct hids_report
{
    uint8_t report_id;
    T_PROFILE_HID_REPORT_TYPE report_type;
    struct hids_report *p_next;
} T_HIDS_REPORT;

typedef struct
{
    uint8_t  b_country_code;
    uint8_t  flags;
    uint16_t bcd_hid;
} T_HID_INFO;

/**
 * @brief hids service def
 *
 */
typedef struct hids_service
{
    uint8_t service_id;
    T_HID_PROTOCOL_MODE protocol_mode;
    T_HID_INFO  hid_info;
    uint8_t hid_suspand_mode;
    uint8_t hids_report_num;
    T_HIDS_REPORT  *p_report_info;
    struct hids_service *p_next;
} T_HIDS_SERVICE;

/**
 * @brief  HIDS client Link control block defination.
 */
typedef struct
{
    bool used;
    uint8_t conn_handle;
    uint8_t hids_service_num;
    T_HIDS_SERVICE *hids_service_info;
} T_HIDS_LINK;
/** End of HIDS_CLIENT_Exported_Types
* @}
*/


/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup HIDS_CLIENT_Exported_Functions HIDS Client Exported Functions
  * @brief
  * @{
  */

/**
  * @brief      Add hids client to application.
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
        hids_client_id = hids_add_client(app_client_callback);
    }
  * \endcode
  */
bool hids_add_client(P_FUN_CLIENT_UUID16_GENERAL_APP_CB app_cb);

/**
 * @brief api of hids write report
 *
 * @param conn_handle le link connect handle
 * @param service_id service instance id
 * @param report_id   cfu report id (2A/2C/2D)
 * @param length data length
 * @param p_data data pointer
 * @return true
 * @return false
 */
bool hids_write_report(uint8_t conn_handle, uint8_t service_id, uint8_t report_id,
                       T_PROFILE_HID_REPORT_TYPE report_type, uint16_t length,
                       uint8_t *p_data);

/** @} End of HIDS_CLIENT_Exported_Functions */

/** @} End of HIDS_CLIENT */




#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _HIDS_CFU_CLIENT_H_ */
