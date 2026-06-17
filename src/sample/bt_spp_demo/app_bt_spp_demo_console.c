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
#include "cli.h"
#include "bt_types.h"
#include "app_io_msg.h"
#include "app_bt_spp_demo_console.h"

bool app_spp_demo_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_send_msg(&msg);
}

bool console_spp(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     addr[6];
    uint8_t     i;
    uint8_t     local_server_chann;
    void       *param_buf;
    char       *spp_data;
    uint8_t     data_len = 0;
    uint8_t    *temp_buf;

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

        action = SPP_DEMO_ACTION_CONNECT;
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        local_server_chann = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);

        p_param += param_len + 1;
        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = SPP_DEMO_ACTION_DISCONNECT;
    }
    else if (!strcmp(subcmd, "disconnect_all"))
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

        action = SPP_DEMO_ACTION_DISCONNECT_ALL;
    }
    else if (!strcmp(subcmd, "send_data"))
    {
        p_param = subcmd + param_len + 1;
        spp_data = p_param;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
            spp_data = spp_data + param_len + 1;
        }

        data_len = param_num - 7;
        temp_buf = malloc(sizeof(uint8_t) * data_len);
        if (temp_buf != NULL)
        {
            for (i = 0; i < data_len; i++)
            {
                temp_buf[i] = (uint8_t)strtol(cli_param_get(spp_data, i, &param_len), NULL, 0);
            }
        }

        action = SPP_DEMO_ACTION_SEND_DATA;
    }
    else
    {
        goto err;
    }

    param_buf = malloc(40 + data_len);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, SPP_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (action == SPP_DEMO_ACTION_DISCONNECT)
        {
            LE_UINT8_TO_STREAM(p, local_server_chann);
        }

        ARRAY_TO_STREAM(p, addr, 6);

        if (action == SPP_DEMO_ACTION_SEND_DATA)
        {
            LE_UINT8_TO_STREAM(p, data_len);
            ARRAY_TO_STREAM(p, temp_buf, data_len);
            free(temp_buf);
        }

        if (app_spp_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "SPP %s from spp demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s.\r\n", subcmd);
    return false;
}

static T_CLI_CMD spp_demo_cmd =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "spp",          /* The command string to type. */
    "\r\nspp:\r\n spp demo start\r\n\r\n",
    console_spp,    /* The function to run. */
    -1              /* Zero or more parameters are expected. */
};

void app_spp_demo_cmd_register(void)
{
    cli_cmd_register(&spp_demo_cmd);
}
