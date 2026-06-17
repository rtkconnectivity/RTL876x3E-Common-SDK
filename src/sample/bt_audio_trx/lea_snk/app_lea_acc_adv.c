/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
#include <string.h>
#include "trace.h"
#include "ascs_def.h"
#include "bass_def.h"
#include "ble_ext_adv.h"
#include "cap.h"
#include "csis_rsi.h"
#include "gatt.h"
#include "mics_def.h"
#include "pacs_def.h"
#include "tmas_def.h"
#include "bt_types.h"
#include "vcs_def.h"
#include "app_audio_policy.h"
#include "app_auto_power_off.h"
#include "app_cfg.h"
#include "app_lea_acc_adv.h"
#include "app_lea_acc_broadcast_audio.h"
#include "app_lea_acc_csis.h"
#include "app_lea_acc_def.h"
#include "app_lea_acc_pacs.h"
#include "app_lea_acc_profile.h"
#include "app_link_util_cs.h"
#include "app_timer.h"

// #if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
/*Appearance: Category bit15~6, Sub-category: bit5~0 */
//Category
#define WEARABLE_AUDIO_DEVICE      0x025
//Sub-category
#define EARBUDS  0X01
#define HEADSET  0X02

#define LE_EXT_ADV_PSRI 0x0001
#define LE_EXT_ADV_ASCS 0x0002
#define LE_EXT_ADV_BASS 0x0004
#define LE_EXT_ADV_TMAS 0x0008
#define LE_EXT_ADV_PACS 0x0010
#define LE_EXT_ADV_VCS  0x0020
#define LE_EXT_ADV_MICS 0x0040
#define LE_EXT_NONE     0xFF

#define LEA_ADV_TMR_LINK_LOST   60
#define LEA_ADV_TMR_PAIRING     120

/*default max adv length include Flags(3), device name(42), CAS(5), ASCS(10), BASS(4), TMAS(6),
Service UUID(12), RSI(8), Appearance(4), Manufacture data(7) and Vendor Gaming mode(3).*/
#define APP_LEA_ADV_LEN         104
#define APP_LEA_APPEARACNCE     (WEARABLE_AUDIO_DEVICE << 6 | (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SINGLE ? EARBUDS : HEADSET))

typedef enum
{
    APP_LEA_TMR_PAIRING      = 0x00,
    APP_LEA_TMR_LINKLOSS     = 0x01,
} T_APP_LEA_TIMER;

static bool stack_disable_lea_adv = false;
static T_BLE_EXT_ADV_MGR_STATE app_lea_adv_state = BLE_EXT_ADV_MGR_ADV_DISABLED;
static uint8_t app_lea_adv_handle = 0xFF;

static uint8_t app_lea_adv_timer_id = 0;
static uint8_t timer_idx_lea_adv = 0;

uint8_t app_lea_adv_data[APP_LEA_ADV_LEN];
uint8_t app_lea_adv_scan_rsp_data[] =
{
    0x03,
    GAP_ADTYPE_APPEARANCE,
    LO_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
    HI_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
};

/*Unicast Server that sent the PDU is available for a general audio use case.
In this specification, a general audio use case means the transmission or reception of Audio Data
that has not been initiated by a higher-layer specification.*/
static uint16_t app_lea_adv_ext_data(uint16_t audio_adv_flag, uint8_t service_num)
{
    uint16_t idx = 0;
    uint8_t lea_flags = GAP_ADTYPE_FLAGS_LIMITED;
    uint8_t name_len = strlen((const char *)app_cfg_nv.device_name_le);

    app_lea_adv_data[idx++] = 0x02;
    app_lea_adv_data[idx++] = GAP_ADTYPE_FLAGS;
#if F_APP_BREDR_SC_CTKD_SUPPORT
    lea_flags |= GAP_ADTYPE_FLAGS_SIMULTANEOUS_LE_BREDR_CONTROLLER |
                 GAP_ADTYPE_FLAGS_SIMULTANEOUS_LE_BREDR_HOST;
#else
    lea_flags |= GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
#endif
    app_lea_adv_data[idx++] = lea_flags;

    app_lea_adv_data[idx++] = name_len + 1;
    app_lea_adv_data[idx++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&app_lea_adv_data[idx], app_cfg_nv.device_name_le, name_len);
    idx += name_len;

    app_lea_adv_data[idx++] = 0x04;
    app_lea_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
    app_lea_adv_data[idx++] = LO_WORD(GATT_UUID_CAS);
    app_lea_adv_data[idx++] = HI_WORD(GATT_UUID_CAS);
    app_lea_adv_data[idx++] = ADV_TARGETED_ANNOUNCEMENT;

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    if (audio_adv_flag & LE_EXT_ADV_ASCS)
    {
        app_lea_adv_data[idx++] = 0x09;
        app_lea_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
        app_lea_adv_data[idx++] = LO_WORD(GATT_UUID_ASCS);
        app_lea_adv_data[idx++] = HI_WORD(GATT_UUID_ASCS);
        app_lea_adv_data[idx++] = ADV_TARGETED_ANNOUNCEMENT;

        LE_UINT16_TO_ARRAY(app_lea_adv_data + idx,
                           app_lea_pacs_get_sink_available_contexts());
        idx += 2;
        LE_UINT16_TO_ARRAY(app_lea_adv_data + idx,
                           app_lea_pacs_get_source_available_contexts());
        idx += 2;
        app_lea_adv_data[idx++] = 0; // metadata length
    }
#endif

#if F_APP_BASS_SUPPORT
    if (audio_adv_flag & LE_EXT_ADV_BASS)
    {
        app_lea_adv_data[idx++] = 0x03;
        app_lea_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
        app_lea_adv_data[idx++] = LO_WORD(GATT_UUID_BASS);
        app_lea_adv_data[idx++] = HI_WORD(GATT_UUID_BASS);
    }
#endif

#if F_APP_TMAS_SUPPORT
    if (audio_adv_flag & LE_EXT_ADV_TMAS)
    {
        uint16_t role = app_lea_profile_get_tmas_role();

        app_lea_adv_data[idx++] = 0x05;
        app_lea_adv_data[idx++] = GAP_ADTYPE_SERVICE_DATA;
        app_lea_adv_data[idx++] = LO_WORD(GATT_UUID_TMAS);
        app_lea_adv_data[idx++] = HI_WORD(GATT_UUID_TMAS);
        app_lea_adv_data[idx++] = LO_WORD(role);
        app_lea_adv_data[idx++] = HI_WORD(role);
    }
#endif

    app_lea_adv_data[idx++] = service_num * 2 + 1;
    app_lea_adv_data[idx++] = GAP_ADTYPE_16BIT_MORE;
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    if (audio_adv_flag & LE_EXT_ADV_ASCS)
    {
        app_lea_adv_data[idx++] = LO_WORD(GATT_UUID_ASCS);
        app_lea_adv_data[idx++] = HI_WORD(GATT_UUID_ASCS);
    }
#endif
#if F_APP_BASS_SUPPORT
    if (audio_adv_flag & LE_EXT_ADV_BASS)
    {
        app_lea_adv_data[idx++] = LO_WORD(GATT_UUID_BASS);
        app_lea_adv_data[idx++] = HI_WORD(GATT_UUID_BASS);
    }
#endif
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
    if (audio_adv_flag & LE_EXT_ADV_PACS)
    {
        app_lea_adv_data[idx++] = LO_WORD(GATT_UUID_PACS);
        app_lea_adv_data[idx++] = HI_WORD(GATT_UUID_PACS);
    }
#endif
#if F_APP_VCS_SUPPORT
    if (audio_adv_flag & LE_EXT_ADV_VCS)
    {
        app_lea_adv_data[idx++] = LO_WORD(GATT_UUID_VCS);
        app_lea_adv_data[idx++] = HI_WORD(GATT_UUID_VCS);
    }
#endif
#if F_APP_MICS_SUPPORT
    if (audio_adv_flag & LE_EXT_ADV_MICS)
    {
        app_lea_adv_data[idx++] = LO_WORD(GATT_UUID_MICS);
        app_lea_adv_data[idx++] = HI_WORD(GATT_UUID_MICS);
    }
#endif

#if F_APP_CSIS_SUPPORT
    if (audio_adv_flag & LE_EXT_ADV_PSRI)
    {
        uint8_t psri_data[CSI_RSI_LEN];

        if (csis_gen_rsi(csis_sirk, psri_data))
        {
            app_lea_adv_data[idx++] = CSI_RSI_LEN + 1;
            app_lea_adv_data[idx++] = GAP_ADTYPE_RSI;
            memcpy(app_lea_adv_data + idx, psri_data, CSI_RSI_LEN);
            idx += CSI_RSI_LEN;
        }
    }
#endif

    app_lea_adv_data[idx++] = 0x03;
    app_lea_adv_data[idx++] = GAP_ADTYPE_APPEARANCE;
    app_lea_adv_data[idx++] = LO_WORD(APP_LEA_APPEARACNCE);
    app_lea_adv_data[idx++] = HI_WORD(APP_LEA_APPEARACNCE);

    app_lea_adv_data[idx++] = 0x06;
    app_lea_adv_data[idx++] = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
    app_lea_adv_data[idx++] = LO_WORD(RTK_COMPANY_ID);
    app_lea_adv_data[idx++] = HI_WORD(RTK_COMPANY_ID);
    app_lea_adv_data[idx++] = 0x02;
    app_lea_adv_data[idx++] = VENDOR_DATA_TYPE_GAMING_MODE;
    app_lea_adv_data[idx++] = app_lea_pacs_is_low_latency();

    return idx;
}

static void app_lea_adv_timeout_cback(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_LEA_TMR_PAIRING:
    case APP_LEA_TMR_LINKLOSS:
        {
            if (app_lea_adv_get_state() == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                if (timer_evt == APP_LEA_TMR_PAIRING)
                {
                    uint8_t info = LEA_ADV_TIMEOUT;

                    app_lea_mgr_adv_sm(LEA_ADV_TIMEOUT, &info);
                }
                else
                {
                    app_lea_mgr_adv_sm(LEA_ADV_STOP, NULL);
                }
            }
#if F_APP_TMAP_BMR_SUPPORT
            if ((app_lea_bca_state() != LEA_BCA_STATE_STREAMING) && !app_link_get_le_link_num())
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BLE_AUDIO, app_cfg_const.timer_auto_power_off);
            }
#endif
        }
        break;

    default:
        break;
    }
}


static void app_lea_adv_cback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;

    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));
    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            APP_PRINT_TRACE4("app_lea_adv_callback: change from %d to %d, adv_handle %d, stop_cause %d",
                             app_lea_adv_state, cb_data.p_ble_state_change->state,
                             cb_data.p_ble_state_change->adv_handle, cb_data.p_ble_state_change->stop_cause);
            app_lea_adv_state = cb_data.p_ble_state_change->state;

            if (app_lea_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                switch (cb_data.p_ble_state_change->stop_cause)
                {
                case BLE_EXT_ADV_STOP_CAUSE_CONN:
                    stack_disable_lea_adv = true;
                    app_lea_mgr_adv_sm(LEA_ADV_STOP, NULL);
                    break;

                default:
                    break;
                }
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        {
            if (stack_disable_lea_adv == true)
            {
                app_lea_mgr_adv_sm(LEA_CIG_START, NULL);
            }
            stack_disable_lea_adv = false;
        }
        break;

    default:
        break;
    }
    return;
}

T_BLE_EXT_ADV_MGR_STATE app_lea_adv_get_state(void)
{
    APP_PRINT_INFO1("app_lea_adv_get_state: %d ", app_lea_adv_state);
    return app_lea_adv_state;
}

void app_lea_mgr_dev_idle(uint8_t event, void *p_data)
{
    APP_PRINT_INFO1("app_lea_mgr_dev_idle: event 0x%x", event);
    switch (event)
    {
    case LEA_ENTER_PAIRING:
        {
            app_lea_adv_start((uint8_t)LEA_ADV_MODE_PAIRING);
        }
        break;

    case LEA_CIG_START:
        {
            if (p_data != NULL)
            {
                if (*(uint16_t *)p_data == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
                {
                    app_audio_tone_type_play(TONE_CIS_START, false, true);
                }
                else if (*(uint16_t *)p_data == (HCI_ERR | HCI_ERR_CONN_TIMEOUT))
                {
                    app_lea_adv_start((uint8_t)LEA_ADV_MODE_LINK_LOSS_LINK_BACK);
                    break;
                }
            }
            app_lea_adv_start((uint8_t)LEA_ADV_MODE_PAIRING);
        }
        break;

    case LEA_MMI:
        {
            app_lea_adv_start((uint8_t)LEA_ADV_MODE_PAIRING);
        }
        break;

    case LEA_ADV_START:
        {
            app_lea_adv_start((uint8_t)LEA_ADV_MODE_PAIRING);
        }
        break;

    default:
        break;
    }
}


void app_lea_mgr_dev_adv(uint8_t event, void *p_data)
{
    APP_PRINT_INFO1("app_lea_mgr_dev_adv: event 0x%x", event);
    switch (event)
    {
    case LEA_POWER_OFF:
        {
            app_lea_adv_stop();
        }
        break;

    case LEA_ENTER_PAIRING:
        {
            app_lea_adv_stop();
            app_lea_adv_start((uint8_t)LEA_ADV_MODE_PAIRING);
        }
        break;

    case LEA_EXIT_PAIRING:
        {
            app_lea_adv_stop();
        }
        break;

    case LEA_ADV_STOP:
        {
            app_lea_adv_stop();
        }
        break;

    case LEA_ADV_TIMEOUT:
        {
            if ((*(T_LEA_ADV_TIMEOUT *)p_data) == LEA_ADV_TIMEOUT)
            {
                app_audio_tone_type_play(TONE_CIS_TIMEOUT, false, false);
            }
            app_lea_adv_stop();
        }
        break;

    case LEA_ADV_START:
        app_lea_adv_start((uint8_t)LEA_ADV_MODE_PAIRING);
        break;

    default:
        break;
    }
}

bool app_lea_mgr_adv_sm(uint8_t event, void *p_data)
{
    switch (app_lea_adv_get_state())
    {
    case BLE_EXT_ADV_MGR_ADV_DISABLED:
        {
            app_lea_mgr_dev_idle(event, p_data);
        }
        break;

    case BLE_EXT_ADV_MGR_ADV_ENABLED:
        {
            app_lea_mgr_dev_adv(event, p_data);
        }
        break;

    default:
        break;
    }
    return true;
}

void app_lea_adv_start(uint8_t mode)
{
    uint8_t public_addr_lsb[6];
    uint8_t len = 0;

    gap_get_param(GAP_PARAM_BD_ADDR, public_addr_lsb);

    APP_PRINT_INFO2("app_lea_adv_start: public addr %s mode %d",
                    TRACE_BDADDR(public_addr_lsb), mode);

    if ((mode == LEA_ADV_MODE_PAIRING) && (app_link_get_le_link_num() > 2))
    {
        return;
    }

    if (app_lea_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
#if F_APP_LE_AUDIO_CIS_RND_ADDR
        ble_ext_adv_mgr_set_random(app_lea_adv_handle, app_cfg_nv.le_single_random_addr);
#endif
        if (ble_ext_adv_mgr_enable(app_lea_adv_handle, 0) == GAP_CAUSE_SUCCESS)
        {
            if ((T_LEA_ADV_MODE)mode == LEA_ADV_MODE_LINK_LOSS_LINK_BACK)
            {
                app_start_timer(&timer_idx_lea_adv, "lea_adv_linkloss", app_lea_adv_timer_id,
                                APP_LEA_TMR_LINKLOSS, 0, false, LEA_ADV_TMR_LINK_LOST * 1000);
            }
            else  if ((T_LEA_ADV_MODE)mode == LEA_ADV_MODE_PAIRING)
            {
                app_start_timer(&timer_idx_lea_adv, "lea_adv_pairing", app_lea_adv_timer_id,
                                APP_LEA_TMR_PAIRING, 0, false, LEA_ADV_TMR_PAIRING * 1000);
            }
        }
    }

    return;
}

void app_lea_adv_stop()
{
    app_stop_timer(&timer_idx_lea_adv);

    ble_ext_adv_mgr_disable(app_lea_adv_handle, 0);
}

static void app_lea_adv_set(uint16_t audio_adv_flag, uint8_t service_num)
{
    uint16_t audio_adv_len = 0;
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_CONN_UNDIRECTED;
    uint16_t adv_interval_min = 0x40;
    uint16_t adv_interval_max = 0x50;

#if F_APP_LE_AUDIO_CIS_RND_ADDR
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
#else
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
#endif
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

#if F_APP_LE_AUDIO_CIS_RND_ADDR
    //the type of static random address is 0xC0
    if ((app_cfg_nv.le_single_random_addr[5] & 0xC0) != 0xC0)
    {
        le_gen_rand_addr(GAP_RAND_ADDR_STATIC, app_cfg_nv.le_single_random_addr);
        app_cfg_store(app_cfg_nv.le_single_random_addr, 6);
        APP_PRINT_TRACE1("app_lea_adv_set: cis_static_random_addr %s",
                         TRACE_BDADDR(app_cfg_nv.le_single_random_addr));

    }
#endif

    audio_adv_len = app_lea_adv_ext_data(audio_adv_flag, service_num);
    ble_ext_adv_mgr_init_adv_params(&app_lea_adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, audio_adv_len, app_lea_adv_data,
                                    sizeof(app_lea_adv_scan_rsp_data), app_lea_adv_scan_rsp_data, NULL);
    ble_ext_adv_mgr_change_adv_phy(app_lea_adv_handle, GAP_PHYS_PRIM_ADV_1M, GAP_PHYS_2M);
    ble_ext_adv_mgr_register_callback(app_lea_adv_cback, app_lea_adv_handle);
}

void app_lea_adv_update(void)
{
    uint16_t audio_adv_len = 0;
    uint16_t audio_adv_flag = 0;
    uint8_t service_num = 0;

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    audio_adv_flag |= LE_EXT_ADV_ASCS;
    service_num++;
#endif
#if F_APP_CSIS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_PSRI;
#endif
#if F_APP_BASS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_BASS;
    service_num++;
#endif
#if F_APP_TMAS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_TMAS;
#endif
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
    audio_adv_flag |= LE_EXT_ADV_PACS;
    service_num++;
#endif
#if F_APP_VCS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_VCS;
    service_num++;
#endif
#if F_APP_MICS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_MICS;
    service_num++;
#endif

    audio_adv_len = app_lea_adv_ext_data(audio_adv_flag, service_num);
    ble_ext_adv_mgr_set_adv_data(app_lea_adv_handle, audio_adv_len, app_lea_adv_data);
}

void app_lea_adv_init(void)
{
    uint16_t audio_adv_flag = 0;
    uint8_t service_num = 0;

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    audio_adv_flag |= LE_EXT_ADV_ASCS;
    service_num++;
#endif
#if F_APP_CSIS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_PSRI;
#endif
#if F_APP_BASS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_BASS;
    service_num++;
#endif
#if F_APP_TMAS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_TMAS;
#endif
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
    audio_adv_flag |= LE_EXT_ADV_PACS;
    service_num++;
#endif
#if F_APP_VCS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_VCS;
    service_num++;
#endif
#if F_APP_MICS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_MICS;
    service_num++;
#endif

    app_lea_adv_set(audio_adv_flag, service_num);
    app_timer_reg_cb(app_lea_adv_timeout_cback, &app_lea_adv_timer_id);
}

#endif
