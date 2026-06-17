/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "string.h"
#include "btm.h"
#include "trace.h"
#include "gui_port.h"
#include "app_bt_policy_int.h"
#include "app_link_util_cs.h"

void app_gui_bt_policy_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_BT_PARAM bt_param;
    T_APP_BR_LINK *p_link = NULL;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));
    APP_PRINT_TRACE1("app_gui_bt_policy_cback event_type :0x%x", event_type);

    switch (event_type)
    {
    case BT_EVENT_LINK_USER_CONFIRMATION_REQ:
    case BT_EVENT_LINK_USER_PASSKEY_REQ:
    case BT_EVENT_LINK_USER_PASSKEY_NOTIF:
        {
            app_gui_set_bond_flag();
        }
        break;
    default:
        break;
    }

}

#endif


/*-----------------------------------------------------------*/
