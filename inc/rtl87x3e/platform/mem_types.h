/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef _MEM_TYPES_H_
#define _MEM_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup 87x3e_MEM_CONFIG Memory Configure
  * @{
  */

/*============================================================================*
 *                               Types
*============================================================================*/
/** @defgroup 87x3e_MEM_CONFIG_Exported_Types Memory Configure Exported Types
  * @{
  */

typedef enum
{
    RAM_TYPE_DATA_ON,
    RAM_TYPE_DATA_OFF,
    RAM_TYPE_BUFFER_ON,
    RAM_TYPE_BUFFER_OFF,
    RAM_TYPE_DTCM0,  //reserve for app
    RAM_TYPE_ITCM1,  //reserve for app
    RAM_TYPE_DSPSHARE,  //reserve for app
    RAM_TYPE_NUM
} RAM_TYPE;

/** @} */ /* End of group 87x3e_MEM_TYPES_Exported_Types */

/** @} */ /* End of group 87x3e_MEM_CONFIG */


#ifdef __cplusplus
}
#endif

#endif /* _MEM_TYPES_H_ */
