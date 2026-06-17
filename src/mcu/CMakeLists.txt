#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
if(CONFIG_REALTEK_SD)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sd_mgr)
endif()

if(CONFIG_REALTEK_FILESYSTEM)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fatfs)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/local_audio)
endif()

if(CONFIG_REALTEK_USB)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/usb/lib)
endif()

if(CONFIG_REALTEK_USB_HAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/usb/hal)
endif()

check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/module)