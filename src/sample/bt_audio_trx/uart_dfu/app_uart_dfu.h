/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_UART_DFU_H_
#define _APP_UART_DFU_H_

#include "patch_header_check.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_SUB_IMAGE_PAYLOAD_IMAGE_AUTH_HEAD_SIZE        304
#define OTA_MERGED_FILE_HEAD_SUB_FILE_INDICATOR_SIZE      32
#define OTA_SUB_BIN_COUNT                                 21

#define OTA_MERGED_FILE_HEAD_SIZE                         40 + OTA_MERGED_FILE_HEAD_SUB_FILE_INDICATOR_SIZE
#define OTA_MERGED_FILE_SUB_HEAD_SIZE                     12
#define OTA_SUB_IMAGE_MP_HEAD_SIZE                        512
#define OTA_SUB_IMAGE_CONTROL_HEAD_SIZE                   12

#define OTA_WRITE_BUFFER_SIZE                             2 * 1024
#define OTA_PACKET_SIZE                                   256 // must can be aligned to OTA_WRITE_BUFFER_SIZE
#define FLASH_OFFSET_TO_NO_CACHE                          0x01000000

#define IMAGE_LOCATION_BANK0                              1
#define IMAGE_LOCATION_BANK1                              2

#define NOT_SUPPORT_BANK_SWITCH                           3
#define READBACK_BUFFER_SIZE                              64

typedef enum
{
    UART_DFU_STATE_IDLE,
    UART_DFU_STATE_MERGED_HEADER,
    UART_DFU_STATE_SUB_FILE_HEADER,
    UART_DFU_STATE_CONTROL_HEADER, //control header
    UART_DFU_STATE_IMAGE_PAYLOAD,
    UART_DFU_STATE_FINISH,
} T_UART_DFU_STATE;

typedef struct _T_MERGED_HEADER
{
    uint16_t signature;
    uint32_t size_of_merged_file;
    uint8_t  checksum[32];
    uint16_t extension;
    uint8_t  sub_file_indicator[OTA_MERGED_FILE_HEAD_SUB_FILE_INDICATOR_SIZE];
    uint8_t  sub_file_count;
} T_MERGED_HEADER;

typedef struct _T_SUB_IMAGE_HEADER
{
    IMG_ID image_id;
    uint32_t download_addr;
    uint32_t size;
    uint32_t reserved;
    uint32_t start_file_offset;
} T_SUB_IMAGE_HEADER;

typedef struct _T_SUB_IMAGE_INFO
{
    T_MERGED_HEADER merged_header;
    T_SUB_IMAGE_HEADER sub_image_header[OTA_SUB_BIN_COUNT];
} T_SUB_BIN_INFO;

typedef struct _T_UART_DFU_MGR
{
    uint16_t buf_index;
    T_UART_DFU_STATE state;
    uint8_t     cur_sub_image_index;
    uint8_t     end_sub_image_index;
    uint32_t    cur_sub_image_relative_offset;
    uint16_t    cur_expect_block_len;
    T_SUB_BIN_INFO sub_bin;
} T_UART_DFU_MGR;

void app_uart_dfu_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                 uint16_t cmd_len, uint8_t *ack_pkt);

#ifdef __cplusplus
}
#endif
#endif
