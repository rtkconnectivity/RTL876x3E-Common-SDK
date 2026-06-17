/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_BC12_H__
#define __USB_BC12_H__
#include <stdint.h>

/** \addtogroup USB_BC12
  * \brief Provide the external charger with BC12 API.
  * @{
  */

/** @defgroup USB_BC12_Exported_Constants USB BC12 Exported Constants
  * @{
  */

/**
 * usb_bc12_api.h
 *
 * \brief   The definition of USB BC12 type.
 *
 */
typedef enum
{
    BC12_TYPE_SDP_0P5A = 0,               /**< BC12 type is SDP, the maximum charger current is 0.5A. */
    BC12_TYPE_DCP_1P5A,                   /**< BC12 type is DCP, the maximum charger current is 1.5A. */
    BC12_TYPE_CDP_1P5A,                   /**< BC12 type is CDP, the maximum charger current is 1.5A. */
    BC12_TYPE_OTHERS_USER_DEFINED_0P5A,   /**< BC12 type is others, the maximum charger current is user-defined. */
    BC12_TYPE_ADP_ERROR = 0xFF,           /**< Error value. */
} T_USB_BC12_TYPE;

/** End of group USB_BC12_Exported_Constants
  * @}
  */

/** @defgroup USB_BC12_Exported_Functions USB BC12 Exported Functions
  * @{
  */

/**
 * \brief   Get USB BC12 type.
 *
 * \details The BC12 type is defined in \ref T_USB_BC12_TYPE.
 *         The maximum charger current is user-defined if the type is \ref BC12_TYPE_OTHERS_USER_DEFINED_0P5A.
 *         For safety, the suggested value should be less than 0.5A.
 *         Please do not charge the battery if the type is BC12_TYPE_ADP_ERROR. Instead, call
 *         this api to obtain the BC12 type once more.
 *
 * \return  BC12 type defined in \ref T_USB_BC12_TYPE.
 */
uint8_t usb_bc12_type_get(void);

/**
 * \brief   Power down BC12.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \details The power will be keeping if the type is DCP (BC1.2 spec chapter 4.4.1).
 *          When the USB is unplugged, bc12 needs to power down.
 */
void usb_bc12_power_down(void);

/** @} */ /* End of group USB_BC12_Exported_Functions */

/** @}*/
/** End of USB_BC12
*/
#endif
