/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "guidef.h"
#include "gui_port.h"
#include "drv_lcd.h"
#include "gui_api.h"
#include "trace.h"
#include "string.h"
#include "rtl876x_gdma.h"
#include "rtl876x_rcc.h"
#include "app_gui.h"
#if ENABLE_PSRAM_FOR_LCD
#include "module_psram.h"
#endif

#define LCD_SECTION_HEIGHT                      20
#define LCD_DMA_CHANNEL_NUM                     gui_port_dc_dma_ch_num
#define LCD_DMA_CHANNEL_INDEX                   DMA_CH_BASE(gui_port_dc_dma_ch_num)

static uint8_t gui_port_dc_dma_ch_num = 0xa5;
static bool gui_task_work = true;

bool gui_port_dc_lcd_is_work(void)
{
    return gui_task_work;
}

void gui_port_dc_set_dma(uint8_t num)
{
    gui_port_dc_dma_ch_num = num;
}

static void memcpy_by_dma_start(void *dst, void *src, uint32_t size)
{
    GUI_ASSERT((uint32_t)dst % 4 == 0);
    GUI_ASSERT((uint32_t)src % 4 == 0);
    GUI_ASSERT(size % 4 == 0);

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum = 0;
    GDMA_InitStruct.GDMA_DIR = GDMA_DIR_MemoryToMemory;
    GDMA_InitStruct.GDMA_BufferSize = size / 4;
    GDMA_InitStruct.GDMA_SourceInc = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize = GDMA_Msize_16;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_16;
    GDMA_InitStruct.GDMA_SourceAddr = (uint32_t)src;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)dst;
    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
}

static void memcpy_by_dma_done(void)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
}

uint32_t rt_tick_get_millisecond(void)
{
    return sys_timestamp_get();
}

void port_gui_lcd_power_on(void)
{
    APP_PRINT_TRACE0("port_gui_lcd_power_on");
    gui_task_work = true;
#if ENABLE_PSRAM_FOR_LCD
    app_psram_exit_dlps();
#endif
    drv_lcd_power_on();
}

void port_gui_lcd_power_off(void)
{
    APP_PRINT_TRACE0("port_gui_lcd_power_off");
    gui_task_work = false;
    drv_lcd_power_off();
#if ENABLE_PSRAM_FOR_LCD
    app_psram_enter_dlps();
#endif
}

#define DEST_ADDR       0x04000000 //FMC_MAIN1_ADDR   PSRAM ADDR
void port_gui_lcd_update(struct gui_dispdev *dc)
{
    uint32_t total_section_cnt = 0;
#if (ENABLE_TE_FOR_LCD == 1)
    static bool first_frame_read_te_type = true;
    static T_LCDC_TE_TYPE te_type;
#endif

    if (dc->pfb_type == PFB_Y_DIRECTION)
    {
        total_section_cnt = ((drv_lcd_get_screen_height() - 1) / LCD_SECTION_HEIGHT + 1);
    }
    else if (dc->pfb_type == PFB_X_DIRECTION)
    {
        total_section_cnt = ((drv_lcd_get_screen_width() - 1) / LCD_SECTION_HEIGHT + 1);
    }

#if (ENABLE_PSRAM_FOR_LCD == 0)
    if (dc->section_count == 0)
    {
#if (ENABLE_TE_FOR_LCD == 1)
        if (first_frame_read_te_type)
        {
            te_type = drv_lcd_get_TE_type();
            first_frame_read_te_type = false;
        }
        drv_lcd_set_TE_type(te_type);
        if (te_type == LCDC_TE_TYPE_SW_TE)
        {
            drv_lcd_te_enable();
            while (!drv_lcd_get_te_trigger_state());
            drv_lcd_set_te_trigger_state(false);
            drv_lcd_te_disable();
        }
#endif
        if (dc->pfb_type == PFB_Y_DIRECTION)
        {
            drv_lcd_set_window(0, dc->fb_height * dc->section_count, dc->fb_width, dc->fb_height);
        }
        else if (dc->pfb_type == PFB_X_DIRECTION)
        {
            drv_lcd_set_window(dc->fb_width * dc->section_count, 0, dc->fb_width, dc->fb_height);
        }
        drv_lcd_start_transfer(dc->frame_buf, dc->fb_width * dc->fb_height);
    }
    else if (dc->section_count == total_section_cnt - 1)
    {
        drv_lcd_transfer_done();
#if (ENABLE_TE_FOR_LCD == 1)
        drv_lcd_set_TE_type(LCDC_TE_TYPE_NO_TE);
#endif
        if (dc->pfb_type == PFB_Y_DIRECTION)
        {
            uint32_t last_height = dc->screen_height - dc->section_count * dc->fb_height;
            drv_lcd_set_window(0, dc->fb_height * dc->section_count, dc->fb_width, last_height);
            drv_lcd_start_transfer(dc->frame_buf, dc->fb_width * last_height);
        }
        else if (dc->pfb_type == PFB_X_DIRECTION)
        {
            uint32_t last_width = dc->screen_width - dc->section_count * dc->fb_width;
            drv_lcd_set_window(dc->fb_width * dc->section_count, 0, last_width, dc->fb_height);
            drv_lcd_start_transfer(dc->frame_buf, dc->fb_height * last_width);
        }
        drv_lcd_transfer_done();
    }
    else
    {
        drv_lcd_transfer_done();
        if (dc->pfb_type == PFB_Y_DIRECTION)
        {
            drv_lcd_set_window(0, dc->fb_height * dc->section_count, dc->fb_width, dc->fb_height);
        }
        else if (dc->pfb_type == PFB_X_DIRECTION)
        {
            drv_lcd_set_window(dc->fb_width * dc->section_count, 0, dc->fb_width, dc->fb_height);
        }
#if (ENABLE_TE_FOR_LCD == 1)
        drv_lcd_set_TE_type(LCDC_TE_TYPE_NO_TE);
#endif
        drv_lcd_start_transfer(dc->frame_buf, dc->fb_width * dc->fb_height);
    }
#else
    uint32_t i = dc->section_count;
    void *dst = (void *)(DEST_ADDR + i * dc->fb_width * dc->fb_height * PIXEL_BYTES);
    if (i == 0)
    {
#if (ENABLE_TE_FOR_LCD == 1)
        if (first_frame_read_te_type)
        {
            te_type = drv_lcd_get_TE_type();
            first_frame_read_te_type = false;
        }
        drv_lcd_set_TE_type(te_type);
#endif

        memcpy_by_dma_start(dst, dc->frame_buf,
                            dc->fb_width * dc->fb_height * PIXEL_BYTES);
    }
    else if (i == total_section_cnt - 1)
    {
        uint32_t last_height = dc->screen_height - dc->section_count * dc->fb_height;
        memcpy_by_dma_done();
        memcpy_by_dma_start(dst, dc->frame_buf, dc->fb_width * last_height * PIXEL_BYTES);
        memcpy_by_dma_done();
#if (ENABLE_TE_FOR_LCD == 1)
        if (te_type == LCDC_TE_TYPE_SW_TE)
        {
            drv_lcd_te_enable();
            while (!drv_lcd_get_te_trigger_state());
            drv_lcd_set_te_trigger_state(false);
            drv_lcd_te_disable();
        }
#endif
        drv_lcd_update((uint8_t *)DEST_ADDR, 0, 0, drv_lcd_get_screen_width(), drv_lcd_get_screen_height());
    }
    else
    {
        memcpy_by_dma_done();
        memcpy_by_dma_start(dst, dc->frame_buf, dc->fb_width * dc->fb_height * PIXEL_BYTES);
    }
#endif
}

static struct gui_dispdev dc =
{
    .type = DC_RAMLESS,
    .section = {0, 0, 0, 0},
    .section_count = 0,

    .lcd_update = port_gui_lcd_update,
    .lcd_power_on = port_gui_lcd_power_on,
    .lcd_power_off = port_gui_lcd_power_off,

    .flash_seq_trans_disable = NULL,
    .flash_seq_trans_enable = NULL,
    .reset_lcd_timer = NULL,
    .get_lcd_us = NULL,

    .lcd_te_wait = NULL,

};

#if (FB_DIRECTION_ROTATE == 1)
static uint8_t __attribute__((aligned(4))) disp_write_buff1_port[LCD_HIGHT * LCD_SECTION_HEIGHT *
                                                                           2];
static uint8_t __attribute__((aligned(4))) disp_write_buff2_port[LCD_HIGHT * LCD_SECTION_HEIGHT *
                                                                           2];
#else
static uint8_t __attribute__((aligned(4))) disp_write_buff1_port[LCD_WIDTH * LCD_SECTION_HEIGHT *
                                                                           2];
static uint8_t __attribute__((aligned(4))) disp_write_buff2_port[LCD_WIDTH * LCD_SECTION_HEIGHT *
                                                                           2];
#endif

void gui_port_dc_init(void)
{
    dc.frame_buf = NULL;
#if (FB_DIRECTION_ROTATE == 1)
    dc.fb_height = drv_lcd_get_screen_height();
    dc.fb_width = LCD_SECTION_HEIGHT;
#else
    dc.fb_height = LCD_SECTION_HEIGHT;
    dc.fb_width = drv_lcd_get_screen_width();
#endif

    dc.disp_buf_1 = disp_write_buff1_port;
    dc.disp_buf_2 = disp_write_buff2_port;
    dc.bit_depth = drv_lcd_get_pixel_bits();

    dc.screen_width =  drv_lcd_get_screen_width();
    dc.screen_height = drv_lcd_get_screen_height();

    gui_dc_info_register(&dc);
    gui_log("gui_port_dc_init: dc addr is 0x%p, line is %d", &dc, __LINE__);
}


