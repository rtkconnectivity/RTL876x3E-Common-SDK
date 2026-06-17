#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/app)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble)

if(CONFIG_REALTEK_BLE_MGR AND (NOT ${IS_CHECK_FLOW}))
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_mgr)
endif()

check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/framework)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mcu)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sample)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/service)