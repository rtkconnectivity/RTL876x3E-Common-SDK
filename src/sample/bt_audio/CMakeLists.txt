#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
 
#all subdirectory are included in bt_audio_private_include_dir
if(NOT compile_lib_only AND CONFIG_REALTEK_BT_AUDIO_APP)
HEADER_DIRECTORIES(bt_audio_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(btAudio PRIVATE ${bt_audio_private_include_dir})

#app preinclude
target_precompile_headers(btAudio PUBLIC app_flags.h)

#app sources
file(GLOB_RECURSE  bt_audio_app_srcs  ./*.c)
target_sources(btAudio PRIVATE ${bt_audio_app_srcs})
endif()
