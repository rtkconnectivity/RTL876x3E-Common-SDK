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
#include "bap.h"
#include "ble_isoch_def.h"
#include "trace.h"
#include "app_lea_cap_ini_main.h"
#include "app_lea_cap_ini_csis.h"
#include "app_lea_cap_ini_cap.h"
#include "app_lea_cap_ini_mcp.h"
#include "app_lea_cap_ini_ccp.h"
#include "app_lea_cap_ini_audio_data.h"
#include "ble_audio.h"
#include "metadata_def.h"

#define CAP_METADATA_DATA_MAX_LEN 20
uint8_t lea_ini_metadata[CAP_METADATA_DATA_MAX_LEN];
uint8_t lea_ini_metadata_len = 0;

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

#if BAP_UNICAST_CLIENT
bool app_lea_ini_unicast_audio_stop(T_BLE_AUDIO_GROUP_HANDLE group_handle, uint32_t cis_timeout_ms)
{
    T_APP_LEA_GROUP_INFO *p_group;

    p_group = app_lea_find_group(group_handle);
    if (p_group != NULL && p_group->lea_unicast.session_handle != NULL)
    {
        if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_PARTIAL_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STARTING)
        {
            if (bap_unicast_audio_stop(p_group->lea_unicast.session_handle, cis_timeout_ms))
            {
                return true;
            }
        }
        else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STOPPING)
        {
            return true;
        }
    }
    APP_PRINT_ERROR1("app_lea_ini_unicast_audio_stop: failed, group_handle 0x%x",
                     group_handle);
    return false;
}

bool app_lea_ini_unicast_audio_release(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;

    p_group = app_lea_find_group(group_handle);
    if (p_group != NULL && p_group->lea_unicast.session_handle != NULL)
    {
        p_group->lea_unicast.release_req = true;
        if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_CONFIGURED ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STARTING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_PARTIAL_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STOPPING)
        {
            if (bap_unicast_audio_release(p_group->lea_unicast.session_handle))
            {
                return true;
            }
        }
        else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE ||
                 p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED)
        {
            if (bap_unicast_audio_remove_session(p_group->lea_unicast.session_handle))
            {
                return true;
            }
        }
        else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_RELEASING)
        {
            return true;
        }
    }
    APP_PRINT_ERROR1("app_lea_ini_unicast_audio_release: failed, group_handle 0x%x",
                     group_handle);
    return false;
}

bool app_lea_ini_ready_to_start(T_APP_LEA_GROUP_INFO *p_group, uint16_t sink_context,
                                uint16_t src_context)
{
    uint8_t dev_num;
    uint8_t ready_dev_num = 0;
    T_AUDIO_DEV_INFO *p_dev_tbl;

    if (p_group->lea_unicast.session_handle)
    {
        if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_RELEASING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STARTING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_PARTIAL_STREAMING ||
            p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_STOPPING)
        {
            APP_PRINT_ERROR2("app_lea_ini_ready_to_start: failed, group_handle 0x%x, invalid bap_state %d",
                             p_group->group_handle, p_group->lea_unicast.bap_state);
            return false;
        }
    }

    dev_num = ble_audio_group_get_dev_num(p_group->group_handle);
    if (dev_num)
    {
        p_dev_tbl = calloc(1, dev_num * sizeof(T_AUDIO_DEV_INFO));
        if (p_dev_tbl)
        {
            if (ble_audio_group_get_info(p_group->group_handle, &dev_num, p_dev_tbl))
            {
                for (uint8_t j = 0; j < dev_num; j++)
                {
                    if (j == 0)
                    {
                        p_group->lea_unicast.dev_num = 1;
                        p_group->lea_unicast.dev_tbl[0] = p_dev_tbl[j].dev_handle;
                    }
                    else if (j == 1)
                    {
                        p_group->lea_unicast.dev_num = 2;
                        p_group->lea_unicast.dev_tbl[1] = p_dev_tbl[j].dev_handle;
                    }

                    if (p_dev_tbl[j].conn_state == GAP_CONN_STATE_CONNECTED)
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(p_dev_tbl[j].conn_handle);
                        if (p_link)
                        {
                            if (p_link->lea_srv_found_flags & LEA_LINK_DISC_CSIS_DONE)
                            {
                                if ((p_link->lea_disc_flags & LEA_LINK_DISC_CSIS_DONE) == 0)
                                {
                                    //wait csis read procedure done
                                    continue;
                                }
                            }
                            if (p_link->lea_srv_found_flags & LEA_LINK_DISC_PACS_DONE)
                            {
                                T_BAP_PACS_INFO pacs_info;
                                if (bap_pacs_get_info(p_link->conn_handle, &pacs_info))
                                {
                                    //Check available context
                                    if (sink_context != 0 &&
                                        (pacs_info.snk_avail_context & sink_context) == 0)
                                    {
                                        continue;
                                    }
                                    if (src_context != 0 &&
                                        (pacs_info.src_avail_context & src_context) == 0)
                                    {
                                        continue;
                                    }
                                    p_group->lea_unicast.ready_conn_handle = p_dev_tbl[j].conn_handle;
                                    ready_dev_num++;
                                }
                            }
                        }
                    }
                }
            }
            free(p_dev_tbl);
            if (ready_dev_num)
            {
                return true;
            }
        }
    }
    APP_PRINT_ERROR1("app_lea_ini_ready_to_start: failed, group_handle 0x%x",
                     p_group->group_handle);
    return false;
}

bool app_lea_ini_select_media_prefer_codec(T_APP_LEA_GROUP_INFO *p_group)
{
    uint32_t cfg_mask;
    uint8_t channel_count = 1;
    uint32_t sup_snk_mask = 0;
    T_CODEC_CFG cfg;

    cfg_mask = bap_unicast_get_audio_cfg_mask(p_group->lea_unicast.session_handle,
                                              LEA_MEDIA_AUDIO_CFG_SUPPORT,
                                              p_group->lea_unicast.dev_num, p_group->lea_unicast.dev_tbl);
    if (cfg_mask)
    {
        uint8_t idx = 0;

        if ((cfg_mask & LEA_MEDIA_AUDIO_CFG_PREFER) != 0)
        {
            cfg_mask &= LEA_MEDIA_AUDIO_CFG_PREFER;
        }

        while (cfg_mask)
        {
            if (cfg_mask & 0x01)
            {
                p_group->lea_unicast.cfg_type = (T_UNICAST_AUDIO_CFG_TYPE)idx;
                break;
            }
            cfg_mask >>= 1;
            idx++;
        }

        if (p_group->lea_unicast.cfg_type == UNICAST_AUDIO_CFG_4)
        {
            channel_count = 2;
        }

        sup_snk_mask = bap_pacs_get_lc3_snk_table_msk(p_group->lea_unicast.ready_conn_handle,
                                                      AUDIO_CONTEXT_MEDIA,
                                                      channel_count, 1);
        if (!sup_snk_mask)
        {
            sup_snk_mask = bap_pacs_get_lc3_snk_table_msk(p_group->lea_unicast.ready_conn_handle,
                                                          AUDIO_CONTEXT_UNSPECIFIED,
                                                          channel_count, 1);
        }
        if (sup_snk_mask != 0)
        {
            if ((sup_snk_mask & LEA_MEDIA_CODEC_CFG_SUPPORT) != 0)
            {
                sup_snk_mask &= LEA_MEDIA_CODEC_CFG_SUPPORT;
            }
            idx = 0;
            while (sup_snk_mask)
            {
                if (sup_snk_mask & 0x01)
                {
                    if (codec_preferred_cfg_get((T_CODEC_CFG_ITEM)idx, &cfg))
                    {
                        p_group->lea_unicast.codec_cfg_item = (T_CODEC_CFG_ITEM)idx;
                        APP_PRINT_INFO2("app_lea_ini_select_media_prefer_codec: cfg_type %d, codec_cfg_item %d",
                                        p_group->lea_unicast.cfg_type, p_group->lea_unicast.codec_cfg_item);
                        return true;
                    }
                }
                sup_snk_mask >>= 1;
                idx++;
            }
        }
    }

    return false;
}

bool app_lea_ini_select_conversation_prefer_codec(T_APP_LEA_GROUP_INFO *p_group)
{
    uint32_t cfg_mask;
    uint8_t channel_count = 1;
    uint32_t sup_snk_mask = 0;
    T_CODEC_CFG cfg;

    cfg_mask = bap_unicast_get_audio_cfg_mask(p_group->lea_unicast.session_handle,
                                              LEA_CONVERSATIONAL_AUDIO_CFG_SUPPORT,
                                              p_group->lea_unicast.dev_num, p_group->lea_unicast.dev_tbl);
    if (cfg_mask)
    {
        uint8_t idx = 0;

        if ((cfg_mask & LEA_CONVERSATIONAL_AUDIO_CFG_PREFER) != 0)
        {
            cfg_mask &= LEA_CONVERSATIONAL_AUDIO_CFG_PREFER;
        }

        while (cfg_mask)
        {
            if (cfg_mask & 0x01)
            {
                p_group->lea_unicast.cfg_type = (T_UNICAST_AUDIO_CFG_TYPE)idx;
                break;
            }
            cfg_mask >>= 1;
            idx++;
        }

        if (p_group->lea_unicast.cfg_type == UNICAST_AUDIO_CFG_5)
        {
            channel_count = 2;
        }

        sup_snk_mask = bap_pacs_get_lc3_snk_table_msk(p_group->lea_unicast.ready_conn_handle,
                                                      AUDIO_CONTEXT_CONVERSATIONAL,
                                                      channel_count, 1);
        if (!sup_snk_mask)
        {
            sup_snk_mask = bap_pacs_get_lc3_snk_table_msk(p_group->lea_unicast.ready_conn_handle,
                                                          AUDIO_CONTEXT_UNSPECIFIED,
                                                          channel_count, 1);
        }
        if (sup_snk_mask != 0)
        {
            if ((sup_snk_mask & LEA_CONVERSATIONAL_CODEC_CFG_SUPPORT) != 0)
            {
                sup_snk_mask &= LEA_CONVERSATIONAL_CODEC_CFG_SUPPORT;
            }
            idx = 0;
            while (sup_snk_mask)
            {
                if (sup_snk_mask & 0x01)
                {
                    if (codec_preferred_cfg_get((T_CODEC_CFG_ITEM)idx, &cfg))
                    {
                        p_group->lea_unicast.codec_cfg_item = (T_CODEC_CFG_ITEM)idx;
                        APP_PRINT_INFO2("app_lea_ini_select_conversation_prefer_codec: cfg_type %d, codec_cfg_item %d",
                                        p_group->lea_unicast.cfg_type, p_group->lea_unicast.codec_cfg_item);
                        return true;
                    }
                }
                sup_snk_mask >>= 1;
                idx++;
            }
        }
    }

    return false;
}

void app_lea_ini_gen_metadata(uint16_t contexts_type, uint8_t ccid_num, uint8_t *p_ccid)
{
    uint8_t idx = 0;
    //contexts_type
    lea_ini_metadata[idx++] = 0x03;
    lea_ini_metadata[idx++] = METADATA_TYPE_STREAMING_AUDIO_CONTEXTS;
    LE_UINT16_TO_ARRAY(lea_ini_metadata + idx,
                       contexts_type);
    idx += 2;
    //ccid
    if (ccid_num && p_ccid != NULL)
    {
        lea_ini_metadata[idx++] = 1 + ccid_num;
        lea_ini_metadata[idx++] = METADATA_TYPE_CCID_LIST;
        memcpy(lea_ini_metadata + idx, p_ccid, ccid_num);
        idx += ccid_num;
    }
    lea_ini_metadata_len = idx;
}

uint32_t app_lea_ini_get_channel_location(T_BAP_PACS_INFO *p_info, uint8_t direction,
                                          uint8_t channel_num, uint32_t used_loc)
{
    uint32_t temp_audio_loc;
    uint32_t audio_loc = 0;
    uint16_t index = 0;
    uint8_t num = 0;

    if (direction == SERVER_AUDIO_SINK)
    {
        temp_audio_loc = p_info->snk_audio_loc;
    }
    else
    {
        temp_audio_loc = p_info->src_audio_loc;
    }
    temp_audio_loc &= ~used_loc;
    while (temp_audio_loc)
    {
        if (temp_audio_loc & 0x01)
        {
            audio_loc |= (1 << index);
            num++;
        }
        temp_audio_loc >>= 1;
        index++;
        if (num >= channel_num)
        {
            break;
        }
    }
    if (num != channel_num)
    {
        APP_PRINT_TRACE3("app_lea_ini_get_channel_location: get failed, direction 0x%x, channel_num %d, used_loc 0x%x",
                         direction, channel_num, used_loc);
        return 0;
    }
    return audio_loc;
}

bool app_lea_ini_config_codec(T_APP_LEA_GROUP_INFO *p_group)
{
    T_BAP_UNICAST_SESSION_INFO session_info;
    T_AUDIO_ASE_CODEC_CFG    ase_codec_cfg = {0};

    if (codec_preferred_cfg_get(p_group->lea_unicast.codec_cfg_item, &ase_codec_cfg.codec_cfg) == false)
    {
        goto failed;
    }
    ase_codec_cfg.codec_id[0] = LC3_CODEC_ID;
    ase_codec_cfg.target_latency = p_group->lea_unicast.target_latency;
    ase_codec_cfg.target_phy = ASE_TARGET_PHY_2M;

    if (bap_unicast_audio_get_session_info(p_group->lea_unicast.session_handle, &session_info))
    {
#if 0
        APP_PRINT_INFO4("app_lea_ini_config_codec: session info, state %d, cfg_type %d, conn_dev_num %d, dev_num %d",
                        session_info.state, session_info.cfg_type,
                        session_info.conn_dev_num, session_info.dev_num);
        for (uint8_t i = 0; i < session_info.dev_num; i++)
        {
            APP_PRINT_INFO4("dev_info[%d]: dev_handle %p, conn_state %d, ase_num %d",
                            i, session_info.dev_info[i].dev_handle,
                            session_info.dev_info[i].conn_state,
                            session_info.dev_info[i].ase_num);
            for (uint8_t j = 0; j < session_info.dev_info[i].ase_num; j++)
            {
                APP_PRINT_INFO6("ase_info[%d]: ase_id %d, cfg_idx %d, ase_state 0x%x, direction %d, channel_num %d",
                                j, session_info.dev_info[i].ase_info[j].ase_id,
                                session_info.dev_info[i].ase_info[j].cfg_idx,
                                session_info.dev_info[i].ase_info[j].ase_state,
                                session_info.dev_info[i].ase_info[j].direction,
                                session_info.dev_info[i].ase_info[j].channel_num);
            }
        }
#endif
        uint32_t sink_channel_allocation;
        uint32_t source_channel_allocation;

        p_group->lea_unicast.sink_used_location = 0;
        p_group->lea_unicast.source_used_location = 0;
        ase_codec_cfg.codec_cfg.type_exist |= CODEC_CFG_AUDIO_CHANNEL_ALLOCATION_EXIST;

        for (uint8_t i = 0; i < session_info.dev_num; i++)
        {
            T_AUDIO_DEV_INFO dev_info;
            T_BAP_PACS_INFO pacs_info;

            if (ble_audio_group_get_dev_info(p_group->group_handle,
                                             session_info.dev_info[i].dev_handle, &dev_info) == false)
            {
                continue;
            }

            if (dev_info.conn_state != GAP_CONN_STATE_CONNECTED)
            {
                continue;
            }

            if (bap_pacs_get_info(dev_info.conn_handle, &pacs_info) == false)
            {
                continue;
            }

            for (uint8_t j = 0; j < session_info.dev_info[i].ase_num; j++)
            {
                if (session_info.dev_info[i].ase_info[j].direction == SERVER_AUDIO_SINK)
                {
                    sink_channel_allocation = app_lea_ini_get_channel_location(&pacs_info,
                                                                               session_info.dev_info[i].ase_info[j].direction,
                                                                               session_info.dev_info[i].ase_info[j].channel_num,
                                                                               p_group->lea_unicast.sink_used_location);
                    ase_codec_cfg.codec_cfg.audio_channel_allocation = sink_channel_allocation;
                    p_group->lea_unicast.sink_used_location |= sink_channel_allocation;
                }
                else
                {
                    source_channel_allocation = app_lea_ini_get_channel_location(&pacs_info,
                                                                                 session_info.dev_info[i].ase_info[j].direction,
                                                                                 session_info.dev_info[i].ase_info[j].channel_num,
                                                                                 p_group->lea_unicast.source_used_location);
                    ase_codec_cfg.codec_cfg.audio_channel_allocation = source_channel_allocation;
                    p_group->lea_unicast.source_used_location |= source_channel_allocation;
                }
                if (bap_unicast_audio_cfg_ase_codec(p_group->lea_unicast.session_handle,
                                                    session_info.dev_info[i].dev_handle,
                                                    session_info.dev_info[i].ase_info[j].cfg_idx, &ase_codec_cfg) == false)
                {
                    goto failed;
                }
            }
        }

    }
    else
    {
        goto failed;
    }
    return true;
failed:
    return false;
}

bool app_lea_ini_start_media(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;
    uint8_t err_idx = 0;

    p_group = app_lea_find_group(group_handle);
    if (p_group)
    {
        if (app_lea_ini_ready_to_start(p_group, AUDIO_CONTEXT_MEDIA, 0))
        {
            if (p_group->lea_unicast.session_handle == NULL)
            {
                p_group->lea_unicast.session_handle = audio_stream_session_allocate(p_group->group_handle);
                if (p_group->lea_unicast.session_handle == NULL)
                {
                    err_idx = 1;
                    goto failed;
                }
#if MCP_MEDIA_CONTROL_SERVER
                if (app_db.mcp_db.mcp_enabled_cccd)
                {
                    p_group->lea_unicast.ccid_num = 1;
                    p_group->lea_unicast.ccid[0] = APP_LEA_GMCS_CCID;
                }
#endif
                p_group->lea_unicast.bap_state = AUDIO_STREAM_STATE_IDLE;
                p_group->lea_unicast.contexts_type = AUDIO_CONTEXT_MEDIA;
                p_group->lea_unicast.target_latency = ASE_TARGET_HIGHER_RELIABILITY;
            }

            if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE)
            {
                if (app_lea_ini_select_media_prefer_codec(p_group) == false)
                {
                    err_idx = 2;
                    goto failed;
                }
                if (bap_unicast_audio_cfg(p_group->lea_unicast.session_handle, p_group->lea_unicast.cfg_type,
                                          p_group->lea_unicast.dev_num, p_group->lea_unicast.dev_tbl) == false)
                {
                    err_idx = 3;
                    goto failed;
                }
                if (app_lea_ini_config_codec(p_group) == false)
                {
                    err_idx = 4;
                    goto failed;
                }
                if (bap_unicast_audio_start(p_group->lea_unicast.session_handle) == false)
                {
                    err_idx = 5;
                    goto failed;
                }
            }
            else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED ||
                     p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_CONFIGURED)
            {
                if (p_group->lea_unicast.contexts_type != AUDIO_CONTEXT_MEDIA)
                {
                    err_idx = 6;
                    goto failed;
                }
                if (bap_unicast_audio_start(p_group->lea_unicast.session_handle) == false)
                {
                    err_idx = 7;
                    goto failed;
                }
            }
            else
            {
                err_idx = 8;
                goto failed;
            }
        }
        else
        {
            err_idx = 9;
            goto failed;
        }
    }
    else
    {
        err_idx = 10;
        goto failed;
    }
    return true;
failed:
    APP_PRINT_ERROR2("app_lea_ini_start_media: failed, group_handle 0x%x, err_idx %d",
                     group_handle, err_idx);
    return false;
}

bool app_lea_ini_start_conversation(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_APP_LEA_GROUP_INFO *p_group;
    uint8_t err_idx = 0;

    p_group = app_lea_find_group(group_handle);
    if (p_group)
    {
        if (app_lea_ini_ready_to_start(p_group, AUDIO_CONTEXT_CONVERSATIONAL, AUDIO_CONTEXT_CONVERSATIONAL))
        {
            if (p_group->lea_unicast.session_handle == NULL)
            {
                p_group->lea_unicast.session_handle = audio_stream_session_allocate(p_group->group_handle);
                if (p_group->lea_unicast.session_handle == NULL)
                {
                    err_idx = 1;
                    goto failed;
                }
#if CCP_CALL_CONTROL_SERVER
                if (app_db.ccp_db.ccp_enabled_cccd)
                {
                    p_group->lea_unicast.ccid_num = 1;
                    p_group->lea_unicast.ccid[0] = APP_LEA_GTBS_CCID;
                }
#endif
                p_group->lea_unicast.bap_state = AUDIO_STREAM_STATE_IDLE;
                p_group->lea_unicast.contexts_type = AUDIO_CONTEXT_CONVERSATIONAL;
                p_group->lea_unicast.target_latency = ASE_TARGET_LOWER_LATENCY;
            }

            if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE)
            {
                if (app_lea_ini_select_conversation_prefer_codec(p_group) == false)
                {
                    err_idx = 2;
                    goto failed;
                }
                if (bap_unicast_audio_cfg(p_group->lea_unicast.session_handle, p_group->lea_unicast.cfg_type,
                                          p_group->lea_unicast.dev_num, p_group->lea_unicast.dev_tbl) == false)
                {
                    err_idx = 3;
                    goto failed;
                }
                if (app_lea_ini_config_codec(p_group) == false)
                {
                    err_idx = 4;
                    goto failed;
                }
                if (bap_unicast_audio_start(p_group->lea_unicast.session_handle) == false)
                {
                    err_idx = 5;
                    goto failed;
                }
            }
            else if (p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED ||
                     p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_CONFIGURED)
            {
                if (p_group->lea_unicast.contexts_type != AUDIO_CONTEXT_CONVERSATIONAL)
                {
                    err_idx = 6;
                    goto failed;
                }
                if (bap_unicast_audio_start(p_group->lea_unicast.session_handle) == false)
                {
                    err_idx = 7;
                    goto failed;
                }
            }
            else
            {
                err_idx = 8;
                goto failed;
            }
        }
        else
        {
            err_idx = 9;
            goto failed;
        }
    }
    else
    {
        err_idx = 10;
        goto failed;
    }

    return true;
failed:
    APP_PRINT_ERROR2("app_lea_ini_start_conversation: failed, group_handle 0x%x, err_idx %d",
                     group_handle, err_idx);
    return false;
}
#endif

void app_lea_ini_group_cb(T_AUDIO_GROUP_MSG msg, T_BLE_AUDIO_GROUP_HANDLE handle,
                          void *buf)
{
    T_APP_LEA_GROUP_INFO *p_group = app_lea_find_group(handle);
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
#if BAP_UNICAST_CLIENT
    case AUDIO_GROUP_MSG_BAP_STATE:
        {
            T_AUDIO_GROUP_BAP_STATE *p_data = (T_AUDIO_GROUP_BAP_STATE *)buf;
            APP_PRINT_INFO6("AUDIO_GROUP_MSG_BAP_STATE: group handle 0x%x, session handle 0x%x, curr_action %d, state %d, result %d, cause 0x%x",
                            handle, p_data->handle, p_data->curr_action,
                            p_data->state, p_data->result, p_data->cause);
            data_uart_print("BAP state: %d\r\n", p_data->state);
            if (p_group != NULL && p_group->lea_unicast.session_handle == p_data->handle)
            {
                p_group->lea_unicast.bap_state = p_data->state;
                if ((p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE ||
                     p_group->lea_unicast.bap_state == AUDIO_STREAM_STATE_IDLE_CONFIGURED) &&
                    p_group->lea_unicast.release_req == true)
                {
                    bap_unicast_audio_remove_session(p_group->lea_unicast.session_handle);
                }
            }
            if (p_data->result == BAP_UNICAST_RESULT_ASE_CP_ERR)
            {
                APP_PRINT_INFO5("AUDIO_GROUP_MSG_BAP_STATE: cp error, conn_handle 0x%x, opcode 0x%x, ase_id %d, response_code 0x%x, reason 0x%x",
                                p_data->addl_info.cp_error.conn_handle, p_data->addl_info.cp_error.opcode,
                                p_data->addl_info.cp_error.ase_id,
                                p_data->addl_info.cp_error.response_code, p_data->addl_info.cp_error.reason);
            }
        }
        break;

    case AUDIO_GROUP_MSG_BAP_START_QOS_CFG:
        {
            T_AUDIO_GROUP_BAP_START_QOS_CFG *p_data = (T_AUDIO_GROUP_BAP_START_QOS_CFG *)buf;
            APP_PRINT_INFO4("AUDIO_GROUP_MSG_BAP_START_QOS_CFG: group handle 0x%x, session handle 0x%x, sink latency 0x%x, source latency 0x%x",
                            handle, p_data->handle,
                            p_data->sink_transport_latency_max, p_data->source_transport_latency_max);
            APP_PRINT_INFO8("AUDIO_GROUP_MSG_BAP_START_QOS_CFG: sink preferred presentation delay(0x%x-0x%x), sink presentation delay(0x%x-0x%x), source preferred presentation delay(0x%x-0x%x), source presentation delay(0x%x-0x%x)",
                            p_data->sink_preferred_presentation_delay_min, p_data->sink_preferred_presentation_delay_max,
                            p_data->sink_presentation_delay_min, p_data->sink_presentation_delay_max,
                            p_data->source_preferred_presentation_delay_min, p_data->source_preferred_presentation_delay_max,
                            p_data->source_presentation_delay_min, p_data->source_presentation_delay_max);

#if 0 //Print QoS information
            T_AUDIO_SESSION_QOS_CFG qos_cfg;
            if (bap_unicast_audio_get_session_qos(p_data->handle, &qos_cfg) == false)
            {
                break;
            }
            APP_PRINT_INFO5("session qos: sca %d, packing %d, framing %d, sdu_interval_m_s %d, sdu_interval_s_m %d",
                            qos_cfg.sca,
                            qos_cfg.packing,
                            qos_cfg.framing,
                            qos_cfg.sdu_interval_m_s,
                            qos_cfg.sdu_interval_s_m);
            APP_PRINT_INFO4("session qos: latency_m_s 0x%x, latency_s_m 0x%x, sink_presentation_delay 0x%x, source_presentation_delay 0x%x",
                            qos_cfg.latency_m_s,
                            qos_cfg.latency_s_m,
                            qos_cfg.sink_presentation_delay,
                            qos_cfg.source_presentation_delay);
            T_AUDIO_ASE_QOS_CFG ase_qos_cfg;
            T_BAP_UNICAST_SESSION_INFO session_info;
            T_BAP_UNICAST_DEV_INFO *p_dev;
            if (bap_unicast_audio_get_session_info(p_data->handle, &session_info))
            {
                for (uint8_t i = 0; i < session_info.dev_num; i++)
                {
                    p_dev = &session_info.dev_info[i];
                    if (p_dev->conn_state == GAP_CONN_STATE_CONNECTED)
                    {
                        for (uint8_t j = 0; j < p_dev->ase_num; j++)
                        {
                            if (bap_unicast_audio_get_ase_qos(p_data->handle, p_dev->dev_handle, p_dev->ase_info[j].ase_id,
                                                              &ase_qos_cfg))
                            {
                                APP_PRINT_INFO3("ase qos: phy 0x%x, max_sdu %d, retransmission_number %d",
                                                ase_qos_cfg.phy,
                                                ase_qos_cfg.max_sdu,
                                                ase_qos_cfg.retransmission_number);
                            }
                        }
                    }
                }
            }
#endif
        }
        break;

    case AUDIO_GROUP_MSG_BAP_START_METADATA_CFG:
        {
            T_AUDIO_GROUP_BAP_START_METADATA_CFG *p_data = (T_AUDIO_GROUP_BAP_START_METADATA_CFG *)buf;

            APP_PRINT_INFO4("AUDIO_GROUP_MSG_BAP_START_METADATA_CFG: group handle 0x%x, session handle 0x%x, dev_handle 0x%x, ase_id %d",
                            handle, p_data->handle, p_data->dev_handle, p_data->ase_id);
            if (p_group != NULL && p_group->lea_unicast.session_handle == p_data->handle)
            {
                app_lea_ini_gen_metadata(p_group->lea_unicast.contexts_type, p_group->lea_unicast.ccid_num,
                                         p_group->lea_unicast.ccid);
            }
            bap_unicast_audio_cfg_ase_metadata(p_data->handle, p_data->dev_handle, p_data->ase_id,
                                               lea_ini_metadata_len, (uint8_t *)lea_ini_metadata);
        }
        break;

    case AUDIO_GROUP_MSG_BAP_SESSION_REMOVE:
        {
            T_AUDIO_GROUP_BAP_SESSION_REMOVE *p_data = (T_AUDIO_GROUP_BAP_SESSION_REMOVE *)buf;

            APP_PRINT_INFO2("AUDIO_GROUP_MSG_BAP_SESSION_REMOVE: group handle 0x%x, session handle 0x%x",
                            handle, p_data->handle);
            if (p_group != NULL && p_group->lea_unicast.session_handle == p_data->handle)
            {
                memset(&p_group->lea_unicast, 0, sizeof(T_APP_LEA_UNICAST));
            }
        }
        break;

    case AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH:
        {
            T_AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH *p_data = (T_AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH *)buf;
            APP_PRINT_INFO6("AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH: group handle 0x%x, session handle 0x%x, dev_handle 0x%x, ase_id 0x%x, direction %d, cis_conn_handle 0x%x",
                            handle, p_data->handle,
                            p_data->dev_handle,
                            p_data->ase_id,
                            p_data->path_direction,
                            p_data->cis_conn_handle);
            T_LEA_SETUP_DATA_PATH data = {0};
            data.iso_mode = CIG_ISO_MODE;
            data.iso_conn_handle = p_data->cis_conn_handle;
            data.codec_parsed_data = p_data->codec_parsed_data;
            data.path_direction = p_data->path_direction;
            data.presentation_delay = 40000;
            if (p_group != NULL && p_group->lea_unicast.session_handle == p_data->handle)
            {
                T_AUDIO_SESSION_QOS_CFG qos_cfg;
                if (bap_unicast_audio_get_session_qos(p_group->lea_unicast.session_handle, &qos_cfg))
                {
                    if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
                    {
                        data.presentation_delay = qos_cfg.sink_presentation_delay;
                    }
                    else
                    {
                        data.presentation_delay = qos_cfg.source_presentation_delay;
                    }
                }
            }
            app_lea_handle_data_path_setup(&data);
        }
        break;

    case AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH:
        {
            T_AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH *p_data = (T_AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH *)buf;
            APP_PRINT_INFO7("AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH: group handle 0x%x, session handle 0x%x, dev_handle 0x%x, ase_id 0x%x, direction %d, cis_conn_handle 0x%x, cause 0x%x",
                            handle, p_data->handle,
                            p_data->dev_handle,
                            p_data->ase_id,
                            p_data->path_direction,
                            p_data->cis_conn_handle,
                            p_data->cause);
            T_LEA_REMOVE_DATA_PATH data;
            data.iso_mode = CIG_ISO_MODE;
            data.iso_conn_handle = p_data->cis_conn_handle;
            data.path_direction = p_data->path_direction;
            app_lea_handle_data_path_remove(&data);
        }
        break;

    case AUDIO_GROUP_MSG_BAP_METADATA_UPDATE:
        {
            T_AUDIO_GROUP_MSG_BAP_METADATA_UPDATE *p_data = (T_AUDIO_GROUP_MSG_BAP_METADATA_UPDATE *)buf;
            APP_PRINT_INFO6("AUDIO_GROUP_MSG_BAP_METADATA_UPDATE: group handle 0x%x, session handle 0x%x, dev_handle 0x%x, ase_id 0x%x, metadata_length %d, p_metadata %b",
                            handle, p_data->handle,
                            p_data->dev_handle,
                            p_data->ase_id,
                            p_data->metadata_length,
                            TRACE_BINARY(p_data->metadata_length, p_data->p_metadata));
        }
        break;
#endif
    default:
        break;
    }
    return;
}

uint16_t app_lea_ini_cap_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_APP_LE_LINK *p_link = NULL;

    switch (msg)
    {
    case LE_AUDIO_MSG_CAP_DIS_DONE:
        {
            T_CAP_DIS_DONE *p_data = (T_CAP_DIS_DONE *)buf;
            APP_PRINT_INFO6("LE_AUDIO_MSG_CAP_DIS_DONE: conn_handle 0x%x, load_from_ftl %d, cas_is_found %d, cas_inc_csis %d, vcs_is_found %d, mics_is_found %d",
                            p_data->conn_handle,
                            p_data->load_from_ftl,
                            p_data->cas_is_found,
                            p_data->cas_inc_csis,
                            p_data->vcs_is_found,
                            p_data->mics_is_found);
            p_link = app_link_find_le_link_by_conn_handle(p_data->conn_handle);
            if (p_link)
            {
                if (p_data->cas_is_found)
                {
                    p_link->lea_disc_flags |= LEA_LINK_DISC_CAS_DONE;
                    p_link->lea_srv_found_flags |= LEA_LINK_DISC_CAS_DONE;
                    if (p_data->cas_inc_csis)
                    {
                        p_link->lea_srv_found_flags |= LEA_LINK_DISC_CSIS_DONE;
                    }
                }
            }
        }
        break;

    case LE_AUDIO_MSG_CLIENT_ATTR_CCCD_INFO:
        {
            T_CLIENT_ATTR_CCCD_INFO *p_data = (T_CLIENT_ATTR_CCCD_INFO *)buf;
            APP_PRINT_INFO8("LE_AUDIO_MSG_CLIENT_ATTR_CCCD_INFO: conn_handle 0x%x, srv_cfg %d, srv_instance_id %d, srv_uuid 0x%x, char_instance_id %d, char_uuid 0x%x, cccd_cfg 0x%x, cause 0x%x",
                            p_data->conn_handle,
                            p_data->srv_cfg,
                            p_data->srv_instance_id,
                            p_data->srv_uuid,
                            p_data->char_instance_id,
                            p_data->char_uuid,
                            p_data->cccd_cfg,
                            p_data->cause);
        }
        break;

    default:
        break;
    }
    return cb_result;
}

void app_lea_ini_cap_init(void)
{
    T_CAP_INIT_PARAMS cap_init_param = {0};

    ble_audio_cback_register(app_lea_ini_cap_handle_msg);
    app_db.cap_role = CAP_ROLE;
    cap_init_param.cap_role = app_db.cap_role;
    cap_init_param.cas_client = true;
#if CSIP_SET_COORDINATOR
    app_lea_ini_csis_init(&cap_init_param);
#endif
#if VCP_VOLUME_CONTROLLER
    cap_init_param.vcp_micp.vcp_vcs_client = true;
#endif
#if MICP_MIC_CONTROLLER
    cap_init_param.vcp_micp.micp_mic_controller = true;
#endif
#if MCP_MEDIA_CONTROL_SERVER
    app_lea_ini_mcp_init_cap(&cap_init_param);
#endif
#if CCP_CALL_CONTROL_SERVER
    app_lea_ini_ccp_init_cap(&cap_init_param);
#endif
    cap_init(&cap_init_param);
}

