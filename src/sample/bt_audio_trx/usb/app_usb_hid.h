/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __APP_USB_HID_H__
#define __APP_USB_HID_H__
#include <stddef.h>
#include "app_msg.h"
#include "app_hid_report_desc.h"
#include "usb_hid.h"

#define HID_MAX_TRANSMISSION_UNIT                   0x40
#define HID_MAX_PENDING_REQ_NUM                     0x08

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

//#define USB_HID_MSG_TYPE_CONSUMER_CRTL              REPORT_ID_CONSUMER_HOT_KEY_INPUT
//#define USB_HID_MSG_TYPE_HID_IN_COMPLETE            0xF3
typedef union
{
    uint8_t data[3];
    struct
    {
        uint8_t id;
        uint8_t vol_inc         : 1;
        uint8_t vol_dec         : 1;
        uint8_t play_pause      : 1;
        uint8_t next_track      : 1;
        uint8_t prev_track       : 1;
        uint8_t stop            : 1;
        uint8_t fast_fwd        : 1;
        uint8_t rewind          : 1;

        uint8_t mute            : 1;
        uint8_t reserved        : 7;
    } report;
} CONSUMER_HID_INPUT_REPORT;

typedef union
{
    uint8_t data[3];
    struct
    {
        uint8_t id;
        uint8_t hook_switch   : 1;
        uint8_t busy_tone     : 1;
        uint8_t line          : 1;
        uint8_t mute          : 1;
        uint8_t flash         : 1;
        uint8_t hold        : 1;
        uint8_t redial    : 1;
        uint8_t key0          : 1;

        uint8_t key1          : 1;
        uint8_t key2          : 1;
        uint8_t key3          : 1;
        uint8_t reject        : 1;
        uint8_t button0       : 1;
        uint8_t button1       : 1;
        uint8_t button2       : 1;
        uint8_t button3       : 1;
    } report;
} TELEPHONY_HID_INPUT_REPORT;

typedef union
{
    uint8_t data[2];
    struct
    {
        uint8_t off_hook        : 1;
        uint8_t speaker         : 1;
        uint8_t mute            : 1;
        uint8_t ring            : 1;
        uint8_t hold            : 1;
        uint8_t mic             : 1;
        uint8_t online          : 1;
        uint8_t reserved        : 1;
    } report;
} TELEPHONY_HID_OUTPUT_REPORT;

typedef union
{
    uint8_t data[5];
    struct
    {
        uint8_t id;
        uint8_t press;
        uint8_t x;
        uint8_t y;
        uint8_t m;
    } pointer;
} MOUSE_HID_INPUT_REPORT;

typedef union
{
    uint8_t data[9];
    struct
    {
        uint8_t id;
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

typedef enum
{
    MMI_USB_NULL                        = 0x00,

    MMI_USB_AUDIO_VOL_UP                = 0x30,
    MMI_USB_AUDIO_VOL_DOWN              = 0x31,
    MMI_USB_AUDIO_PLAY_PAUSE            = 0x32,
    MMI_USB_AUDIO_STOP                  = 0x33,
    MMI_USB_AUDIO_FWD                   = 0x34,
    MMI_USB_AUDIO_BWD                   = 0x35,
    MMI_USB_AUDIO_FASTFORWARD           = 0x36,
    MMI_USB_AUDIO_REWIND                = 0x37,
    MMI_USB_AUDIO_FASTFORWARD_STOP      = 0x3B,
    MMI_USB_AUDIO_REWIND_STOP           = 0x3C,

    MMI_USB_MIC_VOL_UP                  = 0x15,
    MMI_USB_MIC_VOL_DOWN                = 0x16,
    MMI_USB_TEAMS_END_OUTGOING_CALL     = 0x02,
    MMI_USB_TEAMS_ANSWER_CALL           = 0x03,
    MMI_USB_TEAMS_REJECT_CALL           = 0x04,
    MMI_USB_TEAMS_END_ACTIVE_CALL       = 0x05,
    MMI_USB_TEAMS_CANCEL_VOICE_DIAL     = 0x0A,
    MMI_USB_TEAMS_SWITCH_TO_SECOND_CALL = 0x0C,
    MMI_USB_TEAMS_RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL = 0x11,
    MMI_USB_TEAMS_MIC_MUTE_UNMUTE       = 0x06,
    MMI_USB_TEAMS_HOLD_CALL             = 0xAB,

    MMI_USB_TEAMS_MIC_MUTE              = 0x07,
    MMI_USB_TEAMS_MIC_UNMUTE            = 0x08,
    MMI_USB_ENTER_PAIRING_MODE          = 0x6C,
    MMI_USB_TEAMS_BUTTON_SHORT_PRESS    = 0xA9,
    MMI_USB_TEAMS_BUTTON_LONG_PRESS     = 0xAA,

    MMI_USB_TOTAL
} T_MMI_USB_ACTION;

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

extern void *hid_intr_in_handle;
#if USB_HID_SEC_SUPPORT
extern void *hid_intr_in_handle_1;
#endif

void app_usb_mouse_handle_action(uint8_t action);
void app_usb_keyboard_handle_action(uint8_t action);
bool app_usb_hid_interrupt_pipe_send(void *data, uint16_t length);
bool app_usb_hid_interrupt_pipe_send_sec(void *data, uint16_t length);
bool app_usb_hid_interrupt_pipe_send_ep4(void *data, uint32_t length);
void app_usb_hid_open_pipe(void);
void app_usb_mmi_handle_action(uint8_t action);
void app_usb_hid_close_pipe(void);
void app_usb_hid_init(void);
void app_usb_hid_deinit(void);
bool app_usb_hid_get_report_data_is_ready(uint8_t *data, uint16_t length);
#endif
