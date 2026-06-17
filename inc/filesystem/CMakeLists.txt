#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
HEADER_DIRECTORIES(filesystem_externel_dir ./)
target_include_directories(filesystemLib ${filesystemLib_public_type} ${filesystem_externel_dir})