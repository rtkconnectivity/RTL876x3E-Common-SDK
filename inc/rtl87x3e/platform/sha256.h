/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/
#ifndef _SHA256_H_
#define _SHA256_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdbool.h"
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <stdint.h>


/** @defgroup  87x3E_SHA256_API SHA256 API
    * @brief SHA256 API.
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup 87x3e_SHA256_API_EXPORTED_MACROS SHA256 API Exported Macros
    * @{
    */

#define SHA256_BLOCK_LENGTH         64
#define SHA256_DIGEST_LENGTH        32
#define SHA256_DIGEST_STRING_LENGTH (SHA256_DIGEST_LENGTH * 2 + 1)

/** End of 87x3e_SHA256_API_EXPORTED_MACROS
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup 87x3E_SHA256_API_EXPORTED_TYPES SHA256 API Exported Types
    * @{
    */

/**  @brief Context structure to store SHA256 algorithm intermediate information. */
typedef struct SHA256Context
{
    uint32_t state[8];
    uint64_t count;
    uint8_t buf[SHA256_BLOCK_LENGTH];
} SHA256_CTX;

/** End of 87x3E_SHA256_API_EXPORTED_TYPES
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup 87x3E_SHA256_API_EXPORTED_FUNCTIONS SHA256 API Exported Functions
    * @brief
    * @{
    */

/**
    * @brief    SHA-256 initialization, begins a SHA-256 operation.
    * @param    ctx     A context pointer used to store algorithm information.
    * @return   void
    */
extern void SHA256_Init(SHA256_CTX *ctx);

/**
    * @brief    Add bytes into the hash.
    * @note     This is called to input the source data, and can be called several times to load
    *           a large amount of data sequentially.
    * @param    ctx     A context pointer used to store algorithm information.
    * @param    in      A pointer which point to the array of the source data.
    * @param    len     Length of input data, shouldn't be larger than the length of the 'in' array.
    * @return   void
    */
extern void SHA256_Update(SHA256_CTX *ctx, const void *in, size_t len);

/**
    * @brief    SHA-256 finalization, called to retrieve the final calculating result.
                It pads the input data, exports the hash value, and clears the context state.
    * @note     The result is output to an array named 'digest', and the array length is
    *           fixed to 32, which replaces by an understandable macro SHA256_DIGEST_LENGTH.
    * @param    ctx     A context pointer used to store algorithm information.
    * @param    digest  A pointer which point to the array storing calculating result data.
    * @return   void
    */
extern void SHA256_Final(SHA256_CTX *ctx, unsigned char digest[SHA256_DIGEST_LENGTH]);

/**
    * @brief    Alloc the memory for SHA-256 context.
    *           If the allocation success, init the SHA256 context.
    * @param    ctx     A context pointer used to store algorithm information.
    * @return   void
    */
extern bool SHA256_Alloc(SHA256_CTX **ctx);
/**
    * @brief    Free the memory for SHA-256 context.
    * @param    ctx     A context pointer used to store algorithm information.
    * @return   void
    */
extern void SHA256_Free(SHA256_CTX *ctx);
/**
    * @brief    Calculate SHA256 hash of the input data.
    * @param[in]     in      A pointer which points to the array of the source data.
    * @param[in]     len     The length of the input data.
    * @param[out]    result  A pointer which points to the SHA256 result buffer, and
    *                        the result is 32 bytes.
    * @return   void
    */
extern void SHA256(const void *in, size_t len, uint8_t *result);

/** End of 87x3E_SHA256_API_EXPORTED_FUNCTIONS
    * @}
    */


/** End of 87x3E_SHA256_API
    * @}
    */


#ifdef __cplusplus
}
#endif

#endif /* !_SHA256_H_ */
