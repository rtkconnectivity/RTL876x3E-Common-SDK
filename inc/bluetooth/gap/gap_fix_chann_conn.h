/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _LE_FIX_CHANN_CONN_H_
#define _LE_FIX_CHANN_CONN_H_

#include <gap_le.h>
#include <gap.h>

T_GAP_CAUSE le_fixed_chann_reg(uint16_t cid);

T_GAP_CAUSE le_fixed_chann_data_send(uint8_t conn_id, uint16_t cid, uint8_t *p_data,
                                     uint16_t data_len);

#endif /* _LE_FIX_CHANN_CONN_H_ */
