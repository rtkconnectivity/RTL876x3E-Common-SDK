/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "stdlib.h"
#include "os_queue.h"
#include "data_uart.h"
#include "ble_audio.h"
#include "cap.h"
#include "trace.h"
#include "app_lea_cap_com_main.h"
#include "app_lea_cap_com_csis.h"
#include "app_lea_cap_com_cap.h"
#include "ble_audio.h"
#include "vocs_client.h"
#include "aics_client.h"
#include "vcs_client.h"
#include "mics_client.h"

T_APP_LEA_GROUP_INFO *app_lea_find_group_by_conn_handle(uint16_t conn_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;
    for (uint8_t i = 0; i < app_db.group_handle_queue.count; i++)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue, i);
        if (p_group != NULL)
        {
            T_BLE_AUDIO_DEV_HANDLE dev_handle = ble_audio_group_find_dev_by_conn_handle(p_group->group_handle,
                                                                                        conn_handle);
            if (dev_handle)
            {
                return p_group;
            }
        }
    }
    return NULL;
}

T_APP_LEA_GROUP_INFO *app_lea_find_group(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;
    for (uint8_t i = 0; i < app_db.group_handle_queue.count; i++)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue, i);
        if (p_group != NULL && p_group->group_handle == group_handle)
        {
            return p_group;
        }
    }
    return NULL;
}

T_APP_LEA_GROUP_INFO *app_lea_add_group(T_BLE_AUDIO_GROUP_HANDLE group_handle, uint8_t set_mem_size)
{
    T_APP_LEA_GROUP_INFO *p_group;

    p_group = app_lea_find_group(group_handle);
    if (p_group)
    {
        return p_group;
    }
    p_group = calloc(1, sizeof(T_APP_LEA_GROUP_INFO));
    if (p_group)
    {
        p_group->group_handle = group_handle;
        p_group->set_mem_size = set_mem_size;
        data_uart_print("Add Group[%d]: group_handle 0x%x, set_mem_size %d\r\n",
                        app_db.group_handle_queue.count, p_group->group_handle, p_group->set_mem_size);
        os_queue_in(&app_db.group_handle_queue, p_group);
    }
    return p_group;
}

void app_lea_remove_group(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;

    p_group = app_lea_find_group(group_handle);
    if (p_group)
    {
        if (ble_audio_group_release(&group_handle))
        {
            os_queue_delete(&app_db.group_handle_queue, p_group);
            free(p_group);
        }
    }
}

void app_lea_com_group_cb(T_AUDIO_GROUP_MSG msg, T_BLE_AUDIO_GROUP_HANDLE handle,
                          void *buf)
{
    switch (msg)
    {
    case AUDIO_GROUP_MSG_DEV_CONN:
        {
            T_AUDIO_GROUP_MSG_DEV_CONN *p_data = (T_AUDIO_GROUP_MSG_DEV_CONN *)buf;
            APP_PRINT_INFO2("AUDIO_GROUP_MSG_DEV_CONN: group handle 0x%x, dev handle 0x%x",
                            handle, p_data->dev_handle);
        }
        break;

    case AUDIO_GROUP_MSG_DEV_DISCONN:
        {
            T_AUDIO_GROUP_MSG_DEV_DISCONN *p_data = (T_AUDIO_GROUP_MSG_DEV_DISCONN *)buf;
            APP_PRINT_INFO3("AUDIO_GROUP_MSG_DEV_DISCONN: group handle 0x%x, dev handle 0x%x, cause 0x%x",
                            handle, p_data->dev_handle, p_data->cause);
        }
        break;

    case AUDIO_GROUP_MSG_DEV_BOND_REMOVE:
        {
            T_AUDIO_GROUP_MSG_DEV_BOND_REMOVE *p_data = (T_AUDIO_GROUP_MSG_DEV_BOND_REMOVE *)buf;
            APP_PRINT_INFO2("AUDIO_GROUP_MSG_DEV_BOND_REMOVE: group handle 0x%x, dev handle 0x%x",
                            handle, p_data->dev_handle);
            ble_audio_group_del_dev(handle, &p_data->dev_handle);
        }
        break;

    case AUDIO_GROUP_MSG_DEV_BOND_CLEAR:
        {
            APP_PRINT_INFO1("AUDIO_GROUP_MSG_DEV_BOND_CLEAR: group handle 0x%x", handle);
            app_lea_remove_group(handle);
        }
        break;

    case AUDIO_GROUP_MSG_DEV_EMPTY:
        {
            APP_PRINT_INFO1("AUDIO_GROUP_MSG_DEV_EMPTY: group handle 0x%x", handle);
            app_lea_remove_group(handle);
        }
        break;

    default:
        break;
    }
    return;
}

uint16_t app_lea_com_cap_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_CAP_DIS_DONE:
        {
            T_CAP_DIS_DONE *p_dis_done = (T_CAP_DIS_DONE *)buf;
            T_APP_LE_LINK *p_link = NULL;

            APP_PRINT_INFO6("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_CAP_DIS_DONE, conn_handle 0x%x, load_from_ftl %d, cas_is_found %d, cas_inc_csis %d, vcs_is_found %d, mics_is_found %d",
                            p_dis_done->conn_handle,
                            p_dis_done->load_from_ftl,
                            p_dis_done->cas_is_found,
                            p_dis_done->cas_inc_csis,
                            p_dis_done->vcs_is_found,
                            p_dis_done->mics_is_found);
            p_link = app_link_find_le_link_by_conn_handle(p_dis_done->conn_handle);
            if (p_link)
            {
                if (p_dis_done->cas_is_found)
                {
                    p_link->lea_disc_flags |= LEA_LINK_DISC_CAS_DONE;
                    p_link->lea_srv_found_flags |= LEA_LINK_DISC_CAS_DONE;
                    if (p_dis_done->cas_inc_csis)
                    {
                        p_link->lea_srv_found_flags |= LEA_LINK_DISC_CSIS_DONE;
                    }
                }
            }
        }
        break;
#if VCP_VOLUME_CONTROLLER
    case LE_AUDIO_MSG_VCS_CLIENT_DIS_DONE:
        {
            T_VCS_CLIENT_DIS_DONE *p_dis_done = (T_VCS_CLIENT_DIS_DONE *)buf;
            APP_PRINT_INFO4("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_VCS_CLIENT_DIS_DONE, conn_handle 0x%x, is_found %d, load_from_ftl %d, type_exist 0x%x",
                            p_dis_done->conn_handle, p_dis_done->is_found, p_dis_done->load_from_ftl, p_dis_done->type_exist);
            if (p_dis_done->type_exist & VCS_VOLUME_STATE_FLAG)
            {
                T_VOLUME_STATE volume_state;
                if (vcs_get_volume_state(p_dis_done->conn_handle, &volume_state))
                {
                    APP_PRINT_INFO3("LE_AUDIO_MSG_VCS_CLIENT_DIS_DONE: volume_setting %d, mute %d, change_counter 0x%x",
                                    volume_state.volume_setting,  volume_state.mute,  volume_state.change_counter);
                }
            }

            if (p_dis_done->type_exist & VCS_VOLUME_FLAGS_FLAG)
            {
                uint8_t volume_flags;
                if (vcs_get_volume_flags(p_dis_done->conn_handle, &volume_flags))
                {
                    APP_PRINT_INFO1("LE_AUDIO_MSG_VCS_CLIENT_DIS_DONE: volume_flags 0x%x", volume_flags);
                }
            }
        }
        break;

    case LE_AUDIO_MSG_VCS_CLIENT_CP_RESULT:
        {
            T_VCS_CLIENT_CP_RESULT *p_cp_result = (T_VCS_CLIENT_CP_RESULT *)buf;
            APP_PRINT_INFO3("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_VCS_CLIENT_CP_RESULT, conn_handle 0x%x, cause 0x%x, cp_op 0x%x",
                            p_cp_result->conn_handle, p_cp_result->cause, p_cp_result->cp_op);
        }
        break;

    case LE_AUDIO_MSG_VCS_CLIENT_VOLUME_STATE_DATA:
        {
            T_VCS_CLIENT_VOLUME_STATE_DATA *p_data = (T_VCS_CLIENT_VOLUME_STATE_DATA *)buf;
            APP_PRINT_INFO3("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_VCS_CLIENT_VOLUME_STATE_DATA, conn_handle 0x%x, is_notify %d, type %d",
                            p_data->conn_handle, p_data->is_notify, p_data->type);
            if (p_data->type == VCS_CHAR_VOLUME_STATE)
            {
                data_uart_print("VCS_CHAR_VOLUME_STATE: volume_setting %d, mute %d, change_counter 0x%x\r\n",
                                p_data->data.volume_state.volume_setting,  p_data->data.volume_state.mute,
                                p_data->data.volume_state.change_counter);
            }
            else if (p_data->type == VCS_CHAR_VOLUME_FLAGS)
            {
                data_uart_print("VCS_CHAR_VOLUME_FLAGS: volume_flags 0x%x\r\n",
                                p_data->data.volume_flags);
            }
        }
        break;
#endif

#if MICP_MIC_CONTROLLER
    case LE_AUDIO_MSG_MICS_CLIENT_DIS_DONE:
        {
            T_MICS_CLIENT_DIS_DONE *p_dis_done = (T_MICS_CLIENT_DIS_DONE *)buf;
            APP_PRINT_INFO4("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_MICS_CLIENT_DIS_DONE, conn_handle 0x%x, is_found %d, load_from_ftl %d, mic_mute %d",
                            p_dis_done->conn_handle, p_dis_done->is_found, p_dis_done->load_from_ftl, p_dis_done->mic_mute);
        }
        break;

    case LE_AUDIO_MSG_MICS_CLIENT_WRITE_RESULT:
        {
            T_MICS_CLIENT_WRITE_RESULT *p_write_result = (T_MICS_CLIENT_WRITE_RESULT *)buf;
            APP_PRINT_INFO2("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_MICS_CLIENT_WRITE_RESULT, conn_handle 0x%x, cause 0x%x",
                            p_write_result->conn_handle, p_write_result->cause);
        }
        break;

    case LE_AUDIO_MSG_MICS_CLIENT_NOTIFY:
        {
            T_MICS_CLIENT_NOTIFY *p_notify = (T_MICS_CLIENT_NOTIFY *)buf;
            data_uart_print("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_MICS_CLIENT_NOTIFY, conn_handle 0x%x, mic_mute %d\r\n",
                            p_notify->conn_handle, p_notify->mic_mute);
        }
        break;
#endif
#if APP_LEA_VOCS_CLIENT_SUPPORT
    case LE_AUDIO_MSG_VOCS_CLIENT_DIS_DONE:
        {
            T_VOCS_CLIENT_DIS_DONE *p_dis_done = (T_VOCS_CLIENT_DIS_DONE *)buf;
            APP_PRINT_INFO4("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_VOCS_CLIENT_DIS_DONE, conn_handle 0x%x, is_found %d, load_from_ftl %d, srv_num 0x%x",
                            p_dis_done->conn_handle, p_dis_done->is_found, p_dis_done->load_from_ftl, p_dis_done->srv_num);
            if (p_dis_done->is_found)
            {
                for (uint8_t i = 0; i < p_dis_done->srv_num; i++)
                {
                    data_uart_print("VOCS for VCS: srv_instance_id %d enable CCCD\r\n", i);
                    vocs_cfg_cccd(p_dis_done->conn_handle, i,
                                  VOCS_VOLUME_OFFSET_STATE_FLAG | VOCS_AUDIO_LOCATION_FLAG | VOCS_AUDIO_OUTPUT_DES_FLAG, true);
                }
            }
        }
        break;

    case LE_AUDIO_MSG_VOCS_CLIENT_NOTIFY:
        {
            T_VOCS_CLIENT_NOTIFY *p_notify_data = (T_VOCS_CLIENT_NOTIFY *)buf;
            APP_PRINT_INFO3("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_VOCS_CLIENT_NOTIFY, conn_handle 0x%x, srv_instance_id %d, type %d",
                            p_notify_data->conn_handle, p_notify_data->srv_instance_id, p_notify_data->type);
            if (p_notify_data->type == VOCS_CHAR_OFFSET_STATE)
            {
                data_uart_print("VOCS_CHAR_OFFSET_STATE: notify, volume_offset %d, change_counter 0x%x\r\n",
                                p_notify_data->data.volume_offset.volume_offset,
                                p_notify_data->data.volume_offset.change_counter);
            }
            else if (p_notify_data->type == VOCS_CHAR_AUDIO_LOCATION)
            {
                data_uart_print("VOCS_CHAR_AUDIO_LOCATION: notify, audio_location 0x%x\r\n",
                                p_notify_data->data.audio_location);
            }
            else if (p_notify_data->type == VOCS_CHAR_AUDIO_OUTPUT_DESC)
            {
                data_uart_print("VOCS_CHAR_AUDIO_OUTPUT_DESC: notify, output des len %d\r\n",
                                p_notify_data->data.output_des.output_des_len);
                APP_PRINT_INFO2("VOCS_CHAR_AUDIO_OUTPUT_DESC: notify, output des[%d] %b",
                                p_notify_data->data.output_des.output_des_len,
                                TRACE_BINARY(p_notify_data->data.output_des.output_des_len,
                                             p_notify_data->data.output_des.p_output_des));
            }
        }
        break;

    case LE_AUDIO_MSG_VOCS_CLIENT_READ_RESULT:
        {
            T_VOCS_CLIENT_READ_RESULT *p_read_result = (T_VOCS_CLIENT_READ_RESULT *)buf;
            APP_PRINT_INFO4("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_VOCS_CLIENT_READ_RESULT, cause 0x%x, conn_handle 0x%x, srv_instance_id %d, type %d",
                            p_read_result->cause, p_read_result->conn_handle, p_read_result->srv_instance_id,
                            p_read_result->type);
            if (p_read_result->cause == GAP_SUCCESS)
            {
                if (p_read_result->type == VOCS_CHAR_OFFSET_STATE)
                {
                    data_uart_print("VOCS_CHAR_OFFSET_STATE: read result, volume_offset %d, change_counter 0x%x\r\n",
                                    p_read_result->data.volume_offset.volume_offset,
                                    p_read_result->data.volume_offset.change_counter);
                }
                else if (p_read_result->type == VOCS_CHAR_AUDIO_LOCATION)
                {
                    data_uart_print("VOCS_CHAR_AUDIO_LOCATION: read result, audio_location 0x%x\r\n",
                                    p_read_result->data.audio_location);
                }
                else if (p_read_result->type == VOCS_CHAR_AUDIO_OUTPUT_DESC)
                {
                    data_uart_print("VOCS_CHAR_AUDIO_OUTPUT_DESC: read result, output des len %d\r\n",
                                    p_read_result->data.output_des.output_des_len);
                    APP_PRINT_INFO2("VOCS_CHAR_AUDIO_OUTPUT_DESC: read result, output des[%d] %b",
                                    p_read_result->data.output_des.output_des_len,
                                    TRACE_BINARY(p_read_result->data.output_des.output_des_len,
                                                 p_read_result->data.output_des.p_output_des));
                }
            }
        }
        break;

    case LE_AUDIO_MSG_VOCS_CLIENT_CP_RESULT:
        {
            T_VOCS_CLIENT_CP_RESULT *p_cp_result = (T_VOCS_CLIENT_CP_RESULT *)buf;
            APP_PRINT_INFO4("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_VOCS_CLIENT_CP_RESULT, conn_handle 0x%x, srv_instance_id %d, cause 0x%x, cp_op 0x%x",
                            p_cp_result->conn_handle, p_cp_result->srv_instance_id, p_cp_result->cause, p_cp_result->cp_op);
        }
        break;
#endif

#if APP_LEA_AICS_CLIENT_SUPPORT
    case LE_AUDIO_MSG_AICS_CLIENT_DIS_DONE:
        {
            T_AICS_CLIENT_DIS_DONE *p_dis_done = (T_AICS_CLIENT_DIS_DONE *)buf;

            APP_PRINT_INFO4("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_AICS_CLIENT_DIS_DONE, conn_handle 0x%x, is_found %d, load_from_ftl %d, srv_num %d",
                            p_dis_done->conn_handle, p_dis_done->is_found, p_dis_done->load_from_ftl, p_dis_done-> srv_num);
            if (p_dis_done->is_found)
            {
                T_ATTR_INSTANCE attr_instance;
#if VCP_VOLUME_CONTROLLER
                if (vcs_get_all_inc_aics(p_dis_done->conn_handle, &attr_instance))
                {
                    for (uint8_t i = 0; i < attr_instance.instance_num; i++)
                    {
                        data_uart_print("AICS for VCS: srv_instance_id %d enable CCCD\r\n", attr_instance.instance_id[i]);
                        aics_cfg_cccd(p_dis_done->conn_handle, attr_instance.instance_id[i],
                                      AICS_INPUT_STATE_FLAG | AICS_INPUT_STATUS_FLAG | AICS_INPUT_DES_FLAG, true);
                    }
                }
#endif
#if MICP_MIC_CONTROLLER
                if (mics_get_all_inc_aics(p_dis_done->conn_handle, &attr_instance))
                {
                    for (uint8_t i = 0; i < attr_instance.instance_num; i++)
                    {
                        data_uart_print("AICS for MICS: srv_instance_id %d enable CCCD\r\n", attr_instance.instance_id[i]);
                        aics_cfg_cccd(p_dis_done->conn_handle, attr_instance.instance_id[i],
                                      AICS_INPUT_STATE_FLAG | AICS_INPUT_STATUS_FLAG | AICS_INPUT_DES_FLAG, true);
                    }
                }
#endif
            }
        }
        break;

    case LE_AUDIO_MSG_AICS_CLIENT_NOTIFY:
        {
            T_AICS_CLIENT_NOTIFY *p_notify_data = (T_AICS_CLIENT_NOTIFY *)buf;
            APP_PRINT_INFO3("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_AICS_CLIENT_NOTIFY, conn_handle 0x%x, srv_instance_id %d, type %d",
                            p_notify_data->conn_handle, p_notify_data->srv_instance_id, p_notify_data->type);
            if (p_notify_data->type == AICS_CHAR_INPUT_STATE)
            {
                data_uart_print("AICS_CHAR_INPUT_STATE: notify, gain_setting %d, mute %d, gain_mode %d, change_counter 0x%x\r\n",
                                p_notify_data->data.input_state.gain_setting,
                                p_notify_data->data.input_state.mute,
                                p_notify_data->data.input_state.gain_mode,
                                p_notify_data->data.input_state.change_counter);
            }
            else if (p_notify_data->type == AICS_CHAR_INPUT_STATUS)
            {
                data_uart_print("AICS_CHAR_INPUT_STATUS: notify, input_status 0x%x\r\n",
                                p_notify_data->data.input_status);
            }
            else if (p_notify_data->type == AICS_CHAR_INPUT_DES)
            {
                data_uart_print("AICS_CHAR_INPUT_DES: notify, input des len %d\r\n",
                                p_notify_data->data.input_des.input_des_len);
                APP_PRINT_INFO2("AICS_CHAR_INPUT_DES: notify, input des[%d] %b",
                                p_notify_data->data.input_des.input_des_len,
                                TRACE_BINARY(p_notify_data->data.input_des.input_des_len,
                                             p_notify_data->data.input_des.p_input_des));
            }
        }
        break;

    case LE_AUDIO_MSG_AICS_CLIENT_READ_RESULT:
        {
            T_AICS_CLIENT_READ_RESULT *p_read_result = (T_AICS_CLIENT_READ_RESULT *)buf;
            APP_PRINT_INFO4("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_AICS_CLIENT_READ_RESULT, cause 0x%x, conn_handle 0x%x, srv_instance_id %d, type %d",
                            p_read_result->cause, p_read_result->conn_handle, p_read_result->srv_instance_id,
                            p_read_result->type);
            if (p_read_result->cause == GAP_SUCCESS)
            {
                if (p_read_result->type == AICS_CHAR_INPUT_STATE)
                {
                    data_uart_print("AICS_CHAR_INPUT_STATE: read result, gain_setting %d, mute %d, gain_mode %d, change_counter 0x%x\r\n",
                                    p_read_result->data.input_state.gain_setting,
                                    p_read_result->data.input_state.mute,
                                    p_read_result->data.input_state.gain_mode,
                                    p_read_result->data.input_state.change_counter);
                }
                else if (p_read_result->type == AICS_CHAR_INPUT_STATUS)
                {
                    data_uart_print("AICS_CHAR_INPUT_STATUS: read result, input_status 0x%x\r\n",
                                    p_read_result->data.input_status);
                }
                else if (p_read_result->type == AICS_CHAR_GAIN_SETTING_PROP)
                {
                    data_uart_print("AICS_CHAR_GAIN_SETTING_PROP: read result, gain_setting_units %d, gain_setting_min %d, gain_setting_max %d\r\n",
                                    p_read_result->data.setting_prop.gain_setting_units,
                                    p_read_result->data.setting_prop.gain_setting_min,
                                    p_read_result->data.setting_prop.gain_setting_max);
                }
                else if (p_read_result->type == AICS_CHAR_INPUT_TYPE)
                {
                    data_uart_print("AICS_CHAR_INPUT_TYPE: read result, input_type 0x%x\r\n",
                                    p_read_result->data.input_type);
                }
                else if (p_read_result->type == AICS_CHAR_INPUT_DES)
                {
                    data_uart_print("AICS_CHAR_INPUT_DES: read result, input des len %d\r\n",
                                    p_read_result->data.input_des.input_des_len);
                    APP_PRINT_INFO2("AICS_CHAR_INPUT_DES: read result, input des[%d] %b",
                                    p_read_result->data.input_des.input_des_len,
                                    TRACE_BINARY(p_read_result->data.input_des.input_des_len,
                                                 p_read_result->data.input_des.p_input_des));
                }
            }
        }
        break;

    case LE_AUDIO_MSG_AICS_CLIENT_CP_RESULT:
        {
            T_AICS_CLIENT_CP_RESULT *p_cp_result = (T_AICS_CLIENT_CP_RESULT *)buf;
            APP_PRINT_INFO4("app_lea_com_cap_handle_msg: LE_AUDIO_MSG_AICS_CLIENT_CP_RESULT, conn_handle 0x%x, srv_instance_id %d, cause 0x%x, cp_op 0x%x",
                            p_cp_result->conn_handle, p_cp_result->srv_instance_id, p_cp_result->cause, p_cp_result->cp_op);
        }
        break;
#endif
    default:
        break;
    }
    return cb_result;
}

void app_lea_com_cap_init(void)
{
    T_CAP_INIT_PARAMS cap_init_param = {0};

    ble_audio_cback_register(app_lea_com_cap_handle_msg);
    cap_init_param.cap_role = CAP_ROLE;
    cap_init_param.cas_client = true;
#if CSIP_SET_COORDINATOR
    app_lea_com_csis_init(&cap_init_param);
#endif
#if VCP_VOLUME_CONTROLLER
    cap_init_param.vcp_micp.vcp_vcs_client = true;
#endif
#if MICP_MIC_CONTROLLER
    cap_init_param.vcp_micp.micp_mic_controller = true;
#endif
#if APP_LEA_AICS_CLIENT_SUPPORT
    cap_init_param.vcp_micp.vcp_aics_client = true;
#endif
#if APP_LEA_VOCS_CLIENT_SUPPORT
    cap_init_param.vcp_micp.vcp_vocs_client = true;
#endif
    cap_init(&cap_init_param);
}

