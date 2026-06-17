/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "string.h"
#include "trace.h"
#include "tps_gatt_svc.h"
#include "gap_conn_le.h"

static P_FUN_TPS_SERVER_APP_CB pfn_tps_cb = NULL;

/**< @brief  profile/service definition.  */
const T_ATTRIB_APPL tps_attr_tbl[] =
{
    /*----------------- TX Power Service -------------------*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /* wFlags     */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_TX_POWER_SERVICE),    /* service UUID */
            HI_WORD(GATT_UUID_TX_POWER_SERVICE)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* pValueContext */
        GATT_PERM_READ                              /* wPermissions  */
    },

    /* Alert Level Characteristic */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ,           /* characteristic properties */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /* Alert Level Characteristic value  */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_TX_LEVEL),
            HI_WORD(GATT_UUID_CHAR_TX_LEVEL)
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_READ                             /* wPermissions */
    }
};

/**< @brief  TPS profile related services size definition.  */
const static int tps_attr_tbl_size = sizeof(tps_attr_tbl);
const uint16_t tps_char_num = tps_attr_tbl_size / sizeof(T_ATTRIB_APPL);

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
    T_APP_RESULT app_dis_gatt_svc_callback(uint16_t conn_handle, uint16_t cid, uint8_t type,
                                           void *p_data)
    {
        bool ret = true;
        if (type == GATT_MSG_DIS_SERVER_READ_CHAR_IND)
        {
            T_DIS_SERVER_READ_CHAR_IND *p_read = (T_DIS_SERVER_READ_CHAR_IND *)p_data;

            APP_PRINT_INFO3("app_dis_gatt_svc_callback: service id %d, char_uuid 0x%x, offset %d",
                            p_read->service_id, p_read->char_uuid, p_read->offset);

            switch (p_read->char_uuid)
            {
            case GATT_UUID_CHAR_SYSTEM_ID:
                {
                    const uint8_t DISSystemID[DIS_SYSTEM_ID_LENGTH] = {0, 1, 2, 0, 0, 3, 4, 5};
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                DIS_SYSTEM_ID_LENGTH, (uint8_t *)DISSystemID);
                }
                break;

            default:
                break;
            }
        }

        if (ret)
        {
            return APP_RESULT_SUCCESS;
        }
        else
        {
            return APP_RESULT_APP_ERR;
        }
    }
 * \endcode
 */
bool tps_char_read_confirm(uint16_t conn_handle, uint16_t cid, uint8_t service_id,
                           uint16_t char_uuid, uint16_t offset,
                           uint16_t value_len, uint8_t *p_value)
{
    bool ret = false;
    uint16_t attrib_index = gatt_svc_find_char_index_by_uuid16(tps_attr_tbl, char_uuid,
                                                               tps_char_num);
    if (attrib_index == 0)
    {
        PROFILE_PRINT_ERROR0("tps_char_read_confirm: attr not found");
        return ret;
    }

    uint8_t *p_data = p_value;

    return gatt_svc_read_confirm(conn_handle, cid, service_id, attrib_index, p_data + offset,
                                 value_len - offset, value_len, APP_RESULT_SUCCESS);
}

/**
 * @brief read characteristic data from service.
 *
 * @param[in] conn_id   Connection id.
 * @param[in] service_id          ServiceID to be read.
 * @param[in] attrib_index          Attribute index of getting characteristic data.
 * @param[in] offset                offset of characteritic to be read.
 * @param[in,out] length_ptr            length of getting characteristic data.
 * @param[in,out] pp_value            pointer to pointer of characteristic value to be read.
 * @return TProfileResult
*/
T_APP_RESULT tps_attr_read_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                              uint16_t attrib_index, uint16_t offset, uint16_t *length_ptr, uint8_t **pp_value)
{
    T_APP_RESULT  cause  = APP_RESULT_SUCCESS;
    *length_ptr = 0;

    uint8_t conn_id;
    le_get_conn_id_by_handle(conn_handle, &conn_id);

    T_CHAR_UUID char_uuid = gatt_svc_find_char_uuid_by_index(tps_attr_tbl, attrib_index,
                                                             tps_char_num);
    T_TPS_SERVER_READ_TX_POWER read_ind = {0};
    read_ind.service_id = service_id;
    read_ind.offset = offset;
    read_ind.char_uuid = char_uuid.uu.char_uuid16;

    PROFILE_PRINT_INFO4("tps_attr_read_cb: conn_handle 0x%x, service_id 0x%x, char_uuid %d, offset %d",
                        conn_handle, service_id, read_ind.char_uuid, offset);

    switch (read_ind.char_uuid)
    {
    case GATT_UUID_CHAR_TX_LEVEL:
        {
            pfn_tps_cb(conn_handle, cid, GATT_MSG_TPS_SERVER_READ_TX_POWER, (void *)&read_ind);

            if (cause == APP_RESULT_SUCCESS)
            {
                cause = APP_RESULT_PENDING;
            }
        }
        break;

    default:
        PROFILE_PRINT_ERROR1("tps_attr_read_cb: attrib_index %d", attrib_index);
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;
    }

    PROFILE_PRINT_INFO2("tps_attr_read_cb: attrib_index %d, *length_ptr %d", attrib_index, *length_ptr);

    return (cause);
}

// TPS related Service Callbacks
const T_FUN_GATT_EXT_SERVICE_CBS tps_cbs =
{
    tps_attr_read_cb,  // Read callback function pointer
    NULL,           // Write callback function pointer
    NULL            // CCCD update callback function pointer
};

/**
  * @brief Add tx power service to the BLE stack database.
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
         tps_id = tps_add_service(app_handle_profile_message);
     }
  * \endcode
  */
T_SERVER_ID tps_reg_srv(P_FUN_TPS_SERVER_APP_CB app_cb)
{
    T_SERVER_ID service_id;

    if (false == gatt_svc_add(&service_id,
                              (uint8_t *)tps_attr_tbl,
                              tps_attr_tbl_size,
                              &tps_cbs, NULL))
    {
        PROFILE_PRINT_ERROR1("tps_reg_srv: service_id %d", service_id);
        service_id = 0xff;
    }

    pfn_tps_cb = app_cb;

    return service_id;
}
