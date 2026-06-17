#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

HEADER_DIRECTORIES(lib_sysm_header_externel_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(sysmLib ${sysmLib_public_type} ${lib_sysm_header_externel_dir})