/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/

#if F_TRANS_UPDATE_FILE_SUPPORT
#include <string.h>
#include <bt_types.h>
#include "trace.h"
#include "app_main.h"
#include "app_cmd.h"
#include "app_mmi.h"
#include "app_cfg.h"
#include "app_timer.h"
#include "os_msg.h"
#include "os_task.h"
#include "rtl876x.h"
#include "app_bt_policy_api.h"
#include "app_report.h"
#include "stdlib.h"
#include "patch_header_check.h"
#include "app_trans_update_file.h"
#include "ff.h"
#include "gap_conn_le.h"
#include "trans_update_file_service.h"
#include "gap_conn_le.h"
#include "fmc_api.h"
#include "flash_map.h"

/** @defgroup  APP_TRANS_UPDATEFILE_SERVICE APP Trans Updatefile handle
    * @brief APP Trans Updatefile Service to implement trans updatefile feature
    * @{
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_TRANS_UPDATEFILE_Exported_Variables APP Trans Updatefile Exported Variables
    * @brief
    * @{
    */
static PB_TRANS_FUNCTION_STRUCT pb_trans_struct;
static uint8_t timer_idx_pb_trans_file = 0;
static uint8_t timer_idx_pb_trans_sd_active = 0;
static uint8_t playback_trans_timer_id = 0;
/** End of APP_TRANS_UPDATEFILE_Exported_Variables
    * @}
    */

/*============================================================================*
 *                              Private Functions
 *============================================================================*/
/** @defgroup APP_Trans_Updatefile_Exported_Functions APP Trans Updatefile Exported Functions
    * @brief
    * @{
    */
static void app_trans_updatefile_service_prepare_send_notify(uint8_t conn_id, uint16_t event_id,
                                                             uint16_t len, uint8_t *data);

/**
    * @brief    Used to get device information
    * @param    p_data    point of device info data
    * @return   void
    */
void app_trans_updatefile_get_device_info(PLAYBACK_DEVICE_INFO *p_deviceinfo)
{
    if (p_deviceinfo == NULL)
    {
        return;
    }

    memset(p_deviceinfo, 0, sizeof(PLAYBACK_DEVICE_INFO));
    p_deviceinfo->ic_type = IC_TYPE;

    p_deviceinfo->pkt_size          = PLAYBACK_PKT_SIZE;
    p_deviceinfo->buf_check_size    = PLAYBACK_BUF_CHECK_SIZE;
    p_deviceinfo->protocol_ver      = PLAYBACK_PROTOCOL_VERSION;
    p_deviceinfo->mode              = 0x00;
    p_deviceinfo->song_format_type = PB_FORMAT_TYPE_DEFAULT;
}

bool app_trans_updatefile_screenaver_picture_init(uint32_t total_file_length)
{
    if (USER_DATA2_ADDR == 0 || USER_DATA2_SIZE == 0)
    {
        return false;
    }

    if (total_file_length > USER_DATA2_SIZE)
    {
        return false;
    }

    pb_trans_struct.file_total_length = total_file_length;
    pb_trans_struct.cur_offset = 0;
    pb_trans_struct.seq = 0;
    //uint8_t erase_64K_count = (USER_DATA2_SIZE/1024)/64;
    uint8_t erase_4k_count = (USER_DATA2_SIZE / 1024) / 4;
    uint32_t erase_pos = USER_DATA2_ADDR;

    APP_PRINT_TRACE2("app_trans_updatefile_screenaver_picture_init: file trans resource created, erase_4k_count %d, total length %d",
                     erase_4k_count, pb_trans_struct.file_total_length);

    //for (uint8_t i=0;i<erase_64K_count; i++)
    //{
    //    if(fmc_flash_nor_erase(erase_pos, FMC_FLASH_NOR_ERASE_SECTOR) == false)
    //    {
    //        return false;
    //    }
    //    erase_pos += 64*1024;
    //}

    for (uint8_t i = 0; i < erase_4k_count; i++)
    {
        if (fmc_flash_nor_erase(erase_pos, FMC_FLASH_NOR_ERASE_BLOCK) == false)
        {
            return false;
        }
        erase_pos += 4 * 1024;
    }

    return true;
}

static bool app_trans_updatefile_screenaver_picture_data_handle(uint8_t *p_data, uint16_t length)
{
    uint16_t host_pkt_crc;
    uint32_t file_offset;
    uint16_t pkt_len;
    uint8_t *p_file_data;
    bool results = false;

    LE_ARRAY_TO_UINT16(pb_trans_struct.seq, (p_data + 0));
    LE_ARRAY_TO_UINT16(host_pkt_crc, (p_data + 2));
    LE_ARRAY_TO_UINT32(file_offset, (p_data + 4));
    LE_ARRAY_TO_UINT16(pkt_len, (p_data + 8));
    p_file_data = p_data + 10;

    APP_PRINT_TRACE5("app_trans_updatefile_screenaver_picture_data_handle: file data trans, seq %d, crc 0x%x, offset %d, transfered_data_length %d, pkt_len %d",
                     pb_trans_struct.seq, host_pkt_crc, file_offset, pb_trans_struct.cur_offset, pkt_len);

    if ((file_offset == pb_trans_struct.cur_offset) &&
        (pb_trans_struct.cur_offset + pkt_len <= USER_DATA2_SIZE))
    {
        results = fmc_flash_nor_write(USER_DATA2_ADDR + file_offset, p_file_data, pkt_len);
        pb_trans_struct.cur_offset += pkt_len;
    }

    APP_PRINT_TRACE2("app_trans_updatefile_screenaver_picture_data_handle: result %d, data %b", results,
                     TRACE_BINARY(100, p_file_data));

    return results;
}

/**
 * @brief  get 16bit data swapped.
 *
 * @param  val          16bit data to be swapped.
 * @return value after being swapped.
*/
static uint16_t swap_16(uint16_t val)
{
    uint16_t result;

    /* Idiom Recognition for REV16 */
    result = ((val & 0xff) << 8) | ((val & 0xff00) >> 8);

    return result;
}

/**
* @brief calculate checksum of lenth of buffer.
*
* @param  offset             offset of the image.
* @param  length             length of data.
* @param  crcValue          ret crc value point.
* @return  0 if buffer checksum calcs successfully, error line number otherwise
*/
static uint32_t app_trans_updatefile_checkbufcrc(uint8_t *buf, uint32_t length, uint16_t mCrcVal)
{
    uint16_t checksum16 = 0;
    uint32_t result = 0;
    uint32_t i;
    uint16_t *p16;

    p16 = (uint16_t *)buf;
    for (i = 0; i < length / 2; ++i)
    {
        checksum16 = checksum16 ^ (*p16);
        ++p16;
    }

    checksum16 = swap_16(checksum16);
    if (checksum16 != mCrcVal)
    {
        result =  __LINE__;
        goto L_Return;
    }
    return result;

L_Return:
    APP_PRINT_ERROR3("<====playback_checkbufcrc :checksum16 0x%x, mCrcVal 0x%x result:%d",
                     checksum16, mCrcVal, result);
    return result;
}

static uint8_t app_trans_updatefile_cancel_handle(void)
{
    uint16_t res = PB_TRANS_RET_SUCCESS;
    pb_trans_struct.transfer_status = TRANSFER_STOPPED;
    APP_PRINT_TRACE1("app_trans_updatefile_cancel_handle res:0x%x", res);
    bt_sniff_mode_enable(pb_trans_struct.bd_addr, 784, 816, 0, 0);
    return res;
}

/**
    * @brief    exit transfer handle
    * @return   valid result
    */
static void app_trans_updatefile_exit_trans_handle(void)
{
    APP_PRINT_TRACE0("app_trans_updatefile_exit_trans_handle");
}

static void app_set_transfer_status(void)
{
    APP_PRINT_INFO0("transfer started");
    pb_trans_struct.transfer_status = TRANSFER_STARTED;

    bt_sniff_mode_disable(pb_trans_struct.bd_addr);
}

/*============================================================================*
 *                              Public Functions
 *============================================================================*/

/**
    * @brief  The main function to handle all the spp command
    * @param  length length of command id and data
    * @param  p_value data addr
    * @param  app_idx received rx command device index
    * @return void
    */

void app_trans_updatefile_cmd_handle(uint16_t length, uint8_t *p_value, uint8_t app_idx)
{
    uint8_t ack_pkt[3];
    uint16_t cmd_id = *(uint16_t *)p_value;
    uint8_t *p;
    uint8_t results = PB_TRANS_RET_SUCCESS;

    bool ack_flag = false;

    ack_pkt[0] = p_value[0];
    ack_pkt[1] = p_value[1];
    ack_pkt[2] = CMD_SET_STATUS_COMPLETE;

    if (length < 2)
    {
        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
        app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);
        APP_PRINT_ERROR0("app_trans_updatefile_cmd_handle: error length");
        return;
    }

    length = length - 2;
    p = p_value + 2;
    pb_trans_struct.trans_mode = PLAYBACK_TRANS_SPP_MODE;
    pb_trans_struct.id.spp_idx = app_idx;

    APP_PRINT_TRACE2("===>app_trans_updatefile_cmd_handle, cmd_id:0x%x, length:0x%x\n", cmd_id, length);

    switch (cmd_id)
    {
    case CMD_PLAYBACK_QUERY_INFO:
        {
            if (length == PLAYBACK_LENGTH_GET_INFO)
            {
                app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);
                memcpy(pb_trans_struct.bd_addr, app_db.br_link[app_idx].bd_addr, sizeof(pb_trans_struct.bd_addr));

                PLAYBACK_DEVICE_INFO deviceinfo;
                app_trans_updatefile_get_device_info(&deviceinfo);
                deviceinfo.protocol_ver = NEW_PROTOCOL_VERSION;
                app_report_event(CMD_PATH_SPP, EVENT_PLAYBACK_QUERY_INFO, app_idx, (uint8_t *)&deviceinfo,
                                 sizeof(deviceinfo));
            }
            else
            {
                ack_flag = true;
            }
        }
        break;
    case CMD_PLAYBACK_SET_SCENARIO:
        {
            if (length == PLAYBACK_LENGTH_SET_SCENARIO)
            {
                app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);

                T_SPP_PB_RET result = PB_TRANS_RET_SUCCESS;
                uint16_t toatl_length;
                LE_ARRAY_TO_UINT16(toatl_length, p);
                p = p + 2;
                LE_ARRAY_TO_UINT16(pb_trans_struct.transfer_scenario, p);
                p = p + 2;
                uint8_t suffix = *p;

                if ((pb_trans_struct.transfer_scenario != CS_SCREENAVER_PICTURE) || (suffix != BIN_SUFFIX_FILE))
                {

                    result = PB_TRANS_RET_RX_DATA_ERROR;
                }
                else
                {
                    APP_PRINT_ERROR3("CMD_PLAYBACK_SET_SCENARIO: transfer_scenario %d, total length %d, suffix %d",
                                     pb_trans_struct.transfer_scenario, toatl_length, suffix);
                }

                app_report_event(CMD_PATH_SPP, EVENT_PLAYBACK_SET_SCENARIO, app_idx, (uint8_t *)&result,
                                 sizeof(result));
            }
            else
            {
                ack_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_GET_LIST_DATA:
        {

        }
        break;

    case CMD_PLAYBACK_TRANS_START:
        {
            uint16_t file_name_len;
            uint8_t cmd_len;
            uint32_t  total_file_length = 0;
            LE_ARRAY_TO_UINT16(file_name_len, p);
            cmd_len = 2 + file_name_len + 4;

            app_set_transfer_status();
            if (length == cmd_len)
            {
                app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);

                LE_ARRAY_TO_UINT32(total_file_length, p + 2 + file_name_len);
                if (app_trans_updatefile_screenaver_picture_init(total_file_length))
                {
                    app_stop_timer(&timer_idx_pb_trans_sd_active);
                }
                else
                {
                    results = PB_TRANS_RET_CREAT_ERROR;
                }

                app_report_event(CMD_PATH_SPP, EVENT_PLAYBACK_TRANS_START, app_idx, &results, sizeof(results));
            }
            else
            {
                ack_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_TRANS_CONTINUE:
        {
            if (pb_trans_struct.transfer_status == TRANSFER_STARTED)
            {
                if (app_trans_updatefile_screenaver_picture_data_handle(p, length) == true ||
                    pb_trans_struct.cur_offset == pb_trans_struct.file_total_length)
                {
                    if ((pb_trans_struct.seq + 1) % PLAYBACK_PKT_NUM == 0)
                    {
                        results = PB_TRANS_RET_BUF_A_SUCCESS;
                        app_report_event(CMD_PATH_SPP, EVENT_PLAYBACK_REPORT_BUFFER_CHECK, app_idx, &results,
                                         sizeof(results));
                    }
                }
                else
                {
                    results = PB_TRANS_RET_WRITE_ERROR;
                }
            }
            else
            {
                results = PB_TRANS_RET_OPERATION_ERROR;
            }

            ack_pkt[2] = results;
            app_stop_timer(&timer_idx_pb_trans_file);
            app_start_timer(&timer_idx_pb_trans_file, "pb_trans_file",
                            playback_trans_timer_id, APP_TIMER_PB_TRANS_FILE, 0, false,
                            1600);
        }
        break;

    case CMD_PLAYBACK_REPORT_BUFFER_CHECK:
        {
            if (length == PLAYBACK_LENGTH_BUFFER_CHECK_EN)
            {
                app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);
//                results = app_playback_trans_buffer_check_handle(p);
                app_report_event(CMD_PATH_SPP, EVENT_PLAYBACK_REPORT_BUFFER_CHECK, app_idx, &results,
                                 sizeof(results));
            }
            else
            {
                ack_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_VALID_SONG:
        {
            if (length == PLAYBACK_LENGTH_VALID_SONG)
            {
                app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);
                if (pb_trans_struct.cur_offset != pb_trans_struct.file_total_length)
                {
                    results = PB_TRANS_RET_LENTH_ERROR;
                }
                app_report_event(CMD_PATH_SPP, EVENT_PLAYBACK_VALID_SONG, app_idx, &results, sizeof(results));
                app_stop_timer(&timer_idx_pb_trans_file);
                pb_trans_struct.transfer_status = TRANSFER_STOPPED;
                bt_sniff_mode_enable(pb_trans_struct.bd_addr, 784, 816, 0, 0);
            }
            else
            {
                ack_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_TRANS_CANCEL:
        {
            if (length == PLAYBACK_LENGTH_TRANS_CANCEL)
            {
                app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);
                pb_trans_struct.transfer_status = TRANSFER_STOPPED;
                bt_sniff_mode_enable(pb_trans_struct.bd_addr, 784, 816, 0, 0);
                app_report_event(CMD_PATH_SPP, EVENT_PLAYBACK_TRANS_CANCEL, app_idx, &results, sizeof(results));
                app_stop_timer(&timer_idx_pb_trans_file);
            }
            else
            {
                ack_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_EXIT_TRANS:
        {
            app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);
            pb_trans_struct.transfer_status = TRANSFER_STOPPED;
            results = PB_TRANS_RET_SUCCESS;
            app_report_event(CMD_PATH_SPP, EVENT_PLAYBACK_EXIT_TRANS, app_idx, &results, sizeof(results));
            bt_sniff_mode_enable(pb_trans_struct.bd_addr, 784, 816, 0, 0);
        }
        break;

    default:
        ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
        app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);
        break;
    }

    if (ack_flag == true)
    {
        APP_PRINT_TRACE0("app_trans_updatefile_cmd_handle: invalid length");
        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
        app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);
    }

}

//////////////////////////////////////////////////////////////////////////////////////////
//                                BLE
//////////////////////////////////////////////////////////////////////////////////////////
/**
    * @brief    Wrapper function to send notification to peer
    * @note
    * @param    conn_id     ID to identify the connection
    * @param    opcode      Notification on the specified opcode
    * @param    len         Notification data length
    * @param    data        Additional notification data
    * @return   void
    */
#define OFFSET_TEMP (0 + 2) //SATRT + EVENT len
static void app_trans_updatefile_service_prepare_send_notify(uint8_t conn_id, uint16_t event_id,
                                                             uint16_t len, uint8_t *data)
{
    uint8_t *p_buffer = NULL;
    uint16_t mtu_size;
    uint16_t remain_size = len;
    uint8_t *p_data = data;
    uint16_t send_len;

    if ((data == NULL) || (len == 0))
    {
        return;
    }

    le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu_size, conn_id);
    APP_PRINT_TRACE2("app_trans_updatefile_service_prepare_send_notify mtu_size:%d, len:0x%x", mtu_size,
                     len);
    p_buffer = malloc(mtu_size);
    if (p_buffer == NULL)
    {
        return;
    }
    memset(p_buffer, 0, mtu_size);

    if (len < mtu_size - OFFSET_TEMP)
    {
//        p_buffer[0] = PLAYBACK_OPCODE_START_DFU;
//        p_buffer[1] = opcode;
        LE_UINT16_TO_ARRAY((uint8_t *)&p_buffer[0], event_id);
        memcpy(&p_buffer[OFFSET_TEMP], data, len);
        trans_updatefile_service_send_notification(conn_id, p_buffer, len + OFFSET_TEMP);
        free(p_buffer);
        return;
    }

    while (remain_size)
    {
        if (remain_size == len)
        {
//            p_buffer[0] = PLAYBACK_OPCODE_NOTIF;
//            p_buffer[1] = opcode;
            LE_UINT16_TO_ARRAY((uint8_t *)&p_buffer[0], event_id);
            memcpy(&p_buffer[OFFSET_TEMP], p_data, mtu_size - OFFSET_TEMP);
            trans_updatefile_service_send_notification(conn_id, p_buffer, mtu_size);
            p_data += (mtu_size - OFFSET_TEMP);
            remain_size -= (mtu_size - OFFSET_TEMP);
            continue;
        }

        send_len = (remain_size > mtu_size) ? mtu_size : remain_size;
        memcpy(p_buffer, p_data, send_len);
        trans_updatefile_service_send_notification(conn_id, p_buffer, send_len);
        p_data += send_len;
        remain_size -= send_len;
    }

    free(p_buffer);
}

/**
    * @brief    Handle written request characteristic
    * @param    conn_id     ID to identify the connection
    * @param    length      Length of value to be written
    * @param    p_value     Value to be written
    * @return   T_APP_RESULT
    * @retval   Handle result of this request
    */

T_APP_RESULT app_trans_updatefile_ble_handle_cp_req(uint8_t conn_id, uint16_t length,
                                                    uint8_t *p_value)
{
    T_APP_RESULT cause = APP_RESULT_INVALID_PDU;
    uint8_t results = PB_TRANS_RET_SUCCESS;
    uint16_t cmd_id = *(uint16_t *)p_value;
    uint8_t *p = p_value + 2;
    bool error_flag = false;
    uint16_t mtu_size = 0;
    uint16_t buffer_check_size = 0;

    if (length < 2)
    {
        APP_PRINT_ERROR0("app_trans_updatefile_ble_handle_cp_req: error length");
        return APP_RESULT_INVALID_PDU;
    }

    length = length - 2;
//    p = p_value + 2;
    pb_trans_struct.id.ble_conn_id = conn_id;
    pb_trans_struct.trans_mode = PLAYBACK_TRANS_BLE_MODE;
    APP_PRINT_TRACE2("===>app_trans_updatefile_ble_handle_cp_req, cmd_id:0x%x, length:0x%x\n", cmd_id,
                     length);

    switch (cmd_id)
    {
    case CMD_PLAYBACK_QUERY_INFO:
        {
            if (length == PLAYBACK_LENGTH_GET_INFO)
            {
                PLAYBACK_DEVICE_INFO device_info;
                cause = APP_RESULT_SUCCESS;
                app_trans_updatefile_get_device_info(&device_info);
                device_info.protocol_ver = NEW_PROTOCOL_VERSION;
                le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu_size, conn_id);

                device_info.pkt_size = mtu_size - BLE_ATT_HEADER_LENGTH - BLE_UPDATE_FILE_HEADER_LENGTH;
                buffer_check_size = (uint16_t)(device_info.pkt_size * PLAYBACK_PKT_NUM)
                                    device_info.buf_check_size = buffer_check_size;

                app_trans_updatefile_service_prepare_send_notify(conn_id, EVENT_PLAYBACK_QUERY_INFO,
                                                                 sizeof(device_info),
                                                                 (uint8_t *)&device_info);
            }
            else
            {
                error_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_SET_SCENARIO:
        {
            if (length == PLAYBACK_LENGTH_SET_SCENARIO)
            {
                cause = APP_RESULT_SUCCESS;
                T_SPP_PB_RET result = PB_TRANS_RET_SUCCESS;
                uint16_t toatl_length;
                LE_ARRAY_TO_UINT16(toatl_length, p);
                p = p + 2;
                LE_ARRAY_TO_UINT16(pb_trans_struct.transfer_scenario, p);
                p = p + 2;
                uint8_t suffix = *p;

                if ((pb_trans_struct.transfer_scenario != CS_SCREENAVER_PICTURE) || (suffix != BIN_SUFFIX_FILE))
                {

                    result = PB_TRANS_RET_RX_DATA_ERROR;
                }
                else
                {
                    APP_PRINT_ERROR3("CMD_PLAYBACK_SET_SCENARIO: transfer_scenario %d, total length %d, suffix %d",
                                     pb_trans_struct.transfer_scenario, toatl_length, suffix);
                    error_flag = true;
                }

                app_trans_updatefile_service_prepare_send_notify(conn_id, EVENT_PLAYBACK_SET_SCENARIO,
                                                                 sizeof(result),
                                                                 (uint8_t *)&result);
            }
            else
            {
                error_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_GET_LIST_DATA:
        {

        }
        break;

    case CMD_PLAYBACK_TRANS_START:
        {
            uint16_t file_name_len;
            uint8_t cmd_len;
            uint16_t conn_interval_min = 0xc;
            uint16_t conn_interval_max = 0xc;
            uint16_t supervision_timeout = 500;
            uint16_t ce_length_min = 2 * (conn_interval_min - 1);
            uint16_t ce_length_max = 2 * (conn_interval_max - 1);
            uint32_t total_file_length = 0;
            LE_ARRAY_TO_UINT16(file_name_len, p);
            cmd_len = 2 + file_name_len + 4;
            app_set_transfer_status();
            if (length == cmd_len)
            {
                LE_ARRAY_TO_UINT32(total_file_length, p + 2 + file_name_len);
                if (app_trans_updatefile_screenaver_picture_init(total_file_length))
                {
                    cause = APP_RESULT_SUCCESS;
                    app_stop_timer(&timer_idx_pb_trans_sd_active);
                }
                else
                {
                    cause = APP_RESULT_APP_ERR;
                    results = PB_TRANS_RET_CREAT_ERROR;
                }

                le_update_conn_param(conn_id, conn_interval_min, conn_interval_max, 0, supervision_timeout,
                                     ce_length_min, ce_length_max);
                app_trans_updatefile_service_prepare_send_notify(conn_id, EVENT_PLAYBACK_TRANS_START,
                                                                 sizeof(results),
                                                                 &results);
            }
            else
            {
                error_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_TRANS_CONTINUE:
        {
            if (pb_trans_struct.transfer_status == TRANSFER_STARTED)
            {
                if (app_trans_updatefile_screenaver_picture_data_handle(p, length) == true)
                {
                    if ((pb_trans_struct.seq + 1) % PLAYBACK_PKT_NUM == 0 ||
                        pb_trans_struct.cur_offset == pb_trans_struct.file_total_length)
                    {
                        results = PB_TRANS_RET_BUF_A_SUCCESS;
                        app_trans_updatefile_service_prepare_send_notify(conn_id, EVENT_PLAYBACK_REPORT_BUFFER_CHECK,
                                                                         sizeof(results),
                                                                         &results);
                    }
                }
                else
                {
                    results = PB_TRANS_RET_WRITE_ERROR;
                }
            }
            else
            {
                results = PB_TRANS_RET_OPERATION_ERROR;
            }
            app_stop_timer(&timer_idx_pb_trans_file);
            app_start_timer(&timer_idx_pb_trans_file, "pb_trans_file",
                            playback_trans_timer_id, APP_TIMER_PB_TRANS_FILE, 0, false,
                            1600);
        }
        break;

    case CMD_PLAYBACK_VALID_SONG:
        {
            if (length == PLAYBACK_LENGTH_VALID_SONG)
            {
                cause = APP_RESULT_SUCCESS;
                if (pb_trans_struct.cur_offset != pb_trans_struct.file_total_length)
                {
                    results = PB_TRANS_RET_RX_DATA_ERROR;
                }
                app_trans_updatefile_service_prepare_send_notify(conn_id, EVENT_PLAYBACK_VALID_SONG,
                                                                 sizeof(results),
                                                                 &results);
                app_stop_timer(&timer_idx_pb_trans_file);
                pb_trans_struct.transfer_status = TRANSFER_STOPPED;
                bt_sniff_mode_enable(pb_trans_struct.bd_addr, 784, 816, 0, 0);
            }
            else
            {
                error_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_TRANS_CANCEL:
        {
            if (length == PLAYBACK_LENGTH_TRANS_CANCEL)
            {
                cause = APP_RESULT_SUCCESS;
                pb_trans_struct.transfer_status = TRANSFER_STOPPED;
                bt_sniff_mode_enable(pb_trans_struct.bd_addr, 784, 816, 0, 0);
                app_trans_updatefile_service_prepare_send_notify(conn_id, EVENT_PLAYBACK_TRANS_CANCEL,
                                                                 sizeof(results),
                                                                 &results);
                app_stop_timer(&timer_idx_pb_trans_file);
            }
            else
            {
                error_flag = true;
            }
        }
        break;

    case CMD_PLAYBACK_EXIT_TRANS:
        {
            cause = APP_RESULT_SUCCESS;
            pb_trans_struct.transfer_status = TRANSFER_STOPPED;
            app_trans_updatefile_service_prepare_send_notify(conn_id, EVENT_PLAYBACK_EXIT_TRANS,
                                                             sizeof(results),
                                                             &results);
            bt_sniff_mode_enable(pb_trans_struct.bd_addr, 784, 816, 0, 0);
        }
        break;

    default:
        APP_PRINT_ERROR1("app_trans_updatefile_ble_handle_cp_req, cmd_id not expected = %x", cmd_id);
        break;
    }

    if (error_flag)
    {
        APP_PRINT_ERROR0("app_trans_updatefile_ble_handle_cp_req: invalid length");
    }

    return cause;
}

//////////////////////////////////////////////ble end///////////////////////////////////////////////////
/*
    * @brief  avoid using sd card at the same time
*/
void app_trans_updatefile_cancel(void)
{
    if (pb_trans_struct.transfer_status == TRANSFER_STARTED)
    {
        uint8_t results = PB_TRANS_RET_SUCCESS;
        results = app_trans_updatefile_cancel_handle();
        if (pb_trans_struct.trans_mode == PLAYBACK_TRANS_SPP_MODE)
        {
            app_report_event(CMD_PATH_SPP, EVENT_PLAYBACK_TRANS_CANCEL, pb_trans_struct.id.spp_idx,
                             &results, sizeof(results));
        }
        else if (pb_trans_struct.trans_mode == PLAYBACK_TRANS_BLE_MODE)
        {
            app_trans_updatefile_service_prepare_send_notify(pb_trans_struct.id.ble_conn_id,
                                                             EVENT_PLAYBACK_TRANS_CANCEL, sizeof(results), &results);
        }
        app_stop_timer(&timer_idx_pb_trans_file);
    }
}

/////////////////////////////////////////timer/////////////////////////////////////////////////////
/**
    * @brief  timeout callback
    * @param  timer_id  timer id
    * @param  timer_chann  time channel
    * @return void
    */
static void app_trans_updatefile_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE1("app_trans_updatefile_timeout_cb: timer_evt 0x%02x", timer_evt);

    switch (timer_evt)
    {
    case APP_TIMER_PB_TRANS_FILE:
        {
            pb_trans_struct.transfer_status = TRANSFER_STOPPED;
            app_stop_timer(&timer_idx_pb_trans_file);
            bt_sniff_mode_enable(pb_trans_struct.bd_addr, 784, 816, 0, 0);
        }
        break;

    default:
        break;
    }
}
/////////////////////////////////////////timer end/////////////////////////////////////////////////////

void app_trans_update_file_init(void)
{
    pb_trans_struct.transfer_status = TRANSFER_STOPPED;
    app_timer_reg_cb(app_trans_updatefile_timeout_cb, &playback_trans_timer_id);
}

/** End of APP_Trans_Updatefile_Exported_Functions
    * @}
    */

/** @} */ /* End of group APP_TRANS_UPDATEFILE_SERVICE */
#endif
