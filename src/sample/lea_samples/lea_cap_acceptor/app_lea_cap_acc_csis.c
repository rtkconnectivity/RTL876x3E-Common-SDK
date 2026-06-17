/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "ble_audio.h"
#include "trace.h"
#include "app_lea_cap_acc_main.h"
#include "ble_audio.h"
#include "csis_mgr.h"
#include "app_lea_cap_acc_csis.h"
#include "app_lea_cap_acc_adv.h"

#if CSIP_SET_MEMBER

const uint8_t csis_test_sirk[CSIS_SIRK_LEN] = {0x11, 0x22, 0x33, 0xc6, 0xaf, 0xbb, 0x65, 0xa2, 0x5a, 0x41, 0xf1, 0x53, 0x05, 0x68, 0x8e, 0x83};

uint16_t app_lea_acc_csis_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
    case LE_AUDIO_MSG_CSIS_READ_SIRK_IND:
        {
            T_CSIS_READ_SIRK_IND *p_data = (T_CSIS_READ_SIRK_IND *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_CSIS_READ_SIRK_IND: conn_handle 0x%x, service_id %d, sirk_type %d",
                            p_data->conn_handle, p_data->service_id,
                            p_data->sirk_type);
        }
        break;

    case LE_AUDIO_MSG_CSIS_LOCK_STATE:
        {
            T_CSIS_LOCK_STATE *p_data = (T_CSIS_LOCK_STATE *)buf;
            APP_PRINT_INFO4("LE_AUDIO_MSG_CSIS_LOCK_STATE: service_id %d, lock_state %d, lock_address_type %d, lock_address %s",
                            p_data->service_id, p_data->lock_state,
                            p_data->lock_address_type, TRACE_BDADDR(p_data->lock_address));
        }
        break;
    default:
        break;
    }

    return cb_result;
}

void app_lea_acc_csis_init(T_CAP_INIT_PARAMS *p_param)
{
    ble_audio_cback_register(app_lea_acc_csis_handle_msg);
    memcpy(app_db.lea_csis_sirk, csis_test_sirk, CSIS_SIRK_LEN);
    p_param->csis_num = 1;
    p_param->cas.csis_sirk_type = CSIS_SIRK_PLN;
    p_param->cas.csis_size = 2;
    if (app_db.csis_cfg == APP_LEA_CSIS_CFG_RANK_1)
    {
        p_param->cas.csis_rank = 1;
    }
    else if (app_db.csis_cfg == APP_LEA_CSIS_CFG_RANK_2)
    {
        p_param->cas.csis_rank = 2;
    }
    p_param->cas.csis_feature = (SET_MEMBER_SIZE_EXIST | SET_MEMBER_RANK_EXIST |
                                 SET_MEMBER_LOCK_EXIST);
    p_param->cas.csis_sirk = app_db.lea_csis_sirk;
    app_ext_adv_adv_cfg(LE_EXT_ADV_PSRI);
}
#endif

