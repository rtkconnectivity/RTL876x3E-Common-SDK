/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <string.h>
#include "rtl876x_pinmux.h"
#include "trace.h"
#include "os_task.h"
#include "os_mem.h"
#include "os_sched.h"
#include "os_msg.h"
#include "gap.h"
#include "gap_br.h"
#include "btm.h"
#include "sysm.h"
#include "remote.h"
#include "audio.h"
#include "app_msg.h"
#include "app_io_msg.h"
#include "console_uart.h"
#include "app_bt_pan_demo_console.h"
#include "app_msg.h"
#include "app_bt_pan_demo_gap.h"
#include "app_bt_pan_demo.h"
#include "app_bt_pan_demo_link.h"
#include "app_dlps.h"
#if F_APP_DSP_SHM_80KB_TO_MCU_CHECK_SUPPORT
#include "os_heap.h"
#include "mem_config.h"
#include "system_status_api.h"
#endif

/** @defgroup  BT_PAN_DEMO_MAIN BT PAN Demo Main
    * @brief Main file to initialize hardware and BT stack and start task scheduling
    * @{
    */

#define BT_PAN_DEMO_UART_TX          P3_1
#define BT_PAN_DEMO_UART_RX          P3_0

#define BT_PAN_DEMO_DEFAULT_PAGESCAN_WINDOW             0x12
#define BT_PAN_DEMO_DEFAULT_PAGESCAN_INTERVAL           0x800 //0x800
#define BT_PAN_DEMO_DEFAULT_PAGE_TIMEOUT                0x4000
#define BT_PAN_DEMO_DEFAULT_SUPVISIONTIMEOUT            0x1f40 //0x7D00
#define BT_PAN_DEMO_DEFAULT_INQUIRYSCAN_WINDOW          0x12
#define BT_PAN_DEMO_DEFAULT_INQUIRYSCAN_INTERVAL        0x800 //0x1000

#define PLAYBACK_POOL_SIZE                  (13*1024)
#define VOICE_POOL_SIZE                     (2*1024)
#define RECORD_POOL_SIZE                    (1*1024)
#define NOTIFICATION_POOL_SIZE              (8*1024)

#define BT_PAN_DEMO_MAX_NUMBER_OF_GAP_MESSAGE     0x20      //!<  GAP message queue size
#define BT_PAN_DEMO_MAX_NUMBER_OF_IO_MESSAGE      0x20      //!<  IO message queue size
#define BT_PAN_DEMO_MAX_NUMBER_OF_EVENT_MESSAGE   (BT_PAN_DEMO_MAX_NUMBER_OF_GAP_MESSAGE + BT_PAN_DEMO_MAX_NUMBER_OF_IO_MESSAGE)    //!< Event message queue size

#define CONSOLE_UART_RX_BUFFER_SIZE         300

uint8_t bd_addr_local[6]  = {0x13, 0x22, 0x56, 0xab, 0xbc, 0xdd};

void *bt_pan_demo_evt_queue_handle;  //!< Event queue handle
void *bt_pan_demo_io_queue_handle;   //!< IO queue handle

/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
  * @brief  Initialize gap bond manager related parameters
  * @return void
  */
static void app_bt_gap_init(void)
{
    uint16_t supervision_timeout = BT_PAN_DEMO_DEFAULT_SUPVISIONTIMEOUT;

    uint16_t link_policy = GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE;

    uint8_t radio_mode = GAP_RADIO_MODE_VISIBLE_CONNECTABLE;
    bool limited_discoverable = false;
    bool auto_accept_acl = false;

    uint8_t pagescan_type = GAP_RADIO_MODE_VISIBLE_CONNECTABLE;
    uint16_t pagescan_interval = BT_PAN_DEMO_DEFAULT_PAGESCAN_INTERVAL;
    uint16_t pagescan_window = BT_PAN_DEMO_DEFAULT_PAGESCAN_WINDOW;
    uint16_t page_timeout = BT_PAN_DEMO_DEFAULT_PAGE_TIMEOUT;

    uint8_t inquiryscan_type = GAP_INQUIRY_SCAN_TYPE_STANDARD;
    uint16_t inquiryscan_window = BT_PAN_DEMO_DEFAULT_INQUIRYSCAN_WINDOW;
    uint16_t inquiryscan_interval = BT_PAN_DEMO_DEFAULT_INQUIRYSCAN_INTERVAL;
    uint8_t inquiry_mode = GAP_INQUIRY_MODE_EXTENDED_RESULT;

    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_GENERAL_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t oob_enable = false;
    uint8_t bt_mode = GAP_BT_MODE_21ENABLED;

    gap_lib_init();

    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);

    gap_br_set_param(GAP_BR_PARAM_BT_MODE, sizeof(uint8_t), &bt_mode);
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

    gap_set_bd_addr(bd_addr_local);
    memcpy(app_db.local_addr, bd_addr_local, 6);

    gap_br_set_param(GAP_BR_PARAM_NAME, GAP_DEVICE_NAME_LEN, "pan device");
}

/**
 * @brief    Contains the initialization of bt profile
 * @return   void
 */
static void app_bt_profile_init(void)
{
    app_bt_pan_demo_init();
}

/**
 * @brief    Contains the initialization of framework
 * @return   void
 */
static void framework_init(void)
{
    /* System Manager */
    sys_mgr_init(bt_pan_demo_evt_queue_handle);

    /* RemoteController Manager */
    remote_mgr_init(REMOTE_SESSION_ROLE_SINGLE);

    /* Bluetooth Manager */
    bt_mgr_init();

    /* Audio Manager */
    //audio_mgr_init(PLAYBACK_POOL_SIZE, VOICE_POOL_SIZE, RECORD_POOL_SIZE, NOTIFICATION_POOL_SIZE);
}

/**
 * @brief    Contains the initialization of pinmux settings and pad settings
 * @note     All the pinmux settings and pad settings shall be initiated in this function,
 *           but if legacy driver is used, the initialization of pinmux setting and pad setting
 *           should be peformed with the IO initializing.
 * @return   void
 */
void board_init(void)
{
    Pinmux_Config(BT_PAN_DEMO_UART_TX, UART0_TX);
    Pad_Config(BT_PAN_DEMO_UART_TX,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(BT_PAN_DEMO_UART_RX, UART0_RX);
    Pad_Config(BT_PAN_DEMO_UART_RX,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

/**
 * @brief    Contains the initialization of peripherals
 * @note     Both new architecture driver and legacy driver initialization method can be used
 * @return   void
 */
static void driver_init(void)
{
    T_CONSOLE_PARAM console_param;
    T_CONSOLE_OP    console_op;
    T_CONSOLE_UART_CONFIG console_uart_config;

    console_uart_config.one_wire_uart_support = 0;
    console_uart_config.uart_rx_pinmux = BT_PAN_DEMO_UART_RX;
    console_uart_config.uart_tx_pinmux = BT_PAN_DEMO_UART_TX;
    console_uart_config.rx_wake_up_pinmux = BT_PAN_DEMO_UART_RX;
    console_uart_config.enable_rx_wake_up = 0;
    console_uart_config.data_uart_baud_rate = BAUD_RATE_115200;
    console_uart_config.callback = NULL;
    console_uart_config.uart_dma_rx_buffer_size = CONSOLE_UART_RX_BUFFER_SIZE;

#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
    console_uart_config.uart_rx_dma_enable = true;
#else
    console_uart_config.uart_rx_dma_enable = false;
#endif
    console_uart_config.uart_tx_dma_enable = false;
    console_uart_config_init(&console_uart_config);

    console_param.tx_buf_size   = 512;
    console_param.rx_buf_size   = 512;
    console_param.tx_wakeup_pin = BT_PAN_DEMO_UART_TX;
    console_param.rx_wakeup_pin = BT_PAN_DEMO_UART_RX;

    console_op.init = console_uart_init;
    console_op.tx_wakeup_enable = NULL; //console_uart_tx_wakeup_enable;
    console_op.rx_wakeup_enable = NULL; //console_uart_rx_wakeup_enable;
    console_op.write = console_uart_write;
    console_op.wakeup = console_uart_wakeup;
    console_op.tx_empty = NULL;

    console_init(&console_param, &console_op);
    console_set_mode(CONSOLE_MODE_STRING);
}

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_param    Parameters sending to the task
 * @return       void
 */
void pan_demo_main_task(void *p_param)
{
    uint8_t event;

    gap_start_bt_stack(bt_pan_demo_evt_queue_handle, bt_pan_demo_io_queue_handle,
                       BT_PAN_DEMO_MAX_NUMBER_OF_GAP_MESSAGE);

    while (true)
    {
        if (os_msg_recv(bt_pan_demo_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (EVENT_GROUP(event) == EVENT_GROUP_IO)
            {
                T_IO_MSG io_msg;

                if (os_msg_recv(bt_pan_demo_io_queue_handle, &io_msg, 0) == true)
                {
                    if (event == EVENT_IO_TO_APP)
                    {
                        app_io_handle_msg(io_msg);
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
        }
    }
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in PAN Demo APP, thus only one APP task is init here
 * @return   void
 */
void task_init(void)
{
    void *app_task_handle;
    os_msg_queue_create(&bt_pan_demo_io_queue_handle, "ioQ", BT_PAN_DEMO_MAX_NUMBER_OF_IO_MESSAGE,
                        sizeof(T_IO_MSG));
    os_msg_queue_create(&bt_pan_demo_evt_queue_handle, "evtQ", BT_PAN_DEMO_MAX_NUMBER_OF_EVENT_MESSAGE,
                        sizeof(unsigned char));
    os_task_create(&app_task_handle, "pan_demo_main_task", pan_demo_main_task, NULL, 1024 * 3, 1);
}

#if F_APP_DSP_SHM_80KB_TO_MCU_CHECK_SUPPORT
void shm_data_copy(void)
{
#if defined (__CC_ARM) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
    extern unsigned int Load$$SHARE_RAM_DATA$$RW$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RW$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RW$$Length;
    extern unsigned int Load$$SHARE_RAM_DATA$$ZI$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$ZI$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$ZI$$Length;

    uint32_t load_addr = (uint32_t)&Load$$SHARE_RAM_DATA$$RW$$Base;
    uint32_t dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$RW$$Base;
    uint32_t len = (uint32_t)&Image$$SHARE_RAM_DATA$$RW$$Length;
    memcpy((uint8_t *)dest_addr, (uint8_t *)load_addr, len);

    dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$ZI$$Base;
    len = (uint32_t)&Image$$SHARE_RAM_DATA$$ZI$$Length;
    memset((uint8_t *)dest_addr, 0, len);

    extern unsigned int Load$$SHARE_RAM_DATA$$RO$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RO$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RO$$Length;

    load_addr = (uint32_t)&Load$$SHARE_RAM_DATA$$RO$$Base;
    dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$RO$$Base;
    len = (uint32_t)&Image$$SHARE_RAM_DATA$$RO$$Length;
    memcpy((uint8_t *)dest_addr, (uint8_t *)load_addr, len);
#else
    extern uint32_t *__share_ram_load_addr__;
    extern uint32_t *__share_ram_dst_addr__;
    extern uint32_t __ram_code_length__;

    memcpy((uint8_t *)__share_ram_dst_addr__, (uint8_t *)__share_ram_load_addr__, __ram_code_length__);
#endif
}


void ram_config()
{
    /**
     *
        API ret 1, Is80KShared2MCU = 0x01 ==> new dsp header, share 80K
        API ret 1, Is80KShared2MCU = 0x00 ==> new dsp header, snot share 80K
        API ret 0, Is80KShared2MCU = 0x01 ==> old dsp image
        API ret 0, Is80KShared2MCU = 0x00 ==> old dsp image
    */
    sys_hall_set_dsp_share_memory_80k(false);
    heap_shm_set(DSP_SHM_GLOBAL_ADDR, DSP_SHM_TOTAl_SIZE, 0);
    shm_data_copy();
}
#endif

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int main(void)
{
#if F_APP_DSP_SHM_80KB_TO_MCU_CHECK_SUPPORT
    ram_config();
#endif
    board_init();
    driver_init();
    task_init();
    framework_init();
    app_bt_gap_init();
    app_bt_profile_init();
    app_dlps_init();
    app_bt_pan_demo_gap_init();
    app_bt_pan_demo_cmd_register();

    os_sched_start();

    return 0;
}
/** @} */ /* End of group BT_PAN_DEMO_MAIN */


