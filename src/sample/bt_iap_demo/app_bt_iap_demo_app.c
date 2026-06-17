/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "rtl876x_rcc.h"
#include "rtl876x_i2c.h"
#include "platform_utils.h"
#include "bt_types.h"
#include "gap_br.h"
#include "console.h"
#include "btm.h"
#include "bt_sdp.h"
#include "bt_iap.h"
#include "app_bt_iap_demo_sdp.h"
#include "app_bt_iap_demo_app.h"
#include "app_bt_iap_demo_link.h"

#define CP_DEVICE_ADDRESS 0x10

#define MSG_BLUETOOTH_COMPONENT_INFO        0x4E01
#define MSG_START_BT_CONNECTION_UPDATES     0x4E03
#define MSG_BT_CONNECTION_UPDATE            0x4E04
#define MSG_STOP_BT_CONNECTION_UPDATES      0x4E05
#define MSG_START_CALL_STATE_UPDATES        0x4154
#define MSG_CALL_STATE_UPDATE               0x4155
#define MSG_STOP_CALL_STATE_UPDATES         0x4156
#define MSG_START_COMM_UPDATES              0x4157
#define MSG_MUTE_STATUS_UPDATE              0x4160

extern char *local_name;
extern uint8_t local_bd_addr[6];

I2C_TypeDef *p_i2c_cp = I2C0;

uint8_t remote_bd_addr[6];

/** @brief  IAP UUID */
static const T_BT_SDP_UUID_DATA rfcomm_service_uuid16 =
{
    .uuid_16 = UUID_RFCOMM
};

bool iap_demo_conn_start(uint8_t *bd_addr)
{
    APP_PRINT_TRACE0("iap_demo_conn_start");
    if (bt_sdp_discov_start(bd_addr, BT_SDP_UUID16, rfcomm_service_uuid16))
    {
        return true;
    }

    return false;
}

bool iap_demo_disconnect_start(void)
{
    T_IAP_DEMO_LINK *p_link;

    APP_PRINT_TRACE0("iap_demo_disconnect_start");

    p_link = iap_demo_find_link(remote_bd_addr);
    if (p_link != NULL)
    {
        return bt_iap_disconnect_req(remote_bd_addr);
    }

    return false;
}

/**
 * @brief     iAP Tx function for test.
 * @param[in] Remote BT address..
 * @return    void
 */
bool iap_demo_app_tx_data_test(void)
{
    T_IAP_DEMO_LINK *p_link;
    uint8_t iap_tx_data_test[15] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08, 0x09, 0x0a};

    APP_PRINT_TRACE0("iap_demo_app_tx_data_test");
    p_link = iap_demo_find_link(remote_bd_addr);
    if (p_link != NULL)
    {
        if (p_link->session_connected == true)
        {
            char *temp_buff = "iAP Data Sent!\r\n";
            console_write((uint8_t *)temp_buff, strlen(temp_buff));

            return bt_iap_data_send(remote_bd_addr, p_link->session_id, iap_tx_data_test, 15);
        }
    }

    return false;
}

/**
 * @brief     iAP Rx function for test.
 * @param[in] Remote BT address..
 * @param[in] Pointer to iAP Rx buffer.
 * @param[in] iAP Rx buffer length.
 * @return    void
 */
static void iap_demo_app_rx_data_test(uint8_t *bd_addr, uint8_t *buf, uint16_t len)
{
    APP_PRINT_TRACE2("iap_demo_app_rx_data_test: bd_addr %s len %d", TRACE_BDADDR(bd_addr), len);
    for (uint16_t i = 0; i < len; i++)
    {
        APP_PRINT_TRACE2("iap_demo_app_rx_data_test: buf[i:%d] 0x%x", i, buf[i]);
    }

#if F_DLPS_EN
    bt_sniff_mode_enable(bd_addr, 784, 816, 0, 0);
#endif
}

void app_iap_cp_hw_init(void)
{
    //initilize GPIO PINs
    I2C_InitTypeDef I2C_InitStructure = {0};

    //Init I2C

    if (p_i2c_cp == I2C0)
    {
        RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);
    }

    if (p_i2c_cp == I2C1)
    {
        RCC_PeriphClockCmd(APBPeriph_I2C1, APBPeriph_I2C1_CLOCK, ENABLE);
    }

    /* I2C configuration */
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_DeviveMode = I2C_DeviveMode_Master;
    I2C_InitStructure.I2C_AddressMode = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_SlaveAddress = CP_DEVICE_ADDRESS;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;

    /* I2C module Initialize */
    I2C_Init(p_i2c_cp, &I2C_InitStructure);
    /* I2C Peripheral Enable */
    I2C_Cmd(p_i2c_cp, ENABLE);

    I2C_INTConfig(p_i2c_cp, I2C_INT_TX_ABRT, ENABLE);
}

static bool app_iap_cp_check_i2c_flag(uint32_t flag)
{
    FlagStatus flag_status = RESET;
    volatile uint16_t retry_count = 1000;

    while (retry_count)
    {
        flag_status = I2C_GetFlagState(p_i2c_cp, flag);
        if (flag_status == SET)
        {
            //APP_PRINT_TRACE1("app_cp_check_i2c_flag: retry_count %d", retry_count);
            return true;
        }
        else
        {
            retry_count--;
        }
    }

    APP_PRINT_ERROR0("app_iap_cp_check_i2c_flag: 2 CP_CAUSE_TIMEOUT");
    return false;
}

static bool app_iap_cp_write(uint8_t start_addr, uint8_t *data, uint16_t length)
{
    int32_t ret = 0;
    uint8_t no_ack;
    uint16_t i, j;

    APP_PRINT_TRACE0("app_iap_cp_write");

    if (p_i2c_cp == NULL)
    {
        ret = 1;
        goto TAG_END;
    }

    I2C_SetSlaveAddress(p_i2c_cp, CP_DEVICE_ADDRESS);
    j = 100;

    //write device address(W) & start register address
    do
    {
        no_ack = 0;
        if (length == 0)
        {
            p_i2c_cp->IC_DATA_CMD = start_addr | (1 << 9);
        }
        else
        {
            p_i2c_cp->IC_DATA_CMD = start_addr;
        }

        //wait tx fifo empty
        if (app_iap_cp_check_i2c_flag(I2C_FLAG_TFE) == false)
        {
            ret = 2;
            goto TAG_END;
        }

        i = 200; //can not too short!!!!!
        while (1)
        {
            no_ack = I2C_CheckEvent(p_i2c_cp, ABRT_7B_ADDR_NOACK);
            i--;
            if (no_ack == 1 || i == 0)
            {
                break;
            }
        }

        if (no_ack)
        {
            //clear ABRT_7B_ADDR_NOACK bit
            I2C_ClearINTPendingBit(p_i2c_cp, I2C_INT_TX_ABRT);
            //delay min 500us and retry
            platform_delay_us(500);
            j--;
            if (j == 0)
            {
                break;
            }
        }
    }

    while (no_ack);

    //write data
    if (length > 0 && data != NULL)
    {
        while (length > 1)
        {
            p_i2c_cp->IC_DATA_CMD = *data++;
            //wait tx fifo empty
            if (app_iap_cp_check_i2c_flag(I2C_FLAG_TFE) == false)
            {
                ret = 3;
                goto TAG_END;
            }

            length--;
        }

        //stop
        p_i2c_cp->IC_DATA_CMD = ((*data++) | (1 << 9));
        if (app_iap_cp_check_i2c_flag(I2C_FLAG_TFE) == false)
        {
            ret = 4;
            goto TAG_END;
        }

    }

    return true;

TAG_END:
    APP_PRINT_TRACE1("app_iap_cp_write: ret: %d", -ret);
    return false;
}

static bool app_iap_cp_burst_read(uint8_t start_addr, uint8_t *pbuf, uint16_t length)
{
    int32_t ret = 0;
    uint8_t no_ack = 0;
    uint16_t i, j;

    APP_PRINT_TRACE0("app_iap_cp_burst_read");

    if (p_i2c_cp == NULL)
    {
        ret = 1;
        goto TAG_END;
    }

    //write start register address
    if (app_iap_cp_write(start_addr, NULL, 0) == false)
    {
        ret = 2;
        goto TAG_END;
    }

    I2C_SetSlaveAddress(p_i2c_cp, CP_DEVICE_ADDRESS);

    //read command
    j = 100;

    do
    {
        no_ack = 0;
        if (length == 1)
        {
            /* genarate stop signal */
            p_i2c_cp->IC_DATA_CMD = ((0x00) | (1 << 8) | (1 << 9));
        }
        else
        {
            p_i2c_cp->IC_DATA_CMD = (0x00) | (1 << 8);
        }

        //wait tx fifo empty
        if (app_iap_cp_check_i2c_flag(I2C_FLAG_TFE) == false)
        {
            ret = 3;
            goto TAG_END;
        }

        i = 100;
        while (1)
        {
            no_ack = I2C_CheckEvent(p_i2c_cp, ABRT_7B_ADDR_NOACK);
            i--;
            if (no_ack == 1 || i == 0)
            {
                break;
            }
        }

        if (no_ack)
        {
            //clear ABRT_7B_ADDR_NOACK bit
            I2C_ClearINTPendingBit(p_i2c_cp, I2C_INT_TX_ABRT);
            //delay min 500us and retry
            platform_delay_us(500);
            j--;
            if (j == 0)
            {
                break;
            }
        }
    }

    while (no_ack);

    while (length > 1)
    {
        if (length == 2)
        {
            /* genarate stop signal */
            p_i2c_cp->IC_DATA_CMD = ((0x00) | (1 << 8) | (1 << 9));
        }
        else
        {
            p_i2c_cp->IC_DATA_CMD = (0x00) | (1 << 8);
        }

        //wait rx fifo data
        if (app_iap_cp_check_i2c_flag(I2C_FLAG_RFNE) == false)
        {
            ret = 4;
            goto TAG_END;
        }

        *pbuf++ = I2C_ReceiveData(p_i2c_cp);
        length--;
    }

    //wait rx fifo data, last byte
    if (app_iap_cp_check_i2c_flag(I2C_FLAG_RFNE) == false)
    {
        ret = 5;
        goto TAG_END;
    }

    *pbuf++ = (uint8_t)p_i2c_cp->IC_DATA_CMD;

    return true;

TAG_END:
    APP_PRINT_TRACE1("app_iap_cp_burst_read: ret %d", -ret);
    return false;
}

static void app_iap_send_ident_info(uint8_t *bd_addr)
{
    uint8_t *p_buf;
    uint8_t *p;
    uint8_t *p_head;

    p_buf = malloc(300);
    if (p_buf == NULL)
    {
        return;
    }

    APP_PRINT_TRACE1("app_iap_send_ident_info: p_buf %p", p_buf);

    p_head = p_buf;

    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_NAME, local_name, strlen((const char *)local_name) + 1);

    char mudule_identifier[] = "1.0";
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_MODEL_IDENTIFIER, mudule_identifier,
                               sizeof(mudule_identifier));

    char manufacturer[] = "realtek";
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_MANUFACTURER, manufacturer, sizeof(manufacturer));

    char serial_num[] = "1.0";
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_SERIAL_NUMBER, serial_num, sizeof(serial_num));

    char fw_version[6] = "1.0.0";
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_FIRMWARE_VERSION, fw_version, sizeof(fw_version));

    char hw_version[6] = "2.0.0";
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_HARDWARE_VERSION, hw_version, sizeof(hw_version));

    uint8_t msg_sent_by_accessory[8];
    p = msg_sent_by_accessory;
    BE_UINT16_TO_STREAM(p, MSG_BLUETOOTH_COMPONENT_INFO);
    BE_UINT16_TO_STREAM(p, MSG_START_BT_CONNECTION_UPDATES);
    BE_UINT16_TO_STREAM(p, MSG_STOP_BT_CONNECTION_UPDATES);
    BE_UINT16_TO_STREAM(p, BT_IAP_MSG_EA_PROTOCOL_SESSION_STATUS);
    //BE_UINT16_TO_STREAM(p, MSG_START_CALL_STATE_UPDATES);
    //BE_UINT16_TO_STREAM(p, MSG_STOP_CALL_STATE_UPDATES);
    //BE_UINT16_TO_STREAM(p, MSG_MUTE_STATUS_UPDATE);
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_MESSAGES_SENT_BY_ACCESSORY, msg_sent_by_accessory,
                               sizeof(msg_sent_by_accessory));

    uint8_t msg_received_from_device[6];
    p = msg_received_from_device;
    BE_UINT16_TO_STREAM(p, BT_IAP_MSG_START_EA_PROTOCOL_SESSION);
    BE_UINT16_TO_STREAM(p, BT_IAP_MSG_STOP_EA_PROTOCOL_SESSION);
    BE_UINT16_TO_STREAM(p, MSG_BT_CONNECTION_UPDATE);
    //BE_UINT16_TO_STREAM(p, BT_IAP_MSG_EA_PROTOCOL_SESSION_STATUS);
    //BE_UINT16_TO_STREAM(p, MSG_CALL_STATE_UPDATE);
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_MESSAGES_RECEIVED_FROM_DEVICE, msg_received_from_device,
                               sizeof(msg_received_from_device));

    //ID_POWER_SOURCETYPE
    uint8_t power_source = 0;
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_POWER_SOURCETYPE, &power_source, sizeof(power_source));

    //ID_MAXIMUM_CURRENT_DRAWN_FROM_DEVICE

    uint16_t max_current_drawn_from_device = 0;
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_MAXIMUM_CURRENT_DRAWN_FROM_DEVICE,
                               &max_current_drawn_from_device,
                               sizeof(max_current_drawn_from_device));


    //Supported EA protocol group
    {
        uint8_t *p = p_buf;
        p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_SUPPORTED_EXTERNAL_ACCESSORY_PROTOCOL, NULL, 0);

        //group : EA_PROTOCOL_ID
        uint8_t ea_protocol_id = EA_PROTOCOL_ID_RTK;
        p_buf = bt_iap2_prep_param(p_buf, BT_IAP_EXT_ACC_PROTOCOL_ID, &ea_protocol_id,
                                   sizeof(ea_protocol_id));

        char ea_protocol_name[] = "com.rtk.datapath";
        p_buf = bt_iap2_prep_param(p_buf, BT_IAP_EXT_ACC_PROTOCOL_NAME, ea_protocol_name,
                                   sizeof(ea_protocol_name));

        //group EA_PROTOCOL_MATCHACTION
        uint8_t ea_protocol_match_action = 0x02;
        p_buf = bt_iap2_prep_param(p_buf, BT_IAP_EXT_ACC_PROTOCOL_MATCHACTION, &ea_protocol_match_action,
                                   sizeof(ea_protocol_match_action));

        //calculate total Length
        uint16_t total_len = p_buf - p;
        BE_UINT16_TO_STREAM(p, total_len);
    }

    //ID_PREFERRED_APP_MATCH_TEAM_ID
    char app_match_team_id[] = "C6P64J2MZX";
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_PREFERRED_APP_MATCH_TEAM_ID, app_match_team_id,
                               sizeof(app_match_team_id));

    //ID_CURRENT_LANGUAGE
    char current_language[] = "en";
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_CURRENT_LANGUAGE, current_language,
                               sizeof(current_language));

    //ID_SUPPORTED_LANGUAGE
    char supported_language[] = "en";
    p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_SUPPORTED_LANGUAGE, &supported_language,
                               sizeof(supported_language));

    //BT_COMPONENT_ID group
    {
        uint8_t *p = p_buf;
        p_buf = bt_iap2_prep_param(p_buf, BT_IAP_ID_BLUETOOTH_TRANSPORT_COMPONENT, NULL, 0);

        //group : BT_COMPONENT_ID

        uint16_t component_id = 0x5442;
        p_buf = bt_iap2_prep_param(p_buf, BT_IAP_TRANSPORT_COMPONENT_IDENTIFIER, &component_id,
                                   sizeof(component_id));

        //group TRANSPORT_COMPONENT_NAME
        char acc_name[] = "component_name";
        p_buf = bt_iap2_prep_param(p_buf, BT_IAP_TRANSPORT_COMPONENT_NAME, acc_name, sizeof(acc_name));

        //group TRANSPORT_SUPPORTS_IAP2_CONNECTION
        p_buf = bt_iap2_prep_param(p_buf, BT_IAP_TRANSPORT_SUPPORTS_IAP2_CONNECTION, NULL, 0);

        uint8_t mac_addr[6] = {0x00};
        mac_addr[0] = local_bd_addr[5];
        mac_addr[1] = local_bd_addr[4];
        mac_addr[2] = local_bd_addr[3];
        mac_addr[3] = local_bd_addr[2];
        mac_addr[4] = local_bd_addr[1];
        mac_addr[5] = local_bd_addr[0];
        p_buf = bt_iap2_prep_param(p_buf, BT_IAP_BLUETOOTH_TRANSPORT_MEDIA_ACCESS_CONTROL_ADDRESS, mac_addr,
                                   sizeof(mac_addr));

        //calculate total Length
        uint16_t total_len = p_buf - p;
        BE_UINT16_TO_STREAM(p, total_len);
    }

    APP_PRINT_TRACE2("app_iap_send_ident_info: p_buf %p, p_head %p", p_buf, p_head);

    bt_iap_ident_info_send(bd_addr, p_head, p_buf - p_head);
    free(p_head);
}

/**
 * @brief    BT Manager iAP related events are handled in this function
 * @note     Then the event handling function shall be called according to the
 *           event_type of T_BT_EVENT.
 * @param[in] event_type BT manager event type
 * @param[in] event_buf  Pointer to BT manager event buffer.
 * @param[in] buf_len    BT manager event buffer length.
 * @return   void
 */
static void iap_demo_app_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    static bool bt_comp_info_send_flag = false;
    T_BT_EVENT_PARAM *param = event_buf;
    T_IAP_DEMO_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_IAP_CONN_IND:
        {
            uint16_t frame_size = param->iap_conn_ind.frame_size;

            p_link = iap_demo_find_link(param->iap_conn_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("iap_demo_app_bt_cback: no acl link found");
                return;
            }

            bt_iap_connect_cfm(p_link->bd_addr,  true, frame_size, IAP_DEMO_IAP_DEFAULT_CREDITS);
        }
        break;

    case BT_EVENT_IAP_CONN_CMPL:
        {
            p_link = iap_demo_find_link(param->iap_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                memcpy(remote_bd_addr, param->iap_conn_cmpl.bd_addr, 6);
                p_link->rfc_iap_frame_size = param->iap_conn_cmpl.frame_size;
            }
        }
        break;

    case BT_EVENT_IAP_CONN_FAIL:
        break;

    case BT_EVENT_IAP_IDENTITY_INFO_REQ:
        {
            p_link = iap_demo_find_link(param->iap_identity_info_req.bd_addr);
            if (p_link != NULL)
            {
                app_iap_send_ident_info(param->iap_identity_info_req.bd_addr);
            }
        }
        break;

    case BT_EVENT_IAP_AUTHEN_CMPL:
        {
            p_link = iap_demo_find_link(param->iap_authen_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (param->iap_authen_cmpl.result == true)
                {
                    char *temp_buff = "iAP Connected!\r\n";
                    console_write((uint8_t *)temp_buff, strlen(temp_buff));

                    bt_comp_info_send_flag = true;
                    p_link->authen_flag = true;
                }
                else
                {
                    p_link->authen_flag = false;
                    bt_iap_disconnect_req(param->iap_authen_cmpl.bd_addr);
                }
            }
        }
        break;

    case BT_EVENT_IAP_DATA_SESSION_OPEN:
        {
            p_link = iap_demo_find_link(param->iap_data_session_open.bd_addr);
            if (p_link != NULL)
            {
                if (bt_comp_info_send_flag == true)
                {
                    bt_comp_info_send_flag = false;
                    bt_iap_comp_info_send(param->iap_data_session_open.bd_addr, 0x4254, true);
                }
                p_link->session_connected = true;
                p_link->session_id = param->iap_data_session_open.session_id;
            }
        }
        break;

    case BT_EVENT_IAP_DATA_SESSION_CLOSE:
        {
            p_link = iap_demo_find_link(param->iap_data_session_close.bd_addr);
            if (p_link != NULL)
            {
                p_link->session_connected = false;
                p_link->session_id = 0;
            }
        }
        break;

    case BT_EVENT_IAP_DATA_IND:
        {
            p_link = iap_demo_find_link(param->iap_data_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("iap_demo_app_bt_cback: no acl link found");
                return;
            }
            //iAP data recieved.
            iap_demo_app_rx_data_test(param->iap_data_ind.bd_addr, param->iap_data_ind.p_data,
                                      param->iap_data_ind.len);
            bt_iap_ack_send(param->iap_data_ind.bd_addr, param->iap_data_ind.dev_seq_num);
        }
        break;

    case BT_EVENT_IAP_DISCONN_CMPL:
        {
            p_link = iap_demo_find_link(param->iap_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                char *temp_buff = "iAP Disconnected!\r\n";
                console_write((uint8_t *)temp_buff, strlen(temp_buff));

                p_link->authen_flag = false;
                p_link->session_connected = false;
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("iap_demo_app_bt_cback: event_type 0x%04x", event_type);
    }
}

void iap_demo_app_init(void)
{
    APP_PRINT_INFO0("iap_demo_app_init");

    bt_iap_init(IAP_DEMO_RFC_IAP_CHANN_NUM, app_iap_cp_write, app_iap_cp_burst_read);

    bt_mgr_cback_register(iap_demo_app_bt_cback);
}

