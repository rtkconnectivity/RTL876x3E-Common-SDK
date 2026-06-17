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
#include "trace.h"
#include "os_mem.h"
#include "cli.h"
#include "bt_types.h"
#include "app_io_msg.h"
#include "app_bt_hid_demo_console.h"

bool app_bt_hid_demo_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_send_msg(&msg);
}

bool console_hid(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint16_t    interval;
    uint8_t     keycode;
    uint8_t     action;
    uint8_t     addr[6];
    uint8_t     i;
    uint8_t     report_id;
    uint8_t     proto_mode;
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

        action = BT_HID_DEMO_ACTION_HID_CONNECT;
    }
    else if (!strcmp(subcmd, "disconnect"))
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

        action = BT_HID_DEMO_ACTION_HID_DISCONNECT;
    }
    else if (!strcmp(subcmd, "shift_left"))
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

        action = BT_HID_DEMO_ACTION_HID_SHIFT_LEFT;
    }
    else if (!strcmp(subcmd, "shift_right"))
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

        action = BT_HID_DEMO_ACTION_HID_SHIFT_RIGHT;
    }
    else if (!strcmp(subcmd, "shift_up"))
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

        action = BT_HID_DEMO_ACTION_HID_SHIFT_UP;
    }
    else if (!strcmp(subcmd, "shift_down"))
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

        action = BT_HID_DEMO_ACTION_HID_SHIFT_DOWN;
    }
    else if (!strcmp(subcmd, "click"))
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

        action = BT_HID_DEMO_ACTION_HID_CLICK;
    }
    else if (!strcmp(subcmd, "sniff"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        interval = (uint16_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);

        p_param += param_len + 1;
        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BT_HID_DEMO_ACTION_HID_SNIFF;
    }
    else if (!strcmp(subcmd, "enable_dlps"))
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

        action = BT_HID_DEMO_ACTION_HID_ENABLE_DLPS;
    }
    else if (!strcmp(subcmd, "click_keycode"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        keycode = *cli_param_get(p_param, 0, &param_len);

        p_param += param_len + 1;
        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BT_HID_DEMO_ACTION_HID_CLICK_KEYCODE;
    }
    else if (!strcmp(subcmd, "host_connect"))
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

        action = BT_HID_DEMO_ACTION_HID_HOST_CONNECT;
    }
    else if (!strcmp(subcmd, "host_disconnect"))
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

        action = BT_HID_DEMO_ACTION_HID_HOST_DISCONNECT;
    }
    else if (!strcmp(subcmd, "host_bond_delete"))
    {
        action = BT_HID_DEMO_ACTION_HID_HOST_BOND_DELETE;
    }
    else if (!strcmp(subcmd, "host_virtual_cable_unplug"))
    {
        action = BT_HID_DEMO_ACTION_HID_HOST_VIRTUAL_CABLE_UNPLUG;
    }
    else if (!strcmp(subcmd, "host_get_report"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        report_id = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);

        p_param += param_len + 1;
        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BT_HID_DEMO_ACTION_HID_HOST_GET_REPORT;
    }
    else if (!strcmp(subcmd, "host_set_report"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        report_id = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);

        p_param += param_len + 1;
        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BT_HID_DEMO_ACTION_HID_HOST_SET_REPORT;
    }
    else if (!strcmp(subcmd, "host_set_protocol"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        proto_mode = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);

        p_param += param_len + 1;
        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BT_HID_DEMO_ACTION_HID_HOST_SET_PROTOCOL;
    }
    else if (!strcmp(subcmd, "host_interrupt_data_send"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        report_id = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);

        p_param += param_len + 1;
        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BT_HID_DEMO_ACTION_HID_HOST_INTERRUPT_DATA_SEND;
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
        LE_UINT16_TO_STREAM(p, HID_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (action == BT_HID_DEMO_ACTION_HID_SNIFF)
        {
            LE_UINT16_TO_STREAM(p, interval);
        }

        if (action == BT_HID_DEMO_ACTION_HID_CLICK_KEYCODE)
        {
            LE_UINT8_TO_STREAM(p, keycode);
        }

        if (action == BT_HID_DEMO_ACTION_HID_HOST_GET_REPORT)
        {
            LE_UINT8_TO_STREAM(p, report_id);
        }

        if (action == BT_HID_DEMO_ACTION_HID_HOST_SET_REPORT)
        {
            LE_UINT8_TO_STREAM(p, report_id);
        }

        if (action == BT_HID_DEMO_ACTION_HID_HOST_SET_PROTOCOL)
        {
            LE_UINT8_TO_STREAM(p, proto_mode);
        }

        if (action == BT_HID_DEMO_ACTION_HID_HOST_INTERRUPT_DATA_SEND)
        {
            LE_UINT8_TO_STREAM(p, report_id);
        }

        ARRAY_TO_STREAM(p, addr, 6);

        if (app_bt_hid_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "HID %s from bt hid demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (hid connect/disconnect).\r\n", subcmd);
    return false;
}

static T_CLI_CMD bt_hid_demo_cmd =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "hid",         /* The command string to type. */
    "\r\nhid:\r\n bt hid demo start\r\n\r\n",
    console_hid,   /* The function to run. */
    -1              /* Zero or more parameters are expected. */
};

void app_bt_hid_demo_cmd_register(void)
{
    cli_cmd_register(&bt_hid_demo_cmd);
}
