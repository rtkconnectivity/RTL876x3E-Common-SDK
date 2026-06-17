/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
#include <string.h>
#include "trace.h"
#include "asp_svc.h"

#define ASP_SERVICE_UUID 0xFD, 0x63, 0x1C, 0x02, 0xC6, 0xAE, 0x27, 0x93, 0x76, 0x4E, 0x21, 0xAB, 0x00, 0x00, 0x46, 0x7A
#define ASP_SERVICE_9A_UUID 0xFD, 0x63, 0x1C, 0x02, 0xC6, 0xAE, 0x27, 0x93, 0x76, 0x4E, 0x21, 0xAB, 0x01, 0x00, 0x46, 0x7A
#define ASP_SERVICE_9B_UUID 0xFD, 0x63, 0x1C, 0x02, 0xC6, 0xAE, 0x27, 0x93, 0x76, 0x4E, 0x21, 0xAB, 0x02, 0x00, 0x46, 0x7A

static P_FUN_EXT_SERVER_GENERAL_CB pfn_asp_cb = NULL;
static T_SERVER_ID service_id;
static uint8_t asp_service_uuid[16] = {ASP_SERVICE_UUID};

static const T_ATTRIB_APPL asp_attr_tbl[] =
{
    /* <<Primary Service>>, .. 0 */
    {
        (ATTRIB_FLAG_VOID | ATTRIB_FLAG_LE),  /* flags     */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
        },
        UUID_128BIT_SIZE,                            /* bValueLen     */
        (void *)asp_service_uuid,              /* p_value_context */
        GATT_PERM_READ                              /* permissions  */
    },

    /* <<Characteristic>>, ..1 */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* 1flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            /* characteristic properties */
            (GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE | GATT_CHAR_PROP_NOTIFY)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* Message value 2*/
    {
        ATTRIB_FLAG_VALUE_APPL | ATTRIB_FLAG_UUID_128BIT,                     /* flags */
        {                                           /* type_value */
            ASP_SERVICE_9A_UUID
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ | GATT_PERM_WRITE | GATT_PERM_NOTIF_IND           /* permissions */
    },
    /* client characteristic configuration 3*/
    {
        ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL,  /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            /* NOTE: this value has an instantiation for each client, a write to */
            /* this attribute does not modify this default value:                */
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* permissions */
    },
    /* <<Characteristic>>, .. 4*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            /* characteristic properties */
            (GATT_CHAR_PROP_READ | GATT_CHAR_PROP_NOTIFY)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* Attn value 5*/
    {
        ATTRIB_FLAG_VALUE_APPL | ATTRIB_FLAG_UUID_128BIT,                     /* flags */
        {                                           /* type_value */
            ASP_SERVICE_9B_UUID
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ | GATT_PERM_NOTIF_IND        /* permissions */
    },
    /* client characteristic configuration 6*/
    {
        ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL,  /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            /* NOTE: this value has an instantiation for each client, a write to */
            /* this attribute does not modify this default value:                */
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* permissions */
    }
};

/**
 * @brief       Send ASP notification data .
 *
 *
 * @param[in]   conn_id  Connection handle.
 * @param[in]   T_ASP_DATA_TYPE  asp data type.
 * @param[in]   p_data report value pointer.
 * @param[in]   data_len length of report data.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t conn_id = 0;
        uint8_t asp_test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
        asp_send_data(le_get_conn_handle(conn_id), ASP_DATA_9A, asp_test_data, sizeof(asp_test_data));
    }
 * \endcode
 */
bool asp_send_data(uint8_t conn_handle, T_ASP_DATA_TYPE data_type, uint8_t *p_data,
                   uint16_t data_len)
{
    bool ret = true;
    PROFILE_PRINT_INFO3("asp_send_data conn_handle %d, data type %d, data_len %d", conn_handle,
                        data_type, data_len);
    if (data_type == ASP_DATA_9A)
    {
        return gatt_svc_send_data(conn_handle, L2C_FIXED_CID_ATT, service_id,
                                  GATT_ASP_SVC_9A_VALUE_INDEX, p_data,
                                  data_len, GATT_PDU_TYPE_NOTIFICATION);
    }
    else if (data_type == ASP_DATA_9B)
    {
        return gatt_svc_send_data(conn_handle, L2C_FIXED_CID_ATT, service_id,
                                  GATT_ASP_SVC_9B_VALUE_INDEX, p_data,
                                  data_len, GATT_PDU_TYPE_NOTIFICATION);
    }
    else
    {
        ret = false;
    }
    return ret;
}

/**
 * @brief write characteristic data from service.
 *
 * @param[in] conn_handle       Connection handle.
 * @param[in] service_id        ServiceID to be written.
 * @param[in] attrib_index      Attribute index of characteristic.
 * @param[in] write_type        Write type.
 * @param[in] length            Length of writing characteristic data.
 * @param[in] p_value           Pointer to characteristic data.
 * @param[in] p_write_ind_post_proc      Function pointer after ias_attr_write_cb.
 * @return TProfileResult
*/
T_APP_RESULT asp_attr_write_cb(uint16_t conn_handle, uint16_t cid, uint8_t service_id,
                               uint16_t attrib_index, T_WRITE_TYPE write_type,
                               uint16_t length, uint8_t *p_value,
                               P_FUN_EXT_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
    T_APP_RESULT cause  = APP_RESULT_SUCCESS;
    T_ASP_CALLBACK_DATA callback_data;
    if (p_value == NULL)
    {
        PROFILE_PRINT_ERROR2("asp_attr_write_cb: p_value %p length= 0x%x", p_value, length);
        cause = APP_RESULT_INVALID_PDU;
        return cause;
    }

    switch (attrib_index)
    {
    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;

    case GATT_ASP_SVC_9A_VALUE_INDEX:
        callback_data.conn_handle = conn_handle;
        callback_data.cid = cid;
        callback_data.data_len = length;
        callback_data.p_data = p_value;
        break;
    }
    if (pfn_asp_cb && (cause == APP_RESULT_SUCCESS))
    {
        pfn_asp_cb(service_id, (void *)&callback_data);
    }

    return cause;
}

/*********************************************************************
 * SERVICE CALLBACKS
 */
// ASP related Service Callbacks
const T_FUN_GATT_EXT_SERVICE_CBS asp_cbs =
{
    NULL,  // Read callback function pointer
    asp_attr_write_cb, // Write callback function pointer
    NULL  // CCCD update callback function pointer
};

/**
  * @brief Add asp service to the BLE stack database.
  *
  * @param[in]   p_func  Callback when service attribute was read, write or cccd update.
  * @return Service id generated by the BLE stack: @ref T_SERVER_ID.
  * @retval 0xFF Operation failure.
  * @retval others Service id assigned by stack.
  *
  * <b>Example usage</b>
  * \code{.c}
    void profile_init()
    {
        server_init(1);
        asp_id = asp_add_service(app_handle_profile_message);
    }
  * \endcode
  */
T_SERVER_ID asp_add_service(void *p_func)
{
    if (false == gatt_svc_add(&service_id,
                              (uint8_t *)asp_attr_tbl,
                              sizeof(asp_attr_tbl),
                              &asp_cbs, NULL))
    {
        PROFILE_PRINT_ERROR1("asp_add_service: service_id %d", service_id);
        service_id = 0xff;
    }

    pfn_asp_cb = (P_FUN_EXT_SERVER_GENERAL_CB)p_func;
    return service_id;
}
#endif
