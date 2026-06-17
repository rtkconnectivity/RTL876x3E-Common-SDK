/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "bt_types.h"
#include "os_mem.h"
#include "app_msg.h"
#include "app_console_msg.h"
#include "app_bt_audio_link.h"
#include "app_bt_audio_console.h"
#include "app_bt_audio_a2dp.h"
#include "app_bt_audio_avrcp.h"
#include "app_bt_audio_hfp.h"
#include "app_bt_audio_gap.h"

void app_console_handle_msg(T_IO_MSG console_msg)
{
    uint16_t  subtype;
    uint16_t  id;
    uint16_t  action;
    uint8_t  *p;

    p       = console_msg.u.buf;
    subtype = console_msg.subtype;

    switch (subtype)
    {
    case IO_MSG_CONSOLE_STRING_RX:
        LE_STREAM_TO_UINT16(id, p);
        LE_STREAM_TO_UINT8(action, p);

        if (id == GAP_BREDR_ID)
        {
            uint8_t  value;

            switch (action)
            {
            case BT_AUDIO_ACTION_BREDR_LOCAL_ADDR_SET:
                app_bt_audio_gap_local_addr_set(p);
                break;

            case BT_AUDIO_ACTION_BREDR_REMOTE_ADDR_SET:
                app_bt_audio_gap_remote_addr_set(p);
                break;

            case BT_AUDIO_ACTION_BREDR_NAME_SET:
                {
                    uint8_t  len;
                    LE_STREAM_TO_UINT8(len, p);
                    app_bt_audio_gap_local_bt_name_set(p, len);
                }
                break;

            case BT_AUDIO_ACTION_BREDR_INQUIRY_START:
                app_bt_audio_gap_inquiry_start();
                break;

            case BT_AUDIO_ACTION_BREDR_DEVICE_MODE_SET:
                {
                    LE_STREAM_TO_UINT8(value, p);
                    app_bt_audio_gap_device_mode_set(value);
                }
                break;

            default:
                break;
            }
        }
        else if (id == A2DP_ID)
        {
            switch (action)
            {
            case BT_AUDIO_ACTION_A2DP_CONNECT:
                app_bt_audio_a2dp_connect(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_A2DP_DISCONNECT:
                app_bt_audio_a2dp_disconnect(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_A2DP_START:
                app_bt_audio_a2dp_start(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_A2DP_SUSPEND:
                app_bt_audio_a2dp_suspend(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_A2DP_VOLUME_UP:
                app_bt_audio_a2dp_volume_up(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_A2DP_VOLUME_DOWN:
                app_bt_audio_a2dp_volume_down(app_db.remote_addr);
                break;

            default:
                break;
            }
        }
        else if (id == AVRCP_ID)
        {
            switch (action)
            {
            case BT_AUDIO_ACTION_AVRCP_COVER_ART_CONNECT:
                app_bt_audio_avrcp_cover_art_connect(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_AVRCP_COVER_ART_DISCONNECT:
                app_bt_audio_avrcp_cover_art_disconnect(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_AVRCP_COVER_ART_GET:
                app_bt_audio_avrcp_cover_art_get(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_AVRCP_ELEM_ATTR_GET:
                {
                    uint8_t attr_id;

                    LE_STREAM_TO_UINT8(attr_id, p);

                    app_bt_audio_avrcp_element_attr_get(app_db.remote_addr, attr_id);
                }
                break;

            default:
                break;
            }
        }
        else if (id == HFP_ID)
        {
            switch (action)
            {
            case BT_AUDIO_ACTION_HFP_CONNECT:
                app_bt_audio_hfp_connect(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_HFP_DISCONNECT:
                app_bt_audio_hfp_disconnect(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_HFP_SCO_CONNECT:
                app_bt_audio_hfp_sco_connect(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_HFP_SCO_DISCONNECT:
                app_bt_audio_hfp_sco_disconnect(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_HFP_CALL_INCOMING:
                app_bt_audio_hfp_call_incoming(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_HFP_CALL_ANSWER:
                app_bt_audio_hfp_call_answer(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_HFP_CALL_TERMINATE:
                app_bt_audio_hfp_call_terminate(app_db.remote_addr);
                break;

            case BT_AUDIO_ACTION_HFP_VENDOR_AT_CMD:
                app_bt_audio_hfp_vendor_at_cmd(app_db.remote_addr);
                break;

            default:
                break;
            }
        }

        os_mem_free(console_msg.u.buf);
        break;

    default:
        break;
    }
}
