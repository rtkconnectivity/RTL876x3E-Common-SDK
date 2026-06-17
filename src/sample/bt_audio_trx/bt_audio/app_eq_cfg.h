/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_EQ_CFG_H_
#define _APP_EQ_CFG_H_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" { /* __cplusplus */
#endif

/** \defgroup APP_EQ App EQ
 *  \brief App EQ
 *  \{
 */

/** @brief  Read only configurations for eq */
typedef struct
{
    uint8_t user_eq_spk_eq_num;
    uint8_t user_eq_mic_eq_num;

} T_APP_EQ_CFG;


extern T_APP_EQ_CFG app_eq_cfg;


/**
    * @brief  EQ config module init
    * @param  void
    * @return void
    */
void app_eq_cfg_init(void);


/** End of APP_EQ
* @}
*/


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif
