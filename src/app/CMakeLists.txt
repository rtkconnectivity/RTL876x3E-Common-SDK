#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
if(CONFIG_REALTEK_BLE_AUDIO)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_audio_lib)
endif()