/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include <trace.h>
#include "ftl.h"
#include "fmc_api.h"
#include "rtl876x.h"
#include "test_mode.h"
#include "eq_utils.h"
#include "gap_le.h"
#include "app_audio_cfg.h"
#include "app_bt_policy_cfg.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_main.h"
#include "wdg.h"
#include "app_mmi.h"
#include "app_dsp_cfg.h"
#include "app_charger.h"
#include "app_multilink.h"
#include "btm.h"
#include "bt_hfp.h"
#include "bt_hfp_ag.h"



static const T_APP_CFG_NV app_cfg_rw_default =
{
    .hdr.sync_word = APP_MCU_CFG_SYNC_WORD,
    .hdr.length = sizeof(T_APP_CFG_NV),
    .single_tone_timeout_val = 20 * 1000, //20s
    .single_tone_tx_gain = 0,
    .single_tone_channel_num = 20,
    .app_pair_idx_mapping = {0, 1, 2, 3, 4, 5, 6, 7},
    .lea_vol_setting = 200,
    .lea_vol_out_mute = 0,
};

T_APP_CFG_NV app_cfg_nv;


void app_mcu_cfg_nv_load(void)
{
    //read-write data
    ftl_load_from_storage(&app_cfg_nv.hdr, APP_RW_DATA_ADDR, sizeof(app_cfg_nv.hdr));

    if (app_cfg_nv.hdr.sync_word != APP_MCU_CFG_SYNC_WORD)
    {
        //Load factory reset bit first when mppgtool factory reset
        if (app_cfg_nv.hdr.length == 0)
        {
            ftl_load_from_storage(&app_cfg_nv.offset_app_is_power_on, APP_RW_DATA_ADDR + FACTORY_RESET_OFFSET,
                                  4);
        }

        app_cfg_reset();
    }
    else
    {
        uint32_t load_len = app_cfg_nv.hdr.length;

        if (load_len > sizeof(T_APP_CFG_NV))
        {
            uint32_t res = ftl_load_from_storage(&app_cfg_nv, APP_RW_DATA_ADDR, sizeof(T_APP_CFG_NV));

            if (res == 0)
            {
                app_cfg_nv.hdr.length = sizeof(T_APP_CFG_NV);
            }
            else
            {
                APP_PRINT_ERROR1("app_mcu_cfg_const_load, res %d", res);
                app_cfg_reset();
            }
        }
        else
        {
            uint32_t res = ftl_load_from_storage(&app_cfg_nv, APP_RW_DATA_ADDR, load_len);

            if (res == 0)
            {
                if (load_len < sizeof(T_APP_CFG_NV))
                {
                    uint8_t *p_dst = ((uint8_t *)&app_cfg_nv) + load_len;
                    uint8_t *p_src = ((uint8_t *)&app_cfg_rw_default) + load_len;

                    memcpy(p_dst, p_src, sizeof(T_APP_CFG_NV) - load_len);
                }
                app_cfg_nv.hdr.length = sizeof(T_APP_CFG_NV);
            }
            else
            {
                app_cfg_reset();
            }
        }
    }
}

uint32_t app_cfg_reset(void)
{
    uint8_t temp_bd_addr[6];
    uint8_t temp_device_name_legacy[40];
    uint8_t temp_device_name_le[40];
    uint8_t temp_reset_power_on;
    uint8_t temp_reset_done;
    uint32_t temp_sync_word;
    uint8_t temp_power_off_cause_cmd = 0;
    uint8_t temp_le_single_random_addr[6] = {0};

    uint8_t temp_one_wire_aging_test_done = 0;
    uint8_t temp_one_wire_start_force_engage = 0;

    //Keep for restore
    temp_reset_power_on = app_cfg_nv.adp_factory_reset_power_on;
    if (app_cfg_nv.adp_factory_reset_power_on && app_cfg_nv.power_off_cause_cmd)
    {
        temp_power_off_cause_cmd = app_cfg_nv.power_off_cause_cmd;
    }

    temp_reset_done = app_cfg_nv.factory_reset_done;
    temp_sync_word = app_cfg_nv.hdr.sync_word;
    temp_one_wire_aging_test_done = app_cfg_nv.one_wire_aging_test_done;

    memcpy(temp_device_name_legacy, app_cfg_nv.device_name_legacy, 40);
    memcpy(temp_device_name_le, app_cfg_nv.device_name_le, 40);
    memcpy(temp_le_single_random_addr, app_cfg_nv.le_single_random_addr, 6);

    //init with default value
    memcpy(&app_cfg_nv, &app_cfg_rw_default, sizeof(T_APP_CFG_NV));

    //restore some cfg
    memcpy(app_cfg_nv.le_single_random_addr, temp_le_single_random_addr, 6);
    app_cfg_nv.adp_factory_reset_power_on = temp_reset_power_on;
    if (app_cfg_nv.adp_factory_reset_power_on && temp_power_off_cause_cmd)
    {
        app_cfg_nv.power_off_cause_cmd = temp_power_off_cause_cmd;
    }
    app_cfg_nv.factory_reset_done = temp_reset_done;
    memcpy(app_cfg_nv.device_name_legacy, temp_device_name_legacy, 40);
    memcpy(app_cfg_nv.device_name_le, temp_device_name_le, 40);
    app_cfg_nv.one_wire_aging_test_done = temp_one_wire_aging_test_done;


    uint8_t state_of_charge = app_charger_get_soc();
    //Charger module report 0xFF when ADC not ready
    if (state_of_charge <= 100)
    {
        app_cfg_nv.local_level = state_of_charge;
    }


    if ((app_cfg_const.enable_not_reset_device_name == 0) ||
        (temp_sync_word != APP_MCU_CFG_SYNC_WORD))  //First init
    {
        memcpy(&app_cfg_nv.device_name_legacy[0], &app_cfg_const.device_name_legacy_default[0], 40);
        memcpy(&app_cfg_nv.device_name_le[0], &app_cfg_const.device_name_le_default[0], 40);

        // update device name when factory reset
        bt_local_name_set(&app_cfg_nv.device_name_legacy[0], GAP_DEVICE_NAME_LEN);
        le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, &app_cfg_nv.device_name_le[0]);

        if (temp_sync_word != APP_MCU_CFG_SYNC_WORD)
        {
            // Power on is not allowed for the first time when vbat in after factory reset by mppgtool;
            // Unlock disallow power on mode by MFB power on or slide switch power on or bud starts chargring
            app_cfg_nv.disallow_power_on_when_vbat_in = 1;
        }
    }

    app_cfg_nv.bud_role = app_cfg_const.bud_role;
    memcpy(app_cfg_nv.bud_local_addr, app_db.factory_addr, 6);
    app_cfg_nv.app_is_power_on = 0;

#if F_APP_TMAP_BMR_SUPPORT
    app_cfg_nv.bis_audio_gain_level  = app_dsp_cfg_vol.playback_volume_default;
#endif

    app_cfg_nv.voice_prompt_volume_out_level = app_dsp_cfg_vol.voice_prompt_volume_default;
    app_cfg_nv.ringtone_volume_out_level = app_dsp_cfg_vol.ringtone_volume_default;

    memset(&app_cfg_nv.audio_gain_level[0], app_dsp_cfg_vol.playback_volume_default, 8);
    memset(&app_cfg_nv.voice_gain_level[0], app_dsp_cfg_vol.voice_out_volume_default, 8);
    app_cfg_nv.line_in_gain_level = app_dsp_cfg_vol.line_in_volume_out_default;

    app_cfg_nv.voice_prompt_language = app_audio_cfg.voice_prompt_language;

    app_cfg_nv.eq_idx_normal_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, NORMAL_MODE);
    app_cfg_nv.eq_idx_gaming_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, GAMING_MODE);
    app_cfg_nv.eq_idx_anc_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, ANC_MODE);


#if F_APP_LINEIN_SUPPORT
    app_cfg_nv.eq_idx_line_in_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, LINE_IN_MODE);
#endif

    app_cfg_nv.enable_multi_link = (app_multi_is_enabled() == true) ? 1 : 0;
    app_cfg_nv.either_bud_has_vol_ctrl_key = false;
    app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_normal_mode_record;

    app_cfg_nv.pin_code_size = app_bt_policy_cfg.pin_code_size;
    memcpy(&app_cfg_nv.pin_code[0], &app_bt_policy_cfg.pin_code[0], 8);

    return ftl_save_to_storage(&app_cfg_nv, APP_RW_DATA_ADDR, sizeof(T_APP_CFG_NV));
}

uint32_t app_cfg_store_all(void)
{
    return ftl_save_to_storage(&app_cfg_nv, APP_RW_DATA_ADDR, sizeof(T_APP_CFG_NV));
}

uint32_t app_cfg_store(void *pdata, uint16_t size)
{
    uint16_t offset = (uint8_t *)pdata - (uint8_t *)&app_cfg_nv;

    if ((offset % 4) != 0)
    {
        uint16_t old_offset = offset;

        pdata = (uint8_t *)pdata - (offset % 4);
        offset -= (offset % 4);
        size += (old_offset % 4);
    }

    if ((size % 4) != 0)
    {
        size = (size / 4 + 1) * 4;
    }

    APP_PRINT_TRACE2("app_cfg_store: offset 0x%x, size %d", offset, size);

    return ftl_save_to_storage(pdata, offset + APP_RW_DATA_ADDR, size);
}

