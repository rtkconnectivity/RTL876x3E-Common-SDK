/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "os_mem.h"
#include "cli.h"
#include "bt_types.h"
#include "app_io_msg.h"
#include "app_bt_pan_demo_console.h"

bool app_bt_pan_demo_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_send_msg(&msg);
}

bool console_pan(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     addr[6];
    uint8_t     i;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';
    p_param = subcmd + param_len + 1;

    if (!strcmp(subcmd, "connect"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BT_PAN_DEMO_ACTION_PAN_CONNECT;
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_DISCONNECT;
    }
    else if (!strcmp(subcmd, "setup_conn_rsp"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_SETUP_CONN_RSP;
    }
    else if (!strcmp(subcmd, "net_type_filter_set"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_NET_TYPE_FILTER_SET;
    }
    else if (!strcmp(subcmd, "multi_addr_filter_set"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_MULTI_ADDR_FILTER_SET;
    }
    else if (!strcmp(subcmd, "send_general"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_SNED_GENERAL;
    }
    else if (!strcmp(subcmd, "send_compressed"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_SNED_COMPRESSED;
    }
    else if (!strcmp(subcmd, "send_src_only"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_SNED_SRC_ONLY;
    }
    else if (!strcmp(subcmd, "send_dst_only"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_SNED_DST_ONLY;
    }
    else if (!strcmp(subcmd, "arp"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_ARP;
    }
    else if (!strcmp(subcmd, "arp_v6"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_ARP_V6;
    }
    else if (!strcmp(subcmd, "ping"))
    {
#include "pan_test/ping.h"
        char *server_name = (char *)cli_param_get(p_param, 0, &param_len);
        app_bt_pan_set_ping(server_name, param_len);
        action = BT_PAN_DEMO_ACTION_PAN_PING;
    }
    else if (!strcmp(subcmd, "ping_v6"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_PING_V6;
    }
    else if (!strcmp(subcmd, "ping_reply"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_PING_REPLY;
    }
    else if (!strcmp(subcmd, "ping_reply_v6"))
    {
        action = BT_PAN_DEMO_ACTION_PAN_PING_REPLY_V6;
    }
    else if (!strcmp(subcmd, "http"))
    {
#include "pan_test/http.h"
        uint32_t ip_str_len = 0, uri_len = 0;
        char *ip_str = (char *)cli_param_get(p_param, 0, &ip_str_len);
        uint16_t port = (uint16_t)strtol(cli_param_get(p_param, 1, &param_len), NULL, 0);
        char *uri = (char *)cli_param_get(p_param, 2, &uri_len);

        app_bt_pan_set_http(ip_str, ip_str_len, port, uri, uri_len);

        action = BT_PAN_DEMO_ACTION_HTTP;
    }
    else if (!strcmp(subcmd, "sockets"))
    {
#include "pan_test/sockets.h"

        uint32_t param_idx = 0;

        char *mode_str = (char *)cli_param_get(p_param, param_idx++, &param_len);
        app_bt_pan_sockets_set_mode(mode_str);

        if (app_bt_pan_sockets_get_mode() == APP_BT_PAN_SOCKETS_CLIENT)
        {
            char *ip_str = (char *)cli_param_get(p_param, param_idx++, &param_len);
            app_bt_pan_sockets_set_ip_str(ip_str, param_len);
        }

        uint16_t port = (uint16_t)strtol(cli_param_get(p_param, param_idx++, &param_len), NULL, 0);
        app_bt_pan_sockets_set_port(port);

        action = BT_PAN_DEMO_ACTION_SOCKETS;
    }
    else if (!strcmp(subcmd, "sem"))
    {
        action = BT_PAN_DEMO_ACTION_SEM;
    }
    else if (!strcmp(subcmd, "https"))
    {
        action = BT_PAN_DEMO_ACTION_HTTPS_CLIENT;
    }
    else
    {
        goto err;
    }

    param_buf = malloc(40);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, PAN_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (action == BT_PAN_DEMO_ACTION_PAN_CONNECT)
        {
            ARRAY_TO_STREAM(p, addr, 6);
        }

        if (app_bt_pan_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "PAN %s from bt pan demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (pan connect/disconnect).\r\n", subcmd);
    return false;
}

static T_CLI_CMD bt_pan_demo_cmd =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "pan",         /* The command string to type. */
    "\r\npan:\r\n bt pan demo start\r\n\r\n",
    console_pan,   /* The function to run. */
    -1              /* Zero or more parameters are expected. */
};

void app_bt_pan_demo_cmd_register(void)
{
    cli_cmd_register(&bt_pan_demo_cmd);
}
