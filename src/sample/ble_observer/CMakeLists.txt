#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

#all subdirectory are included in ble_observer_private_include_dir
if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_OBSERVER)
HEADER_DIRECTORIES(ble_observer_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(bleObserver PRIVATE ${ble_observer_private_include_dir})

#app preinclude
target_precompile_headers(bleObserver PUBLIC app_observer_flags.h)
#to be removed--ble_audio_lib had include this header file

#sources add
#common sources
file(GLOB_RECURSE  ble_observer_app_srcs  ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

target_sources(bleObserver
				PRIVATE ${ble_observer_app_srcs})

endif()

