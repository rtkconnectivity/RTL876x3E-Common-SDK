/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/
#ifndef SECTION_H
#define SECTION_H


/** @defgroup 87x3E_SECTION   Section
  * @brief Memory section definition for user application.
  * @{
  */

/*============================================================================*
 *                              Macro
*============================================================================*/
/** @defgroup 87x3E_SECTION_EXPORTED_MACROS Section Exported Macros
    * @{
    */
#define FLASH_HEADER     __attribute__((section(".flash.header")))     __attribute__((used))
#define FLASH_HEADER_EXT __attribute__((section(".flash.header_ext"))) __attribute__((used))
#define RAM_TEXT_SECTION __attribute__((section(".ram_text")))
#define SHM_DATA_SECTION __attribute__((section(".shm.data")))
#define ISR_TEXT_SECTION __attribute__((section(".isr.text"))) /*not very urgent isr*/
#define BUFFER_RAM_DATA_SECTION __attribute__((section(".buffer_ram_data")))

#define RAM_DATAON_RANDOM_SECTION     __attribute__((section(".ram.dataon.random")))
/** @} */ /*End of group 87x3E_SECTION_EXPORTED_MACROS*/
/** @} */ /* End of group 87x3E_SECTION */



#endif // SECTION_H
