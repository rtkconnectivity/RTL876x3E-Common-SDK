/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _AUDIO_DEMO_CONSOLE_H_
#define _AUDIO_DEMO_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @brief Audio demo command */
typedef enum
{
    AUDIO_ID              = 0x0000,
    VOICE_ID              = 0x0001,
    RECORD_ID             = 0x0002,
    LINE_IN_ID            = 0x0003,
    NOTIFICATION_ID       = 0x0004,
    APT_ID                = 0x0006,
    LLAPT_ID              = 0x0007,
    ANC_ID                = 0x0008,
    VAD_ID                = 0x0009,
    PIPE_ID               = 0x000A,
    TRACK_ID              = 0x000B,
} T_AUDIO_DEMO_CMD_ID;

#define AUDIO_DEMO_ACTION_RING_TONE_PLAY           0x01
#define AUDIO_DEMO_ACTION_VOICE_PROMPT_PLAY        0x02
#define AUDIO_DEMO_ACTION_APT_PLAY                 0x03
#define AUDIO_DEMO_ACTION_APT_STOP                 0x04
#define AUDIO_DEMO_ACTION_LLAPT_PLAY               0x05
#define AUDIO_DEMO_ACTION_LLAPT_STOP               0x06
#define AUDIO_DEMO_ACTION_LINE_IN_START            0x07
#define AUDIO_DEMO_ACTION_LINE_IN_STOP             0x08
#define AUDIO_DEMO_ACTION_ANC_ENABLE               0x09
#define AUDIO_DEMO_ACTION_ANC_DISABLE              0x0A

#define AUDIO_DEMO_ACTION_PIPE_CREATE              0x01
#define AUDIO_DEMO_ACTION_PIPE_START               0x02
#define AUDIO_DEMO_ACTION_PIPE_STOP                0x03
#define AUDIO_DEMO_ACTION_PIPE_RELEASE             0x04

#define AUDIO_DEMO_ACTION_TRACK_CREATE             0x01
#define AUDIO_DEMO_ACTION_TRACK_START              0x02
#define AUDIO_DEMO_ACTION_TRACK_STOP               0x03
#define AUDIO_DEMO_ACTION_TRACK_PAUSE              0x04
#define AUDIO_DEMO_ACTION_TRACK_RESTART            0x05
#define AUDIO_DEMO_ACTION_TRACK_RELEASE            0x06
#define AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_SET     0x07
#define AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_MUTE    0x08
#define AUDIO_DEMO_ACTION_TRACK_VOLUME_OUT_UNMUTE  0x09

void app_audio_demo_cmd_register(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_DEMO_CONSOLE_H_ */
