/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "rtl876x_rcc.h"
#include "rtl876x_i2c.h"
#include "bt_types.h"
#include "gap_br.h"
#include "console.h"
#include "btm.h"
#include "bt_sdp.h"
#include "bt_pbap.h"
#include "app_bt_pbap_demo_app.h"
#include "app_bt_pbap_demo_link.h"

extern char *local_name;
extern uint8_t local_bd_addr[6];

uint8_t remote_bd_addr[6];

/** @brief  PBAP UUID */
static const T_BT_SDP_UUID_DATA pbap_service_uuid16 =
{
    .uuid_16 = UUID_PBAP_PSE
};

bool pbap_demo_conn_start(uint8_t *bd_addr)
{
    APP_PRINT_TRACE0("pbap_demo_conn_start");
    if (bt_sdp_discov_start(bd_addr, BT_SDP_UUID16, pbap_service_uuid16))
    {
        return true;
    }

    return false;
}

bool pbap_demo_disconnect_start(void)
{
    T_PBAP_DEMO_LINK *p_link;

    APP_PRINT_TRACE0("pbap_demo_disconnect_start");

    p_link = pbap_demo_find_link(remote_bd_addr);
    if (p_link != NULL)
    {
        return bt_pbap_disconnect_req(remote_bd_addr);
    }

    return false;
}

/**
 * @brief     PBAP Tx function for test.
 * @param[in] phone book path.
 * @return    void
 */

bool pbap_demo_app_set_phone_book(char *p_param)
{
    T_PBAP_DEMO_LINK *p_link;
    T_BT_PBAP_REPOSITORY repos;
    T_BT_PBAP_PATH       path;

    APP_PRINT_TRACE1("PBAP demo app set phone book: param %s", TRACE_STRING(p_param));
    p_link = pbap_demo_find_link(remote_bd_addr);
    if (p_link != NULL)
    {
        if (strstr(p_param, "local"))
        {
            repos = BT_PBAP_REPOSITORY_LOCAL;
        }
        else if (strstr(p_param, "sim1"))
        {
            repos = BT_PBAP_REPOSITORY_SIM1;
        }
        else
        {
            repos = BT_PBAP_REPOSITORY_LOCAL;
        }

        if (strstr(p_param, "root"))
        {
            path = BT_PBAP_PATH_ROOT;
        }
        else if (strstr(p_param, "telecom"))
        {
            path = BT_PBAP_PATH_TELECOM;
        }
        else if (strstr(p_param, "pb"))
        {
            path = BT_PBAP_PATH_PB;
        }
        else if (strstr(p_param, "ich"))
        {
            path = BT_PBAP_PATH_ICH;
        }
        else if (strstr(p_param, "och"))
        {
            path = BT_PBAP_PATH_OCH;
        }
        else if (strstr(p_param, "mch"))
        {
            path = BT_PBAP_PATH_MCH;
        }
        else if (strstr(p_param, "cch"))
        {
            path = BT_PBAP_PATH_CCH;
        }
        else
        {
            path = BT_PBAP_PATH_ROOT;
        }

        APP_PRINT_TRACE2("PBAP demo app set phone book: repos %d path %d", repos, path);
        return bt_pbap_phone_book_set(remote_bd_addr, repos, path);
    }

    return false;
}

/**
 * @brief     PBAP Tx function for test.
 * @param[in] Remote caller id number.
 * @return    void
 */
bool pbap_demo_app_pull_caller_id_name(char *p_number)
{
    T_PBAP_DEMO_LINK *p_link;

    APP_PRINT_TRACE0("PBAP demo app pull caller id name");
    p_link = pbap_demo_find_link(remote_bd_addr);
    if (p_link != NULL)
    {
        return bt_pbap_vcard_listing_by_number_pull(remote_bd_addr, p_number);
    }

    return false;
}

/**
 * @brief    BT Manager PBAP related events are handled in this function
 * @note     Then the event handling function shall be called according to the
 *           event_type of T_BT_EVENT.
 * @param[in] event_type BT manager event type
 * @param[in] event_buf  Pointer to BT manager event buffer.
 * @param[in] buf_len    BT manager event buffer length.
 * @return   void
 */
static void pbap_demo_app_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_PBAP_DEMO_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_PBAP_CONN_CMPL:
        {
            p_link = pbap_demo_find_link(param->pbap_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                memcpy(remote_bd_addr, param->pbap_conn_cmpl.bd_addr, 6);
                char *temp_buff = "PBAP Connected!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    case BT_EVENT_PBAP_CONN_FAIL:
        break;

    case BT_EVENT_PBAP_DISCONN_CMPL:
        {
            p_link = pbap_demo_find_link(param->pbap_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                char *temp_buff = "PBAP Disconnected!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    case BT_EVENT_PBAP_SET_PHONE_BOOK_CMPL:
        {
            p_link = pbap_demo_find_link(param->pbap_set_phone_book_cmpl.bd_addr);
            if (p_link != NULL)
            {
                char *temp_buff = "PBAP set phone book complete!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));
            }
        }
        break;

    case BT_EVENT_PBAP_CALLER_ID_NAME:
        {
            p_link = pbap_demo_find_link(param->pbap_caller_id_name.bd_addr);
            if (p_link != NULL)
            {
                if (param->pbap_caller_id_name.name_len > 0)
                {
                    console_write((uint8_t *)param->pbap_caller_id_name.name_ptr, param->pbap_caller_id_name.name_len);
                }
                else
                {
                    char *temp_buff = "PBAP pull caller id name failed!\r\n";
                    console_write((uint8_t *)temp_buff, strlen(temp_buff));
                }
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("pbap_demo_app_bt_cback: event_type 0x%04x", event_type);
    }
}

void pbap_demo_app_init(void)
{
    APP_PRINT_INFO0("pbap_demo_app_init");

    bt_pbap_init();

    bt_mgr_cback_register(pbap_demo_app_bt_cback);
}

