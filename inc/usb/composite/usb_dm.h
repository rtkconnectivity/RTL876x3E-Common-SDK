/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_DM__
#define __USB_DM__
#include <stdbool.h>
#include <stdint.h>

/** \addtogroup USB_DM
  * \brief USB device manager.
  * @{
  */

/** @defgroup USB_Dev_Manager_Exported_Constants USB Device Manager Exported Constants
  * @{
  */

/**
 * \brief   The definition of USB speed.
 *
 */
#define USB_SPEED_FULL  0
/**
 * \brief   The definition of USB speed.
 *
 */
#define USB_SPEED_HIGH  1

/**
 * \brief   USB device manager event used in \ref USB_DM_CB.
 *
 */
typedef enum
{
    USB_DM_EVT_STATUS_IND                   = 0,
    USB_DM_EVT_BC12_DETECT                  = 1,
    USB_DM_EVT_EP_IN_NAK                    = 2,
    USB_DM_EVT_SETUP_PEEK                   = 3,
} T_USB_DM_EVT;

typedef union
{
    uint16_t d16;

    struct
    {
        uint16_t status_ind: 1;
        uint16_t bc12_det: 1;
        uint16_t in_nak: 1;
        uint16_t setup_peek: 1;
        uint16_t rsv: 12;
    } b;

} T_USB_DM_EVT_MSK;

/**
 * \brief   The USB power state obtained from event \ref T_USB_DM_EVT.
 *
 */
typedef enum  {USB_PDN = 0,
               USB_ATTACHED,
               USB_POWERED,
               USB_DEFAULT,
               USB_ADDRESSED,
               USB_CONFIGURED,
               USB_SUSPENDED,
               USB_RESUMING,
              } T_USB_POWER_STATE;

/** End of group USB_Dev_Manager_Exported_Constants
  * @}
  */

/** @defgroup USB_Dev_Manager_Exported_Types USB Device Manager Exported Types
  * @{
  */

/**
 * \brief USB device manager event parameter status indication.
 * \param state: The current state.
 * \param info: Specific information depends on \ref state.
 *              If state is \ref USB_DEFAULT, it indicates the actual enumeration speed.
 *
 */
typedef struct _usb_dm_evt_param_status_ind
{
    T_USB_POWER_STATE state;
    union
    {
        uint8_t speed;
    } info;
} T_USB_DM_EVT_PARAM_STATUS_IND;

/**
 * \brief The event parameter of bc12 detect.
 * \param type: The bc12 detect result.
 */
typedef struct _usb_dm_evt_param_bc12_det
{
    uint8_t type;
} T_USB_DM_EVT_PARAM_BC12_DET;

/**
 * \brief USB dm event parameter in endpoint nak.
 * \param addr: The endpoint address.
 */
typedef struct _usb_dm_evt_param_ep_in_nak
{
    uint8_t ep_num;
} T_USB_DM_EVT_PARAM_EP_IN_NAK;

/**
 * \brief USB dm event parameter setup packet.
 * \param pkt: The setup packet.
 */
typedef struct _usb_dm_evt_param_setup_peek
{
    uint8_t *pkt;
} T_USB_DM_EVT_PARAM_SETUP_PEEK;

/**
 * \brief The event parameters.
 *
 */
typedef union _usb_dm_evt_param
{
    T_USB_DM_EVT_PARAM_STATUS_IND status_ind;
    T_USB_DM_EVT_PARAM_BC12_DET bc12_det;
    T_USB_DM_EVT_PARAM_EP_IN_NAK ep_in_nak;
    T_USB_DM_EVT_PARAM_SETUP_PEEK setup_peek;
} T_USB_DM_EVT_PARAM;

/**
 * \brief  The USB device manager callback.
 *
 * \param T_USB_DM_EVT USB device manager event defined in \ref T_USB_DM_EVT.
 *
 * \param uint32_t Optional parameters depending on different event.
 *
 */
typedef bool (*USB_DM_CB)(T_USB_DM_EVT, T_USB_DM_EVT_PARAM *);

/**
 * \brief   USB settings such as speed, etc.
 *
 */
typedef struct _t_usb_core_config
{
    uint8_t speed;
    struct
    {
        uint8_t uac_enable: 1;
        uint8_t hid_enable: 1;
        uint8_t rsv: 6;
    } class_set;
} T_USB_CORE_CONFIG;

/** End of group USB_Dev_Manager_Exported_Types
  * @}
  */

/** @defgroup USB_Dev_Manager_Exported_Functions USB Device Manager Exported Functions
  * @{
  */

/**
 * \brief  Initialize the USB core.
 *
 * \param config USB core settings in \ref T_USB_CORE_CONFIG.
 *
 */
void usb_dm_core_init(T_USB_CORE_CONFIG config);

/**
 * \brief  Start the USB core, this api will start the USB task.
 *
 * \param bc12_detect Do the bc12 detect if it is true.
 *
 */
void usb_dm_start(bool bc12_detect);

/**
 * \brief  Stop the USB core, this api will stop the USB task.
 *
 */
void usb_dm_stop(void);

/**
 * \brief   Register USB dm callback.
 *
 * \param evt_msk USB dm event mask \ref T_USB_DM_EVT_MSK.
 * \param cb USB dm callback \ref USB_DM_CB.
 *
 */
void usb_dm_cb_register(T_USB_DM_EVT_MSK evt_msk, USB_DM_CB cb);

/**
 * \brief   Change event mask.
 *
 * \param cb USB dm callback \ref USB_DM_CB.
 * \param evt_msk USB dm event mask \ref T_USB_DM_EVT_MSK.
 *
 */
void usb_dm_evt_msk_change(T_USB_DM_EVT_MSK evt_msk, USB_DM_CB cb);

/**
 * \brief   Unregister USB dm callback.
 *
 * \param cb USB dm callback \ref USB_DM_CB.
 */
void usb_dm_cb_unregister(USB_DM_CB cb);

/** @} */ /* End of group USB_Dev_Manager_Exported_Functions */
/** @}*/
/** End of USB_DM
*/
#endif
