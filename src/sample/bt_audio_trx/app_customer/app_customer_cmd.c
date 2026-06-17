/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_customer_cmd.h"
#include "app_customer_audio_policy.h"
#include "app_customer_bt.h"
#include "app_cmd.h"
#include "trace.h"
#include "test_mode.h"
#include "app_device.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "bt_bond.h"
#include "os_mem.h"
#include "cfg_item_api.h"
#include "aw87390_driver_interface.h"
#include "pm.h"

#include "aw_customer_driver_interface.h"
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
#include "app_music_ctrl.h"
#endif
#include "log_api.h"


void app_customer_handle_cmd_set(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                                 uint8_t rx_seqn, uint8_t app_idx)
{
    uint16_t cmd_id;
    uint8_t  ack_pkt[3];

    cmd_id     = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    ack_pkt[0] = cmd_ptr[0];
    ack_pkt[1] = cmd_ptr[1];
    ack_pkt[2] = CMD_SET_STATUS_COMPLETE;

    APP_PRINT_TRACE4("app_customer_handle_cmd_set++: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u, rx_seqn 0x%02x",
                     cmd_id, cmd_len, cmd_path, rx_seqn);

    switch (cmd_id)
    {
    /* customer customer command handler*/
    case CMD_XM_ENTER_HCI_MODE:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            switch_into_hci_mode();
        }
        break;

    case CMD_XM_DEVICE_REBOOT:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_cfg_store_all();
            app_device_reboot(1000, RESET_ALL);
        }
        break;

    case CMD_XM_MMI:
        {
            bool handle = true;

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            switch (cmd_ptr[2])
            {
#if F_APP_MUSIC_LOCAL_PLAY_SUPPORT
            case XM_MMI_MODE_LOCAL_PLAY:
            case XM_MMI_MODE_A2DP_SOURCE:
            case XM_MMI_SEAMLESS_SWITCH_LOCAL_PLAY:
            case XM_MMI_SEAMLESS_SWITCH_A2DP_SRC:
            case XM_MMI_ENABLE_REPORT_PLAY_TIME:
            case XM_MMI_DISABLE_REPORT_PLAY_TIME:
                app_music_handle_xmmi_cmd(app_idx, (T_CMD_PATH)cmd_path, cmd_ptr, cmd_len, ack_pkt);
                break;
#endif

            case XM_MMI_BT_BOND_CLEAR_ALL:
                {
                    bt_bond_clear();
                }
                break;
            default:
                handle = false;
                break;
            }

            if (handle == true)
            {
                APP_PRINT_TRACE1("CMD_XM_MMI: %x", cmd_ptr[2]);
            }
        }
        break;

#if (TARGET_RTL8773DO || TARGET_RTL8773DFL)
    case CMD_XM_SET_XTAL:
    case CMD_XM_SET_GAIN_K:
    case CMD_XM_SET_FLATNESS_K:
        {
            ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#else
    case CMD_XM_SET_XTAL:
        {
            if (cmd_ptr[2] > 0x7F)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            else if (cfg_update_xtal(cmd_ptr[2]) != true)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_SET_GAIN_K:
        {
            if (cmd_ptr[2] > 0x0C && cmd_ptr[2] < 0xF4)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            else if (cfg_update_txgaink(cmd_ptr[2]) != true)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_SET_FLATNESS_K:
        {
            if ((cmd_ptr[2] > 0x08 && cmd_ptr[2] < 0xF8) ||
                (cmd_ptr[3] > 0x08 && cmd_ptr[3] < 0xF8) ||
                (cmd_ptr[4] > 0x08 && cmd_ptr[4] < 0xF8) ||
                (cmd_ptr[5] > 0x08 && cmd_ptr[5] < 0xF8))
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }
            else if (cfg_update_txgain_flatk(cmd_ptr[2] * 4, cmd_ptr[3] * 4,
                                             cmd_ptr[4] * 4, cmd_ptr[5] * 4) != true)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif
    case CMD_XM_GET_FACTORY_RESET:
        {
            uint8_t freset_done;

            freset_done = app_cfg_nv.factory_reset_done;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            app_report_event(cmd_path, EVENT_XM_FACTORY_RESET_STATUS, app_idx, &freset_done,
                             sizeof(freset_done));
        }
        break;

    case CMD_XM_GATT_DATA_TRANSFER:
        {
        }
        break;

    case CMD_XM_MUSIC:
    case CMD_XM_SET_VOLUME_LEVEL:
#if F_APP_CUSTOMER_AUDIO_POLICY_SUPPORT
    case CMD_XM_TTS:
        app_customer_audio_handle_cmd(app_idx, (T_CMD_PATH)cmd_path, cmd_ptr, cmd_len, ack_pkt);
#endif
        break;

    case CMD_XM_AUDIO_TEST_GET_PA_ID:
        {
            uint8_t chip_id = 0;

            if (aw87390_get_chip_id(&chip_id) != true)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            }
            else
            {
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                app_report_event(cmd_path, EVENT_XM_AUDIO_TEST_PA_ID, app_idx, (uint8_t *)&chip_id,
                                 sizeof(chip_id));
            }
        }
        break;

    case CMD_XM_AUDIO_TEST_BYPASS:
        {
            if (cmd_ptr[2] == 0) /*enable DSP algo*/
            {
#if F_APP_CUSTOMER_AUDIO_POLICY_SUPPORT
                app_customer_audio_all_bypassed(false);
#endif
            }
            else if (cmd_ptr[2] == 1) /*bypass DSP algo*/
            {
#if F_APP_CUSTOMER_AUDIO_POLICY_SUPPORT
                app_customer_audio_all_bypassed(true);
#endif
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_SET_PA_MODE:
        {
            uint8_t aw_mode = cmd_ptr[2];
            set_customer_aw_mdoe_type(aw_mode);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_DISABLE_VERBOSE_LOG:
        {
            uint8_t log_type = cmd_ptr[2];
            bool log_key = cmd_ptr[3];
            uint8_t module = cmd_ptr[3];
            uint8_t level =  cmd_ptr[4];
            bool turnon = cmd_ptr[5];
            if (log_type == 1)
            {
                if (log_key)
                {
                    log_enable_set(true);
//                    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, LEVEL_TRACE, 1);
//                    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, LEVEL_INFO, 1);
//                    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, LEVEL_WARN, 1);
//                    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, LEVEL_ERROR, 1);
                    //log_module_trace_set(MODULE_LOWERSTACK, LEVEL_ERROR, 1);
                }
                else
                {
                    log_enable_set(false);
                }
            }
            else if (log_type == 2)
            {
                log_module_trace_set((T_MODULE_ID)module, level, turnon);
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_XM_TEST:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            switch (cmd_ptr[2])
            {
            case XM_TEST_START_REQ_RRAME...XM_TEST_TONE:
//                app_customer_audio_handle_test_cmd(app_idx, (T_CMD_PATH)cmd_path, cmd_ptr, cmd_len, ack_pkt);
                break;
            case XM_TEST_MEM:
                {
                    os_mem_peek_printf();
                }
                break;
            default:
                break;
            }
        }
        break;

    case CMD_XM_SET_CPU_FREQ:
        {
            struct
            {
                uint16_t cmd_id;
                uint8_t  freq_mhz;
            } __attribute__((packed)) *cmd = (typeof(cmd))cmd_ptr;

            uint8_t freq_handle = 0;
            uint32_t actual_mhz = 0;
            pm_cpu_freq_req(&freq_handle, cmd->freq_mhz, &actual_mhz);

            APP_PRINT_TRACE2("app_customer_handle_cmd_set: target freq %d, actual freq %d", cmd->freq_mhz,
                             actual_mhz);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;


    case CMD_XM_BT_SET_ADDR:
    case CMD_XM_SPP_DATA_TRANSFER:
    case CMD_XM_SNIFF_MODE_CTRL:
    case CMD_INQUIRY:
    case CMD_BT_HFP_SCO_MAG:
    case CMD_XM_SET_MODE:
    case CMD_XM_USER_CFM_REQ:
        app_customer_bt_handle_cmd(app_idx, (T_CMD_PATH)cmd_path, cmd_ptr, cmd_len, ack_pkt);
        break;

    default:
        break;

    }
}


