#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

#app sources
file(GLOB_RECURSE  io_demo_mian  ./main.c)

file(GLOB_RECURSE  io_demo_adc  ./adc/*.c)
file(GLOB_RECURSE  io_demo_adp_1wire  ./adp_1wire/*.c)
file(GLOB_RECURSE  io_demo_captouch  ./captouch/*.c)
file(GLOB_RECURSE  io_demo_dlps    ./dlps/*.c)
file(GLOB_RECURSE  io_demo_gdma    ./gdma/*.c)
file(GLOB_RECURSE  io_demo_gpio    ./gpio/*.c)
file(GLOB_RECURSE  io_demo_i2c     ./i2c/*.c)

file(GLOB_RECURSE  io_demo_i2s     ./i2s/*.c)
file(GLOB_RECURSE  io_demo_i2s_remove_src  ./i2s/master_dsp_control/i2s_master_dsp_control_demo.c)
list(REMOVE_ITEM io_demo_i2s ${io_demo_i2s_remove_src})

file(GLOB_RECURSE  io_demo_ir     ./ir/*.c)
file(GLOB_RECURSE  io_demo_keyscan     ./keyscan/*.c)
file(GLOB_RECURSE  io_demo_led     ./led/*.c)
file(GLOB_RECURSE  io_demo_lpc     ./lpc/*.c)
file(GLOB_RECURSE  io_demo_qdec     ./qdec/*.c)

file(GLOB_RECURSE  io_demo_rtc     ./rtc/*.c)
file(GLOB_RECURSE  io_demo_rtc_remove_src  ./rtc/clock/rtc_clock_demo.c)
list(REMOVE_ITEM io_demo_rtc ${io_demo_rtc_remove_src})

file(GLOB_RECURSE  io_demo_sdio     ./sdio/*.c)

file(GLOB_RECURSE  io_demo_spi     ./spi/*.c)
file(GLOB_RECURSE  io_demo_spi_remove_src  ./spi/spi0_hs/spi0_hs_dma_demo.c)
list(REMOVE_ITEM io_demo_spi ${io_demo_spi_remove_src})

file(GLOB_RECURSE  io_demo_tim     ./tim/*.c)
file(GLOB_RECURSE  io_demo_uart     ./uart/*.c)
target_sources(ioDemo
               PRIVATE ${io_demo_mian}
               PRIVATE ${io_demo_adc}
               PRIVATE ${io_demo_adp_1wire}
               PRIVATE ${io_demo_captouch}
               PRIVATE ${io_demo_dlps}
               PRIVATE ${io_demo_gdma}
               PRIVATE ${io_demo_gpio}
               PRIVATE ${io_demo_i2c}
               PRIVATE ${io_demo_ir}
               PRIVATE ${io_demo_keyscan}
               PRIVATE ${io_demo_led}
               PRIVATE ${io_demo_lpc}
               PRIVATE ${io_demo_qdec}
               PRIVATE ${io_demo_rtc}
               PRIVATE ${io_demo_sdio}
               PRIVATE ${io_demo_spi}
               PRIVATE ${io_demo_tim}
               PRIVATE ${io_demo_i2s}
               PRIVATE ${io_demo_uart})
HEADER_DIRECTORIES(io_demo_app_internal_headers ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ioDemo PRIVATE ${io_demo_app_internal_headers})

