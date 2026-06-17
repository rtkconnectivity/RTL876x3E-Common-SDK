/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_HID_DEMO_H_
#define _APP_BT_HID_DEMO_H_

#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_HID_DEMO BT HID Demo
* @brief APP BT HID Demo
* @{
*/

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_BT_HID_DEMO_Exported_Macros BT HID Demo Exported Macros
  * @brief
  * @{
  */



/** End of APP_BT_HID_DEMO_Exported_Macros
* @}
*/

typedef struct t_hid_host_descriptor
{
    struct t_hid_host_descriptor *next;
    uint8_t                       bd_addr[6];
    uint16_t                      descriptor_len;
    uint8_t                       descriptor[0];
} T_HID_HOST_DESCRIPTOR;

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BT_HID_DEMO_Exported_Functions BT HID Demo Exported Functions
  * @brief
  * @{
  */

void app_bt_hid_demo_init(void);

bool app_bt_hid_demo_connect(uint8_t *bd_addr);

bool app_bt_hid_demo_disconnect(uint8_t *bd_addr);

bool app_bt_hid_demo_mouse_shift_left(uint8_t *bd_addr);

bool app_bt_hid_demo_mouse_shift_right(uint8_t *bd_addr);

bool app_bt_hid_demo_mouse_shift_up(uint8_t *bd_addr);

bool app_bt_hid_demo_mouse_shift_down(uint8_t *bd_addr);

bool app_bt_hid_demo_mouse_click(uint8_t *bd_addr);

bool app_bt_hid_demo_sniff(uint8_t *bd_addr, uint16_t interval);

bool app_bt_hid_demo_enable_dlps(uint8_t *bd_addr);

bool app_bt_hid_demo_keyboard_click_keycode(uint8_t *bd_addr, uint8_t keycode);

bool app_bt_hid_demo_host_connect(uint8_t *bd_addr);

bool app_bt_hid_demo_host_disconnect(uint8_t *bd_addr);

bool app_bt_hid_demo_host_bond_delete(uint8_t *bd_addr);

bool app_bt_hid_demo_host_virtual_cable_unplug(uint8_t *bd_addr);

bool app_bt_hid_demo_host_get_report(uint8_t *bd_addr,
                                     uint8_t  report_id);

bool app_bt_hid_demo_host_set_report(uint8_t *bd_addr,
                                     uint8_t  report_id);

bool app_bt_hid_demo_host_set_protocol(uint8_t *bd_addr,
                                       uint8_t  proto_mode);

bool app_bt_hid_demo_host_interrupt_data_send(uint8_t *bd_addr,
                                              uint8_t  report_id);

void app_bt_hid_demo_host_handle_interrupt_report(T_HID_HOST_DESCRIPTOR *hid_descriptor,
                                                  const uint8_t         *report,
                                                  uint16_t               report_len);

T_HID_HOST_DESCRIPTOR *app_hid_host_alloc_descriptor(uint8_t   bd_addr[6],
                                                     uint8_t  *descriptor,
                                                     uint16_t  descriptor_len);

T_HID_HOST_DESCRIPTOR *app_hid_host_descriptor_find(uint8_t *bd_addr);

void app_hid_host_free_descriptor(T_HID_HOST_DESCRIPTOR *hid_descriptor);

/** @} End of APP_BT_HID_DEMO_Exported_Functions */

/** @} End of APP_BT_HID_DEMO */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_HID_DEMO_H_ */
