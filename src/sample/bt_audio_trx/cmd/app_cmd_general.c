/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "string.h"
#include "audio_volume.h"
#include "voice_prompt.h"
#include "fmc_api.h"
#include "feature_check.h"
#include "patch_header_check.h"
#include "gap_le.h"
#include "app_audio_cfg.h"
#include "app_audio_policy.h"
#include "app_bt_policy_cfg.h"
#include "app_cmd.h"

#include "app_cmd_general.h"
#include "app_main.h"
#include "app_mmi.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_ota.h"
#include "app_iap.h"
#include "app_ble_common_adv.h"

#if F_APP_USER_EQ_SUPPORT
#include "app_eq_cfg.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_bond_manager.h"
#include "app_tri_dongle_usb_hid_ctrl.h"
#endif

#define DEVICE_BUD_SIDE_BOTH        2

//for CMD_INFO_REQ
#define CMD_INFO_STATUS_VALID       0x00
#define CMD_INFO_STATUS_ERROR       0x01
#define CMD_SUPPORT_VER_CHECK_LEN   3

enum
{
    ENABLE_AUTO_ACCEPT_ACL_ACF_REQ  = 0x00,
    ENABLE_AUTO_REJECT_ACL_ACF_REQ  = 0x01,
    FORWARD_ACL_ACF_REQ_TO_HOST     = 0x02,
};

// for get FW version type
typedef enum
{
    GET_PRIMARY_FW_VERSION           = 0x00,
    GET_SECONDARY_FW_VERSION         = 0x01,
    GET_PRIMARY_OTA_FW_VERSION       = 0x02,
    GET_SECONDARY_OTA_FW_VERSION     = 0x03,
} T_CMD_GET_FW_VER_TYPE;

enum
{
    SYSTEM_CONFIGS          = 0x00,
    ROM_PATCH_IMAGE         = 0x01,
    APP_IMAGE               = 0x02,
    DSP_SYSTEM_IMAGE        = 0x03,
    DSP_APP_IMAGE           = 0x04,
    FTL_DATA                = 0x05,
    ANC_IMAGE               = 0x06,
    LOG_PARTITION           = 0x07,
    CORE_DUMP_PARTITION     = 0x08,
};

/* Define application support status */
#define SNK_SUPPORT_GET_SET_LE_NAME                 1
#define SNK_SUPPORT_GET_SET_BR_NAME                 1
#define SNK_SUPPORT_GET_SET_VP_LANGUAGE             1
#define SNK_SUPPORT_GET_BATTERY_LEVEL               1



#define FLASH_ALL                   0xFF
#define ALL_DUMP_IMAGE_MASK         ((0x01 << SYSTEM_CONFIGS) | (0x01 << ROM_PATCH_IMAGE) | (0x01 << APP_IMAGE) \
                                     | (0x01 << DSP_SYSTEM_IMAGE) | (0x01 << DSP_APP_IMAGE) \
                                     | (0x01 << FTL_DATA) |(0x01 << CORE_DUMP_PARTITION))


enum
{
//for CMD_GET_FLASH_DATA and EVENT_REPORT_FLASH_DATA
    START_TRANS             = 0x00,
    CONTINUE_TRANS          = 0x01,
    SUPPORT_IMAGE_TYPE      = 0x02,
};

enum
{
    TRANS_DATA_INFO             = 0x00,
    CONTINUE_TRANS_DATA         = 0x01,
    END_TRANS_DATA              = 0x02,
    SUPPORT_IMAGE_TYPE_INFO     = 0x03,
};

T_SNK_CAPABILITY app_cmd_get_system_capability(void)
{
    T_SNK_CAPABILITY snk_capability;

    memset(&snk_capability, 0, sizeof(T_SNK_CAPABILITY));
    snk_capability.snk_support_get_set_le_name = SNK_SUPPORT_GET_SET_LE_NAME;
    snk_capability.snk_support_get_set_br_name = SNK_SUPPORT_GET_SET_BR_NAME;
    snk_capability.snk_support_get_set_vp_language = SNK_SUPPORT_GET_SET_VP_LANGUAGE;
    snk_capability.snk_support_get_battery_info = SNK_SUPPORT_GET_BATTERY_LEVEL;
    snk_capability.snk_support_ota = true;

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
    {
        snk_capability.snk_support_change_channel = 1;
        snk_capability.snk_support_get_set_rws_state = 1;
    }
    snk_capability.snk_support_get_set_apt_state = false;
    snk_capability.snk_support_get_set_eq_state = true;
    snk_capability.snk_support_get_set_vad_state = false;

    snk_capability.snk_support_ansc = false;
    snk_capability.snk_support_vibrator = false;
    snk_capability.snk_support_gaming_mode = false;

    snk_capability.snk_support_gaming_mode_eq = eq_utils_num_get(SPK_SW_EQ, GAMING_MODE) >= 1;
    snk_capability.snk_support_anc_eq = eq_utils_num_get(SPK_SW_EQ, ANC_MODE) >= 1;
    snk_capability.snk_support_multilink_support = app_bt_policy_cfg_nv.enable_multi_link;
    snk_capability.snk_support_phone_set_anc_eq = false;
    snk_capability.snk_support_new_report_bud_status_flow = true;

#if F_APP_USER_EQ_SUPPORT
    if (app_eq_cfg.user_eq_spk_eq_num != 0 || app_eq_cfg.user_eq_mic_eq_num != 0)
    {
        snk_capability.snk_support_user_eq = true;
    }
#endif

    snk_capability.snk_support_multilink_support = app_bt_policy_cfg_nv.enable_multi_link;

#if F_APP_DATA_CAPTURE_SUPPORT
    snk_capability.snk_support_data_capture = 1;
    snk_capability.snk_support_3bin_scenario = 1;
#endif

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (eq_utils_num_get(SPK_SW_EQ, VOICE_SPK_MODE) != 0)
    {
        snk_capability.snk_support_voice_eq = 1;
    }
#endif

#if F_APP_AUDIO_VOICE_SPK_EQ_INDEPENDENT_CFG
    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
    {
        snk_capability.snk_support_spk_eq_independent_cfg = 1;
    }
#endif

#if F_APP_AUDIO_VOICE_SPK_EQ_COMPENSATION_CFG
    snk_capability.snk_support_spk_eq_compensation_cfg = 1;
#endif

#if F_APP_CHARGE_CASE_DEMO_SUPPORT
    snk_capability.snk_support_charging_case = 1;
#endif

#if F_TRANS_UPDATE_FILE_SUPPORT
    snk_capability.snk_support_wallpaper_update = 1;
#endif

    return snk_capability;
}


//T_FLASH_DATA initialization
void app_flash_data_set_param(uint8_t flash_type, uint8_t cmd_path, uint8_t app_idx)
{
    app_cmd_mgr.flash.type = flash_type;
    app_cmd_mgr.flash.start_addr = 0x800000;
    app_cmd_mgr.flash.size = 0x00;

    switch (flash_type)
    {
    case FLASH_ALL:
        {
            app_cmd_mgr.flash.start_addr = 0x800000;
            app_cmd_mgr.flash.size = 0x100000;
        }
        break;

    case SYSTEM_CONFIGS:
        {
            app_cmd_mgr.flash.start_addr = flash_partition_addr_get(PARTITION_FLASH_OCCD);
            app_cmd_mgr.flash.size = flash_partition_size_get(PARTITION_FLASH_OCCD) & 0x00FFFFFF;
        }
        break;

    case ROM_PATCH_IMAGE:
        {
            app_cmd_mgr.flash.start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_MCUPATCH);
            app_cmd_mgr.flash.size = get_bank_size_by_img_id(IMG_MCUPATCH);
        }
        break;

    case APP_IMAGE:
        {
            app_cmd_mgr.flash.start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_MCUAPP);
            app_cmd_mgr.flash.size = get_bank_size_by_img_id(IMG_MCUAPP);
        }
        break;

    case DSP_SYSTEM_IMAGE:
        {
            app_cmd_mgr.flash.start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_DSPSYSTEM);
            app_cmd_mgr.flash.size = get_bank_size_by_img_id(IMG_DSPSYSTEM);
        }
        break;

    case DSP_APP_IMAGE:
        {
            app_cmd_mgr.flash.start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_DSPAPP);
            app_cmd_mgr.flash.size = get_bank_size_by_img_id(IMG_DSPAPP);
        }
        break;

    case FTL_DATA:
        {
            app_cmd_mgr.flash.start_addr = flash_partition_addr_get(PARTITION_FLASH_FTL);
            app_cmd_mgr.flash.size = flash_partition_size_get(PARTITION_FLASH_FTL) & 0x00FFFFFF;
        }
        break;

    case ANC_IMAGE:
        {
            app_cmd_mgr.flash.start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_ANC);
            app_cmd_mgr.flash.size = get_bank_size_by_img_id(IMG_ANC);
        }
        break;

    case LOG_PARTITION:
        {
            //add later;
        }
        break;

    case CORE_DUMP_PARTITION:
        {
            app_cmd_mgr.flash.start_addr = flash_partition_addr_get(PARTITION_FLASH_HARDFAULT_RECORD);
            app_cmd_mgr.flash.size = flash_partition_size_get(PARTITION_FLASH_HARDFAULT_RECORD);
        }
        break;

    default:
        break;
    }

    app_cmd_mgr.flash.start_addr_tmp = app_cmd_mgr.flash.start_addr;

    //report TRANS_DATA_INFO param
    uint8_t paras[10];

    paras[0] = TRANS_DATA_INFO;
    paras[1] = app_cmd_mgr.flash.type;

    paras[2] = (uint8_t)(app_cmd_mgr.flash.size);
    paras[3] = (uint8_t)(app_cmd_mgr.flash.size >> 8);
    paras[4] = (uint8_t)(app_cmd_mgr.flash.size >> 16);
    paras[5] = (uint8_t)(app_cmd_mgr.flash.size >> 24);

    paras[6] = (uint8_t)(app_cmd_mgr.flash.start_addr);
    paras[7] = (uint8_t)(app_cmd_mgr.flash.start_addr >> 8);
    paras[8] = (uint8_t)(app_cmd_mgr.flash.start_addr >> 16);
    paras[9] = (uint8_t)(app_cmd_mgr.flash.start_addr >> 24);

    app_report_event(cmd_path, EVENT_REPORT_FLASH_DATA, app_idx, paras, sizeof(paras));
}

void app_cmd_get_fw_version(uint8_t *p_data)
{
    uint8_t temp_buff[13];
    T_IMG_HEADER_FORMAT *p_app_header = (T_IMG_HEADER_FORMAT *)flash_cur_bank_img_header_addr_get(
                                            FLASH_IMG_MCUAPP);
    T_IMG_HEADER_FORMAT *p_patch_header = (T_IMG_HEADER_FORMAT *)flash_cur_bank_img_header_addr_get(
                                              FLASH_IMG_SYSPATCH);
    T_PATCH_IMG_VER_FORMAT *p_patch_img_ver = (T_PATCH_IMG_VER_FORMAT *) & (p_patch_header->git_ver);

    T_IMG_HEADER_FORMAT *p_app_ui_header = (T_IMG_HEADER_FORMAT *)flash_cur_bank_img_header_addr_get(
                                               FLASH_IMG_MCUCONFIG);
    T_APP_UI_IMG_VER_FORMAT *p_app_ui_ver = (T_APP_UI_IMG_VER_FORMAT *) & (p_app_ui_header->git_ver);

    temp_buff[0] = p_app_header->git_ver.sub_version._version_major;
    temp_buff[1] = p_app_header->git_ver.sub_version._version_minor;
    temp_buff[2] = p_app_header->git_ver.sub_version._version_revision;

    // currently 5 bits, must be 0
    // temp_buff[3] = p_app_header->git_ver.sub_version._version_reserve >> 8;
    temp_buff[3] = p_app_header->git_ver.sub_version._version_reserve & 0xFF;
    temp_buff[4] = p_app_header->git_ver.sub_version._version_reserve >> 8;

    temp_buff[5] = p_patch_img_ver->ver_major;
    temp_buff[6] = p_patch_img_ver->ver_minor;
    temp_buff[7] = p_patch_img_ver->ver_revision >> 8;
    temp_buff[8] = p_patch_img_ver->ver_revision;

    temp_buff[9] = p_app_ui_ver->ver_reserved;
    temp_buff[10] = p_app_ui_ver->ver_revision;
    temp_buff[11] = p_app_ui_ver->ver_minor;
    temp_buff[12] = p_app_ui_ver->ver_major;

    memcpy((void *)p_data, (void *)&temp_buff, 13);
}


static bool app_cmd_distribute_payload(uint8_t *buf, uint16_t len)
{
    uint8_t module_type = buf[0];
    uint8_t msg_type    = buf[1];
    T_CMD_PARSE_CBACK_ITEM *p_item;

    p_item = (T_CMD_PARSE_CBACK_ITEM *)app_cmd_mgr.cmd_parse.cb_list.p_first;

    while (p_item != NULL)
    {
        if (p_item->module_type == module_type)
        {
            p_item->parse_cback(msg_type, buf + 2, len - 2);

            return true;
        }

        p_item = p_item->p_next;
    }

    return false;
}

static bool app_cmd_compose_payload(uint8_t *data, uint16_t data_len)
{
    static uint8_t cur_seq = 0;
    static uint8_t *buf = NULL;
    static uint16_t buf_offset = 0;

    uint8_t type = data[1];
    uint8_t seq = data[2];
    uint16_t total_len = data[3] + (data[4] << 8);

    data += 5;
    data_len -= 5;

    if (cur_seq != seq || data_len == 0)
    {
        cur_seq = 0;
        buf_offset = 0;

        if (buf)
        {
            free(buf);
            buf = NULL;
        }

        return CMD_SET_STATUS_PROCESS_FAIL;
    }

    if (type == PKT_TYPE_SINGLE || type == PKT_TYPE_START)
    {
        cur_seq = 0;
        buf_offset = 0;

        if (buf)
        {
            free(buf);
            buf = NULL;
        }

        buf = malloc(sizeof(uint8_t) * total_len);
    }

    memcpy(buf + buf_offset, data, data_len);

    if (type == PKT_TYPE_SINGLE || type == PKT_TYPE_END)
    {
        app_cmd_distribute_payload(buf, total_len);
        free(buf);
        buf = NULL;
        cur_seq = 0;
        buf_offset = 0;
    }
    else
    {
        cur_seq += 1;
        buf_offset += data_len;
    }

    return CMD_SET_STATUS_COMPLETE;
}


void app_read_flash(uint32_t start_addr, uint8_t cmd_path, uint8_t app_idx)
{
    uint32_t start_addr_tmp;
    uint16_t data_send_len;

    data_send_len = 0x200;// in case assert fail
    start_addr_tmp = start_addr;

    if (cmd_path == CMD_PATH_SPP)
    {
        APP_PRINT_TRACE1("app_read_flash: rfc_frame_size %d",
                         app_db.br_link[app_idx].vendor_spp.frame_size);
        if (app_db.br_link[app_idx].vendor_spp.frame_size - 12 < data_send_len)
        {
            data_send_len = app_db.br_link[app_idx].vendor_spp.frame_size - 12;
        }
    }
    else if (cmd_path == CMD_PATH_IAP)
    {
#if (F_APP_IAP_RTK_SUPPORT && F_APP_IAP_SUPPORT)
        T_APP_IAP_HDL app_iap_hdl = NULL;
        app_iap_hdl = app_iap_search_by_addr(app_db.br_link[app_idx].bd_addr);
        uint16_t frame_size = app_iap_get_frame_size(app_iap_hdl);

        APP_PRINT_TRACE1("app_read_flash: iap frame_size %d", frame_size);
        if (frame_size - 12 < data_send_len)
        {
            data_send_len = frame_size - 12;
        }
#endif
    }
    else if (cmd_path == CMD_PATH_LE)
    {
        APP_PRINT_TRACE1("app_read_flash: mtu_size %d", app_db.le_link[app_idx].mtu_size);
        if (app_db.le_link[app_idx].mtu_size - 15 < data_send_len)
        {
            data_send_len = app_db.le_link[app_idx].mtu_size - 15;
        }
    }

    uint8_t *data = malloc(data_send_len + 6);

    if (data != NULL)
    {
        if (start_addr + data_send_len >= app_cmd_mgr.flash.start_addr + app_cmd_mgr.flash.size)
        {
            data_send_len = app_cmd_mgr.flash.start_addr + app_cmd_mgr.flash.size - start_addr;
            data[0] = END_TRANS_DATA;
        }
        else
        {
            data[0] = CONTINUE_TRANS_DATA;
        }

        data[1] = app_cmd_mgr.flash.type;
        data[2] = (uint8_t)(start_addr_tmp);
        data[3] = (uint8_t)(start_addr_tmp >> 8);
        data[4] = (uint8_t)(start_addr_tmp >> 16);
        data[5] = (uint8_t)(start_addr_tmp >> 24);

        if (fmc_flash_nor_read(start_addr_tmp, &data[6], data_send_len))// read flash data
        {
            app_report_event(cmd_path, EVENT_REPORT_FLASH_DATA, app_idx, data, data_send_len + 6);
        }

        app_cmd_mgr.flash.start_addr_tmp += data_send_len;
        free(data);
    }
}


static void app_cmd_le_name_set(uint8_t *device_name, uint8_t name_len)
{
    memcpy(app_cfg_nv.device_name_le, device_name, name_len + 1);
    app_cfg_store(app_cfg_nv.device_name_le, 40);
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, name_len, device_name);
    app_ble_common_adv_name_refresh();
}

static void app_cmd_legacy_name_set(uint8_t *device_name, uint8_t name_len)
{
    memcpy(app_cfg_nv.device_name_legacy, device_name, name_len + 1);
    app_cfg_store(app_cfg_nv.device_name_legacy, 40);
    bt_local_name_set(device_name, name_len);
}

void app_cmd_general_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                                uint8_t app_idx, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_MMI:
        {
            uint8_t mmi_action = cmd_ptr[3];
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if (mmi_action == MMI_ENTER_DUT_FROM_SPP)
            {
                app_start_timer_enter_dut_from_spp(app_idx);
                break;
            }
            else if (mmi_action == MMI_DEV_POWER_ON)
            {
                app_device_handle_power_on_cmd();
                break;
            }

            if (mmi_action == MMI_DEV_POWER_OFF)
            {
                app_db.power_off_cause = POWER_OFF_CAUSE_CMD_SET;
            }

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            if (mmi_action == MMI_DEV_FACTORY_RESET)
            {
                app_bond_mgr_ftl_erase();

                app_tri_dongle_notify_ble_bond_list_report();
                app_tri_dongle_notify_br_bond_list_report();
            }
#endif

            app_mmi_handle_action(cmd_ptr[3]);
        }
        break;


    case CMD_INFO_REQ:
        {
            uint8_t info_type = cmd_ptr[2];
            uint8_t report_to_phone_len = 6;
            uint8_t buf[report_to_phone_len];

            if (info_type == CMD_SET_INFO_TYPE_VERSION)
            {
                if (!app_db.eq_ctrl_by_src &&
                    app_cmd_check_src_eq_spec_version(cmd_path, app_idx))
                {
                    app_cmd_update_eq_ctrl(true, true);
                }

                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                if (cmd_len > CMD_SUPPORT_VER_CHECK_LEN) // update SRC support version
                {
                    T_SRC_SUPPORT_VER_FORMAT *version = app_cmd_get_src_version(cmd_path, app_idx);

                    memcpy(version->version, &cmd_ptr[3], 4);

                    if (!app_cmd_check_src_eq_spec_version(cmd_path, app_idx))
                    {
                        app_cmd_update_eq_ctrl(false, true);
                    }

                    if (!app_cmd_check_src_cmd_version(cmd_path, app_idx))
                    {
                        // version not support
                    }
                }

                buf[0] = info_type;
                buf[1] = CMD_INFO_STATUS_VALID;

                buf[2] = CMD_SET_VER_MAJOR;
                buf[3] = CMD_SET_VER_MINOR;
                buf[4] = EQ_SPEC_VER_MAJOR;
                buf[5] = EQ_SPEC_VER_MINOR;

                if (report_to_phone_len > 0)
                {
                    app_report_event(cmd_path, EVENT_INFO_RSP, app_idx, buf, report_to_phone_len);
                }
            }
            else if (info_type == CMD_INFO_GET_CAPABILITY)
            {
                T_SNK_CAPABILITY current_snk_cap;
                uint8_t evt_param[11];

                evt_param[0] = info_type;
                evt_param[1] = CMD_INFO_STATUS_VALID;
                current_snk_cap = app_cmd_get_system_capability();
                memcpy(&evt_param[2], (uint8_t *)&current_snk_cap, sizeof(T_SNK_CAPABILITY));

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_report_event(cmd_path, EVENT_INFO_RSP, app_idx, evt_param, sizeof(evt_param));
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            }
        }
        break;

    case CMD_SET_CFG:
        {
            if ((cmd_ptr[2] == CFG_SET_LE_NAME) || (cmd_ptr[2] == CFG_SET_LEGACY_NAME))
            {
                if ((cmd_path == CMD_PATH_SPP) ||
                    (cmd_path == CMD_PATH_IAP) ||
                    (cmd_path == CMD_PATH_GATT_OVER_BREDR) ||
                    (cmd_path == CMD_PATH_LE) ||
                    (cmd_path == CMD_PATH_UART))
                {
                    uint8_t name_len;
                    uint8_t device_name[40];

                    name_len = cmd_ptr[3];

                    if (name_len >= GAP_DEVICE_NAME_LEN)
                    {
                        name_len = GAP_DEVICE_NAME_LEN - 1;
                    }
                    memcpy(device_name, &cmd_ptr[4], name_len);
                    device_name[name_len] = 0;

                    if (cmd_ptr[2] == CFG_SET_LE_NAME)
                    {
                        app_cmd_le_name_set(device_name, name_len);

                        if (app_cfg_const.le_name_sync_to_legacy_name)
                        {
                            app_cmd_legacy_name_set(device_name, name_len);
                        }
                    }
                    else if (cmd_ptr[2] == CFG_SET_LEGACY_NAME)
                    {
                        app_cmd_legacy_name_set(device_name, name_len);

                        if (app_cfg_const.le_name_sync_to_legacy_name)
                        {
                            app_cmd_le_name_set(device_name, name_len);
                        }
                    }
                }
            }
            else if ((cmd_ptr[2] == CFG_SET_AUDIO_LATENCY) || (cmd_ptr[2] == CFG_SET_SUPPORT_CODEC))
            {
            }
            else if (cmd_ptr[2] == CFG_SET_HFP_REPORT_BATT)
            {
                app_db.hfp_report_batt = cmd_ptr[3];
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_GET_CFG_SETTING:
        {
            uint8_t get_type = cmd_ptr[2];

            if (get_type >= CFG_GET_MAX)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if ((get_type == CFG_GET_LE_NAME) || (get_type == CFG_GET_LEGACY_NAME) ||
                (get_type == CFG_GET_IC_NAME))
            {
                uint8_t p_name[40 + 2];
                uint8_t *p_buf;
                uint8_t name_len;

                if (get_type == CFG_GET_LEGACY_NAME)
                {
                    name_len = strlen((const char *)app_cfg_nv.device_name_legacy);
                    p_buf = app_cfg_nv.device_name_legacy;
                }
                else if (get_type == CFG_GET_LE_NAME)
                {
                    name_len = strlen((const char *)app_cfg_nv.device_name_le);
                    p_buf = app_cfg_nv.device_name_le;
                }
                else
                {
                    name_len = strlen((const char *)IC_NAME);
                    p_buf = (uint8_t *)IC_NAME;
                }

                p_name[0] = get_type;
                p_name[1] = name_len;
                memcpy(&p_name[2], p_buf, name_len);

                app_report_event(cmd_path, EVENT_REPORT_CFG_TYPE, app_idx, &p_name[0], name_len + 2);
            }
            else if (get_type == CFG_GET_COMPANY_ID_AND_MODEL_ID)
            {
                uint8_t buf[5];

                buf[0] = get_type;

                // use little endian method
                buf[1] = 0x5D;
                buf[2] = 0x00;
                buf[3] = 0x34;
                buf[4] = 0x12;

                app_report_event(cmd_path, EVENT_REPORT_CFG_TYPE, app_idx, buf, sizeof(buf));
            }
        }
        break;

    case CMD_INDICATION:
        {
            if (cmd_ptr[2] == 0)//report MAC address of smart phone
            {
                memcpy(app_db.le_link[app_idx].bd_addr, &cmd_ptr[3], 6);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LANGUAGE_GET:
        {
            uint8_t buf[2];

            buf[0] = app_cfg_nv.voice_prompt_language;
            buf[1] = voice_prompt_supported_languages_get();

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(cmd_path, EVENT_LANGUAGE_REPORT, app_idx, buf, 2);
        }
        break;

    case CMD_LANGUAGE_SET:
        {
            if (voice_prompt_supported_languages_get() & BIT(cmd_ptr[2]))
            {
                if (voice_prompt_language_set((T_VOICE_PROMPT_LANGUAGE_ID)cmd_ptr[2]) == true)
                {
                    bool need_to_save_to_flash = false;

                    if (cmd_ptr[2] != app_cfg_nv.voice_prompt_language)
                    {
                        need_to_save_to_flash = true;
                    }

                    app_cfg_nv.voice_prompt_language = cmd_ptr[2] ;
                    //app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_VP_LANGUAGE);

                    if (need_to_save_to_flash)
                    {
                        app_cfg_store(&app_cfg_nv.voice_prompt_language, 1);
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_GET_STATUS:
        {
            /* This command is used to get specific RWS info.
            * Only one status can be reported at one time.
            * Use CMD_GET_BUD_INFO instead to get complete RWS bud info.
            */
            uint8_t buf[3];
            uint8_t report_len = 2;

            buf[0] = cmd_ptr[2]; //status_index

            switch (cmd_ptr[2])
            {
            case GET_STATUS_RWS_STATE:
                {
                    buf[1] = app_db.remote_session_state;
                }
                break;

            case GET_STATUS_RWS_CHANNEL:
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        buf[1] = (app_cfg_nv.spk_channel << 4) | app_db.remote_spk_channel;
                    }
                    else
                    {
                        buf[1] = (app_audio_cfg.solo_speaker_channel << 4);
                    }
                }
                break;

            case GET_STATUS_BATTERY_STATUS:
                {
                    buf[1] = app_db.local_batt_level;
                    buf[2] = app_db.remote_batt_level;
                    report_len = 3;
                }
                break;

            case GET_STATUS_APP_STATE:
                {
                    buf[1] = app_bt_policy_get_state();
                }
                break;

            case GET_STATUS_BUD_ROLE:
                {
                    buf[1] = app_cfg_const.bud_role;
                }
                break;

            case GET_STATUS_VOLUME:
                {
                    T_AUDIO_STREAM_TYPE volume_type;

                    if (app_hfp_get_call_status() != APP_HFP_CALL_IDLE)
                    {
                        volume_type = AUDIO_STREAM_TYPE_VOICE;
                    }
                    else
                    {
                        volume_type = AUDIO_STREAM_TYPE_PLAYBACK;
                    }

                    buf[1] = audio_volume_out_get(volume_type);
                    buf[2] = audio_volume_out_max_get(volume_type);
                    report_len = 3;
                }
                break;

            case GET_STATUS_RWS_BUD_SIDE:
                {
                    buf[1] = app_cfg_const.bud_side;
                }
                break;


#if F_APP_DEVICE_CMD_SUPPORT
            case GET_STATUS_FACTORY_RESET_STATUS:
                {
                    buf[1] = app_cfg_nv.factory_reset_done;
                }
                break;

            case GET_STATUS_AUTO_REJECT_CONN_REQ_STATUS:
                {
                    if (app_cmd_mgr.conn_req_flags.auto_reject_en)
                    {
                        buf[1] = ENABLE_AUTO_REJECT_ACL_ACF_REQ;
                    }
                    else if (app_cmd_mgr.conn_req_flags.auto_accept_en)
                    {
                        buf[1] = ENABLE_AUTO_ACCEPT_ACL_ACF_REQ;
                    }
                    else
                    {
                        buf[1] = FORWARD_ACL_ACF_REQ_TO_HOST;
                    }
                }
                break;

            case GET_STATUS_RADIO_MODE:
                {
                    buf[1] = app_bt_policy_get_radio_mode();
                }
                break;

            case GET_STATUS_SCO_STATUS:
                {
                    buf[1] = app_link_get_sco_conn_num();
                }
                break;

            case GET_STATUS_MIC_MUTE_STATUS:
                {
                    buf[1] = app_audio_is_mic_mute();
                }
                break;
#endif

            default:
                break;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(cmd_path, EVENT_REPORT_STATUS, app_idx, buf, report_len);
        }
        break;

    case CMD_GET_BUD_INFO:
        {
            /* This command is used when snk_support_new_report_bud_status_flow is true.
             * Return complete RWS bud info.
             */
            uint8_t buf[6];

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_get_bud_info(&buf[0]);
            app_report_event(cmd_path, EVENT_REPORT_BUD_INFO, app_idx, buf, sizeof(buf));
        }
        break;

    case CMD_GET_FW_VERSION:
        {
            uint8_t report_data[2];
            report_data[0] = cmd_path;
            report_data[1] = app_idx;

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            switch (cmd_ptr[2])
            {
            case GET_PRIMARY_FW_VERSION:
                {
                    uint8_t data[13] = {0};

                    app_cmd_get_fw_version(&data[0]);
                    app_report_event(report_data[0], EVENT_FW_VERSION, report_data[1], data, sizeof(data));
                }
                break;


            }
        }
        break;

    case CMD_WDG_RESET:
        {
            uint8_t wdg_status = 0x00;

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(cmd_path, EVENT_WDG_RESET, app_idx, &wdg_status, 1);
        }
        break;

    case CMD_GET_FLASH_DATA:
        {
            switch (cmd_ptr[2])
            {
            case START_TRANS:
                {
                    if ((0x01 << cmd_ptr[3]) & ALL_DUMP_IMAGE_MASK)
                    {
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                        app_flash_data_set_param(cmd_ptr[3], cmd_path, app_idx);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    }
                }
                break;

            case CONTINUE_TRANS:
                {
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    app_read_flash(app_cmd_mgr.flash.start_addr_tmp, cmd_path, app_idx);
                }
                break;

            case SUPPORT_IMAGE_TYPE:
                {
                    uint8_t paras[5];

                    paras[0] = SUPPORT_IMAGE_TYPE_INFO;
                    paras[1] = (uint8_t)(ALL_DUMP_IMAGE_MASK);
                    paras[2] = (uint8_t)(ALL_DUMP_IMAGE_MASK >> 8);
                    paras[3] = (uint8_t)(ALL_DUMP_IMAGE_MASK >> 16);
                    paras[4] = (uint8_t)(ALL_DUMP_IMAGE_MASK >> 24);

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    app_report_event(cmd_path, EVENT_REPORT_FLASH_DATA, app_idx, paras, sizeof(paras));
                }
                break;

            default:
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
                break;
            }
        }
        break;

    case CMD_GET_PACKAGE_ID:
        {
            uint8_t temp_buff[2];

            temp_buff[0] = feature_check_get_chip_id();
            temp_buff[1] = feature_check_get_pkg_id();

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(cmd_path, EVENT_REPORT_PACKAGE_ID, app_idx, temp_buff, 2);
        }
        break;

    case CMD_GET_EAR_DETECTION_STATUS:
        {
            uint8_t status = 0;


            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(cmd_path, EVENT_EAR_DETECTION_STATUS, app_idx, &status, sizeof(status));
        }
        break;

    case CMD_REG_ACCESS:
        {
            uint32_t addr;
            uint32_t value;
            uint8_t report[5];
            uint32_t *report_value = (uint32_t *)(report + 1);

            memcpy(&addr, &cmd_ptr[4], sizeof(uint32_t));
            memcpy(&value, &cmd_ptr[8], sizeof(uint32_t));

            report[0] = false;

            if (cmd_ptr[2] == REG_ACCESS_READ)
            {
                switch (cmd_ptr[3])
                {
                case REG_ACCESS_TYPE_AON:
                    {
                        *report_value = btaon_fast_read_safe_8b(addr);
                    }
                    break;

                case REG_ACCESS_TYPE_AON2B:
                    {
                        *report_value = btaon_fast_read_safe(addr);
                    }
                    break;

                case REG_ACCESS_TYPE_DIRECT:
                    {
                        *report_value = HAL_READ32(addr, 0);
                    }
                    break;

                default:
                    break;
                }
            }
            else if (cmd_ptr[2] == REG_ACCESS_WRITE)
            {
                switch (cmd_ptr[3])
                {
                case REG_ACCESS_TYPE_AON:
                    {
                        btaon_fast_write_safe_8b(addr, value);
                    }
                    break;

                case REG_ACCESS_TYPE_AON2B:
                    {
                        btaon_fast_write_safe(addr, value);
                    }
                    break;

                case REG_ACCESS_TYPE_DIRECT:
                    {
                        HAL_WRITE32(addr, 0, value);
                    }
                    break;

                default:
                    break;
                }
            }
            else
            {
                report[0] = true;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(cmd_path, EVENT_REG_ACCESS, app_idx, report, sizeof(report));
        }
        break;

    case CMD_SEND_RAW_PAYLOAD:
        {
            uint8_t direction = cmd_ptr[2];

            if (cmd_len - 2 < 7)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            else
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                    (direction != app_cfg_const.bud_side || direction == DEVICE_BUD_SIDE_BOTH))
                {

                }

                if (direction == app_cfg_const.bud_side || direction == DEVICE_BUD_SIDE_BOTH)
                {
                    ack_pkt[2] = app_cmd_compose_payload(cmd_ptr + 2, cmd_len - 2);
                }
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

#if F_APP_SPDIF_SUPPORT
    case CMD_SET_PLAYBACK_DEVICE:
        {
            uint32_t playback_device = 0;
            uint8_t *p_data = &cmd_ptr[2];
            LE_STREAM_TO_UINT32(playback_device, p_data);
            if (playback_device != AUDIO_DEVICE_OUT_SPDIF && playback_device != AUDIO_DEVICE_OUT_SPK)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            else
            {
                app_db.playback_device = playback_device;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

    default:
        break;
    }
}
