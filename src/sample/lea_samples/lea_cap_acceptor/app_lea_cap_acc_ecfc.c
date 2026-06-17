/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "trace.h"
#include "gap_conn_le.h"
#include "app_lea_cap_acc_ecfc.h"

#if APP_LEA_EATT_SUPPORT
uint8_t app_ecfc_le_proto_id = 0xff;
uint16_t app_ecfc_local_mtu = 247;

uint16_t app_lea_ecfc_callback(void *p_buf, T_GAP_ECFC_MSG msg)
{
    uint16_t result = 0;

    switch (msg)
    {
    case GAP_ECFC_PROTO_REG_RSP:
        {
            T_GAP_ECFC_PROTO_REG_RSP *p_rsp = (T_GAP_ECFC_PROTO_REG_RSP *)p_buf;
            APP_PRINT_INFO3("GAP_ECFC_PROTO_REG_RSP: proto_id %d, psm 0x%x, cause 0x%x",
                            p_rsp->proto_id,
                            p_rsp->psm,
                            p_rsp->cause);
        }
        break;

    case GAP_ECFC_CONN_IND:
        {
            T_GAP_ECFC_CONN_IND *p_ind = (T_GAP_ECFC_CONN_IND *)p_buf;
            T_GAP_ECFC_CONN_CFM_CAUSE cause = GAP_ECFC_CONN_ACCEPT;
            APP_PRINT_INFO8("GAP_ECFC_CONN_IND: proto_id %d, conn_handle 0x%x, remote_mtu %d, cid_num %d, cis %b, bd_addr %s, bd_type 0x%x, identity_id 0x%x",
                            p_ind->proto_id,
                            p_ind->conn_handle,
                            p_ind->remote_mtu,
                            p_ind->cid_num,
                            TRACE_BINARY(GAP_ECFC_CREATE_CHANN_MAX_NUM * 2, p_ind->cid),
                            TRACE_BDADDR(p_ind->bd_addr),
                            p_ind->bd_type,
                            p_ind->identity_id);
            if (p_ind->remote_mtu > 512)
            {
                cause = GAP_ECFC_CONN_UNACCEPTABLE_PARAMS;
            }

            gap_ecfc_send_conn_cfm(p_ind->conn_handle, p_ind->identity_id,
                                   cause, p_ind->cid, p_ind->cid_num, app_ecfc_local_mtu);
        }
        break;

    case GAP_ECFC_CONN_CMPL:
        {
            T_GAP_ECFC_CONN_CMPL_INFO *p_info = (T_GAP_ECFC_CONN_CMPL_INFO *)p_buf;

            APP_PRINT_INFO6("GAP_ECFC_CONN_CMPL: proto_id %d, cause 0x%x, conn_handle 0x%x, ds_data_offset %d, bd_addr %s, bd_type 0x%x",
                            p_info->proto_id,
                            p_info->cause,
                            p_info->conn_handle,
                            p_info->ds_data_offset,
                            TRACE_BDADDR(p_info->bd_addr),
                            p_info->bd_type);
            APP_PRINT_INFO3("GAP_ECFC_CONN_CMPL: remote_mtu %d, local_mtu %d, local_mps %d",
                            p_info->remote_mtu,
                            p_info->local_mtu,
                            p_info->local_mps);
            for (uint8_t i = 0; i < p_info->cid_num; i++)
            {
                APP_PRINT_INFO2("GAP_ECFC_CONN_CMPL: cis[%d] 0x%x", i, p_info->cid[i]);
            }
        }
        break;

    case GAP_ECFC_DISCONN_IND:
        {
            T_GAP_ECFC_DISCONN_IND *p_ind = (T_GAP_ECFC_DISCONN_IND *)p_buf;
            APP_PRINT_INFO4("GAP_ECFC_DISCONN_IND: proto_id %d, conn_handle 0x%x, cid 0x%x, cause 0x%x",
                            p_ind->proto_id,
                            p_ind->conn_handle,
                            p_ind->cid,
                            p_ind->cause);
        }
        break;

    case GAP_ECFC_DISCONN_RSP:
        {
            T_GAP_ECFC_DISCONN_RSP *p_rsp = (T_GAP_ECFC_DISCONN_RSP *)p_buf;
            APP_PRINT_INFO4("GAP_ECFC_DISCONN_RSP: proto_id %d, conn_handle 0x%x, cid 0x%x, cause 0x%x",
                            p_rsp->proto_id,
                            p_rsp->conn_handle,
                            p_rsp->cid,
                            p_rsp->cause);
        }
        break;

    default:
        break;
    }
    return result;
}

void app_lea_ecfc_init(void)
{
    gap_ecfc_init(1);
    gap_ecfc_reg_proto(PSM_EATT, app_lea_ecfc_callback, true, &app_ecfc_le_proto_id,
                       GAP_ECFC_DATA_PATH_GATT);
}
#endif

