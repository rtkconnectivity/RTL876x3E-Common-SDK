/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_CSIS_SUPPORT
#include "cap.h"

uint8_t csis_sirk[CSIS_SIRK_LEN] = {0x11, 0x22, 0x33, 0xc6, 0xaf, 0xbb, 0x65, 0xa2, 0x5a, 0x41, 0xf1, 0x53, 0x05, 0x68, 0x8e, 0x83};

void app_lea_csis_init(T_CAP_INIT_PARAMS *p_param)
{
    uint8_t size = 1;
    uint8_t rank = 1;

    p_param->csis_num = 1;
    p_param->cas.enable = true;
#if F_APP_LEA_CSIS_ENC
    p_param->cas.csis_sirk_type = CSIS_SIRK_ENC;
#else
    p_param->cas.csis_sirk_type = CSIS_SIRK_PLN;
#endif
    p_param->cas.csis_size = size;
    p_param->cas.csis_rank = rank;
    p_param->cas.csis_feature = (SET_MEMBER_LOCK_EXIST | SET_MEMBER_SIZE_EXIST | SET_MEMBER_RANK_EXIST |
                                 SET_MEMBER_SIRK_NOTIFY_SUPPORT);
    p_param->cas.csis_sirk = csis_sirk;
}
#endif
