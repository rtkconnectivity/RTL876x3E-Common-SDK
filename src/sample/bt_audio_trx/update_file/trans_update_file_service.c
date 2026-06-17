/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_TRANS_UPDATE_FILE_SUPPORT
/*============================================================================*
 *                              Header Files
 *============================================================================*/

#include <gatt.h>
#include <bt_types.h>
#include "trace.h"
#include "app_main.h"
#include "app_cfg.h"
#include "trans_update_file_service.h"
#include "gap_conn_le.h"
#include "bt_gatt_svc.h"
#include "app_trans_update_file.h"

/** @defgroup  PLAYBACK_SERVICE PLAYBACK Service
    * @brief LE Service to implement PLAYBACK feature
    * @{
    */

/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @defgroup PLAYBACK_SERVICE_Exported_Constants PLAYBACK service Exported Constants
    * @{
    */

/** @brief  PLAYBACK service UUID */
static const uint8_t GATT_UUID_TRANS_UPDATEFILE_SERVICE[16] = {GATT_UUID128_TRANS_UPDATEFILE_SERVICE_ADV};

/** @brief  PLAYBACK profile/service definition
*   @note   Here is an example of PLAYBACK service table including Write
*/
static const T_ATTRIB_APPL gatt_extended_service_table[] =
{
    /*--------------------------PLAYBACK Service ---------------------------*/
    /* <<Primary Service>>, .. 0 */
    {
        (ATTRIB_FLAG_VOID | ATTRIB_FLAG_LE),        /* flags */
        {
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),     /* type_value */
        },
        UUID_128BIT_SIZE,                           /* bValueLen */
        (void *)GATT_UUID_TRANS_UPDATEFILE_SERVICE,              /* p_value_context */
        GATT_PERM_READ                              /* permissions */
    },

    /* <<Characteristic1>>, .. 1 write*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {   /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_WRITE | GATT_CHAR_PROP_WRITE_NO_RSP,            /* characteristic properties */
            //XXXXMJMJ GATT_CHAR_PROP_INDICATE,     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /*  PLAYBACK characteristic value 2 */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {   /* type_value */
            LO_WORD(GATT_UUID_CHAR_TRANS_UPDATEFILE_WRITE),
            HI_WORD(GATT_UUID_CHAR_TRANS_UPDATEFILE_WRITE),
        },
        0,                                          /* variable size */ //2
        (void *)NULL,
        GATT_PERM_WRITE            /* permissions */ //GATT_PERM_WRITE GATT_PERM_READ | GATT_PERM_WRITE
    },

    /* <<Characteristic2>>, .. 3, notifiy */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {   /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_NOTIFY),                    /* characteristic properties */
            //XXXXMJMJ GATT_CHAR_PROP_INDICATE,     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /*  PLAYBACK characteristic value 4 */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {   /* type_value */
            LO_WORD(GATT_UUID_CHAR_TRANS_UPDATEFILE_NOTIFY),
            HI_WORD(GATT_UUID_CHAR_TRANS_UPDATEFILE_NOTIFY),
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_NOTIF_IND                               /* permissions */ //GATT_PERM_NONE GATT_PERM_NOTIF_IND GATT_PERM_NOTIF_IND_ENCRYPTED_REQ
    },

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

};

/** End of PLAYBACK_SERVICE_Exported_Constants
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup PLAYBACK_SERVICE_Exported_Variables PLAYBACK service Exported Variables
    * @brief
    * @{
    */

/** @brief  Service ID only used in this file */
static T_SERVER_ID srv_id_local;

/** @brief  Function pointer used to send event to application from PLAYBACK service
*   @note   It is initiated in trans_updatefile_add_service()
*/
static P_FUN_EXT_SERVER_GENERAL_CB  p_trans_updatefile_extended_cb = NULL;
/** End of PLAYBACK_SERVICE_Exported_Variables
    * @}
    */

/*============================================================================*
 *                              Private Functions
 *============================================================================*/
/** @defgroup PLAYBACK_SERVICE_Exported_Functions PLAYBACK service Exported Functions
    * @brief
    * @{
    */
/**
    * @brief    Write characteristic data from service
    * @param    conn_id     ID to identify the connection
    * @param    service_id   Service ID to be written
    * @param    attr_index  Attribute index of characteristic
    * @param    write_type  Write type of data to be written
    * @param    length      Length of value to be written
    * @param    p_value     Value to be written
    * @param    p_write_ind_post_proc   Write indicate post procedure
    * @return   T_APP_RESULT
    * @retval   Profile procedure result
    */
static T_APP_RESULT trans_updatefile_service_attr_write_cb(uint16_t conn_handle, uint16_t cid,
                                                           T_SERVER_ID service_id,
                                                           uint16_t attr_index, T_WRITE_TYPE write_type, uint16_t length,
                                                           uint8_t *p_value, P_FUN_EXT_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
    T_APP_RESULT  cause = APP_RESULT_SUCCESS;
    uint8_t conn_id;
    le_get_conn_id_by_handle(conn_handle, &conn_id);

    APP_PRINT_INFO2("trans_updatefile_service_attr_write_cb: attr_index 0x%02x, length %d", attr_index,
                    length);

    if (BLE_SERVICE_CHAR_TRANS_UPDATEFILE_WRITE_INDEX == attr_index)
    {
        cause = app_trans_updatefile_ble_handle_cp_req(conn_id, length, p_value);
    }
    else
    {
        APP_PRINT_ERROR0("trans_updatefile_service_attr_write_cb: unknown attr_index");
        cause = APP_RESULT_ATTR_NOT_FOUND;
    }
    return cause;

}

void trans_updatefile_cccd_update_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                                     uint16_t attrib_idx,
                                     uint16_t cccd_bits)
{
    T_TRANS_UPDATEFILE_CALLBACK_DATA callback_data;
    callback_data.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;

    uint8_t conn_id;
    le_get_conn_id_by_handle(conn_handle, &conn_id);
    callback_data.conn_id = conn_id;
    callback_data.conn_handle = conn_handle;
    callback_data.cid = cid;

    APP_PRINT_INFO2("trans_updatefile_cccd_update_cb: attrib_idx %d, cccd_bits 0x%04x", attrib_idx,
                    cccd_bits);
    switch (attrib_idx)
    {
    case BLE_SERVICE_CHAR_TRANS_UPDATEFILE_DATA_CCCD_INDEX:
        {

            if (cccd_bits & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
            {
                // Enable Notification
            }
            else
            {
            }

            callback_data.msg_data.notification_indification_index =
                BLE_SERVICE_CHAR_TRANS_UPDATEFILE_NOTIFY_INDEX;

            if (p_trans_updatefile_extended_cb)
            {
                p_trans_updatefile_extended_cb(service_id, (void *)&callback_data);
            }
            break;
        }
    default:
        break;
    }
    return;
}

/** @brief  PLAYBACK BLE Service Callbacks */
static const T_FUN_GATT_EXT_SERVICE_CBS   playback_service_cbs =
{
    NULL,                           /**< Read callback function pointer */
    trans_updatefile_service_attr_write_cb, /**< Write callback function pointer */
    trans_updatefile_cccd_update_cb         /**< CCCD update callback function pointer */
};

/**
    * @brief    Add PLAYBACK BLE service to application
    * @param    p_func  Pointer of APP callback function called by profile
    * @return   Service ID auto generated by profile layer
    * @retval   A T_SERVER_ID type value
    */
T_SERVER_ID trans_updatefile_add_service(void *p_func)
{
    T_SERVER_ID service_id;
    if (false == gatt_svc_add(&service_id,
                              (uint8_t *)gatt_extended_service_table,
                              sizeof(gatt_extended_service_table),
                              &playback_service_cbs, NULL))
    {
        APP_PRINT_ERROR1("trans_updatefile_add_service: service_id %d", service_id);
        service_id = 0xff;
        return service_id;
    }
    p_trans_updatefile_extended_cb = (P_FUN_EXT_SERVER_GENERAL_CB)p_func;
    srv_id_local = service_id;
    APP_PRINT_INFO1("trans_updatefile_add_service success: service_id %d", service_id);
    return service_id;
}
/**
* @brief    Send notification to peer side
* @param    conn_id  PID to identify the connection
* @param    p_data  value to be send to peer
* @param    data_len  data length of the value to be send
* @return   void
*/
void trans_updatefile_service_send_notification(uint8_t conn_id, uint8_t *p_data, uint16_t data_len)
{
    APP_PRINT_INFO2("trans_updatefile_service_send_notification, conn_id:0x%x, len:%d", conn_id,
                    data_len);
    gatt_svc_send_data(le_get_conn_handle(conn_id), L2C_FIXED_CID_ATT, srv_id_local,
                       BLE_SERVICE_CHAR_TRANS_UPDATEFILE_NOTIFY_INDEX, p_data, data_len,
                       GATT_PDU_TYPE_NOTIFICATION);
}
/** End of PLAYBACK_SERVICE_Exported_Functions
    * @}
    */

/** @} */ /* End of group PLAYBACK_SERVICE */
#endif
