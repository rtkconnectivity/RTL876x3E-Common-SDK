/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_BT_HID_HOST_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "btm.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "bt_hid_host.h"
#include "bt_hid_parser.h"
#include "app_link_util_cs.h"
#include "app_hid_host.h"
#include "app_main.h"
#if F_APP_MULTI_2EP_1_BR_1_BLE_HID
#include "app_tri_dongle_mgr.h"
#include "app_tri_dongle_usb_hid_ctrl.h"
#endif

static const T_BT_SDP_UUID_DATA hid_service_uuid16 =
{
    .uuid_16 = UUID_HUMAN_INTERFACE_DEVICE_SERVICE
};

static bool app_hid_host_connect(uint8_t *bd_addr)
{
    return bt_sdp_discov_start(bd_addr, BT_SDP_UUID16, hid_service_uuid16);
}

static bool app_hid_host_disconnect(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return bt_hid_host_disconnect_req(bd_addr);
    }
    return false;
}

void app_hid_host_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                 uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE3("app_hid_host_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);

    switch (cmd_id)
    {
    case CMD_HID_HOST_CONNECT:
        {
            uint8_t link_id = cmd_ptr[2];
            app_hid_host_connect(app_db.br_link[link_id].bd_addr);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_HID_HOST_DISCONNECT:
        {
            uint8_t link_id = cmd_ptr[2];
            app_hid_host_disconnect(app_db.br_link[link_id].bd_addr);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        break;
    }
}

#define CHAR_ILLEGAL     0xff
#define CHAR_RETURN     '\n'
#define CHAR_ESCAPE      27
#define CHAR_TAB         '\t'
#define CHAR_BACKSPACE   0x7f

static const uint8_t keytable_us_none[] =
{
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /*   0-3 */
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',                   /*  4-13 */
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',                   /* 14-23 */
    'u', 'v', 'w', 'x', 'y', 'z',                                       /* 24-29 */
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',                   /* 30-39 */
    CHAR_RETURN, CHAR_ESCAPE, CHAR_BACKSPACE, CHAR_TAB, ' ',            /* 40-44 */
    '-', '=', '[', ']', '\\', CHAR_ILLEGAL, ';', '\'', 0x60, ',',       /* 45-54 */
    '.', '/', CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,   /* 55-60 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 61-64 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 65-68 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 69-72 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 73-76 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 77-80 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 81-84 */
    '*', '-', '+', '\n', '1', '2', '3', '4', '5',                       /* 85-97 */
    '6', '7', '8', '9', '0', '.', 0xa7,                                 /* 97-100 */
};

static const uint8_t keytable_us_shift[] =
{
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /*  0-3  */
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',                   /*  4-13 */
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',                   /* 14-23 */
    'U', 'V', 'W', 'X', 'Y', 'Z',                                       /* 24-29 */
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',                   /* 30-39 */
    CHAR_RETURN, CHAR_ESCAPE, CHAR_BACKSPACE, CHAR_TAB, ' ',            /* 40-44 */
    '_', '+', '{', '}', '|', CHAR_ILLEGAL, ':', '"', 0x7E, '<',         /* 45-54 */
    '>', '?', CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,   /* 55-60 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 61-64 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 65-68 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 69-72 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 73-76 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 77-80 */
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL,             /* 81-84 */
    '*', '-', '+', '\n', '1', '2', '3', '4', '5',                       /* 85-97 */
    '6', '7', '8', '9', '0', '.', 0xb1,                                 /* 97-100 */
};

typedef struct t_hid_host_descriptor
{
    struct t_hid_host_descriptor *next;
    uint8_t                       bd_addr[6];
    uint16_t                      descriptor_len;
    uint8_t                       descriptor[0];
} T_HID_HOST_DESCRIPTOR;

T_OS_QUEUE descriptor_list;

T_HID_HOST_DESCRIPTOR *app_hid_host_alloc_descriptor(uint8_t   bd_addr[6],
                                                     uint8_t  *descriptor,
                                                     uint16_t  descriptor_len)
{
    T_HID_HOST_DESCRIPTOR *hid_descriptor;

    hid_descriptor = malloc(sizeof(T_HID_HOST_DESCRIPTOR) + descriptor_len);
    if (hid_descriptor != NULL)
    {
        hid_descriptor->descriptor_len = descriptor_len;
        memcpy(hid_descriptor->descriptor, descriptor, descriptor_len);
        memcpy(hid_descriptor->bd_addr, bd_addr, 6);
        os_queue_in(&descriptor_list, hid_descriptor);
    }

    return hid_descriptor;
}

T_HID_HOST_DESCRIPTOR *app_hid_host_descriptor_find(uint8_t *bd_addr)
{
    T_HID_HOST_DESCRIPTOR *hid_descriptor;

    hid_descriptor = os_queue_peek(&descriptor_list, 0);
    while (hid_descriptor != NULL)
    {
        if (!memcmp(hid_descriptor->bd_addr, bd_addr, 6))
        {
            break;
        }

        hid_descriptor = hid_descriptor->next;
    }

    return hid_descriptor;
}

void app_hid_host_free_descriptor(T_HID_HOST_DESCRIPTOR *hid_descriptor)
{
    if (os_queue_delete(&descriptor_list, hid_descriptor) == true)
    {
        free(hid_descriptor);
    }
}

#define NUM_KEYS 6
static uint8_t last_keys[NUM_KEYS];
#if (CONFIG_REALTEK_BTM_HID_HOST_SUPPORT == 1)
void app_hid_host_handle_interrupt_report(T_HID_HOST_DESCRIPTOR *hid_descriptor,
                                          const uint8_t         *report,
                                          uint16_t               report_len)
{
    T_BT_HID_PARSER parser;
    int             shift = 0;
    uint8_t         new_keys[NUM_KEYS];
    int             new_keys_count = 0;

    memset(new_keys, 0, sizeof(new_keys));
    bt_hid_parser_init(&parser,
                       hid_descriptor->descriptor,
                       hid_descriptor->descriptor_len,
                       BT_HID_REPORT_TYPE_INPUT,
                       report,
                       report_len);
    while (bt_hid_parser_has_more(&parser))
    {
        uint16_t usage_page;
        uint16_t usage;
        int32_t  value;

        bt_hid_parser_access_report_field(&parser,
                                          &usage_page,
                                          &usage,
                                          &value);
        if (usage_page != 0x07) { continue; }
        switch (usage)
        {
        case 0xe1:
        case 0xe6:
            if (value)
            {
                shift = 1;
            }
            continue;
        case 0x00:
            continue;
        default:
            break;
        }
        if (usage >= sizeof(keytable_us_none)) { continue; }

        // store new keys
        new_keys[new_keys_count++] = (uint8_t) usage;

        // check if usage was used last time (and ignore in that case)
        int i;
        for (i = 0; i < NUM_KEYS; i++)
        {
            if (usage == last_keys[i])
            {
                usage = 0;
            }
        }
        if (usage == 0) { continue; }

        uint8_t key;
        if (shift)
        {
            key = keytable_us_shift[usage];
        }
        else
        {
            key = keytable_us_none[usage];
        }
        if (key == CHAR_ILLEGAL) { continue; }
        if (key == CHAR_BACKSPACE)
        {
            // go back one char, print space, go back one char again
            APP_PRINT_INFO0("app_hid_host_handle_interrupt_report \b \b");
            continue;
        }
        APP_PRINT_INFO1("app_hid_host_handle_interrupt_report %c", key);
    }
    memcpy(last_keys, new_keys, NUM_KEYS);
}
#endif

static void app_hid_host_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link = NULL;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_SDP_ATTR_INFO:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->sdp_attr_info.bd_addr);

            if (p_link != NULL)
            {
                T_BT_SDP_ATTR_INFO *sdp_info = &param->sdp_attr_info.info;

                if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_HUMAN_INTERFACE_DEVICE_SERVICE)
                {
                    if (app_hid_host_descriptor_find(param->sdp_attr_info.bd_addr) == NULL)
                    {
                        uint8_t  *p_attr_start = sdp_info->p_attr;
                        uint8_t  *p_attr_end = p_attr_start + sdp_info->attr_len;
                        uint8_t  *p_attr = NULL;
                        uint8_t  *p_attr_param = NULL;
                        uint8_t  *p_elem = NULL;
                        uint8_t  *descriptor = NULL;
                        uint32_t  descriptor_len = 0;
                        uint8_t   type = 0;

                        p_attr = bt_sdp_attr_find(p_attr_start, p_attr_end - p_attr_start, SDP_ATTR_HID_DESC_LIST);
                        if (p_attr)
                        {
                            uint8_t loop = 1;

                            while ((p_elem = bt_sdp_elem_access(p_attr, p_attr_end - p_attr, loop)) != NULL)
                            {
                                p_attr_param = bt_sdp_elem_access(p_elem, p_attr_end - p_elem, 2);

                                if (p_attr_param)
                                {
                                    descriptor = bt_sdp_elem_decode(p_attr_param, p_attr_end - p_attr_param, &descriptor_len, &type);
                                    break;
                                }
                                loop++;
                            }
                        }
                        if (descriptor)
                        {
                            T_HID_HOST_DESCRIPTOR *hid_descriptor;

                            hid_descriptor = app_hid_host_alloc_descriptor(param->sdp_attr_info.bd_addr,
                                                                           descriptor,
                                                                           descriptor_len);
                            if (HID_PROFILE_MASK & p_link->connected_profile)
                            {
                                bt_hid_host_descriptor_set(param->sdp_attr_info.bd_addr,
                                                           hid_descriptor->descriptor,
                                                           hid_descriptor->descriptor_len);
                            }
                        }
                    }
                }
            }
        }
        break;

    case BT_EVENT_HID_HOST_CONN_IND:
        {
            p_link = app_link_find_br_link(param->hid_host_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_hid_host_connect_cfm(p_link->bd_addr, true);
            }
        }
        break;

    case BT_EVENT_HID_HOST_CONN_CMPL:
        {
            T_HID_HOST_DESCRIPTOR *hid_descriptor;

            hid_descriptor = app_hid_host_descriptor_find(param->hid_host_conn_cmpl.bd_addr);
            if (hid_descriptor != NULL)
            {
                bt_hid_host_descriptor_set(param->hid_host_conn_cmpl.bd_addr,
                                           hid_descriptor->descriptor,
                                           hid_descriptor->descriptor_len);
#if F_APP_MULTI_2EP_1_BR_1_BLE_HID
                app_tri_dongle_usb_hid_recfg_br_report_desc(param->hid_host_conn_cmpl.bd_addr,
                                                            hid_descriptor->descriptor,
                                                            hid_descriptor->descriptor_len);
                app_tri_dongle_mgr_target_profle(APP_TRI_DONGLE_CONN_PROFILE_READY, HID_PROFILE_MASK);

                app_tri_dongle_set_br_hid_device(true, param->hid_host_conn_cmpl.bd_addr);
#endif
            }
            else
            {
                bt_sdp_discov_start(param->hid_host_conn_cmpl.bd_addr, BT_SDP_UUID16, hid_service_uuid16);
            }
        }
        break;

    case BT_EVENT_HID_HOST_CONN_FAIL:
        {
            T_HID_HOST_DESCRIPTOR *hid_descriptor;

            hid_descriptor = app_hid_host_descriptor_find(param->hid_host_conn_fail.bd_addr);
            if (hid_descriptor != NULL)
            {
                app_hid_host_free_descriptor(hid_descriptor);
            }
        }
        break;

    case BT_EVENT_HID_HOST_DISCONN_CMPL:
        {
            p_link = app_link_find_br_link(param->hid_host_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (param->hid_host_disconn_cmpl.virtual_unplug)
                {
                    T_HID_HOST_DESCRIPTOR *hid_descriptor;

                    hid_descriptor = app_hid_host_descriptor_find(param->hid_host_disconn_cmpl.bd_addr);
                    if (hid_descriptor != NULL)
                    {
                        app_hid_host_free_descriptor(hid_descriptor);
#if F_APP_MULTI_2EP_1_BR_1_BLE_HID
                        app_tri_dongle_inreport_br_release(p_link);
                        app_tri_dongle_set_br_hid_device(false, param->hid_host_disconn_cmpl.bd_addr);
#endif
                    }
                }
            }
        }
        break;

    case BT_EVENT_HID_HOST_HID_CONTROL_IND:
        {

        }
        break;

    case BT_EVENT_HID_HOST_INTERRUPT_DATA_IND:
        {
            p_link = app_link_find_br_link(param->hid_host_interrupt_data_ind.bd_addr);
            if (p_link != NULL)
            {
                uint16_t               report_size;
                uint8_t                report_type;
                uint16_t               report_len;
                uint8_t               *report;
                T_HID_HOST_DESCRIPTOR *hid_descriptor;

                report_size = param->hid_host_interrupt_data_ind.report_size;
                report_type = (T_BT_HID_HOST_REPORT_TYPE)param->hid_host_interrupt_data_ind.report_type;
                hid_descriptor = app_hid_host_descriptor_find(param->hid_host_interrupt_data_ind.bd_addr);
                if (hid_descriptor != NULL)
                {
                    if (param->hid_host_interrupt_data_ind.report_id != 0)
                    {
                        report_len = 1 + report_size;
                        report = calloc(1, report_len);
                        report[0] = param->hid_host_interrupt_data_ind.report_id;
                        memcpy(&report[1], param->hid_host_interrupt_data_ind.p_data, report_size);
                    }
                    else
                    {
                        report_len = report_size;
                        report = calloc(1, report_len);
                        memcpy(report, param->hid_host_interrupt_data_ind.p_data, report_size);
                    }

#if (F_APP_MULTI_2EP_1_BR_1_BLE_HID == 0)
                    if (report_type == BT_HID_HOST_REPORT_TYPE_INPUT)
                    {
#if (CONFIG_REALTEK_BTM_HID_HOST_SUPPORT == 1)

                        app_hid_host_handle_interrupt_report(hid_descriptor, report, report_len);
#endif
                    }
#else
                    if (report_type == BT_HID_HOST_REPORT_TYPE_INPUT)
                    {

                        app_tri_dongle_usb_br_hid_inreport_handle(param->hid_host_interrupt_data_ind.bd_addr, report,
                                                                  report_len, param->hid_host_interrupt_data_ind.report_id);
                    }
                    else if (report_type == BT_HID_HOST_REPORT_TYPE_FEATURE)
                    {
                        app_tri_dongle_usb_hid_read_br_feature_handle(param->hid_host_interrupt_data_ind.bd_addr, report,
                                                                      report_len, param->hid_host_interrupt_data_ind.report_id);
                    }
#endif

                    free(report);
                }
            }
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_hid_host_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_hid_host_init(void)
{
#if (F_APP_MULTI_2EP_1_BR_1_BLE_HID == 0)
    if (app_cfg_const.supported_profile_mask & HID_PROFILE_MASK)
#endif
    {
        bt_hid_host_init(true);
        os_queue_init(&descriptor_list);
        bt_mgr_cback_register(app_hid_host_bt_cback);
    }
}
#endif
