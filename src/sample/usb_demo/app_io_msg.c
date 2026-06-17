/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "os_sync.h"
#include "os_msg.h"
#include "trace.h"
#include "app_main.h"
#include "section.h"

#if F_APP_USB_AUDIO_SUPPORT || F_APP_USB_HID_SUPPORT
#include "app_usb.h"
#endif

RAM_TEXT_SECTION bool app_io_msg_send(T_IO_MSG *io_msg)
{
    T_EVENT_TYPE event = EVENT_IO_TO_APP;
    bool ret = false;

    if (os_msg_send(audio_io_queue_handle, io_msg, 0) == true)
    {
        ret = os_msg_send(audio_evt_queue_handle, &event, 0);
    }

    if (ret == false)
    {
        APP_PRINT_ERROR3("app_io_msg_send failed: type 0x%x, subtype 0x%x, param 0x%x",
                         io_msg->type, io_msg->subtype, io_msg->u.param);
    }

    return ret;
}

void app_io_handle_msg(T_IO_MSG io_driver_msg_recv)
{
    uint16_t msgtype = io_driver_msg_recv.type;

    switch (msgtype)
    {
    case IO_MSG_TYPE_GPIO:
        {
            switch (io_driver_msg_recv.subtype)
            {
            case IO_MSG_GPIO_KEY:
                {
#if GPIO_KEY_EN
                    extern void app_key_handle_msg(T_IO_MSG * io_driver_msg_recv);
                    app_key_handle_msg(&io_driver_msg_recv);
#endif
                }
                break;

            case IO_MSG_GPIO_UART_WAKE_UP:
                {
//                    app_transfer_handle_msg(&io_driver_msg_recv);
                }
                break;

            case IO_MSG_GPIO_CHARGER:
                {
//                    app_charger_handle_msg(&io_driver_msg_recv);
                }
                break;

            case IO_MSG_GPIO_ADAPTOR_PLUG:
            case IO_MSG_GPIO_ADAPTOR_UNPLUG:
                {
//                    app_adp_msg_handler(io_driver_msg_recv.subtype);
                }
                break;

            default:
                break;
            }
        }
        break;

#if F_APP_CONSOLE_SUPPORT
    case IO_MSG_TYPE_CONSOLE:
        {
            app_console_handle_msg(io_driver_msg_recv);
        }
        break;
#endif

#if F_APP_USB_AUDIO_SUPPORT || F_APP_USB_HID_SUPPORT
    case IO_MSG_TYPE_USB_UAC:
        {
            app_usb_msg_handle(&io_driver_msg_recv);
        }
        break;

    case IO_MSG_TYPE_USB_HID:
        {

        }
        break;
#endif

    default:
        break;
    }
}

