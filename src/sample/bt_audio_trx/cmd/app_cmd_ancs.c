/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#if F_APP_BT_ANCS_CLIENT_SUPPORT
#include <trace.h>
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "ancs.h"
#include "ancs_gatt_client.h"
#include "app_customer_cmd.h"
#include "app_cmd.h"
#include "app_cmd_ancs.h"
#include "app_main.h"

typedef enum
{
    PARAM_START_POS = 2,
} CMD_POS;


static struct
{
    struct
    {
        uint8_t app_idx;
        T_CMD_PATH cur_path;
    } app_cmd;
} ancs =
{
    .app_cmd.app_idx = 0x0,
    .app_cmd.cur_path = CMD_PATH_UART
};

#define HI_WORD(x)  ((uint8_t)((x & 0xFF00) >> 8))
#define LO_WORD(x)  ((uint8_t)(x))

static void set_trans_info(uint8_t app_idx, T_CMD_PATH cur_path)
{
    ancs.app_cmd.app_idx = app_idx;
    ancs.app_cmd.cur_path = cur_path;
}


static void send_cmd_evt(uint16_t event_id, uint8_t *params, uint16_t len)
{
    app_report_event(ancs.app_cmd.cur_path, event_id, ancs.app_cmd.app_idx, params, len);
}


static void send_notify_enable_evt(uint16_t conn_handle, T_ANCS_SOURCE ancs_write_type)
{
    T_EVENT_ID evt = (T_EVENT_ID)0xff;

    if (ancs_write_type == ANCS_NOTIFICATION_SOURCE)
    {
        evt = EVENT_ANCS_NOTI_SRC_NOTIFY_EN;
    }
    else if (ancs_write_type == ANCS_DATA_SOURCE)
    {
        evt = EVENT_ANCS_DATA_SRC_NOTIFY_EN;
    }
    else
    {
        return;
    }

    struct
    {
        uint8_t app_link_id;
    } __attribute__((packed)) rpt = {};

    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
    rpt.app_link_id = p_link->id;

    send_cmd_evt(evt, (uint8_t *)&rpt, sizeof(rpt));
}


static void send_notification(uint16_t conn_handle, T_ANCS_SOURCE type, uint8_t *noti_data,
                              uint16_t len)
{
    enum
    {
        APP_LINK_ID         = 0,
        ANCS_DATA_TYPE_POS  = APP_LINK_ID + 1,
        PKT_TYPE_POS        = ANCS_DATA_TYPE_POS + 1,
        TOTAL_LEN_POS       = PKT_TYPE_POS + 1,
        PKT_LEN_POS         = TOTAL_LEN_POS + 2,
        PKT_CONTENT_POS     = PKT_LEN_POS + 2,
    } PARAM_POS;

    uint8_t *params = calloc(1, PKT_CONTENT_POS + len);

    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    params[APP_LINK_ID] = p_link->id;
    params[ANCS_DATA_TYPE_POS] = type;
    params[PKT_TYPE_POS] = PKT_TYPE_SINGLE;
    params[TOTAL_LEN_POS] = (uint8_t)len;
    params[TOTAL_LEN_POS + 1] = len >> 8;
    params[PKT_LEN_POS] = params[TOTAL_LEN_POS];
    params[PKT_LEN_POS + 1] = params[TOTAL_LEN_POS + 1];

    memcpy(&params[PKT_CONTENT_POS], noti_data, len);

    send_cmd_evt(EVENT_ANCS_NOTIFICATION, params, PKT_CONTENT_POS + len);

    free(params);
}



void ancs_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr, uint8_t *ack_pkt)
{
    bool ret = false;
    typedef struct
    {
        uint16_t cmd_id;
        uint8_t  status;
    } __attribute__((packed)) *ACK_PKT;

    set_trans_info(app_idx, cmd_path);
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE2("ancs_handle_cmd: cmd_id 0x%04x, app_link_id %d", cmd_id,
                     cmd_ptr[PARAM_START_POS]);

    switch (cmd_id)
    {
    case CMD_ANCS_REGISTER:
        {
            uint8_t app_link_id = cmd_ptr[PARAM_START_POS];
        }
        break;

    case CMD_ANCS_GET_NOTIFICATION_ATTR:
        {

            enum
            {
                APP_LINK_ID_POS     = PARAM_START_POS,
                NOTI_UID_POS        = APP_LINK_ID_POS + 1,
                ATTR_IDS_LEN_POS    = NOTI_UID_POS + 4,
                ATTR_IDS_POS        = ATTR_IDS_LEN_POS + 1,
            } PARAMS_POS;

            uint8_t app_link_id = cmd_ptr[APP_LINK_ID_POS];
            uint32_t noti_uid = 0; LE_ARRAY_TO_UINT32(noti_uid, &cmd_ptr[NOTI_UID_POS]);
            uint8_t attr_ids_len = cmd_ptr[ATTR_IDS_LEN_POS];
            uint32_t cpy_len = (attr_ids_len < ANCS_ATTR_IDS_MAX_LEN) ? attr_ids_len : ANCS_ATTR_IDS_MAX_LEN;

            ret = ancs_client_cp_get_notification_attr(app_db.le_link[app_link_id].conn_handle,
                                                       noti_uid, &cmd_ptr[ATTR_IDS_POS], attr_ids_len);
            if (ret == false)
            {
                ((ACK_PKT)ack_pkt)->status = (uint8_t)CMD_SET_STATUS_PROCESS_FAIL;
            }

        }
        break;

    case CMD_ANCS_GET_APP_ATTR:
        {
            enum
            {
                APP_LINK_ID_POS = PARAM_START_POS,
                APP_IDENT_LEN_POS = APP_LINK_ID_POS + 1,
                APP_IDENT_POS = APP_IDENT_LEN_POS + 1
            } PARAMS_POS;

            uint8_t app_link_id = cmd_ptr[APP_LINK_ID_POS];
            uint8_t app_ident_len = cmd_ptr[APP_IDENT_LEN_POS];
            uint8_t *app_idents = &cmd_ptr[APP_IDENT_POS];
            char app_identifier[ANCS_APP_IDENTIFIER_MAX_LEN];
            uint32_t cpy_len = 0;
            uint8_t attr_id_list[14];
            uint8_t cur_index = 0;

            cpy_len = app_ident_len < (ANCS_APP_IDENTIFIER_MAX_LEN - 1) ? app_ident_len :
                      (ANCS_APP_IDENTIFIER_MAX_LEN - 1);
            memcpy(app_identifier, app_idents, cpy_len);

            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_APP_IDENTIFIER;
            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_TITLE;
            attr_id_list[cur_index++] = LO_WORD(ANCS_MAX_ATTR_LEN);
            attr_id_list[cur_index++] = HI_WORD(ANCS_MAX_ATTR_LEN);
            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_SUB_TITLE;
            attr_id_list[cur_index++] = LO_WORD(ANCS_MAX_ATTR_LEN);
            attr_id_list[cur_index++] = HI_WORD(ANCS_MAX_ATTR_LEN);
            attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_DATE;

            ret = ancs_client_cp_get_app_attr(app_db.le_link[app_link_id].conn_handle,
                                              (char *)app_identifier, attr_id_list, cur_index);
            if (ret == false)
            {
                ((ACK_PKT)ack_pkt)->status = (uint8_t)CMD_SET_STATUS_PROCESS_FAIL;
            }
        }
        break;

    case CMD_ANCS_PERFORM_NOTIFICATION_ACTION:
        {
            enum
            {
                APP_LINK_ID_POS = PARAM_START_POS,
                NOTI_UID_POS = APP_LINK_ID_POS + 1,
                ACTION_ID_POS = NOTI_UID_POS + 4,
            } PARAMS_POS;

            uint8_t app_link_id = cmd_ptr[PARAM_START_POS];
            uint32_t noti_uid = 0;
            LE_ARRAY_TO_UINT32(noti_uid, &cmd_ptr[NOTI_UID_POS]);
            uint8_t action_id = cmd_ptr[ACTION_ID_POS];

            ret = ancs_client_cp_perform_notification_action(app_db.le_link[app_link_id].conn_handle,
                                                             noti_uid, action_id);
            if (ret == false)
            {
                ((ACK_PKT)ack_pkt)->status = (uint8_t)CMD_SET_STATUS_PROCESS_FAIL;
            }
        }
        break;

    default:
        break;
    }

    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
}

#endif
