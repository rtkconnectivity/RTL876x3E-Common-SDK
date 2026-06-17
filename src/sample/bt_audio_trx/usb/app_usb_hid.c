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
#include "app_usb.h"
#include "app_hid_report_desc.h"
#include "os_sync.h"
#if F_APP_CFU_PASSTHROUGH_OTA_SUPPORT
#include "app_cfu_passthrough_ota.h"
#endif
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_usb_hid_ctrl.h"
//#include "app_tri_dongle_volume.h"
#endif

#if F_APP_CFU_FEATURE_SUPPORT
#include <stdlib.h>
#include "app_cfu_transfer.h"
#endif

#define USB_HID_MOUSE_EN    1
#define USB_HID_KEYBOARD_EN 1
#define USB_VOL_CMD_LEN     3
#define USB_HID_MIN(a, b)      ((a) < (b) ? (a) : (b))

void *hid_intr_in_handle = NULL;
#if USB_HID_SEC_SUPPORT
void *hid_intr_in_handle_1 = NULL;
#endif
#if USB_HID_CFU_USE_SINGLE_EP
void *hid_intr_in_handle_2 = NULL;
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
static T_USB_HID_DB usb_hid_db  = {.p_sema = NULL, .p_get_report_data = NULL, .get_report_len = 0};

static void app_usb_hid_interrupt_in_complete_result(uint32_t result, uint8_t *buf)
{
    // optimize loading
    //APP_PRINT_TRACE2("app_usb_hid_interrupt_in_complete_result %x, buf %b", result,
    //                 TRACE_BINARY(result, buf));
    if ((result <= USB_VOL_CMD_LEN) &&
        (buf[0] == REPORT_ID_CONSUMER_HOT_KEY_INPUT))   // optimize loading
    {
        T_IO_MSG gpio_msg = {0};
        gpio_msg.type = IO_MSG_TYPE_TRI_DONGLE_USB_HID;
        gpio_msg.subtype = USB_HID_MSG_INREPORT_IN_COMPLETE;

        if (app_io_msg_send(&gpio_msg) == false)
        {
            //error
        }
    }
}

// when get report need waiting data, the API is used.
bool app_usb_hid_get_report_data_is_ready(uint8_t *data, uint16_t length)
{
    bool ret = false;
    USB_PRINT_TRACE2("app_usb_hid_get_report_data_is_ready (%p), len:%d", usb_hid_db.p_get_report_data,
                     length);
    if (usb_hid_db.p_get_report_data != NULL)
    {
        if (usb_hid_db.get_report_len == length)
        {
            memcpy(usb_hid_db.p_get_report_data, data, usb_hid_db.get_report_len);
            ret = true;
        }
        else
        {
            memcpy(usb_hid_db.p_get_report_data, data, MIN(usb_hid_db.get_report_len, length));
        }
    }
    os_sem_give(usb_hid_db.p_sema);
    return ret;
}
#endif

static uint32_t app_usb_hid_interrupt_pipe_send_complete(void *handle, void *buf, uint32_t result,
                                                         int status)
{
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_usb_hid_interrupt_in_complete_result(result, (uint8_t *)buf);
#endif
    return 0;
}

bool app_usb_hid_interrupt_pipe_send(void *data, uint16_t length)
{
    bool ret = false;

    if ((app_usb_power_state() == USB_CONFIGURED) & (hid_intr_in_handle != NULL))
    {
        ret = usb_hid_data_pipe_send(hid_intr_in_handle, data, length);
    }

    return ret;
}

#if USB_HID_SEC_SUPPORT
bool app_usb_hid_interrupt_pipe_send_sec(void *data, uint16_t length)
{
    bool ret = false;

    if ((app_usb_power_state() == USB_CONFIGURED) & (hid_intr_in_handle_1 != NULL))
    {
        ret = usb_hid_data_pipe_send(hid_intr_in_handle_1, data, length);
    }

    return ret;
}
#endif

#if USB_HID_CFU_USE_SINGLE_EP
bool app_usb_hid_interrupt_pipe_send_ep4(void *data, uint32_t length)
{
    bool ret = false;

    if ((app_usb_power_state() == USB_CONFIGURED) & (hid_intr_in_handle_2 != NULL))
    {
        ret = usb_hid_data_pipe_send(hid_intr_in_handle_2, data, length);
    }

    return ret;
}
#endif

void app_usb_hid_open_pipe(void)
{
    if (hid_intr_in_handle == NULL)
    {
        T_USB_HID_ATTR attr =
        {
            .zlp = 1,
            .high_throughput = 0,
            .congestion_ctrl = HID_CONGESTION_CTRL_DROP_CUR,
            .rsv = 0,
            .mtu = HID_MAX_TRANSMISSION_UNIT
        };
        hid_intr_in_handle = usb_hid_data_pipe_open(HID_INT_IN_EP_1, attr, HID_MAX_PENDING_REQ_NUM,
                                                    app_usb_hid_interrupt_pipe_send_complete);
    }
#if USB_HID_SEC_SUPPORT
    if ((hid_intr_in_handle_1 == NULL) && inst1)
    {
        T_USB_HID_ATTR attr =
        {
            .zlp = 1,
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            .high_throughput = 1,
#else
            .high_throughput = 0,
#endif
            .congestion_ctrl = HID_CONGESTION_CTRL_DROP_CUR,
            .rsv = 0,
            .mtu = HID_MAX_TRANSMISSION_UNIT
        };
#if USB_HID_CFU_USE_SINGLE_EP
        hid_intr_in_handle_1 = usb_hid_data_pipe_open(HID_INT_IN_EP_2, attr, HID_MAX_PENDING_REQ_NUM,
                                                      app_usb_hid_interrupt_pipe_send_complete);
#else
        hid_intr_in_handle_1 = usb_hid_data_pipe_open(HID_INT_IN_EP_4, attr, HID_MAX_PENDING_REQ_NUM,
                                                      app_usb_hid_interrupt_pipe_send_complete);
#endif
    }
#endif

#if USB_HID_CFU_USE_SINGLE_EP
    if (hid_intr_in_handle_2 == NULL)
    {
        T_USB_HID_ATTR attr =
        {
            .zlp = 1,
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            .high_throughput = 1,
#else
            .high_throughput = 0,
#endif
            .congestion_ctrl = HID_CONGESTION_CTRL_DROP_CUR,
            .rsv = 0,
            .mtu = HID_MAX_TRANSMISSION_UNIT
        };
        hid_intr_in_handle_2 = usb_hid_data_pipe_open(HID_INT_IN_EP_4, attr, HID_MAX_PENDING_REQ_NUM,
                                                      app_usb_hid_interrupt_pipe_send_complete);
    }
#endif
}

void app_usb_hid_close_pipe(void)
{
    if (hid_intr_in_handle != NULL)
    {
        usb_hid_data_pipe_close(hid_intr_in_handle);
        hid_intr_in_handle = NULL;
    }
#if USB_HID_SEC_SUPPORT
    if (hid_intr_in_handle_1 != NULL)
    {
        usb_hid_data_pipe_close(hid_intr_in_handle_1);
        hid_intr_in_handle_1 = NULL;
    }
#endif
}

#if USB_HID_MOUSE_EN
void app_usb_mouse_handle_action(uint8_t action)
{
    MOUSE_HID_INPUT_REPORT mouse_code;

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

    mouse_code.pointer.id = HID_REPORT_ID_MOUSE;
    app_usb_hid_interrupt_pipe_send(&mouse_code, sizeof(mouse_code));

    memset(&mouse_code, 0, sizeof(MOUSE_HID_INPUT_REPORT));
    mouse_code.pointer.id = HID_REPORT_ID_MOUSE;
    app_usb_hid_interrupt_pipe_send(&mouse_code, sizeof(mouse_code));
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

    key_code.key.id = HID_REPORT_ID_KEYBOARD;
    app_usb_hid_interrupt_pipe_send(&key_code, sizeof(key_code));

    memset(&key_code, 0, sizeof(KEY_HID_INPUT_REPORT));
    key_code.key.id = HID_REPORT_ID_KEYBOARD;
    app_usb_hid_interrupt_pipe_send(&key_code, sizeof(key_code));
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

void app_usb_mmi_handle_action(uint8_t action)
{
    APP_PRINT_TRACE1("app_usb_mmi_handle_action: action 0x%02x", action);
    switch (action)
    {
    case MMI_USB_AUDIO_VOL_UP:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.vol_inc = 1;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
            hid_data.report.vol_inc = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;
    case MMI_USB_AUDIO_VOL_DOWN:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.vol_dec = 1;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
            hid_data.report.vol_dec = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;
    case MMI_USB_AUDIO_PLAY_PAUSE:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.play_pause = 1;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
            hid_data.report.play_pause = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;
    case MMI_USB_AUDIO_STOP:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.stop = 1;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
            hid_data.report.stop = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;
    case MMI_USB_AUDIO_FWD:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.next_track = 1;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
            hid_data.report.next_track = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;
    case MMI_USB_AUDIO_BWD:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.prev_track = 1;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
            hid_data.report.prev_track = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;
    case MMI_USB_AUDIO_FASTFORWARD:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.fast_fwd = 1;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
            hid_data.report.fast_fwd = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;
    case MMI_USB_AUDIO_REWIND:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.rewind = 1;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
            hid_data.report.rewind = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;
    case MMI_USB_AUDIO_FASTFORWARD_STOP:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.fast_fwd = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;
    case MMI_USB_AUDIO_REWIND_STOP:
        {
            CONSUMER_HID_INPUT_REPORT hid_data;
            memset(&hid_data, 0, sizeof(CONSUMER_HID_INPUT_REPORT));
            hid_data.report.id = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
            hid_data.report.rewind = 0;
#if USB_HID_CFU_USE_SINGLE_EP
            app_usb_hid_interrupt_pipe_send_ep4((uint8_t *)&hid_data, sizeof(hid_data));
#else
            app_usb_hid_interrupt_pipe_send((uint8_t *)&hid_data, sizeof(hid_data));
#endif
        }
        break;

    default:
        break;
    }
}

static int8_t app_usb_hid_set_report_handle(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                            uint16_t length)
{
    APP_PRINT_TRACE3("app_usb_hid_set_report_handle id 0x%x, length %d, data %b",
                     req_value.id, length, TRACE_BINARY(USB_HID_MIN(3, length), data));

    // TO DO
    if (app_usb_power_state() == USB_CONFIGURED)
    {
        switch (req_value.id)
        {
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#if F_APP_CFU_FEATURE_SUPPORT
        case REPORT_ID_CFU_FEATURE_EX:
        case REPORT_ID_CFU_OFFER_INPUT:
        case REPORT_ID_CFU_FEATURE:
        case REPORT_ID_CFU_PAYLOAD_INPUT:
            app_cfu_set_report(req_value, data, length);
            break;
#endif
#if F_APP_CFU_PASSTHROUGH_OTA_SUPPORT
        case REPORT_ID_PASSTHROUGH_CFU_FEATURE_EX:
        case REPORT_ID_PASSTHROUGH_CFU_OFFER_INPUT:
        case REPORT_ID_PASSTHROUGH_CFU_FEATURE:
        case REPORT_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT:
            app_cfu_passthrough_ota_set_report(req_value, data, length);
            break;
#endif
        default:
            app_tri_dongle_usb_hid_set_report_handle(req_value, data, length);
            break;
#endif
        }
    }

    return 0;
}

static int8_t app_usb_hid_get_report_handle(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                            uint16_t *length)
{
    uint8_t ret = 0;

    APP_PRINT_TRACE2("app_usb_hid_get_report_handle, id 0x%x, type 0x%x", req_value.id, req_value.type);

    // TO DO;
    if (app_usb_power_state() == USB_CONFIGURED)
    {
        switch (req_value.id)
        {
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#if F_APP_CFU_FEATURE_SUPPORT
        case REPORT_ID_CFU_FEATURE:
        case REPORT_ID_CFU_FEATURE_EX:
            ret = app_cfu_get_report(req_value, data, length);
            break;
#endif
#if F_APP_CFU_PASSTHROUGH_OTA_SUPPORT
        case REPORT_ID_PASSTHROUGH_CFU_FEATURE:
        case REPORT_ID_PASSTHROUGH_CFU_FEATURE_EX:
            ret = app_cfu_passthrough_ota_get_report(req_value, data, length);
            break;
#endif
        default:
            ret = app_tri_dongle_usb_hid_get_report_handle(req_value, data, length);
            break;
#endif
        }
    }

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    if (ret)
    {
        usb_hid_db.p_get_report_data = data;
        usb_hid_db.get_report_len = *length;

        APP_PRINT_WARN0("app_tri_dongle_usb_hid_get_report_handle waiting...");
        if (usb_hid_db.p_sema == NULL)
        {
            if (os_sem_create(&usb_hid_db.p_sema, "usb_hid_sema", 0, 1) == false)
            {
                return 0;
            }
        }
        if (os_sem_take(usb_hid_db.p_sema, 0xFFFFFFFFUL))
        {
            usb_hid_db.p_get_report_data = NULL;
            usb_hid_db.get_report_len = 0;
            ret = 0;
        }
    }

    APP_PRINT_TRACE4("app_usb_hid_get_report_handle ID 0x%x, result %d, length 0x%x, %b", req_value.id,
                     ret, *length, TRACE_BINARY(*length, data));
#endif
    return ret;
}

void app_usb_hid_init(void)
{
    T_HID_CBS cbs = {.get_report = (INT_IN_FUNC)app_usb_hid_get_report_handle, .set_report = (INT_OUT_FUNC)app_usb_hid_set_report_handle};
    usb_hid_ual_register(cbs);
}

void app_usb_hid_deinit(void)
{
    usb_hid_ual_unregister();
}
#endif

