/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _HRS_SERVICE_DEF_H
#define _HRS_SERVICE_DEF_H

#ifdef __cplusplus
extern "C"  {
#endif      /* __cplusplus */

/* Add Includes here */
#include "profile_server.h"


/** @defgroup HRS Heart Rate Service
  * @brief  Heart Rate Service
  * @details

    The Heart Rate Service exposes heart rate and other data related to a heart rate sensor intended for fitness applications.

    Applications shall register heart rate service during initialization through @ref hrs_add_service function.

    The Heart Rate Measurement characteristic is used to send a heart rate measurement. Included in the characteristic are a Flags field (for showing the presence of optional fields and features
    supported), a heart rate measurement value field and, depending upon the contents of the Flags field, an Energy Expended field and an RR-Interval field. The RR-Interval represents the time between
    two consecutive R waves in an Electrocardiogram (ECG) waveform.
    Applications can send heart rate measurement value through @ref hrs_heart_rate_measurement_value_notify function.

    The Body Sensor Location characteristic of the device is used to describe the intended location of the heart rate measurement for the device.
    The value of the Body Sensor Location characteristic is static while in a connection.

    The Heart Rate Control Point characteristic is used to enable a Client to write control points to a Server to control behavior.
    Support for this characteristic is mandatory if the Server supports the Energy Expended feature.
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup HRS_Exported_Macros HRS Exported Macros
  * @brief
  * @{
  */


/** @defgroup HRS_Read_Info HRS Read Info
  * @brief  Parameter for reading characteristic value.
  * @{
  */
#define HRS_READ_BODY_SENSOR_LOCATION_VALUE                 1
/**< Read Body Sensor Location characteristic value index. */
/** @} */

/** @defgroup HRS_Notify_Indicate_Info HRS Notify Indicate Info
  * @brief  Parameter for enabling or disabling notification or indication.
  * @{
  */
#define HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_ENABLE        1
/**< Heart Rate Measurement characteristic notification enabled index. */
#define HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_DISABLE       2
/**< Heart Rate Measurement characteristic notification disabled index. */
/** @} */



/** @defgroup HRS_Sensor_Location HRS Sensor Location
  * @brief  Body Sensor Location Value
  * @{
  */
#define  BODY_SENSOR_LOCATION_VALUE_OTHER                   0 //!< Other.
#define  BODY_SENSOR_LOCATION_VALUE_CHEST                   1 //!< Chest.
#define  BODY_SENSOR_LOCATION_VALUE_WRIST                   2 //!< Wrist.
#define  BODY_SENSOR_LOCATION_VALUE_FINGER                  3 //!< Finger.
#define  BODY_SENSOR_LOCATION_VALUE_HAND                    4 //!< Hand.
#define  BODY_SENSOR_LOCATION_VALUE_EAR_LOBE                5 //!< Ear Lobe.
#define  BODY_SENSOR_LOCATION_VALUE_FOOT                    6 //!< Foot.
/** @} */

/** @defgroup HRS_DEFs HRS Definitions
  * @brief  HRS definitions.
  * @{
  */
#define HRS_MAX_CTL_PNT_VALUE                               1
/**< Maximum length of Heart Rate Control Point characteristic value. */
/** @} */

/** End of HRS_Exported_Macros
* @}
*/



/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup HRS_Exported_Types HRS Exported Types
  * @brief
  * @{
  */

typedef enum
{
    HRS_HEART_RATE_CP_OPCODE_RESERVED = 0, /**< Reserved for Future Use. */
    HRS_HEART_RATE_CP_OPCODE_RESET_ENERGY_EXPENDED = 1
                                                     /**< Reset Energy Expended. Resets the value of the Energy Expended field in the
                                                      *   Heart Rate Measurement characteristic to 0. */
} T_HRS_HEART_RATE_CP_OPCODE;


/** Notification indication flag. */
typedef struct
{
    uint8_t heart_rate_measurement_notify_enable: 1;
    uint8_t rfu: 7;
} HRS_NOTIFY_INDICATE_FLAG;

/** Heart Rate Measurement Value Flag. */
typedef struct
{
    uint8_t heart_rate_value_format_bit: 1;
    uint8_t sensor_contact_status_bits: 2;
    uint8_t energy_expended_status_bit: 1;
    uint8_t rr_interval_bit: 1;
    uint8_t rfu: 3;
} T_HEART_RATE_MEASUREMENT_VALUE_FLAG;

/** Heart Rate Measurement Value. */
typedef struct
{
    T_HEART_RATE_MEASUREMENT_VALUE_FLAG flag;
    uint16_t heart_rate_measurement_value;
    uint16_t energy_expended;
    uint16_t *p_rr_interval;
    uint16_t rr_interval_len;
} T_HEART_RATE_MEASUREMENT_VALUE;

/**
 * @brief HRS Control Point data, variable length during connection, maximum can reach 17 octets.
 *
 * HRS Control Point data used to store the Control Point Command received from the client.
*/
typedef struct
{
    uint8_t    cur_length; /**<  Length of current CSC Control Point data. */
    uint8_t    value[HRS_MAX_CTL_PNT_VALUE]; /**<  Value of current CSC Control Point data. */
} T_HRS_CONTROL_POINT;

typedef struct
{
    T_HRS_HEART_RATE_CP_OPCODE opcode;
} T_HRS_WRITE_MSG;

typedef union
{
    uint8_t notification_indification_index;
    uint8_t read_value_index;
    T_HRS_WRITE_MSG write;
} T_HRS_UPSTREAM_MSG_DATA;

typedef struct
{
    T_SERVICE_CALLBACK_TYPE     msg_type;
    T_HRS_UPSTREAM_MSG_DATA    msg_data;
} T_HRS_CALLBACK_DATA;
/** End of HRS_Exported_Types
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup HRS_Exported_Functions HRS Exported Functions
  * @brief
  * @{
  */

/**
 * @brief       Add heart rate service to the Bluetooth Host.
 *
 *
 * @param[in]   p_func  Callback when service attribute was read, written, or CCCD updated.
 * @return Service ID generated by the Bluetooth Host: @ref T_SERVER_ID.
 * @retval 0xFF Operation failure.
 * @retval others Service ID assigned by Bluetooth Host.
 *
 * <b>Example usage</b>
 * \code{.c}
    void profile_init()
    {
        server_init(service_num);
        hrs_id = hrs_add_service(app_handle_profile_message);
    }
 * \endcode
 */
uint8_t hrs_add_service(void *p_func);

/**
 * @brief       Send Body Sensor Location read confirmation.
 *
 * @note APP can call this function when receive the body sensor location read callback.
 *       If application accept the read request, the cause of hrs read callback shall be
 *       set to @ref APP_RESULT_PENDING. And then call this API to send the read confirmation
 *       with cause @ref APP_RESULT_SUCCESS.
 *
 * \xrefitem Experimental_Added_API_2_14_0_0 "Experimental Added Since 2.14.0.0" "Experimental Added API"
 *
 * @param[in]   conn_id                  Connection id.
 * @param[in]   service_id               Service id.
 * @param[in]   hrs_body_sensor_location HRS body sensor location characteristic value.
 * @param[in]   cause                    Cause: @ref T_APP_RESULT.
 *
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    T_APP_RESULT hrs_cb(T_SERVER_ID service_id, T_HRS_CALLBACK_DATA *cb_data)
    {
        switch (cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (cb_data->msg_data.read_value_index == HRS_READ_BODY_SENSOR_LOCATION_VALUE)
                {
                    return APP_RESULT_PENDING;
                }
            }
            break;
        ......

        return APP_RESULT_SUCCESS;
    }

    void test(void)
    {
        bool result = hrs_body_sensor_location_read_confirm(conn_id, service_id, hrs_body_sensor_location,
                                                            cause);
    }
 * \endcode
 */
bool hrs_body_sensor_location_read_confirm(uint8_t conn_id, T_SERVER_ID service_id,
                                           uint8_t hrs_body_sensor_location, T_APP_RESULT cause);

/**
 * @brief       Send Heart Rate Measurement characteristic notification data.
 *
 * @param[in]   conn_id                       Connection id.
 * @param[in]   service_id                    Service id.
 * @param[in]   heart_rate_measurement_value  Heart rate measurement notify value: @ref T_HEART_RATE_MEASUREMENT_VALUE.
 *
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        bool result = hrs_heart_rate_measurement_value_notify(conn_id, service_id, heart_rate_measurement_value);
    }
 * \endcode
 */
bool hrs_heart_rate_measurement_value_notify(uint8_t conn_id, T_SERVER_ID service_id,
                                             T_HEART_RATE_MEASUREMENT_VALUE heart_rate_measurement_value);

/** @} End of HRS_Exported_Functions */

/** @} End of HRS */


#ifdef __cplusplus
}
#endif

#endif /* _HRS_SERVICE_DEF_H */

