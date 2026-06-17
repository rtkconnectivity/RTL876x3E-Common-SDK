/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include "stdlib.h"
#include "remote.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_link_util_cs.h"


#include "audio_track.h"
#include "eq.h"

uint32_t app_connected_profiles(void)
{
    uint32_t i, connected_profiles = 0;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        connected_profiles |= app_db.br_link[i].connected_profile;
    }
    return connected_profiles;
}


uint8_t app_connected_profile_link_num(uint32_t profile_mask)
{
    uint8_t i, link_number = 0;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].connected_profile & profile_mask)
        {
            link_number++;
        }

    }
    return link_number;
}

uint8_t app_link_get_sco_conn_num(void)
{
    uint8_t i;
    uint8_t num = 0;
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used == true &&
            app_db.br_link[i].sco.conn_handle != 0)
        {
            num++;
        }
    }
    return num;
}

uint8_t app_link_get_a2dp_start_num(void)
{
    uint8_t i;
    uint8_t num = 0;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used == true &&
            app_db.br_link[i].a2dp.is_streaming == true)
        {
            num++;
        }
    }

    return num;
}

T_APP_BR_LINK *app_link_find_br_link(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = NULL;
    uint8_t        i;

    if (bd_addr != NULL)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_db.br_link[i].used == true &&
                !memcmp(app_db.br_link[i].bd_addr, bd_addr, 6))
            {
                p_link = &app_db.br_link[i];
                break;
            }
        }
    }

    return p_link;
}

T_APP_BR_LINK *app_link_find_br_link_by_link_id(uint8_t link_id)
{
    T_APP_BR_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used && app_db.br_link[i].id == link_id)
        {
            p_link = &app_db.br_link[i];
            break;
        }
    }

    return p_link;
}

T_APP_BR_LINK *app_link_find_br_link_by_conn_handle(uint16_t conn_handle)
{
    T_APP_BR_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used && app_db.br_link[i].acl.conn_handle == conn_handle)
        {
            p_link = &app_db.br_link[i];
            break;
        }
    }

    return p_link;
}


T_APP_BR_LINK *app_link_alloc_br_link(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = NULL;
    uint8_t i;

    if (bd_addr != NULL)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_db.br_link[i].used == false)
            {
                p_link = &app_db.br_link[i];

                p_link->used = true;
                p_link->id   = i;
                memcpy(p_link->bd_addr, bd_addr, 6);
                break;
            }
        }
    }



    return p_link;
}

bool app_link_free_br_link(T_APP_BR_LINK *p_link)
{
    if (p_link != NULL)
    {
        if (p_link->used == true)
        {


            if (p_link->p_gfps_cmd != NULL)
            {
                free(p_link->p_gfps_cmd);
            }
            if (p_link->sco.track_handle != NULL)
            {
                audio_track_release(p_link->sco.track_handle);
            }
            if (p_link->a2dp.track_handle != NULL)
            {
                audio_track_release(p_link->a2dp.track_handle);
            }

            if (p_link->eq.instance != NULL)
            {
                eq_release(p_link->eq.instance);
            }

#if F_APP_VOICE_SPK_EQ_SUPPORT
            if (p_link->eq.voice_mic_eq_instance != NULL)
            {
                eq_release(p_link->eq.voice_mic_eq_instance);
            }

            if (p_link->eq.voice_spk_eq_instance != NULL)
            {
                eq_release(p_link->eq.voice_spk_eq_instance);
            }
#endif

            memset(p_link, 0, sizeof(T_APP_BR_LINK));

            return true;
        }
    }

    return false;
}

T_APP_LE_LINK *app_link_find_le_link_by_conn_id(uint8_t conn_id)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].conn_id == conn_id)
        {
            p_link = &app_db.le_link[i];
            break;
        }
    }

    return p_link;
}

T_APP_LE_LINK *app_link_find_le_link_by_link_id(uint8_t link_id)
{
    T_APP_LE_LINK    *p_link = NULL;
    uint8_t          i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].id == link_id)
        {
            p_link = &app_db.le_link[i];
            break;
        }
    }

    return p_link;
}

T_APP_LE_LINK *app_link_find_le_link_by_conn_handle(uint16_t conn_handle)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].conn_handle == conn_handle)
        {
            p_link = &app_db.le_link[i];
            break;
        }
    }

    return p_link;
}

T_APP_LE_LINK *app_link_find_le_link_by_addr(uint8_t *bd_addr)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t i;

    if (bd_addr != NULL)
    {
        for (i = 0; i < MAX_BLE_LINK_NUM; i++)
        {
            if (app_db.le_link[i].used == true &&
                !memcmp(app_db.le_link[i].bd_addr, bd_addr, 6))
            {
                p_link = &app_db.le_link[i];
                break;
            }
        }
    }

    return p_link;
}

T_APP_LE_LINK *app_link_alloc_le_link_by_conn_id(uint8_t conn_id)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == false)
        {
            p_link = &app_db.le_link[i];

            p_link->used    = true;
            p_link->id      = i;
            p_link->conn_id = conn_id;
            break;
        }
    }

    return p_link;
}

bool app_link_free_le_link(T_APP_LE_LINK *p_link)
{
    if (p_link != NULL)
    {
        if (p_link->used == true)
        {

            if (p_link->cmd.buf != NULL)
            {
                free(p_link->cmd.buf);
            }

            if (p_link->audio_set_eq_info.eq_data_buf != NULL)
            {
                free(p_link->audio_set_eq_info.eq_data_buf);
            }

            if (p_link->audio_get_eq_info.eq_data_buf != NULL)
            {
                free(p_link->audio_get_eq_info.eq_data_buf);
            }

#if BAP_BROADCAST_ASSISTANT
            if (p_link->brs_char_tbl)
            {
                free(p_link->brs_char_tbl);
            }
#endif

            while (p_link->disc_cb_list.count > 0)
            {
                T_LE_DISC_CB_ENTRY *p_entry;
                p_entry = os_queue_out(&p_link->disc_cb_list);
                if (p_entry)
                {
                    free(p_entry);
                }
            }
            memset(p_link, 0, sizeof(T_APP_LE_LINK));
            p_link->conn_id = 0xFF;
            return true;
        }
    }

    return false;
}

bool app_link_reg_le_link_disc_cb(uint8_t conn_id, P_FUN_LE_LINK_DISC_CB p_fun_cb)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(conn_id);

    if (p_link != NULL)
    {
        T_LE_DISC_CB_ENTRY *p_entry;

        for (uint8_t i = 0; i < p_link->disc_cb_list.count; i++)
        {
            p_entry = os_queue_peek(&p_link->disc_cb_list, i);
            if (p_entry != NULL && p_entry->disc_callback == p_fun_cb)
            {
                return true;
            }
        }

        p_entry = calloc(1, sizeof(T_LE_DISC_CB_ENTRY));
        if (p_entry != NULL)
        {
            p_entry->disc_callback = p_fun_cb;
            os_queue_in(&p_link->disc_cb_list, p_entry);
            return true;
        }
    }

    return false;
}

uint8_t app_link_get_le_link_num(void)
{
    uint8_t num = 0;
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true)
        {
            num++;
        }
    }

    return num;
}

uint8_t app_link_get_le_encrypted_link_num(void)
{
    uint8_t num = 0;
    uint8_t i = 0;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true && app_db.le_link[i].encryption_status == LE_LINK_ENCRYPTIONED)
        {
            num++;
        }
    }

    return num;
}

bool app_link_check_b2s_link_by_id(uint8_t id)
{
    bool ret = false;

    if (app_db.br_link[id].used)
    {
        ret = true;
    }

    return ret;
}

T_APP_BR_LINK *app_link_find_b2s_link(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = NULL;

    p_link = app_link_find_br_link(bd_addr);

    return p_link;
}

uint8_t app_link_get_b2s_link_num(void)
{
    uint8_t num = 0;
    uint8_t i = 0;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used)
        {
            num++;
        }
    }
    return num;
}
