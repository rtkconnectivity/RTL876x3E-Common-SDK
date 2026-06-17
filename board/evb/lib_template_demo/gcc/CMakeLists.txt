#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
#Set lib target
add_library(templateDemoLib STATIC "")
set_target_properties(templateDemoLib PROPERTIES 
					  ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
					  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
					  OUTPUT_NAME ${CONFIG_REALTEK_SYSM_TARGET_NAME}
					  PREFIX ""
					  )
					  
#set(Sysm_Version_Path "${REALTEK_SDK_BASE_PATH}/src/framework/sysm/sys_ver.h")
#generate_version("sysm" ${Sysm_Version_Path})

#Set the dependency library
target_link_libraries(templateDemoLib 
					 PUBLIC romLib 
					 PUBLIC halLib)
#Set preinclude headers					 
target_precompile_headers(templateDemoLib PUBLIC  ${kconfig_header_dir_path}/config.h)

#Set include headers folder	
target_include_directories(templateDemoLib PUBLIC ${kconfig_header_dir_path})
set(build_target_list ${build_target_list} "sysmLib" CACHE INTERNAL "build target list")

#Set lib output folder	
add_custom_command(TARGET sysmLib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sysmLib> ${lib_output_path_temp_dirt}
    COMMENT "Copying sysmLib to ${lib_output_path}/../temp"
)
