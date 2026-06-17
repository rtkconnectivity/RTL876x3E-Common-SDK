/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _USB_AUDIO_DRIVER_H_
#define _USB_AUDIO_DRIVER_H_
#include <stdbool.h>
#include <stdint.h>

/**
 * \addtogroup USB_Audio_Driver
 * \brief The module primarily offers the parts needed to put USB audio class into practice. In addition
 * to supporting multiple function instances, such as two speakers operating simultaneously,
 * the driver supports UAC 1.0 and UAC 2.0. Additionally, the driver allows for several alternative
 * settings for each interface.
 * @{
  */

/**
 * @defgroup USB_AUDIO_DRIVER_USAGE How to Implement a USB Audio Interface
 * @{
 *
 * \brief This section provides a comprehensive guide on implementing a USB audio interface,
 *        complete with sample code for your reference.
 *
 * \section USB_AUDIO_DRIVER_ALLOCATE_INSTANCE Allocate Instance
 *
 * Allocate an instance of the function using \ref usb_audio_driver_inst_alloc, and the input version
 * is \ref USB_AUDIO_VERSION_1 and \ref USB_AUDIO_VERSION_2, which are the UAC versions.
 *
 * \par Example
 * \code
 *      void *demo_instance = usb_audio_driver_inst_alloc(USB_AUDIO_VERSION_1, 2, 2);
 * \endcode
 *
 * \section USB_AUDIO_DRIVER_AUDIO_CONTROL_INTERFACE Audio Control Interface
 * Implement control interface as follows:
 *    - Implement the descriptor arrays.
 *    - Implement the control-related processes. \n
 *      The main structure to handle individual controls, including volume control, is
 *      \ref T_USB_AUDIO_DRIVER_CTRL. Multiple controls can be included in a single control
 *      unit. For instance, a single feature unit may have volume and mute controls at the
 *      same time. Thus, one control entity \ref T_USB_AUDIO_DRIVER_CTRL_ENTITY consists of
 *      no less than one \ref T_USB_AUDIO_DRIVER_CTRL. The overall control architecture
 *      specified in the descriptor arrays is composed of all control entities.
 *
 * \par Example
 * \code
 *      #include "usb_spec20.h"
 *      #include "usb_audio1_spec.h"
 *      #include "usb_audio1_ctrl.h"
 *
 *      T_USB_INTERFACE_DESC demo_ac_desc =
 *      {
 *          ...init...
 *      };
 *      T_UAC1_AC_HDR_DESC demo_ac_hdr_desc =
 *      {
 *          ...init...
 *      };
 *      T_UAC1_IT_DESC demo_input_terminal_desc =
 *      {
 *          ...init...
 *      };
 *      T_UAC1_OT_DESC demo_output_terminal_desc =
 *      {
 *          ...init...
 *      };
 *      T_UAC1_FEATURE_UNIT_DESC(UNIT_ID, 3) demo_feature_unit_desc =
 *      {
 *          ..init...
 *          .bUnitID = UNIT_ID,
 *          .bmaControls[0] = UAC_CONTROL_BIT(UAC1_FU_MUTE_CONTROL) | UAC_CONTROL_BIT(UAC1_FU_VOLUME_CONTROL),
 *          ...init...
 *      }
 *
 *      T_USB_DESC_HDR *demo_ctrl_descs_fs[] =
 *      {
 *          (T_USB_DESC_HDR*)&demo_ac_desc,
 *          (T_USB_DESC_HDR*)&demo_ac_hdr_desc,
 *          (T_USB_DESC_HDR*)&demo_input_terminal_desc,
 *          (T_USB_DESC_HDR*)&demo_output_terminal_desc,
 *          (T_USB_DESC_HDR*)&demo_feature_unit_desc,
 *          NULL
 *      };
 *
 *      ...
 *      T_USB_DESC_HDR *demo_ctrl_descs_hs[] =
 *      {
 *          ...
 *          NULL
 *      };
 *
 *
 *      T_CTRL_ATTR demo_vol_attr_spk =
 *      {
 *          ...init...
 *      };
 *
 *      static T_USB_AUDIO_DRIVER_CTRL_ATTR uac1_drv_ctrl_attr = {.data = NULL, .len = 0};
 *      T_USB_AUDIO_DRIVER_CTRL_ATTR *demo_vol_attr_get_spk(T_USB_AUDIO_DRIVER_CTRL_ATTR *drv_attr,
 *                                                     uint8_t cmd)
 *      {
 *          T_CTRL_ATTR *vol_attr = (T_CTRL_ATTR *)drv_attr->data;
 *          int32_t data = ((int32_t *)(vol_attr->data))[cmd - 1];
 *          uac1_drv_ctrl_attr.data = &data;
 *          uac1_drv_ctrl_attr.len = sizeof(data);
 *          return &uac1_drv_ctrl_attr;
 *      }
 *
 *      int32_t demo_vol_attr_set_spk(T_USB_AUDIO_DRIVER_CTRL_ATTR *drv_attr, uint8_t cmd, int value)
 *      {
 *          //Process
 *          return 0;
 *      }
 *      T_USB_AUDIO_DRIVER_CTRL demo_vol_ctrl_spk =
 *      {
 *          .type = UAC1_FU_VOLUME_CONTROL,
 *          .attr = {.data = (void *) &demo_vol_attr_spk, .len = sizeof(demo_vol_attr_spk)},
 *          .get = (T_USB_AUDIO_CTRL_GET_FUNC)demo_vol_attr_get_spk,
 *          .set = (T_USB_AUDIO_CTRL_SET_FUNC)demo_vol_attr_set_spk,
 *      };
 *
 *      T_CTRL_ATTR demo_mute_attr_spk =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_AUDIO_DRIVER_CTRL_ATTR *demo_mute_attr_get_spk(T_USB_AUDIO_DRIVER_CTRL_ATTR *drv_attr,
 *                                                     uint8_t cmd)
 *      {
 *          T_CTRL_ATTR *mute_attr = (T_CTRL_ATTR *)drv_attr->data;
 *          int32_t data = mute_attr->attr.cur;
 *          uac1_drv_ctrl_attr.data = &data;
 *          uac1_drv_ctrl_attr.len = sizeof(data);
 *          return &uac1_drv_ctrl_attr;
 *      }
 *
 *      int32_t demo_mute_attr_set_spk(T_USB_AUDIO_DRIVER_CTRL_ATTR *drv_attr, uint8_t cmd, int value)
 *      {
 *          //Process
 *          return 0;
 *      }
 *      T_USB_AUDIO_DRIVER_CTRL demo_mute_ctrl_spk =
 *      {
 *          .type = UAC1_FU_VOLUME_CONTROL,
 *          .attr = {.data = (void *) &demo_mute_attr_spk, .len = sizeof(demo_mute_attr_spk)},
 *          .get = (T_USB_AUDIO_CTRL_GET_FUNC)demo_mute_attr_get_spk,
 *          .set = (T_USB_AUDIO_CTRL_SET_FUNC)demo_mute_attr_set_spk,
 *      };
 *
 *      T_USB_AUDIO_DRIVER_CTRL_ENTITY(UNIT_ID, 3) demo_ctrl_entity =
 *      {
 *          .entity_id = UNIT_ID,
 *          .ctrls[0] = &demo_vol_ctrl_spk,
 *          .ctrls[1] = &demo_mute_ctrl_spk,
 *          .ctrls[2] = NULL,
 *      };
 *
 *      T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR *demo_all_ctrls[] =
 *      {
 *          &demo_ctrl_entity,
 *          NULL,
 *      };
 *
 *      usb_audio_driver_desc_register(demo_instance, demo_ctrl_descs_fs, demo_ctrl_descs_hs);
 *      usb_audio_driver_ctrl_register(demo_instance, (T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR **)demo_all_ctrls);
 * \endcode
 *
 * \section USB_AUDIO_DRIVER_AUDIO_STREAMING_INTERFACE Audio Streaming Interface
 * Implement audio streaming interface as follows:
 *   - Implement descriptor arrays of the interfaces.
 *   - Initialize streaming attributes.
 *   - Register callbacks to process audio data.
 *
 * \par Example
 * \code
 *      #include "usb_spec20.h"
 *      #include "usb_audio1_spec.h"
 *
 *      T_USB_INTERFACE_DESC demo_if_alt0_desc =
 *      {
 *          ...init...
 *      };
 *      T_USB_INTERFACE_DESC demo_if_alt1_desc =
 *      {
 *          ...init...
 *      };
 *      T_UAC1_AS_HDR_DESC demo_as_hdr_desc =
 *      {
 *          ...init...
 *      };
 *      T_UAC1_FMT_TYPE_I_DESC(spk, sample rate num) demo_fmt_desc =
 *      {
 *          ...init...
 *      };
 *      T_UAC1_STD_ISO_EP_DESC demo_std_iso_out_ep_desc =
 *      {
 *          ...init...
 *      };
 *      T_UAC1_ISO_EP_DESC demo_cs_iso_out_ep_desc =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_DESC_HDR* demo_as_descs_spk_fs[] =
 *      {
 *          (T_USB_DESC_HDR*)&demo_if_alt0_desc,
 *          (T_USB_DESC_HDR*)&demo_if_alt1_desc,
 *          (T_USB_DESC_HDR*)&demo_as_hdr_desc,
 *          (T_USB_DESC_HDR*)&demo_fmt_desc,
 *          (T_USB_DESC_HDR*)&demo_std_iso_out_ep_desc,
 *          (T_USB_DESC_HDR*)&demo_cs_iso_out_ep_desc,
 *          NULL,
 *      }
 *
 *      ...
 *      T_USB_DESC_HDR* demo_as_descs_spk_hs[] =
 *      {
 *          ...
 *          NULL,
 *      }
 *
 *      usb_audio_driver_desc_register(demo_instance, demo_as_descs_spk_fs, demo_as_descs_spk_hs);
 *
 *      T_USB_AUDIO_DRIVER_ATTR demo_attr_alt0 =
 *      {
 *          .dir = USB_AUDIO_DIR_OUT,
 *          .chann_num = 0,
 *          .bit_width = 0,
 *          .cur_sample_rate = 0,
 *          .max_sample_rate = 0,
 *      };
 *      usb_audio_driver_attr_init(demo_instance, 0, demo_attr_alt0);
 *
 *      T_USB_AUDIO_DRIVER_ATTR demo_attr_alt1 =
 *      {
 *          .dir = USB_AUDIO_DIR_OUT,
 *          .chann_num = same as bNrChannels in demo_fmt_desc,
 *          .bit_width = same as bBitResolution in demo_fmt_desc,
 *          .cur_sample_rate = default sample rate,
 *          .max_sample_rate = max sample rate in demo_fmt_desc,
 *      };
 *      usb_audio_driver_attr_init(demo_instance, 1, demo_attr_alt1);
 *
 *      int demo_usb_audio_activate(uint8_t dir, uint8_t bit_res, uint32_t sample_rate, uint8_t chan_num)
 *      {
 *          //Process
 *      }
 *      int demo_usb_audio_deactivate(uint8_t dir)
 *      {
 *          //Process
 *      }
 *      int demo_usb_audio_downstream(uint8_t *buf, uint16_t len)
 *      {
 *          //Process the downstream buffer data, where 'len' is the data length.
 *      }
 *      T_USB_AUDIO_DRIVER_CBS demo_cbs =
 *      {
 *          .activate = demo_usb_audio_activate,
 *          .deactivate = demo_usb_audio_deactivate,
 *          .upstream = NULL,
 *          .downstream = demo_usb_audio_downstream,
 *      }
 *      usb_audio_driver_cb_register(demo_instance, &demo_cbs);
 * \endcode
 *
 * \section USB_AUDIO_DRIVER_INITIALIZE_AUDIO_DRIVER Initialize Audio Driver
 * Call `usb_audio_driver_init` to initialize the USB audio driver.
 */
/** @}*/

/** @defgroup USB_Audio_Driver_Exported_Constants USB Audio Driver Exported Constants
  * @{
  */

/**
 * \brief  The version of USB audio class.
 *
 */
#define USB_AUDIO_VERSION_1         0x01
/**
 * \brief  The version of USB audio class.
 *
 */
#define USB_AUDIO_VERSION_2         0x02

/**
 * \brief  The maximum tolerance of the actual sample rate.
 *
 * \ingroup USB_CORE
 */
#define PERCENT_SAMPLE_RATE_TOLERANCE   (10)

/**
 * \brief  Convert the feedback value to 10.14 format as defined in chapter 5.12.4.2 of the
 *         specification `usb_20.pdf`.
 *
 * \ingroup USB_CORE
 */
#define TO_FB_FORMAT_10_14(f)    (((f/1000) << 14) | ((f%1000) << 4))

/**
 * \brief  Convert the feedback value to 16.16 format as defined in chapter 5.12.4.2 of the
 *         specification `usb_20.pdf`.
 *
 * \ingroup USB_CORE
 */
#define TO_FB_FORMAT_16_16(f)    (((f/1000) << 13) | ((f%1000) << 3))

/**
 * \brief   The USB audio stream direction.
 *
 */
typedef enum
{
    USB_AUDIO_DIR_OUT = 1,
    USB_AUDIO_DIR_IN = 2,
} T_USB_AUDIO_DIR;

/**
 * \brief USB audio driver control for each control entity.
 *
 * \param entity_id: The id of entity defined in the control interface,
 *                   it \b must be \b equal \b to bTerminalID of the corresponding unit descriptor.
 * \param ctrls: Control functions of the given control selector.
 */
#define T_USB_AUDIO_DRIVER_CTRL_ENTITY(id, num)     \
    struct _usb_audio_driver_ctrl_entity##id            \
    {                                                   \
        uint8_t     entity_id;                          \
        uint8_t     ctrl_num;                           \
        T_USB_AUDIO_DRIVER_CTRL *ctrls[num];                \
    }

/** End of group USB_Audio_Driver_Exported_Constants
  * @}
  */

/** @defgroup USB_Audio_Driver_Exported_Types USB Audio Driver Exported Types
  * @{
  */

/**
 * \brief   The USB audio interrupt pipe.
 *
 */
typedef void   *USB_AUDIO_DRIVER_INTR_PIPE;

/**
 * \brief   The callback defined in \ref T_USB_AUDIO_DRIVER_CBS.
 *
 */
typedef  int (*USB_AUDIO_DRV_CB_ACTIVATE)(uint8_t dir, uint8_t bit_res, uint32_t sample_rate,
                                          uint8_t chan_num);
/**
* \brief   The callback defined in \ref T_USB_AUDIO_DRIVER_CBS.
*
*/
typedef  int (*USB_AUDIO_DRV_CB_DEACTIVATE)(uint8_t dir);
/**
 * \brief   The callback defined in \ref T_USB_AUDIO_DRIVER_CBS.
 *
 */
typedef  int (*USB_AUDIO_DRV_CB_XMIT)(uint8_t *buf, uint16_t len);
/**
 * \brief USB audio driver callbacks to transfer necessary information or audio data to the upper
 *        software layers.
 *
 * \param activate: USB audio function is ready to transmit audio data. The parameter \p dir is
 *                  defined in \ref T_USB_AUDIO_DIR; \p bit_res, \p sample_rate, and \p chan_num
 *                  together describe the attributes of the audio data.
 * \param deactivate: USB audio function terminates the audio data transfer. The parameter \p dir
 *                    is defined in \ref T_USB_AUDIO_DIR.
 * \param upstream: Transmit data from device to host. The parameter \p buf is the audio data that will be sent,
 *                      and \p len is the length of the \p buf.
 * \param downstream: Transmit data from host to device. The parameter \p buf is the audio data that has been
 *                    received, and \p len is the length of the \p buf.
 * \param feedback_d: Transmit feedback value of downstream from device to host. The parameter \p buf is the feedback
 *                    data that will be sent, and \p len is the length of the \p buf.
 */
typedef struct _usb_audio_driver_cbs
{
    USB_AUDIO_DRV_CB_ACTIVATE activate;
    USB_AUDIO_DRV_CB_DEACTIVATE deactivate;
    USB_AUDIO_DRV_CB_XMIT upstream;
    USB_AUDIO_DRV_CB_XMIT downstream;
    USB_AUDIO_DRV_CB_XMIT feedback_d;
} T_USB_AUDIO_DRIVER_CBS;

/**
 * \brief The control attributes of USB audio driver.
 *
 * \param data: The contents of attribute. For example, USB audio 1.0 attribute can be defined as int
 *              attribute[5], the array is indexed by xxx_CUR/xxx_MIN/xxx_MAX/xxx_RES/xxx_MEM as
 *              defined in chapter 5 of https://www.usb.org/document-library/audio-device-document-10.
 *              The attributes for USB audio 2.0 is defined in chaptor 5.2.3 in
 *              https://www.usb.org/document-library/audio-devices-rev-20-and-adopters-agreement.
 * \param len: The length of data.
 *
 */
typedef struct _usb_audio_driver_ctrl_attr
{
    void *data;
    uint32_t len;
} T_USB_AUDIO_DRIVER_CTRL_ATTR;
/**
 * \brief USB audio callbacks to process control selectors' commands.
 *
 */
typedef T_USB_AUDIO_DRIVER_CTRL_ATTR *(*T_USB_AUDIO_CTRL_GET_FUNC)(T_USB_AUDIO_DRIVER_CTRL_ATTR
                                                                   *attr, uint8_t cmd);
/**
 * \brief USB audio callbacks to process control selectors' commands.
 *
 */
typedef int (*T_USB_AUDIO_CTRL_SET_FUNC)(T_USB_AUDIO_DRIVER_CTRL_ATTR *attr, uint8_t cmd,
                                         int value);
/**
 * \brief Implement the procession of control selectors.
 *
 * \param type: The control type. For USB audio 1.0, refer to A.10 of
 *              https://www.usb.org/document-library/audio-device-document-10.
 *              For USB audio 2.0, refer to A.17 of
 *              https://www.usb.org/document-library/audio-devices-rev-20-and-adopters-agreement.
 * \param attr: The attributes defined in \ref T_USB_AUDIO_DRIVER_CTRL_ATTR.
 * \param set: It is used to handle SET_XXX requests, such as SET_CUR and so on.
 * \param get: It is used to handle GET_XXX requests, such as GET_CUR and so on.
 *
 */
typedef struct _usb_audio_driver_ctrl
{
    uint8_t type;
    T_USB_AUDIO_DRIVER_CTRL_ATTR attr;
    T_USB_AUDIO_CTRL_SET_FUNC set;
    T_USB_AUDIO_CTRL_GET_FUNC get;
} T_USB_AUDIO_DRIVER_CTRL;

/**
 * \brief The common header of the controls defined in \ref T_USB_AUDIO_DRIVER_CTRL_ENTITY.
 *
 */
typedef struct _usb_audio_driver_ctrl_entity_hdr
{
    uint8_t     entity_id;
    uint8_t     ctrl_num;
} T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR;
/**
 * \brief The USB audio stream attributes.
 *
 * \param dir: Refer to \ref T_USB_AUDIO_DIR.
 * \param chann_num: The channel number of the given alternate setting.
 * \param bit_width: The bit width of the given alternate setting.
 * \param max_sample_rate: The maximum sample rate of the given alternate setting.
 */
typedef struct _usb_audio_driver_attr
{
    T_USB_AUDIO_DIR dir;
    uint8_t         chann_num;
    uint8_t         bit_width;
    uint32_t        cur_sample_rate;
    uint32_t        max_sample_rate;
} T_USB_AUDIO_DRIVER_ATTR;
/**
 * \brief  The common header of all USB descriptors.
 * \param bLength: The length of the descriptor.
 * \param bDescriptorType: The descriptor type of the descriptor.
 *
 */
typedef struct _usb_audio_driver_desc_hdr
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
} __attribute__((packed)) T_USB_AUDIO_DRIVER_DESC_HDR;

/** End of group USB_Audio_Driver_Exported_Types
  * @}
  */

/** @defgroup USB_Audio_Driver_Exported_Functions USB Audio Driver Exported Functions
  * @{
  */

/**
 * \brief Allocate the audio function instance, which possesses independent audio
 *        control/streaming capabilities.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param version \ref USB_AUDIO_VERSION_1 and \ref USB_AUDIO_VERSION_2.
 * \param proc_interval_out The interval for downstream interrupt handling.
 * \param proc_interval_in The interval for upstream interrupt handling.
 * \return The audio function instance.
 *
 * \par Example
 * Please refer to \ref USB_AUDIO_DRIVER_USAGE.
 */
void *usb_audio_driver_inst_alloc(uint8_t version, uint8_t proc_interval_out,
                                  uint8_t proc_interval_in);

/**
 * \brief Free the audio function instance alloacted by \ref usb_audio_driver_inst_alloc.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param inst The instance alloacted by \ref usb_audio_driver_inst_alloc.
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_AUDIO_DRIVER_USAGE.
 */
int usb_audio_driver_inst_free(void *inst);
/**
 * \brief Register interface descriptors to the driver.
 *
 * \param inst The audio instance returned by \ref usb_audio_driver_inst_alloc.
 * \param desc_fs The interface descriptors of full speed.
 * \param desc_hs The interface descriptors of high speed.
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \b Audio \b Control \b Interface & \b Audio \b Streaming \b Interface in \ref USB_AUDIO_DRIVER_USAGE.
 *
 */
int usb_audio_driver_desc_register(void *inst, T_USB_AUDIO_DRIVER_DESC_HDR **descs_fs,
                                   T_USB_AUDIO_DRIVER_DESC_HDR **descs_hs);
/**
 * \brief Register USB audio callbacks.
 *
 * \param inst The audio instance returned by \ref usb_audio_driver_inst_alloc.
 * \param cbs Refer to \ref T_USB_AUDIO_DRIVER_CBS.
 *
 * \par Example
 * Please refer to \b Audio \b Streaming \b Interface in \ref USB_AUDIO_DRIVER_USAGE.
 */
int usb_audio_driver_cb_register(void *inst, T_USB_AUDIO_DRIVER_CBS *cbs);
/**
 * \brief Register the USB audio control process.
 *
 * \param inst The audio instance returned by \ref usb_audio_driver_inst_alloc.
 * \param ctrl  Refer to \ref T_USB_AUDIO_DRIVER_CTRL_ENTITY.
 *
 * \par Example
 * Please refer to \b Audio \b Control \b Interface in \ref USB_AUDIO_DRIVER_USAGE.
 *
 */
int usb_audio_driver_ctrl_register(void *inst, T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR *ctrl[]);
/**
 * \brief Unregister interface descriptors.
 *
 * \param inst The audio instance returned by \ref usb_audio_driver_inst_alloc.
 *
 */
int usb_audio_driver_desc_unregister(void *inst);
/**
 * \brief Unregister interface callbacks.
 *
 * \param inst The audio instance returned by \ref usb_audio_driver_inst_alloc.
 *
 */
int usb_audio_driver_cb_unregister(void *inst);
/**
 * \brief Unregister interface control process.
 *
 * \param inst The audio instance returned by \ref usb_audio_driver_inst_alloc.
 *
 */
int usb_audio_driver_ctrl_unregister(void *inst);
/**
 * \brief The USB audio interface frequency that is set to the USB audio driver. It is only used in
 *        USB audio 2.0, because its sample freqency control is implemented in the control interface.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param inst The audio instance returned by \ref usb_audio_driver_inst_alloc.
 * \param dir Refer to \ref T_USB_AUDIO_DIR.
 * \param freq The sample frequency.
 *
 */
int usb_audio_driver_freq_change(void *inst, T_USB_AUDIO_DIR dir, uint32_t freq);
/**
 * \brief Initialize the USB audio attributes for the given alternate setting.
 *
 * \param inst The audio instance returned by \ref usb_audio_driver_inst_alloc.
 * \param alt_num The alternate setting defined in bAlternateSetting of interface descriptor.
 * \param attr Refer to \ref T_USB_AUDIO_DRIVER_ATTR.
 *
 * \par Example
 * Please refer to \b Audio \b Streaming \b Interface in \ref USB_AUDIO_DRIVER_USAGE.
 *
 */
int usb_audio_driver_attr_init(void *inst, uint8_t alt_num, T_USB_AUDIO_DRIVER_ATTR attr);

/**
 * \brief Open the interrupt endpoint data message pipe of the optional audio control.
 *
 * \param inst The audio instance returned by \ref usb_audio_driver_inst_alloc.
 * \return Refer to \ref USB_AUDIO_DRIVER_INTR_PIPE.
 */
USB_AUDIO_DRIVER_INTR_PIPE usb_audio_driver_intr_msg_pipe_open(void *inst);

/**
 * \brief Send interrupt data message.
 *
 * \param handle The return value of \ref usb_audio_driver_intr_msg_pipe_open.
 * \param data The data messages to send.
 * \param len The length of \ref data.
 * \return Refer to `errno.h`.
 */
int usb_audio_driver_intr_msg_pipe_send(USB_AUDIO_DRIVER_INTR_PIPE handle, uint8_t *data,
                                        uint8_t len);

/**
 * \brief Close the interrupt endpoint data message pipe of the optional audio control.
 *
 * \param handle The return value of \ref usb_audio_driver_intr_msg_pipe_open.
 * \return Refer to `errno.h`.
 */
int usb_audio_driver_intr_msg_pipe_close(USB_AUDIO_DRIVER_INTR_PIPE handle);

/** @} */ /* End of group USB_Audio_Driver_Exported_Functions */
/** @}*/
#endif
