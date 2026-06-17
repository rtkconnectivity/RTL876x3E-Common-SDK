/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __AMS__H
#define __AMS__H


#include "ams_client.h"
#include "app_report.h"
#include "app_msg.h"


typedef enum
{
    AMS_MSG_TYPE_REMOTE_CMD,
    AMS_MSG_TYPE_ENTITY_UPDATE_CMD,
    AMS_MSG_TYPE_WRITE_ENTITY_ATTR,
    AMS_MSG_TYPE_READ_ENTITY_ATTR,
    AMS_MSG_TYPE_MAX,
} T_AMS_MSG_TYPE;



typedef struct
{
    T_AMS_MSG_TYPE type;

    struct
    {
        T_AMS_REMOTE_CMD_ID cmd_id;
    } remote_cmd;

    struct
    {
        uint8_t *data;
        uint32_t len;
    } entity_update;
} T_AMS_MSG;

void ams_media_control(uint16_t conn_handle, T_AMS_REMOTE_CMD_ID cmd_id);

void ams_init(void);

void ams_handle_cmd(uint8_t app_idx, T_CMD_PATH cmd_path, uint8_t *cmd_ptr, uint8_t *ack_pkt);

#endif






