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
#include "app_lea_cap_com_user_cmd.h"
#include "app_lea_cap_com_link.h"
#include "app_lea_cap_com_scan.h"
#include "app_lea_cap_com_main.h"
#include "app_lea_cap_com_gap.h"
#include "app_lea_cap_com_cap.h"
#include "app_lea_cap_com_bap.h"
#include "ble_conn.h"
#include "cap.h"
#include "vcs_client.h"
#include "mics_client.h"
#include "aics_client.h"
#include "vocs_client.h"

/** @defgroup  APP_LEA_CAP_COM_CMD CAP Commander User Command
    * @brief This file handles CAP Commander User Command.
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
    T_GAP_CAUSE cause = GAP_CAUSE_SUCCESS;

    app_lea_scan_start();

    return (T_USER_CMD_PARSE_RESULT)cause;
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
    T_GAP_CAUSE cause = GAP_CAUSE_SUCCESS;
    app_lea_scan_stop();
    return (T_USER_CMD_PARSE_RESULT)cause;
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

#if BAP_BROADCAST_ASSISTANT
/**
 * @brief Start extended scan for BAAS
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "baasscan",
        "baasscan \n\r",
        "Start extended scan for BAAS\n\r",
        cmd_baasscan
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_baasscan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause = GAP_CAUSE_SUCCESS;

    app_lea_baas_scan_start();

    return (T_USER_CMD_PARSE_RESULT)cause;
}

/**
 * @brief Stop extended scan for BAAS
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "baasstopscan",
        "baasstopscan\n\r",
        "Stop extended scan for BAAS\n\r",
        cmd_baasstopscan
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_baasstopscan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause = GAP_CAUSE_SUCCESS;
    app_lea_baas_scan_stop();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

/**
 * @brief Show scan BAAS dev list
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "showbaasdev",
        "showbaasdev\n\r",
        "Show scan BAAS dev list\n\r",
        cmd_showbaasdev
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_showbaasdev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_APP_LEA_BAAS_SCAN_DEV *p_dev;
    data_uart_print("BAAS dev list num: %d\r\n", app_db.scan_baas_dev_queue.count);
    for (uint8_t i = 0; i < app_db.scan_baas_dev_queue.count; i++)
    {
        p_dev = (T_APP_LEA_BAAS_SCAN_DEV *)os_queue_peek(&app_db.scan_baas_dev_queue, i);
        if (p_dev != NULL)
        {
            data_uart_print("RemoteBd[%d] = [%02x:%02x:%02x:%02x:%02x:%02x], type %d, broadcast_id [%02x:%02x:%02x]\r\n",
                            i,
                            p_dev->dev_info.bd_addr[5], p_dev->dev_info.bd_addr[4],
                            p_dev->dev_info.bd_addr[3], p_dev->dev_info.bd_addr[2],
                            p_dev->dev_info.bd_addr[1], p_dev->dev_info.bd_addr[0], p_dev->dev_info.bd_type,
                            p_dev->dev_info.broadcast_id[0],
                            p_dev->dev_info.broadcast_id[1],
                            p_dev->dev_info.broadcast_id[2]);
        }
    }
    return (RESULT_SUCESS);
}

/**
 * @brief Show sync handle list
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "showsync",
        "showsync\n\r",
        "Show sync handle list\n\r",
        cmd_showsync
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_showsync(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_APP_LEA_SYNC_INFO *p_sync_info;

    data_uart_print("Sync handle list num: %d\r\n", app_db.sync_handle_queue.count);
    for (uint8_t i = 0; i < app_db.sync_handle_queue.count; i++)
    {
        p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue, i);
        if (p_sync_info != NULL)
        {
            T_BLE_AUDIO_SYNC_INFO sync_info;
            data_uart_print("Sync handle[%d]: sync_handle 0x%x, pa_state %d\r\n",
                            i, p_sync_info->sync_handle, p_sync_info->pa_state);

            if (ble_audio_sync_get_info(p_sync_info->sync_handle, &sync_info))
            {
                data_uart_print("advertiser_address = [%02x:%02x:%02x:%02x:%02x:%02x], type %d, broadcast_id [%02x:%02x:%02x]\r\n",
                                sync_info.advertiser_address[5], sync_info.advertiser_address[4],
                                sync_info.advertiser_address[3], sync_info.advertiser_address[2],
                                sync_info.advertiser_address[1], sync_info.advertiser_address[0],
                                sync_info.advertiser_address_type,
                                sync_info.broadcast_id[0],
                                sync_info.broadcast_id[1],
                                sync_info.broadcast_id[2]);
            }
        }
    }
    return (RESULT_SUCESS);
}

/**
 * @brief Remove Sync Handle
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "rmsync",
        "rmsync [sync_idx]\n\r",
        "Remove Sync Handle\n\r",
        cmd_rmsync
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_rmsync(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    uint8_t sync_idx = p_parse_value->dw_param[0];

    if (sync_idx < app_db.sync_handle_queue.count)
    {
        T_APP_LEA_SYNC_INFO *p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                                                sync_idx);
        if (p_sync_info)
        {
            app_lea_remove_sync(p_sync_info->sync_handle);
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
 * @brief PA Sync by device idx
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "pasyncdev",
        "pasyncdev [dev_idx]\n\r",
        "PA Sync by device idx\n\r",
        cmd_pasyncdev
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_pasyncdev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    uint8_t dev_idx = p_parse_value->dw_param[0];

    if (dev_idx < app_db.scan_baas_dev_queue.count)
    {
        T_APP_LEA_BAAS_SCAN_DEV *p_dev = (T_APP_LEA_BAAS_SCAN_DEV *)os_queue_peek(
                                             &app_db.scan_baas_dev_queue,
                                             dev_idx);
        if (p_dev)
        {
            ret = app_lea_pa_sync_by_dev_info(&p_dev->dev_info);
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
 * @brief PA Sync
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "pasync",
        "pasync [sync_idx]\n\r",
        "PA Sync\n\r",
        cmd_pasync
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_pasync(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    uint8_t sync_idx = p_parse_value->dw_param[0];

    if (sync_idx < app_db.sync_handle_queue.count)
    {
        T_APP_LEA_SYNC_INFO *p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                                                sync_idx);
        if (p_sync_info)
        {
            ret = app_lea_pa_sync(p_sync_info);
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
 * @brief PA Terminate
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "patermin",
        "patermin [sync_idx]\n\r",
        "PA Terminate\n\r",
        cmd_patermin
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_patermin(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    uint8_t sync_idx = p_parse_value->dw_param[0];

    if (sync_idx < app_db.sync_handle_queue.count)
    {
        T_APP_LEA_SYNC_INFO *p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                                                sync_idx);
        if (p_sync_info)
        {
            ret = app_lea_pa_terminate(p_sync_info);
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
 * @brief Broadcast Audio Reception Start procedure
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bststart",
        "bststart [group_idx] [sync_idx]\n\r",
        "Broadcast Audio Reception Start\n\r",
        cmd_bststart
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bststart(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_APP_LEA_GROUP_INFO *p_group = NULL;
    T_APP_LEA_SYNC_INFO *p_sync_info = NULL;
    uint8_t group_idx = p_parse_value->dw_param[0];
    uint8_t sync_idx = p_parse_value->dw_param[1];

    if (group_idx < app_db.group_handle_queue.count)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                        group_idx);
    }

    if (sync_idx < app_db.sync_handle_queue.count)
    {
        p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                           sync_idx);
    }
    if (p_group != NULL && p_sync_info != NULL)
    {
        app_lea_com_bst_reception_start(p_group->group_handle, p_sync_info->sync_handle);
        ret = true;
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
        "bststop [group_idx] [sync_idx]\n\r",
        "Broadcast Audio Reception Stop\n\r",
        cmd_bststop
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bststop(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_APP_LEA_GROUP_INFO *p_group = NULL;
    T_APP_LEA_SYNC_INFO *p_sync_info = NULL;
    uint8_t group_idx = p_parse_value->dw_param[0];
    uint8_t sync_idx = p_parse_value->dw_param[1];

    if (group_idx < app_db.group_handle_queue.count)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                        group_idx);
    }

    if (sync_idx < app_db.sync_handle_queue.count)
    {
        p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                           sync_idx);
    }
    if (p_group != NULL && p_sync_info != NULL)
    {
        app_lea_com_bst_reception_stop(p_group->group_handle, p_sync_info->sync_handle);
        ret = true;
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
        "bstrm [group_idx] [sync_idx]\n\r",
        "Broadcast Audio Reception Remove\n\r",
        cmd_bstrm
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bstrm(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_APP_LEA_GROUP_INFO *p_group = NULL;
    T_APP_LEA_SYNC_INFO *p_sync_info = NULL;
    uint8_t group_idx = p_parse_value->dw_param[0];
    uint8_t sync_idx = p_parse_value->dw_param[1];

    if (group_idx < app_db.group_handle_queue.count)
    {
        p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                        group_idx);
    }

    if (sync_idx < app_db.sync_handle_queue.count)
    {
        p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                           sync_idx);
    }
    if (p_group != NULL && p_sync_info != NULL)
    {
        app_lea_com_bst_reception_remove(p_group->group_handle, p_sync_info->sync_handle);
        ret = true;
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
 * @brief Set vcs control point
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "gvcscp",
        "gvcscp [group_idx] [op]\n\r",
        "Set vcs control point\n\r",
        cmd_gvcscp
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_gvcscp(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_LE_AUDIO_CAUSE cause = LE_AUDIO_CAUSE_INVALID_PARAM;
    uint8_t group_idx = p_parse_value->dw_param[0];
    T_VCS_CP_OP op = (T_VCS_CP_OP)p_parse_value->dw_param[1];

    if (op == VCS_CP_SET_ABSOLUTE_VOLUME ||
        op == VCS_CP_UNMUTE ||
        op == VCS_CP_MUTE)
    {
        return (RESULT_ERR);
    }

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            cause = vcs_write_cp_by_group(p_group->group_handle, op, NULL);
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
#if CSIP_SET_COORDINATOR
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
 * @brief Get mics mute
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "micinfo",
        "micinfo [conn_id]\n\r",
        "Get mics mute\n\r",
        cmd_micinfo
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_micinfo(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_MICS_MUTE mics_mute;
    uint8_t conn_id = p_parse_value->dw_param[0];

    if (mics_get_mute_value(le_get_conn_handle(conn_id), &mics_mute))
    {
        data_uart_print("Get MIC Info: mics_mute %d\r\n",
                        mics_mute);
    }
    return (RESULT_SUCESS);
}
#endif

#if APP_LEA_VOCS_CLIENT_SUPPORT
/**
 * @brief Read VOCS characteristic value
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "vocsread",
        "vocsread [conn_id] [srv_instance_id] [type]\n\r",
        "Read VOCS characteristic value\n\r",
        cmd_vocsread
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_vocsread(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];
    uint8_t type = p_parse_value->dw_param[2];

    if (vocs_read_char_value(le_get_conn_handle(conn_id), srv_instance_id, (T_VOCS_CHAR_TYPE)type))
    {
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
}

/**
 * @brief Write the Audio Output Description
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "vocsoutput",
        "vocsoutput [conn_id] [srv_instance_id]\n\r",
        "Write the Audio Output Description\n\r",
        cmd_vocsoutput
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_vocsoutput(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];
    char output_des[] = "Test output des";

    if (vocs_write_output_des(le_get_conn_handle(conn_id), srv_instance_id,
                              (uint8_t *)output_des, sizeof(output_des)))
    {
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
}

/**
 * @brief Write audio location
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "vocslocation",
        "vocslocation [conn_id] [srv_instance_id]\n\r",
        "Write audio location\n\r",
        cmd_vocslocation
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_vocslocation(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];
    uint32_t audio_location = 0;

    if (vocs_write_audio_location(le_get_conn_handle(conn_id), srv_instance_id,  audio_location))
    {
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
}

/**
 * @brief Write volume offset control point
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "vocscp",
        "vocscp [conn_id] [srv_instance_id] [volume_offset]\n\r",
        "Write volume offset control point\n\r",
        cmd_vocscp
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_vocscp(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];
    T_VOCS_CP_PARAM param = {0};

    param.volume_offset = p_parse_value->dw_param[2];
    if (vocs_write_cp(le_get_conn_handle(conn_id), srv_instance_id,  VOCS_CP_SET_VOLUME_OFFSET, &param))
    {
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
}

/**
 * @brief Write volume offset control point by group
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "gvocscp",
        "gvocscp [group_idx] [srv_instance_id] [volume_offset]\n\r",
        "Write volume offset control point by group\n\r",
        cmd_gvocscp
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_gvocscp(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_LE_AUDIO_CAUSE cause = LE_AUDIO_CAUSE_INVALID_PARAM;
    uint8_t group_idx = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];
    T_VOCS_CP_PARAM param = {0};

    param.volume_offset = p_parse_value->dw_param[2];
    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            cause = vocs_write_cp_by_group(p_group->group_handle, srv_instance_id, VOCS_CP_SET_VOLUME_OFFSET,
                                           &param);
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

/**
 * @brief Get VOCS information
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "vocsinfo",
        "vocsinfo [conn_id] [srv_instance_id]\n\r",
        "Get VOCS information\n\r",
        cmd_vocsinfo
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_vocsinfo(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_VOCS_SRV_DATA srv_data;
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];

    if (vocs_get_srv_data(le_get_conn_handle(conn_id), srv_instance_id, &srv_data))
    {
        data_uart_print("Get VOCS Info: srv_instance_id %d, type_exist 0x%x\r\n",
                        srv_data.srv_instance_id, srv_data.type_exist);

        if (srv_data.type_exist & VOCS_VOLUME_OFFSET_STATE_FLAG)
        {
            data_uart_print("VOCS_CHAR_OFFSET_STATE: volume_offset %d, change_counter 0x%x\r\n",
                            srv_data.volume_offset.volume_offset,
                            srv_data.volume_offset.change_counter);
        }

        if (srv_data.type_exist & VOCS_AUDIO_LOCATION_FLAG)
        {
            data_uart_print("VOCS_CHAR_AUDIO_LOCATION: audio_location 0x%x\r\n",
                            srv_data.audio_location);
        }

        if (srv_data.type_exist & VOCS_AUDIO_OUTPUT_DES_FLAG)
        {
            data_uart_print("VOCS_CHAR_AUDIO_OUTPUT_DESC: output des[%d]\r\n",
                            srv_data.output_des.output_des_len);
        }
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
}
#endif

#if APP_LEA_AICS_CLIENT_SUPPORT
/**
 * @brief Read AICS characteristic value
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "aicsread",
        "aicsread [conn_id] [srv_instance_id] [type]\n\r",
        "Read AICS characteristic value\n\r",
        cmd_aicsread
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_aicsread(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];
    uint8_t type = p_parse_value->dw_param[2];

    if (aics_read_char_value(le_get_conn_handle(conn_id), srv_instance_id, (T_AICS_CHAR_TYPE)type))
    {
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
}

/**
 * @brief Write the Audio Input Description
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "aicsinput",
        "aicsinput [conn_id] [srv_instance_id]\n\r",
        "Write the Audio Input Description\n\r",
        cmd_aicsinput
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_aicsinput(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];
    char input_des[] = "Test input des";

    if (aics_write_input_des(le_get_conn_handle(conn_id), srv_instance_id,
                             (uint8_t *)input_des, sizeof(input_des)))
    {
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
}

/**
 * @brief Write audio input control point
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "aicscp",
        "aicscp [conn_id] [srv_instance_id] [op]\n\r",
        "Write audio input control point\n\r",
        cmd_aicscp
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_aicscp(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];
    T_AICS_CP_OP op = (T_AICS_CP_OP)p_parse_value->dw_param[2];
    T_AICS_CP_PARAM param = {0};

    if (op == AICS_CP_SET_GAIN_SETTING)
    {
        param.gaining_setting = p_parse_value->dw_param[3];
    }
    if (aics_write_cp(le_get_conn_handle(conn_id), srv_instance_id,  op, &param))
    {
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
}

/**
 * @brief Write audio input control point by group
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "gaicscp",
        "gaicscp [group_idx] [srv_instance_id] [op]\n\r",
        "Write audio input control point by group\n\r",
        cmd_gaicscp
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_gaicscp(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_LE_AUDIO_CAUSE cause = LE_AUDIO_CAUSE_INVALID_PARAM;
    uint8_t group_idx = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];
    T_AICS_CP_OP op = (T_AICS_CP_OP)p_parse_value->dw_param[2];
    T_AICS_CP_PARAM param = {0};

    if (op == AICS_CP_SET_GAIN_SETTING)
    {
        param.gaining_setting = p_parse_value->dw_param[3];
    }

    if (group_idx < app_db.group_handle_queue.count)
    {
        T_APP_LEA_GROUP_INFO *p_group = (T_APP_LEA_GROUP_INFO *)os_queue_peek(&app_db.group_handle_queue,
                                                                              group_idx);
        if (p_group)
        {
            cause = aics_write_cp_by_group(p_group->group_handle, srv_instance_id, op, &param);
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

/**
 * @brief Get AICS information
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "aicsinfo",
        "aicsinfo [conn_id] [srv_instance_id]\n\r",
        "Get AICS information\n\r",
        cmd_aicsinfo
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_aicsinfo(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_AICS_SRV_DATA srv_data;
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t srv_instance_id = p_parse_value->dw_param[1];

    if (aics_get_srv_data(le_get_conn_handle(conn_id), srv_instance_id, &srv_data))
    {
        data_uart_print("Get AICS Info: srv_instance_id %d, type_exist 0x%x\r\n",
                        srv_data.srv_instance_id, srv_data.type_exist);
        if (srv_data.type_exist & AICS_INPUT_STATE_FLAG)
        {
            data_uart_print("AICS_CHAR_INPUT_STATE: gain_setting %d, mute %d, gain_mode %d, change_counter %d\r\n",
                            srv_data.input_state.gain_setting,
                            srv_data.input_state.mute,
                            srv_data.input_state.gain_mode,
                            srv_data.input_state.change_counter);
        }

        if (srv_data.type_exist & AICS_INPUT_STATUS_FLAG)
        {
            data_uart_print("AICS_CHAR_INPUT_STATUS: input_status 0x%x\r\n",
                            srv_data.input_status);
        }

        if (srv_data.type_exist & AICS_GAIN_SETTING_PROP_FLAG)
        {
            data_uart_print("AICS_CHAR_GAIN_SETTING_PROP: gain_setting_units %d, gain_setting_min %d, gain_setting_max %d\r\n",
                            srv_data.setting_prop.gain_setting_units,
                            srv_data.setting_prop.gain_setting_min,
                            srv_data.setting_prop.gain_setting_max);
        }

        if (srv_data.type_exist & AICS_INPUT_TYPE_FLAG)
        {
            data_uart_print("AICS_UUID_CHAR_INPUT_TYPE: input_type 0x%x\r\n",
                            srv_data.input_type);
        }

        if (srv_data.type_exist & AICS_INPUT_DES_FLAG)
        {
            data_uart_print("AICS_CHAR_INPUT_DES: input des[%d] \r\n",
                            srv_data.input_des.input_des_len);
        }
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
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
        "gscan [group_idx]\n\r",
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
#if BAP_BROADCAST_ASSISTANT
    {
        "baasscan",
        "baasscan \n\r",
        "Start extended scan for BAAS\n\r",
        cmd_baasscan
    },
    {
        "baasstopscan",
        "baasstopscan\n\r",
        "Stop extended scan for BAAS\n\r",
        cmd_baasstopscan
    },
    {
        "showbaasdev",
        "showbaasdev\n\r",
        "Show scan BAAS dev list\n\r",
        cmd_showbaasdev
    },
    {
        "showsync",
        "showsync\n\r",
        "Show sync handle list\n\r",
        cmd_showsync
    },
    {
        "rmsync",
        "rmsync [sync_idx]\n\r",
        "Remove Sync Handle\n\r",
        cmd_rmsync
    },
    {
        "pasyncdev",
        "pasyncdev [dev_idx]\n\r",
        "PA Sync by device idx\n\r",
        cmd_pasyncdev
    },
    {
        "pasync",
        "pasync [sync_idx]\n\r",
        "PA Sync\n\r",
        cmd_pasync
    },
    {
        "patermin",
        "patermin [sync_idx]\n\r",
        "PA Terminate\n\r",
        cmd_patermin
    },
    {
        "bststart",
        "bststart [group_idx] [sync_idx]\n\r",
        "Broadcast Audio Reception Start\n\r",
        cmd_bststart
    },
    {
        "bststop",
        "bststop [group_idx] [sync_idx]\n\r",
        "Broadcast Audio Reception Stop\n\r",
        cmd_bststop
    },
    {
        "bstrm",
        "bstrm [group_idx] [sync_idx]\n\r",
        "Broadcast Audio Reception Remove\n\r",
        cmd_bstrm
    },
#endif
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
    {
        "gvcscp",
        "gvcscp [group_idx] [op]\n\r",
        "Set vcs control point\n\r",
        cmd_gvcscp
    },
#endif
#if MICP_MIC_CONTROLLER
    {
        "gmicmute",
        "gmicmute [group_idx] [mute]\n\r",
        "Set mics mute\n\r",
        cmd_gmicmute
    },
    {
        "micinfo",
        "micinfo [conn_id]\n\r",
        "Get mics mute\n\r",
        cmd_micinfo
    },
#endif
#if APP_LEA_VOCS_CLIENT_SUPPORT
    {
        "vocsread",
        "vocsread [conn_id] [srv_instance_id] [type]\n\r",
        "Read VOCS characteristic value\n\r",
        cmd_vocsread
    },
    {
        "vocsoutput",
        "vocsoutput [conn_id] [srv_instance_id]\n\r",
        "Write the Audio Output Description\n\r",
        cmd_vocsoutput
    },
    {
        "vocslocation",
        "vocslocation [conn_id] [srv_instance_id]\n\r",
        "Write audio location\n\r",
        cmd_vocslocation
    },
    {
        "vocscp",
        "vocscp [conn_id] [srv_instance_id] [volume_offset]\n\r",
        "Write volume offset control point\n\r",
        cmd_vocscp
    },
    {
        "gvocscp",
        "gvocscp [group_idx] [srv_instance_id] [volume_offset]\n\r",
        "Write volume offset control point by group\n\r",
        cmd_gvocscp
    },
    {
        "vocsinfo",
        "vocsinfo [conn_id] [srv_instance_id]\n\r",
        "Get VOCS information\n\r",
        cmd_vocsinfo
    },
#endif
#if APP_LEA_AICS_CLIENT_SUPPORT
    {
        "aicsread",
        "aicsread [conn_id] [srv_instance_id] [type]\n\r",
        "Read AICS characteristic value\n\r",
        cmd_aicsread
    },
    {
        "aicsinput",
        "aicsinput [conn_id] [srv_instance_id]\n\r",
        "Write the Audio Input Description\n\r",
        cmd_aicsinput
    },
    {
        "aicscp",
        "aicscp [conn_id] [srv_instance_id] [op]\n\r",
        "Write audio input control point\n\r",
        cmd_aicscp
    },
    {
        "gaicscp",
        "gaicscp [group_idx] [srv_instance_id] [op]\n\r",
        "Write audio input control point by group\n\r",
        cmd_gaicscp
    },
    {
        "aicsinfo",
        "aicsinfo [conn_id] [srv_instance_id]\n\r",
        "Get AICS information\n\r",
        cmd_aicsinfo
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
/** @} */ /* End of group APP_LEA_CAP_COM_CMD */


