#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

#all subdirectory are included in audio_demo_private_include_dir
if(NOT compile_lib_only AND CONFIG_REALTEK_APP_AUDIO_DEMO)
HEADER_DIRECTORIES(audio_demo_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(audioDemo PRIVATE ${audio_demo_private_include_dir})

#app preinclude
target_precompile_headers(audioDemo PUBLIC app_flags.h)

#app sources
file(GLOB_RECURSE audio_demo_app_srcs ./*.c)
target_sources(audioDemo PRIVATE ${audio_demo_app_srcs})
endif()
