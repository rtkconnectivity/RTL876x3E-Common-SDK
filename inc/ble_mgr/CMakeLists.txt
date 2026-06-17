#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

HEADER_DIRECTORIES(ble_mgr_header_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(bleMgrLib ${bleMgrLib_public_type} ${ble_mgr_header_dir})


