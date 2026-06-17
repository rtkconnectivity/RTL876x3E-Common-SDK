/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_CHARGING_CASE_CMD_SUPPORT
#include <string.h>
#include "trace.h"
#include "stdlib.h"
#include "app_cmd.h"
#include "app_report.h"
#include "app_cfg_nv.h"
#include "app_charging_case_cmd.h"
#include "app_ble_gap.h"
#include "app_ble_common_adv.h"
#include "app_main.h"
#include "app_audio_policy.h"
#include "app_bt_policy_cfg.h"
#include "app_timer.h"
#include "btm.h"
#include "gap.h"
#include "sysm.h"
//#include "sys_mgr.h"
#include "platform_utils.h"
#include "bt_bond_le.h"
#include "bt_bond_le_sync.h"
#include "app_le_transfer.h"
#if F_APP_GUI_SUPPORT
#include "app_panel_le_db.h"
#endif

typedef enum
{
    APP_TIMER_FIND_CHARGING_CASE,
} T_APP_CMD_TIMER;

#define FIND_CHARGERBOX_VP_PLAY_INTERVAL 1000
#define FP_MAC_ADDR_LEN 6
#define FP_LINK_KEY_LEN 16
#define CHARGECASE_BLE_SCAN_DURATION   (0)

static uint8_t remote_bd_addr[6] = {0};
static uint8_t app_charging_case_timer_id = 0;
static uint8_t timer_idx_find_charging_case = 0;
static T_GAP_REMOTE_ADDR_TYPE remote_addr_type;

T_GAP_CAUSE app_charging_case_create_conn(uint8_t *p_bd_addr, uint8_t addr_type)
{
    T_GAP_CAUSE gap_cause;
    uint8_t init_phys = GAP_PHYS_CONN_INIT_2M_BIT | GAP_PHYS_CONN_INIT_1M_BIT;
    T_GAP_LOCAL_ADDR_TYPE own_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;

    gap_cause = le_connect(init_phys, p_bd_addr, (T_GAP_REMOTE_ADDR_TYPE)addr_type,
                           own_addr_type, CHARGECASE_BLE_SCAN_DURATION);
    return gap_cause;
}

void app_charging_case_handle_le_disconn(uint8_t *bd_addr, uint8_t local_disc_cause)
{
    uint8_t temp_addr[6] = {0};
    // Due to only support one ble link in chargecase demo, so if rcv disconn event, dut has to reconnect paired headset except for recieving the cmd from headset about enabling ble adv.
    APP_PRINT_INFO3("app_charging_case_handle_le_disconn: bd_addr %s, headset bd %s, disc cause %d",
                    TRACE_BDADDR(bd_addr), TRACE_BDADDR(app_cfg_nv.chargecase_remote_bd),
                    local_disc_cause);

    if (local_disc_cause == LE_LOCAL_DISC_CAUSE_CHARGING_CASE_OTA)
    {
        // do not reconnect
    }
    else
    {
        if (memcmp(temp_addr, app_cfg_nv.chargecase_remote_bd, 6) != 0)
        {
            app_charging_case_create_conn(app_cfg_nv.chargecase_remote_bd, GAP_REMOTE_ADDR_LE_PUBLIC);
        }
    }
}

static void app_charging_case_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO1("app_charging_case_timeout_cb: timer_evt %d", timer_evt);
    switch (timer_evt)
    {
    case APP_TIMER_FIND_CHARGING_CASE:
        {
            T_APP_AUDIO_TONE_TYPE tone_type = (T_APP_AUDIO_TONE_TYPE)param;
            app_audio_tone_type_play(tone_type, false, false);
        }
        break;

    default:
        break;
    }
}

void app_charging_case_tone_play(T_APP_AUDIO_TONE_TYPE tone_type)
{
    app_audio_tone_type_play(tone_type, false, false);
    app_start_timer(&timer_idx_find_charging_case, "find_charging_case", app_charging_case_timer_id,
                    APP_TIMER_FIND_CHARGING_CASE, tone_type, true,
                    FIND_CHARGERBOX_VP_PLAY_INTERVAL);
}

void app_charging_case_tone_stop(T_APP_AUDIO_TONE_TYPE tone_type)
{
    app_audio_tone_type_cancel(tone_type, false);
    app_stop_timer(&timer_idx_find_charging_case);
}

void app_charging_case_link_key_random_gen(uint8_t *p_buf)
{
    uint8_t *p = p_buf;
    LE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
    LE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
    LE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
    LE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
}

static bool app_charging_case_set_fast_pair_info(uint8_t *p_link_key, uint8_t addr_type)
{
    APP_PRINT_INFO1("app_charging_case_set_fast_pair_info: link_key %b",
                    TRACE_BINARY(16, p_link_key));
    bool result = false;

    T_BT_LE_DEV_DATA *p_info = NULL;
    T_BT_LE_DEV_INFO *p_dev_info = NULL;
    p_info = calloc(1, sizeof(T_BT_LE_DEV_DATA));

    if (p_info)
    {
        p_dev_info = &p_info->dev_info;
        p_info->bond_info.remote_bd_type = addr_type;
        p_info->bond_info.local_bd_type = GAP_LOCAL_ADDR_LE_PUBLIC;
        memcpy(p_info->bond_info.remote_bd, remote_bd_addr, FP_MAC_ADDR_LEN);
        memcpy(p_info->bond_info.local_bd, app_cfg_nv.bud_local_addr, 6);

        p_dev_info->flags = LE_KEY_STORE_LOCAL_LTK_BIT | LE_KEY_STORE_REMOTE_LTK_BIT;
        p_dev_info->local_ltk[0] = 28;
        memcpy(&p_dev_info->local_ltk[4], p_link_key, FP_LINK_KEY_LEN);
        p_dev_info->local_ltk[30] = FP_LINK_KEY_LEN;
        p_dev_info->local_ltk[31] = GAP_KEY_LE_LOCAL_LTK;
        p_dev_info->remote_ltk[0] = 28;
        memcpy(&p_dev_info->remote_ltk[4], p_link_key, FP_LINK_KEY_LEN);
        p_dev_info->remote_ltk[30] = FP_LINK_KEY_LEN;
        p_dev_info->remote_ltk[31] = GAP_KEY_LE_REMOTE_LTK;

        if (bt_le_dev_info_set(&p_info->bond_info, &p_info->dev_info))
        {
            result = true;
        }

        free(p_info);
    }

    return result;
}

void app_charging_case_handle_auto_pair(uint8_t *bd_addr)
{
    uint8_t temp[6] = {0};
    T_APP_LE_LINK *p_link;

    if (!bd_addr)
    {
        return;
    }

    APP_PRINT_TRACE2("app_charging_case_handle_auto_pair: new auto pair device, bd %s,  old bd %s",
                     TRACE_BDADDR(bd_addr), TRACE_BDADDR(app_cfg_nv.chargecase_remote_bd));

    if (memcmp(bd_addr, app_cfg_nv.chargecase_remote_bd, 6) == 0)
    {
        return;
    }

    if (memcmp(app_cfg_nv.chargecase_remote_bd, temp, 6) != 0)
    {
        p_link = app_link_find_le_link_by_addr(app_cfg_nv.chargecase_remote_bd);
        if (p_link != NULL)
        {
            app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_CHARGING_CASE_RECONNECT_PRI_BUD);
        }
    }

    memcpy(app_cfg_nv.chargecase_remote_bd, bd_addr, 6);
}

void app_charging_case_cmd_handle(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                  uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    APP_PRINT_TRACE3("app_charging_case_cmd_handle: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);

    switch (cmd_id)
    {
    case CMD_CHARGER_CASE_FIND_CHARGERBOX:
        {
            uint8_t find_status = cmd_ptr[2];
            if (find_status)
            {
                app_charging_case_tone_play(TONE_APT_EQ_9);
            }
            else
            {
                app_charging_case_tone_stop(TONE_APT_EQ_9);
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_CHARGER_CASE_REPORT_STATUS:
        {
            //charging case auto pair when bud in case
            if (cmd_path == CMD_PATH_UART)
            {
                T_LE_BOND_ENTRY *p_entry = NULL;
                uint8_t local_bd_addr[6] = {0};
                uint8_t temp_buff[24] = {0};
                uint8_t key_len;
                uint8_t link_key[16] = {0};
                bool result = true;

                memcpy(remote_bd_addr, &cmd_ptr[2], 6);
                remote_addr_type = (T_GAP_REMOTE_ADDR_TYPE)cmd_ptr[8];
                memcpy(local_bd_addr, app_cfg_nv.bud_local_addr, 6);
                uint8_t local_bd_type = GAP_LOCAL_ADDR_LE_PUBLIC;
                temp_buff[1] = GAP_LOCAL_ADDR_LE_PUBLIC;
                memcpy(&temp_buff[2], local_bd_addr, 6);

                p_entry = bt_le_find_key_entry(remote_bd_addr, remote_addr_type, local_bd_addr, local_bd_type);

                if (p_entry)
                {
                    T_APP_LE_LINK *p_link = app_link_find_le_link_by_addr(remote_bd_addr);
                    temp_buff[0] = CHARGING_CASE_BOND_STATE_BONED;

                    if (bt_le_get_dev_ltk(p_entry, false, &key_len, link_key))
                    {
                        memcpy(&temp_buff[8], link_key, key_len);
                    }

                    if (p_link == NULL)
                    {
                        app_charging_case_create_conn(remote_bd_addr, remote_addr_type);
                    }
                }
                else
                {
                    temp_buff[0] = CHARGING_CASE_BOND_STATE_NONE;
                    app_charging_case_link_key_random_gen(link_key);

                    if (app_charging_case_set_fast_pair_info(link_key, remote_addr_type))
                    {
                        memcpy(&temp_buff[8], link_key, FP_LINK_KEY_LEN);
                        app_charging_case_handle_auto_pair(remote_bd_addr);
                    }
                    else
                    {
                        APP_PRINT_TRACE0("app_charging_case_cmd_handle: set pair info fail");
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        result = false;
                    }
                }

                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                if (result)
                {
                    app_report_charging_case_cmd(CMD_PATH_UART, CMD_CHARGER_CASE_LE_LINK_INFO, 0, temp_buff,
                                                 sizeof(temp_buff));
                }
            }
        }
        break;

    case CMD_CHARGER_CASE_BUD_AUTO_PAIR_SUC:
        {
            if (app_charging_case_create_conn(remote_bd_addr, remote_addr_type))
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        break;
    }
}

void app_charging_case_headset_event_handler(uint16_t conn_handle, uint8_t *p_data,
                                             uint16_t data_len)
{
    APP_PRINT_INFO3("app_charging_case_headset_event_handler: conn_handle 0x%x, data_len %d, data %b",
                    conn_handle, data_len, TRACE_BINARY(data_len, p_data));
    uint8_t rx_seqn;
    uint16_t event_len;
    bool ack_flag = true;
    T_CMD_SET_STATUS result  = CMD_SET_STATUS_COMPLETE;

    if (data_len > 5)
    {
        if (p_data[0] == CMD_SYNC_BYTE)
        {
            rx_seqn = p_data[1];
            event_len = (p_data[2] | (p_data[3] << 8)) + 4; //sync_byte, seqn, length
            if ((data_len == event_len) && (rx_seqn != 0))
            {
                uint16_t event_id = (uint16_t)(p_data[4] | (p_data[5] << 8));
                switch (event_id)
                {
                case EVENT_ACK:
                    {
                        ack_flag = false;
                    }
                    break;
                case EVENT_CHARGER_CASE_CONN_STATUS:
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
                        if (!p_link)
                        {
                            APP_PRINT_ERROR0("app_charging_case_headset_event_handler: le link not found");
                            return;
                        }

                        if (p_data[6] == CHARGING_CASE_CMD_BATTERY_STATUS)
                        {
#if F_GUI_CHARGEBOX_DEMO
                            app_panel_le_db_set_bettery_level(p_link->bd_addr, &p_data[7]);
#endif
                        }
                        else if (p_data[6] == CHARGING_CASE_CMD_BUD_INFO)
                        {
#if F_GUI_CHARGEBOX_DEMO
                            app_panel_le_db_set_remote_name(p_link->bd_addr, &p_data[7], 40);
#endif
                        }
                        else if (p_data[6] == CHARGING_CASE_CMD_CONNECT_STATUS)
                        {
#if F_GUI_CHARGEBOX_DEMO
                            app_panel_le_db_set_remote_link_status(p_link->bd_addr, p_data[7]);
#endif
                        }
                    }
                    break;
                case EVENT_CHARGER_CASE_SET_ADV:
                    {
                        if (p_data[6] == CHARGING_CASE_CMD_ENTER_OTA)
                        {
                            T_APP_LE_LINK *p_link;

                            p_link = app_link_find_le_link_by_conn_handle(conn_handle);
                            if (p_link != NULL)
                            {
                                app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_CHARGING_CASE_OTA);
                            }

                            if (app_db.device_state != APP_DEVICE_STATE_OFF)
                            {
                                if (p_data[7] == CHARGING_CASE_ENABLE_LE_ADV)
                                {
                                    app_ble_common_adv_start(0);
                                }
                                else if (p_data[7] == CHARGING_CASE_ENABLE_LEGACY_ADV)
                                {
                                    bt_device_mode_set(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
                                }
                                else
                                {
                                    APP_PRINT_INFO1("app_charging_case_cmd_handler: error path %d", p_data[7]);
                                }
                            }
                        }
                    }
                    break;

                case EVENT_CHARGER_CASE_FIND_CHARGERBOX:
                    {
                        uint8_t find_status = p_data[6];

                        if (find_status)
                        {
                            app_charging_case_tone_play(TONE_APT_EQ_9);
                        }
                        else
                        {
                            app_charging_case_tone_stop(TONE_APT_EQ_9);
                        }
                    }
                    break;
                case EVENT_AVRCP_REPORT_ELEMENT_ATTR:
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
                        if (!p_link)
                        {
                            APP_PRINT_ERROR0("app_charging_case_headset_event_handler: le link not found");
                            return;
                        }

                        if (p_data[6] == 0)
                        {
                            uint32_t attribute_id = (uint32_t)(p_data[7] | (p_data[8] << 8) | (p_data[9] << 16) |
                                                               (p_data[10] << 24));
                            uint16_t attribute_length = (p_data[13] | (p_data[14] << 8));

                            APP_PRINT_INFO2("app_charging_case_headset_event_handler: attribute_id %d, attribute_length %d",
                                            attribute_id, attribute_length);

#if F_GUI_CHARGEBOX_DEMO
                            if (AVRCP_TITLE == attribute_id)
                            {
                                app_tb_set_title(&p_data[15], attribute_length);
                            }
                            else if (AVRCP_NAME_OF_ARTIST == attribute_id)
                            {
                                app_tb_set_artist(&p_data[15], attribute_length);
                            }
                            else if (AVRCP_NAME_OF_ALBUM == attribute_id)
                            {
                                app_tb_set_album(&p_data[15], attribute_length);
                            }
                            else
                            {
                                //not handle other msg
                            }
#endif
                        }
                        else
                        {
                            APP_PRINT_ERROR0("app_charging_case_headset_event_handler: not single data");
                        }

                    }
                    break;
                case EVENT_VOLUME_SYNC:
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
                        if (!p_link)
                        {
                            APP_PRINT_ERROR0("app_charging_case_headset_event_handler: le link not found");
                            return;
                        }
#if F_GUI_CHARGEBOX_DEMO
                        app_panel_le_db_set_volume(p_link->bd_addr, p_data[12]);
#endif
                    }
                    break;
                case EVENT_LISTENING_STATE_STATUS:
                    {
                        T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);
                        if (!p_link)
                        {
                            APP_PRINT_ERROR0("app_charging_case_headset_event_handler: le link not found");
                            return;
                        }
#if F_GUI_CHARGEBOX_DEMO
                        app_panel_le_db_set_anc_status(p_link->bd_addr, p_data[6]);
#endif
                    }
                    break;
                default:
                    result = CMD_SET_STATUS_UNKNOW_CMD;
                    break;
                }

                if (ack_flag)
                {
                    app_le_transfer_send_ack(event_id, result);
                }
            }
        }
    }
}

void app_charging_case_send_cmd(uint16_t cmd_id)
{
    uint8_t temp_buff[3] = {0};
    bool cmd_flag = true;
    APP_PRINT_INFO1("app_charging_case_send_cmd: cmd_id %d", cmd_id);

    switch (cmd_id)
    {
    case AD2B_CMD_FACTORY_RESET:
        {
            temp_buff[0] = AD2B_CMD_FACTORY_RESET;
            temp_buff[1] = 0x55;
        }
        break;

    case AD2B_CMD_OPEN_CASE:
        {
            temp_buff[0] = AD2B_CMD_OPEN_CASE;
        }
        break;

    case AD2B_CMD_CLOSE_CASE:
        {
            temp_buff[0] = AD2B_CMD_CLOSE_CASE;
        }
        break;

    case AD2B_CMD_ENTER_PAIRING:
        {
            temp_buff[0] = AD2B_CMD_ENTER_PAIRING;
        }
        break;

    case AD2B_CMD_ENTER_DUT_MODE:
        {
            temp_buff[0] = AD2B_CMD_ENTER_DUT_MODE;
            temp_buff[1] = 0xAA;
        }
        break;

    case AD2B_CMD_CASE_CHARGER:
        {
            temp_buff[0] = AD2B_CMD_CASE_CHARGER;
        }
        break;

    default:
        cmd_flag = false;
        break;
    }

    if (cmd_flag)
    {
        app_report_charging_case_cmd(CMD_PATH_UART, CMD_ADP_CMD_CONTROL, 0, temp_buff, sizeof(temp_buff));
        app_report_charging_case_cmd(CMD_PATH_UART, CMD_ADP_CMD_CONTROL, 2, temp_buff, sizeof(temp_buff));
    }
}

static void app_charging_case_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    uint8_t temp_addr[6] = {0};

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            if (memcmp(temp_addr, app_cfg_nv.chargecase_remote_bd, 6))
            {
                app_charging_case_create_conn(app_cfg_nv.chargecase_remote_bd, GAP_REMOTE_ADDR_LE_PUBLIC);
            }
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {

        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_charging_case_dm_cback: event_type 0x%04x", event_type);
    }
}

void app_charging_case_init(void)
{
    sys_mgr_cback_register(app_charging_case_dm_cback);
    app_timer_reg_cb(app_charging_case_timeout_cb, &app_charging_case_timer_id);
}
#endif

