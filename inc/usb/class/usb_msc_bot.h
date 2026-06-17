/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_MSC_BOT_H__
#define __USB_MSC_BOT_H__
#include <stdint.h>

/** \addtogroup USB_MSC_BOT
  * \brief USB mass storage struct.
  * @{
  */

/** @defgroup USB_MSC_BOT_Exported_Constants USB MSC BOT Exported Constants
  * @{
  */

/**
 * usb_msc_bot.h
 *
 * \brief  Get Max LUN device request.
 *         bRequest field set to 254 (FEh).
 */
#define USB_MS_REQ_GET_MAX_LUN          (0xFE)

/**
 * usb_msc_bot.h
 *
 * \brief  This request is used to reset the mass storage device and its associated interface.
 *         bRequest field set to 255 (FFh).
 */
#define USB_MS_REQ_BO_RESET             (0xFF)

/** End of group USB_MSC_BOT_Exported_Constants
  * @}
  */

/** @defgroup USB_MSC_BOT_Exported_Types USB MSC BOT Exported Types
  * @{
  */

#pragma pack(push,1)

/**
 * usb_msc_bot.h
 *
 * \brief   USB command block wrapper.
 *
 * \param dCBWSignature Signature that helps identify this data packet as a CBW.
 * \param dCBWTag  A Command Block Tag sent by the host.
 * \param dCBWDataTransferLength The number of bytes of data that the host expects to transfer on the Bulk-In or Bulk-Out endpoint during the execution of this command.
 * \param bmCBWFlags  The bmCBWFlags show CBW command block wrapper flag.
 * \param bCBWLUN The device Logical Unit Number (LUN) to which the command block is being sent.
 * \param rsv0  reserve.
 * \param bCBWCBLength The valid length of the CBWCB in bytes.
 * \param CBWCB  The command block to be executed by the device.
 *
 */
typedef struct _t_cbw
{
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN: 4;
    uint8_t rsv0: 4;
    uint8_t bCBWCBLength: 5;
    uint8_t rsv1: 3;
    uint8_t CBWCB[16];
} T_CBW;

/**
 * usb_msc_bot.h
 *
 * \brief   Command Status Wrapper.
 *
 * \param dCSWSignature Signature that helps identify this data packet as a CSW.
 * \param dCSWTag  The device shall set this field to the value received in the dCBWTag of the associated CBW.
 * \param dCSWDataResidue Command block wrapper remaining bytes.
 * \param bCSWStatus  bCSWStatus indicates the success or failure of the command.
 *
 */
typedef struct _t_csw
{
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
} T_CSW;

#pragma pack(pop)

/**
 * usb_msc_bot.h
 *
 * \brief   Command Block Status Values.
 */
typedef enum
{
    CB_STATUS_OK,
    CB_STATUS_FAILED,
    CB_STATUS_PHASE_ERROR,
} T_CB_STATUS;

/** End of group USB_MSC_BOT_Exported_Types
  * @}
  */
/** @}*/
#endif
