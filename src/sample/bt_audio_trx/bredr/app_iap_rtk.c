/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_IAP_RTK_SUPPORT
#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "os_queue.h"
#include "common_stream.h"
#include "iap_stream.h"
#include "app_cfg.h"
#include "app_transfer_cfg.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_iap_rtk.h"
#include "app_iap.h"
#include "app_flags.h"

typedef enum t_relay_msg_type
{
    RTK_IAP_CREATE = 0,
    RTK_IAP_DELETE = 1,
    RTK_IAP_CONNECTED = 2,
    RTK_IAP_DISCONNECTED = 3,
    SYNC_ALL       = 4,
    RTK_IAP_MSG_MAX,
} T_RELAY_MSG_TYPE;

typedef struct t_rtk_iap_info T_RTK_IAP_INFO;

struct t_rtk_iap_info
{
    T_RTK_IAP_INFO  *p_next;
    bool            session_connected;
    uint16_t        session_id;
    uint8_t         bd_addr[6];
};

typedef struct t_relay_info
{
    uint32_t num;
    T_RTK_IAP_INFO iap_infos[1];
} T_RELAY_INFO;

static struct
{
    T_OS_QUEUE queue;
} app_iap_rtk;


#define INVALID_SESSION_ID 0xFFFF

static T_RTK_IAP_INFO *app_iap_rtk_search_by_addr(uint8_t *addr)
{
    T_RTK_IAP_INFO *p_elem = (T_RTK_IAP_INFO *)app_iap_rtk.queue.p_first;

    for (; p_elem != NULL; p_elem = p_elem->p_next)
    {
        if (memcmp(p_elem->bd_addr, addr, 6) == 0)
        {
            return p_elem;
        }
    }

    return NULL;
}

static void app_iap_rtk_ready_to_read_process(COMMON_STREAM stream)
{
    T_APP_BR_LINK *p_link;
    uint8_t bd_addr[6];
    uint8_t *p_data = NULL;
    uint32_t len = 0;
    uint16_t data_len;
    uint8_t app_idx;
    uint16_t total_len;
    uint8_t *data = NULL;

    common_stream_read(stream, &data, &len);

    p_data = data;
    data_len = len;

    common_stream_get_addr(stream, bd_addr);

    p_link = app_link_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        APP_PRINT_ERROR0("app_iap_rtk_ready_to_read_process: no acl link found");
        free(data);
        return;
    }

    app_idx = p_link->id;

    if (app_transfer_cfg.enable_embedded_cmd)
    {
        app_cmd_proc_data(CMD_PATH_IAP, &p_link->cmd.buf, &p_link->cmd.len, p_data, data_len, app_idx);
    }
}

static void app_iap_rtk_stream_cb(COMMON_STREAM stream, COMMON_STREAM_EVENT event)
{
    APP_PRINT_TRACE1("app_iap_rtk_stream_cb: event %d", event);

    switch (event)
    {
    case COMMON_STREAM_CONNECTED_EVENT:
        {
            T_RTK_IAP_INFO *p_iap_info = NULL;
            IAP_STREAM iap_stream = (IAP_STREAM) stream;

            p_iap_info = app_iap_rtk_search_by_addr(stream->bd_addr);

            if (p_iap_info)
            {
                p_iap_info->session_connected = true;
                p_iap_info->session_id = iap_stream->eap_session_id;
            }
        }
        break;

    case COMMON_STREAM_DISCONNECTED_EVENT:
        {
            T_RTK_IAP_INFO *p_iap_info = NULL;

            p_iap_info = app_iap_rtk_search_by_addr(stream->bd_addr);
            if (p_iap_info)
            {
                p_iap_info->session_connected = false;
                p_iap_info->session_id = INVALID_SESSION_ID;
            }
        }
        break;

    case COMMON_STREAM_READY_TO_READ_EVENT:
        {
            app_iap_rtk_ready_to_read_process(stream);
        }
        break;

    default:
        {
            APP_PRINT_ERROR1("app_iap_rtk_stream_cb: unknown event %d", event);
        }
        break;
    }
}

bool app_iap_rtk_delete(uint8_t *bd_addr)
{
    IAP_STREAM iap_rtk_stream = NULL;

    APP_PRINT_TRACE1("app_iap_rtk_delete: bd_addr %s", TRACE_BDADDR(bd_addr));

    iap_rtk_stream = iap_stream_find_by_ea_protocol_id(EA_PROTOCOL_ID_RTK, bd_addr);

    if (iap_rtk_stream == NULL)
    {
        APP_PRINT_ERROR0("app_iap_rtk_delete: iap_rtk_stream is NULL");
        return false;
    }

    common_stream_delete((COMMON_STREAM)iap_rtk_stream);

    T_RTK_IAP_INFO *p_elem = NULL;
    p_elem = app_iap_rtk_search_by_addr(bd_addr);

    if (p_elem != NULL)
    {
        os_queue_delete(&app_iap_rtk.queue, p_elem);
        free(p_elem);
    }

    return true;
}

bool app_iap_rtk_create(uint8_t *bd_addr)
{
    APP_PRINT_TRACE1("app_iap_rtk_create: %s", TRACE_BDADDR(bd_addr));

    IAP_INIT_SETTINGS iap_stream_settings =
    {
        .bd_addr            = bd_addr,
        .stream_cb          = app_iap_rtk_stream_cb,
        .ea_protocol_id     = EA_PROTOCOL_ID_RTK,
    };

    iap_stream_create(&iap_stream_settings);

    T_RTK_IAP_INFO *p_elem = NULL;

    p_elem = app_iap_rtk_search_by_addr(bd_addr);
    if (p_elem == NULL)
    {
        p_elem = calloc(1, sizeof(*p_elem));
        p_elem->p_next = NULL;
        p_elem->session_connected = false;
        p_elem->session_id = INVALID_SESSION_ID;
        memcpy(p_elem->bd_addr, bd_addr, sizeof(p_elem->bd_addr));
        os_queue_in(&app_iap_rtk.queue, p_elem);
    }

    return true;
}


bool app_iap_rtk_launch(uint8_t *bd_addr, T_BT_IAP_APP_LAUNCH_METHOD method)
{
    T_RTK_IAP_INFO *p_iap_info = NULL;

    p_iap_info = app_iap_rtk_search_by_addr(bd_addr);
    if (p_iap_info)
    {
        if (p_iap_info->session_connected)
        {
            APP_PRINT_ERROR0("app_iap_rtk_launch: this rtk iap already connected");
            return false;
        }
    }

    char boundle_id[] = "com.realtek.EADemo2";
    bt_iap_app_launch(bd_addr, boundle_id, sizeof(boundle_id), method);

    return true;
}

bool app_iap_rtk_connected(uint8_t *bd_addr)
{
    T_RTK_IAP_INFO *p_iap_info = NULL;

    p_iap_info = app_iap_rtk_search_by_addr(bd_addr);
    if (p_iap_info)
    {
        return p_iap_info->session_connected;
    }

    return false;
}

bool app_iap_rtk_send(uint8_t *bd_addr, uint8_t *data, uint32_t len)
{
    T_RTK_IAP_INFO *p_iap_info = NULL;

    p_iap_info = app_iap_rtk_search_by_addr(bd_addr);
    if (p_iap_info)
    {
        return bt_iap_data_send(bd_addr,
                                p_iap_info->session_id, data, len);
    }

    return false;
}

IAP_SEND_RESULT app_iap_rtk_transfer(uint8_t app_idx, uint8_t *data, uint32_t len)
{
    IAP_SEND_RESULT result = IAP_SEND_OK;
    T_APP_IAP_HDL app_iap_hdl = NULL;
    app_iap_hdl = app_iap_search_by_addr(app_db.br_link[app_idx].bd_addr);

    if ((app_db.br_link[app_idx].connected_profile & IAP_PROFILE_MASK) == 0)
    {
        result = IAP_PROFILE_NOT_CONN;
        return result;
    }

    if (!app_iap_rtk_connected(app_db.br_link[app_idx].bd_addr))
    {
        if (app_iap_is_authened(app_iap_hdl))
        {
            app_iap_rtk_launch(app_db.br_link[app_idx].bd_addr, BT_IAP_APP_LAUNCH_WITH_USER_ALERT);
            result = IAP_RTK_LAUNCHED;
        }
        else
        {
            result = IAP_SESSION_NOT_CONN;
        }
        return result;
    }

    if (!app_iap_rtk_send(app_db.br_link[app_idx].bd_addr, data, len))
    {
        result = IAP_SEND_FAIL;
    }

    return result;
}

void app_iap_rtk_init(void)
{
    os_queue_init(&app_iap_rtk.queue);
}
#endif

