/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_AUDIO_SPEC_H__
#define __USB_AUDIO_SPEC_H__
#include <stdint.h>

#define UAC_SUBCLASS_UNDEFINED          0x00
#define UAC_SUBCLASS_AUDIOCONTROL       0x01
#define UAC_SUBCLASS_AUDIOSTREAMING     0x02
#define UAC_SUBCLASS_MIDISTREAMING      0x03

#define UAC_DT_CS_UNDEFINE                0x20
#define UAC_DT_CS_DEVICE                  0x21
#define UAC_DT_CS_CONFIGURATION           0x22
#define UAC_DT_CS_STRING                  0x23
#define UAC_DT_CS_INTERFACE               0x24
#define UAC_DT_CS_ENDPOINT                0x25

#pragma pack(push,1)
typedef struct _uac_cs_desc_hdr
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDescriptorSubtype;
} T_UAC_CS_DESC_HDR;
#pragma pack(pop)
#endif
