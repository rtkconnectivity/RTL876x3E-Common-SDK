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
#include "gap_bond_le.h"
#include "gap_conn_le.h"
#include "app_lea_cap_acc_user_cmd.h"
#include "app_lea_cap_acc_bap.h"
#include "app_lea_cap_acc_main.h"
#include "app_lea_cap_acc_profile.h"
#include "app_lea_cap_acc_mcp.h"
#include "app_lea_cap_acc_ccp.h"
#include "ble_conn.h"
#include "vcs_mgr.h"
#include "vocs_mgr.h"
#include "mics_mgr.h"
#include "aics_mgr.h"
#include "mcp_client.h"
#include "ccp_client.h"

/** @defgroup  APP_LEA_CAP_ACC_CMD CAP Acceptor User Command
    * @brief This file handles CAP Acceptor User Command.
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
 * @brief Init stack, configuration parameters.
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "reg",
        "reg [csis_cfg]\n\r",
        "Init stack, configuration parameters\n\r",
        cmd_reg
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_reg(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_db.csis_cfg = (T_APP_LEA_CSIS_CFG)p_parse_value->dw_param[0];

    app_lea_profile_init();

    /* Register BT Stack. */
    if (bt_stack_start())
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

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

#if (BAP_BROADCAST_SINK || BAP_SCAN_DELEGATOR)
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
            data_uart_print("Sync handle[%d]: sync_handle 0x%x, pa_state %d, big_state %d\r\n",
                            i, p_sync_info->sync_handle, p_sync_info->pa_state,
                            p_sync_info->big_state);

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
 * @brief BIG Sync
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bigsync",
        "bigsync [sync_idx]\n\r",
        "BIG Sync\n\r",
        cmd_bigsync
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bigsync(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    uint8_t sync_idx = p_parse_value->dw_param[0];

    if (sync_idx < app_db.sync_handle_queue.count)
    {
        T_APP_LEA_SYNC_INFO *p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                                                sync_idx);
        if (p_sync_info)
        {
            app_lea_big_sync(p_sync_info);
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
 * @brief BIG Terminate
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bigtermin",
        "bigtermin [sync_idx]\n\r",
        "BIG Terminate\n\r",
        cmd_bigtermin
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bigtermin(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    uint8_t sync_idx = p_parse_value->dw_param[0];

    if (sync_idx < app_db.sync_handle_queue.count)
    {
        T_APP_LEA_SYNC_INFO *p_sync_info = (T_APP_LEA_SYNC_INFO *)os_queue_peek(&app_db.sync_handle_queue,
                                                                                sync_idx);
        if (p_sync_info)
        {
            ret = app_lea_big_terminate(p_sync_info);
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

#if VCP_VOLUME_RENDERER
/**
 * @brief Update VCS service parameters
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "vcsupdate",
        "vcsupdate [test_idx]\n\r",
        "Update VCS service parameters\n\r",
        cmd_vcsupdate
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_vcsupdate(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_VCS_PARAM vcs_param;
    uint8_t test_idx = p_parse_value->dw_param[0];

    if (vcs_get_param(&vcs_param))
    {
        if (test_idx == 0)
        {
            vcs_param.volume_setting = 10;
        }
        else if (test_idx == 1)
        {
            vcs_param.mute = 0;
        }
        else if (test_idx == 2)
        {
            vcs_param.mute = 1;
        }
        else if (test_idx == 3)
        {
            vcs_param.volume_flags = VCS_RESET_VOLUME_SETTING;
        }
        ret = vcs_set_param(&vcs_param);
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

#if APP_LEA_VOCS_SUPPORT
/**
 * @brief Update VOCS service parameters
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "vocsupdate",
        "vocsupdate [test_idx]\n\r",
        "Update VOCS service parameters\n\r",
        cmd_vocsupdate
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_vocsupdate(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_VOCS_PARAM_SET vocs_param = {0};
    uint8_t test_idx = p_parse_value->dw_param[0];

    if (test_idx == 0)
    {
        vocs_param.set_mask |= VOCS_VOLUME_OFFSET_STATE_FLAG;
        vocs_param.volume_offset = 20;
    }
    else if (test_idx == 1)
    {
        vocs_param.set_mask |= VOCS_AUDIO_LOCATION_FLAG;
        vocs_param.audio_location = AUDIO_LOCATION_FR;
    }
    else if (test_idx == 2)
    {
        char audio_output_des_test[] = "Test Speaker1";
        vocs_param.set_mask |= VOCS_AUDIO_OUTPUT_DES_FLAG;
        vocs_param.output_des.p_output_des = (uint8_t *)audio_output_des_test;
        vocs_param.output_des.output_des_len = sizeof(audio_output_des_test);
    }

    ret = vocs_set_param(0, &vocs_param);

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
#endif

#if MICP_MIC_DEVICE
/**
 * @brief Update MICS service parameters
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "micupdate",
        "micupdate [mute]\n\r",
        "Update MICS service parameters\n\r",
        cmd_micupdate
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_micupdate(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    T_MICS_PARAM mics_param;

    mics_param.mic_mute = (T_MICS_MUTE)p_parse_value->dw_param[0];

    ret = mics_set_param(&mics_param);

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

#if (APP_LEA_AICS_FOR_VCS_SUPPORT ||APP_LEA_AICS_FOR_MICS_SUPPORT)
/**
 * @brief Update AICS service parameters
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "aicsupdate",
        "aicsupdate [srv_instance_id] [test_idx]\n\r",
        "Update AICS service parameters\n\r",
        cmd_aicsupdate
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_aicsupdate(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    uint8_t srv_instance_id = p_parse_value->dw_param[0];
    uint8_t test_idx = p_parse_value->dw_param[1];

    if (test_idx == 0)
    {
        T_AICS_INPUT_STATE input_state;
        input_state.gain_mode = AICS_GAIN_MODE_MANUAL;
        input_state.gain_setting = 10;
        input_state.mute = AICS_MUTED;
        ret = aics_set_param(srv_instance_id, AICS_PARAM_INPUT_STATE, sizeof(input_state),
                             (uint8_t *)&input_state, false);
    }
    else if (test_idx == 1)
    {
        uint8_t input_status = AICS_INPUT_STATUS_INACTIVE;
        ret = aics_set_param(srv_instance_id, AICS_PARAM_INPUT_STATUS, sizeof(input_status), &input_status,
                             false);
    }
    else if (test_idx == 2)
    {
        char audio_input_des_test[] = "Test Speaker1";
        ret = aics_set_param(srv_instance_id, AICS_PARAM_INPUT_DES, sizeof(audio_input_des_test),
                             (uint8_t *)audio_input_des_test, false);
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

#if MCP_MEDIA_CONTROL_CLIENT
/**
 * @brief MCP Control Point
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "mcpcp",
        "mcpcp [ccid] [opcode]\n\r",
        "MCP Control Point\n\r",
        cmd_mcpcp
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_mcpcp(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM param = {0};
    param.opcode = p_parse_value->dw_param[1];
    app_lea_acc_mcp_cp(p_parse_value->dw_param[0], &param);
    return (RESULT_SUCESS);
}

/**
 * @brief Read MCS characteristic
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "mcpread",
        "mcpread [conn_id] [gmcs] [srv_instance_id] [char_uuid]\n\r",
        "Read MCS characteristic\n\r",
        cmd_mcpread
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_mcpread(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t gmcs = p_parse_value->dw_param[1];
    uint8_t srv_instance_id = p_parse_value->dw_param[2];
    uint16_t char_uuid = p_parse_value->dw_param[3];

    if (mcp_client_read_char_value(le_get_conn_handle(conn_id), srv_instance_id, char_uuid,
                                   gmcs) == false)
    {
        return (RESULT_ERR);
    }
    return (RESULT_SUCESS);
}

/**
 * @brief Configure MCS CCCD
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "mcpcccd",
        "mcpcccd [conn_id] [gmcs] [srv_instance_id] [cfg_flags]\n\r",
        "Configure MCS CCCD\n\r",
        cmd_mcpcccd
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_mcpcccd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t gmcs = p_parse_value->dw_param[1];
    uint8_t srv_instance_id = p_parse_value->dw_param[2];
    uint32_t cfg_flags = p_parse_value->dw_param[3];

    mcp_client_cfg_cccd(le_get_conn_handle(conn_id), cfg_flags, gmcs, srv_instance_id, true);

    return (RESULT_SUCESS);
}
#endif

#if (CCP_CALL_CONTROL_CLIENT || MCP_MEDIA_CONTROL_CLIENT)
/**
 * @brief Show content control services infomation
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "showcontent",
        "showcontent\n\r",
        "Show content control services infomation\n\r",
        cmd_showcontent
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_showcontent(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_lea_context_print();

    return (RESULT_SUCESS);
}
#endif

#if CCP_CALL_CONTROL_CLIENT
/**
 * @brief CCP Control Action
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "ccpctrl",
        "ccpctrl [action] [conn_handle] [ccid] [call_index]\n\r",
        "CCP Control Action\n\r",
        cmd_ccpctrl
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_ccpctrl(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_APP_LEA_CCP_CALL_DATA param = {0};
    param.conn_handle = p_parse_value->dw_param[1];
    param.ccid = p_parse_value->dw_param[2];
    param.call_index = p_parse_value->dw_param[3];
    app_lea_acc_ccp_control((T_APP_LEA_CCP_CALL_ACTION)p_parse_value->dw_param[0], &param);
    return (RESULT_SUCESS);
}

/**
 * @brief Read CCP characteristic
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "ccpread",
        "ccpread [conn_id] [gtbs] [srv_instance_id] [char_uuid]\n\r",
        "Read CCP characteristic\n\r",
        cmd_ccpread
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_ccpread(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t gtbs = p_parse_value->dw_param[1];
    uint8_t srv_instance_id = p_parse_value->dw_param[2];
    uint16_t char_uuid = p_parse_value->dw_param[3];

    if (ccp_client_read_char_value(le_get_conn_handle(conn_id), srv_instance_id, char_uuid,
                                   gtbs) == false)
    {
        return (RESULT_ERR);
    }
    return (RESULT_SUCESS);
}

/**
 * @brief Configure CCP CCCD
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "ccpcccd",
        "ccpcccd [conn_id] [gtbs] [srv_instance_id] [cfg_flags]\n\r",
        "Configure CCP CCCD\n\r",
        cmd_ccpcccd
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_ccpcccd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t gtbs = p_parse_value->dw_param[1];
    uint8_t srv_instance_id = p_parse_value->dw_param[2];
    uint32_t cfg_flags = p_parse_value->dw_param[3];

    ccp_client_cfg_cccd(le_get_conn_handle(conn_id), cfg_flags, gtbs, srv_instance_id, true);

    return (RESULT_SUCESS);
}
#endif

/** @brief  User command table */
const T_USER_CMD_TABLE_ENTRY user_cmd_table[] =
{
    /************************** Common cmd *************************************/
    {
        "reg",
        "reg [csis_cfg]\n\r",
        "Init stack, reg test case\n\r",
        cmd_reg
    },
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
        "bondclear",
        "bondclear\n\r",
        "Clear all bonded devices information\n\r",
        cmd_bondclear
    },
#if (BAP_BROADCAST_SINK || BAP_SCAN_DELEGATOR)
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
        "showbaasdev",
        "showbaasdev\n\r",
        "Show scan BAAS dev list\n\r",
        cmd_showbaasdev
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
        "bigsync",
        "bigsync [sync_idx]\n\r",
        "BIG Sync\n\r",
        cmd_bigsync
    },
    {
        "bigtermin",
        "bigtermin [sync_idx]\n\r",
        "BIG Terminate\n\r",
        cmd_bigtermin
    },
#endif
#if VCP_VOLUME_RENDERER
    {
        "vcsupdate",
        "vcsupdate [test_idx]\n\r",
        "Update VCS service parameters\n\r",
        cmd_vcsupdate
    },
#endif
#if APP_LEA_VOCS_SUPPORT
    {
        "vocsupdate",
        "vocsupdate [test_idx]\n\r",
        "Update VOCS service parameters\n\r",
        cmd_vocsupdate
    },
#endif
#if MICP_MIC_DEVICE
    {
        "micupdate",
        "micupdate [mute]\n\r",
        "Update MICS service parameters\n\r",
        cmd_micupdate
    },
#endif
#if (APP_LEA_AICS_FOR_VCS_SUPPORT || APP_LEA_AICS_FOR_MICS_SUPPORT)
    {
        "aicsupdate",
        "aicsupdate [srv_instance_id] [test_idx]\n\r",
        "Update AICS service parameters\n\r",
        cmd_aicsupdate
    },
#endif
#if MCP_MEDIA_CONTROL_CLIENT
    {
        "mcpcp",
        "mcpcp [ccid] [opcode]\n\r",
        "MCP Control Point\n\r",
        cmd_mcpcp
    },
    {
        "mcpread",
        "mcpread [conn_id] [gmcs] [srv_instance_id] [char_uuid]\n\r",
        "Read MCS characteristic\n\r",
        cmd_mcpread
    },
    {
        "mcpcccd",
        "mcpcccd [conn_id] [gmcs] [srv_instance_id] [cfg_flags]\n\r",
        "Configure MCS CCCD\n\r",
        cmd_mcpcccd
    },
#endif
#if (CCP_CALL_CONTROL_CLIENT || MCP_MEDIA_CONTROL_CLIENT)
    {
        "showcontent",
        "showcontent\n\r",
        "Show content control services infomation\n\r",
        cmd_showcontent
    },
#endif
#if CCP_CALL_CONTROL_CLIENT
    {
        "ccpctrl",
        "ccpctrl [action] [conn_handle] [ccid] [call_index]\n\r",
        "CCP Control Action\n\r",
        cmd_ccpctrl
    },
    {
        "ccpread",
        "ccpread [conn_id] [gtbs] [srv_instance_id] [char_uuid]\n\r",
        "Read CCP characteristic\n\r",
        cmd_ccpread
    },
    {
        "ccpcccd",
        "ccpcccd [conn_id] [gtbs] [srv_instance_id] [cfg_flags]\n\r",
        "Configure CCP CCCD\n\r",
        cmd_ccpcccd
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
/** @} */ /* End of group APP_LEA_CAP_ACC_CMD */


