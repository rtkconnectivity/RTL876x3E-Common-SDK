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
#include "app_bt_pbap_demo_console.h"

bool pbap_demo_send_console_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_send_msg(&msg);
}

bool console_pbap(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
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
        action = APP_ACTION_PBAP_CONNECT;
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        action = APP_ACTION_PBAP_DISCONNECT;
    }
    else if (!strcmp(subcmd, "set_phone_book"))
    {
        action = APP_ACTION_PBAP_SET_PHONE_BOOK;
    }
    else if (!strcmp(subcmd, "pull_caller_id_name"))
    {
        action = APP_ACTION_PBAP_PULL_CALLER_ID_NAME;
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
        LE_UINT16_TO_STREAM(p, PBAP_ID);
        LE_UINT8_TO_STREAM(p, action);
        if (action == APP_ACTION_PBAP_CONNECT)
        {
            uint8_t     addr[6];

            p_param = subcmd + param_len + 1;
            for (i = 0; i < 6; i++)
            {
                addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
            }
            ARRAY_TO_STREAM(p, addr, 6);
        }
        else if (action == APP_ACTION_PBAP_SET_PHONE_BOOK)
        {
            p_param = subcmd + param_len + 1;
            for (i = 0; i < 2; i++)
            {
                p_param = (char *)cli_param_get(p_param, i, &param_len);
                ARRAY_TO_STREAM(p, p_param, param_len);
            }
            *p = '\0';
        }
        else if (action == APP_ACTION_PBAP_PULL_CALLER_ID_NAME)
        {
            p_param = subcmd + param_len + 1;
            p_param = (char *)cli_param_get(p_param, 0, &param_len);
            ARRAY_TO_STREAM(p, p_param, param_len);
            *p = '\0';
        }

        if (pbap_demo_send_console_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "pbap %s from console.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (pbap connect/disconnect/data_send).\r\n", subcmd);
    return false;
}

static T_CLI_CMD app_cmd_pbap =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "pbap",         /* The command string to type. */
    "\r\npbap:\r\n pbap connect\r\n pbap disconnect\r\n pbap data_send\r\n\r\n",
    console_pbap,   /* The function to run. */
    -1              /* Zero or more parameters are expected. */
};

void app_pbap_cmd_register(void)
{
    cli_cmd_register(&app_cmd_pbap);
}
