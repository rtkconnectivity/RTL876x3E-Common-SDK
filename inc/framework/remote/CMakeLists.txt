#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

HEADER_DIRECTORIES(remote_externel_dir ./)
target_include_directories(remoteLib ${remoteLib_public_type} ${remote_externel_dir})

message("---------[LIB]remote_externel_dir :")
foreach(ITEM ${remote_externel_dir})
   message(${ITEM})
endforeach()
message("---------[LIB]remote_externel_dir ------------")
