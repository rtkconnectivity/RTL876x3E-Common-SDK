/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "ringtone.h"
#include "gap_conn_le.h"
#include "stdlib.h"
#include "dfu_api.h"
#include "ota_service.h"
#include "transmit_service.h"
#include "app_ble_service.h"
#include "app_ble_common_adv.h"
#include "app_cmd.h"

#include "app_main.h"
#include "app_transfer.h"
#include "app_transfer_cfg.h"
#include "app_cfg.h"
#include "bas.h"
#include "dis.h"
#include "profile_server_ext.h"


#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
#include "app_findmy.h"
#include "fmna_gatt.h"
#include "fmna_gatt_platform.h"
#include "fmna_util.h"
#include "app_findmy_ble.h"
#endif

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
#define MAX_BLE_SRV_NUM 16
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "gfps.h"
#include "app_gfps_cfg.h"
#include "app_gfps_finder.h"
#endif

#if F_APP_BLE_HID_DEVICE_SUPPORT
#include "hids_kb.h"
#endif

#include "gap_chann.h"

#if F_TRANS_UPDATE_FILE_SUPPORT
#include "trans_update_file_service.h"
#endif

#include "rtk_vendor_dis_gatt_svc.h"

static T_APP_RESULT common_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    uint8_t conn_id = 0xff;

    T_SERVER_EXT_APP_CB_DATA *p_para = (T_SERVER_EXT_APP_CB_DATA *)p_data;
    le_get_conn_id_by_handle(p_para->event_data.send_data_result.conn_handle, &conn_id);


    APP_PRINT_INFO3("app_ble_service common_cb: conn_id %d, service_id %d, eventId %d", conn_id,
                    service_id, p_para->eventId);

    switch (p_para->eventId)
    {
    case PROFILE_EVT_SRV_REG_AFTER_INIT_COMPLETE:
        {
        }
        break;
    case PROFILE_EVT_SRV_REG_COMPLETE:
        break;

    case PROFILE_EVT_SEND_DATA_COMPLETE:
        /*APP_PRINT_TRACE2("PROFILE_EVT_SEND_DATA_COMPLETE: cause 0x%x, attrib_idx %d",
                         p_para->event_data.send_data_result.cause,
                         p_para->event_data.send_data_result.attrib_idx);*/

        if (!gatt_svc_handle_profile_data_cmpl(p_para->event_data.send_data_result.conn_handle,
                                               p_para->event_data.send_data_result.cid,
                                               p_para->event_data.send_data_result.service_id,
                                               p_para->event_data.send_data_result.attrib_idx,
                                               p_para->event_data.send_data_result.credits,
                                               p_para->event_data.send_data_result.cause))
        {
            APP_PRINT_ERROR0("gatt_svc_handle_profile_data_cmpl failed");
        }

        if (p_para->event_data.send_data_result.attrib_idx == TRANSMIT_SVC_TX_DATA_INDEX)
        {
            T_CMD_PATH cmd_path = CMD_PATH_NONE;
            if (app_link_find_le_link_by_conn_handle(p_para->event_data.send_data_result.conn_handle))
            {
                cmd_path = CMD_PATH_LE;
            }
            else if (app_link_find_br_link_by_conn_handle(p_para->event_data.send_data_result.conn_handle))
            {
                cmd_path = CMD_PATH_GATT_OVER_BREDR;
            }
            APP_PRINT_INFO1("app_ble_service common_cb: PROFILE_EVT_SEND_DATA_COMPLETE cmd_path %d", cmd_path);
            app_pop_data_transfer_queue(cmd_path, true, 0xff);
        }

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
        fmna_gatt_platform_handle_send_data_complete(p_para->event_data.send_data_result.cause,
                                                     p_para->event_data.send_data_result.service_id);
#endif
        break;

    default:
        break;
    }
    return app_result;
}


static T_APP_RESULT transmit_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_CMD_PATH cmd_path = CMD_PATH_LE;

    T_APP_LE_LINK *p_le_link = NULL;
    T_APP_BR_LINK *p_br_link = NULL;
    T_TRANSMIT_SRV_CALLBACK_DATA *p_callback = (T_TRANSMIT_SRV_CALLBACK_DATA *)p_data;

    if ((p_callback->chann_type == GAP_CHANN_TYPE_LE_ATT) ||
        (p_callback->chann_type == GAP_CHANN_TYPE_LE_ECFC))
    {
        cmd_path = CMD_PATH_LE;
        p_le_link = app_link_find_le_link_by_conn_id(p_callback->conn_id);
    }
    else
    {
        cmd_path = CMD_PATH_GATT_OVER_BREDR;
        p_br_link = app_link_find_br_link_by_conn_handle(p_callback->conn_handle);
    }

    APP_PRINT_INFO8("app_ble_service_transmit_srv_cb: cmd_path %d, conn_handle %d, cid %d, conn_id %d, chann_type %d, msg_type %d, p_le_link %p, p_br_link %p",
                    cmd_path,
                    p_callback->conn_handle,
                    p_callback->cid,
                    p_callback->conn_id,
                    p_callback->chann_type,
                    p_callback->msg_type,
                    p_le_link,
                    p_br_link);

    if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
    {
        uint8_t         *p_data;
        uint16_t        data_len;
        uint16_t        total_len;

        p_data = p_callback->msg_data.rx_data.p_value;
        data_len = p_callback->msg_data.rx_data.len;

        if (p_callback->attr_index == TRANSMIT_SVC_RX_DATA_INDEX)
        {
            if (app_transfer_cfg.enable_embedded_cmd)
            {
                if ((cmd_path == CMD_PATH_LE) && (p_le_link != NULL))
                {
                    app_cmd_proc_data(CMD_PATH_LE, &p_le_link->cmd.buf,
                                      &p_le_link->cmd.len, p_data, data_len, p_le_link->id);
                }
                else if ((cmd_path == CMD_PATH_GATT_OVER_BREDR) && (p_br_link != NULL))
                {
                    app_cmd_proc_data(CMD_PATH_GATT_OVER_BREDR, &p_br_link->cmd.buf,
                                      &p_br_link->cmd.len, p_data, data_len, p_br_link->id);
                }
            }
            else if (app_cfg_const.enable_data_uart)
            {
                uint8_t     *tx_ptr;
                uint8_t     pkt_type;
                uint16_t    pkt_len;

                pkt_type = PKT_TYPE_SINGLE;
                total_len = data_len;
                while (data_len)
                {
                    if (data_len > (app_db.external_mcu_mtu - 12))
                    {
                        pkt_len = app_db.external_mcu_mtu - 12;
                        if (pkt_type == PKT_TYPE_SINGLE)
                        {
                            pkt_type = PKT_TYPE_START;
                        }
                        else
                        {
                            pkt_type = PKT_TYPE_CONT;
                        }
                    }
                    else
                    {
                        pkt_len = data_len;
                        if (pkt_type != PKT_TYPE_SINGLE)
                        {
                            pkt_type = PKT_TYPE_END;
                        }
                    }
                    if (p_le_link == NULL)
                    {
                        APP_PRINT_ERROR0("app_ble_service_transmit_srv_cb: p_le_link is empty");
                    }
                    else
                    {
                        tx_ptr = malloc(pkt_len + 6);
                        if (tx_ptr != NULL)
                        {
                            tx_ptr[0] = p_le_link->id;
                            tx_ptr[1] = pkt_type;
                            tx_ptr[2] = (uint8_t)total_len;
                            tx_ptr[3] = (uint8_t)(total_len >> 8);
                            tx_ptr[4] = (uint8_t)pkt_len;
                            tx_ptr[5] = (uint8_t)(pkt_len >> 8);
                            memcpy(&tx_ptr[6], p_data, pkt_len);

                            app_report_event(CMD_PATH_UART, EVENT_LE_DATA_TRANSFER, 0, tx_ptr, pkt_len + 6);

                            free(tx_ptr);
                        }
                        p_data += pkt_len;
                        data_len -= pkt_len;
                    }
                }
//                else
//                {
//                    APP_PRINT_ERROR0("app_ble_service_transmit_srv_cb: enable_embedded_cmd and enable_data_uart not set");
//                }
            }
        }
        else if (p_callback->attr_index == TRANSMIT_SVC_DELAY_MODE_INDEX)
        {
            //handle  p_data data_len here
            APP_PRINT_INFO2("app_ble_service_transmit_srv_cb: TRANSMIT_SVC_DELAY_MODE_INDEX data_len %d, data %b",
                            data_len, TRACE_BINARY(data_len, p_data));
        }
    }
    else if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION)
    {
        if (p_callback->attr_index == TRANSMIT_SVC_TX_DATA_CCCD_INDEX)
        {
            if ((cmd_path == CMD_PATH_LE) && (p_le_link != NULL))
            {
                app_ble_common_adv_set_conn_id(p_callback->conn_id);
            }

            if (p_callback->msg_data.notification_indification_value ==       TRANSMIT_SVC_TX_DATA_CCCD_ENABLE)
            {
                if ((cmd_path == CMD_PATH_LE) && (p_le_link != NULL))
                {
                    p_le_link->cmd.tx_mask |= TX_ENABLE_CCCD_BIT;
                }
                else if ((cmd_path == CMD_PATH_GATT_OVER_BREDR) && (p_br_link != NULL))
                {
                    p_br_link->cmd.tx_mask |= TX_ENABLE_CCCD_BIT;
                }
                APP_PRINT_INFO0("app_ble_service_transmit_srv_cb: TRANSMIT_SVC_TX_DATA_CCCD_ENABLE");
            }
            else if (p_callback->msg_data.notification_indification_value == TRANSMIT_SVC_TX_DATA_CCCD_DISABLE)
            {
                if ((cmd_path == CMD_PATH_LE) && (p_le_link != NULL))
                {
                    p_le_link->cmd.tx_mask &= ~TX_ENABLE_CCCD_BIT;
                }
                else if ((cmd_path == CMD_PATH_GATT_OVER_BREDR) && (p_br_link != NULL))
                {
                    //TODO:
                    //p_br_link->cmd.tx_mask &= ~TX_ENABLE_CCCD_BIT;
                }
                APP_PRINT_INFO0("app_ble_service_transmit_srv_cb: TRANSMIT_SVC_TX_DATA_CCCD_DISABLE");
            }
        }
    }

    return app_result;
}


static T_APP_RESULT  bas_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_BAS_CALLBACK_DATA *p_bas_cb_data = (T_BAS_CALLBACK_DATA *)p_data;
    switch (p_bas_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            if (p_bas_cb_data->msg_data.notification_indification_index == BAS_NOTIFY_BATTERY_LEVEL_ENABLE)
            {
                APP_PRINT_INFO0("app_ble_service_bas_srv_cb: BAS_NOTIFY_BATTERY_LEVEL_ENABLE");
            }
            else if (p_bas_cb_data->msg_data.notification_indification_index ==
                     BAS_NOTIFY_BATTERY_LEVEL_DISABLE)
            {
                APP_PRINT_INFO0("app_ble_service_bas_srv_cb: BAS_NOTIFY_BATTERY_LEVEL_DISABLE");
            }
        }
        break;

    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            /* Update battery level for bas using */
            bas_set_parameter(BAS_PARAM_BATTERY_LEVEL, 1, &app_db.local_batt_level);
            APP_PRINT_INFO1("app_ble_service_bas_srv_cb: local_batt_level %d", app_db.local_batt_level);
        }
        break;

    default:
        break;
    }

    return app_result;
}

static T_APP_RESULT app_ble_service_dis_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_DIS_CALLBACK_DATA *p_dis_cb_data = (T_DIS_CALLBACK_DATA *)p_data;
    switch (p_dis_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            if (p_dis_cb_data->msg_data.read_value_index == DIS_READ_FIRMWARE_REV_INDEX)
            {
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
                app_gfps_read_firmware_version();
#endif
            }
            else if (p_dis_cb_data->msg_data.read_value_index == DIS_READ_PNP_ID_INDEX)
            {

            }
        }
        break;

    default:
        break;
    }

    return app_result;
}

static T_APP_RESULT app_ble_service_ota_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_OTA_CALLBACK_DATA *p_ota_cb_data = (T_OTA_CALLBACK_DATA *)p_data;
    APP_PRINT_INFO3("app_ble_service_ota_srv_cb: service_id %d, msg_type %d, channel_type %d",
                    service_id, p_ota_cb_data->msg_type, p_ota_cb_data->chann_type);
    switch (p_ota_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        break;
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        if (OTA_WRITE_CHAR_VAL == p_ota_cb_data->msg_data.write.opcode &&
            OTA_VALUE_ENTER == p_ota_cb_data->msg_data.write.value)
        {
            /* Check battery level first */
            if (app_db.local_batt_level >= 30)
            {
//                T_APP_LE_LINK *p_link;
//                p_link = app_link_find_le_link_by_conn_id(p_ota_cb_data->conn_id);
//                /* Battery level is greater than or equal to 30 percent */
//                if (p_link != NULL)
//                {
//                    app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_SWITCH_TO_OTA);
//                }
                APP_PRINT_INFO1("app_ble_service_ota_srv_cb: Preparing switch into OTA mode conn_id %d",
                                p_ota_cb_data->conn_id);
            }
            else
            {
                /* Battery level is less than 30 percent */
                APP_PRINT_WARN1("app_ble_service_ota_srv_cb: Battery level is not enough to support OTA, local_batt_level %d",
                                app_db.local_batt_level);
            }
        }
        break;

    default:

        break;
    }

    return app_result;
}

#if F_APP_BLE_HID_DEVICE_SUPPORT
static T_APP_RESULT app_ble_service_hids_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT cb_result = APP_RESULT_SUCCESS;
    T_HID_CALLBACK_DATA *p_hid_cb_data = (T_HID_CALLBACK_DATA *)p_data;

    APP_PRINT_INFO2("app_ble_service_hids_srv_cb: service_id %d, msg_type %d",
                    service_id, p_hid_cb_data->msg_type);
    switch (p_hid_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        {
//            APP_PRINT_INFO1("app_ble_service_hids_srv_cb: write_type %d",
//                            p_hid_cb_data->msg_data.write_msg.write_type);
        }
        break;

    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
//            APP_PRINT_INFO1("app_ble_service_hids_srv_cb: index %d ",
//                            p_hid_cb_data->msg_data.read_value_index);
        }
        break;

    default:
        break;
    }

    return cb_result;
}
#endif

#if F_APP_GATT_OVER_BREDR_SUPPORT
T_APP_RESULT app_ble_service_rvdis_gatt_svc_cb(uint16_t conn_handle, uint16_t cid, uint8_t type,
                                               void *p_data)
{
    bool ret = true;
    if (type == GATT_MSG_RVDIS_SERVER_READ_CHAR_IND)
    {
        T_RVDIS_SERVER_READ_CHAR_IND *p_read = (T_RVDIS_SERVER_READ_CHAR_IND *)p_data;

        APP_PRINT_INFO3("app_ble_service_rvdis_gatt_svc_cb: service id %d, char_uuid 0x%x, offset %d",
                        p_read->service_id, p_read->char_uuid, p_read->offset);

        switch (p_read->char_uuid)
        {
        case GATT_UUID_CHAR_PREFERRED_GATT_TRANSPORT_TYPE:
            {
                uint8_t preferred_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_LE;

#if F_APP_GATT_OVER_BREDR_SUPPORT
                preferred_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_BREDR;
#endif
                ret = rvdis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid,
                                              p_read->offset, GATT_CHAR_PREFERRED_GATT_TRANSPORT_TYPE_LEN, &preferred_transport_type);
            }
            break;

        case GATT_UUID_CHAR_CURRENT_GATT_TRANSPORT_TYPE:
            {
                uint8_t current_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_LE;

                if (cid == L2C_FIXED_CID_ATT)
                {
                    current_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_LE;
                }
                else
                {
                    current_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_BREDR;
                }


                ret = rvdis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid,
                                              p_read->offset, GATT_CHAR_CURRENT_GATT_TRANSPORT_TYPE_LEN, &current_transport_type);
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
#endif

void app_ble_service_init(void)
{
    server_cfg_use_ext_api(true);
    APP_PRINT_INFO0("app_ble_service_init: server_cfg_use_ext_api true");
    server_ext_register_app_cb(common_cb);

    /** NOTES: 4 includes transimit service, ota service, bas, dis service.
     *  if more ble service are added, you need to modify this value.
     * */

    uint8_t server_num = 4;

#if F_APP_BLE_HID_DEVICE_SUPPORT
    server_num++;
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
#if CONFIG_REALTEK_FINDMY_USE_UARP
    server_num += 4;
#else
    server_num += 3;
#endif
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    server_num += 2;
#endif

#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT
    server_num += 2; //PACS and CAS
#endif

#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT
    server_num++; //ASCS
#endif

#if F_APP_TMAP_BMR_SUPPORT
    server_num++; //BASS
#endif

#if F_APP_VCS_SUPPORT
    server_num++;
#endif

#if F_APP_MICS_SUPPORT
    server_num++;
#endif

#if F_APP_CSIS_SUPPORT
    server_num++;
#endif

#if (CCP_CALL_CONTROL_SERVER || MCP_MEDIA_CONTROL_SERVER)
    server_num += 2;
#endif

#if F_APP_TMAS_SUPPORT
    server_num++;
#endif

#if F_TRANS_UPDATE_FILE_SUPPORT
    server_num++;
#endif

#if F_APP_GATT_OVER_BREDR_SUPPORT
    server_num++;
#endif

    server_init(server_num);

    transmit_srv_add(transmit_srv_cb);

    ota_add_service(app_ble_service_ota_srv_cb);

    bas_add_service(bas_srv_cb);

#if F_APP_BLE_HID_DEVICE_SUPPORT
    hids_add_service(app_ble_service_hids_srv_cb);
#endif

    dis_add_service(app_ble_service_dis_srv_cb);

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    app_findmy_init();
#endif

#if F_TRANS_UPDATE_FILE_SUPPORT
    trans_updatefile_add_service(NULL);
#endif

#if F_APP_GATT_OVER_BREDR_SUPPORT
    rvdis_add_service(app_ble_service_rvdis_gatt_svc_cb);
#endif

}
