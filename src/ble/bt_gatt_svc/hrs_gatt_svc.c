/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "string.h"
#include "trace.h"
#include "os_mem.h"
#include "gap_conn_le.h"
#include "hrs_gatt_svc.h"

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/
#define HRS_CP_RSPCODE_RESERVED                    0x00
#define HRS_CP_RSPCODE_SUCCESS                     0x01
#define HRS_CP_RSPCODE_OPCODE_UNSUPPORT            0x02
#define HRS_CP_RSPCODE_INVALID_PARAMETER           0x03
#define HRS_CP_RSPCODE_OPERATION_FAILED            0x04

#define HEART_RATE_VALUE_FORMAT_UINT8               0
#define HEART_RATE_VALUE_FORMAT_UINT16              1

#define HRS_ENERGY_EXPENDED_FEATURE_SUPPORT
#define HRS_BODY_SENSOR_LOCATION_CHAR_SUPPORT
#define HRS_CP_OPERATE_ACTIVE(x) (x == HRS_HEART_RATE_CP_OPCODE_RESET_ENERGY_EXPENDED)

/** Notification indication flag. */
typedef struct
{
    uint8_t heart_rate_measurement_notify_enable: 1;
    uint8_t rfu: 7;
} HRS_NOTIFY_INDICATE_FLAG;

T_HRS_CP_DATA hrs_cp_data = {0};
HRS_NOTIFY_INDICATE_FLAG hrs_notify_indicate_flag = {0};

/**<  Function pointer used to send event to application from location and navigation profile. */
static P_FUN_HRS_SERVER_APP_CB pfn_hrs_app_cb = NULL;

/**< @brief  profile/service definition.  */
static const T_ATTRIB_APPL hrs_attr_tbl[] =
{
    /*----------------- Heart Rate Service -------------------*/
    /* <<Primary Service>>, .. 0,*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),   /* wFlags     */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_SERVICE_HEART_RATE),              /* service UUID */
            HI_WORD(GATT_UUID_SERVICE_HEART_RATE)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* pValueContext */
        GATT_PERM_READ                              /* wPermissions  */
    },

    /* <<Characteristic>>, Heart Rate Measurement*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_NOTIFY/* characteristic properties */
            )
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
    /* Temperature Measurement value 2,*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HRS_HEART_RATE_MEASUREMENT),
            HI_WORD(GATT_UUID_CHAR_HRS_HEART_RATE_MEASUREMENT)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_NOTIF_IND/* wPermissions */
    },

    /* client characteristic configuration */
    {
        ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL,                   /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            /* NOTE: this value has an instantiation for each client, a write to */
            /* this attribute does not modify this default value:                */
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* wPermissions */
    }
#ifdef HRS_BODY_SENSOR_LOCATION_CHAR_SUPPORT
    ,
    /* <<Characteristic>>Body Sensor Location*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ/* characteristic properties */
            )
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HRS_BODY_SENSOR_LOCATION),
            HI_WORD(GATT_UUID_CHAR_HRS_BODY_SENSOR_LOCATION)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    }
#endif

#ifdef HRS_ENERGY_EXPENDED_FEATURE_SUPPORT
    ,
    /* <<Characteristic>> Heart Rate Control Point*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_WRITE/* characteristic properties */
            )
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HRS_HEART_RATE_CP),
            HI_WORD(GATT_UUID_CHAR_HRS_HEART_RATE_CP)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_WRITE/* wPermissions */
    }
#endif
};

const uint16_t hrs_attr_tbl_size = sizeof(hrs_attr_tbl);
const uint16_t hrs_char_num = hrs_attr_tbl_size / sizeof(T_ATTRIB_APPL);

bool hrs_body_sensor_location_read_confirm(uint16_t conn_handle, uint16_t cid, uint8_t service_id,
                                           uint16_t char_uuid, uint16_t offset,
                                           uint16_t value_len, uint8_t *p_value)
{
    bool ret = false;
    uint16_t attrib_index = gatt_svc_find_char_index_by_uuid16(hrs_attr_tbl, char_uuid,
                                                               hrs_char_num);
    if (attrib_index == 0)
    {
        PROFILE_PRINT_ERROR0("hrs_body_sensor_location_read_confirm: attr not found");
        return ret;
    }

    uint8_t *p_data = p_value;

    return gatt_svc_read_confirm(conn_handle, cid, service_id, attrib_index, p_data + offset,
                                 value_len - offset, value_len, APP_RESULT_SUCCESS);
}

bool hrs_send_heart_rate_measurement_value_notify(uint16_t conn_handle, uint16_t cid,
                                                  T_SERVER_ID service_id,
                                                  T_HEART_RATE_MEASUREMENT_VALUE heart_rate_measurement_value)
{
    uint8_t conn_id = 0xFF;
    le_get_conn_id_by_handle(conn_handle, &conn_id);

    uint16_t value_len = sizeof(uint8_t);  /* The length of heart_rate_measurement_value.flag */
    uint16_t mtu_size = 23;
    bool ret = false;

    le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu_size, conn_id);

    if (heart_rate_measurement_value.flag.heart_rate_value_format_bit ==
        HEART_RATE_VALUE_FORMAT_UINT8)
    {
        value_len += sizeof(uint8_t);
    }
    else
    {
        value_len += sizeof(uint16_t);
    }

    if (heart_rate_measurement_value.flag.energy_expended_status_bit)
    {
        value_len += sizeof(uint16_t);
    }

    if (heart_rate_measurement_value.flag.rr_interval_bit)
    {
        value_len += heart_rate_measurement_value.rr_interval_len;
    }


    if (value_len > mtu_size - 3)
    {
        PROFILE_PRINT_ERROR1("hrs_send_heart_rate_measurement_value_notify: value_len %d is too long",
                             value_len);
        return false;
    }

    uint8_t *p_heart_rate_measurement_value = os_mem_zalloc(RAM_TYPE_DATA_ON, value_len);

    if (p_heart_rate_measurement_value)
    {
        uint16_t cur_offset = 0;
        memcpy(p_heart_rate_measurement_value, &heart_rate_measurement_value.flag, 1);

        cur_offset += 1;

        if (heart_rate_measurement_value.flag.heart_rate_value_format_bit ==
            HEART_RATE_VALUE_FORMAT_UINT8)
        {
            memcpy(&p_heart_rate_measurement_value[cur_offset],
                   &heart_rate_measurement_value.heart_rate_measurement_value, 1);
            cur_offset += 1;
        }
        else
        {
            memcpy(&p_heart_rate_measurement_value[cur_offset],
                   &heart_rate_measurement_value.heart_rate_measurement_value, 2);
            cur_offset += 2;
        }

        if (heart_rate_measurement_value.flag.energy_expended_status_bit)
        {
            memcpy(&p_heart_rate_measurement_value[cur_offset],
                   &heart_rate_measurement_value.energy_expended, 2);
            cur_offset += 2;
        }

        if (heart_rate_measurement_value.flag.rr_interval_bit)
        {
            memcpy(&p_heart_rate_measurement_value[cur_offset],
                   heart_rate_measurement_value.p_rr_interval,
                   heart_rate_measurement_value.rr_interval_len);
        }

        PROFILE_PRINT_INFO0("hrs_send_heart_rate_measurement_value_notify");

        uint16_t attrib_index = gatt_svc_find_char_index_by_uuid16(hrs_attr_tbl,
                                                                   GATT_UUID_CHAR_HRS_HEART_RATE_MEASUREMENT,
                                                                   hrs_char_num);
        if (attrib_index == 0)
        {
            PROFILE_PRINT_ERROR0("hrs_send_heart_rate_measurement_value_notify: attr not found");
            return ret;
        }

        ret = gatt_svc_send_data(conn_handle, cid, service_id, attrib_index,
                                 p_heart_rate_measurement_value, value_len, GATT_PDU_TYPE_NOTIFICATION);

        os_mem_free(p_heart_rate_measurement_value);
    }
    else
    {
        PROFILE_PRINT_ERROR0("hrs_send_heart_rate_measurement_value_notify: alloc buf fail");
    }

    return ret;
}

/**
 * @brief  handle control point write request.
 *
 * @param[in] conn_handle   Connection handle of the ACL link.
 * @param[in] cid           Local CID assigned by Bluetooth stack.
 * @param[in] service_id    Service ID.
 * @param write_length      Write request data length.
 * @param value_ptr         Pointer to write request data.
 * @return none
 * @retval  void
*/
static T_APP_RESULT hrs_handle_cp_operation(uint16_t conn_handle, uint16_t cid,
                                            T_SERVER_ID service_id, uint16_t char_uuid,
                                            uint16_t write_length, uint8_t *p_value)
{
    T_APP_RESULT cause  = APP_RESULT_SUCCESS;
    uint16_t parameter_length = 0;

    memcpy(hrs_cp_data.value, p_value, write_length);
    if (write_length >= 1)
    {
        parameter_length = write_length - 1;
    }

    PROFILE_PRINT_INFO1("hrs_handle_cp_operation: opcode 0x%x", hrs_cp_data.value[0]);

    switch (hrs_cp_data.value[0])
    {
    case HRS_HEART_RATE_CP_OPCODE_RESET_ENERGY_EXPENDED:
        {
            if (!hrs_notify_indicate_flag.heart_rate_measurement_notify_enable)
            {
                cause = APP_RESULT_PROC_ALREADY_IN_PROGRESS;
            }
            else if (parameter_length != 0)
            {
                cause = APP_RESULT_INVALID_PDU;
            }

            T_HRS_SERVER_WRITE_CP hrs_cp;
            hrs_cp.service_id = service_id;
            hrs_cp.char_uuid = char_uuid;
            hrs_cp.opcode = (T_HRS_HEART_RATE_CP_OPCODE)hrs_cp_data.value[0];
            pfn_hrs_app_cb(conn_handle, cid, GATT_MSG_HRS_SERVER_WRITE_CP, (void *)&hrs_cp);
        }
        break;

    default:
        {
            cause = APP_RESULT_APP_ERR;
        }
        break;
    }

    return cause;
}


/**
 * @brief read characteristic data from service.
 *
 * @param conn_handle       Connection handle of the ACL link.
 * @param cid               Local CID assigned by Bluetooth stack.
 * @param[in] service_id    Service ID.
 * @param[in] attrib_index  Attribute index of getting characteristic data.
 * @param[in] offset        Used for Blob Read.
 * @param[in,out] p_length  length of getting characteristic data.
 * @param[in,out] pp_value  data got from service.
 * @return Profile procedure result
*/
T_APP_RESULT hrs_attr_read_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                              uint16_t attrib_index, uint16_t offset,
                              uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    T_CHAR_UUID char_uuid;

    *p_length = 0;
    char_uuid = gatt_svc_find_char_uuid_by_index(hrs_attr_tbl, attrib_index, hrs_char_num);

    T_HRS_SERVER_READ_CHAR_IND read_ind = {0};
    read_ind.service_id = service_id;
    read_ind.offset = offset;
    read_ind.char_uuid = char_uuid.uu.char_uuid16;

    PROFILE_PRINT_INFO4("hrs_attr_read_cb: conn_handle 0x%x, service_id 0x%x, char_uuid %d, offset %d",
                        conn_handle, service_id, read_ind.char_uuid, offset);

    switch (read_ind.char_uuid)
    {
    case GATT_UUID_CHAR_HRS_BODY_SENSOR_LOCATION:
        {
            if (pfn_hrs_app_cb)
            {
                cause = pfn_hrs_app_cb(conn_handle, cid,
                                       GATT_MSG_HRS_SERVER_READ_CHAR_IND,
                                       (void *)&read_ind);
            }

            if (cause == APP_RESULT_SUCCESS)
            {
                cause = APP_RESULT_PENDING;
            }
        }
        break;

    default:
        {
            PROFILE_PRINT_ERROR1("hrs_attr_read_cb: attrib_index 0x%x not found", attrib_index);
            cause = APP_RESULT_ATTR_NOT_FOUND;
        }
        break;
    }

    return cause;
}

/**
 * @brief write characteristic data from service.
 *
 * @param conn_handle       Connection handle of the ACL link.
 * @param cid               Local CID assigned by Bluetooth stack.
 * @param[in] service_id    ServiceID to be written.
 * @param[in] attrib_index  Attribute index of characteristic.
 * @param[in] length        Length of value to be written.
 * @param[in] p_value       Value to be written.
 * @return Profile procedure result
*/
T_APP_RESULT hrs_attr_write_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                               uint16_t attrib_index, T_WRITE_TYPE write_type,
                               uint16_t length, uint8_t *p_value,
                               P_FUN_EXT_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    T_CHAR_UUID char_uuid = gatt_svc_find_char_uuid_by_index(hrs_attr_tbl, attrib_index, hrs_char_num);

    PROFILE_PRINT_INFO4("hrs_attr_write_cb: conn_handle 0x%x, service_id 0x%x, char_uuid %d, attrib_index %d",
                        conn_handle, service_id, char_uuid.uu.char_uuid16, attrib_index);

    switch (char_uuid.uu.char_uuid16)
    {
    case GATT_UUID_CHAR_HRS_HEART_RATE_CP:
        {
            /* Attribute value has variable size, make sure written value size is valid. */
            if ((length > HRS_MAX_CP_VALUE_LEN) || (p_value == NULL))
            {
                cause  = APP_RESULT_INVALID_VALUE_SIZE;
            }
            /* Make sure Control Point is not "Process already in progress". */
            else if (HRS_CP_OPERATE_ACTIVE(hrs_cp_data.value[0]))
            {
                cause  = APP_RESULT_PROC_ALREADY_IN_PROGRESS;
            }
            /* Make sure Control Point is configured indication enable. */
            else if (!hrs_notify_indicate_flag.heart_rate_measurement_notify_enable)
            {
                cause = APP_RESULT_CCCD_IMPROPERLY_CONFIGURED;
            }
            else
            {
                cause = hrs_handle_cp_operation(conn_handle, cid, service_id,
                                                GATT_UUID_CHAR_HRS_HEART_RATE_CP,
                                                length, p_value);
            }
        }
        break;

    default:
        {
            PROFILE_PRINT_ERROR1("hrs_attr_write_cb: attrib_index 0x%x not found", attrib_index);
            cause  = APP_RESULT_ATTR_NOT_FOUND;
        }
        break;
    }
    hrs_cp_data.value[0] = HRS_HEART_RATE_CP_OPCODE_RESERVED;

    return cause;
}

/**
 * @brief update CCCD bits from stack.
 *
 * @param conn_handle       Connection handle of the ACL link.
 * @param cid               Local CID assigned by Bluetooth stack.
 * @param[in] service_id    Service ID.
 * @param[in] attrib_index  Attribute index of characteristic data.
 * @param[in] ccc_bits      CCCD bits from stack.
 * @return None
*/
void hrs_cccd_update_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                        uint16_t attrib_index, uint16_t ccc_bits)
{
    bool bHandle = true;
    T_CHAR_UUID char_uuid = gatt_svc_find_char_uuid_by_index(hrs_attr_tbl, attrib_index, hrs_char_num);

    PROFILE_PRINT_INFO4("hrs_cccd_update_cb: conn_handle 0x%x, service_id 0x%x, char_uuid %d, attrib_index %d",
                        conn_handle, service_id, char_uuid.uu.char_uuid16, attrib_index);

    T_HRS_SERVER_CCCD_INFO hrs_cccd_info;
    hrs_cccd_info.service_id = service_id;
    hrs_cccd_info.char_uuid = char_uuid.uu.char_uuid16;

    switch (char_uuid.uu.char_uuid16)
    {
    case GATT_UUID_CHAR_HRS_HEART_RATE_MEASUREMENT:
        {
            if (ccc_bits & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
            {
                // Enable Notification
                hrs_notify_indicate_flag.heart_rate_measurement_notify_enable = 1;
            }
            else
            {
                hrs_notify_indicate_flag.heart_rate_measurement_notify_enable = 0;
            }

            hrs_cccd_info.cccd_cfg = ccc_bits;
            pfn_hrs_app_cb(conn_handle, cid, GATT_MSG_HRS_SERVER_CCCD_INFO, (void *)&hrs_cccd_info);
        }
        break;

    default:
        {
            bHandle = false;
        }
        break;
    }

    return;
}

/**
 * @brief Heart Rate Service Callbacks.
*/
const T_FUN_GATT_EXT_SERVICE_CBS hrs_cbs =
{
    hrs_attr_read_cb,  // Read callback function pointer
    hrs_attr_write_cb, // Write callback function pointer
    hrs_cccd_update_cb  // CCCD update callback function pointer
};

T_SERVER_ID hrs_reg_srv(P_FUN_HRS_SERVER_APP_CB app_cb)
{
    T_SERVER_ID service_id;
    if (false == gatt_svc_add(&service_id,
                              (uint8_t *)hrs_attr_tbl,
                              hrs_attr_tbl_size,
                              &hrs_cbs, NULL))
    {
        PROFILE_PRINT_ERROR1("hrs_reg_srv: service_id %d", service_id);
        service_id = 0xff;
    }

    pfn_hrs_app_cb = app_cb;

    return service_id;
}


