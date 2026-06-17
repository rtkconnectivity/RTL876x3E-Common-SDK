#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
cmake_minimum_required (VERSION 3.5)

#include cmake module 
include(./cmake/build.cmake)
include(./cmake/compile.cmake)
include(./cmake/version_generate.cmake)

#for window mingw32 makefile: avoid make error
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_SYSTEM_NAME Generic)

#how to set project name
project(bumblebeeSDK)

set(compile_lib ON)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../hal)
set(compile_lib OFF)
if(${CONFIG_REALTEK_HAL})
add_subdirectory(../ realtek_hal)
endif()
endif()

check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bin)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/board/evb)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/inc)

