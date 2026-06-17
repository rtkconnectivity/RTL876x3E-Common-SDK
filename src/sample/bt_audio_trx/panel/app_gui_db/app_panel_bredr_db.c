/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_GUI_SUPPORT
#include "app_panel_bredr_db.h"
#include "app_panel_db_common.h"
#include "trace.h"

T_APP_GUI_BREDR_DATA app_gui_bredr_data;
static uint8_t active_bd_addr[6] = {0};

T_APP_GUI_BREDR_LINK_DATA *app_panel_bredr_find_active_br_link(void)
{
    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_gui_bredr_data.gui_bredr_link_data[i].active == true)
        {
            return &app_gui_bredr_data.gui_bredr_link_data[i];
        }
    }
    return NULL;
}

T_APP_GUI_BREDR_LINK_DATA *app_panel_bredr_find_br_link(uint8_t *bd_addr)
{
    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if ((app_gui_bredr_data.gui_bredr_link_data[i].active == true) &&
            (memcmp(app_gui_bredr_data.gui_bredr_link_data[i].bd_addr, bd_addr, 6) == 0))
        {
            return &app_gui_bredr_data.gui_bredr_link_data[i];
        }
    }
    return NULL;
}

void app_panel_bredr_alloc_link(uint8_t *bd_addr)
{
    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_gui_bredr_data.gui_bredr_link_data[i].active == false)
        {
            app_gui_bredr_data.gui_bredr_link_data[i].active = true;
            memcpy(app_gui_bredr_data.gui_bredr_link_data[i].bd_addr, bd_addr, 6);
            app_panel_bredr_check_awake_gui();
            return;
        }
    }
}

void app_panel_bredr_free_link(uint8_t *bd_addr)
{
    T_APP_GUI_BREDR_LINK_DATA *gui_bredr_link = NULL;

    gui_bredr_link = app_panel_bredr_find_br_link(bd_addr);
    if (gui_bredr_link)
    {
        memset((uint8_t *)gui_bredr_link, 0, sizeof(T_APP_GUI_BREDR_LINK_DATA));
    }

    app_panel_bredr_check_awake_gui();
}

void app_panel_bredr_check_awake_gui(void)
{
    uint8_t temp_addr[6] = {0};
    bool update_flag = false;

    T_APP_GUI_BREDR_LINK_DATA *bredr_link = app_panel_bredr_find_active_br_link();

    APP_PRINT_INFO2("app_panel_bredr_check_awake_gui: bredr_link %p, active_bd_addr %s", bredr_link,
                    TRACE_BDADDR(active_bd_addr));

    if ((memcmp(active_bd_addr, temp_addr, 6) == 0) && bredr_link)
    {
        memcpy(active_bd_addr, bredr_link->bd_addr, 6);
        update_flag = true;
    }
    else if ((memcmp(active_bd_addr, temp_addr, 6) != 0) && !bredr_link)
    {
        memset(active_bd_addr, 0, sizeof(active_bd_addr));
        update_flag = true;
    }
    else if (bredr_link && (memcmp(bredr_link->bd_addr, active_bd_addr, 6) != 0))
    {
        memcpy(active_bd_addr, bredr_link->bd_addr, 6);
        update_flag = true;
    }

    APP_PRINT_INFO1("app_panel_bredr_check_awake_gui: update %d", update_flag);

    if (update_flag)
    {
        app_panel_db_common_awake_gui_task();
    }
}


#endif


/*-----------------------------------------------------------*/
