#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
 
#all subdirectory are included in bt_pan_demo_private_include_dir
if(NOT compile_lib_only AND CONFIG_REALTEK_BT_PAN_DEMO_APP)
HEADER_DIRECTORIES(bt_pan_demo_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(btPanDemo PRIVATE ${bt_pan_demo_private_include_dir})

#app preinclude
target_precompile_headers(btPanDemo PUBLIC app_flags.h)

#app sources
file(GLOB_RECURSE  bt_pan_demo_app_srcs  ./*.c)
target_sources(btPanDemo PRIVATE ${bt_pan_demo_app_srcs})
endif()
