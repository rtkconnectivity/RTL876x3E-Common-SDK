/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_HID_SUPPORT
#include <stdint.h>
#include "trace.h"
#include "usb_hid_desc.h"
#include "usb_hid.h"
#include "app_usb_hid.h"
#include "string.h"

static void *hid_mmi_handle = NULL;

#if USB_HID_MOUSE_EN
void app_usb_mouse_handle_action(uint8_t action)
{
    MOUSE_HID_INPUT_REPORT mouse_code;

    if (hid_mmi_handle == NULL)
    {
        T_USB_HID_ATTR attr =
        {
            .zlp = 1,
            .high_throughput = 0,
            .congestion_ctrl = HID_CONGESTION_CTRL_DROP_CUR,
            .rsv = 0,
            .mtu = sizeof(mouse_code)
        };
        hid_mmi_handle = usb_hid_data_pipe_open(HID_INT_IN_EP_1, attr, HID_MAX_PENDING_REQ_NUM, NULL);
    }
    APP_PRINT_INFO1("app_usb_mouse_handle_action, action:0x%x", action);
    memset(&mouse_code, 0, sizeof(MOUSE_HID_INPUT_REPORT));
    switch (action)
    {
    case MMI_MOUSE_UP:
        {
            mouse_code.pointer.y = 0xE0;
        }
        break;

    case MMI_MOUSE_DWON:
        {
            mouse_code.pointer.y = 0x20;
        }
        break;

    case MMI_MOUSE_LEFT:
        {
            mouse_code.pointer.x = 0xE0;
        }
        break;

    case MMI_MOUSE_RIGHT:
        {
            mouse_code.pointer.x = 0x20;
        }
        break;

    default:
        {
            return;
        }
    }

    usb_hid_data_pipe_send(hid_mmi_handle, mouse_code.data, sizeof(mouse_code));

    memset(&mouse_code, 0, sizeof(MOUSE_HID_INPUT_REPORT));
    usb_hid_data_pipe_send(hid_mmi_handle, mouse_code.data, sizeof(mouse_code));
}
#endif

#if USB_HID_KEYBOARD_EN
#define NUMBER_CNT      10
#define LETTER_CNT      26

static uint8_t s_keycode_cnt = 0;
void app_usb_keyboard_handle_action(uint8_t action)
{
    KEY_HID_INPUT_REPORT key_code;
    uint8_t num_offset = 0;
    uint8_t letter_offset = 0;

    if (hid_mmi_handle == NULL)
    {
        T_USB_HID_ATTR attr =
        {
            .zlp = 1,
            .high_throughput = 0,
            .congestion_ctrl = HID_CONGESTION_CTRL_DROP_CUR,
            .rsv = 0,
            .mtu = sizeof(key_code)
        };
        hid_mmi_handle = usb_hid_data_pipe_open(HID_INT_IN_EP_1, attr, HID_MAX_PENDING_REQ_NUM, NULL);
    }
    APP_PRINT_INFO1("app_usb_keyboard_handle_action, action:0x%x", action);
    memset(&key_code, 0, sizeof(KEY_HID_INPUT_REPORT));
    switch (action)
    {
    case MMI_KEYBOARD_NUM:
        {
            num_offset = s_keycode_cnt;
            APP_PRINT_TRACE1("keycode_num: %d", num_offset);
            key_code.key.code1 = KEY_ID_1_End + num_offset;
        }
        break;

    case MMI_KEYBOARD_LETTER:
        {
            letter_offset = s_keycode_cnt - NUMBER_CNT;
            APP_PRINT_TRACE1("keycode_letter: %d", letter_offset);
            key_code.key.code1 = KEY_ID_a_A + letter_offset;
        }
        break;

    default:
        {
            return;
        }
    }

    usb_hid_data_pipe_send(hid_mmi_handle, key_code.data, sizeof(key_code));

    memset(&key_code, 0, sizeof(KEY_HID_INPUT_REPORT));
    usb_hid_data_pipe_send(hid_mmi_handle, key_code.data, sizeof(key_code));
}

void app_usb_keyboard_input_demo(void)
{
    if (s_keycode_cnt < NUMBER_CNT)
    {
        app_usb_keyboard_handle_action(MMI_KEYBOARD_NUM);
        s_keycode_cnt++;
    }
    else if (s_keycode_cnt < NUMBER_CNT + LETTER_CNT)
    {
        app_usb_keyboard_handle_action(MMI_KEYBOARD_LETTER);
        s_keycode_cnt++;
    }

    if (s_keycode_cnt >= NUMBER_CNT + LETTER_CNT)
    {
        s_keycode_cnt = 0;
    }

}

#endif

static int8_t app_usb_hid_set_report_handle(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                            uint16_t length)
{
    APP_PRINT_TRACE2("app_usb_hid_set_report_handle id 0x%x, data0 0x%x", req_value.id, data[0]);
    // TO DO
    return 0;
}

static int8_t app_usb_hid_get_report_handle(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                            uint16_t *length)
{
    uint8_t ret = 0;

    APP_PRINT_TRACE2("app_usb_hid_get_report_handle, id 0x%x, type 0x%x", req_value.id, req_value.type);

    // TO DO;

    return ret;
}

void app_usb_hid_init(void)
{
    T_HID_CBS cbs = {.get_report = (INT_IN_FUNC)app_usb_hid_get_report_handle, .set_report = (INT_OUT_FUNC)app_usb_hid_set_report_handle};
    usb_hid_ual_register(cbs);
}
#endif

