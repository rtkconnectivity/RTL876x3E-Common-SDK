#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

#all subdirectory are included in ble_broadcaster_private_include_dir
if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_BROADCASTER)
HEADER_DIRECTORIES(ble_broadcaster_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(bleBroadcaster PRIVATE ${ble_broadcaster_private_include_dir})
#app preinclude
target_precompile_headers(bleBroadcaster PUBLIC app_broadcaster_flags.h)
#to be removed--ble_audio_lib had include this header file

#sources add
#common sources
file(GLOB_RECURSE  ble_broadcaster_app_srcs   ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
message(${ble_broadcaster_app_srcs})
target_sources(bleBroadcaster
				PRIVATE ${ble_broadcaster_app_srcs})

endif()

