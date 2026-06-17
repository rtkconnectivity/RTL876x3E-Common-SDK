#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

#all subdirectory are included in ble_bt5_central_private_include_dir
if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_BT5_CENTRAL)
HEADER_DIRECTORIES(ble_bt5_central_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(bleBT5Central PRIVATE ${ble_bt5_central_private_include_dir})

#app preinclude
target_precompile_headers(bleBT5Central PUBLIC app_bt5_central_flags.h)
#to be removed--ble_audio_lib had include this header file

#sources add
#common sources
file(GLOB_RECURSE  ble_bt5_central_app_srcs  ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

target_sources(bleBT5Central
				PRIVATE ${ble_bt5_central_app_srcs})

endif()

