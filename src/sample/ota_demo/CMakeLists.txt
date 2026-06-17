#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

#all subdirectory are included in ota_demo_private_include_dir
if(NOT compile_lib_only AND CONFIG_REALTEK_APP_OTA_DEMO)
HEADER_DIRECTORIES(ota_demo_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(otaDemo PRIVATE ${ota_demo_private_include_dir})

#app preinclude
target_precompile_headers(otaDemo PUBLIC app_flags.h)

#app sources
file(GLOB_RECURSE ota_demo_app_srcs ./*.c)
target_sources(otaDemo PRIVATE ${ota_demo_app_srcs})
endif()
