/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "os_sync.h"
#include "os_msg.h"
#include "section.h"
#include "trace.h"
#include "app_adp.h"
#include "app_main.h"
#include "app_charger.h"
#include "app_ble_gap.h"
#include "app_transfer.h"
#include "app_console_msg.h"
#include "app_line_in.h"
#include "app_key_process.h"
#include "app_key_gpio.h"

#if (F_SOURCE_PLAY_SUPPORT == 1)
#include "app_src_play.h"
#endif

#if F_APP_QDECODE_SUPPORT
#include "app_qdec.h"
#endif

#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
#include "a2dp_src_stream_ctrl.h"
#endif

#if (F_APP_SPI_ROLE_MASTER || F_APP_SPI_ROLE_SLAVE)
#include "app_spi_common.h"
#endif

#if F_APP_A2DP_XMIT_SRC_LEA_SUPPORT
#include "app_a2dp_xmit_lea.h"
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
#include "app_findmy.h"
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_ecc.h"
#include "app_gfps_cfg.h"
#endif

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include "ble_audio.h"
#endif
#if F_APP_USB_AUDIO_SUPPORT | F_APP_USB_MSC_SUPPORT | F_APP_USB_HID_SUPPORT | F_APP_USB_CDC_SUPPORT
#include "app_usb.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_usb_hid_ctrl.h"
#if F_APP_BT_AUDIO_TRI_DONGLE_2_4G
#include "app_tri_dongle_spi.h"
#endif
#include "app_tri_dongle_uac.h"
#endif

#if F_APP_ENABLE_TWO_ONE_WIRE_UART
#include "app_uart.h"
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
        APP_PRINT_ERROR3("app_io_msg_send failed, type 0x%x, subtype 0x%x, param 0x%x",
                         io_msg->type, io_msg->subtype, io_msg->u.param);
    }

    return ret;
}

void app_io_msg_handler(T_IO_MSG io_driver_msg_recv)
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
                    app_key_handle_msg(&io_driver_msg_recv);
                }
                break;

            case IO_MSG_GPIO_UART_WAKE_UP:
                {
                    app_transfer_handle_msg(&io_driver_msg_recv);
                }
                break;

            case IO_MSG_GPIO_CHARGER:
                {
                    app_charger_handle_msg(&io_driver_msg_recv);
                }
                break;

            case IO_MSG_GPIO_ADAPTOR_DAT:
                {

                }
                break;

#if F_APP_LINEIN_SUPPORT
            case IO_MSG_GPIO_LINE_IN:
                {
                    app_line_in_detect_msg_handler(&io_driver_msg_recv);
                }
                break;
#endif

            case IO_MSG_GPIO_KEY0:
                {
                    key_gpio_key0_debounce_start();
                }
                break;

#if F_APP_QDECODE_SUPPORT
            case IO_MSG_GPIO_QDEC:
                {
                    app_qdec_msg_handler(&io_driver_msg_recv);
                }
                break;
#endif

            case IO_MSG_GPIO_SMARTBOX_COMMAND_PROTECT:
                {

                }
                break;

            case IO_MSG_GPIO_ADP_INT:
                {

                }
                break;

            case IO_MSG_GPIO_ADP_HW_TIMER_HANDLER:
                {

                }
                break;

            case IO_MSG_GPIO_ADAPTOR_PLUG:
            case IO_MSG_GPIO_ADAPTOR_UNPLUG:
                {
                    app_adp_msg_handler(io_driver_msg_recv.subtype);
                }
                break;

            case IO_MSG_GPIO_BUD_LOC_CHANGE:
                {

                }
                break;

            default:
                break;
            }
        }
        break;

    case IO_MSG_TYPE_BT_STATUS:
        {
            app_ble_gap_handle_gap_msg(&io_driver_msg_recv);

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
            app_findmy_handle_gap_msg(&io_driver_msg_recv);
#endif
        }
        break;

#if F_APP_CONSOLE_SUPPORT
    case IO_MSG_TYPE_CONSOLE:
        {
            app_console_handle_msg(io_driver_msg_recv);
        }
        break;
#endif

#if F_APP_BT_AUDIO_TRI_DONGLE_2_4G
    case IO_MSG_TYPE_SPI:
        {
            app_tri_dongle_handle_spi_msg(&io_driver_msg_recv);
        }
        break;
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    case IO_MSG_TYPE_ECC:
        {
            app_gfps_ecc_handle_msg();
        }
        break;
#endif

#if F_APP_MUSIC_A2DP_SOURCE_SUPPORT
    case IO_MSG_TYPE_A2DP_SRC:
        {
            a2dp_src_stream_handle_msg(io_driver_msg_recv);
        }
        break;
#endif

#if F_APP_USB_AUDIO_SUPPORT | F_APP_USB_MSC_SUPPORT | F_APP_USB_HID_SUPPORT | F_APP_USB_CDC_SUPPORT
    case IO_MSG_TYPE_USB_DEV:
        {
            app_usb_msg_handle(&io_driver_msg_recv);
        }
        break;
#endif

#if (F_APP_SPI_ROLE_MASTER || F_APP_SPI_ROLE_SLAVE)
    case IO_MSG_TYPE_AUDIO_TRANS_SPI:
        {
            app_spi_msg_handle(io_driver_msg_recv);
        }
        break;
#endif

    case IO_MSG_TYPE_WRISTBNAD:
        {
            switch (io_driver_msg_recv.subtype)
            {
            default:
                break;
            }
        }
        break;

#if F_APP_A2DP_XMIT_SRC_LEA_SUPPORT
    case IO_MSG_TYPE_LEA_SRC:
        {
            app_a2dp_xmit_lea_msg_handle(io_driver_msg_recv);
        }
        break;
#endif

#if CONFIG_REALTEK_FINDMY_FEATURE_SUPPORT
    case IO_MSG_TYPE_FINDMY:
        {
            app_findmy_handle_io_msg(io_driver_msg_recv);
        }
        break;
#endif

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
    case IO_MSG_TYPE_LE_AUDIO:
        {
            ble_audio_handle_msg(&io_driver_msg_recv);
        }
        break;
#endif

#if (F_SOURCE_PLAY_SUPPORT == 1)
    case IO_MSG_TYPE_SRC_PLAY:
        {
            app_src_play_handle_msg(&io_driver_msg_recv);
        }
        break;
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    case IO_MSG_TYPE_TRI_DONGLE_USB_HID:
        {
            app_tri_dongle_usb_hid_handle_msg(&io_driver_msg_recv);
        }
        break;

    case IO_MSG_TYPE_USB_UAC:
        {
            app_tri_dongle_uac_handle_msg(&io_driver_msg_recv);
        }
        break;
#endif

#if (F_APP_ENABLE_TWO_ONE_WIRE_UART == 1)
    case IO_MSG_TYPE_UART:
        {
            app_uart_handle_rx_msg(&io_driver_msg_recv);
        }
        break;
#endif

    default:
        break;
    }
}
