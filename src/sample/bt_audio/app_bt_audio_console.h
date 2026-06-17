/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _BT_AUDIO_CONSOLE_H_
#define _BT_AUDIO_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_AUDIO_CONSOLE BT Audio Console
* @brief BT Audio command
* @{
*/

typedef enum
{
    GAP_BREDR_ID          = 0x0001,
    A2DP_ID               = 0x0002,
    AVRCP_ID              = 0x0003,
    HFP_ID                = 0x0004,
} T_BT_AUDIO_CMD_ID;

#define BT_AUDIO_ACTION_BREDR_LOCAL_ADDR_SET           0x01
#define BT_AUDIO_ACTION_BREDR_REMOTE_ADDR_SET          0x02
#define BT_AUDIO_ACTION_BREDR_NAME_SET                 0x03
#define BT_AUDIO_ACTION_BREDR_INQUIRY_START            0x04
#define BT_AUDIO_ACTION_BREDR_DEVICE_MODE_SET          0x05

#define BT_AUDIO_ACTION_A2DP_CONNECT           0x01
#define BT_AUDIO_ACTION_A2DP_DISCONNECT        0x02
#define BT_AUDIO_ACTION_A2DP_START             0x03
#define BT_AUDIO_ACTION_A2DP_SUSPEND           0x04
#define BT_AUDIO_ACTION_A2DP_VOLUME_UP         0x05
#define BT_AUDIO_ACTION_A2DP_VOLUME_DOWN       0x06

#define BT_AUDIO_ACTION_AVRCP_COVER_ART_CONNECT    0x01
#define BT_AUDIO_ACTION_AVRCP_COVER_ART_DISCONNECT 0x02
#define BT_AUDIO_ACTION_AVRCP_COVER_ART_GET        0x03
#define BT_AUDIO_ACTION_AVRCP_ELEM_ATTR_GET        0x04

#define BT_AUDIO_ACTION_HFP_CONNECT            0x01
#define BT_AUDIO_ACTION_HFP_DISCONNECT         0x02
#define BT_AUDIO_ACTION_HFP_SCO_CONNECT        0x03
#define BT_AUDIO_ACTION_HFP_SCO_DISCONNECT     0x04
#define BT_AUDIO_ACTION_HFP_CALL_INCOMING      0x05
#define BT_AUDIO_ACTION_HFP_CALL_ANSWER        0x06
#define BT_AUDIO_ACTION_HFP_CALL_TERMINATE     0x07
#define BT_AUDIO_ACTION_HFP_VENDOR_AT_CMD      0x08

void app_bt_audio_cmd_register(void);

/** @} End of APP_BT_AUDIO_CONSOLE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_AUDIO_CONSOLE_H_ */
