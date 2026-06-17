/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "ble_mgr.h"
#include "string.h"
#include "stdint.h"
#include "ble_scan.h"
#include "app_bt5_central_scan.h"
#include <app_bt5_central_link_mgr.h>

/** @defgroup  BT5_CENTRAL_APP BT5 Central Application
    * @brief This file handles BLE BT5 central application routines.
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/
#define HI_WORD(x)  ((uint8_t)((x & 0xFF00) >> 8))
#define LO_WORD(x)  ((uint8_t)(x))

#if APP_RECOMBINE_ADV_DATA
uint8_t *p_temp_data = NULL;/**< The location to save ext adv data */
#endif
static BLE_SCAN_HDL app_scan_hdl = NULL;
void app_handle_ext_adv_report(uint16_t event_type, T_GAP_EXT_ADV_EVT_DATA_STATUS data_status,
                               uint8_t *bd_addr, uint8_t data_len, uint8_t *p_data);
/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief app_scan_cb
 *        used to notify APP when receive BLE_SCAN_REPORT or BLE_SCAN_PARAM_CHANGES
 *        BLE_SCAN_REPORT:
 *        report this event to APP when scanning to advertising device.
 *        BLE_SCAN_PARAM_CHANGES:
 *        report this event to APP when scan parameter has been modified.
 * @param evt  @ref BLE_SCAN_EVT
 * @param data @ref BLE_SCAN_EVT_DATA Information of le extended advertising report.
 */
static void app_scan_cb(BLE_SCAN_EVT evt, BLE_SCAN_EVT_DATA *data)
{
    uint8_t scan_state = ble_scan_get_cur_state();
    switch (evt)
    {
    case BLE_SCAN_REPORT:
        APP_PRINT_INFO6("app_scan_cb: BLE_SCAN_REPORT event_type 0x%x, bd_addr %s, addr_type %d, rssi %d, data_len %d, data_status 0x%x",
                        data->report->event_type,
                        TRACE_BDADDR(data->report->bd_addr),
                        data->report->addr_type,
                        data->report->rssi,
                        data->report->data_len,
                        data->report->data_status);

        if (data->report->rssi > (0 - 60))
        {
            APP_PRINT_INFO2("app_scan_cb: add device into device list rssi %d, bd_addr %b",
                            data->report->rssi, TRACE_BDADDR(data->report->bd_addr));
            link_mgr_add_device(data->report->bd_addr, data->report->addr_type);
        }

#if APP_RECOMBINE_ADV_DATA
        if (!(data->report->event_type & GAP_EXT_ADV_REPORT_BIT_USE_LEGACY_ADV))
        {
            /* If the advertisement uses extended advertising PDUs, recombine advertising data. */
            app_handle_ext_adv_report(data->report->event_type,
                                      data->report->data_status, data->report->bd_addr,
                                      data->report->data_len, data->report->p_data);
        }
#endif
        break;

    default:
        break;
    }
}

/**
 * @brief app_scan_start
 * @param scan_interval The frequency of scan, in units of 0.625ms, range: 0x0004 to 0xFFFF.
 * @param scan_window   The length of scan, in units of 0.625ms, range: 0x0004 to 0xFFFF.
 */
void app_scan_start(uint16_t scan_interval, uint16_t scan_window, uint8_t scan_phy)
{
    BLE_SCAN_PARAM param;
    BLE_SCAN_FILTER scan_filter;

    APP_PRINT_TRACE2("app_scan_start: scan_interval 0x%x, scan_window 0x%x",
                     scan_interval, scan_window);

    memset(&param, 0, sizeof(param));
    memset(&scan_filter, 0, sizeof(scan_filter));

    param.scan_param_1m.scan_type = GAP_SCAN_MODE_ACTIVE;
    param.scan_param_1m.scan_interval = scan_interval;
    param.scan_param_1m.scan_window = scan_window;
#if LE_CODED_PHY_SUPPORT
    param.scan_param_coded.scan_type = GAP_SCAN_MODE_ACTIVE;
    param.scan_param_coded.scan_interval = scan_interval;
    param.scan_param_coded.scan_window = scan_window;
#endif
    param.ext_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_DISABLE;
    param.ext_filter_policy = GAP_SCAN_FILTER_ANY;
    param.own_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    param.phys = scan_phy;

    scan_filter.filter_flags = BLE_SCAN_FILTER_NONE;
    ble_scan_start(&app_scan_hdl, app_scan_cb, &param, &scan_filter);
}

/**
 * @brief app_scan_stop
 *
 */
void app_scan_stop(void)
{
    ble_scan_stop(&app_scan_hdl);
}

#if APP_RECOMBINE_ADV_DATA
/**
  * @brief Check the length of received advertising data.
  * @param[in] data_len      Length of recieved advertising data.
  * @retval true:   Lenth of received advertising data is less than or equal to APP_MAX_EXT_ADV_TOTAL_LEN.
            false:  Lenth of received advertising data is greater than APP_MAX_EXT_ADV_TOTAL_LEN.
  */
bool app_check_adv_data_len(void)
{
    if (ext_adv_data->data_len > APP_MAX_EXT_ADV_TOTAL_LEN)
    {
        APP_PRINT_ERROR2("app_check_adv_data_len: The length of received advertising data is %d, exceeds APP_MAX_EXT_ADV_TOTAL_LEN %d",
                         ext_adv_data->data_len, APP_MAX_EXT_ADV_TOTAL_LEN);
        /* Update failed recombination parameters for next recombination. */
        fail_event_type = ext_adv_data->event_type;
        memcpy(fail_bd_addr, ext_adv_data->bd_addr, GAP_BD_ADDR_LEN);
        /* Reset recombination parameters. */
        ext_adv_data->data_len = 0;
        ext_adv_data->flag = false;
        return false;
    }
    else
    {
        return true;
    }
}

/**
  * @brief Handle callback GAP_MSG_LE_EXT_ADV_REPORT_INFO to recombine advertising data.
  * @param[in] event_type    Advertisement event type.
  * @param[in] data_status   Data status @ref T_GAP_EXT_ADV_EVT_DATA_STATUS.
  * @param[in] bd_addr       Peer device address.
  * @param[in] data_len      Length of data.
  * @param[in] p_data        Advertising data.
  * @retval void
  */
void app_handle_ext_adv_report(uint16_t event_type, T_GAP_EXT_ADV_EVT_DATA_STATUS data_status,
                               uint8_t *bd_addr, uint8_t data_len, uint8_t *p_data)
{
    APP_PRINT_INFO2("app_handle_ext_adv_report: Old ext_adv_data->flag is %d, data status is 0x%x",
                    ext_adv_data->flag, data_status);

    /* Recombine advertising data from one device. */
    switch (data_status)
    {
    case GAP_EXT_ADV_EVT_DATA_STATUS_COMPLETE:
        /* Advertising data is complete. */
        if (ext_adv_data->flag)
        {
            if ((memcmp(ext_adv_data->bd_addr, bd_addr, GAP_BD_ADDR_LEN) == 0) &&
                (ext_adv_data->event_type == event_type))
            {
                /* The advertising report is the expected report. */
                if ((memcmp(fail_bd_addr, bd_addr, GAP_BD_ADDR_LEN) == 0) && (fail_event_type == event_type))
                {
                    APP_PRINT_ERROR2("app_handle_ext_adv_report: The advertising data is destroyed by last failed recombination, last failed bd_addr %s, last failed event type 0x%x",
                                     TRACE_BDADDR(fail_bd_addr), fail_event_type);
                }
                else
                {
                    /* Update length of advertising data, and check whether the length exceeds APP_MAX_EXT_ADV_TOTAL_LEN. */
                    ext_adv_data->data_len += data_len;
                    if (!(app_check_adv_data_len()))
                    {
                        return;
                    }
                    memcpy(p_temp_data, p_data, data_len);
                    STREAM_SKIP_LEN(p_temp_data, data_len);
                    APP_PRINT_INFO3("app_handle_ext_adv_report: Data from bd_addr %s is complete, event type is 0x%x, total data length is %d",
                                    TRACE_BDADDR(ext_adv_data->bd_addr), ext_adv_data->event_type, ext_adv_data->data_len);
                    if (ext_adv_data->data_len > 5)
                    {
                        APP_PRINT_INFO5("app_handle_ext_adv_report: First five datas are 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
                                        ext_adv_data->p_data[0], ext_adv_data->p_data[1], ext_adv_data->p_data[2], ext_adv_data->p_data[3],
                                        ext_adv_data->p_data[4]);
                        APP_PRINT_INFO5("app_handle_ext_adv_report: Last five datas are 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
                                        ext_adv_data->p_data[ext_adv_data->data_len - 5], ext_adv_data->p_data[ext_adv_data->data_len - 4],
                                        ext_adv_data->p_data[ext_adv_data->data_len - 3],
                                        ext_adv_data->p_data[ext_adv_data->data_len - 2], ext_adv_data->p_data[ext_adv_data->data_len - 1]);
                    }
                }
                /*  Reset recombination parameters. */
                memset(fail_bd_addr, 0, GAP_BD_ADDR_LEN);
                ext_adv_data->data_len = 0;
                ext_adv_data->flag = false;
            }
            else
            {
                APP_PRINT_INFO2("app_handle_ext_adv_report: New data which is not saved from bd_addr %s is complete, total data length is %d",
                                TRACE_BDADDR(bd_addr), data_len);
            }
        }
        else
        {
            if ((memcmp(fail_bd_addr, bd_addr, GAP_BD_ADDR_LEN) == 0) && (fail_event_type == event_type))
            {
                APP_PRINT_ERROR2("app_handle_ext_adv_report: The advertising data is destroyed by last failed recombination, last failed bd_addr %s, last failed event type 0x%x",
                                 TRACE_BDADDR(fail_bd_addr), fail_event_type);
            }
            else
            {
                /* Update recombination parameters, and check whether the length exceeds APP_MAX_EXT_ADV_TOTAL_LEN. */
                ext_adv_data->flag = true;
                ext_adv_data->data_len = data_len;
                memcpy(ext_adv_data->bd_addr, bd_addr, GAP_BD_ADDR_LEN);
                ext_adv_data->event_type = event_type;
                if (!(app_check_adv_data_len()))
                {
                    return;
                }
                p_temp_data = ext_adv_data->p_data;
                memcpy(p_temp_data, p_data, data_len);
                STREAM_SKIP_LEN(p_temp_data, data_len);
                APP_PRINT_INFO2("app_handle_ext_adv_report: Data from bd_addr %s is complete, total data length is %d",
                                TRACE_BDADDR(ext_adv_data->bd_addr), data_len);
                ext_adv_data->data_len = 0;
                ext_adv_data->flag = false;
            }
            memset(fail_bd_addr, 0, GAP_BD_ADDR_LEN);
        }
        break;

    case GAP_EXT_ADV_EVT_DATA_STATUS_MORE:
        /* Advertising data is incomplete, more data to come. */
        if (ext_adv_data->flag)
        {
            if ((memcmp(ext_adv_data->bd_addr, bd_addr, GAP_BD_ADDR_LEN) == 0) &&
                (ext_adv_data->event_type == event_type))
            {
                /* The advertising report is the expected report. */
                ext_adv_data->data_len += data_len;
                if (!(app_check_adv_data_len()))
                {
                    return;
                }
                memcpy(p_temp_data, p_data, data_len);
                STREAM_SKIP_LEN(p_temp_data, data_len);
                APP_PRINT_INFO2("app_handle_ext_adv_report: Continuation data from bd_addr %s is incomplete, data length is %d, and waiting more data",
                                TRACE_BDADDR(ext_adv_data->bd_addr), ext_adv_data->data_len);
            }
            else
            {
                APP_PRINT_INFO2("app_handle_ext_adv_report: New data which is not saved from bd_addr %s is incomplete, data length is %d",
                                TRACE_BDADDR(bd_addr), data_len);
            }
        }
        else
        {
            /* Update recombination parameters for first fragment, and check whether the length exceeds APP_MAX_EXT_ADV_TOTAL_LEN. */
            ext_adv_data->flag = true;
            ext_adv_data->data_len = data_len;
            memcpy(ext_adv_data->bd_addr, bd_addr, GAP_BD_ADDR_LEN);
            ext_adv_data->event_type = event_type;
            if (!(app_check_adv_data_len()))
            {
                return;
            }
            p_temp_data = ext_adv_data->p_data;
            memcpy(p_temp_data, p_data, data_len);
            STREAM_SKIP_LEN(p_temp_data, data_len);
            APP_PRINT_INFO2("app_handle_ext_adv_report:First Data from bd_addr %s, data length is %d, and waiting more data",
                            TRACE_BDADDR(ext_adv_data->bd_addr), ext_adv_data->data_len);
        }
        break;

    case GAP_EXT_ADV_EVT_DATA_STATUS_TRUNCATED:
        /* Advertising data is incomplete, data truncated, no more to come. */
        if (ext_adv_data->flag && (memcmp(ext_adv_data->bd_addr, bd_addr, GAP_BD_ADDR_LEN) == 0) &&
            (ext_adv_data->event_type == event_type))
        {
            /* If data is truncated, reset recombination parameters. */
            ext_adv_data->data_len += data_len;
            if (!(app_check_adv_data_len()))
            {
                return;
            }
            memcpy(p_temp_data, p_data, data_len);
            STREAM_SKIP_LEN(p_temp_data, data_len);
            APP_PRINT_INFO3("app_handle_ext_adv_report: Continuation data from bd_addr %s is truncated, event type is 0x%x, data length is %d, and no more data to come",
                            TRACE_BDADDR(ext_adv_data->bd_addr), ext_adv_data->event_type, ext_adv_data->data_len);
            memset(fail_bd_addr, 0, GAP_BD_ADDR_LEN);
            ext_adv_data->data_len = 0;
            ext_adv_data->flag = false;
        }
        break;

    case GAP_EXT_ADV_EVT_DATA_STATUS_RFU:
        /* Reserved for future use. */
        break;

    default:
        APP_PRINT_ERROR1("app_handle_ext_adv_report: unhandled data_status 0x%x", data_status);
        break;
    }
    APP_PRINT_INFO1("app_handle_ext_adv_report: New ext_adv_data->flag is %d", ext_adv_data->flag);
}
#endif
/** @} */ /* End of group BT5_CENTRAL_APP */
