#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
HEADER_DIRECTORIES(lib_lwip_header_internal_dir ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories(lwipLib ${lwipLib_public_type} ${lib_lwip_header_internal_dir})
target_include_directories(lwipLib ${lwipLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/src/include/lwip")
target_include_directories(lwipLib ${lwipLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/src/include")
target_include_directories(lwipLib ${lwipLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(lwipLib ${lwipLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/src/arch")
target_include_directories(lwipLib ${lwipLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(lwipLib ${lwipLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/netif")


file(GLOB_RECURSE  lib_lwip_source_internal_dir *.c)
file(GLOB_RECURSE  lib_lwip_source_remove_sources  
										   ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/ethernetif.c
										   )
list(REMOVE_ITEM lib_lwip_source_internal_dir ${lib_lwip_source_remove_sources})
target_sources(lwipLib ${lwipLib_private_type} ${lib_lwip_source_internal_dir})

