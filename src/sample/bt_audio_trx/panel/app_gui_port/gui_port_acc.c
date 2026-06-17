/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include <draw_img.h>
#include "gui_api.h"

#if (ENABLE_PPE_FUNCTION == 1)
extern void hw_acc_init(void);
extern void hw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect);
#else
extern void sw_acc_init(void);
extern void sw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, gui_rect_t *rect);
#endif

static struct acc_engine acc =
{
#if (ENABLE_PPE_FUNCTION == 1)
    .blit = hw_acc_blit
#else
    .blit = sw_acc_blit
#endif
};


void gui_port_acc_init(void)
{
#if (ENABLE_PPE_FUNCTION == 1)
    hw_acc_init();
    acc.blit = hw_acc_blit;
#else
    sw_acc_init();
    acc.blit = sw_acc_blit;
#endif
    gui_acc_info_register(&acc);
}
