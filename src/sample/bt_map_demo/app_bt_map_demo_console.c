/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "cli.h"
#include "bt_types.h"
#include "app_io_msg.h"
#include "app_bt_map_demo_console.h"

bool app_map_demo_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_send_msg(&msg);
}

bool console_map(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     folder;
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

    if (!strcmp(subcmd, "connect"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = MAP_DEMO_ACTION_CONNECT;
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        action = MAP_DEMO_ACTION_DISCONNECT;
    }
    else if (!strcmp(subcmd, "mns_on"))
    {
        action = MAP_DEMO_ACTION_MNS_ON;
    }
    else if (!strcmp(subcmd, "mns_off"))
    {
        action = MAP_DEMO_ACTION_MNS_OFF;
    }
    else if (!strcmp(subcmd, "set_folder"))
    {
        if (param_num != 2)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        folder = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);

        action = MAP_DEMO_ACTION_SET_FOLDER;
    }
    else if (!strcmp(subcmd, "get_folder_listing"))
    {
        action = MAP_DEMO_ACTION_GET_FOLDER_LISTING;
    }
    else if (!strcmp(subcmd, "get_msg_listing"))
    {
        action = MAP_DEMO_ACTION_GET_MSG_LISTING;
    }
    else if (!strcmp(subcmd, "get_msg"))
    {
        action = MAP_DEMO_ACTION_GET_MSG;
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
        LE_UINT16_TO_STREAM(p, MAP_ID);
        LE_UINT8_TO_STREAM(p, action);
        if (action == MAP_DEMO_ACTION_CONNECT)
        {
            ARRAY_TO_STREAM(p, addr, 6);
        }
        else if (action == MAP_DEMO_ACTION_SET_FOLDER)
        {
            LE_UINT8_TO_STREAM(p, folder);
        }

        if (app_map_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "MAP %s from map demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (map connect/disconnect).\r\n", subcmd);
    return false;
}

static T_CLI_CMD map_demo_cmd =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "map",          /* The command string to type. */
    "\r\nmap:\r\n map demo start\r\n\r\n",
    console_map,    /* The function to run. */
    -1              /* Zero or more parameters are expected. */
};

void app_map_demo_cmd_register(void)
{
    cli_cmd_register(&map_demo_cmd);
}
