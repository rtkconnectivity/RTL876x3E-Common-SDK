#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
add_library(lwipLib STATIC "")
set_target_properties(lwipLib PROPERTIES 
					  ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
					  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
					  OUTPUT_NAME liblwip
					  PREFIX "")


target_link_libraries(lwipLib 
					 PUBLIC romLib 
					 PUBLIC halLib)
					 
target_precompile_headers(lwipLib PUBLIC  ${kconfig_header_dir_path}/config.h)
target_include_directories(lwipLib PUBLIC ${kconfig_header_dir_path})
set(build_target_list ${build_target_list} "lwipLib" CACHE INTERNAL "build target list")

add_custom_command(TARGET lwipLib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lwipLib> ${lib_output_path_temp_dirt}
    COMMENT "Copying lwipLib to ${lib_output_path}/../temp"
)