/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __EXTERNAL_FLASH_TEST_H
#define __EXTERNAL_FLASH_TEST_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup SPI_EXTERNAL_FLASH_TEST External Flash Test
  * @brief External flash SPI test module
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/**
  * @brief  External flash test case definition
  */

typedef enum
{
    FLASH_TEST_POLLING_MODE,
    FLASH_TEST_INTERRUPT_MODE,
    FLASH_TEST_GDMA_MODE,
    FLASH_TEST_ERASE,
} EXT_FLASH_TEST_CASE;

/**
  * @brief  External flash test event definition
  */

typedef enum
{
    FLASH_TEST_START,
    FLASH_TEST_WRITE_FINISH,
    FLASH_TEST_READ_FINISH,
} EXT_FLASH_TEST_EVENT;

/** End of group External flash status register bits
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/**
  * @brief Earse flash.
  * @param  test_case: external flash test case.
  * @param  event: external flash test event.
  * @return none
  *
  */
void flash_test_send_msg(EXT_FLASH_TEST_CASE test_case, EXT_FLASH_TEST_EVENT event);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__EXTERNAL_FLASH_TEST_H*/

/** @} */ /* End of group SPI_EXTERNAL_FLASH_TEST */


