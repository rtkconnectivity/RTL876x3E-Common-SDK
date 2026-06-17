/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_BT_AUDIO_LINK_H_
#define _APP_BT_AUDIO_LINK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_AUDIO_LINK BT Audio APP Link
  * @brief BT Audio APP Link
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup APP_BT_AUDIO_LINK_Exported_Macros BT Audio Link Macros
  * @brief
  * @{
  */

/** @brief  Define links number. range: 0-4 */
#define APP_BT_AUDIO_MAX_BR_LINK_NUM                 4

/** @brief  The maximum volume level */
#define  A2DP_VOLUME_MAX_LEVEL        15

/** @brief  The minimum volume level */
#define  A2DP_VOLUME_MIN_LEVEL        0

/** End of APP_BT_AUDIO_LINK_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup APP_BT_AUDIO_LINK_Exported_Types BT Audio Link Types
  * @brief
  * @{
  */

/**  @brief  APP's Bluetooth BR/EDR link connection database */
typedef struct
{
    uint8_t             bd_addr[6];
    bool                used;
    uint8_t             id;
    void               *a2dp_track_handle;
    void               *sco_track_handle;
    bool                is_streaming;
    uint8_t             a2dp_curr_volume;
    uint8_t             avrcp_play_status;
    uint8_t             a2dp_codec_type;
    union
    {
        struct
        {
            uint8_t sampling_frequency;
            uint8_t channel_mode;
            uint8_t block_length;
            uint8_t subbands;
            uint8_t allocation_method;
            uint8_t min_bitpool;
            uint8_t max_bitpool;
        } sbc;
        struct
        {
            uint8_t  object_type;
            uint16_t sampling_frequency;
            uint8_t  channel_number;
            bool     vbr_supported;
            uint32_t bit_rate;
        } aac;
        struct
        {
            uint8_t  info[12];
        } vendor;
    } a2dp_codec_info;
} T_APP_BT_AUDIO_LINK;

/**  @brief  App define global app data structure */
typedef struct
{
    T_APP_BT_AUDIO_LINK     br_link[APP_BT_AUDIO_MAX_BR_LINK_NUM];
    uint8_t                 a2dp_role;
    uint8_t                 hfp_role;
    uint8_t                 remote_addr[6];
} T_APP_BT_AUDIO_APP_DB;
/** End of APP_BT_AUDIO_LINK_Exported_Types
* @}
*/

extern T_APP_BT_AUDIO_APP_DB app_db;

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BT_AUDIO_LINK_Exported_Functions BT Audio Link Exported Functions
  * @brief
  * @{
  */

/**
 * @brief     Find the APP SPP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP SPP data instance. If returned handle is NULL, the APP SPP
 *            data instance was failed to create.
 */
T_APP_BT_AUDIO_LINK *app_bt_audio_find_link(uint8_t *bd_addr);

/**
 * @brief     Allocate the APP SPP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP SPP data instance. If returned handle is NULL, the APP SPP
 *            data instance was failed to create.
 */
T_APP_BT_AUDIO_LINK *app_bt_audio_alloc_link(uint8_t *bd_addr);

/**
 * @brief     Free the APP SPP data instance with Remote BT address.
 * @param[in] bd_addr Remote BT address.
 * @return    APP SPP data instance. If returned handle is NULL, the APP SPP
 *            data instance was failed to create.
 */
bool app_bt_audio_free_link(T_APP_BT_AUDIO_LINK *p_link);

/** @} End of APP_BT_AUDIO_LINK_Exported_Functions */

/** @} End of APP_BT_AUDIO_LINK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_AUDIO_LINK_H_ */
