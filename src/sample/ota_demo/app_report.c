/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_link_util.h"
#include "app_transfer.h"
#include "app_report.h"

static void app_report_spp_event(T_APP_BR_LINK *p_link, uint16_t event_id, uint8_t *data,
                                 uint16_t len, uint8_t cmd_path)
{
    uint32_t check_prof = 0;

    if (cmd_path == CMD_PATH_SPP)
    {
        check_prof = SPP_PROFILE_MASK;
    }

    if (p_link->connected_profile & check_prof)
    {
        uint8_t *buf;

        buf = malloc(len + 6);
        if (buf == NULL)
        {
            return;
        }

        buf[0] = CMD_SYNC_BYTE;
        p_link->tx_event_seqn++;
        if (p_link->tx_event_seqn == 0)
        {
            p_link->tx_event_seqn = 1;
        }
        buf[1] = p_link->tx_event_seqn;
        buf[2] = (uint8_t)(len + 2);
        buf[3] = (uint8_t)((len + 2) >> 8);
        buf[4] = (uint8_t)event_id;
        buf[5] = (uint8_t)(event_id >> 8);

        if (len)
        {
            memcpy(&buf[6], data, len);
        }

        if (app_transfer_push_data_queue(cmd_path, buf, len + 6, p_link->id) == false)
        {
            free(buf);
        }
    }
}

void app_report_event(uint8_t cmd_path, uint16_t event_id, uint8_t app_index, uint8_t *data,
                      uint16_t len)
{
    APP_PRINT_TRACE4("app_report_event: cmd_path %d, event_id 0x%04x, app_index %d, data_len %d",
                     cmd_path, event_id, app_index, len);

    switch (cmd_path)
    {
    case CMD_PATH_SPP:
        if (app_index < MAX_BR_LINK_NUM)
        {
            app_report_spp_event(&app_db.br_link[app_index], event_id, data, len, cmd_path);
        }
        break;

    default:
        break;
    }
}
