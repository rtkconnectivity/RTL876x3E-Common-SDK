/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_USB_AUDIO_HID_H__
#define __APP_USB_AUDIO_HID_H__

typedef enum
{
    MMI_SPK_VOL_UP          = 0x0,
    MMI_SPK_VOL_DOWN        = 0x1,
    MMI_AV_PLAY_PAUSE       = 0x2,
    MMI_AV_FWD              = 0x3,
    MMI_AV_BWD              = 0x4,
} T_USB_HID_MMI;

void app_usb_audio_mmi_handle_action(uint8_t action);
void app_usb_audio_mmi_init(void);
#endif
