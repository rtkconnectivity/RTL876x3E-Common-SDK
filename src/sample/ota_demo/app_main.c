/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include "os_mem.h"
#include "os_sched.h"
#include "os_msg.h"
#include "os_task.h"
#include "os_ext.h"
#include "trace.h"
#include "audio.h"
#include "sysm.h"
#include "gap_br.h"
#include "gap.h"
#include "fmc_api.h"
#include "spp_stream.h"
#include "ble_stream.h"
#include "app_main.h"
#include "app_gap.h"
#include "app_ble_gap.h"
#include "app_ble_service.h"
#include "app_transfer.h"
#include "app_timer.h"
#include "app_sdp.h"
#include "app_spp.h"
#include "app_cmd.h"
#include "app_ota.h"
#include "app_cfg.h"


/** @defgroup APP_MAIN App Main
  * @brief Main entry function for audio sample application.
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup OTA_APP_TASK_Exported_Macros OTA App Task Exported Macros
    * @{
    */
/**Task priorities */
#define APP_TASK_PRIORITY       1
/**Task stack size in bytes */
#define APP_TASK_STACK_SIZE     1024 * 2

#define MAX_NUMBER_OF_GAP_MESSAGE       0x20
#define MAX_NUMBER_OF_IO_MESSAGE        0x20
#define MAX_NUMBER_OF_EVENT_MESSAGE     (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE)

#define MAX_NUMBER_OF_DSP_MSG           0x20    //!< number of dsp message reserved for DSP message handling.
#define MAX_NUMBER_OF_CODEC_MSG         0x10    //!< number of codec message reserved for CODEC message handling.
#define MAX_NUMBER_OF_ANC_MSG           0x10    //!< number of anc message reserved for ANC message handling.
#define MAX_NUMBER_OF_SYS_MSG           0x20    //!< indicate SYS timer queue size
#define MAX_NUMBER_OF_LOADER_MSG        0x20    //!< indicate Bin Loader queue size
#define MAX_NUMBER_OF_APP_TIMER_MODULE  0x30    //!< indicate app timer module size

/** indicate rx event queue size*/
#define MAX_NUMBER_OF_RX_EVENT      \
    (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE + MAX_NUMBER_OF_APP_TIMER_MODULE + \
     MAX_NUMBER_OF_DSP_MSG + MAX_NUMBER_OF_CODEC_MSG + MAX_NUMBER_OF_ANC_MSG + \
     MAX_NUMBER_OF_SYS_MSG + MAX_NUMBER_OF_LOADER_MSG)

#define DEFAULT_PAGESCAN_WINDOW         0x12
#define DEFAULT_PAGESCAN_INTERVAL       0x800
#define DEFAULT_PAGE_TIMEOUT            0x2000
#define DEFAULT_SUPVISIONTIMEOUT        0x1f40
#define DEFAULT_INQUIRYSCAN_WINDOW      0x12
#define DEFAULT_INQUIRYSCAN_INTERVAL    0x1000

#define PLAYBACK_POOL_SIZE              (13*1024)
#define VOICE_POOL_SIZE                 (2*1024)
#define RECORD_POOL_SIZE                (1*1024)
#define NOTIFICATION_POOL_SIZE          (8*1024)

void *ota_demo_evt_queue_handle;
void *ota_demo_io_queue_handle;

T_APP_DB app_db;
T_APP_CFG_NV app_cfg_nv;

/**
* @brief board_init() contains the initialization of pinmux settings and pad settings.
*
*   All the pinmux settings and pad settings shall be initiated in this function.
*   But if legacy driver is used, the initialization of pinmux setting and pad setting
*   should be peformed with the IO initializing.
*
* @return void
*/
static void board_init(void)
{
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Peripheral APP, thus only one APP task is init here
 * @return   void
 */


static void app_bt_gap_init(void)
{
    uint32_t class_of_device = (uint32_t)(0x18 | (0x04 << 8) | (0x24 << 16));

    uint16_t supervision_timeout = DEFAULT_SUPVISIONTIMEOUT;
    uint16_t link_policy = GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE;
    uint8_t radio_mode = GAP_RADIO_MODE_VISIBLE_CONNECTABLE;
    bool limited_discoverable = false;
    bool auto_accept_acl = false;

    uint8_t  pagescan_type = GAP_RADIO_MODE_VISIBLE_CONNECTABLE;//GAP_PAGE_SCAN_TYPE_INTERLACED;
    uint16_t pagescan_interval = DEFAULT_PAGESCAN_INTERVAL;
    uint16_t pagescan_window = DEFAULT_PAGESCAN_WINDOW;
    uint16_t page_timeout = DEFAULT_PAGE_TIMEOUT;

    uint8_t inquiryscan_type = GAP_INQUIRY_SCAN_TYPE_STANDARD;
    uint16_t inquiryscan_window = DEFAULT_INQUIRYSCAN_WINDOW;
    uint16_t inquiryscan_interval = DEFAULT_INQUIRYSCAN_INTERVAL;
    uint8_t inquiry_mode = GAP_INQUIRY_MODE_EXTENDED_RESULT;

    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_GENERAL_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t oob_enable = false;
    uint8_t bt_mode = GAP_BT_MODE_21ENABLED;

    gap_lib_init();

    //1: to be slave when accept the acl connect request by default.
    gap_br_cfg_accept_role(1);

    gap_br_set_param(GAP_BR_PARAM_NAME, GAP_DEVICE_NAME_LEN, "ota demo");

    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);

    gap_br_set_param(GAP_BR_PARAM_BT_MODE, sizeof(uint8_t), &bt_mode);
    gap_br_set_param(GAP_BR_PARAM_COD, sizeof(uint32_t), &class_of_device);
    gap_br_set_param(GAP_BR_PARAM_LINK_POLICY, sizeof(uint16_t), &link_policy);
    gap_br_set_param(GAP_BR_PARAM_SUPV_TOUT, sizeof(uint16_t), &supervision_timeout);
    gap_br_set_param(GAP_BR_PARAM_AUTO_ACCEPT_ACL, sizeof(bool), &auto_accept_acl);

    gap_br_set_param(GAP_BR_PARAM_RADIO_MODE, sizeof(uint8_t), &radio_mode);
    gap_br_set_param(GAP_BR_PARAM_LIMIT_DISCOV, sizeof(bool), &limited_discoverable);

    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_TYPE, sizeof(uint8_t), &pagescan_type);
    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_INTERVAL, sizeof(uint16_t), &pagescan_interval);
    gap_br_set_param(GAP_BR_PARAM_PAGE_SCAN_WINDOW, sizeof(uint16_t), &pagescan_window);
    gap_br_set_param(GAP_BR_PARAM_PAGE_TIMEOUT, sizeof(uint16_t), &page_timeout);

    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_TYPE, sizeof(uint8_t), &inquiryscan_type);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_INTERVAL, sizeof(uint16_t), &inquiryscan_interval);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_SCAN_WINDOW, sizeof(uint16_t), &inquiryscan_window);
    gap_br_set_param(GAP_BR_PARAM_INQUIRY_MODE, sizeof(uint8_t), &inquiry_mode);

#ifdef OTA_DEMO_BY_BLE_ENABLE
    app_ble_gap_param_init();
#endif
}

static void framework_init(void)
{
    /* System Manager */
    sys_mgr_init(ota_demo_evt_queue_handle);

    /* RemoteController Manager */
    remote_mgr_init(REMOTE_SESSION_ROLE_SINGLE);

    /* Bluetooth Manager */
    bt_mgr_init();

    /* Audio Manager */
    audio_mgr_init(PLAYBACK_POOL_SIZE, VOICE_POOL_SIZE, RECORD_POOL_SIZE, NOTIFICATION_POOL_SIZE);
}

void app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        {
            app_ble_gap_handle_gap_msg(&io_msg);
        }
        break;
    default:
        break;
    }
}

/**
    * @brief        App task to handle events & messages
    * @param[in]    p_params    Parameters sending to the task
    * @return       void
    */
static void ota_demo_main_task(void *p_params)
{
    char event;

    APP_PRINT_INFO2("ota_demo_main_task OTA_DEMO_TASK_STACK_SIZE:%d OTA_DEMO_TASK_PRIORITY:%d",
                    APP_TASK_STACK_SIZE,
                    APP_TASK_PRIORITY);

    gap_start_bt_stack(ota_demo_evt_queue_handle, ota_demo_io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

    while (true)
    {
        if (os_msg_recv(ota_demo_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (EVENT_GROUP(event) == EVENT_GROUP_IO)
            {
                T_IO_MSG io_msg;

                if (os_msg_recv(ota_demo_io_queue_handle, &io_msg, 0) == true)
                {
                    if (event == EVENT_IO_TO_APP)
                    {
                        app_handle_io_msg(io_msg);
                    }
                }
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_STACK)
            {
                gap_handle_msg(event);
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_FRAMEWORK)
            {
                sys_mgr_event_handle(event);
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_APP)
            {
                app_timer_handle_msg(event);
            }
        }
    }
}

/** @} */ /* End of group OTA_APP_TASK_Exported_Functions */

/** @} */ /* End of group OTA_APP_TASK */


int main(void)
{
    void *app_task_handle;

    APP_PRINT_INFO0("OTA_DEMO START FROM HW_RESET");

    os_msg_queue_create(&ota_demo_io_queue_handle, "ioQ", MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&ota_demo_evt_queue_handle, "evtQ", MAX_NUMBER_OF_EVENT_MESSAGE,
                        sizeof(unsigned char));

    app_init_timer(ota_demo_evt_queue_handle, MAX_NUMBER_OF_APP_TIMER_MODULE);

    board_init();
    framework_init();

    app_bt_gap_init();
    app_ota_init();

    app_gap_init();

#ifdef OTA_DEMO_BY_BLE_ENABLE
    app_ble_gap_init();
    app_ble_service_init();
#endif

    app_sdp_init();
    app_spp_init();

    app_transfer_init();

    os_task_create(
        &app_task_handle,       /* Task handle. */
        "ota_demo_main_task",   /* Text name for the task, to facilitate debug only. */
        ota_demo_main_task,     /* Pointer to the function that implements the task. */
        (void *)0,              /* We are not using the task parameter. */
        APP_TASK_STACK_SIZE,    /* Stack depth in bytes, 1KB. */
        APP_TASK_PRIORITY       /* This task will run at priority 2. */
    );

    os_sched_start();

    return 0;
}


/** End of APP_MAIN
* @}
*/

