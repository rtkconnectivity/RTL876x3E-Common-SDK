/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "ble_audio.h"
#include "trace.h"
#include "app_lea_cap_com_main.h"
#include "ble_audio.h"
#include "csis_client.h"
#include "app_lea_cap_com_csis.h"
#include "app_lea_cap_com_scan.h"
#include "app_lea_cap_com_gap.h"
#include "app_lea_cap_com_cap.h"

#if CSIP_SET_COORDINATOR
uint16_t app_lea_com_csis_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_CSIS_CLIENT_DIS_DONE:
        {
            T_CSIS_CLIENT_DIS_DONE *p_dis_done = (T_CSIS_CLIENT_DIS_DONE *)buf;
            APP_PRINT_INFO4("LE_AUDIO_MSG_CSIS_CLIENT_DIS_DONE: conn_handle 0x%x, is_found %d, load_from_ftl %d, srv_num %d",
                            p_dis_done->conn_handle, p_dis_done->is_found,
                            p_dis_done->load_from_ftl, p_dis_done->srv_num);
            if (p_dis_done->is_found == false)
            {
                T_APP_LEA_GROUP_INFO *p_group = app_lea_find_group_by_conn_handle(p_dis_done->conn_handle);
                if (p_group == NULL)
                {
                    T_BLE_AUDIO_GROUP_HANDLE group_handle = ble_audio_group_allocate();
                    if (group_handle)
                    {
                        uint8_t bd_addr[6];
                        uint8_t addr_type;

                        ble_audio_group_reg_cb(group_handle, app_lea_com_group_cb);
                        if (gap_chann_get_addr(p_dis_done->conn_handle, bd_addr, &addr_type))
                        {
                            ble_audio_group_add_dev(group_handle, bd_addr, addr_type);
                            app_lea_add_group(group_handle, 0);
                        }
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CSIS_CLIENT_READ_RESULT:
        {
            T_BLE_AUDIO_GROUP_HANDLE group_handle = NULL;
            T_CSIS_CLIENT_READ_RESULT *p_read_result = (T_CSIS_CLIENT_READ_RESULT *)buf;

            APP_PRINT_INFO6("LE_AUDIO_MSG_CSIS_CLIENT_READ_RESULT: cause 0x%x, conn_handle 0x%x, group_handle %p, dev_handle %p, bd_addr %s, addr_type %d",
                            p_read_result->cause,
                            p_read_result->conn_handle,
                            p_read_result->group_handle,
                            p_read_result->dev_handle,
                            TRACE_BDADDR(p_read_result->mem_info.bd_addr),
                            p_read_result->mem_info.addr_type);
            APP_PRINT_INFO7("LE_AUDIO_MSG_CSIS_CLIENT_READ_RESULT: srv_instance_id %d, char_exit 0x%x, srv_uuid 0x%x, rank %d, set_mem_size %d, sirk_type %d, SIRK %b",
                            p_read_result->mem_info.srv_instance_id,
                            p_read_result->mem_info.char_exit,
                            p_read_result->mem_info.srv_uuid,
                            p_read_result->mem_info.rank,
                            p_read_result->mem_info.set_mem_size,
                            p_read_result->mem_info.sirk_type,
                            TRACE_BINARY(CSIS_SIRK_LEN, p_read_result->mem_info.sirk));
            if (p_read_result->cause == GAP_SUCCESS)
            {
                if (p_read_result->group_handle == NULL)
                {
                    if (set_coordinator_add_group(&group_handle, app_lea_com_group_cb,
                                                  &p_read_result->dev_handle, &p_read_result->mem_info))
                    {
                        app_lea_add_group(group_handle, p_read_result->mem_info.set_mem_size);
                    }
                }
                else
                {
                    if (p_read_result->dev_handle == NULL)
                    {
                        set_coordinator_add_dev(p_read_result->group_handle,
                                                &p_read_result->dev_handle, &p_read_result->mem_info);
                    }
                }
                T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_read_result->conn_handle);
                if (p_link)
                {
                    p_link->lea_disc_flags |= LEA_LINK_DISC_CSIS_DONE;
                }
            }
            if (p_read_result->group_handle == NULL && p_read_result->mem_info.set_mem_size > 1)
            {
                app_lea_group_scan_start(group_handle);
            }
        }
        break;

    case LE_AUDIO_MSG_CSIS_CLIENT_LOCK_STATE:
        {
            T_CSIS_CLIENT_LOCK_STATE *p_result = (T_CSIS_CLIENT_LOCK_STATE *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_CSIS_CLIENT_LOCK_STATE: group_handle %p, event 0x%x, state %d",
                            p_result->group_handle,
                            p_result->event,
                            p_result->lock_state);
        }
        break;

    case LE_AUDIO_MSG_CSIS_CLIENT_SEARCH_TIMEOUT:
        {
            T_CSIS_CLIENT_SEARCH_TIMEOUT *p_result = (T_CSIS_CLIENT_SEARCH_TIMEOUT *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_CSIS_CLIENT_SEARCH_TIMEOUT: group_handle %p, set_mem_size %d, search_done %d",
                            p_result->group_handle, p_result->set_mem_size,
                            p_result->search_done);
            app_lea_group_scan_stop();
        }
        break;

    case LE_AUDIO_MSG_CSIS_CLIENT_SEARCH_DONE:
        {
            T_CSIS_CLIENT_SEARCH_DONE *p_result = (T_CSIS_CLIENT_SEARCH_DONE *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_CSIS_CLIENT_SEARCH_DONE: group_handle %p, set_mem_size %d, search_done %d",
                            p_result->group_handle, p_result->set_mem_size,
                            p_result->search_done);
            app_lea_group_scan_stop();
        }
        break;

    case LE_AUDIO_MSG_CSIS_CLIENT_SET_MEM_FOUND:
        {
            T_CSIS_CLIENT_SET_MEM_FOUND *p_add = (T_CSIS_CLIENT_SET_MEM_FOUND *)buf;
            APP_PRINT_INFO8("LE_AUDIO_MSG_CSIS_CLIENT_SET_MEM_FOUND: group_handle %p, dev_handle %p, bd_addr %s, addr_type %d, srv_uuid 0x%x, rank %d, set_mem_size %d, SIRK %b",
                            p_add->group_handle,
                            p_add->dev_handle,
                            TRACE_BDADDR(p_add->bd_addr),
                            p_add->addr_type,
                            p_add->srv_uuid,
                            p_add->rank,
                            p_add->set_mem_size,
                            TRACE_BINARY(CSIS_SIRK_LEN, p_add->sirk));
            app_lea_add_conn_dev(p_add->addr_type, p_add->bd_addr);
        }
        break;

    default:
        break;
    }

    return cb_result;
}

void app_lea_com_csis_init(T_CAP_INIT_PARAMS *p_param)
{
    ble_audio_cback_register(app_lea_com_csis_handle_msg);
    p_param->csip_set_coordinator = true;
}
#endif

