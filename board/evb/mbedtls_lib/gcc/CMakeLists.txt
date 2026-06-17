#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
add_library(mbedtlsLib STATIC "")
set_target_properties(mbedtlsLib PROPERTIES 
					  ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
					  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
					  OUTPUT_NAME libmbedtls
					  PREFIX "")


target_link_libraries(mbedtlsLib 
					 PUBLIC romLib 
					 PUBLIC halLib)
					 
target_precompile_headers(mbedtlsLib PUBLIC  ${kconfig_header_dir_path}/config.h)
target_include_directories(mbedtlsLib PUBLIC ${kconfig_header_dir_path})
set(build_target_list ${build_target_list} "mbedtlsLib" CACHE INTERNAL "build target list")

add_custom_command(TARGET mbedtlsLib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mbedtlsLib> ${lib_output_path_temp_dirt}
    COMMENT "Copying mbedtlsLib to ${lib_output_path}/../temp"
)