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
#include "app_dlps.h"

#if ENABLE_PSRAM_FOR_LCD
#if (TARGET_RTL8773DO == 1)
#define DEST_ADDR       FMC_MAIN4_ADDR
#elif (TARGET_RTL87X3EP == 1)
#define PSRAM_BUF1   (uint32_t)FMC_MAIN1_ADDR
#define PSRAM_BUF2   (uint32_t)(FMC_MAIN1_ADDR + LCD_WIDTH * LCD_HIGHT * PIXEL_BYTES)
static uint32_t psram_offset = 0;
static uint8_t *p_psram_addr = (uint8_t *)PSRAM_BUF1;
#else
#define DEST_ADDR       0x04000000 //FMC_MAIN1_ADDR   PSRAM ADDR
#endif
#endif

#define LCD_SECTION_HEIGHT                      10
#if FB_DIRECTION_ROTATE
#define FRAME_BUF_SIZE   (LCD_HIGHT * LCD_SECTION_HEIGHT * PIXEL_BYTES)
#else
#define FRAME_BUF_SIZE   (LCD_WIDTH * LCD_SECTION_HEIGHT * PIXEL_BYTES)
#endif

static uint8_t gui_port_dc_dma_ch_num = 0xa5;
static bool gui_task_work = true;
static bool display_has_installed = false;
#if (ENABLE_PPE_FUNCTION == 1)
#include "section.h"
SHM_DATA_SECTION static uint8_t __attribute__((aligned(4))) disp_write_buff1_port[FRAME_BUF_SIZE];
SHM_DATA_SECTION static uint8_t __attribute__((aligned(4))) disp_write_buff2_port[FRAME_BUF_SIZE];
#else
static uint8_t  *disp_write_buff1_port = NULL;
static uint8_t  *disp_write_buff2_port = NULL;
#endif
#if FB_DATA_ROTATE
static uint8_t  *disp_write_temp_port = NULL;
#endif

#define LCD_DMA_CHANNEL_NUM                     gui_port_dc_dma_ch_num
#define LCD_DMA_CHANNEL_INDEX                   DMA_CH_BASE(gui_port_dc_dma_ch_num)

bool gui_port_dc_lcd_is_work(void)
{
    return gui_task_work;
}

void gui_port_dc_set_dma(uint8_t num)
{
    gui_port_dc_dma_ch_num = num;
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
    gui_log("port_gui_lcd_power_on\n");

    if (app_dlps_check_enter_bits(APP_DLPS_ENTER_CHECK_DISPLAY))
    {
        return;
    }

    if (display_has_installed == false)
    {
        return;
    }
    gui_task_work = true;
#if ENABLE_PSRAM_FOR_LCD
    app_psram_exit_dlps();
#endif
    drv_lcd_power_on();
    /*psram and power manager set*/
    sys_hall_auto_sleep_in_idle(false);

    app_dlps_disable(APP_DLPS_ENTER_CHECK_DISPLAY);
}

void port_gui_lcd_power_off(void)
{
    gui_log("port_gui_lcd_power_off\n");
    if (display_has_installed == false)
    {
        return;
    }

    if (!app_dlps_check_enter_bits(APP_DLPS_ENTER_CHECK_DISPLAY))
    {
        return;
    }

    gui_task_work = false;
#if defined(ENABLE_PSRAM_FOR_LCD) && (TARGET_RTL87X3EP == 1)
    drv_lcd_transfer_done();
#endif

#if ENABLE_PSRAM_FOR_LCD
    app_psram_enter_dlps();
#endif
    drv_lcd_power_off();
    sys_hall_auto_sleep_in_idle(true);

    app_dlps_enable(APP_DLPS_ENTER_CHECK_DISPLAY);
}

#if (ENABLE_PSRAM_FOR_LCD == 1)
#if (TARGET_RTL87X3EP == 1)
static void memcpy_by_dma_start(uint8_t *buf, uint32_t len)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum = LCD_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR = GDMA_DIR_MemoryToMemory;
    GDMA_InitStruct.GDMA_BufferSize = len;
    GDMA_InitStruct.GDMA_SourceInc = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_SourceMsize = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_32;
    GDMA_InitStruct.GDMA_SourceAddr = (uint32_t)buf;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)p_psram_addr + psram_offset * 2;
    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
}

void port_gui_lcd_update_with_psram(struct gui_dispdev *dc)
{
    static bool first_frame_read_te_type = true;
    static T_LCDC_TE_TYPE te_type;

    uint32_t total_section_cnt = ((drv_lcd_get_screen_height() - 1) / LCD_SECTION_HEIGHT + 1);
    if (dc->section_count == 0)
    {
        /*set TE type at each first section*/
        if (first_frame_read_te_type)
        {
            te_type = drv_lcd_get_TE_type();
            first_frame_read_te_type = false;
        }
        drv_lcd_set_TE_type(te_type);

        memcpy_by_dma_start(dc->frame_buf, dc->fb_width * dc->fb_height);
        psram_offset += dc->fb_width * dc->fb_height;
    }
    else if (dc->section_count == total_section_cnt - 1)
    {
        uint32_t last_height = dc->screen_height - dc->section_count * dc->fb_height;
        memcpy_by_dma_done();
        memcpy_by_dma_start(dc->frame_buf, dc->fb_width * last_height);
        psram_offset = 0;
        memcpy_by_dma_done();

        if (te_type == LCDC_TE_TYPE_SW_TE)
        {
            drv_lcd_te_enable();
            while (!drv_lcd_get_te_trigger_state());
            drv_lcd_set_te_trigger_state(false);
            drv_lcd_te_disable();
        }
        drv_lcd_transfer_done();
        drv_lcd_set_window(0, 0, dc->screen_width, dc->screen_height);
        drv_lcd_start_transfer(p_psram_addr, dc->screen_width * dc->screen_height);

        if ((uint32_t)p_psram_addr == PSRAM_BUF1)
        {
            p_psram_addr = (uint8_t *) PSRAM_BUF2;
        }
        else
        {
            p_psram_addr = (uint8_t *) PSRAM_BUF1;
        }
    }
    else
    {
        memcpy_by_dma_done();
        memcpy_by_dma_start(dc->frame_buf, dc->fb_width * dc->fb_height);
        psram_offset += dc->fb_width * dc->fb_height;
    }
}
#else
static void memcpy_by_dma_start(void *dst, void *src, uint32_t size)
{
    GUI_ASSERT((uint32_t)dst % 4 == 0);
    GUI_ASSERT((uint32_t)src % 4 == 0);
    GUI_ASSERT(size % 4 == 0);

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum = LCD_DMA_CHANNEL_NUM;
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

void port_gui_lcd_update_with_psram(struct gui_dispdev *dc)
{
    uint32_t total_section_cnt = 0;
    uint32_t last_height = 0;
    uint32_t last_width = 0;
#if (ENABLE_TE_FOR_LCD == 1)
    static bool first_frame_read_te_type = true;
    static T_LCDC_TE_TYPE te_type;
#endif

    total_section_cnt = ((drv_lcd_get_screen_height() - 1) / LCD_SECTION_HEIGHT + 1);

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
}
#endif
#endif

#if (ENABLE_PSRAM_FOR_LCD == 0)
#if (FB_DATA_ROTATE == 1)
void rotate_pixels(uint8_t *src_buf, uint8_t *dest_buf, int width, int height)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            uint8_t *src_pixel = src_buf + ((y * width + x) * 2);
            uint8_t *dest_pixel = dest_buf + (((width - 1 - x) * height + y) * 2);

            dest_pixel[0] = src_pixel[0];
            dest_pixel[1] = src_pixel[1];
            dest_pixel[2] = src_pixel[2];
        }
    }
}

void port_gui_lcd_update_with_fb_data_rotate(struct gui_dispdev *dc)
{
#if (ENABLE_TE_FOR_LCD == 1)
    static bool first_frame_read_te_type = true;
    static T_LCDC_TE_TYPE te_type;
#endif

    //uint32_t total_section_cnt = ((drv_lcd_get_screen_height() - 1) / LCD_SECTION_HEIGHT + 1);

    if (dc->section_count == 0)
    {
#if (ENABLE_TE_FOR_LCD == 1)
        /*set TE type at each first section*/
        if (first_frame_read_te_type)
        {
            te_type = drv_lcd_get_TE_type();
            first_frame_read_te_type = false;
        }
        drv_lcd_set_TE_type(te_type);

        /*if TE type is software without psram, then wait here for software TE signal*/
        if (te_type == LCDC_TE_TYPE_SW_TE)
        {
            drv_lcd_te_enable();
            while (!drv_lcd_get_te_trigger_state());
            drv_lcd_set_te_trigger_state(false);
            drv_lcd_te_disable();
        }
#endif

        rotate_pixels(dc->frame_buf, disp_write_temp_port, dc->section.x2 - dc->section.x1 + 1,
                      dc->section.y2 - dc->section.y1 + 1);
        dc->frame_buf = disp_write_temp_port;
        drv_lcd_set_window(dc->section.y1, dc->screen_width - LCD_SECTION_HEIGHT,
                           dc->section.y2 - dc->section.y1 + 1, dc->section.x2 - dc->section.x1 + 1);
        drv_lcd_start_transfer(dc->frame_buf, dc->fb_width * dc->fb_height);
    }
    else if (dc->section_count == dc->section_total - 1)
    {
        drv_lcd_transfer_done();
        rotate_pixels(dc->frame_buf, disp_write_temp_port, dc->section.x2 - dc->section.x1 + 1,
                      dc->section.y2 - dc->section.y1 + 1);
        dc->frame_buf = disp_write_temp_port;
        drv_lcd_set_window(dc->section.y1, 0, dc->section.y2 - dc->section.y1 + 1,
                           dc->section.x2 - dc->section.x1 + 1);
#if (ENABLE_TE_FOR_LCD == 1)
        drv_lcd_set_TE_type(LCDC_TE_TYPE_NO_TE);
#endif
        drv_lcd_start_transfer(dc->frame_buf, (dc->section.x2 - dc->section.x1 + 1) * dc->fb_height);
        drv_lcd_transfer_done();
    }
    else
    {
        drv_lcd_transfer_done();
        rotate_pixels(dc->frame_buf, disp_write_temp_port, dc->section.x2 - dc->section.x1 + 1,
                      dc->section.y2 - dc->section.y1 + 1);
        dc->frame_buf = disp_write_temp_port;
        drv_lcd_set_window(dc->section.y1,
                           dc->screen_width - LCD_SECTION_HEIGHT - dc->section_count * (dc->section.x2 - dc->section.x1 + 1), \
                           dc->section.y2 - dc->section.y1 + 1, dc->section.x2 - dc->section.x1 + 1);
#if (ENABLE_TE_FOR_LCD == 1)
        drv_lcd_set_TE_type(LCDC_TE_TYPE_NO_TE);
#endif
        drv_lcd_start_transfer(dc->frame_buf, dc->fb_width * dc->fb_height);
    }
}
#else
void port_gui_lcd_update(struct gui_dispdev *dc)
{
    uint32_t total_section_cnt = 0;
    uint32_t last_height = 0;
    uint32_t last_width = 0;
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

    if (dc->section_count == 0)
    {
#if (ENABLE_TE_FOR_LCD == 1)
        /*set TE type at each first section*/
        if (first_frame_read_te_type)
        {
            te_type = drv_lcd_get_TE_type();
            first_frame_read_te_type = false;
        }
        drv_lcd_set_TE_type(te_type);

        /*if TE type is software without psram, then wait here for software TE signal*/
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
            last_height = dc->screen_height - dc->section_count * dc->fb_height;
            drv_lcd_set_window(0, dc->fb_height * dc->section_count, dc->fb_width, last_height);
            drv_lcd_start_transfer(dc->frame_buf, dc->fb_width * last_height);
        }
        else if (dc->pfb_type == PFB_X_DIRECTION)
        {
            last_width = dc->screen_width - dc->section_count * dc->fb_width;
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
}
#endif
#endif

static struct gui_dispdev dc =
{
    .type = DC_RAMLESS,
    .section = {0, 0, 0, 0},
    .section_count = 0,

#if (ENABLE_PSRAM_FOR_LCD == 1)
    .lcd_update = port_gui_lcd_update_with_psram,
#else
#if (FB_DATA_ROTATE == 1)
    .lcd_update = port_gui_lcd_update_with_fb_data_rotate,
#else
    .lcd_update = port_gui_lcd_update,
#endif
#endif
    .lcd_power_on = port_gui_lcd_power_on,
    .lcd_power_off = port_gui_lcd_power_off,

    .flash_seq_trans_disable = NULL,
    .flash_seq_trans_enable = NULL,
    .reset_lcd_timer = NULL,
    .get_lcd_us = NULL,

    .lcd_te_wait = NULL,

};

void gui_port_dc_fb_init(void)
{
#if (ENABLE_PPE_FUNCTION == 0)
    disp_write_buff1_port = calloc(FRAME_BUF_SIZE, 1);
    disp_write_buff2_port = calloc(FRAME_BUF_SIZE, 1);
#endif

#if FB_DATA_ROTATE
    disp_write_temp_port = calloc(FRAME_BUF_SIZE, 1);
    if (!disp_write_temp_port)
    {
        gui_log("gui_port_dc_fb_init: temp fb malloc failed ");
    }
#endif
    if (!disp_write_buff1_port || !disp_write_buff2_port)
    {
        gui_log("gui_port_dc_fb_init: fb malloc failed ");
    }
}

void gui_port_dc_init(void)
{
    dc.frame_buf = NULL;
#if FB_DIRECTION_ROTATE
    dc.fb_height = drv_lcd_get_screen_height();
    dc.fb_width = LCD_SECTION_HEIGHT;
#else
    dc.fb_height = LCD_SECTION_HEIGHT;
    dc.fb_width = drv_lcd_get_screen_width();
#endif

    app_dlps_disable(APP_DLPS_ENTER_CHECK_DISPLAY);
    gui_port_dc_fb_init();

    dc.disp_buf_1 = disp_write_buff1_port;
    dc.disp_buf_2 = disp_write_buff2_port;
    dc.bit_depth = drv_lcd_get_pixel_bits();

    dc.screen_width =  drv_lcd_get_screen_width();
    dc.screen_height = drv_lcd_get_screen_height();

    gui_dc_info_register(&dc);
    display_has_installed = true;
    gui_log("gui_port_dc_init ");
    gui_log("dc addr is 0x%p, line is %d", &dc, __LINE__);
    sys_hall_auto_sleep_in_idle(false);
}


