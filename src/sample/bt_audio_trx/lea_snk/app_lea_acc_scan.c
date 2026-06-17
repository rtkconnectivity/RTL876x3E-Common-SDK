/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_TMAP_BMR_SUPPORT
#include <string.h>
#include "trace.h"
#include "ble_audio_def.h"
#include "ble_scan.h"
#include "pbp_def.h"
#include "app_cfg.h"
#include "app_auto_power_off.h"
#include "app_lea_acc_scan.h"
#include "app_lea_acc_broadcast_audio.h"
#include "app_link_util_cs.h"
#include "app_timer.h"

typedef struct
{
    uint8_t report_data_len;
    uint8_t *p_report_data;
    uint16_t uuid;
} T_APP_LEA_SCAN_ADV_INFO;


#define LE_AUDIO_SCAN_TIME 120
static uint8_t app_lea_scan_timer_id = 0;
static uint8_t timer_idx_lea_scan_to = 0;

//////TIMER
typedef enum
{
    APP_LEA_SCAN_TIMER_TO         = 0x00,
} T_LEA_SCAN_TIMER;

static BLE_SCAN_HDL app_lea_scan_handle = NULL;

static bool app_lea_scan_filter_service_data(T_APP_LEA_SCAN_ADV_INFO adv_info,
                                             uint8_t **p_service_data,
                                             uint16_t *p_data_len)
{
    uint8_t *p_buffer = NULL;
    uint16_t pos = 0;
    bool found = false;

    while (pos < adv_info.report_data_len)
    {
        /* Length of the AD structure. */
        uint16_t length = adv_info.p_report_data[pos++];
        uint8_t type;

        if (length <= 1)
        {
            return false;
        }

        if ((pos + length) <= adv_info.report_data_len)
        {
            /* Copy the AD Data to buffer. */
            p_buffer = adv_info.p_report_data + pos + 1;
            /* AD Type, one octet. */
            type = adv_info.p_report_data[pos];

            switch (type)
            {
            case GAP_ADTYPE_SERVICE_DATA:
            case GAP_ADTYPE_16BIT_MORE:         //just for pts
                {
                    uint16_t srv_uuid;

                    LE_STREAM_TO_UINT16(srv_uuid, p_buffer);

                    if (srv_uuid == adv_info.uuid)
                    {
                        *p_service_data = p_buffer;
                        *p_data_len = length - 3;
                        found = true;
                    }
                }
                break;

            default:
                break;
            }
        }
        else
        {
            APP_PRINT_ERROR0("app_lea_scan_filter_service_data format error!");
            return false;
        }

        pos += length;
    }
    return found;
}

static bool app_lea_scan_report(T_LE_EXT_ADV_REPORT_INFO *p_report)
{
    int8_t error_code;

    uint8_t *p_service_data = NULL;
    uint16_t service_data_len = 0;
    uint8_t *p_pba_data = NULL;
    uint16_t pba_data_len = 0;
    T_LEA_BRS_INFO src_info = {0};
    T_APP_LEA_SCAN_ADV_INFO adv_info = {0};

    adv_info.report_data_len = p_report->data_len;
    adv_info.p_report_data = p_report->p_data;
    adv_info.uuid = BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID;
    if (!app_lea_scan_filter_service_data(adv_info, &p_service_data, &service_data_len))
    {
        error_code = 1;
        goto error;
    }

    APP_PRINT_INFO1("app_lea_scan_report: bca state 0x%x", app_lea_bca_state());

    adv_info.uuid = PUBIC_BROADCAST_ANNOUNCEMENT_SRV_UUID;
    if (app_lea_scan_filter_service_data(adv_info, &p_pba_data, &pba_data_len))
    {
        if (pba_data_len)
        {
            if (*p_pba_data & PUBIC_BROADCAST_BIT_ENCRYPTED)
            {
                if (memcmp(p_report->bd_addr, app_cfg_nv.lea_srcaddr, GAP_BD_ADDR_LEN) ||
                    (p_report->addr_type != app_cfg_nv.lea_srctype) ||
                    !app_cfg_nv.lea_encryp || !app_cfg_nv.lea_valid)
                {
                    APP_PRINT_ERROR1("app_lea_scan_report encrypted w/o code! %s",
                                     TRACE_BDADDR(app_cfg_nv.lea_srcaddr));
                    error_code = 3;
                    goto error;
                }
            }
        }
    }

    src_info.adv_addr_type =  p_report->addr_type;
    src_info.advertiser_sid = p_report->adv_sid;
    memcpy(src_info.adv_addr, p_report->bd_addr, GAP_BD_ADDR_LEN);
    memcpy(src_info.broadcast_id, p_service_data, service_data_len);
    app_lea_bca_scan_info(&src_info);
    return true;

error:
    APP_PRINT_ERROR1("app_lea_scan_report: error %d", -error_code);
    return false;
}



static void app_lea_scan_cb(BLE_SCAN_EVT state, BLE_SCAN_EVT_DATA *data)
{
    APP_PRINT_TRACE1("app_lea_scan_cb: state %d", state);

    switch (state)
    {
    case BLE_SCAN_REPORT:
        app_lea_scan_report(data->report);
        break;

    default:
        break;
    }
}

static void app_lea_scan_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_LEA_SCAN_TIMER_TO:
        {
            app_stop_timer(&timer_idx_lea_scan_to);
            if ((app_lea_bca_state() != LEA_BCA_STATE_STREAMING) && !app_link_get_le_link_num())
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BLE_AUDIO, app_cfg_const.timer_auto_power_off);
            }
            app_lea_bca_sm(LEA_SCAN_TIMEOUT, NULL);
        }
        break;

    default:
        break;
    }
}

void app_lea_acc_scan_start()
{
    BLE_SCAN_PARAM param;

    /* Initialize extended scan parameters */
    param.own_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    param.phys = GAP_EXT_SCAN_PHYS_1M_BIT;
    param.ext_filter_policy = GAP_SCAN_FILTER_ANY_RPA;
    param.ext_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

    param.scan_param_1m.scan_type = GAP_SCAN_MODE_PASSIVE;
    param.scan_param_1m.scan_interval = 0x140;
    param.scan_param_1m.scan_window = 0xD0;

    param.scan_param_coded.scan_type = GAP_SCAN_MODE_PASSIVE;
    param.scan_param_coded.scan_interval = 0x0050;
    param.scan_param_coded.scan_window = 0x0025;

    /* Enable extended scan */
    ble_scan_start(&app_lea_scan_handle, app_lea_scan_cb, &param, NULL);

    app_start_timer(&timer_idx_lea_scan_to, "lea_scan_start",
                    app_lea_scan_timer_id, APP_LEA_SCAN_TIMER_TO, 0, false,
                    LE_AUDIO_SCAN_TIME * 1000);
}

void app_lea_acc_scan_stop()
{
    APP_PRINT_INFO0("app_lea_acc_scan_stop");
    app_stop_timer(&timer_idx_lea_scan_to);
    ble_scan_stop(&app_lea_scan_handle);
}


void app_lea_scan_init()
{
    app_timer_reg_cb(app_lea_scan_timeout_cb, &app_lea_scan_timer_id);
}
#endif

