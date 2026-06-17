/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "trace.h"
#include "gatt.h"
#include "data_uart.h"
#include "ble_ext_adv.h"
#include "app_lea_cap_acc_adv.h"
#include "app_lea_cap_acc_main.h"
#include "csis_def.h"
#include "csis_rsi.h"
#include "ascs_def.h"
#include "cap.h"
#include "bass_def.h"

#define LE_EXT_TEST_ADV_DATA_LEN                    100

uint8_t app_ext_adv_handle;

uint16_t le_ext_adv_len = 3;
uint16_t le_ext_adv_flag = 0;

uint8_t ext_adv_data[LE_EXT_TEST_ADV_DATA_LEN] =
{
    /* Flags */
    0x02,             /* length */
    GAP_ADTYPE_FLAGS, /* type="Flags" */
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
    //GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_SIMULTANEOUS_LE_BREDR_CONTROLLER |GAP_ADTYPE_FLAGS_SIMULTANEOUS_LE_BREDR_HOST,
    /* Local name */
    0x07,             /* length */
    GAP_ADTYPE_SERVICE_DATA,
    LO_WORD(GATT_UUID_ASCS),
    HI_WORD(GATT_UUID_ASCS),
    0, 0, 0, 0
};

const uint8_t scan_response_data[] =
{
    /* Local name */
    0x0A,
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B', 'L', 'E', '_', 'A', 'u', 'd', 'i', 'o',
    0x03,
    GAP_ADTYPE_APPEARANCE,
    LO_WORD(GAP_GATT_APPEARANCE_GENERIC_MEDIA_PLAYER),
    HI_WORD(GAP_GATT_APPEARANCE_GENERIC_MEDIA_PLAYER),
};

#if CSIP_SET_MEMBER
static void app_ext_adv_gen_rsi(uint8_t *p_sirk)
{
    uint8_t psri_data[CSI_RSI_LEN];
    if (csis_gen_rsi(p_sirk, psri_data))
    {
        uint16_t idx = le_ext_adv_len;
        ext_adv_data[idx] = CSI_RSI_LEN + 1;
        idx++;
        ext_adv_data[idx] = GAP_ADTYPE_RSI;
        idx++;
        memcpy(ext_adv_data + idx, psri_data, CSI_RSI_LEN);
        idx += CSI_RSI_LEN;
        le_ext_adv_len = idx;
    }
}
#endif

static bool app_ext_adv_gen_ascs_data(uint8_t *p_data, uint8_t *p_buf_len, uint8_t announcement,
                                      uint8_t meta_data_len, const uint8_t *p_meta_data)
{
    uint8_t idx = 0;
    if (p_data == NULL || p_buf_len == NULL)
    {
        return false;
    }
    if (*p_buf_len < (9 + meta_data_len) ||
        (meta_data_len > 0 && p_meta_data == NULL))
    {
        return false;
    }
    p_data[idx++] = 9 + meta_data_len;
    p_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
    p_data[idx++] = LO_WORD(GATT_UUID_ASCS);
    p_data[idx++] = HI_WORD(GATT_UUID_ASCS);
    p_data[idx++] = announcement;
    LE_UINT16_TO_ARRAY(p_data + idx,
                       app_db.lea_sink_available_contexts);
    idx += 2;
    LE_UINT16_TO_ARRAY(p_data + idx,
                       app_db.lea_source_available_contexts);
    idx += 2;
    p_data[idx++] = meta_data_len;
    if (meta_data_len > 0)
    {
        memcpy(p_data + idx, p_meta_data, meta_data_len);
        idx += meta_data_len;
    }
    *p_buf_len = idx;
    return true;
}

/*Unicast Server that sent the PDU is available for a general audio use case.
In this specification, a general audio use case means the transmission or reception of Audio Data
that has not been initiated by a higher-layer specification.*/
static void app_ext_adv_data_general_audio(void)
{
    uint16_t idx = le_ext_adv_len;
    APP_PRINT_TRACE1("app_ext_adv_data_general_audio: le_ext_adv_flag 0x%x", le_ext_adv_flag);

    if (le_ext_adv_flag & LE_EXT_ADV_ASCS)
    {
        uint8_t buf_len = LE_EXT_TEST_ADV_DATA_LEN - idx;
        if (app_ext_adv_gen_ascs_data(&ext_adv_data[idx], &buf_len, ADV_GENERAL_ANNOUNCEMENT,
                                      0, NULL))
        {
            idx += buf_len;
        }
    }

    if ((le_ext_adv_flag & LE_EXT_ADV_CAP_GEN) ||
        (le_ext_adv_flag & LE_EXT_ADV_CAP_TARGET))
    {
        ext_adv_data[idx] = 0x04;
        idx++;
        ext_adv_data[idx] = GAP_ADTYPE_SERVICE_DATA;
        idx++;
        ext_adv_data[idx] = LO_WORD(GATT_UUID_CAS);
        idx++;
        ext_adv_data[idx] = HI_WORD(GATT_UUID_CAS);
        idx++;
        if (le_ext_adv_flag & LE_EXT_ADV_CAP_GEN)
        {
            ext_adv_data[idx] = ADV_GENERAL_ANNOUNCEMENT;
            idx++;
        }
        else
        {
            ext_adv_data[idx] = ADV_TARGETED_ANNOUNCEMENT;
            idx++;
        }
    }
#if BAP_SCAN_DELEGATOR
    if (le_ext_adv_flag & LE_EXT_ADV_BASS)
    {
        ext_adv_data[idx] = 0x03;
        idx++;
        ext_adv_data[idx] = GAP_ADTYPE_SERVICE_DATA;
        idx++;
        ext_adv_data[idx] = LO_WORD(GATT_UUID_BASS);
        idx++;
        ext_adv_data[idx] = HI_WORD(GATT_UUID_BASS);
        idx++;
    }
#endif
    le_ext_adv_len = idx;
#if CSIP_SET_MEMBER
    if (le_ext_adv_flag & LE_EXT_ADV_PSRI)
    {
        app_ext_adv_gen_rsi((uint8_t *)app_db.lea_csis_sirk);
    }
#endif

    memcpy(&ext_adv_data[le_ext_adv_len], scan_response_data, sizeof(scan_response_data));
    le_ext_adv_len += sizeof(scan_response_data);
}

void app_ext_adv_adv_cfg(uint16_t flag)
{
    le_ext_adv_flag |= flag;
}

void app_ext_adv_update(void)
{
    app_ext_adv_data_general_audio();
    ble_ext_adv_mgr_set_adv_data(app_ext_adv_handle, le_ext_adv_len, ext_adv_data);
}

T_GAP_CAUSE app_ext_adv_start(void)
{
    return ble_ext_adv_mgr_enable(app_ext_adv_handle, 0);
}

T_GAP_CAUSE app_ext_adv_stop(void)
{
    return ble_ext_adv_mgr_disable(app_ext_adv_handle, 0);
}

static void app_ext_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));
    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            APP_PRINT_TRACE4("ble_adv_data_callback: BLE_EXT_ADV_STATE_CHANGE, adv_handle %d, state %d, stop_cause %d, app_cause %d",
                             cb_data.p_ble_state_change->adv_handle,
                             cb_data.p_ble_state_change->state,
                             cb_data.p_ble_state_change->stop_cause,
                             cb_data.p_ble_state_change->app_cause);
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        APP_PRINT_TRACE1("app_ble_common_adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x",
                         cb_data.p_ble_conn_info->conn_id);
        break;

    default:
        break;
    }
    return;
}

bool app_ext_adv_params_init(void)
{
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_CONN_UNDIRECTED;
    uint32_t primary_adv_interval_min = DEFAULT_ADVERTISING_INTERVAL_MIN;
    uint32_t primary_adv_interval_max = DEFAULT_ADVERTISING_INTERVAL_MAX;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t peer_address[6] = {0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;

    app_ext_adv_data_general_audio();

    if (ble_ext_adv_mgr_init_adv_params(&app_ext_adv_handle, adv_event_prop, primary_adv_interval_min,
                                        primary_adv_interval_max, own_address_type, peer_address_type, peer_address,
                                        filter_policy, le_ext_adv_len, ext_adv_data,
                                        0, NULL, NULL) == GAP_CAUSE_SUCCESS)
    {
        ble_ext_adv_mgr_register_callback(app_ext_adv_callback, app_ext_adv_handle);
        return true;
    }
    return false;
}

