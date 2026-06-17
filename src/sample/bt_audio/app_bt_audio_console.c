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
#include "app_bt_audio_console.h"

bool app_bt_audio_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_send_msg(&msg);
}

bool console_bredr(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     addr[6];
    uint8_t     i;
    uint8_t     value;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "addr_set"))
    {
        subcmd += param_len + 1;

        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "local"))
        {
            action = BT_AUDIO_ACTION_BREDR_LOCAL_ADDR_SET;
        }
        else if (!strcmp(subcmd, "remote"))
        {
            action = BT_AUDIO_ACTION_BREDR_REMOTE_ADDR_SET;
        }
        else
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else if (!strcmp(subcmd, "inquiry"))
    {
        action = BT_AUDIO_ACTION_BREDR_INQUIRY_START;
    }
    else if (!strcmp(subcmd, "name_set"))
    {
        action = BT_AUDIO_ACTION_BREDR_NAME_SET;
    }
    else if (!strcmp(subcmd, "device_mode_set"))
    {
        subcmd += param_len + 1;

        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "idle"))
        {
            value = 0;
            action = BT_AUDIO_ACTION_BREDR_DEVICE_MODE_SET;
        }
        else if (!strcmp(subcmd, "discoverable"))
        {
            value = 1;
            action = BT_AUDIO_ACTION_BREDR_DEVICE_MODE_SET;
        }
        else if (!strcmp(subcmd, "connectable"))
        {
            value = 2;
            action = BT_AUDIO_ACTION_BREDR_DEVICE_MODE_SET;
        }
        else if (!strcmp(subcmd, "discoverable_connectable"))
        {
            value = 3;
            action = BT_AUDIO_ACTION_BREDR_DEVICE_MODE_SET;
        }
        else
        {
            goto err;
        }
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
        LE_UINT16_TO_STREAM(p, GAP_BREDR_ID);
        LE_UINT8_TO_STREAM(p, action);

        if ((action == BT_AUDIO_ACTION_BREDR_LOCAL_ADDR_SET) ||
            (action == BT_AUDIO_ACTION_BREDR_REMOTE_ADDR_SET))
        {
            ARRAY_TO_STREAM(p, addr, 6);
        }
        else if (action == BT_AUDIO_ACTION_BREDR_NAME_SET)
        {
            uint8_t  len;
            len  = buf_len - param_len - 1;
            if (len + 4 > 40)
            {
                LE_UINT8_TO_STREAM(p, 40 - 4);
                ARRAY_TO_STREAM(p, subcmd + param_len + 1, 40 - 4);
            }
            else
            {
                LE_UINT8_TO_STREAM(p, len);
                ARRAY_TO_STREAM(p, subcmd + param_len + 1, len);
            }
        }
        else if (action == BT_AUDIO_ACTION_BREDR_DEVICE_MODE_SET)
        {
            LE_UINT8_TO_STREAM(p, value);
        }

        if (app_bt_audio_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }
    }

    snprintf(buf, buf_len, "BREDR %s from bt audio.\r\n", subcmd);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s.\r\n", subcmd);
    return false;
}

static T_CLI_CMD bt_audio_cmd_bredr =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "bredr",         /* The command string to type. */
    "\r\nbredr:\r\n configure Bluetooth bredr param\r\n\r\n",
    console_bredr,   /* The function to run. */
    -1              /* Zero or more parameters are expected. */
};

bool console_a2dp(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
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
        action = BT_AUDIO_ACTION_A2DP_CONNECT;
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        action = BT_AUDIO_ACTION_A2DP_DISCONNECT;
    }
    else if (!strcmp(subcmd, "start"))
    {
        action = BT_AUDIO_ACTION_A2DP_START;
    }
    else if (!strcmp(subcmd, "suspend"))
    {
        action = BT_AUDIO_ACTION_A2DP_SUSPEND;
    }
    else if (!strcmp(subcmd, "vol+"))
    {
        action = BT_AUDIO_ACTION_A2DP_VOLUME_UP;
    }
    else if (!strcmp(subcmd, "vol-"))
    {
        action = BT_AUDIO_ACTION_A2DP_VOLUME_DOWN;
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 40);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, A2DP_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (app_bt_audio_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "A2DP %s from bt audio.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (a2dp connect/disconnect).\r\n", subcmd);
    return false;
}

static T_CLI_CMD bt_audio_cmd_a2dp =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "a2dp",         /* The command string to type. */
    "\r\na2dp:\r\n a2dp demo start\r\n\r\n",
    console_a2dp,   /* The function to run. */
    -1              /* Zero or more parameters are expected. */
};

bool console_avrcp(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     attr_id;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "cover_art_connect"))
    {
        action = BT_AUDIO_ACTION_AVRCP_COVER_ART_CONNECT;
    }
    else if (!strcmp(subcmd, "cover_art_disconnect"))
    {
        action = BT_AUDIO_ACTION_AVRCP_COVER_ART_DISCONNECT;
    }
    else if (!strcmp(subcmd, "cover_art_get"))
    {
        action = BT_AUDIO_ACTION_AVRCP_COVER_ART_GET;
    }
    else if (!strcmp(subcmd, "attr_get"))
    {
        if (param_num != 2)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        attr_id = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);

        action = BT_AUDIO_ACTION_AVRCP_ELEM_ATTR_GET;
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 40);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, AVRCP_ID);
        LE_UINT8_TO_STREAM(p, action);
        if (action == BT_AUDIO_ACTION_AVRCP_ELEM_ATTR_GET)
        {
            LE_UINT8_TO_STREAM(p, attr_id);
        }

        if (app_bt_audio_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "AVRCP %s from bt audio.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (avrcp connect/disconnect).\r\n", subcmd);
    return false;
}

static T_CLI_CMD bt_audio_cmd_avrcp =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "avrcp",         /* The command string to type. */
    "\r\navrcp:\r\n avrcp demo start\r\n\r\n",
    console_avrcp,   /* The function to run. */
    -1              /* Zero or more parameters are expected. */
};

bool console_hfp(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
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
        action = BT_AUDIO_ACTION_HFP_CONNECT;
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        action = BT_AUDIO_ACTION_HFP_DISCONNECT;
    }
    else if (!strcmp(subcmd, "sco_connect"))
    {
        action = BT_AUDIO_ACTION_HFP_SCO_CONNECT;
    }
    else if (!strcmp(subcmd, "call_incoming"))
    {
        action = BT_AUDIO_ACTION_HFP_CALL_INCOMING;
    }
    else if (!strcmp(subcmd, "call_answer"))
    {
        action = BT_AUDIO_ACTION_HFP_CALL_ANSWER;
    }
    else if (!strcmp(subcmd, "call_terminate"))
    {
        action = BT_AUDIO_ACTION_HFP_CALL_TERMINATE;
    }
    else if (!strcmp(subcmd, "sco_disconnect"))
    {
        action = BT_AUDIO_ACTION_HFP_SCO_DISCONNECT;
    }
    else if (!strcmp(subcmd, "vendor_at_cmd"))
    {
        action = BT_AUDIO_ACTION_HFP_VENDOR_AT_CMD;
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 40);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, HFP_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (app_bt_audio_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "HFP %s from bt audio.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (hfp connect/disconnect).\r\n", subcmd);
    return false;
}

static T_CLI_CMD bt_audio_cmd_hfp =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "hfp",          /* The command string to type. */
    "\r\nhfp:\r\n hfp demo start\r\n\r\n",
    console_hfp,    /* The function to run. */
    -1              /* Zero or more parameters are expected. */
};

void app_bt_audio_cmd_register(void)
{
    cli_cmd_register(&bt_audio_cmd_bredr);
#if (A2DP_DEMO_SUPPORT == 1)
    cli_cmd_register(&bt_audio_cmd_a2dp);
    cli_cmd_register(&bt_audio_cmd_avrcp);
#endif

#if (HFP_DEMO_SUPPORT == 1)
    cli_cmd_register(&bt_audio_cmd_hfp);
#endif
}
