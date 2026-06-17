/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if GPIO_KEY_EN
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "section.h"
#include "rtl876x_pinmux.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"
#include "app_usb_hid.h"
#include "app_io_msg.h"

/** @defgroup  GPIO_INT_DEMO  GPIO INTERRUPT DEMO
    * @brief  Gpio interrupt implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Gpio_Interrupt_Exported_Macros Gpio Interrupt Exported Macros
  * @brief
  * @{
  */
#if TARGET_RTL8773DO
#define KEY_HIGH_ACTIVE_EN  1
#else
#define KEY_HIGH_ACTIVE_EN  0
#endif

#define GPIO_DEMO_INPUT_PIN0                      P1_0

typedef enum
{
    KEY_CLICK_0           = 0,
    KEY_CLICK_1,
    KEY_CLICK_2,
    KEY_CLICK_3,
    KEY_CLICK_4,
} T_KEY_CNT_NUM;

/** @} */ /* End of group Gpio_Interrupt_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gpio_Interrupt_Exported_Functions Gpio Interrupt Exported Functions
  * @brief
  * @{
  */

void app_key_send_msg(uint32_t param)
{
    T_IO_MSG button_msg;

    button_msg.type = IO_MSG_TYPE_GPIO;
    button_msg.subtype = IO_MSG_GPIO_KEY;
    button_msg.u.param = param;

    APP_PRINT_TRACE1("app_key_send_msg param:0x%x", param);
    app_io_msg_send(&button_msg);
}

void app_key_gpio_press(void)
{
    static uint8_t s_key_cnt = KEY_CLICK_1;
    APP_PRINT_TRACE1("app_key_gpio_press, cnt:%d", s_key_cnt);

    switch (s_key_cnt)
    {
    case KEY_CLICK_1:
    case KEY_CLICK_2:
    case KEY_CLICK_3:
        {
            app_key_send_msg(s_key_cnt);
            s_key_cnt++;
        }
        break;
    case KEY_CLICK_4:
        {
            app_key_send_msg(s_key_cnt);
            s_key_cnt = KEY_CLICK_1;
        }
        break;
    default:
        s_key_cnt = KEY_CLICK_1;
        break;
    }
}

#if USB_HID_KEYBOARD_EN
void app_key_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    uint8_t key_num = 0;

    key_num = io_driver_msg_recv->u.param & 0xFF;
    APP_PRINT_TRACE1("app_key_handle_msg key_num:%d", key_num);
    extern void app_usb_keyboard_input_demo(void);
    app_usb_keyboard_input_demo();
}
#elif USB_HID_MOUSE_EN
void app_key_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    uint8_t key_num = 0;

    key_num = io_driver_msg_recv->u.param & 0xFF;
    APP_PRINT_TRACE1("app_key_handle_msg key_num:%d", key_num);
    if (key_num == KEY_CLICK_1)
    {
        app_usb_mouse_handle_action(MMI_MOUSE_UP);
    }
    else if (key_num == KEY_CLICK_2)
    {
        app_usb_mouse_handle_action(MMI_MOUSE_DWON);
    }
    else if (key_num == KEY_CLICK_3)
    {
        app_usb_mouse_handle_action(MMI_MOUSE_LEFT);
    }
    else if (key_num == KEY_CLICK_4)
    {
        app_usb_mouse_handle_action(MMI_MOUSE_RIGHT);
    }
}
#endif

ISR_TEXT_SECTION
static void gpio_isr_cb(uint32_t context)
{
    uint8_t pin_index = (uint32_t)context;
    T_GPIO_LEVEL gpio_level = hal_gpio_get_input_level(pin_index);

    IO_PRINT_INFO2("gpio_isr_cb: pin_name %s, gpio_level %d", TRACE_STRING(Pad_GetPinName(pin_index)),
                   gpio_level);

    if (gpio_level == GPIO_LEVEL_LOW)
    {
        hal_gpio_irq_change_polarity(pin_index, GPIO_IRQ_ACTIVE_HIGH);
    }
    else
    {
        hal_gpio_irq_change_polarity(pin_index, GPIO_IRQ_ACTIVE_LOW);
    }
#if KEY_HIGH_ACTIVE_EN
    if (gpio_level == GPIO_LEVEL_HIGH)
#else
    if (gpio_level == GPIO_LEVEL_LOW)
#endif
    {
        app_key_gpio_press();
    }
}

void key_init(void)
{
    hal_gpio_init();
    hal_gpio_int_init();
    hal_gpio_set_debounce_time(30);

#if KEY_HIGH_ACTIVE_EN
    hal_gpio_init_pin(GPIO_DEMO_INPUT_PIN0, GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_DOWN);
    hal_gpio_set_up_irq(GPIO_DEMO_INPUT_PIN0, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_HIGH, true);
    hal_gpio_register_isr_callback(GPIO_DEMO_INPUT_PIN0, gpio_isr_cb, GPIO_DEMO_INPUT_PIN0);
    hal_gpio_irq_enable(GPIO_DEMO_INPUT_PIN0);
#else
    hal_gpio_init_pin(GPIO_DEMO_INPUT_PIN0, GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_UP);
    hal_gpio_set_up_irq(GPIO_DEMO_INPUT_PIN0, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
    hal_gpio_register_isr_callback(GPIO_DEMO_INPUT_PIN0, gpio_isr_cb, GPIO_DEMO_INPUT_PIN0);
    hal_gpio_irq_enable(GPIO_DEMO_INPUT_PIN0);
#endif
}

#endif
