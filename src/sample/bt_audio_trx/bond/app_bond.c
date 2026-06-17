/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include "string.h"
#include "stdlib.h"
#include "trace.h"
#include "bt_bond.h"
#include "btm.h"
#include "remote.h"
#include "gap_br.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_cfg_nv.h"
#include "app_bond.h"

#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
#include "bt_bond_le.h"
#include "gap_conn_le.h"
extern bool le_get_conn_local_addr(uint16_t conn_handle, uint8_t *bd_addr, uint8_t *bd_type);
#endif

typedef struct t_link_record_item
{
    struct t_link_record_item *p_next;
    T_APP_LINK_RECORD record;
} T_APP_LINK_RECORD_ITEM;

typedef struct
{
    uint8_t bond_num;
    uint8_t rsvd[3];
    uint8_t link_record[0];
} T_APP_REMOTE_MSG_PAYLOAD_LINK_RECORD_XMIT;

static T_OS_QUEUE sec_diff_link_record_list;

void app_bond_push_sec_diff_link_record(T_APP_LINK_RECORD *link_record)
{
    T_APP_LINK_RECORD_ITEM *record_item;
    T_APP_LINK_RECORD *record;

    record_item = calloc(1, sizeof(T_APP_LINK_RECORD_ITEM));
    if (record_item != NULL)
    {
        record = &record_item->record;

        memcpy(record, link_record, sizeof(T_APP_LINK_RECORD));
        os_queue_in(&sec_diff_link_record_list, record_item);

        APP_PRINT_TRACE8("app_bond_push_sec_diff_link_record: bond %x, priority %x, link_key %x %x %x, bd %x %x %x",
                         record->bond_flag, record->priority,
                         record->link_key[0], record->link_key[1],
                         record->link_key[2], record->bd_addr[0],
                         record->bd_addr[1], record->bd_addr[2]);
    }
}

uint8_t app_bond_b2s_num_get()
{
    uint8_t pair_idx;
    uint8_t bond_num;
    uint8_t bud_record_num = 0;
    uint8_t local_addr[6];
    uint8_t peer_addr[6];

    bond_num = bt_bond_num_get();

    remote_local_addr_get(local_addr);
    remote_peer_addr_get(peer_addr);
    if (bt_bond_index_get(local_addr, &pair_idx) == true)
    {
        bud_record_num++;
    }
    if (bt_bond_index_get(peer_addr, &pair_idx) == true)
    {
        bud_record_num++;
    }

    if (bond_num > bud_record_num)
    {
        return bond_num - bud_record_num;
    }

    return 0;
}

bool app_bond_b2s_addr_get(uint8_t priority, uint8_t *bd_addr)
{
    uint8_t i;
    uint8_t bond_num;
    uint8_t addr[6];
    uint8_t local_addr[6];
    uint8_t peer_addr[6];

    remote_local_addr_get(local_addr);
    remote_peer_addr_get(peer_addr);

    bond_num = bt_bond_num_get();
    for (i = 1; i <= bond_num; i++)
    {
        if (bt_bond_addr_get(i, addr) == true)
        {
            if (!memcmp(addr, local_addr, 6) || !memcmp(addr, peer_addr, 6))
            {
                priority++;
            }
            else
            {
                if (priority == i)
                {
                    memcpy(bd_addr, addr, 6);
                    return true;
                }
            }
        }
    }

    return false;
}


bool app_bond_key_set(uint8_t *bd_addr, uint8_t *linkkey, uint8_t key_type)
{
    return bt_bond_key_set(bd_addr, linkkey, key_type);
}

bool app_bond_get_pair_idx_mapping(uint8_t *bd_addr, uint8_t *index)
{
    uint8_t pair_idx;

    if (bt_bond_index_get(bd_addr, &pair_idx) == false)
    {
        APP_PRINT_TRACE0("app_bond_get_pair_idx_mapping false");
        return false;
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        if (app_cfg_nv.app_pair_idx_mapping[i] == pair_idx)
        {
            *index = i;
            APP_PRINT_TRACE0("app_bond_get_pair_idx_mapping success");
            return true;
        }
    }

    return false;
}

bool app_bond_pop_sec_diff_link_record(uint8_t *bd_addr, T_APP_LINK_RECORD *link_record)
{
    bool ret = false;
    T_APP_LINK_RECORD_ITEM *record_item;
    T_APP_LINK_RECORD *record;
    uint8_t i = 0;

    record_item = os_queue_peek(&sec_diff_link_record_list, i++);
    while (record_item != NULL)
    {
        record = &record_item->record;

        if (!memcmp(bd_addr, record->bd_addr, 6))
        {
            ret = true;

            APP_PRINT_TRACE8("app_bond_pop_sec_diff_link_record: bond %x, priority %x, link_key %x %x %x, bd %x %x %x",
                             record->bond_flag, record->priority,
                             record->link_key[0], record->link_key[1],
                             record->link_key[2], record->bd_addr[0],
                             record->bd_addr[1], record->bd_addr[2]);

            memcpy(link_record, record, sizeof(T_APP_LINK_RECORD));

            os_queue_delete(&sec_diff_link_record_list, record_item);
            free(record_item);

            break;
        }

        record_item = os_queue_peek(&sec_diff_link_record_list, i++);
    }

    return ret;
}

void app_bond_clear_sec_diff_link_record(void)
{
    T_APP_LINK_RECORD_ITEM *record_item;
    T_APP_LINK_RECORD *record;

    record_item = os_queue_out(&sec_diff_link_record_list);

    while (record_item != NULL)
    {
        record = &record_item->record;

        APP_PRINT_TRACE8("app_bond_clear_sec_diff_link_record: bond %x, priority %x, link_key %x %x %x, bd %x %x %x",
                         record->bond_flag, record->priority,
                         record->link_key[0], record->link_key[1],
                         record->link_key[2], record->bd_addr[0],
                         record->bd_addr[1], record->bd_addr[2]);

        free(record_item);

        record_item = os_queue_out(&sec_diff_link_record_list);
    }
}

void app_bond_set_priority(uint8_t *bd_addr)
{
    uint8_t temp_addr[6];

    if (app_bond_b2s_addr_get(1, temp_addr) == true)
    {
        if (memcmp(bd_addr, temp_addr, 6))
        {
            bt_bond_priority_set(bd_addr);
        }
    }
}

uint8_t app_bond_get_b2s_link_record(T_APP_LINK_RECORD *link_record, uint8_t link_num)
{
    uint8_t i;
    uint8_t num = 0;
    uint8_t key_type;
    uint8_t key[16];
    uint8_t addr[6];
    uint32_t bond_flag = 0;

    /* collect the link record by priority */
    for (i = 0; i < link_num; i++)
    {
        if (app_bond_b2s_addr_get(i + 1, addr) == false)
        {
            continue;
        }

        bt_bond_flag_get(addr, &bond_flag);

        if (bt_bond_key_get(addr, key, &key_type) == false)
        {
            continue;
        }

        link_record[num].key_type = key_type;
        link_record[num].bond_flag = bond_flag;
        link_record[num].priority = i + 1;
        memcpy(link_record[num].bd_addr, addr, 6);
        memcpy(link_record[num].link_key, key, 16);
        num++;
    }

    return num;
}


void app_bond_clear_all_keys(void)
{
    uint8_t i;
    uint8_t bond_num;
    T_APP_LINK_RECORD *link_record;

    bond_num = app_bond_b2s_num_get();

    link_record = malloc(sizeof(T_APP_LINK_RECORD) * bond_num);
    if (link_record != NULL)
    {
        bond_num = app_bond_get_b2s_link_record(link_record, bond_num);

        for (i = 0; i < bond_num; i++)
        {
            bt_bond_delete(link_record[i].bd_addr);
        }
    }
    free(link_record);
}

#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
void app_bond_le_set_bond_flag(void *p_link_info, uint16_t bond_flag)
{
    if (p_link_info != NULL)
    {
        T_APP_LE_LINK *p_link = (T_APP_LE_LINK *)p_link_info;
        T_LE_BOND_ENTRY *p_entry = NULL;
        T_GAP_REMOTE_ADDR_TYPE bd_type;
        uint8_t local_bd[GAP_BD_ADDR_LEN];
        uint8_t local_bd_type = LE_BOND_LOCAL_ADDRESS_ANY;

        le_get_conn_local_addr(p_link->conn_handle, local_bd, &local_bd_type);
        le_get_conn_addr(p_link->conn_id, p_link->bd_addr, (uint8_t *)&bd_type);
        p_entry = bt_le_find_key_entry(p_link->bd_addr, (T_GAP_REMOTE_ADDR_TYPE)bd_type, local_bd,
                                       local_bd_type);
        if (p_entry != NULL)
        {
            bt_le_set_bond_flag(p_entry, bond_flag);
        }
    }
}

void app_bond_le_clear_non_lea_keys(void)
{
    T_LE_BOND_ENTRY *p_entry = NULL;
    uint8_t max_bond_num = bt_le_get_max_le_paired_device_num();

    for (uint8_t idx = 0; idx < max_bond_num; idx ++)
    {
        p_entry = bt_le_find_key_entry_by_idx(idx);
        if (p_entry != NULL)
        {
            uint16_t le_bond_flag = 0;

            bt_le_get_bond_flag(p_entry, &le_bond_flag);
            if (!(le_bond_flag & BOND_FLAG_LEA))
            {
                bt_le_delete_bond(p_entry);
            }
        }
    }
}
#endif

void app_bond_init(void)
{
    os_queue_init(&sec_diff_link_record_list);
}
