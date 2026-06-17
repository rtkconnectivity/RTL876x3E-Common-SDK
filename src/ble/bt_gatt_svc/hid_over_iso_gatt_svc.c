/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "string.h"
#include "trace.h"
#include "gap_conn_le.h"
#include "hid_over_iso_gatt_svc.h"
/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

#define GATT_SVC_HID_OVER_ISO_CHAR_PROPERTIES_INDEX                 2 /**< @brief Index for properties chars's value */
#define GATT_SVC_HID_OVER_ISO_CHAR_LE_HID_OPERATION_MODE_INDEX      4 /**< @brief Index for LE HID operation mode chars's value */
#define GATT_SVC_HID_OVER_ISO_CHAR_LE_HID_OPERATION_MODE_CCCD_INDEX 5 /**< @brief CCCD Index for LE HID operation mode chars's value */

/**<  Function pointer used to send event to application from HID over ISO service profile. */
static P_FUN_HID_OVER_ISO_SERVER_APP_CB  pfn_hid_over_iso_cb = NULL;
/**< @brief  profile/service definition.  */
static const T_ATTRIB_APPL hid_over_iso_attr_tbl[] =
{
    /*----------------- HID over ISO Service -------------------*/
    /* <<Primary Service>>, .. */
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),   /* flags     */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_HID_OVER_ISO),              /* service UUID */
            HI_WORD(GATT_UUID_HID_OVER_ISO)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* p_value_context */
        GATT_PERM_READ                              /* permissions  */
    },

    /* <<Characteristic>>, .. */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ),                  /* characteristic properties */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* HID over ISO properties value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_HID_OVER_ISO_PROPERTIES),
            HI_WORD(GATT_UUID_CHAR_HID_OVER_ISO_PROPERTIES)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* LE HID operation mode Characteristic */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
#if LE_HID_OPERATION_MODE_CHAR_INDICATION_SUPPORT
            (GATT_CHAR_PROP_WRITE | GATT_CHAR_PROP_INDICATE), /* characteristic properties */
#else
            GATT_CHAR_PROP_WRITE
#endif
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /* LE HID operation mode Characteristic value  */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_LE_HID_OPERATION_MODE),
            HI_WORD(GATT_UUID_CHAR_LE_HID_OPERATION_MODE)
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_WRITE                             /* wPermissions */
    }
#if LE_HID_OPERATION_MODE_CHAR_INDICATION_SUPPORT
    ,
    /* client characteristic configuration */
    {
        ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL,                   /* flags */
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
#endif
};
/**< @brief  HID over ISO service size definition.  */
const static uint16_t hid_over_iso_attr_tbl_size = sizeof(hid_over_iso_attr_tbl);

static T_HID_OVER_ISO_PROPERTIES_VALUE *p_hid_over_iso_properties = NULL;

bool hid_over_iso_set_properties(const T_HID_OVER_ISO_PROPERTIES_VALUE *p_properties)
{
    bool err_idx = 0;

    if (le_get_active_link_num() != 0)
    {
        err_idx = 1;
        goto failed;
    }

    if (p_properties == NULL)
    {
        err_idx = 2;
        goto failed;
    }

    if (p_properties->reports_num < HYBRID_MODE_ISO_REPORTS_NUM_MIN ||
        p_properties->reports_num > HYBRID_MODE_ISO_REPORTS_NUM_MAX)
    {
        err_idx = 3;
        goto failed;
    }
    p_hid_over_iso_properties = (T_HID_OVER_ISO_PROPERTIES_VALUE *)p_properties;
    return true;

failed:
    PROFILE_PRINT_ERROR1("hid_over_iso_set_properties: failed, err_idx %d", err_idx);
    return false;
}

#if LE_HID_OPERATION_MODE_CHAR_INDICATION_SUPPORT
bool hid_over_iso_service_send_le_hid_operation_mode_indication(uint16_t conn_handle,
                                                                uint8_t service_id,
                                                                T_LE_HID_OPERATION_MODE_VALUE *p_operation)
{
    uint8_t buf[sizeof(T_LE_HID_OPERATION_MODE_VALUE)];
    uint8_t *p = buf;

    if (p_operation == NULL)
    {
        return false;
    }

    LE_UINT8_TO_STREAM(p, p_operation->opcode);

    if (p_operation->opcode == LE_HID_OPERATION_MODE_CHAR_OPCODE_SELECT_HYBRID_OPERATION_MODE)
    {
        LE_UINT8_TO_STREAM(p, p_operation->param.hybrid_param.cig_id);
        LE_UINT8_TO_STREAM(p, p_operation->param.hybrid_param.cis_id);
        LE_UINT16_TO_STREAM(p, p_operation->param.hybrid_param.report_interval);
        memcpy(p, &p_operation->param.hybrid_param.hybrid_mode_iso_reports_enable,
               p_operation->param.hybrid_param.reports_num);
        p += p_operation->param.hybrid_param.reports_num;
    }

    return gatt_svc_send_data(conn_handle, L2C_FIXED_CID_ATT, service_id,
                              GATT_SVC_HID_OVER_ISO_CHAR_LE_HID_OPERATION_MODE_INDEX,
                              buf, p - buf, GATT_PDU_TYPE_INDICATION);
}
#endif

/**
 * @brief read characteristic data from service.
 *
 * @param conn_handle       Connection handle of the ACL link.
 * @param cid               Local CID assigned by Bluetooth stack.
 * @param service_id        ServiceID to be read.
 * @param attrib_index      Attribute index of getting characteristic data.
 * @param offset            Offset of characteristic to be read.
 * @param p_length          Length of getting characteristic data.
 * @param pp_value          Pointer to pointer of characteristic value to be read.
 * @return T_APP_RESULT
*/
T_APP_RESULT hid_over_iso_attr_read_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                                       uint16_t attrib_index,
                                       uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause = APP_RESULT_ATTR_NOT_FOUND;
    *p_length = 0;

    if (attrib_index == GATT_SVC_HID_OVER_ISO_CHAR_PROPERTIES_INDEX)
    {
        if (p_hid_over_iso_properties != NULL)
        {
            uint8_t buf[sizeof(T_HID_OVER_ISO_PROPERTIES_VALUE)];
            uint8_t *p = buf;

            LE_UINT8_TO_STREAM(p, p_hid_over_iso_properties->features);
            LE_UINT16_TO_STREAM(p, p_hid_over_iso_properties->supported_report_intervals);
            LE_UINT8_TO_STREAM(p, p_hid_over_iso_properties->max_sdu_size_device_to_host);
            LE_UINT8_TO_STREAM(p, p_hid_over_iso_properties->max_sdu_size_host_to_device);

            for (uint8_t i = 0; i < p_hid_over_iso_properties->reports_num; i++)
            {
                memcpy(p, &p_hid_over_iso_properties->reports[i], sizeof(T_HYBRID_MODE_ISO_REPORTS));
                p += sizeof(T_HYBRID_MODE_ISO_REPORTS);
            }

            if (gatt_svc_read_confirm(conn_handle, cid, service_id,
                                      GATT_SVC_HID_OVER_ISO_CHAR_PROPERTIES_INDEX,
                                      buf, p - buf, p - buf, APP_RESULT_SUCCESS))
            {
                cause = APP_RESULT_PENDING;
            }
        }
        else
        {
            cause = APP_RESULT_INVALID_VALUE_SIZE;
        }
    }

    PROFILE_PRINT_INFO2("hid_over_iso_attr_read_cb: attrib_index %d cause 0x%x", attrib_index, cause);

    return (cause);
}

/**
 * @brief write characteristic data from service.
 *
 * @param[in] conn_handle       Connection handle.
 * @param[in] cid               Local CID assigned by Bluetooth stack.
 * @param[in] service_id        ServiceID to be written.
 * @param[in] attrib_index      Attribute index of characteristic.
 * @param[in] write_type        Write type.
 * @param[in] length            Length of writing characteristic data.
 * @param[in] p_value           Pointer to characteristic data.
 * @param[in] p_write_ind_post_proc      Function pointer after ias_attr_write_cb.
 * @return TProfileResult
*/
T_APP_RESULT hid_over_iso_attr_write_cb(uint16_t conn_handle, uint16_t cid, uint8_t service_id,
                                        uint16_t attrib_index, T_WRITE_TYPE write_type,
                                        uint16_t length, uint8_t *p_value,
                                        P_FUN_EXT_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
    T_APP_RESULT cause  = APP_RESULT_SUCCESS;

    if (p_value == NULL)
    {
        PROFILE_PRINT_ERROR2("hid_over_iso_attr_write_cb: p_value %p length 0x%x", p_value, length);
        cause = APP_RESULT_INVALID_PDU;
        return cause;
    }

    switch (attrib_index)
    {
    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;

    case GATT_SVC_HID_OVER_ISO_CHAR_LE_HID_OPERATION_MODE_INDEX:
        {
            T_HID_OVER_ISO_SERVICE_WRITE_LE_HID_OPERATION_MODE write_ind = {0};
            write_ind.service_id = service_id;

            if (length == 0)
            {
                cause = APP_RESULT_INVALID_VALUE_SIZE;
            }
            else
            {
                write_ind.operation.opcode = p_value[0];
                if (write_ind.operation.opcode == LE_HID_OPERATION_MODE_CHAR_OPCODE_SELECT_HYBRID_OPERATION_MODE)
                {
                    if (p_hid_over_iso_properties == NULL ||
                        (p_hid_over_iso_properties->features &
                         HID_OVER_ISO_PROPERTIES_CHAR_FEATURES_BIT_VALUE_DEVICE_MODE_CHANGE_SUPPORED) == 0)
                    {
                        cause = (T_APP_RESULT)ATT_ERR_HID_OVER_ISO_UNSUPPORTED_FEATURE;
                    }
                    else if (length < 6 || length > 13)
                    {
                        cause = APP_RESULT_INVALID_VALUE_SIZE;
                    }
                    else
                    {
                        write_ind.operation.param.hybrid_param.cig_id = p_value[1];
                        write_ind.operation.param.hybrid_param.cis_id = p_value[2];
                        LE_ARRAY_TO_UINT16(write_ind.operation.param.hybrid_param.report_interval, p_value + 3);
                        write_ind.operation.param.hybrid_param.reports_num = length - 5;
                        memcpy(write_ind.operation.param.hybrid_param.hybrid_mode_iso_reports_enable, p_value + 5,
                               write_ind.operation.param.hybrid_param.reports_num);
                    }
                }
                else if (write_ind.operation.opcode ==
                         LE_HID_OPERATION_MODE_CHAR_OPCODE_SELECT_DEFAULT_OPERATION_MODE)
                {

                }
                else
                {
                    cause = (T_APP_RESULT)ATT_ERR_HID_OVER_ISO_OPCODE_OUTSIDE_RANGE;
                }
            }

            if (pfn_hid_over_iso_cb && (cause == APP_RESULT_SUCCESS))
            {
                cause = pfn_hid_over_iso_cb(conn_handle, cid,
                                            GATT_MSG_HID_OVER_ISO_SERVICE_WRITE_LE_HID_OPERATION_MODE, (void *)&write_ind);
            }
        }
        break;
    }

    return cause;
}

/**
 * @brief update CCCD bits from stack.
 *
 * @param conn_handle       Connection handle of the ACL link.
 * @param cid               Local CID assigned by Bluetooth stack.
 * @param service_id        Service ID.
 * @param index             Attribute index of characteristic data.
 * @param ccc_bits          CCCD bits from stack.
 * @return None
*/
void hid_over_iso_cccd_update_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                                 uint16_t index,
                                 uint16_t ccc_bits)
{
#if LE_HID_OPERATION_MODE_CHAR_INDICATION_SUPPORT
    T_HID_OVER_ISO_SERVICE_CCCD_UPDATE cccd_info = {0};
    cccd_info.service_id = service_id;

    PROFILE_PRINT_INFO2("hid_over_iso_cccd_update_cb: index %d ccc_bits 0x%04x", index, ccc_bits);

    if (index == GATT_SVC_HID_OVER_ISO_CHAR_LE_HID_OPERATION_MODE_CCCD_INDEX)
    {
        cccd_info.cccd_cfg = ccc_bits;
        if (pfn_hid_over_iso_cb)
        {
            pfn_hid_over_iso_cb(conn_handle, cid, GATT_MSG_HID_OVER_ISO_SERVICE_CCCD_UPDATE,
                                (void *)&cccd_info);
        }
    }
#endif
    return;
}

/**
 * @brief HID over ISO Service Callbacks.
*/
const T_FUN_GATT_EXT_SERVICE_CBS  hid_over_iso_cbs =
{
    hid_over_iso_attr_read_cb,  // Read callback function pointer
    hid_over_iso_attr_write_cb, // Write callback function pointer
    hid_over_iso_cccd_update_cb  // CCCD update callback function pointer
};

T_SERVER_ID hid_over_iso_reg_srv(P_FUN_HID_OVER_ISO_SERVER_APP_CB app_cb)
{
    T_SERVER_ID service_id;
    if (false == gatt_svc_add(&service_id,
                              (uint8_t *)hid_over_iso_attr_tbl,
                              hid_over_iso_attr_tbl_size,
                              &hid_over_iso_cbs, NULL))
    {
        PROFILE_PRINT_ERROR1("hid_over_iso_reg_srv: service_id %d", service_id);
        service_id = 0xff;
    }

    pfn_hid_over_iso_cb = app_cb;
    return service_id;
}

