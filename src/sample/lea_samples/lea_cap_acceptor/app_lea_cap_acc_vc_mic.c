/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "ble_audio.h"
#include "trace.h"
#include "app_lea_cap_acc_main.h"
#include "app_lea_cap_acc_cfg.h"
#include "ble_audio.h"
#include "vcs_mgr.h"
#include "mics_mgr.h"
#include "data_uart.h"
#include "aics_mgr.h"
#include "vocs_mgr.h"

#if APP_LEA_VOCS_SUPPORT
const char app_audio_output_des_left[] = "Left output dec";
const char app_audio_output_des_right[] = "Right output dec";
const char app_audio_output_des[] = "Output dec";
#endif
#if APP_LEA_AICS_FOR_VCS_SUPPORT
const char app_audio_input_des_vcs[] = "AICS for VCS input dec";
#endif
#if APP_LEA_AICS_FOR_MICS_SUPPORT
const char app_audio_input_des_mics[] = "AICS for MICS input dec";
#endif

uint16_t app_lea_acc_vc_mic_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (msg)
    {
#if VCP_VOLUME_RENDERER
    case LE_AUDIO_MSG_VCS_VOLUME_CP_IND:
        {
            T_VCS_VOLUME_CP_IND *p_volume_state = (T_VCS_VOLUME_CP_IND *)buf;
            APP_PRINT_INFO3("app_lea_acc_vc_mic_handle_msg: LE_AUDIO_MSG_VCS_VOLUME_CP_IND, conn_handle 0x%x, volume_setting %d, mute %d",
                            p_volume_state->conn_handle, p_volume_state->volume_setting,
                            p_volume_state->mute);
            T_VCS_PARAM vcs_param;
            if (vcs_get_param(&vcs_param))
            {
                app_cfg_nv.lea_vcs_vol_setting = vcs_param.volume_setting;
                app_cfg_nv.lea_vcs_mute = vcs_param.mute;
                app_cfg_nv.lea_vcs_change_cnt = vcs_param.change_counter;
                app_cfg_nv.lea_vcs_vol_flag = vcs_param.volume_flags;
                app_cfg_store_all();
                data_uart_print("LE_AUDIO_MSG_VCS_VOLUME_CP_IND: volume_setting %d, mute %d, cp_op %d\r\n",
                                p_volume_state->volume_setting,
                                p_volume_state->mute, p_volume_state->cp_op);
            }
        }
        break;
#endif
#if MICP_MIC_DEVICE
    case LE_AUDIO_MSG_MICS_WRITE_MUTE_IND:
        {
            T_MICS_WRITE_MUTE_IND *p_write_ind = (T_MICS_WRITE_MUTE_IND *)buf;

            APP_PRINT_INFO1("app_lea_acc_vc_mic_handle_msg: LE_AUDIO_MSG_MICS_WRITE_MUTE_IND, mic_mute %d",
                            p_write_ind->mic_mute);
            data_uart_print("LE_AUDIO_MSG_MICS_WRITE_MUTE_IND: mic_mute %d\r\n", p_write_ind->mic_mute);
        }
        break;
#endif
#if (APP_LEA_AICS_FOR_VCS_SUPPORT || APP_LEA_AICS_FOR_MICS_SUPPORT)
    case LE_AUDIO_MSG_AICS_CP_IND:
        {
            T_AICS_CP_IND *p_cp_ind = (T_AICS_CP_IND *)buf;
            data_uart_print("LE_AUDIO_MSG_AICS_CP_IND: conn_handle 0x%x, srv_instance_id %d, cp_op %d, gain_setting %d \r\n",
                            p_cp_ind->conn_handle,
                            p_cp_ind->srv_instance_id,
                            p_cp_ind->cp_op,
                            p_cp_ind->gain_setting);
            APP_PRINT_INFO4("app_lea_acc_vc_mic_handle_msg: LE_AUDIO_MSG_AICS_CP_IND, conn_handle 0x%x, srv_instance_id %d, cp_op %d, gain_setting %d",
                            p_cp_ind->conn_handle,
                            p_cp_ind->srv_instance_id,
                            p_cp_ind->cp_op,
                            p_cp_ind->gain_setting);
        }
        break;

    case LE_AUDIO_MSG_AICS_WRITE_INPUT_DES_IND:
        {
            T_AICS_WRITE_INPUT_DES_IND *p_write_ind = (T_AICS_WRITE_INPUT_DES_IND *)buf;
            data_uart_print("LE_AUDIO_MSG_AICS_WRITE_INPUT_DES_IND: conn_handle 0x%x, srv_instance_id %d, input des len %d \r\n",
                            p_write_ind->conn_handle,
                            p_write_ind->srv_instance_id,
                            p_write_ind->input_des.input_des_len);
            APP_PRINT_INFO4("app_lea_acc_vc_mic_handle_msg: LE_AUDIO_MSG_AICS_WRITE_INPUT_DES_IND, conn_handle 0x%x, srv_instance_id %d, input des[%d] %b",
                            p_write_ind->conn_handle,
                            p_write_ind->srv_instance_id,
                            p_write_ind->input_des.input_des_len,
                            TRACE_BINARY(p_write_ind->input_des.input_des_len, p_write_ind->input_des.p_input_des));
        }
        break;
#endif
#if APP_LEA_VOCS_SUPPORT
    case LE_AUDIO_MSG_VOCS_WRITE_OFFSET_STATE_IND:
        {
            T_VOCS_WRITE_OFFSET_STATE_IND *p_write_ind = (T_VOCS_WRITE_OFFSET_STATE_IND *)buf;
            data_uart_print("LE_AUDIO_MSG_VOCS_WRITE_OFFSET_STATE_IND: conn_handle 0x%x, srv_instance_id %d, volume_offset %d\r\n",
                            p_write_ind->conn_handle,
                            p_write_ind->srv_instance_id,
                            p_write_ind->volume_offset);
            APP_PRINT_INFO3("app_lea_acc_vc_mic_handle_msg: LE_AUDIO_MSG_VOCS_WRITE_OFFSET_STATE_IND, conn_handle 0x%x, srv_instance_id %d, volume_offset %d",
                            p_write_ind->conn_handle,
                            p_write_ind->srv_instance_id,
                            p_write_ind->volume_offset);
        }
        break;

    case LE_AUDIO_MSG_VOCS_WRITE_AUDIO_LOCATION_IND:
        {
            T_VOCS_WRITE_AUDIO_LOCATION_IND *p_write_ind = (T_VOCS_WRITE_AUDIO_LOCATION_IND *)buf;
            data_uart_print("LE_AUDIO_MSG_AICS_WRITE_INPUT_DES_IND: conn_handle 0x%x, srv_instance_id %d, audio_location 0x%x\r\n",
                            p_write_ind->conn_handle,
                            p_write_ind->srv_instance_id,
                            p_write_ind->audio_location);
            APP_PRINT_INFO3("app_lea_acc_vc_mic_handle_msg: LE_AUDIO_MSG_VOCS_WRITE_AUDIO_LOCATION_IND, conn_handle 0x%x, srv_instance_id %d, audio_location 0x%x",
                            p_write_ind->conn_handle,
                            p_write_ind->srv_instance_id,
                            p_write_ind->audio_location);
        }
        break;
    case LE_AUDIO_MSG_VOCS_WRITE_OUTPUT_DES_IND:
        {
            T_VOCS_WRITE_OUTPUT_DES_IND *p_write_ind = (T_VOCS_WRITE_OUTPUT_DES_IND *)buf;
            data_uart_print("LE_AUDIO_MSG_VOCS_WRITE_OUTPUT_DES_IND: conn_handle 0x%x, srv_instance_id %d, output des len %d\r\n",
                            p_write_ind->conn_handle,
                            p_write_ind->srv_instance_id,
                            p_write_ind->output_des.output_des_len);
            APP_PRINT_INFO4("app_lea_acc_vc_mic_handle_msg: LE_AUDIO_MSG_VOCS_WRITE_OUTPUT_DES_IND, conn_handle 0x%x, srv_instance_id %d, output des[%d] %b",
                            p_write_ind->conn_handle,
                            p_write_ind->srv_instance_id,
                            p_write_ind->output_des.output_des_len,
                            TRACE_BINARY(p_write_ind->output_des.output_des_len, p_write_ind->output_des.p_output_des));
        }
        break;
#endif
    default:
        break;
    }

    return cb_result;
}

void app_lea_acc_vc_mic_init(void)
{
    T_BLE_AUDIO_VC_MIC_PARAMS vc_mic_param = {0};
#if MICP_MIC_DEVICE
    T_MICS_PARAM mics_param = {MICS_NOT_MUTE};
#endif
#if VCP_VOLUME_RENDERER
    T_VCS_PARAM vcs_param = {0};
#endif

#if APP_LEA_VOCS_SUPPORT
    uint8_t vocs_feature[1];
    vc_mic_param.vocs_num = 1;
    vocs_feature[0] = VOCS_AUDIO_LOCATION_WRITE_WITHOUT_RSP_SUPPORT |
                      VOCS_AUDIO_LOCATION_NOTIFY_SUPPORT |
                      VOCS_AUDIO_OUTPUT_DES_WRITE_WITHOUT_RSP_SUPPORT | VOCS_AUDIO_OUTPUT_DES_NOTIFY_SUPPORT;
    vc_mic_param.p_vocs_feature_tbl = vocs_feature;
#endif
#if (APP_LEA_AICS_FOR_VCS_SUPPORT || APP_LEA_AICS_FOR_MICS_SUPPORT)
    uint8_t aics_idx = 0;

#if APP_LEA_AICS_FOR_VCS_SUPPORT
    uint8_t aics_vcs_srv_instance_id = 0;
    uint8_t vcs_id_array[1] = {0};
    aics_vcs_srv_instance_id = aics_idx;
    vcs_id_array[0] = aics_vcs_srv_instance_id;
    vc_mic_param.aics_vcs_num++;
    vc_mic_param.aics_total_num++;
    aics_idx++;
    vc_mic_param.p_aics_vcs_tbl = vcs_id_array;
#endif
#if APP_LEA_AICS_FOR_MICS_SUPPORT
    uint8_t aics_mics_srv_instance_id = 0;
    uint8_t mics_id_array[1] = {0};
    aics_mics_srv_instance_id = aics_idx;
    mics_id_array[0] = aics_mics_srv_instance_id;
    vc_mic_param.aics_mics_num++;
    vc_mic_param.aics_total_num++;
    vc_mic_param.p_aics_mics_tbl = mics_id_array;
#endif
#endif
#if VCP_VOLUME_RENDERER
    vc_mic_param.vcs_enable = true;
#endif
#if MICP_MIC_DEVICE
    vc_mic_param.mics_enable = true;
#endif

    ble_audio_vc_mic_init(&vc_mic_param);

#if APP_LEA_VOCS_SUPPORT
    T_VOCS_PARAM_SET param_vocs;

    param_vocs.set_mask = VOCS_VOLUME_OFFSET_STATE_FLAG | VOCS_AUDIO_LOCATION_FLAG |
                          VOCS_AUDIO_OUTPUT_DES_FLAG;
    param_vocs.volume_offset = 0;
    param_vocs.change_counter = 0;
    if (app_db.csis_cfg == APP_LEA_CSIS_CFG_RANK_1)
    {
        param_vocs.audio_location = AUDIO_LOCATION_FL;
        param_vocs.output_des.p_output_des = (uint8_t *)app_audio_output_des_left;
        param_vocs.output_des.output_des_len = sizeof(app_audio_output_des_left);
    }
    else if (app_db.csis_cfg == APP_LEA_CSIS_CFG_RANK_2)
    {
        param_vocs.audio_location = AUDIO_LOCATION_FR;
        param_vocs.output_des.p_output_des = (uint8_t *)app_audio_output_des_right;
        param_vocs.output_des.output_des_len = sizeof(app_audio_output_des_right);
    }
    else
    {
        param_vocs.audio_location = AUDIO_LOCATION_FL | AUDIO_LOCATION_FR;
        param_vocs.output_des.p_output_des = (uint8_t *)app_audio_output_des;
        param_vocs.output_des.output_des_len = sizeof(app_audio_output_des);
    }
    vocs_set_param(0, &param_vocs);
#endif

#if (APP_LEA_AICS_FOR_VCS_SUPPORT || APP_LEA_AICS_FOR_MICS_SUPPORT)
    T_AICS_INPUT_STATE input_state;
    T_AICS_GAIN_SETTING_PROP gain_prop;
    uint8_t input_type;
    uint8_t input_status;

#if APP_LEA_AICS_FOR_VCS_SUPPORT
    input_type = AUDIO_INPUT_BLUETOOTH;
    input_status = AICS_INPUT_STATUS_ACTIVE;
    input_state.change_counter = 0;
    input_state.gain_mode = AICS_GAIN_MODE_MANUAL;
    input_state.gain_setting = 20;
    input_state.mute = AICS_MUTED;
    gain_prop.gain_setting_max = 30;
    gain_prop.gain_setting_min = -30;
    gain_prop.gain_setting_units = 5;

    aics_set_param(aics_vcs_srv_instance_id, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                   (uint8_t *)&input_state,
                   true);
    aics_set_param(aics_vcs_srv_instance_id, AICS_PARAM_GAIN_SETTING_PROP,
                   sizeof(T_AICS_GAIN_SETTING_PROP),
                   (uint8_t *)&gain_prop, false);
    aics_set_param(aics_vcs_srv_instance_id, AICS_PARAM_INPUT_TYPE, sizeof(uint8_t), &input_type,
                   false);
    aics_set_param(aics_vcs_srv_instance_id, AICS_PARAM_INPUT_STATUS, sizeof(uint8_t), &input_status,
                   false);
    aics_set_param(aics_vcs_srv_instance_id, AICS_PARAM_INPUT_DES, strlen(app_audio_input_des_vcs),
                   (uint8_t *)app_audio_input_des_vcs,
                   false);
#endif

#if APP_LEA_AICS_FOR_MICS_SUPPORT
    input_type = AUDIO_INPUT_MICROPHONE;
    input_status = AICS_INPUT_STATUS_INACTIVE;
    input_state.change_counter = 0;
    input_state.gain_mode = AICS_GAIN_MODE_MANUAL;
    input_state.gain_setting = 20;
    input_state.mute = AICS_NOT_MUTED;
    gain_prop.gain_setting_max = 30;
    gain_prop.gain_setting_min = -30;
    gain_prop.gain_setting_units = 6;
    aics_set_param(aics_mics_srv_instance_id, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                   (uint8_t *)&input_state,
                   true);
    aics_set_param(aics_mics_srv_instance_id, AICS_PARAM_GAIN_SETTING_PROP,
                   sizeof(T_AICS_GAIN_SETTING_PROP),
                   (uint8_t *)&gain_prop, false);
    aics_set_param(aics_mics_srv_instance_id, AICS_PARAM_INPUT_TYPE, sizeof(uint8_t), &input_type,
                   false);
    aics_set_param(aics_mics_srv_instance_id, AICS_PARAM_INPUT_STATUS, sizeof(uint8_t), &input_status,
                   false);
    aics_set_param(aics_mics_srv_instance_id, AICS_PARAM_INPUT_DES, strlen(app_audio_input_des_mics),
                   (uint8_t *)app_audio_input_des_mics, false);
#endif
#endif

#if MICP_MIC_DEVICE
    mics_param.mic_mute = MICS_NOT_MUTE;
    mics_set_param(&mics_param);
#endif

#if VCP_VOLUME_RENDERER
    vcs_param.volume_setting = app_cfg_nv.lea_vcs_vol_setting;
    vcs_param.mute = app_cfg_nv.lea_vcs_mute;
    vcs_param.change_counter = app_cfg_nv.lea_vcs_change_cnt;
    vcs_param.volume_flags = app_cfg_nv.lea_vcs_vol_flag;
    vcs_param.step_size = app_cfg_nv.lea_vcs_step_size;
    vcs_set_param(&vcs_param);
#endif

    ble_audio_cback_register(app_lea_acc_vc_mic_handle_msg);
}

