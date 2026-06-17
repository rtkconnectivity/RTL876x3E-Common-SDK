/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_CAP_ACC_CFG_H_
#define _APP_LEA_CAP_ACC_CFG_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

//FTL start
#define APP_RW_DATA_ADDR                    3072

#define DATA_SYNC_WORD                      0xAA55AA55
#define DATA_SYNC_WORD_LEN                  4

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @brief  Read/Write configurations for inbox audio application  */
typedef struct
{
    uint32_t sync_word;
    uint32_t length;
} T_APP_CFG_NV_HDR;

typedef struct
{
    T_APP_CFG_NV_HDR hdr;
#if VCP_VOLUME_RENDERER
    uint8_t lea_vcs_vol_setting;
    uint8_t lea_vcs_mute;
    uint8_t lea_vcs_change_cnt;
    uint8_t lea_vcs_vol_flag;
    uint8_t lea_vcs_step_size;
#endif
} T_APP_CFG_NV;

extern T_APP_CFG_NV app_cfg_nv;

uint32_t app_cfg_store_all(void);
void app_cfg_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
