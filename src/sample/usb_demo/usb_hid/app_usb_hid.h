/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_USB_MMI_H__
#define __APP_USB_MMI_H__

typedef union
{
    uint8_t data[4];
    struct
    {
        uint8_t press;
        uint8_t x;
        uint8_t y;
    } pointer;
} MOUSE_HID_INPUT_REPORT;

typedef union
{
    uint8_t data[8];
    struct
    {
        // the system must look at the modifier key bits (Byte 0, bits 7-0)
        // to determine if any of the SHIFT, CTRL, ALT,
        // or GUI keys has changed state since the last report
        uint8_t modifier;
        uint8_t resv;
        uint8_t code1;
        uint8_t code2;
        uint8_t code3;
        uint8_t code4;
        uint8_t code5;
        uint8_t code6;
    } key;
} KEY_HID_INPUT_REPORT;

typedef enum
{
    MMI_MOUSE_UP            = 0x5,
    MMI_MOUSE_DWON          = 0x6,
    MMI_MOUSE_LEFT          = 0x7,
    MMI_MOUSE_RIGHT         = 0x8,
    MMI_KEYBOARD_NUM,
    MMI_KEYBOARD_LETTER,
} T_USB_HID_MMI_DEMO;

#define HID_MAX_PENDING_REQ_NUM    0x05

#define KEY_ID_a_A          0x04
#define KEY_ID_b_B          0x05
#define KEY_ID_c_C          0x06
#define KEY_ID_d_D          0x07
#define KEY_ID_e_E          0x08
#define KEY_ID_f_F          0x09
#define KEY_ID_g_G          0x0A
#define KEY_ID_h_H          0x0B
#define KEY_ID_i_I          0x0C
#define KEY_ID_j_J          0x0D
#define KEY_ID_k_K          0x0E
#define KEY_ID_l_L          0x0F
#define KEY_ID_m_M          0x10
#define KEY_ID_n_N          0x11
#define KEY_ID_o_O          0x12
#define KEY_ID_p_P          0x13
#define KEY_ID_q_Q          0x14
#define KEY_ID_r_R          0x15
#define KEY_ID_s_S          0x16
#define KEY_ID_t_T          0x17
#define KEY_ID_u_U          0x18
#define KEY_ID_v_V          0x19
#define KEY_ID_w_W          0x1A
#define KEY_ID_x_X          0x1B
#define KEY_ID_y_Y          0x1C
#define KEY_ID_z_Z          0x1D

#define KEY_ID_1_End        0x59
#define KEY_ID_2_Down_Arrow 0x5A
#define KEY_ID_3_PageDn     0x5B
#define KEY_ID_4_Left_Arrow 0x5C
#define KEY_ID_5            0x5D
#define KEY_ID_6_Right_Arrow 0x5E
#define KEY_ID_7_Home       0x5F
#define KEY_ID_8_Up_Arrow   0x60
#define KEY_ID_9_PageUp     0x61
#define KEY_ID_0_Insert     0x62

void app_usb_mouse_handle_action(uint8_t action);
void app_usb_keyboard_handle_action(uint8_t action);
void app_usb_hid_init(void);
#endif
