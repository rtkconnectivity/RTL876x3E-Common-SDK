/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_SOURCE_PLAY_SUPPORT

#include "trace.h"
#include "app_cmd.h"
#include "app_src_play.h"
#include "gap_br.h"

void app_src_play_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                 uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
#if F_APP_AUTO_POWER_TEST_LOG
    TEST_PRINT_TRACE3("app_src_play_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                      cmd_id, cmd_len, cmd_path);
#else
    APP_PRINT_TRACE3("app_src_play_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u",
                     cmd_id, cmd_len, cmd_path);
#endif

    switch (cmd_id)
    {
    case CMD_SRC_PLAY_SET_SRC_ROUTE:
        {
            T_SOURCE_ROUTE src_route = (T_SOURCE_ROUTE)cmd_ptr[2];
            uint8_t dir = cmd_ptr[3];
            if (!app_src_play_set_src_route(src_route, dir))
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
    case CMD_SRC_PLAY_GET_SRC_ROUTE:
        {
            ack_pkt[2] = app_src_play_get_src_route();
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;
    case CMD_SRC_PLAY_SET_PLAY_ROUTE:
        {
            uint8_t play_route = cmd_ptr[2];
            app_src_play_set_play_route((T_PLAY_ROUTE)play_route);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SRC_PLAY_GET_PLAY_ROUTE:
        {
            ack_pkt[2] = app_src_play_get_play_route();
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SRC_PLAY_ROUTE_IN_START:
        {
            app_src_play_route_in_start();
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SRC_PLAY_ROUTE_IN_STOP:
        {
            app_src_play_route_in_stop();
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SRC_PLAY_ROUTE_OUT_START:
        {
            if (app_src_play_route_out_start())
            {
                ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SRC_PLAY_ROUTE_OUT_STOP:
        {
            app_src_play_route_out_stop();
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SRC_PLAY_SET_MULTI_ESCO:
        {
            uint8_t activate = cmd_ptr[2];
            uint8_t policy = cmd_ptr[3];
            gap_br_vendor_set_active_sco(0, activate, policy);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SRC_PLAY_ATTACH_FEATURE:
        {
            uint16_t feature = (uint16_t)(cmd_ptr[2] | (cmd_ptr[3] << 8));
            bool check_nrec_result = true;
            bool check_localplay_result = true;

            if ((feature & SRC_PLAY_FEATURE_NREC) && !app_src_play_check_attach_feature_nrec())
            {
                check_nrec_result = false;
            }

            if ((feature & SRC_PLAY_FEATURE_APPEND_LOCALPLAY) &&
                !app_src_play_check_attach_feature_append_localplay())
            {
                check_localplay_result = false;
            }

            if (check_nrec_result && check_localplay_result)
            {
                app_src_play_attach_features(feature);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_SCENARIO_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SRC_PLAY_DETACH_FEATURE:
        {
            uint8_t feature = (uint16_t)(cmd_ptr[2] | (cmd_ptr[3] << 8));
            app_src_play_detach_features(feature);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    default:
        break;
    }
}
#endif
