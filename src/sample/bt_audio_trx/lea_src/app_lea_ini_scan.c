/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include "trace.h"
#include "app_lea_ini_scan.h"
#include "ble_scan.h"
#include "app_report.h"

#include "app_cmd.h"
#include "ascs_def.h"
#include "bass_def.h"
#include "csis_rsi.h"
#include "cap.h"
#include "csis_client.h"
#include "app_main.h"
#include "string.h"
#include "stdlib.h"
#if BAP_BROADCAST_ASSISTANT
#include "gap_ext_adv.h"
#endif
#if F_APP_GUI_SUPPORT
#include "app_panel_le_db.h"
#endif
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
#include "app_tri_dongle_cmd.h"
#endif

typedef enum
{
    LEA_GROUP_SCAN_STOPPED,
    LEA_GROUP_SCAN_STARTED
} T_LEA_GROUP_SCAN_STATE;

static BLE_SCAN_HDL app_lea_ini_scan_handle = NULL;

T_APP_LEA_SCAN_DEV *app_lea_find_scan_dev(uint8_t *bd_addr, uint8_t bd_type)
{
    T_APP_LEA_SCAN_DEV *p_dev;
    for (uint8_t i = 0; i < app_db.scan_dev_queue.count; i++)
    {
        p_dev = (T_APP_LEA_SCAN_DEV *)os_queue_peek(&app_db.scan_dev_queue, i);
        if (p_dev != NULL && p_dev->bd_type == bd_type &&
            memcmp(bd_addr, p_dev->bd_addr, GAP_BD_ADDR_LEN) == 0)
        {
            return p_dev;
        }
    }
    return NULL;
}

T_APP_LEA_SCAN_DEV *app_lea_add_scan_dev(uint8_t *bd_addr, uint8_t bd_type,
                                         T_APP_LEA_ADV_DATA *p_lea_data)
{
    T_APP_LEA_SCAN_DEV *p_dev;

    p_dev = app_lea_find_scan_dev(bd_addr, bd_type);
    if (p_dev)
    {
        return p_dev;
    }
    p_dev = calloc(1, sizeof(T_APP_LEA_SCAN_DEV));
    if (p_dev)
    {
        memcpy(p_dev->bd_addr, bd_addr, GAP_BD_ADDR_LEN);
        p_dev->bd_type = bd_type;
        memcpy(&p_dev->adv_data, p_lea_data, sizeof(T_APP_LEA_ADV_DATA));
        os_queue_in(&app_db.scan_dev_queue, p_dev);
    }
    return p_dev;
}

void app_lea_clear_scan_dev(void)
{
    T_APP_LEA_SCAN_DEV *p_dev;
    while ((p_dev = (T_APP_LEA_SCAN_DEV *)os_queue_out(&app_db.scan_dev_queue)) != NULL)
    {
        free(p_dev);
    }
}

static void app_lea_ini_scan_data_parse(uint8_t report_data_len, uint8_t *p_report_data,
                                        T_APP_LEA_ADV_DATA *p_lea_data)
{
    uint8_t *p_buffer;
    uint16_t pos = 0;

    while (pos < report_data_len)
    {
        /* Length of the AD structure. */
        uint16_t length = p_report_data[pos++];
        uint8_t type;

        if (length < 1)
        {
            return;
        }

        if ((length > 0x01) && ((pos + length) <= report_data_len))
        {
            /* Copy the AD Data to buffer. */
            p_buffer = p_report_data + pos + 1;
            /* AD Type, one octet. */
            type = p_report_data[pos];

            switch (type)
            {
            case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                {
                    p_lea_data->dev_name_len = length - 1;
                    uint8_t *p_name = p_buffer;
                    if (p_lea_data->dev_name_len >= GAP_DEVICE_NAME_LEN)
                    {
                        p_lea_data->dev_name_len = GAP_DEVICE_NAME_LEN - 1;
                    }
                    memcpy(p_lea_data->dev_name, p_name, p_lea_data->dev_name_len);
                }
                break;
            case GAP_ADTYPE_SERVICE_DATA:
                {
                    uint16_t uuid;
                    LE_STREAM_TO_UINT16(uuid, p_buffer);
                    if (uuid == GATT_UUID_ASCS)
                    {
                        p_lea_data->adv_data_flags |= APP_LEA_ADV_DATA_ASCS_BIT;
                        LE_STREAM_TO_UINT8(p_lea_data->ascs_announcement_type, p_buffer);
                        LE_STREAM_TO_UINT16(p_lea_data->ascs_sink_available_contexts, p_buffer);
                        LE_STREAM_TO_UINT16(p_lea_data->ascs_source_available_contexts, p_buffer);
                    }
                    else if (uuid == GATT_UUID_CAS)
                    {
                        p_lea_data->adv_data_flags |= APP_LEA_ADV_DATA_CAP_BIT;
                        LE_STREAM_TO_UINT8(p_lea_data->cap_announcement_type, p_buffer);
                    }
                    else if (uuid == GATT_UUID_BASS)
                    {
                        p_lea_data->adv_data_flags |= APP_LEA_ADV_DATA_BASS_BIT;
                    }
                }
                break;

            case GAP_ADTYPE_RSI:
                {
                    p_lea_data->adv_data_flags |= APP_LEA_ADV_DATA_RSI_BIT;
                }
                break;

            default:
                break;
            }
        }

        pos += length;
    }
}

static void app_lea_ini_scan_report_result(uint16_t event_type, uint8_t *bd_addr, uint8_t addr_type,
                                           T_APP_LEA_ADV_DATA *p_lea_data)
{
    struct
    {
        uint16_t    event_type;
        uint8_t     addr_type;
        uint8_t     bd_addr[6];
        uint8_t     name_len;
        uint8_t     name[GAP_DEVICE_NAME_LEN];
    } __attribute__((packed)) rpt = {};


    rpt.event_type = event_type;
    rpt.addr_type = addr_type;
    memcpy(rpt.bd_addr, bd_addr, 6);
    rpt.name_len = p_lea_data->dev_name_len;
    if (rpt.name_len >= GAP_DEVICE_NAME_LEN)
    {
        rpt.name_len = GAP_DEVICE_NAME_LEN - 1;
    }
    memcpy(rpt.name, &(p_lea_data->dev_name), rpt.name_len);
    app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_SCAN_INFO, 0, (uint8_t *)&rpt, sizeof(rpt));
}

void app_lea_ini_scan_report(T_LE_EXT_ADV_REPORT_INFO *p_report)
{
    T_APP_LEA_ADV_DATA lea_data = {0};
    T_APP_LEA_SCAN_DEV *p_dev;
    APP_PRINT_INFO3("app_lea_ini_scan_report: event_type 0x%x, bd_addr %s, addr_type %d",
                    p_report->event_type,
                    TRACE_BDADDR(p_report->bd_addr),
                    p_report->addr_type);
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
    if (app_tri_dongle_filter_ble_disc(p_report->bd_addr))
    {
        APP_PRINT_TRACE0("ever use cmd disc lea ini");
        return;
    }
#endif
    p_dev = app_lea_find_scan_dev(p_report->bd_addr, p_report->addr_type);

    if (p_dev)
    {
        return;
    }
    app_lea_ini_scan_data_parse(p_report->data_len, p_report->p_data, &lea_data);
    if (lea_data.adv_data_flags)
    {

        APP_PRINT_INFO5("app_lea_ini_scan_report: adv_data_flags 0x%x, ascs(type %d, sink 0x%x, source 0x%x), cap(type %d)",
                        lea_data.adv_data_flags,
                        lea_data.ascs_announcement_type,
                        lea_data.ascs_sink_available_contexts,
                        lea_data.ascs_source_available_contexts,
                        lea_data.cap_announcement_type);
        app_lea_add_scan_dev(p_report->bd_addr, p_report->addr_type, &lea_data);
        app_lea_ini_scan_report_result(p_report->event_type,
                                       p_report->bd_addr,
                                       p_report->addr_type,
                                       &lea_data);
#if CSIP_SET_COORDINATOR
        if (lea_data.adv_data_flags & APP_LEA_ADV_DATA_RSI_BIT)
        {
            set_coordinator_check_adv_rsi(p_report->data_len, p_report->p_data,
                                          p_report->bd_addr, p_report->addr_type);
        }
#endif
    }
}

void app_lea_ini_scan_cb(BLE_SCAN_EVT state, BLE_SCAN_EVT_DATA *data)
{
    switch (state)
    {
    case BLE_SCAN_REPORT:
        app_lea_ini_scan_report(data->report);
        break;

    default:
        break;
    }
}

bool app_lea_ini_scan_get_state(void)
{
    return (app_lea_ini_scan_handle == NULL) ? false : true;
}

void app_lea_ini_scan_stop()
{
    APP_PRINT_INFO0("app_ble_scan_stop");
    ble_scan_stop(&app_lea_ini_scan_handle);
    app_lea_clear_scan_dev();
}

void app_lea_ini_scan_start(void)
{
    APP_PRINT_INFO0("app_ble_scan_start");
    BLE_SCAN_PARAM param;

    param.own_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    param.phys = GAP_EXT_SCAN_PHYS_1M_BIT;
    param.ext_filter_policy = GAP_SCAN_FILTER_ANY;
    param.ext_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

    param.scan_param_1m.scan_type = GAP_SCAN_MODE_PASSIVE;
    param.scan_param_1m.scan_interval = 0x140;
    param.scan_param_1m.scan_window = 0xD0;

    param.scan_param_coded.scan_type = GAP_SCAN_MODE_PASSIVE;
    param.scan_param_coded.scan_interval = 0x0050;
    param.scan_param_coded.scan_window = 0x0025;

    if (ble_scan_start(&app_lea_ini_scan_handle, app_lea_ini_scan_cb, &param, NULL))
    {
        app_lea_clear_scan_dev();
    }
}

static void app_lea_report_group_scan_state(T_BLE_AUDIO_GROUP_HANDLE group_handle,
                                            T_LEA_GROUP_SCAN_STATE state)
{
    struct
    {
        void *group_handle;
        uint8_t   state;
    } __attribute__((packed)) rpt =
    {
        .group_handle = group_handle,
        .state = state,
    };
    app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_CSIS_SCAN_STATE, 0, (uint8_t *)&rpt, sizeof(rpt));
}

bool app_lea_group_scan_start(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    app_lea_ini_scan_start();
    APP_PRINT_INFO1("app_lea_group_scan_start: group_handle %p", group_handle);
    if (set_coordinator_cfg_discover(group_handle, true, 30000))
    {
        app_lea_report_group_scan_state(group_handle, LEA_GROUP_SCAN_STARTED);
        return true;
    }
    return false;
}

bool app_lea_group_scan_stop(void)
{
    APP_PRINT_INFO0("app_lea_group_scan_stop");
    app_lea_ini_scan_stop();
    if (set_coordinator_cfg_discover(NULL, false, 0))
    {
        app_lea_report_group_scan_state(NULL, LEA_GROUP_SCAN_STOPPED);
        return true;
    }
    return false;
}

#if BAP_BROADCAST_ASSISTANT
static BLE_SCAN_HDL app_lea_baas_scan_handle = NULL;

static void app_lea_ini_baas_scan_report_result(uint16_t scan_baas_dev_count, uint8_t *bd_addr,
                                                uint8_t addr_type, T_APP_LEA_BAAS_DEV_INFO *p_lea_dev)
{
    struct
    {
        uint16_t    scan_baas_dev_count;
        uint8_t     addr_type;
        uint8_t     bd_addr[6];
        uint8_t     name_len;
        uint8_t     name[GAP_DEVICE_NAME_LEN];
    } __attribute__((packed)) rpt = {};


    rpt.scan_baas_dev_count = scan_baas_dev_count;
    rpt.addr_type = addr_type;
    memcpy(rpt.bd_addr, bd_addr, 6);
    rpt.name_len = p_lea_dev->dev_name_len;
    if (rpt.name_len >= GAP_DEVICE_NAME_LEN)
    {
        rpt.name_len = GAP_DEVICE_NAME_LEN - 1;
    }
    memcpy(rpt.name, &(p_lea_dev->dev_name), rpt.name_len);
#if F_APP_GUI_SUPPORT
    T_APP_GUI_LE_BASS_SCAN_RSP_DATA scan_rsp = {0};
    T_APP_LEA_BAAS_SCAN_DEV *p_dev = (T_APP_LEA_BAAS_SCAN_DEV *)os_queue_peek(
                                         &app_db.scan_baas_dev_queue,
                                         scan_baas_dev_count - 1);
    if (p_dev)
    {
        scan_rsp.index = scan_baas_dev_count - 1;
        scan_rsp.device_name = p_dev->dev_info.dev_name;
        app_panel_le_db_add_bass_scan_dev(&scan_rsp);
    }
#endif
    app_report_event(CMD_PATH_UART, EVENT_LE_AUDIO_BAAS_SCAN_INFO, 0, (uint8_t *)&rpt, sizeof(rpt));
}


T_APP_LEA_BAAS_SCAN_DEV *app_lea_find_baas_scan_dev(uint8_t *bd_addr, uint8_t bd_type)
{
    T_APP_LEA_BAAS_SCAN_DEV *p_dev;
    for (uint8_t i = 0; i < app_db.scan_baas_dev_queue.count; i++)
    {
        p_dev = (T_APP_LEA_BAAS_SCAN_DEV *)os_queue_peek(&app_db.scan_baas_dev_queue, i);
        if (p_dev != NULL && p_dev->dev_info.bd_type == bd_type &&
            memcmp(bd_addr, p_dev->dev_info.bd_addr, GAP_BD_ADDR_LEN) == 0)
        {
            return p_dev;
        }
    }
    return NULL;
}

T_APP_LEA_BAAS_SCAN_DEV *app_lea_add_baas_scan_dev(T_APP_LEA_BAAS_DEV_INFO *p_lea_data)
{
    T_APP_LEA_BAAS_SCAN_DEV *p_dev;

    p_dev = app_lea_find_baas_scan_dev(p_lea_data->bd_addr, p_lea_data->bd_type);
    if (p_dev)
    {
        return p_dev;
    }
    p_dev = calloc(1, sizeof(T_APP_LEA_BAAS_SCAN_DEV));
    if (p_dev)
    {
        memcpy(&p_dev->dev_info, p_lea_data, sizeof(T_APP_LEA_BAAS_DEV_INFO));
        APP_PRINT_INFO1("app_lea_add_baas_scan_dev: remotebd %d", app_db.scan_baas_dev_queue.count);
        os_queue_in(&app_db.scan_baas_dev_queue, p_dev);
    }
    return p_dev;
}

void app_lea_clear_baas_scan_dev(void)
{
    T_APP_LEA_BAAS_SCAN_DEV *p_dev;
    while ((p_dev = (T_APP_LEA_BAAS_SCAN_DEV *)os_queue_out(&app_db.scan_baas_dev_queue)) != NULL)
    {
        free(p_dev);
    }
}


static bool app_lea_scan_parse_baas_report(uint8_t report_data_len, uint8_t *p_report_data,
                                           T_APP_LEA_BAAS_DEV_INFO *p_lea_data)
{
    uint8_t *p_buffer;
    uint16_t pos = 0;
    bool ret = false;

    while (pos < report_data_len)
    {
        /* Length of the AD structure. */
        uint16_t length = p_report_data[pos++];
        uint8_t type;

        if (length < 1)
        {
            return false;
        }

        if ((length > 0x01) && ((pos + length) <= report_data_len))
        {
            /* Copy the AD Data to buffer. */
            p_buffer = p_report_data + pos + 1;
            /* AD Type, one octet. */
            type = p_report_data[pos];

            switch (type)
            {
            case GAP_ADTYPE_BROADCAST_NAME:
                {
                    p_lea_data->dev_name_len = length - 1;
                    uint8_t *p_name = p_buffer;
                    if (p_lea_data->dev_name_len >= GAP_DEVICE_NAME_LEN)
                    {
                        p_lea_data->dev_name_len = GAP_DEVICE_NAME_LEN - 1;
                    }
                    memcpy(p_lea_data->dev_name, p_name, p_lea_data->dev_name_len);
                }
                break;
            case GAP_ADTYPE_SERVICE_DATA:
                {
                    uint16_t uuid;
                    LE_STREAM_TO_UINT16(uuid, p_buffer);
                    if (uuid == BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID)
                    {
                        memcpy(p_lea_data->broadcast_id, p_buffer, 3);
                        ret = true;
                    }
                }
                break;

            default:
                break;
            }
        }

        pos += length;
    }
    return ret;
}

void app_lea_baas_scan_report(T_LE_EXT_ADV_REPORT_INFO *p_report)
{
    T_APP_LEA_BAAS_DEV_INFO lea_dev = {0};
    T_APP_LEA_BAAS_SCAN_DEV *p_dev;

    p_dev = app_lea_find_baas_scan_dev(p_report->bd_addr, p_report->addr_type);

    if (p_dev)
    {
        return;
    }
    if (app_lea_scan_parse_baas_report(p_report->data_len, p_report->p_data, &lea_dev))
    {
        lea_dev.adv_sid = p_report->adv_sid;
        memcpy(lea_dev.bd_addr, p_report->bd_addr, GAP_BD_ADDR_LEN);
        lea_dev.bd_type = p_report->addr_type;
        APP_PRINT_INFO4("app_lea_scan_report: event_type 0x%x, bd_addr %s, addr_type %d, adv_sid %d",
                        p_report->event_type,
                        TRACE_BDADDR(p_report->bd_addr),
                        p_report->addr_type,
                        p_report->adv_sid);

        app_lea_add_baas_scan_dev(&lea_dev);
        app_lea_ini_baas_scan_report_result(app_db.scan_baas_dev_queue.count, p_report->bd_addr,
                                            p_report->addr_type,
                                            &lea_dev);
    }
}

void app_lea_baas_scan_cb(BLE_SCAN_EVT state, BLE_SCAN_EVT_DATA *data)
{
    APP_PRINT_INFO1("app_lea_bass_scan_cb: state %d", state);

    switch (state)
    {
    case BLE_SCAN_REPORT:
        app_lea_baas_scan_report(data->report);
        break;

    default:
        break;
    }
}

void app_lea_baas_scan_start(void)
{
    APP_PRINT_INFO0("app_lea_baas_scan_start");

    if (app_lea_baas_scan_handle == NULL)
    {
        BLE_SCAN_PARAM param;
        BLE_SCAN_FILTER scan_filter = {0};

        /* Initialize extended scan parameters */
        param.own_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
        param.phys = GAP_EXT_SCAN_PHYS_1M_BIT;
        param.ext_filter_policy = GAP_SCAN_FILTER_ANY;
        param.ext_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

        param.scan_param_1m.scan_type = GAP_SCAN_MODE_PASSIVE;
        param.scan_param_1m.scan_interval = 0x140;
        param.scan_param_1m.scan_window = 0xD0;

        param.scan_param_coded.scan_type = GAP_SCAN_MODE_PASSIVE;
        param.scan_param_coded.scan_interval = 0x0050;
        param.scan_param_coded.scan_window = 0x0025;

        scan_filter.filter_flags = BLE_SCAN_FILTER_EVT_TYPE_BIT;
        scan_filter.evt_type = LE_EXT_ADV_EXTENDED_ADV_NON_SCAN_NON_CONN_UNDIRECTED;

        /* Enable extended scan */
        if (ble_scan_start(&app_lea_baas_scan_handle, app_lea_baas_scan_cb, &param, &scan_filter))
        {
            app_lea_clear_baas_scan_dev();
        }
    }
}

void app_lea_baas_scan_stop()
{
    APP_PRINT_INFO0("app_lea_baas_scan_stop");
    ble_scan_stop(&app_lea_baas_scan_handle);
    //app_lea_clear_baas_scan_dev();
}
#endif
#endif
