/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "gap_le_types.h"
#include "app_ble_common_adv.h"
#include "ble_ext_adv.h"
#include "app_main.h"
#include "app_ble_gap.h"
#include "app_ble_rand_addr_mgr.h"


typedef struct CB_ELEM CB_ELEM;

typedef struct CB_ELEM
{
    CB_ELEM             *p_next;
    LE_COMMON_ADV_CB    cb;
} CB_ELEM;

struct
{
    bool use_static_addr_type;
    uint8_t adv_handle;
    uint8_t conn_id;
    T_BLE_EXT_ADV_MGR_STATE state;
    T_OS_QUEUE cb_queue;
} le_common_adv =
{
#if F_APP_SC_KEY_DERIVE_SUPPORT
    .use_static_addr_type = false,
#else
    .use_static_addr_type = true,
#endif
    .adv_handle = 0xff,
    .conn_id = 0xff,
    .state = BLE_EXT_ADV_MGR_ADV_DISABLED,
    .cb_queue = {0}
};



static uint8_t app_ble_common_adv_data[31] =
{
    /* This modification is used to solve issue: BLE name shown on Android list */
    /* TX Power Level */
    0x02,/* length */
    GAP_ADTYPE_POWER_LEVEL,
    0x00, /* Don't care now */

    /* Service */
    17,/* length */
    GAP_ADTYPE_128BIT_COMPLETE,/* "Complete 128-bit UUIDs available"*/
    /*transmit service uuid*/
    0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFD, 0x02, 0x00,
    0x00,/*bud role:0x00 single,0x01 pri, 0x02 sec*/

    /* Manufacturer Specific Data */
    9,/* length */
    0xFF,
    0x5D, 0x00 /* Company ID */
    /*local address*/
};


static void app_ble_common_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));
    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            APP_PRINT_TRACE2("app_ble_common_adv_callback: adv_state %d, adv_handle %d",
                             cb_data.p_ble_state_change->state, cb_data.p_ble_state_change->adv_handle);
            le_common_adv.state = cb_data.p_ble_state_change->state;
            if (le_common_adv.state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
            }
            else
            {
                switch (cb_data.p_ble_state_change->stop_cause)
                {
                case BLE_EXT_ADV_STOP_CAUSE_APP:
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_CONN:
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_TIMEOUT:

                    break;

                default:
                    break;
                }
                APP_PRINT_TRACE2("app_ble_common_adv_callback: stack stop adv cause 0x%x, app stop adv cause 0x%02x",
                                 cb_data.p_ble_state_change->stop_cause, cb_data.p_ble_state_change->app_cause);
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        APP_PRINT_TRACE4("app_ble_common_adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x, adv_handle %d, local_addr_type %d, local_bd %b",
                         cb_data.p_ble_conn_info->conn_id,
                         cb_data.p_ble_conn_info->adv_handle,
                         cb_data.p_ble_conn_info->local_addr_type,
                         TRACE_BDADDR(cb_data.p_ble_conn_info->local_addr));
        break;

    default:
        break;
    }

    CB_ELEM *p_cb_elem = (CB_ELEM *)le_common_adv.cb_queue.p_first;
    APP_PRINT_TRACE1("app_ble_common_adv_callback: p_cb_elem %p", p_cb_elem);

    for (; p_cb_elem != NULL; p_cb_elem = p_cb_elem->p_next)
    {
        APP_PRINT_TRACE1("app_ble_common_adv_callback: cb %p", p_cb_elem->cb);
        p_cb_elem->cb(cb_type, cb_data);
    }

    return;
}

bool app_ble_common_adv_start(uint16_t duration_10ms)
{
    if (le_common_adv.state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        if (ble_ext_adv_mgr_enable(le_common_adv.adv_handle, duration_10ms) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        APP_PRINT_TRACE0("app_ble_common_adv_start: Already started");
        return true;
    }
}

bool app_ble_common_adv_stop(int8_t app_cause)
{
    APP_PRINT_INFO0("app_ble_common_adv_stop");
    if (ble_ext_adv_mgr_disable(le_common_adv.adv_handle, app_cause) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

T_BLE_EXT_ADV_MGR_STATE app_ble_common_adv_get_state(void)
{
    return le_common_adv.state;
}

uint8_t app_ble_common_adv_get_conn_id(void)
{
    return le_common_adv.conn_id;
}

void app_ble_common_adv_set_conn_id(uint8_t conn_id)
{
    le_common_adv.conn_id = conn_id;
}

void app_ble_common_adv_reset_conn_id(void)
{
    le_common_adv.conn_id = 0xff;
}

void app_ble_common_adv_name_refresh(void)
{
    app_ble_gap_gen_scan_rsp_data(&scan_rsp_data_len, scan_rsp_data);
    ble_ext_adv_mgr_set_scan_response_data(le_common_adv.adv_handle, scan_rsp_data_len, scan_rsp_data);
}

#if F_APP_GATT_OVER_BREDR_SUPPORT
extern uint8_t GATT_UUID128_RVDIS[16];
static void app_ble_common_adv_load_rvdis_uuid(uint8_t *data)
{
    memcpy(data, GATT_UUID128_RVDIS, sizeof(GATT_UUID128_RVDIS));
}
#endif

void app_ble_common_adv_init(uint8_t factory_addr[6])
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;//GAP_LOCAL_ADDR_LE_PUBLIC

    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    uint8_t data_len = 8;

    for (int i = 0; i < 6; i++)
    {
        app_ble_common_adv_data[25 + i] =  factory_addr[5 - i];
    }

#if F_APP_GATT_OVER_BREDR_SUPPORT
    app_ble_common_adv_load_rvdis_uuid(&app_ble_common_adv_data[5]);
#endif

    app_ble_gap_gen_scan_rsp_data(&scan_rsp_data_len, scan_rsp_data);

    uint8_t le_random_addr[6] = {0};

    app_ble_rand_addr_get(le_random_addr);

    ble_ext_adv_mgr_init_adv_params(&le_common_adv.adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, 23 + data_len, app_ble_common_adv_data,
                                    scan_rsp_data_len, scan_rsp_data, le_random_addr);

    APP_PRINT_TRACE2("app_ble_common_adv_init: app_ble_common_adv_data %b, le_random_addr %s",
                     TRACE_BINARY(sizeof(app_ble_common_adv_data), app_ble_common_adv_data),
                     TRACE_BDADDR(le_random_addr));

    ble_ext_adv_mgr_register_callback(app_ble_common_adv_callback, le_common_adv.adv_handle);

    os_queue_init(&le_common_adv.cb_queue);
}


