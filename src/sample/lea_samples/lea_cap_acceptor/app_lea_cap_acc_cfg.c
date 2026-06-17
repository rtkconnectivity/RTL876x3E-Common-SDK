/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "app_lea_cap_acc_cfg.h"
#include "ftl.h"
#include "trace.h"
#include "app_lea_cap_acc_vc_mic.h"

T_APP_CFG_NV app_cfg_nv;

static const T_APP_CFG_NV app_cfg_rw_default =
{
    .hdr.sync_word = DATA_SYNC_WORD,
    .hdr.length = sizeof(T_APP_CFG_NV),
#if VCP_VOLUME_RENDERER
    .lea_vcs_vol_setting = APP_VCS_DEFAULT_VOL_SETTING,
    .lea_vcs_mute = APP_VCS_DEFAULT_MUTE,
    .lea_vcs_change_cnt = APP_VCS_DEFAULT_CHG_CNT,
    .lea_vcs_vol_flag = APP_VCS_DEFAULT_VOL_FLAG,
    .lea_vcs_step_size = APP_VCS_DEFAULT_STEP_SIZE,
#endif
};

uint32_t app_cfg_reset(void)
{
    memcpy(&app_cfg_nv, &app_cfg_rw_default, sizeof(T_APP_CFG_NV));
    return ftl_save_to_storage(&app_cfg_nv, APP_RW_DATA_ADDR, sizeof(T_APP_CFG_NV));
}

static void app_cfg_load(void)
{
    //read-write data
    ftl_load_from_storage(&app_cfg_nv.hdr, APP_RW_DATA_ADDR, sizeof(app_cfg_nv.hdr));

    if (app_cfg_nv.hdr.sync_word != DATA_SYNC_WORD)
    {
        app_cfg_reset();
    }
    else
    {
        uint32_t load_len = app_cfg_nv.hdr.length;

        if (load_len > sizeof(T_APP_CFG_NV))
        {
            APP_PRINT_ERROR0("app_cfg_load, error");
            app_cfg_reset();
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

uint32_t app_cfg_store_all(void)
{
    return ftl_save_to_storage(&app_cfg_nv, APP_RW_DATA_ADDR, sizeof(T_APP_CFG_NV));
}

void app_cfg_init(void)
{
    app_cfg_load();
}

