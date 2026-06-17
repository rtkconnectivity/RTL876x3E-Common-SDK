/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#ifndef __GUI_INTERFACE_H__
#define __GUI_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "gui_win.h"
#include "gui_tab.h"
#include "def_event.h"
#include "gui_message.h"

typedef enum
{
    GUI_EVENT_USER_INIT = GUI_EVENT_USER_DEFINE,
    GUI_EVENT_CALL_INCOMING,
    GUI_EVENT_CALL_OUTGOING,
    GUI_EVENT_CALL_ACTIVE,
    GUI_EVENT_CALL_VOLUME,
    GUI_EVENT_CALL_END,
    GUI_EVENT_PLAYER,
    GUI_EVENT_PHONE,
    GUI_EVENT_BUDS,
    GUI_EVENT_BUDS_DEVICES,
    GUI_EVENT_BUDS_SEARCH,
    GUI_EVENT_BENCHMARK_START,
    GUI_EVENT_BENCHMARK_NEXT_SCENRAIO,
    GUI_EVENT_BENCHMARK_UPDATE_FRAME_COST,
    GUI_EVENT_RECORD,
    GUI_EVENT_RECORD_UPDATE_TIME,
    GUI_EVENT_WATCHFACE_UPDATE,
    GUI_EVENT_HEARTRATE_RECORD,
    GUI_EVENT_HEARTRATE_DELETE,
} gui_event_user_t;

void gui_update_by_event(gui_event_user_t event, void *data, bool force_update);

#ifdef __cplusplus
}
#endif
#endif
