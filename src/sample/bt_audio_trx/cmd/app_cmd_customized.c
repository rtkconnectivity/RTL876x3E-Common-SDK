/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "rtl876x_pinmux.h"
#include "audio_probe.h"
#include "app_cmd.h"

#include "app_cmd_customized.h"
#include "app_main.h"

void app_cmd_customized_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                                   uint8_t app_idx, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_IO_PIN_PULL_HIGH:
        {
            uint8_t report_status = 0;
            uint8_t pin_num = cmd_ptr[2];

            Pad_Config(pin_num, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);

            app_report_event(cmd_path, EVENT_IO_PIN_PULL_HIGH, app_idx, &report_status, 1);
        }
        break;

    case CMD_ENTER_BAT_OFF_MODE:
        {
            uint8_t report_status = 0;

            app_report_event(cmd_path, EVENT_ENTER_BAT_OFF_MODE, app_idx, &report_status, 1);
        }
        break;

    case CMD_MIC_MP_VERIFY_BY_HFP:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            uint8_t specified_mic;
            uint8_t report_status = 0;

            specified_mic = cmd_ptr[2];

            APP_PRINT_INFO4("CMD_MIC_MP_VERIFY_BY_HFP specified_mic = %x, pri_sel_ori = %x, pri_sel_new = %x, pri_type_new = %x",
                            specified_mic, app_db.mic_mp_verify_pri_sel_ori, cmd_ptr[3], cmd_ptr[4]);

            if (specified_mic)
            {
                if (app_db.mic_mp_verify_pri_sel_ori == APP_MIC_SEL_DISABLE)
                {
                    app_db.mic_mp_verify_pri_sel_ori = audio_probe_get_voice_primary_mic_sel();
                    app_db.mic_mp_verify_pri_type_ori = audio_probe_get_voice_primary_mic_type();
                    app_db.mic_mp_verify_sec_sel_ori = audio_probe_get_voice_secondary_mic_sel();
                    app_db.mic_mp_verify_sec_type_ori = audio_probe_get_voice_secondary_mic_type();
                }

                audio_probe_set_voice_primary_mic(cmd_ptr[3], cmd_ptr[4]);
                auido_probe_set_voice_secondary_mic(APP_MIC_SEL_DISABLE, app_db.mic_mp_verify_sec_type_ori);
            }
            else
            {
                if (app_db.mic_mp_verify_pri_sel_ori != APP_MIC_SEL_DISABLE)
                {
                    audio_probe_set_voice_primary_mic(app_db.mic_mp_verify_pri_sel_ori,
                                                      app_db.mic_mp_verify_pri_type_ori);
                    auido_probe_set_voice_secondary_mic(app_db.mic_mp_verify_sec_sel_ori,
                                                        app_db.mic_mp_verify_sec_type_ori);

                    app_db.mic_mp_verify_pri_sel_ori = APP_MIC_SEL_DISABLE;
                }
            }

            app_report_event(cmd_path, EVENT_MIC_MP_VERIFY_BY_HFP, app_idx, &report_status, 1);
        }
        break;

    default:
        break;
    }
}
