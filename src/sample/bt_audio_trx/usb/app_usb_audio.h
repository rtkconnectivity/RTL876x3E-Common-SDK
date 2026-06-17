/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_USB_AUDIO_H_
#define _APP_USB_AUDIO_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_track.h"
#include "usb_audio.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_USB_AUDIO APP USB Audio
  * @brief usb audio stream user application-playback/capture.
  * @{
  */

/**
 * app_usb_audio.h
 *
 * \brief   USB Audio scenario definition
 *
 * \ingroup APP_USB_AUDIO
 */
#define USB_AUDIO_SCENARIO_PLAYBACK  (1<<0)
#define USB_AUDIO_SCENARIO_CAPTURE   (1<<1)
#define USB_AUDIO_SCENARIO_ALL       (USB_AUDIO_SCENARIO_PLAYBACK|USB_AUDIO_SCENARIO_CAPTURE)

/**
* app_usb_audio.h
*
* \brief   notify stream out
*
* \param[in] label the pipe of @ref T_USB_AUDIO_STREAM
*
* \ingroup UAW
*/
void usb_audio_stream_data_trans_msg(uint32_t label);
/**
 * app_usb_audio.h
 *
 * \brief   notify stream active
 *
 * \param[in] label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] attr usb audio pipe attribute @ref T_USB_AUDIO_PIPE_ATTR
 *
 * \return true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_active(uint32_t label, T_USB_AUDIO_PIPE_ATTR attr);

/**
 * app_usb_audio.h
 *
 * \brief   notify stream deactive
 *
 * label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] dir downstrea or upstream
 *
 * \return true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_deactive(uint32_t label, uint8_t dir);

/**
 * app_usb_audio.h
 *
 * \brief   donwn stream data
 *
 * \param[in] buf data buffer
 * \param[in] len length of data to write
 * \param[in] label the pipe of @ref T_USB_AUDIO_STREAM
 *
 * \return true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_data_xmit_out(uint8_t *data, uint16_t length, uint32_t label);

/**
 * app_usb_audio.h
 *
 * \brief   donwn stream data msg in app task
 *
 * \param[in] label the pipe of @ref T_USB_AUDIO_STREAM
 *
 * \return true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_data_xmit_out_handle(uint32_t label);

/**
 * app_usb_audio.h
 *
 * \brief   write usb audio stream data
 *          is called
 *
 * \param[in] buf data buffer
 * \param[in] len length of data to write
 *
 * \return actual length of written data
 *
 * \ingroup UAW
 */
bool app_usb_audio_us_data_write(uint8_t *data, uint16_t length);

/**
 * app_usb_audio.h
 *
 * \brief   get usb audio upstream remaining pool size
 *
 * \return remaining pool size
 *
 * \ingroup UAW
 */
uint16_t app_usb_audio_us_get_remaining_pool_size(void);

/**
 * app_usb_audio.h
 *
 * \brief   get usb audio upstream data size
 *
 * \return data size
 *
 * \ingroup UAW
 */
uint16_t app_usb_audio_us_get_data_len(void);

/**
 * app_usb_audio.h
 *
 * \brief   clear up-streaming buffer
 *
 * \ingroup UAW
 */
void app_usb_audio_us_clear_up(void);

/**
 * app_usb_audio.h
 *
 * \brief   usb audio upstream pool size is changing
 *
 * \param[in] data_space used data size
 * \param[in] free_space remaining pool size
 * \param[in] byte_out usb transfer data size
 *
 * \ingroup UAW
 */
void app_usb_audio_us_size_change(uint16_t data_space, uint16_t free_space,
                                  uint16_t byte_out);

/**
 * app_usb_audio.h
 *
 * \brief   upstream data is empty handle
 *
 * \return  true/false
 *
 * \ingroup UAW
 */
bool app_usb_audio_us_data_empty(void);

/**
 * app_usb_audio.h
 *
 * \brief   usb audio set speaker volume
 *
 * \param[in] label label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] vol volume value
 *
 * \ingroup UAW
 */
bool app_usb_audio_set_spk_vol(uint32_t label, uint16_t vol);

/**
 * app_usb_audio.h
 *
 * \brief   usb audio set speaker mute
 *
 * \param[in] label label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] is_mute value
 *
 * \ingroup UAW
 */
bool app_usb_audio_set_spk_mute(uint32_t label, bool is_mute);

/**
 * app_usb_audio.h
 *
 * \brief   usb audio set mic volume
 *
 * \param[in] label label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] vol volume value
 *
 * \ingroup UAW
 */
bool app_usb_audio_set_mic_vol(uint32_t label, uint16_t vol);

/**
 * app_usb_audio.h
 *
 * \brief   usb audio set mic mute
 *
 * \param[in] label label the pipe of @ref T_USB_AUDIO_STREAM
 * \param[in] mute value
 *
 * \ingroup UAW
 */
bool app_usb_audio_set_mic_mute(uint32_t label, bool is_mute);

/**
 * app_usb_audio.h
 *
 * \brief   init app usb audio
 *
 * \ingroup APP_USB_AUDIO
 */
void app_usb_audio_init(void);
/** @}*/
/** End of APP_USB_AUDIO
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_USB_AUDIO_H_ */
