/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "gap.h"
#include "trace.h"
#include "stdlib.h"
#include "gap_bond_le.h"
#include "gap_conn_le.h"
#include "app_lea_cap_ini_user_cmd.h"
#include "app_lea_cap_ini_link.h"
#include "app_lea_cap_ini_scan.h"
#include "app_lea_cap_ini_main.h"
#include "app_lea_cap_ini_gap.h"
#include "app_lea_cap_ini_cap.h"
#include "app_lea_cap_ini_bap.h"
#include "app_lea_cap_ini_bap_bsrc.h"
#include "app_lea_cap_ini_mcp.h"
#include "ble_conn.h"
#include "cap.h"

/** @defgroup  APP_LEA_CAP_INI_CMD CAP Initiator User Command
    * @brief This file handles CAP Initiator User Command.
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @brief User command interface data, used to parse the commands from Data UART. */
T_USER_CMD_IF    user_cmd_if;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief LE connection param update request
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "conupdreq",
        "conupdreq [conn_id] [interval_min] [interval_max] [latency] [supervision_timeout]\n\r",
        "LE connection param update request\r\n\
        sample: conupdreq 0 0x30 0x40 0 500\n\r",
        cmd_conupdreq
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_conupdreq(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret;
    uint8_t  conn_id = p_parse_value->dw_param[0];
    uint16_t conn_interval_min = p_parse_value->dw_param[1];
    uint16_t conn_interval_max = p_parse_value->dw_param[2];
    uint16_t conn_latency = p_parse_value->dw_param[3];
    uint16_t supervision_timeout = p_parse_value->dw_param[4];

    ret = ble_set_prefer_conn_param(conn_id, conn_interval_min,
                                    conn_interval_max, conn_latency,
                                    supervision_timeout);
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

/**
 * @brief Disconnect to remote device
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "disc",
        "disc [conn_id]\n\r",
        "Disconnect to remote device\n\r",
        cmd_disc
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_disc(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;
    cause = le_disconnect(conn_id);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

/**
 * @brief Show scan dev list
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "showdev",
        "showdev\n\r",
        "Show scan dev list\n\r",
        cmd_showdev
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_showdev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_APP_LEA_SCAN_DEV *p_dev;
    data_uart_print("dev list num: %d\r\n", app_db.scan_dev_queue.count);
    for (uint8_t i = 0; i < app_db.scan_dev_queue.count; i++)
    {
        p_dev = (T_APP_LEA_SCAN_DEV *)os_queue_peek(&app_db.scan_dev_queue, i);
        if (p_dev != NULL)
        {
            data_uart_print("RemoteBd[%d] = [%02x:%02x:%02x:%02x:%02x:%02x], type %d, adv_data_flags 0x%x\r\n",
                            i,
                            p_dev->bd_addr[5], p_dev->bd_addr[4],
                            p_dev->bd_addr[3], p_dev->bd_addr[2],
                            p_dev->bd_addr[1], p_dev->bd_addr[0], p_dev->bd_type,
                            p_dev->adv_data.adv_data_flags);
        }
    }
    return (RESULT_SUCESS);
}

/**
 * @brief Connect to remote device: use showdev to show idx
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "condev",
        "condev [dev_idx]\n\r",
        "Connect to remote device: use showdev to show idx\r\n\
        [dev_idx]: use cmd showdev to show idx before use this cmd\r\n\
        sample: condev 0\n\r",
        cmd_condev
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_condev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t dev_idx = p_parse_value->dw_param[0];
    if (dev_idx < app_db.scan_dev_queue.count)
    {
        T_APP_LEA_SCAN_DEV *p_dev = (T_APP_LEA_SCAN_DEV *)os_queue_peek(&app_db.scan_dev_queue, dev_idx);
        if (p_dev)
        {
            app_lea_add_conn_dev(p_dev->bd_type, p_dev->bd_addr);
        }
        return RESULT_SUCESS;
    }
    else
    {
        return RESULT_ERR;
    }
}

/**
 * @brief Clear all bonded devices information
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bondclear",
        "bondclear\n\r",
        "Clear all bonded devices information\n\r",
        cmd_bondclear
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bondclear(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_bond_clear_all_keys();
    data_uart_print("le_clear_all_keys\r\n");
    return (RESULT_SUCESS);
}

/**
 * @brief Start extended scan
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "leascan",
        "leascan \n\r",
        "Start extended scan\n\r",
        cmd_leascan
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_leascan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_lea_scan_start();

    return (RESULT_SUCESS);
}

/**
 * @brief Stop extended scan
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "leastopscan",
        "leastopscan\n\r",
        "Stop extended scan\n\r",
        cmd_leastopscan
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_leastopscan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_lea_scan_stop();
    return (RESULT_SUCESS);
}

/**
 * @brief Start group scan
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "gscan",
        "gscan [group_idx]\n\r",
        "Start group scan\n\r",
        cmd_gscan
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_gscan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t group_idx = p_parse_value->dw_param[0];

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group && p_group->set_mem_size != 0)
        {
            app_lea_group_scan_start(p_group->group_handle);
            return (RESULT_SUCESS);
        }
    }
    return (RESULT_ERR);
}

/**
 * @brief Stop group scan
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "stopgscan",
        "stopgscan\n\r",
        "Stop group scan\n\r",
        cmd_stopgscan
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_stopgscan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_lea_group_scan_stop();
    return (RESULT_SUCESS);
}

/**
 * @brief Show group list
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "showgroup",
        "showgroup\n\r",
        "Show group list\n\r",
        cmd_showgroup
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_showgroup(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_APP_LEA_GROUP_INFO *p_group;

    data_uart_print("group list num: %d\r\n", app_db.group_handle_queue.count);
    for (uint8_t i = 0; i < app_db.group_handle_queue.count; i++)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue, i);
        if (p_group != NULL)
        {
            uint8_t dev_num;
            T_AUDIO_DEV_INFO *p_dev_tbl;

            data_uart_print("Group[%d]: group_handle 0x%x, set_mem_size %d\r\n",
                            i, p_group->group_handle, p_group->set_mem_size);
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
                            data_uart_print("    Dev[%d]: is_used %d, conn_state %d\r\n",
                                            j, p_dev_tbl[j].is_used, p_dev_tbl[j].conn_state);
                        }
                    }
                    free(p_dev_tbl);
                }
            }
        }
    }
    return (RESULT_SUCESS);
}

#if VCP_VOLUME_CONTROLLER
/**
 * @brief Set vcs volume
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "gvcsvolume",
        "gvcsvolume [group_idx] [volume_setting]\n\r",
        "Set vcs volume\n\r",
        cmd_gvcsvolume
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_gvcsvolume(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_LE_AUDIO_CAUSE cause = LE_AUDIO_CAUSE_INVALID_PARAM;
#if CSIP_SET_COORDINATOR
    uint8_t group_idx = p_parse_value->dw_param[0];
    uint8_t volume_setting = p_parse_value->dw_param[1];

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            cause = cap_vcs_change_volume(p_group->group_handle, volume_setting);
        }
    }
#endif
    if (cause == LE_AUDIO_CAUSE_SUCCESS)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

/**
 * @brief Set vcs mute
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "gvcsmute",
        "gvcsmute [group_idx] [mute]\n\r",
        "Set vcs mute\n\r",
        cmd_gvcsmute
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_gvcsmute(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_LE_AUDIO_CAUSE cause = LE_AUDIO_CAUSE_INVALID_PARAM;
#if CSIP_SET_COORDINATOR
    uint8_t group_idx = p_parse_value->dw_param[0];
    uint8_t mute = p_parse_value->dw_param[1];

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            cause = cap_vcs_change_mute(p_group->group_handle, mute);
        }
    }
#endif
    if (cause == LE_AUDIO_CAUSE_SUCCESS)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}
#endif

#if MICP_MIC_CONTROLLER
/**
 * @brief Set mics mute
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "gmicmute",
        "gmicmute [group_idx] [mute]\n\r",
        "Set mics mute\n\r",
        cmd_gmicmute
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_gmicmute(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_LE_AUDIO_CAUSE cause = LE_AUDIO_CAUSE_INVALID_PARAM;
    uint8_t group_idx = p_parse_value->dw_param[0];
    T_MICS_MUTE mute = (T_MICS_MUTE)p_parse_value->dw_param[1];

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            cause = cap_mics_change_mute(p_group->group_handle, mute);
        }
    }
    if (cause == LE_AUDIO_CAUSE_SUCCESS)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}
#endif

#if BAP_UNICAST_CLIENT
/**
 * @brief Start media stream
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "startmedia",
        "startmedia [group_idx]\n\r",
        "Start media stream\n\r",
        cmd_startmedia
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_startmedia(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;;
    uint8_t group_idx = p_parse_value->dw_param[0];

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            ret = app_lea_ini_start_media(p_group->group_handle);
        }
    }
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

/**
 * @brief Start conversation stream
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "startconvo",
        "startconvo [group_idx]\n\r",
        "Start conversation stream\n\r",
        cmd_startconvo
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_startconvo(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;;
    uint8_t group_idx = p_parse_value->dw_param[0];

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            ret = app_lea_ini_start_conversation(p_group->group_handle);
        }
    }
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

/**
 * @brief Release stream
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "releasestream",
        "releasestream [group_idx]\n\r",
        "Release stream\n\r",
        cmd_releasestream
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_releasestream(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;;
    uint8_t group_idx = p_parse_value->dw_param[0];

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            ret = app_lea_ini_unicast_audio_release(p_group->group_handle);
        }
    }
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

/**
 * @brief Stop stream
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "stopstream",
        "stopstream [group_idx] [cis_timeout_ms]\n\r",
        "Stop stream\n\r",
        cmd_stopstream
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_stopstream(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;;
    uint8_t group_idx = p_parse_value->dw_param[0];
    uint32_t cis_timeout_ms = p_parse_value->dw_param[1];

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            ret = app_lea_ini_unicast_audio_stop(p_group->group_handle, cis_timeout_ms);
        }
    }
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}
#endif

#if BAP_BROADCAST_SOURCE
/**
 * @brief Broadcast Source initialization
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bsrcinit",
        "bsrcinit [config_idx]\n\r",
        "Broadcast Source initialization\n\r",
        cmd_bsrcinit
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bsrcinit(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t config_idx = p_parse_value->dw_param[0];
    T_CODEC_CFG_ITEM index = CODEC_CFG_ITEM_16_2;
    T_QOS_CFG_TYPE qos_type = QOS_CFG_BIS_HIG_RELIABILITY;
    uint8_t bis_num = 1;
    T_GAP_LOCAL_ADDR_TYPE local_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    bool encryption = false;

    if (config_idx == 0)
    {
        bis_num = 1;
    }
    else if (config_idx == 1)
    {
        bis_num = 2;
    }
    else if (config_idx == 2)
    {
        bis_num = 1;
        encryption = true;
    }
    app_lea_bsrc_init(index, qos_type, bis_num, local_addr_type, encryption);
    return (RESULT_SUCESS);
}

/**
 * @brief Broadcast Audio Stop procedure
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bsrcstop",
        "bsrcstop [release]\n\r",
        "Broadcast Audio Stop procedure\n\r",
        cmd_bsrcstop
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bsrcstop(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    bool release = p_parse_value->dw_param[0];

    ret = app_lea_bsrc_stop(release);

    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

/**
 * @brief Broadcast Audio Start procedure
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bsrcstart",
        "bsrcstart\n\r",
        "Broadcast Audio Start procedure\n\r",
        cmd_bsrcstart
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bsrcstart(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;

    ret = app_lea_bsrc_start();

    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}
#endif

#if (BAP_BROADCAST_ASSISTANT && BAP_BROADCAST_SOURCE)
/**
 * @brief Broadcast Audio Reception Start procedure
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bststart",
        "bststart [group_idx]\n\r",
        "Broadcast Audio Reception Start\n\r",
        cmd_bststart
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bststart(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_APP_LEA_GROUP_INFO *p_group = NULL;
    uint8_t group_idx = p_parse_value->dw_param[0];

    if (group_idx < app_db.group_handle_queue.count)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                        group_idx);
        if (p_group != NULL)
        {
            app_lea_ini_bst_reception_start(p_group->group_handle, app_db.bsrc_db.source_handle);
            ret = true;
        }
    }

    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

/**
 * @brief Broadcast Audio Reception Stop procedure
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bststop",
        "bststop [group_idx]\n\r",
        "Broadcast Audio Reception Stop\n\r",
        cmd_bststop
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bststop(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_APP_LEA_GROUP_INFO *p_group = NULL;
    uint8_t group_idx = p_parse_value->dw_param[0];

    if (group_idx < app_db.group_handle_queue.count)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                        group_idx);
        if (p_group != NULL)
        {
            app_lea_ini_bst_reception_stop(p_group->group_handle, app_db.bsrc_db.source_handle);
            ret = true;
        }
    }

    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

/**
 * @brief Broadcast Audio Reception Remove procedure
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bstrm",
        "bstrm [group_idx]\n\r",
        "Broadcast Audio Reception Remove\n\r",
        cmd_bstrm
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bstrm(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_APP_LEA_GROUP_INFO *p_group = NULL;
    uint8_t group_idx = p_parse_value->dw_param[0];

    if (group_idx < app_db.group_handle_queue.count)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                        group_idx);
        if (p_group != NULL)
        {
            app_lea_ini_bst_reception_remove(p_group->group_handle, app_db.bsrc_db.source_handle);
            ret = true;
        }
    }

    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}
#endif

#if MCP_MEDIA_CONTROL_SERVER
/**
 * @brief MCP State Update
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "mcpstate",
        "mcpstate [state]\n\r",
        "MCP State Update\n\r",
        cmd_mcpstate
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_mcpstate(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_lea_ini_mcp_update_media_state((T_MCS_MEDIA_STATE)p_parse_value->dw_param[0]);

    return (RESULT_SUCESS);
}

/**
 * @brief MCP Notification
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "mcpnotify",
        "mcpnotify [char_uuid] [param]\n\r",
        "MCP Notification\n\r",
        cmd_mcpnotify
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_mcpnotify(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_MCP_SERVER_SEND_DATA_PARAM data_param = {0};

    if (p_parse_value->dw_param[0] == MCS_UUID_CHAR_TRACK_CHANGED)
    {
        data_param.char_uuid = MCS_UUID_CHAR_TRACK_CHANGED;
    }
    else if (p_parse_value->dw_param[0] == MCS_UUID_CHAR_TRACK_DURATION)
    {
        data_param.char_uuid = MCS_UUID_CHAR_TRACK_DURATION;
        data_param.param.track_duration = p_parse_value->dw_param[1];
        app_db.mcp_db.track_duration = p_parse_value->dw_param[1];
    }
    else if (p_parse_value->dw_param[0] == MCS_UUID_CHAR_TRACK_POSITION)
    {
        data_param.char_uuid = MCS_UUID_CHAR_TRACK_POSITION;
        data_param.param.track_position = p_parse_value->dw_param[1];
        app_db.mcp_db.track_position = p_parse_value->dw_param[1];
    }
    else
    {
        return (RESULT_ERR);
    }

    ret = mcp_server_send_data(app_db.mcp_db.gmcs_id, &data_param);
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}
#endif

#if CCP_CALL_CONTROL_SERVER
/**
 * @brief CCP Control Action
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "ccpaction",
        "ccpaction [msg] [call_index]\n\r",
        "CCP Control Action\n\r",
        cmd_ccpaction
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_ccpaction(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_APP_TBS_OUTPUT_MSG msg = (T_APP_TBS_OUTPUT_MSG)p_parse_value->dw_param[0];
    T_APP_TBS_OUTPUT_MSG_DATA msg_data = {0};
    uint8_t p_incoming_call_uri[] = "Incoming call";
    uint8_t p_outgoing_call_uri[] = "Outgoing call";

    if (msg == APP_TBS_OUTPUT_MSG_REMOTE_INCOMING_CALL)
    {
        msg_data.remote_incoming_call.call_uri_len =  strlen("Incoming call");
        msg_data.remote_incoming_call.p_call_uri =  p_incoming_call_uri;
    }
    else if (msg == APP_TBS_OUTPUT_MSG_LOCAL_ORIGINATE)
    {
        msg_data.local_originate.call_uri_len =  strlen("Outgoing call");
        msg_data.local_originate.p_call_uri =  p_outgoing_call_uri;
    }
    else if (msg == APP_TBS_OUTPUT_MSG_REMOTE_TERMINATE)
    {
        msg_data.remote_terminate.call_index =  p_parse_value->dw_param[1];
        msg_data.remote_terminate.reason_code = TBS_TERMINATION_REASON_CODES_REMOTE_END_CALL;
    }
    else if (msg == APP_TBS_OUTPUT_MSG_LOCAL_TERMINATE)
    {
        msg_data.local_terminate.call_index =  p_parse_value->dw_param[1];
        msg_data.local_terminate.reason_code = TBS_TERMINATION_REASON_CODES_SERVER_END_CALL;
    }
    else
    {
        msg_data.call_index =  p_parse_value->dw_param[1];
    }

    app_lea_ini_ccp_control(msg, &msg_data);

    return (RESULT_SUCESS);
}
#endif

/** @brief  User command table */
const T_USER_CMD_TABLE_ENTRY user_cmd_table[] =
{
    /************************** Common cmd *************************************/
    {
        "conupdreq",
        "conupdreq [conn_id] [interval_min] [interval_max] [latency] [supervision_timeout]\n\r",
        "LE connection param update request\r\n\
        sample: conupdreq 0 0x30 0x40 0 500\n\r",
        cmd_conupdreq
    },
    {
        "disc",
        "disc [conn_id]\n\r",
        "Disconnect to remote device\n\r",
        cmd_disc
    },
    {
        "condev",
        "condev [dev_idx]\n\r",
        "Connect to remote device: use showdev to show idx\r\n\
        [dev_idx]: use cmd showdev to show idx before use this cmd\r\n\
        sample: condev 0\n\r",
        cmd_condev
    },
    {
        "bondclear",
        "bondclear\n\r",
        "Clear all bonded devices information\n\r",
        cmd_bondclear
    },
    {
        "leascan",
        "leascan \n\r",
        "Start extended scan\n\r",
        cmd_leascan
    },
    {
        "leastopscan",
        "leastopscan\n\r",
        "Stop extended scan\n\r",
        cmd_leastopscan
    },
    {
        "gscan",
        "gscan [group_idx] \n\r",
        "Start group scan\n\r",
        cmd_gscan
    },
    {
        "stopgscan",
        "stopgscan\n\r",
        "Stop group scan\n\r",
        cmd_stopgscan
    },
    {
        "showdev",
        "showdev\n\r",
        "Show scan dev list\n\r",
        cmd_showdev
    },
    {
        "showgroup",
        "showgroup\n\r",
        "Show group list\n\r",
        cmd_showgroup
    },
#if VCP_VOLUME_CONTROLLER
    {
        "gvcsvolume",
        "gvcsvolume [group_idx] [volume_setting]\n\r",
        "Set vcs volume\n\r",
        cmd_gvcsvolume
    },
    {
        "gvcsmute",
        "gvcsmute [group_idx] [mute]\n\r",
        "Set vcs mute\n\r",
        cmd_gvcsmute
    },
#endif
#if MICP_MIC_CONTROLLER
    {
        "gmicmute",
        "gmicmute [group_idx] [mute]\n\r",
        "Set mics mute\n\r",
        cmd_gmicmute
    },
#endif
#if BAP_UNICAST_CLIENT
    {
        "startmedia",
        "startmedia [group_idx]\n\r",
        "Start media stream\n\r",
        cmd_startmedia
    },
    {
        "startconvo",
        "startconvo [group_idx]\n\r",
        "Start conversation stream\n\r",
        cmd_startconvo
    },
    {
        "releasestream",
        "releasestream [group_idx]\n\r",
        "Release stream\n\r",
        cmd_releasestream
    },
    {
        "stopstream",
        "stopstream [group_idx] [cis_timeout_ms]\n\r",
        "Stop stream\n\r",
        cmd_stopstream
    },
#endif
#if BAP_BROADCAST_SOURCE
    {
        "bsrcinit",
        "bsrcinit [config_idx]\n\r",
        "Broadcast Source initialization\n\r",
        cmd_bsrcinit
    },
    {
        "bsrcstart",
        "bsrcstart\n\r",
        "Broadcast Audio Start procedure\n\r",
        cmd_bsrcstart
    },
    {
        "bsrcstop",
        "bsrcstop [release]\n\r",
        "Broadcast Audio Stop procedure\n\r",
        cmd_bsrcstop
    },
#endif

#if (BAP_BROADCAST_ASSISTANT && BAP_BROADCAST_SOURCE)
    {
        "bststart",
        "bststart [group_idx]\n\r",
        "Broadcast Audio Reception Start\n\r",
        cmd_bststart
    },
    {
        "bststop",
        "bststop [group_idx]\n\r",
        "Broadcast Audio Reception Stop\n\r",
        cmd_bststop
    },
    {
        "bstrm",
        "bstrm [group_idx]\n\r",
        "Broadcast Audio Reception Remove\n\r",
        cmd_bstrm
    },
#endif
#if MCP_MEDIA_CONTROL_SERVER
    {
        "mcpstate",
        "mcpstate [state]\n\r",
        "MCP State Update\n\r",
        cmd_mcpstate
    },
    {
        "mcpnotify",
        "mcpnotify [char_uuid] [param]\n\r",
        "MCP Notification\n\r",
        cmd_mcpnotify
    },
#endif
#if CCP_CALL_CONTROL_SERVER
    {
        "ccpaction",
        "ccpaction [msg] [call_index]\n\r",
        "CCP Control Action\n\r",
        cmd_ccpaction
    },
#endif
    /* MUST be at the end: */
    {
        0,
        0,
        0,
        0
    }
};
/** @} */ /* End of group APP_LEA_CAP_INI_CMD */


