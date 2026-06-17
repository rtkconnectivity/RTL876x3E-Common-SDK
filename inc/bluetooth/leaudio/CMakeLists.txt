#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

HEADER_DIRECTORIES(ble_audio_lib_header_dir ./)
target_include_directories(bleAudioLib ${bleAudioLib_public_type} ${ble_audio_lib_header_dir})

target_precompile_headers(bleAudioLib ${bleAudioLib_public_type}  ble_audio_flags.h)