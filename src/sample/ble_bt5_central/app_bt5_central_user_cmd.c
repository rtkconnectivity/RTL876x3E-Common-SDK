/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "string.h"
#include "gap_ext_scan.h"
#include "app_bt5_central_scan.h"
#include "app_bt5_central_link_mgr.h"
#include "app_bt5_central_user_cmd.h"


/** @defgroup  BT5_CENTRAL_USER_CMD BT5 Central User Cmd
    * @brief This file handles BT5 Central User Command.
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
 * @brief Show all devices connecting status
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "showcon",
        "showcon\n\r",
        "Show all devices connecting status\n\r",
        cmd_showcon
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_showcon(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id;
    T_GAP_CONN_INFO conn_info;
    for (conn_id = 0; conn_id < APP_MAX_LINKS; conn_id++)
    {
        if (le_get_conn_info(conn_id, &conn_info))
        {
            data_uart_print("ShowCon conn_id %d state 0x%x role %d\r\n", conn_id,
                            conn_info.conn_state, conn_info.role);
            data_uart_print("RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d\r\n",
                            conn_info.remote_bd[5], conn_info.remote_bd[4],
                            conn_info.remote_bd[3], conn_info.remote_bd[2],
                            conn_info.remote_bd[1], conn_info.remote_bd[0],
                            conn_info.remote_bd_type);

            APP_PRINT_INFO3("ShowCon conn_id %d state 0x%x role %d\r\n",
                            conn_id, conn_info.conn_state, conn_info.role);
            APP_PRINT_INFO2("RemoteBd = [%b] type = %d\r\n",
                            TRACE_BDADDR(conn_info.remote_bd), conn_info.remote_bd_type);
        }
    }
    data_uart_print("active link num %d,  idle link num %d\r\n",
                    le_get_active_link_num(), le_get_idle_link_num());
    return (RESULT_SUCESS);
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

/************************** Central only *************************************/
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
    uint8_t i;
    data_uart_print("dev list\r\n");
    for (i = 0; i < dev_list_count; i++)
    {
        data_uart_print("RemoteBd[%d] = [%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
                        i,
                        dev_list[i].bd_addr[5], dev_list[i].bd_addr[4],
                        dev_list[i].bd_addr[3], dev_list[i].bd_addr[2],
                        dev_list[i].bd_addr[1], dev_list[i].bd_addr[0]);

        APP_PRINT_INFO2("RemoteBd[%d] = [%b]\r\n",
                        i, TRACE_BDADDR(dev_list[i].bd_addr));
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
        "condev [idx] [init_phys]\n\r",
        "Connect to remote device: use showdev to show idx\r\n\
        [idx]: use cmd showdev to show idx before use this cmd\r\n\
        [init_phys]:0x001(GAP_PHYS_CONN_INIT_1M_BIT), 0x011(GAP_PHYS_CONN_INIT_2M_BIT | GAP_PHYS_CONN_INIT_1M_BIT),
                    0x100(GAP_PHYS_CONN_INIT_CODED_BIT),0x101(GAP_PHYS_CONN_INIT_CODED_BIT | GAP_PHYS_CONN_INIT_1M_BIT),
                    0x110(GAP_PHYS_CONN_INIT_CODED_BIT | GAP_PHYS_CONN_INIT_2M_BIT),0x111(GAP_PHYS_CONN_INIT_CODED_BIT |GAP_PHYS_CONN_INIT_2M_BIT |GAP_PHYS_CONN_INIT_1M_BIT)
        sample: condev 0 0x001\n\r",
        cmd_condev
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_condev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t dev_idx = p_parse_value->dw_param[0];
    if (dev_idx < dev_list_count)
    {
        T_GAP_CAUSE cause;
        T_GAP_LE_CONN_REQ_PARAM conn_req_param;
        T_GAP_LOCAL_ADDR_TYPE local_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
        uint8_t  init_phys = GAP_PHYS_CONN_INIT_1M_BIT;
        uint32_t input_phys = 0x01;
        conn_req_param.scan_interval = 0x10;
        conn_req_param.scan_window = 0x10;
        conn_req_param.conn_interval_min = 80;
        conn_req_param.conn_interval_max = 80;
        conn_req_param.conn_latency = 0;
        conn_req_param.supv_tout = 1000;
        conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
        conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);

        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
        le_set_conn_param(GAP_CONN_PARAM_2M, &conn_req_param);
#if LE_CODED_PHY_SUPPORT
        le_set_conn_param(GAP_CONN_PARAM_CODED, &conn_req_param);
#endif
        if (p_parse_value->param_count > 1)
        {
            input_phys = p_parse_value->dw_param[1];
        }

        switch (input_phys)
        {
        case 0x001:
            init_phys = GAP_PHYS_CONN_INIT_1M_BIT;
            break;
        case 0x011:
            init_phys = GAP_PHYS_CONN_INIT_2M_BIT | GAP_PHYS_CONN_INIT_1M_BIT;
            break;
#if LE_CODED_PHY_SUPPORT
        case 0x100:
            init_phys = GAP_PHYS_CONN_INIT_CODED_BIT;
            break;
        case 0x101:
            init_phys = GAP_PHYS_CONN_INIT_CODED_BIT | GAP_PHYS_CONN_INIT_1M_BIT;
            break;
        case 0x110:
            init_phys = GAP_PHYS_CONN_INIT_CODED_BIT | GAP_PHYS_CONN_INIT_2M_BIT;
            break;
        case 0x111:
            init_phys = GAP_PHYS_CONN_INIT_CODED_BIT |
                        GAP_PHYS_CONN_INIT_2M_BIT |
                        GAP_PHYS_CONN_INIT_1M_BIT;
            break;
#endif
        default:
            break;
        }
        data_uart_print("init_phys %d\r\n", init_phys);
        cause = le_connect(init_phys, dev_list[dev_idx].bd_addr,
                           (T_GAP_REMOTE_ADDR_TYPE)dev_list[dev_idx].bd_type,
                           local_addr_type,
                           1000);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }
    else
    {
        return RESULT_ERR;
    }
}

/**
 * @brief Start extended scan
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "escan",
        "escan [scan_mode] [scan_phys]\n\r",
        "Start extended scan\r\n\
        [scan_mode]: 0-(continue scanning until scanning is disabled)\r\n\
                     1-(scan for the duration within a scan period, and scan periods continue until scanning is disabled)\r\n\
                     2-(continue scanning until duration has expired)\r\n\
        [scan_phys]: set scan PHYs to 1(LE 1M PHY), 4(LE Coded PHY) or 5(LE 1M PHY and LE Coded PHY)\r\n\
        sample: escan 0 1\n\r",
        cmd_escan
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_escan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause = GAP_CAUSE_SUCCESS;
    uint8_t scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT;

//    uint16_t ext_scan_duration = 0;
//    uint16_t ext_scan_period   = 0;
    uint16_t scan_interval     = 440;
    uint16_t scan_window       = 220;
    T_EXT_SCAN_MODE scan_mode  = SCAN_UNTIL_DISABLED;

    link_mgr_clear_device_list();
    if (p_parse_value->param_count > 0)
    {
        scan_mode = (T_EXT_SCAN_MODE)p_parse_value->dw_param[0];
    }
    if (p_parse_value->param_count > 1)
    {
        scan_phys = p_parse_value->dw_param[1];
    }

    if (scan_mode == SCAN_UNTIL_DISABLED)
    {
//        ext_scan_duration = 0;
//        ext_scan_period = 0;
        app_scan_start(scan_interval, scan_window, scan_phys);
    }
    else if (scan_mode == PERIOD_SCAN_UNTIL_DISABLED)
    {
//        ext_scan_duration = 500;
//        ext_scan_period = 8;
        app_scan_start(scan_interval, scan_window, scan_phys);
    }
    else if (scan_mode == SCAN_UNTIL_DURATION_EXPIRED)
    {
//        ext_scan_duration = 500;
//        ext_scan_period = 0;
        app_scan_start(scan_interval, scan_window, scan_phys);
    }

    return (T_USER_CMD_PARSE_RESULT)cause;
}

/**
 * @brief Stop extended scan
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "stopescan",
        "stopescan\n\r",
        "Stop extended scan\n\r",
        cmd_stopescan
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_stopescan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause = GAP_CAUSE_SUCCESS;
    app_scan_stop();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

#if APP_RECOMBINE_ADV_DATA
/**
 * @brief Recombine advertising data
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "radvdata",
        "radvdata\n\r",
        "Recombine advertising data from one device\n\r",
        cmd_radvdata
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_radvdata(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    ext_adv_data->flag = false;
    ext_adv_data->data_len = 0;
    memset(fail_bd_addr, 0, GAP_BD_ADDR_LEN);
    return (RESULT_SUCESS);
}

/**
 * @brief Stop recombine advertising data
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "sadvdata",
        "sadvdata\n\r",
        "Stop recombine advertising data\n\r",
        cmd_sadvdata
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_sadvdata(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    ext_adv_data->flag = true;
    ext_adv_data->data_len = 0;
    memset(ext_adv_data->bd_addr, 0, GAP_BD_ADDR_LEN);
    return (RESULT_SUCESS);
}
#endif


/** @brief  User command table */
const T_USER_CMD_TABLE_ENTRY user_cmd_table[] =
{
    /************************** Common cmd *************************************/
    {
        "showcon",
        "showcon\n\r",
        "Show all devices connecting status\n\r",
        cmd_showcon
    },
    {
        "disc",
        "disc [conn_id]\n\r",
        "Disconnect to remote device\n\r",
        cmd_disc
    },
    /************************** Central only *************************************/
    {
        "showdev",
        "showdev\n\r",
        "Show scan dev list\n\r",
        cmd_showdev
    },
    {
        "condev",
        "condev [idx] [init_phys]\n\r",
        "Connect to remote device: use showdev to show idx\r\n\
        [idx]: use cmd showdev to show idx before use this cmd\r\n\
        [init_phys]: 0x001(LE 1M PHY),0x100(LE Coded PHY),... \r\n\
        sample: condev 0 0x001\n\r",
        cmd_condev
    },
    {
        "escan",
        "escan [scan_mode] [scan_phys]\n\r",
        "Start extended scan\r\n\
        [scan_mode]: 0-(continue scanning until scanning is disabled)\r\n\
                     1-(scan for the duration within a scan period, and scan periods continue until scanning is disabled)\r\n\
                     2-(continue scanning until duration has expired)\r\n\
        [scan_phys]: set scan PHYs to 1(LE 1M PHY), 4(LE Coded PHY) or 5(LE 1M PHY and LE Coded PHY)\r\n\
        sample: escan 0 4\n\r",
        cmd_escan
    },
    {
        "stopescan",
        "stopescan\n\r",
        "Stop extended scan\n\r",
        cmd_stopescan
    },
#if APP_RECOMBINE_ADV_DATA
    {
        "radvdata",
        "radvdata\n\r",
        "Recombine advertising data from one device\n\r",
        cmd_radvdata
    },
    {
        "sadvdata",
        "sadvdata\n\r",
        "Stop recombine advertising data\n\r",
        cmd_sadvdata
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
/** @} */ /* End of group BT5_CENTRAL_USER_CMD */
