/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "ftl.h"
#include "trace.h"
#include "string.h"
#include "app_bt5_peripheral_adv.h"

/** @defgroup  BT5_PERIPH_STACK_API BT5 Peripheral Stack API
    * @brief This file provides APIs about extended advertising parameters.
    * @{
    */
/*============================================================================*
 *                              Constants
 *============================================================================*/
#define APP_STATIC_RANDOM_ADDR_OFFSET 0xC00

/**
 * @brief ext_adv_data
 * GAP - Advertisement data (best kept short to conserve power)
 */
static uint8_t ext_adv_data[] =
{
    /* Flags */
    0x02,/* length */
    GAP_ADTYPE_FLAGS,/* type="Flags" */
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
    /* Local name */
    0x13,/* length */
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B', 'L', 'E', '_', 'B', 'T', '5', '_', 'P', 'e', 'r', 'i', 'p', 'h', 'e', 'r', 'a', 'l',
    /* Manufacturer Specific Data */
    0xd7,/* length */
    GAP_ADTYPE_MANUFACTURER_SPECIFIC,
    0x5d, 0x00,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
    0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5,
    0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
    0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9,
    0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb,
    0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xd, 0xd, 0xd, 0xd,
};

/**
 * @brief ext_scan_rsp_data
 *  GAP - Scan response data (best kept short to conserve power)
 */
static uint8_t ext_scan_rsp_data[] =
{
    /* Manufacturer Specific Data */
    0xfc,/* length */
    GAP_ADTYPE_MANUFACTURER_SPECIFIC,
    0x5d, 0x00,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
    0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5,
    0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
    0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9,
    0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb,
    0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd, 0xd,
    0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf
};

/*============================================================================*
 *                              Variables
 *============================================================================*/
static uint8_t adv_handle;/**< Advertising handle*/
static T_BLE_EXT_ADV_MGR_STATE adv_state = BLE_EXT_ADV_MGR_ADV_DISABLED;

/**
 * @brief static random address struct which is storged in ftl
 *
 */
typedef struct
{
    uint8_t      is_exist;
    uint8_t      reserved;
    uint8_t      bd_addr[GAP_BD_ADDR_LEN];
} T_APP_STATIC_RANDOM_ADDR;

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief app_peripheral_adv_load_static_random_address
 * if random address already exist in ftl, then load this address.
 * @param p_addr
 * @return uint32_t
 */
uint32_t app_peripheral_adv_load_static_random_address(T_APP_STATIC_RANDOM_ADDR *p_addr)
{
    uint32_t result;
    result = ftl_load_from_storage(p_addr, APP_STATIC_RANDOM_ADDR_OFFSET,
                                   sizeof(T_APP_STATIC_RANDOM_ADDR));
    APP_PRINT_INFO1("app_peripheral_adv_load_static_random_address: result 0x%x", result);
    if (result)
    {
        memset(p_addr, 0, sizeof(T_APP_STATIC_RANDOM_ADDR));
    }
    return result;
}

/**
 * @brief app_peripheral_adv_save_static_random_address
 * save static random address into ftl
 * @param p_addr
 * @return uint32_t
 */
uint32_t app_peripheral_adv_save_static_random_address(T_APP_STATIC_RANDOM_ADDR *p_addr)
{
    APP_PRINT_INFO0("app_peripheral_adv_save_static_random_address");
    return ftl_save_to_storage(p_addr, APP_STATIC_RANDOM_ADDR_OFFSET, sizeof(T_APP_STATIC_RANDOM_ADDR));
}

/**
 * @brief app_peripheral_adv_callback
 * @param cb_type
 * @ref BLE_EXT_ADV_STATE_CHANGE this message will be send to APP when advertising state changed
 * @ref BLE_EXT_ADV_SET_CONN_INFO this message will be send to APP when connection state changed
 *
 * @param p_cb_data
 */
static void app_peripheral_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));

    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            APP_PRINT_TRACE2("app_peripheral_adv_callback: adv_state %d, adv_handle %d",
                             cb_data.p_ble_state_change->state, cb_data.p_ble_state_change->adv_handle);
            adv_state = cb_data.p_ble_state_change->state;

            if (adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                APP_PRINT_TRACE0("app_peripheral_adv_callback: BLE_EXT_ADV_MGR_ADV_ENABLED");
            }
            else if (adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                APP_PRINT_TRACE0("app_peripheral_adv_callback: BLE_EXT_ADV_MGR_ADV_DISABLED");
                switch (cb_data.p_ble_state_change->stop_cause)
                {
                case BLE_EXT_ADV_STOP_CAUSE_APP:
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_CONN:
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_TIMEOUT:
                    break;

                default:
                    break;
                }
                APP_PRINT_TRACE2("app_peripheral_adv_callback: stack stop adv cause 0x%x, app stop adv cause 0x%02x",
                                 cb_data.p_ble_state_change->stop_cause, cb_data.p_ble_state_change->app_cause);
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        APP_PRINT_TRACE2("app_peripheral_adv_callback: BLE_EXT_ADV_SET_CONN_INFO adv_handle %d, conn_id 0x%x,",
                         cb_data.p_ble_conn_info->adv_handle,
                         cb_data.p_ble_conn_info->conn_id);
        break;

    default:
        break;
    }
}

/**
 * @brief app_peripheral_adv_init_nonconn_nonscan_undirect_public
 *        advertising parameter initialization: use public address to advertise non-connectable and non-scannable undirected
 *        advertising using extended advertising PDUs.
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_peripheral_adv_init_nonconn_nonscan_undirect_public(void)
{
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop;
    adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_NON_SCAN_NON_CONN_UNDIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;

    /* Initialize extended advertising data(max size when only one advertising set is using = 1024 bytes)*/
    ble_ext_adv_mgr_init_adv_params(&adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, sizeof(ext_adv_data), ext_adv_data,
                                    0, NULL, NULL);

    ble_ext_adv_mgr_change_adv_phy(adv_handle, primary_adv_phy, secondary_adv_phy);
    ble_ext_adv_mgr_register_callback(app_peripheral_adv_callback, adv_handle);
}

/**
 * @brief app_peripheral_adv_init_nonconn_nonscan_direct_public
 *        advertising parameter initialization: use public address to advertise non-connectable and non-scannable directed advertising using
 *        extended advertising PDUs
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_peripheral_adv_init_nonconn_nonscan_direct_public(void)
{
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop;
    adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_NON_SCAN_NON_CONN_DIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    /* peer_address_type and peer_address shall be valid */
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0x42, 0x44, 0x33, 0x22, 0x11, 0x00};
    /* filter_policy shall be ignored */
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;

    /* Initialize extended advertising data(max size when only one advertising set is using = 1024 bytes)*/
    ble_ext_adv_mgr_init_adv_params(&adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, sizeof(ext_adv_data), ext_adv_data,
                                    0, NULL, NULL);

    ble_ext_adv_mgr_change_adv_phy(adv_handle, primary_adv_phy, secondary_adv_phy);
    ble_ext_adv_mgr_register_callback(app_peripheral_adv_callback, adv_handle);
}

/**
 * @brief app_peripheral_adv_init_conn_undirect_public
 *        advertising parameter initialization: use public address to advertise connectable undirected advertising using
 *        extended advertising PDUs
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_peripheral_adv_init_conn_undirect_public(void)
{
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop;
    adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_CONN_UNDIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;

    if (ADVERTISING_PHY == APP_PRIMARY_1M_SECONDARY_2M)
    {
        primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
        secondary_adv_phy = GAP_PHYS_2M;
    }
#if F_BT_LE_CODED_PHY_SUPPORT
    else if (ADVERTISING_PHY == APP_PRIMARY_CODED_SECONDARY_CODED)
    {
        primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
        secondary_adv_phy = GAP_PHYS_CODED;
    }
#endif
    else if (ADVERTISING_PHY == APP_PRIMARY_1M_SECONDARY_1M)
    {
        primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
        secondary_adv_phy = GAP_PHYS_1M;
    }

    /* Initialize extended advertising data(max size = 245 bytes)*/
    ble_ext_adv_mgr_init_adv_params(&adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, sizeof(ext_adv_data), ext_adv_data,
                                    0, NULL, NULL);

    ble_ext_adv_mgr_change_adv_phy(adv_handle, primary_adv_phy, secondary_adv_phy);
    ble_ext_adv_mgr_register_callback(app_peripheral_adv_callback, adv_handle);
}

/**
 * @brief app_peripheral_adv_init_conn_undirect_random
 *        advertising parameter initialization: use static random address to advertise connectable undirected advertising using
 *        extended advertising PDUs
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_peripheral_adv_init_conn_undirect_random(void)
{
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop;
    adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_CONN_UNDIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;

    if (ADVERTISING_PHY == APP_PRIMARY_1M_SECONDARY_2M)
    {
        primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
        secondary_adv_phy = GAP_PHYS_2M;
    }
#if F_BT_LE_CODED_PHY_SUPPORT
    else if (ADVERTISING_PHY == APP_PRIMARY_CODED_SECONDARY_CODED)
    {
        primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
        secondary_adv_phy = GAP_PHYS_CODED;
    }
#endif
    else if (ADVERTISING_PHY == APP_PRIMARY_1M_SECONDARY_1M)
    {
        primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
        secondary_adv_phy = GAP_PHYS_1M;
    }

    bool gen_addr = true;
    T_APP_STATIC_RANDOM_ADDR random_addr;

    if (app_peripheral_adv_load_static_random_address(&random_addr) == 0)
    {
        if ((random_addr.is_exist == true) && ((random_addr.bd_addr[5] & 0xC0) == 0xC0))
        {
            gen_addr = false;
        }
    }
    if (gen_addr)
    {
        if (le_gen_rand_addr(GAP_RAND_ADDR_STATIC, random_addr.bd_addr) == GAP_CAUSE_SUCCESS)
        {
            random_addr.is_exist = true;
            app_peripheral_adv_save_static_random_address(&random_addr);
        }
    }
    APP_PRINT_INFO1("app_peripheral_adv_init_conn_random: random address %b",
                    TRACE_BDADDR(random_addr.bd_addr));

    /* Initialize extended advertising data(max size = 245 bytes)*/
    ble_ext_adv_mgr_init_adv_params(&adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, sizeof(ext_adv_data), ext_adv_data,
                                    0, NULL, random_addr.bd_addr);

    ble_ext_adv_mgr_change_adv_phy(adv_handle, primary_adv_phy, secondary_adv_phy);
    ble_ext_adv_mgr_register_callback(app_peripheral_adv_callback, adv_handle);
}

/**
 * @brief app_peripheral_adv_init_conn_direct_public
 *        advertising parameter initialization: use public address to advertise connectable directed advertising using
 *        extended advertising PDUs
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_peripheral_adv_init_conn_direct_public(void)
{
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop;
    adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_CONN_DIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    /* peer_address_type and peer_address shall be valid */
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t peer_address[6] = {0x42, 0x44, 0x33, 0x22, 0x11, 0x00};
    /* filter_policy shall be ignored */
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;

    /* Initialize extended advertising data(max size = 239 bytes)*/
    ble_ext_adv_mgr_init_adv_params(&adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, sizeof(ext_adv_data), ext_adv_data,
                                    0, NULL, NULL);

    ble_ext_adv_mgr_change_adv_phy(adv_handle, primary_adv_phy, secondary_adv_phy);
    ble_ext_adv_mgr_register_callback(app_peripheral_adv_callback, adv_handle);
}

/**
 * @brief app_peripheral_adv_init_scan_undirect_public
 *        advertising parameter initialization: use public address to advertise scannable undirected advertising using
 *        extended advertising PDUs
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_peripheral_adv_init_scan_undirect_public(void)
{
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop;
    adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_SCAN_UNDIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;

    /* Initialize scan response data(max size = 991 bytes)*/
    ble_ext_adv_mgr_init_adv_params(&adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, 0, NULL,
                                    sizeof(ext_scan_rsp_data), ext_scan_rsp_data, NULL);

    ble_ext_adv_mgr_change_adv_phy(adv_handle, primary_adv_phy, secondary_adv_phy);
    ble_ext_adv_mgr_register_callback(app_peripheral_adv_callback, adv_handle);
}

/**
 * @brief app_peripheral_adv_init_scan_direct_public
 *        advertising parameter initialization: use public address to advertise scannable directed advertising using
 *        extended advertising PDUs
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval.In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_peripheral_adv_init_scan_direct_public(void)
{
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop;
    adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_SCAN_DIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    /* peer_address_type and peer_address shall be valid */
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t peer_address[6] = {0x42, 0x44, 0x33, 0x22, 0x11, 0x00};
    /* filter_policy shall be ignored */
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;

    /* Initialize extended scan response data(max size = 991 bytes)*/
    ble_ext_adv_mgr_init_adv_params(&adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, 0, NULL,
                                    sizeof(ext_scan_rsp_data), ext_scan_rsp_data, NULL);

    ble_ext_adv_mgr_change_adv_phy(adv_handle, primary_adv_phy, secondary_adv_phy);
    ble_ext_adv_mgr_register_callback(app_peripheral_adv_callback, adv_handle);
}
/**
 * @brief app_peripheral_adv_start
 *        start advertising
 * @param duration_10ms If non-zero, indicates the duration that advertising is enabled.
                        0x0000:        Always advertising, no advertising duration.
                        0x0001-0xFFFF: Advertising duration, in units of 10ms.
 * @return true         BLE protocol stack has already receive this command and ready to execute,
 *                      when this command execution complete, BLE protocol stack will send BLE_EXT_ADV_MGR_ADV_ENABLED to APP. @ref app_peripheral_adv_callback
 * @return false        There has some errors that cause the BLE protocol stack fail to receive this command.
 */
bool app_peripheral_adv_start(uint16_t duration_10ms)
{
    if (adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        APP_PRINT_INFO0("app_peripheral_adv_start");
        if (ble_ext_adv_mgr_enable(adv_handle, duration_10ms) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
        return false;
    }

    APP_PRINT_TRACE0("app_peripheral_adv_start: Already started");
    return true;
}

/**
 * @brief app_peripheral_adv_stop
 *        stop advertising
 * @param app_cause please reference app_adv_stop_cause.h
 *                  if you want to add new advertising stop cause, please added in app_adv_stop_cause.h
 * @return true     BLE protocol stack has already receive this command and ready to execute,
 *                  when this command execution complete, BLE protocol stack will send BLE_EXT_ADV_MGR_ADV_DISABLED to APP. @ref app_peripheral_adv_callback
 * @return false    There has some errors that cause the BLE protocol stack fail to receive this command.
 */
bool app_peripheral_adv_stop(int8_t app_cause)
{
    if (adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
    {
        if (ble_ext_adv_mgr_disable(adv_handle, app_cause) == GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_INFO0("app_peripheral_adv_stop");
            return true;
        }
        return false;
    }

    APP_PRINT_TRACE0("app_peripheral_adv_stop: Already stoped");
    return true;
}

/**
 * @brief app_peripheral_adv_get_state
 *        get advertising state
 * @return T_BLE_EXT_ADV_MGR_STATE
 */
T_BLE_EXT_ADV_MGR_STATE app_peripheral_adv_get_state(void)
{
    return ble_ext_adv_mgr_get_adv_state(adv_handle);
}

/**
 * @brief app_peripheral_adv_update_randomaddr  update random address
 *
 * @param random_address
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_peripheral_adv_update_randomaddr(uint8_t *random_address)
{
    return ble_ext_adv_mgr_set_random(adv_handle, random_address);
}

/**
 * @brief app_peripheral_adv_update_advdata  update advertising data
 *
 * @param p_adv_data   BLE protocol stack will not reallocate memory for adv data,
 *                     so p_adv_data shall point to a global memory.
 *                     if you don't want to set adv data, set default value NULL.
 * @param adv_data_len Advertising data or scan response data length shall not exceed 31 bytes.
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_peripheral_adv_update_advdata(uint8_t *p_adv_data, uint16_t adv_data_len)
{
    return ble_ext_adv_mgr_set_adv_data(adv_handle, adv_data_len, p_adv_data);
}

/**
 * @brief app_peripheral_adv_update_scanrspdata update scan response data
 *
 * @param p_scan_data   BLE protocol stack will not reallocate memory for scan response data,
 *                      so p_scan_data shall point to a global memory.
 *                      if you don't want to set scan response data, set default value NULL.
 * @param scan_data_len Advertising data or scan response data length shall not exceed 31 bytes.
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_peripheral_adv_update_scanrspdata(uint8_t *p_scan_data, uint16_t scan_data_len)
{
    return ble_ext_adv_mgr_set_scan_response_data(adv_handle, scan_data_len, p_scan_data);
}

/**
 * @brief app_peripheral_adv_update_interval update advertising interval
 *
 * @param adv_interval advertising interval. In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_peripheral_adv_update_interval(uint16_t adv_interval)
{
    return ble_ext_adv_mgr_change_adv_interval(adv_handle, adv_interval);
}

/** @} */ /* End of group BT5_PERIPH_STACK_API */
