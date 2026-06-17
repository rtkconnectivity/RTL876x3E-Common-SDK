#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

HEADER_DIRECTORIES(console_externel_dir ./)
target_include_directories(consoleLib ${consoleLib_public_type} ${console_externel_dir})

message("---------[LIB]console_externel_dir :")
foreach(ITEM ${console_externel_dir})
   message(${ITEM})
endforeach()
message("---------[LIB]console_externel_dir ------------")
