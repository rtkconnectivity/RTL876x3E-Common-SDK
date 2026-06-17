#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
HEADER_DIRECTORIES(usb_hal_inc_header_dir ./)
target_include_directories(usbHalLib ${usbHalLib_private_type} ${usb_hal_inc_header_dir})