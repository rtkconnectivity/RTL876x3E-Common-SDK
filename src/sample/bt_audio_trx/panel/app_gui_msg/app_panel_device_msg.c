/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "trace.h"
#include "app_panel_device_msg.h"
#if F_GUI_SDCARD_LIST_DEMO
#include "app_sdcard_list_demo.h"
#endif
#include "app_src_playback.h"
#include "app_src_play.h"
#include "resource.h"

void app_panel_device_msg_handle(T_APP_GUI_MSG *data)
{
    APP_PRINT_INFO1("app_panel_device_msg_handle: msg type %d", data->subtype);
    switch (data->subtype)
    {
    case GUI_DEVICE_SUBEVENT_STOP_SD_MUSIC:
        {
#if F_GUI_SDCARD_LIST_DEMO
            app_src_play_sd_stop(PLAY_ROUTE_LOCAL);
#endif
        }
        break;
    case GUI_DEVICE_SUBEVENT_PLAY_SD_MUSIC:
        {
#if F_GUI_SDCARD_LIST_DEMO
            T_HEAD_INFO *head_info = (T_HEAD_INFO *)data->u.param;
            app_src_play_set_play_route(PLAY_ROUTE_LOCAL);
            app_src_play_sd_update_name((uint8_t *)(MUSIC_NAME_BIN_ADDR + head_info->offset),
                                        head_info->length);
            //APP_PRINT_INFO2("app_panel_device_msg_handle: music play, length %d: data %b", head_info->length,
            //                TRACE_BINARY(head_info->length, (uint8_t *)(MUSIC_NAME_BIN_ADDR + head_info->offset)));
            app_src_play_sd_start(PLAY_ROUTE_LOCAL);
#endif
        }
        break;
    default:
        break;
    }
}

#endif


/*-----------------------------------------------------------*/
