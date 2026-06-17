/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/
#ifndef _APP_SPP_PLAYBACK_H_
#define _APP_SPP_PLAYBACK_H_

//#include "patch_header_check.h"
//#include "app_relay.h"
#include "stdint.h"
#include "gap.h"
//#include "app_gpio.h"
/** @defgroup  APP_PLAYBACK_SERVICE APP SPP_PLAYBACK handle
    * @brief APP PLAYBACK Service to implement SPP_PLAYBACK feature
    * @{
    */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_PLAYBACK_SERVICE_Exported_Macros App SPP_PLAYBACK service Exported Macros
    * @brief
    * @{
    */


/*ota version num*/
#define BLE_PLAYBACK_VERSION     0x2
#define SPP_PLAYBACK_VERSION     0x5
#define NEW_PROTOCOL_VERSION     0x3
/*ic type*/
#define PLAYBACK_BBPRO              0x04
#define PLAYBACK_BBPRO2             0x07
/*query info*/
//#define PLAYBACK_MAX_BUFFER_SIZE     PLAYBACK_BUF_CHECK_SIZE
#define BLE_UPDATE_FILE_CMDID_LENGTH  (2)
#define BLE_UPDATE_FILE_SEQ_LENGTH    (2)
#define BLE_UPDATE_FILE_CRC_LENGTH    (2)
#define BLE_UPDATE_FILE_OFFSET_LENGTH (4)
#define BLE_UPDATE_FILE_LEN_LENGTH    (2)
#define BLE_UPDATE_FILE_HEADER_LENGTH (BLE_UPDATE_FILE_CMDID_LENGTH + BLE_UPDATE_FILE_SEQ_LENGTH + BLE_UPDATE_FILE_CRC_LENGTH + BLE_UPDATE_FILE_OFFSET_LENGTH + BLE_UPDATE_FILE_LEN_LENGTH)
#define BLE_ATT_HEADER_LENGTH         (3)

//rx and write
#define PLAYBACK_PKT_SIZE           0x300 //0x100
#define PLAYBACK_PKT_NUM            3
#define PLAYBACK_BUF_CHECK_SIZE     (PLAYBACK_PKT_SIZE * PLAYBACK_PKT_NUM) //0x800
#define PLAYBACK_PROTOCOL_VERSION   1 // 2020-10-20 version:1
#define PLAYBACK_MODE_SINGLE        0x00
#define PLAYBACK_MODE_COUPLE        0x01
#define PLAYBACK_MODE_COUPLE_PRI    (0x01 << 1)
#define PLAYBACK_MODE_COUPLE_SEC    0x00

#define PLAYBACK_LIST_HEAD_LEN          1
#define PLAYBACK_LIST_READ_LENTH_LEN    2
//read and tx
#define PLAYBACK_READ_FRAME_SIZE    0x100
#define PLAYBACK_READ_PKT_SIZE      (PLAYBACK_READ_FRAME_SIZE + \
                                     PLAYBACK_LIST_HEAD_LEN + \
                                     PLAYBACK_LIST_READ_LENTH_LEN)

#define PLAYBACK_BUF_A_EVENT      0x01
#define PLAYBACK_BUF_B_EVENT      0x02

/** @brief  spp playback command length. */
#define PLAYBACK_LENGTH_GET_INFO            0
#define PLAYBACK_LENGTH_GET_LIST_DATA       1
#define PLAYBACK_LENGTH_BUFFER_CHECK_EN     4
#define PLAYBACK_LENGTH_VALID_SONG          6
#define PLAYBACK_LRNGTH_TRIGGER_ROLE_SWAP   0
#define PLAYBACK_LENGTH_TRANS_CANCEL        0
#define PLAYBACK_LRNGTH_DELETE_SONG         2
#define PLAYBACK_LRNGTH_DELETE_ALL_SONG     0
#define PLAYBACK_LRNGTH_GET_SD_SPACE_INFO   0
#define PLAYBACK_LRNGTH_GET_FLASH_SPACE_INFO 0
#define PLAYBACK_LRNGTH_DELETE_ALL_BY_FORMAT 1
#define PLAYBACK_LENGTH_SET_SCENARIO        5

//byte wide
#define PLAYBACK_LEN_PLAYLIST_IDX           2 // ~ 2020-10-19: 1
#define PLAYBACK_LEN_FILE_IDX               2
#define PLAYBACK_LEN_NAME_LENTH             2
/*bit set of device info data3*/
#define PB_DEVICE_FEATURE_SUPPORT_BUFFER_CHECK     (1 << 0)
#define PB_DEVICE_FEATURE_RWS_SINGLE               (1 << 5)
#define PB_DEVICE_FEATURE_RWS_LEFT                 (1 << 6)
#define PB_DEVICE_FEATURE_RWS_UPDATE_SUCCESS       (1 << 7)

/** End of APP_PLAYBACK_SERVICE_Exported_Macros
    * @}
    */


/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_PLAYBACK_SERVICE_Exported_Types APP SPP_PLAYBACK Service Exported Types
    * @brief
    * @{
    */
/** @brief  PLAYBACK support format tye */
typedef enum
{
    PB_FORMAT_TYPE_DEFAULT          = 0x00,
    PB_FORMAT_TYPE_AAC              = 0x01,
    PB_FORMAT_TYPE_MP3              = 0x02,
    PB_FORMAT_TYPE_FLAC             = 0x04,
    PB_FORMAT_TYPE_WAV              = 0x08,
    PB_FORMAT_TYPE_RSVD
} T_PLAYBACK_FORMAT_TYPE;

typedef enum
{
    WATCH_GENERAL_FILE              = 0x100,
    WATCH_ONE_TIME_FILE             = 0x101,
    WATCH_PICTURE_RESOURCE          = 0x102,
    CS_SCREENAVER_PICTURE           = 0x200
} T_TRANSFER_SCENARIO_TYPE;

typedef enum
{
    MP3_SUFFIX_FILE,
    MP4_SUFFIX_FILE,
    RTK_SUFFIX_FILE,
    AAC_SUFFIX_FILE,
    FLAC_SUFFIX_FILE,
    TXT_SUFFIX_FILE,
    DAT_SUFFIX_FILE,
    BIN_SUFFIX_FILE
} T_TRANSFER_FILE_SUFFIX_TYPE;

typedef union
{
    uint8_t value[32];
    struct
    {
        uint16_t pkt_size;          //byte0-byte1
        uint16_t buf_check_size;    //byte2-byte3
        uint8_t  protocol_ver;      //byte4
        uint8_t  mode;              //byte5
        uint8_t  ic_type;           //byte6
        uint8_t  song_format_type;  //byte7
        uint8_t  rsvd[25];          //byte8-byte31
    };
} PLAYBACK_DEVICE_INFO;

/** @brief  PLAYBACK timer callback */
typedef enum
{
    APP_TIMER_PB_TRANS_FILE
} T_APP_TIMER_PLAYBACK_TRANS;

/** @brief  PLAYBACK transfer mode */
typedef enum
{
    PB_TRANS_RET_SUCCESS                      = 0x01,
    PB_TRANS_RET_CRC_ERROR                    = 0x05,
    PB_TRANS_RET_LENTH_ERROR                  = 0x06,
    PB_TRANS_RET_WRITE_ERROR                  = 0x07,
    PB_TRANS_RET_READ_ERROR                   = 0x10,
    PB_TRANS_RET_CREAT_ERROR                  = 0x11,
    PB_TRANS_RET_RX_DATA_ERROR                = 0x12,
    PB_TRANS_RET_OPERATION_ERROR              = 0x13,
    PB_TRANS_RET_OFFSET_ERROR                 = 0x14,
    PB_TRANS_RET_ROLE_SWAP_ERROR              = 0x15,
    PB_TRANS_RET_COUPLE_DISCON_ERROR          = 0x16,
    PB_TRANS_RET_CLOSE_ERROR                  = 0x17,
    PB_TRANS_RET_BUF_FULL_ERROR               = 0x18,
    PB_TRANS_RET_SEQUENCE_ERROR               = 0x19,
    PB_TRANS_RET_DELETE_ERROR                 = 0x1A,
    PB_TRANS_RET_DELETE_OPENED_ERROR          = 0x1B,

    PB_TRANS_RET_BUF_A_SUCCESS                = 0xF1,
    PB_TRANS_RET_BUF_B_SUCCESS                = 0xF2,

} T_SPP_PB_RET;

/** @brief  PLAYBACK mode */
typedef enum
{
    PLAYBACK_TRANS_SPP_MODE,
    PLAYBACK_TRANS_BLE_MODE,
} T_PB_TRANS_MODE;

/** @brief  Table used to Judge whether it is transfering  */
typedef enum
{
    TRANSFER_STOPPED,
    TRANSFER_STARTED,
} T_TRANSFER_STATUS;

typedef union _SPP_IDX_BLE_CONN_ID
{
    uint8_t spp_idx;
    uint8_t ble_conn_id;
} SPP_IDX_BLE_CONN_ID;

/** @brief  Table used to store Extended Device Information */
typedef struct _SPP_PB_FUNCTION_STRUCT
{
    uint32_t file_total_length;
    uint16_t seq;
    uint32_t cur_offset;
    uint8_t bd_addr[6];
    uint8_t trans_mode;
    T_TRANSFER_STATUS transfer_status;
    uint16_t transfer_scenario;
    SPP_IDX_BLE_CONN_ID id;
} PB_TRANS_FUNCTION_STRUCT;

//-------------------------------ble------------------------------//

/** End of APP_PLAYBACK_SERVICE_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_PLAYBACK_SERVICE_Exported_Functions APP PLAYBACK service Functions
    * @brief
    * @{
    */
/**
    * @brief  The main function to handle all the spp command
    * @param  length length of command id and data
    * @param  p_value data addr
    * @param  app_idx received rx command device index
    * @return void
    */
void app_trans_updatefile_cmd_handle(uint16_t length, uint8_t *p_value, uint8_t app_idx);

/**
    * @brief  app spp update file init
    * @param  void
    * @return void
    */
void app_trans_update_file_init(void);

/**
    * @brief  get playback transfer status
    * @return True:is doing ota; False: is not doing ota
    */
bool app_playback_trans_is_busy(void);

/**
    * @brief  The main function to handle all the ble command
    * @param  length length of command id and data
    * @param  p_value data addr
    * @param  conn_id received rx command conn_id
    * @return void
    */
T_APP_RESULT app_trans_updatefile_ble_handle_cp_req(uint8_t conn_id, uint16_t length,
                                                    uint8_t *p_value);
/** @} */ /* End of group APP_PLAYBACK_SERVICE_Exported_Functions */

/** @} */ /* End of group APP_PLAYBACK_SERVICE */
#endif
