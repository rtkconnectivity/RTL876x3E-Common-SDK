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
#include "app_audio_io_msg.h"
#include "app_audio_demo_console.h"

bool app_audio_demo_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_send_msg(&msg);
}

bool console_notification(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action = 0;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "ringtone"))
    {
        if (param_num != 2)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        subcmd = (char *)cli_param_get(p_param, 0, &param_len);

        if (!strcmp(subcmd, "play"))
        {
            action = AUDIO_DEMO_ACTION_RING_TONE_PLAY;
        }
    }
    else if (!strcmp(subcmd, "vp"))
    {
        if (param_num != 2)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        subcmd = (char *)cli_param_get(p_param, 0, &param_len);

        if (!strcmp(subcmd, "play"))
        {
            action = AUDIO_DEMO_ACTION_VOICE_PROMPT_PLAY;
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
        LE_UINT16_TO_STREAM(p, NOTIFICATION_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (app_audio_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "notification %s from audio demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (notification play/stop).\r\n", subcmd);
    return false;
}

bool console_apt(const char *cmd_str, char *buf, size_t buf_len)
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

    if (!strcmp(subcmd, "enable"))
    {
        action = AUDIO_DEMO_ACTION_APT_PLAY;
    }
    else if (!strcmp(subcmd, "disable"))
    {
        action = AUDIO_DEMO_ACTION_APT_STOP;
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
        LE_UINT16_TO_STREAM(p, APT_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (app_audio_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "apt %s from audio demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (apt play/stop).\r\n", subcmd);
    return false;
}

bool console_llapt(const char *cmd_str, char *buf, size_t buf_len)
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

    if (!strcmp(subcmd, "enable"))
    {
        action = AUDIO_DEMO_ACTION_LLAPT_PLAY;
    }
    else if (!strcmp(subcmd, "disable"))
    {
        action = AUDIO_DEMO_ACTION_LLAPT_STOP;
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
        LE_UINT16_TO_STREAM(p, LLAPT_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (app_audio_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "llapt %s from audio demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (llapt play/stop).\r\n", subcmd);
    return false;
}

bool console_line_in(const char *cmd_str, char *buf, size_t buf_len)
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

    if (!strcmp(subcmd, "start"))
    {
        action = AUDIO_DEMO_ACTION_LINE_IN_START;
    }
    else if (!strcmp(subcmd, "stop"))
    {
        action = AUDIO_DEMO_ACTION_LINE_IN_STOP;
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
        LE_UINT16_TO_STREAM(p, LINE_IN_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (app_audio_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "line_in %s from audio demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (line_in start/stop).\r\n", subcmd);
    return false;
}

bool console_anc(const char *cmd_str, char *buf, size_t buf_len)
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

    if (!strcmp(subcmd, "enable"))
    {
        action = AUDIO_DEMO_ACTION_ANC_ENABLE;
    }
    else if (!strcmp(subcmd, "disable"))
    {
        action = AUDIO_DEMO_ACTION_ANC_DISABLE;
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
        LE_UINT16_TO_STREAM(p, ANC_ID);
        LE_UINT8_TO_STREAM(p, action);

        if (app_audio_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "anc %s from audio demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (anc enable/disable).\r\n", subcmd);
    return false;
}

bool console_pipe(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     src_type = 0;
    uint8_t     snk_type = 0;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "create"))
    {
        if (param_num != 3)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        src_type = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);
        snk_type = (uint8_t)strtol(cli_param_get(p_param, 1, &param_len), NULL, 0);

        action = AUDIO_DEMO_ACTION_PIPE_CREATE;
    }
    else if (!strcmp(subcmd, "start"))
    {
        action = AUDIO_DEMO_ACTION_PIPE_START;
    }
    else if (!strcmp(subcmd, "stop"))
    {
        action = AUDIO_DEMO_ACTION_PIPE_STOP;
    }
    else if (!strcmp(subcmd, "release"))
    {
        action = AUDIO_DEMO_ACTION_PIPE_RELEASE;
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
        LE_UINT16_TO_STREAM(p, PIPE_ID);
        LE_UINT8_TO_STREAM(p, action);
        LE_UINT8_TO_STREAM(p, src_type);
        LE_UINT8_TO_STREAM(p, snk_type);

        if (app_audio_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "pipe %s from audio demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s .\r\n", subcmd);
    return false;
}

bool console_track(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     stream_type;
    uint8_t     format_type;
    uint8_t     volume_out;
    uint8_t     volume_in;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "create"))
    {
        if (param_num != 5)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        stream_type = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);
        format_type = (uint8_t)strtol(cli_param_get(p_param, 1, &param_len), NULL, 0);
        volume_out = (uint8_t)strtol(cli_param_get(p_param, 2, &param_len), NULL, 0);
        volume_in = (uint8_t)strtol(cli_param_get(p_param, 3, &param_len), NULL, 0);

        action = AUDIO_DEMO_ACTION_TRACK_CREATE;
    }
    else if (!strcmp(subcmd, "start"))
    {
        action = AUDIO_DEMO_ACTION_TRACK_START;
    }
    else if (!strcmp(subcmd, "stop"))
    {
        action = AUDIO_DEMO_ACTION_TRACK_STOP;
    }
    else if (!strcmp(subcmd, "pause"))
    {
        action = AUDIO_DEMO_ACTION_TRACK_PAUSE;
    }
    else if (!strcmp(subcmd, "restart"))
    {
        action = AUDIO_DEMO_ACTION_TRACK_RESTART;
    }
    else if (!strcmp(subcmd, "release"))
    {
        action = AUDIO_DEMO_ACTION_TRACK_RELEASE;
    }
    else if (!strcmp(subcmd, "vol_out_set"))
    {
        if (param_num != 2)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        volume_out = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);

        action = AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_SET;
    }
    else if (!strcmp(subcmd, "vol_out_mute"))
    {
        action = AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_MUTE;
    }
    else if (!strcmp(subcmd, "vol_out_unmute"))
    {
        action = AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_UNMUTE;
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
        LE_UINT16_TO_STREAM(p, TRACK_ID);
        LE_UINT8_TO_STREAM(p, action);
        if (action == AUDIO_DEMO_ACTION_TRACK_CREATE)
        {
            LE_UINT8_TO_STREAM(p, stream_type);
            LE_UINT8_TO_STREAM(p, format_type);
            LE_UINT8_TO_STREAM(p, volume_out);
            LE_UINT8_TO_STREAM(p, volume_in);
        }
        else if (action == AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_SET)
        {
            LE_UINT8_TO_STREAM(p, volume_out);
        }

        if (app_audio_demo_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            free(param_buf);
            goto err;
        }

        snprintf(buf, buf_len, "track %s from audio demo.\r\n", subcmd);
    }

    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s .\r\n", subcmd);
    return false;
}

#if (NOTIFICATION_DEMO_SUPPORT == 1)
static T_CLI_CMD audio_demo_notification_cmd =
{
    NULL,            /* Next command pointed to NULL. */
    NULL,            /* Next subcommand. */
    "notification",  /* The command string to type. */
    "\r\notification:\r\n audio demo start\r\n\r\n",
    console_notification,   /* The function to run. */
    -1               /* Zero or more parameters are expected. */
};
#endif

#if (APT_DEMO_SUPPORT == 1)
static T_CLI_CMD audio_demo_apt_cmd =
{
    NULL,            /* Next command pointed to NULL. */
    NULL,            /* Next subcommand. */
    "apt",           /* The command string to type. */
    "\r\napt:\r\n apt demo start\r\n\r\n",
    console_apt,     /* The function to run. */
    -1               /* Zero or more parameters are expected. */
};
#endif

#if (LLAPT_DEMO_SUPPORT == 1)
static T_CLI_CMD audio_demo_llapt_cmd =
{
    NULL,            /* Next command pointed to NULL. */
    NULL,            /* Next subcommand. */
    "llapt",         /* The command string to type. */
    "\r\nllapt:\r\n llapt demo start\r\n\r\n",
    console_llapt,   /* The function to run. */
    -1               /* Zero or more parameters are expected. */
};
#endif

#if (LINE_IN_DEMO_SUPPORT == 1)
static T_CLI_CMD audio_demo_line_in_cmd =
{
    NULL,            /* Next command pointed to NULL. */
    NULL,            /* Next subcommand. */
    "line_in",       /* The command string to type. */
    "\r\nline_in:\r\n line_in demo start\r\n\r\n",
    console_line_in, /* The function to run. */
    -1               /* Zero or more parameters are expected. */
};
#endif

#if (ANC_DEMO_SUPPORT == 1)
static T_CLI_CMD audio_demo_anc_cmd =
{
    NULL,            /* Next command pointed to NULL. */
    NULL,            /* Next subcommand. */
    "anc",           /* The command string to type. */
    "\r\nanc:\r\n anc demo start\r\n\r\n",
    console_anc,     /* The function to run. */
    -1               /* Zero or more parameters are expected. */
};
#endif

#if (AUDIO_PIPE_DEMO_SUPPORT == 1)
static T_CLI_CMD audio_demo_pipe_cmd =
{
    NULL,            /* Next command pointed to NULL. */
    NULL,            /* Next subcommand. */
    "pipe",          /* The command string to type. */
    "\r\npipe:\r\n audio pipe demo start\r\n\r\n",
    console_pipe,    /* The function to run. */
    -1               /* Zero or more parameters are expected. */
};
#endif

#if (AUDIO_TRACK_DEMO_SUPPORT == 1)
static T_CLI_CMD audio_demo_track_cmd =
{
    NULL,            /* Next command pointed to NULL. */
    NULL,            /* Next subcommand. */
    "track",         /* The command string to type. */
    "\r\ntrack:\r\n audio track demo start\r\n\r\n",
    console_track,   /* The function to run. */
    -1               /* Zero or more parameters are expected. */
};
#endif

void app_audio_demo_cmd_register(void)
{
#if (NOTIFICATION_DEMO_SUPPORT == 1)
    cli_cmd_register(&audio_demo_notification_cmd);
#endif

#if (APT_DEMO_SUPPORT == 1)
    cli_cmd_register(&audio_demo_apt_cmd);
#endif

#if (LLAPT_DEMO_SUPPORT == 1)
    cli_cmd_register(&audio_demo_llapt_cmd);
#endif

#if (LINE_IN_DEMO_SUPPORT == 1)
    cli_cmd_register(&audio_demo_line_in_cmd);
#endif

#if (ANC_DEMO_SUPPORT == 1)
    cli_cmd_register(&audio_demo_anc_cmd);
#endif

#if (AUDIO_PIPE_DEMO_SUPPORT == 1)
    cli_cmd_register(&audio_demo_pipe_cmd);
#endif

#if (AUDIO_TRACK_DEMO_SUPPORT == 1)
    cli_cmd_register(&audio_demo_track_cmd);
#endif
}
