/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "stdbool.h"
#include "vector_table.h"

/** @defgroup  HAL_VectorTable    Vector Table EXT
    * @brief Vector table EXT wrapper.
    * @{
    */

/** @defgroup HAL_VectorTable_Exported_Functions Vector Table EXT Exported Functions
    * @brief
    * @{
    */

/**
    * @brief    Update vector table in Ram.
    * @param    v_num  Vector number in @ref VECTORn_Type.
    * @param    isr_handler ISR execution function. See @ref IRQ_Fun.
    * @retval   true Success.
    * @retval   false Fail.
    * @note     Update ISR execution function.
    */
bool RamVectorTableUpdate(VECTORn_Type v_num, IRQ_Fun isr_handler);

/** @} */ /* End of group HAL_VectorTable_Exported_Functions */

/** @} */ /* End of group HAL_VectorTable */


