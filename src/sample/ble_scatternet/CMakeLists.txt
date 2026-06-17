#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

#all subdirectory are included in ble_scatternet_private_include_dir
if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_SCATTERNET)
HEADER_DIRECTORIES(ble_scatternet_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(bleScatternet PRIVATE ${ble_scatternet_private_include_dir})

#app preinclude
target_precompile_headers(bleScatternet PUBLIC app_scatternet_flags.h)
#to be removed--ble_audio_lib had include this header file

#sources add
#common sources
file(GLOB_RECURSE  ble_scatternet_app_srcs  ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

target_sources(bleScatternet
				PRIVATE ${ble_scatternet_app_srcs})

endif()

