/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_UART_DFU
#include <string.h>
#include <stdlib.h>
#include "app_uart_dfu.h"
#include "app_cmd.h"
#include "app_report.h"
#include "app_device.h"
#include "trace.h"
#include "dfu_api.h"
#include "fmc_api.h"
#include "ota_ext.h"
#include "app_dlps.h"

typedef enum
{
    UART_DFU_START_REQ  = 0,
    UART_DFU_DATA_IND   = 1,
    UART_DFU_ABORT      = 2,
    UART_DFU_REBOOT     = 3,
    UART_DFU_TEST_EN    = 4,
    UART_DFU_GET_VER    = 5,
} T_UART_DFU_OPCODE;

typedef enum
{
    UART_DFU_REJECT     = 0,
    UART_DFU_ACCEPT     = 1,
} T_UART_DFU_START_RSP;

typedef enum
{
    UART_DFU_FAIL,
    UART_DFU_SUCCESS,
} T_UART_DFU_RESULT;


T_UART_DFU_MGR dfu_mgr = {0};

uint8_t *ota_write_buf = NULL;
bool force_update = false;

static uint8_t count_one(uint8_t num)
{
    uint8_t count;
    for (count = 0; num != 0; count++)
    {
        num &= (num - 1);
    }
    return count;
}

static uint8_t uart_dfu_get_sub_image_header_count(uint8_t *p_data, uint8_t len)
{
    uint8_t file_count = 0;
    for (uint8_t i = 0; i < len; i++)
    {
        file_count += count_one(*(p_data + i));
    }
    return file_count;
}

void uart_dfu_merged_header_parse(uint8_t *p)
{
    LE_STREAM_TO_UINT16(dfu_mgr.sub_bin.merged_header.signature, p);
    LE_STREAM_TO_UINT32(dfu_mgr.sub_bin.merged_header.size_of_merged_file, p);
    memcpy(dfu_mgr.sub_bin.merged_header.checksum, p,
           sizeof(dfu_mgr.sub_bin.merged_header.checksum));
    p += sizeof(dfu_mgr.sub_bin.merged_header.checksum);
    LE_STREAM_TO_UINT16(dfu_mgr.sub_bin.merged_header.extension, p);
    memcpy(dfu_mgr.sub_bin.merged_header.sub_file_indicator, p,
           sizeof(dfu_mgr.sub_bin.merged_header.sub_file_indicator));
    dfu_mgr.sub_bin.merged_header.sub_file_count =
        uart_dfu_get_sub_image_header_count(
            dfu_mgr.sub_bin.merged_header.sub_file_indicator,
            sizeof(dfu_mgr.sub_bin.merged_header.sub_file_indicator));

    APP_PRINT_INFO6("uart_dfu_merged_header_parse: sub_image_header signature 0x%04x, size_of_merged_file 0x%08x, checksum %b, extension %x, sub_file_indicator 0x%x, sub_file_count %d",
                    dfu_mgr.sub_bin.merged_header.signature,
                    dfu_mgr.sub_bin.merged_header.size_of_merged_file,
                    TRACE_BINARY(sizeof(dfu_mgr.sub_bin.merged_header.checksum),
                                 dfu_mgr.sub_bin.merged_header.checksum),
                    dfu_mgr.sub_bin.merged_header.extension,
                    dfu_mgr.sub_bin.merged_header.sub_file_indicator,
                    dfu_mgr.sub_bin.merged_header.sub_file_count);
}

void uart_dfu_handle_sub_file_header(uint8_t *p_data)
{
    for (uint8_t i = 0; i < dfu_mgr.sub_bin.merged_header.sub_file_count; i++)
    {
        LE_STREAM_TO_UINT32(dfu_mgr.sub_bin.sub_image_header[i].download_addr,
                            p_data);
        LE_STREAM_TO_UINT32(dfu_mgr.sub_bin.sub_image_header[i].size, p_data);
        LE_STREAM_TO_UINT32(dfu_mgr.sub_bin.sub_image_header[i].reserved, p_data);
        if (i == 0)
        {
            dfu_mgr.sub_bin.sub_image_header[i].start_file_offset =
                OTA_MERGED_FILE_HEAD_SIZE +
                OTA_MERGED_FILE_SUB_HEAD_SIZE * dfu_mgr.sub_bin.merged_header.sub_file_count;
        }
        else
        {
            dfu_mgr.sub_bin.sub_image_header[i].start_file_offset =
                dfu_mgr.sub_bin.sub_image_header[i -
                                                 1].start_file_offset + dfu_mgr.sub_bin.sub_image_header[i - 1].size;
        }
        APP_PRINT_INFO4("uart_dfu_handle_sub_file_header: single %d, download_addr %p, size 0x%08x, start_file_offset 0x%08x",
                        i,
                        dfu_mgr.sub_bin.sub_image_header[i].download_addr,
                        dfu_mgr.sub_bin.sub_image_header[i].size,
                        dfu_mgr.sub_bin.sub_image_header[i].start_file_offset
                       );
    }
}

uint8_t uart_dfu_get_active_bank(void)
{
    uint8_t active_bank;
    uint32_t ota_bank0_addr;
    if (is_ota_support_bank_switch())
    {
        ota_bank0_addr = flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0);
        if (ota_bank0_addr == get_active_ota_bank_addr())
        {
            active_bank = IMAGE_LOCATION_BANK0;
        }
        else
        {
            active_bank = IMAGE_LOCATION_BANK1;
        }
    }
    else
    {
        active_bank = NOT_SUPPORT_BANK_SWITCH;
    }
    return active_bank;
}

void uart_dfu_single_check_set_sub_bin_info(void)
{
    uint8_t active_bank = uart_dfu_get_active_bank();
    if (active_bank == IMAGE_LOCATION_BANK0)
    {
        dfu_mgr.cur_sub_image_index =
            dfu_mgr.sub_bin.merged_header.sub_file_count /
            2;
        dfu_mgr.end_sub_image_index =
            dfu_mgr.sub_bin.merged_header.sub_file_count;
    }
    else if (active_bank == IMAGE_LOCATION_BANK1)
    {
        dfu_mgr.cur_sub_image_index = 0;
        dfu_mgr.end_sub_image_index =
            dfu_mgr.sub_bin.merged_header.sub_file_count /
            2;
        dfu_mgr.end_sub_image_index++; //common voice prompt data image
    }
    else
    {
        APP_PRINT_ERROR1("uart_dfu_single_check_set_sub_bin_info: active_bank %d", active_bank);
    }
    dfu_mgr.cur_sub_image_relative_offset = 0;
    APP_PRINT_INFO5("uart_dfu_single_check_set_sub_bin_info: current bud_role %d, app_cfg_const.bud_role %d, active_bank %d, cur_sub_image_index %d, end_sub_image_index %d",
                    app_cfg_nv.bud_role, app_cfg_const.bud_role, active_bank,
                    dfu_mgr.cur_sub_image_index,
                    dfu_mgr.end_sub_image_index);
}

bool uart_dfu_print_control_header(uint8_t *p_data, uint16_t len)
{
    bool ret = true;
    T_IMG_CTRL_HEADER_FORMAT *p_img_header = (T_IMG_CTRL_HEADER_FORMAT *)p_data;
    APP_PRINT_INFO6("uart_dfu_print_control_header 1: "
                    " crc16 0x%04x, ic_type 0x%02x, secure_version 0x%02x, value 0x%04x, image_id 0x%04x, payload_len 0x%08x",
                    p_img_header->crc16, p_img_header->ic_type, p_img_header->secure_version,
                    p_img_header->ctrl_flag.value,
                    p_img_header->image_id, p_img_header->payload_len);
    APP_PRINT_INFO8("uart_dfu_print_control_header 2: "
                    "xip %d, enc %d, load_when_boot %d, enc_load %d, enc_key_select %d, "
                    "not_ready %d, not_obsolete %d, integrity_check_en_in_boot %d",
                    p_img_header->ctrl_flag.xip, p_img_header->ctrl_flag.enc, p_img_header->ctrl_flag.load_when_boot,
                    p_img_header->ctrl_flag.enc_load, p_img_header->ctrl_flag.enc_key_select,
                    p_img_header->ctrl_flag.not_ready,
                    p_img_header->ctrl_flag.not_obsolete, p_img_header->ctrl_flag.integrity_check_en_in_boot);
    return ret;
}

uint32_t uart_dfu_write_to_flash(uint32_t dfu_base_addr, uint32_t offset, uint32_t length,
                                 void *p_void)
{
    uint8_t ret = 0;
    uint8_t readback_buffer[READBACK_BUFFER_SIZE];
    uint32_t read_back_len;
    uint32_t dest_addr;
    uint8_t *p_src = (uint8_t *)p_void;
    uint32_t remain_size = length;
    uint8_t bp_level;
    fmc_flash_nor_get_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0),
                            &bp_level);
    fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0),
                            0);

    if (offset == 0)
    {
        T_IMG_HEADER_FORMAT *p_header = (T_IMG_HEADER_FORMAT *)p_void;
        p_header->ctrl_header.ctrl_flag.not_ready = 0x1;
    }
    if (((offset + dfu_base_addr) % 0x1000) == 0)
    {
        fmc_flash_nor_erase(dfu_base_addr + offset, FMC_FLASH_NOR_ERASE_SECTOR);
    }
    else
    {
        if (((offset + dfu_base_addr) / 0x1000) != ((offset + length + dfu_base_addr) / 0x1000))
        {
            if ((offset + length + dfu_base_addr) % 0x1000)
            {
                fmc_flash_nor_erase((dfu_base_addr + offset + length) & ~(0x1000 - 1), FMC_FLASH_NOR_ERASE_SECTOR);
            }
        }
    }
    APP_PRINT_INFO3("uart_dfu_write_to_flash: dfu_base_addr 0x%08x, offset 0x%x, length 0x%x, ",
                    dfu_base_addr, offset, length);
    if (!fmc_flash_nor_write(dfu_base_addr + offset, p_void, length))
    {
        ret = 2;
    }
    extern void cache_flush_by_addr(uint32_t *addr, uint32_t length);
    cache_flush_by_addr((uint32_t *)(dfu_base_addr + offset), length);
    fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0),
                            bp_level);
    dest_addr = dfu_base_addr + offset;
    while (remain_size)
    {
        read_back_len = (remain_size >= READBACK_BUFFER_SIZE) ? READBACK_BUFFER_SIZE : remain_size;
        if (!fmc_flash_nor_read(dest_addr, readback_buffer, read_back_len))
        {
            ret = 3;
            break;
        }
        if (memcmp(readback_buffer, p_src, read_back_len) != 0)
        {
            ret = 4;
            break;
        }
        dest_addr += read_back_len;
        p_src += read_back_len;
        remain_size -= read_back_len;
    }
    if (ret)
    {
        APP_PRINT_ERROR1("uart_dfu_write_to_flash: ret %d", ret);
    }
    return ret;
}

static void uart_dfu_clear_notready_flag(void)
{
    uint8_t bp_level;
    fmc_flash_nor_get_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0), &bp_level);
    fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0), 0);
    app_ota_clear_notready_flag();
    fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0), bp_level);
}

static void uart_dfu_update_complete_check(void)
{
    APP_PRINT_INFO0("uart_dfu_update_complete_check");
    uart_dfu_clear_notready_flag();
    dfu_mgr.state = UART_DFU_STATE_FINISH;
}

static bool uart_dfu_check_image(void *p_header)
{
    return check_image_sum((uint32_t)p_header);
}


static void uart_dfu_req_data(uint8_t cmd_path, uint32_t next_block_offset,
                              uint16_t next_block_length)
{
    struct
    {
        uint32_t   offset;
        uint16_t   length;
    } __attribute__((packed)) rpt =
    {
        .offset = next_block_offset,
        .length = next_block_length,
    };
    dfu_mgr.cur_expect_block_len = next_block_length;
    app_report_event(cmd_path, EVENT_DFU_DATA_REQ, 0, (uint8_t *)&rpt, sizeof(rpt));
}

static void uart_dfu_restore(void)
{
    memset(&dfu_mgr, 0, sizeof(T_UART_DFU_MGR));
    if (ota_write_buf != NULL)
    {
        free(ota_write_buf);
        ota_write_buf = NULL;
    }
    force_update = false;
    app_dlps_enable(APP_DLPS_ENTER_CHECK_OTA_TOOLING_PARK);
}

static void uart_duf_erase_ota_header(void)
{
    if (force_update)
    {
        force_update = false;
        if (is_ota_support_bank_switch())
        {
            APP_PRINT_TRACE0("uart_duf_erase_ota_header");
            fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0), 0);
            uint32_t header_addr = get_active_ota_bank_addr();
            fmc_flash_nor_erase(header_addr, FMC_FLASH_NOR_ERASE_SECTOR);
        }
    }
}

static void uart_dfu_handle_abort_req(void)
{
    APP_PRINT_INFO0("uart_dfu_handle_abort_req");
    uart_dfu_restore();
}

static void uart_dfu_handle_reboot_req(void)
{
    APP_PRINT_INFO0("uart_dfu_handle_reboot_req");
    uart_duf_erase_ota_header();
    app_device_reboot(3000, RESET_ALL);
}

static uint32_t uart_dfu_get_ota_header_ver(void)
{
    T_IMG_HEADER_FORMAT *ota_header;
    ota_header = (T_IMG_HEADER_FORMAT *)app_get_active_bank_addr(IMG_OTA);
    return ota_header->ver_val;
}

static void uart_dfu_handle_get_ver(uint8_t cmd_path)
{
    uint32_t local_version = uart_dfu_get_ota_header_ver();
    app_report_event(cmd_path, EVENT_DFU_LOCAL_VERSION, 0, (uint8_t *)&local_version,
                     sizeof(local_version));
}

static void uart_dfu_handle_test_en(void)
{
    force_update = true;
}

static void uart_dfu_handle_start_req(uint8_t cmd_path, uint8_t *data, uint16_t len)
{
    uint32_t dfu_version, local_version;
    uint32_t next_block_offset;
    uint16_t next_block_length;
    uint8_t ret = 0;
    T_UART_DFU_START_RSP rsp_state = UART_DFU_REJECT;

    LE_STREAM_TO_UINT32(dfu_version, data);
    local_version = uart_dfu_get_ota_header_ver();

    APP_PRINT_INFO3("uart_dfu_handle_start_req: force_update %d, version local 0x%08x, dfu 0x%08x",
                    force_update, local_version, dfu_version);

    if (!force_update && local_version > dfu_version)
    {
        ret = 1;
        goto ERR;
    }

    if (dfu_mgr.state != UART_DFU_STATE_IDLE)
    {
        ret = 2;
        goto ERR;
    }

    ota_write_buf = malloc(OTA_WRITE_BUFFER_SIZE);
    if (ota_write_buf == NULL)
    {
        ret = 3;
        goto ERR;
    }

    app_dlps_disable(APP_DLPS_ENTER_CHECK_OTA_TOOLING_PARK);

    dfu_mgr.state = UART_DFU_STATE_MERGED_HEADER;
    rsp_state = UART_DFU_ACCEPT;
    app_report_event(cmd_path, EVENT_DFU_START_RSP, 0, (uint8_t *)&rsp_state, sizeof(rsp_state));

    next_block_offset = 0;
    next_block_length = OTA_MERGED_FILE_HEAD_SIZE;
    uart_dfu_req_data(cmd_path, next_block_offset, next_block_length);
    return;

ERR:
    APP_PRINT_INFO1("uart_dfu_handle_start_req: failed %d", -ret);
    app_report_event(cmd_path, EVENT_DFU_START_RSP, 0, (uint8_t *)&rsp_state, sizeof(rsp_state));

}

static void uart_dfu_handle_data_ind(uint8_t cmd_path, uint8_t *data, uint16_t len)
{
    uint32_t next_block_offset = 0;
    uint16_t next_block_length = 0;

    if (dfu_mgr.cur_expect_block_len != len)
    {
        APP_PRINT_ERROR2("uart_dfu_handle: cur_expect_block_len %d, coming length %d",
                         dfu_mgr.cur_expect_block_len,
                         len);
        T_UART_DFU_RESULT duf_result = UART_DFU_FAIL;
        app_report_event(cmd_path, EVENT_DFU_RESULT, 0, (uint8_t *)&duf_result, sizeof(duf_result));
        return;
    }

    if (dfu_mgr.state  == UART_DFU_STATE_MERGED_HEADER) //handle merged header
    {
        APP_PRINT_INFO0("uart_dfu_handle: UART_DFU_STATE_MERGED_HEADER");
        uart_dfu_merged_header_parse(data);

        next_block_offset = OTA_MERGED_FILE_HEAD_SIZE;
        next_block_length = dfu_mgr.sub_bin.merged_header.sub_file_count *
                            OTA_MERGED_FILE_SUB_HEAD_SIZE;
        uart_dfu_req_data(cmd_path, next_block_offset, next_block_length);

        dfu_mgr.state  = UART_DFU_STATE_SUB_FILE_HEADER;
    }
    else if (dfu_mgr.state == UART_DFU_STATE_SUB_FILE_HEADER) //handle sub_file headers
    {
        APP_PRINT_INFO0("uart_dfu_handle: UART_DFU_STATE_SUB_FILE_HEADER");
        uart_dfu_handle_sub_file_header(data);
        uart_dfu_single_check_set_sub_bin_info();

        dfu_mgr.cur_sub_image_relative_offset += OTA_SUB_IMAGE_MP_HEAD_SIZE;
        next_block_offset = dfu_mgr.sub_bin.sub_image_header[dfu_mgr.cur_sub_image_index].start_file_offset
                            +
                            dfu_mgr.cur_sub_image_relative_offset +
                            OTA_SUB_IMAGE_PAYLOAD_IMAGE_AUTH_HEAD_SIZE; // skip auth header to read control header
        next_block_length = OTA_SUB_IMAGE_CONTROL_HEAD_SIZE;
        uart_dfu_req_data(cmd_path, next_block_offset, next_block_length);

        dfu_mgr.state = UART_DFU_STATE_CONTROL_HEADER;
    }
    else if (dfu_mgr.state == UART_DFU_STATE_CONTROL_HEADER) // READ handle control header
    {
        APP_PRINT_INFO0("uart_dfu_handle: UART_DFU_STATE_CONTROL_HEADER");
        uart_dfu_print_control_header(data, OTA_SUB_IMAGE_CONTROL_HEAD_SIZE);

        next_block_offset = dfu_mgr.sub_bin.sub_image_header[dfu_mgr.cur_sub_image_index].start_file_offset
                            +
                            dfu_mgr.cur_sub_image_relative_offset;
        next_block_length = OTA_PACKET_SIZE;
        uart_dfu_req_data(cmd_path, next_block_offset, next_block_length);

        dfu_mgr.state = UART_DFU_STATE_IMAGE_PAYLOAD;
    }
    else if (dfu_mgr.state == UART_DFU_STATE_IMAGE_PAYLOAD)
    {
        // Real pkt could be download
        memcpy(ota_write_buf + dfu_mgr.buf_index, data, len);
        dfu_mgr.buf_index += len;
        //update cur_sub_image_relative_offset
        dfu_mgr.cur_sub_image_relative_offset += len;
        APP_PRINT_INFO1("uart_dfu_handle: UART_DFU_STATE_IMAGE_PAYLOAD buf_index 0x%x",
                        dfu_mgr.buf_index);
        if ((dfu_mgr.buf_index == OTA_WRITE_BUFFER_SIZE) || // Every 4k write once
            (dfu_mgr.cur_sub_image_relative_offset ==
             dfu_mgr.sub_bin.sub_image_header[dfu_mgr.cur_sub_image_index].size)) // Or last packet
        {
            uint32_t dfu_base_addr =
                dfu_mgr.sub_bin.sub_image_header[dfu_mgr.cur_sub_image_index].download_addr;

            uint32_t offset = dfu_mgr.cur_sub_image_relative_offset - OTA_SUB_IMAGE_MP_HEAD_SIZE -
                              dfu_mgr.buf_index;
            APP_PRINT_INFO0("uart_dfu_handle: UART_DFU_STATE_IMAGE_PAYLOAD write to flash");
            uint32_t write_result = uart_dfu_write_to_flash(dfu_base_addr, offset, dfu_mgr.buf_index,
                                                            ota_write_buf);
            if (write_result != 0)
            {
                APP_PRINT_ERROR1("uart_dfu_handle: uart_dfu_write_to_flash failed ret %d", write_result);
                uart_dfu_restore();
                T_UART_DFU_RESULT duf_result = UART_DFU_FAIL;
                app_report_event(CMD_PATH_UART, EVENT_DFU_RESULT, 0, (uint8_t *)&duf_result, sizeof(duf_result));
                return;
            }
            dfu_mgr.buf_index = 0;

            //last pkt of one sub image, check image
            if (dfu_mgr.cur_sub_image_relative_offset
                ==
                dfu_mgr.sub_bin.sub_image_header[dfu_mgr.cur_sub_image_index].size)
            {
                bool check_image = uart_dfu_check_image((void *)dfu_base_addr);
                APP_PRINT_INFO1("uart_dfu_handle: uart_dfu_check_image %d", check_image);
                if (check_image)
                {
                    //update cur_sub_image_relative_offset
                    dfu_mgr.cur_sub_image_relative_offset = 0;
                    dfu_mgr.cur_sub_image_index++;
                    //if it is last sub image, complete check
                    if (dfu_mgr.cur_sub_image_index == dfu_mgr.end_sub_image_index)
                    {
                        //OTA complete
                        dfu_mgr.cur_sub_image_relative_offset = 0;
                        dfu_mgr.cur_sub_image_index = 0;
                        uart_dfu_update_complete_check();
                        // fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0), 0);

                        // uint32_t header_addr = get_active_ota_bank_addr();
                        // fmc_flash_nor_erase(header_addr, FMC_FLASH_NOR_ERASE_SECTOR);
                        T_UART_DFU_RESULT duf_result = UART_DFU_SUCCESS;
                        app_report_event(CMD_PATH_UART, EVENT_DFU_RESULT, 0, (uint8_t *)&duf_result, sizeof(duf_result));
                        return;
                    }
                    else
                    {
                        //request Nth sub image
                        APP_PRINT_INFO1("uart_dfu_handle: start update %d sub image",
                                        dfu_mgr.cur_sub_image_index);
                        //skip mp header OTA_SUB_IMAGE_MP_HEAD_SIZE = 512 bytes
                        dfu_mgr.cur_sub_image_relative_offset += OTA_SUB_IMAGE_MP_HEAD_SIZE;

                        next_block_offset = dfu_mgr.cur_sub_image_relative_offset +
                                            dfu_mgr.sub_bin.sub_image_header[dfu_mgr.cur_sub_image_index].start_file_offset
                                            + OTA_SUB_IMAGE_PAYLOAD_IMAGE_AUTH_HEAD_SIZE;

                        next_block_length = OTA_SUB_IMAGE_CONTROL_HEAD_SIZE;
                        dfu_mgr.state = UART_DFU_STATE_CONTROL_HEADER;
                        uart_dfu_req_data(cmd_path, next_block_offset, next_block_length);
                        return;
                    }
                }
                else
                {
                    APP_PRINT_ERROR0("uart_dfu_handle: uart_dfu_check_image failed");
                    T_UART_DFU_RESULT duf_result = UART_DFU_FAIL;
                    app_report_event(CMD_PATH_UART, EVENT_DFU_RESULT, 0, (uint8_t *)&duf_result, sizeof(duf_result));
                    uart_dfu_restore();
                    return;
                }
            }
        }
        //prepare next block
        next_block_length = OTA_PACKET_SIZE;
        if (dfu_mgr.cur_sub_image_relative_offset + OTA_PACKET_SIZE >
            dfu_mgr.sub_bin.sub_image_header[dfu_mgr.cur_sub_image_index].size)
        {
            next_block_length =
                dfu_mgr.sub_bin.sub_image_header[dfu_mgr.cur_sub_image_index].size
                -
                dfu_mgr.cur_sub_image_relative_offset;
        }
        next_block_offset = dfu_mgr.cur_sub_image_relative_offset +
                            dfu_mgr.sub_bin.sub_image_header[dfu_mgr.cur_sub_image_index].start_file_offset;
        dfu_mgr.state = UART_DFU_STATE_IMAGE_PAYLOAD;
        uart_dfu_req_data(cmd_path, next_block_offset, next_block_length);
        APP_PRINT_INFO1("uart_dfu_handle: cur relative offset 0x%x", dfu_mgr.cur_sub_image_relative_offset);
    }
}

void app_uart_dfu_process(uint8_t cmd_path, uint16_t opcode, uint8_t *p_data, uint16_t len)
{
    switch (opcode)
    {
    case UART_DFU_TEST_EN:
        uart_dfu_handle_test_en();
        break;
    case UART_DFU_START_REQ:
        uart_dfu_handle_start_req(cmd_path, p_data, len);
        break;

    case UART_DFU_DATA_IND:
        uart_dfu_handle_data_ind(cmd_path, p_data, len);
        break;

    case UART_DFU_REBOOT:
        uart_dfu_handle_reboot_req();
        break;

    case UART_DFU_ABORT:
        uart_dfu_handle_abort_req();
        break;

    case UART_DFU_GET_VER:
        uart_dfu_handle_get_ver(cmd_path);
        break;

    default:
        break;
    }
}

void app_uart_dfu_handle_cmd_set(uint8_t app_idx, uint8_t cmd_path, uint8_t *cmd_ptr,
                                 uint16_t cmd_len, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    if (cmd_id == CMD_UART_DFU)
    {
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        uint16_t opcode = (uint16_t)(cmd_ptr[2] | (cmd_ptr[3] << 8));
        app_uart_dfu_process(cmd_path, opcode, &cmd_ptr[4], cmd_len - 4);
    }
}
#endif
