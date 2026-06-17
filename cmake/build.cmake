#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
#Get ic info from kconfig/cmake build command
string(REGEX MATCH "RTL8773E" CONFIG_RTL87X3EP "${kconfig_path}")
string(REGEX MATCH "RTL8763E" CONFIG_RTL87X3E "${kconfig_path}")
string(REGEX MATCH "RTL8773D" CONFIG_RTL87X3D "${kconfig_path}")

#Set bank info from kconfig/cmake build command
string(FIND "${kconfig_path}" "_" last_underscore_pos REVERSE)
math(EXPR start_pos "${last_underscore_pos} + 1")
string(SUBSTRING "${kconfig_path}" ${start_pos} -1 build_bank)
message(STATUS "[BUILD] Build bank: ${build_bank}")

#set lib out put directory
if(CONFIG_RTL87X3E)
set(lib_output_path ${CMAKE_CURRENT_SOURCE_DIR}/bin/rtl87x3e/lib_gcc)
message("Lib output path: ${lib_output_path}")
set(lib_output_path_temp_dirt ${lib_output_path}/../temp)
add_definitions(-D IC_TYPE_RTL87X3E)
file(MAKE_DIRECTORY ${lib_output_path_temp_dirt})
endif()

set(REALTEK_SDK_BASE_PATH1 "${CMAKE_CURRENT_SOURCE_DIR}")
set(REALTEK_SDK_BASE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "SDK base path ${REALTEK_SDK_BASE_PATH}")

#set path for app_msg.h ext
set(sdk_app_dir_path ${CMAKE_CURRENT_SOURCE_DIR}/inc/app)
message("app_dir_path  ${sdk_app_dir_path}")

#set boot src
set(realtek_start_up_boot_file ${CMAKE_CURRENT_SOURCE_DIR}/src/mcu/rtl87x3/system_rtl87x3.c)

if(CONFIG_RTL87X3E)
set(arm_start_up_boot_file ${CMAKE_CURRENT_SOURCE_DIR}/src/mcu/rtl87x3/arm/startup_rtl87x3e_gcc.s)
endif()


################kconfig setup#######################
#prebuild scripts for kconconfig
string(REGEX REPLACE "\\\\" "/" kconfig_path ${kconfig_path})
string(REGEX REPLACE "/defconfig.*" "" app_path ${kconfig_path})
string(REGEX REPLACE "${app_path}/defconfig." "" bin_output_path ${kconfig_path})

set(gen_dir ${CMAKE_CURRENT_BINARY_DIR}/include/generated)
file(MAKE_DIRECTORY ${gen_dir})

if(${compile_lib_only})
set(general_kconfig_path ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)
message("compile lib")
else()
set(general_kconfig_path ${app_path}/../Kconfig)


endif()

find_program(Python_EXECUTABLE python NO_CMAKE_FIND_ROOTO_PATH) 
set(Python_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/tool/Gadgets/gcc_tool/kconfig.py ${general_kconfig_path} ${gen_dir}/.config ${gen_dir}/config.h ${gen_dir}/config.cmake ${gen_dir}/sourcelist.txt ${kconfig_path}")
STRING(REPLACE " " ";" Python_ARGS ${Python_ARGS})
message("Python_EXECUTABLE: ${Python_EXECUTABLE}")
message("Python_ARGS: ${Python_ARGS}")
execute_process(
  COMMAND ${Python_EXECUTABLE} ${Python_ARGS}
)
include(${gen_dir}/config.cmake)

#set config.h path
set(kconfig_header_dir_path "${gen_dir}")
message("KCONFIG_PATH ${kconfig_path}")
message("app_path ${app_path}")

#flash map for app
set(CURRENT_FLASH_MAP_FULL_PATH "${REALTEK_SDK_BASE_PATH}/${CONFIG_REALTEK_FLASH_MAP_PATH}")
include_directories("${CURRENT_FLASH_MAP_FULL_PATH}")

message("flash map path: ${CURRENT_FLASH_MAP_FULL_PATH}")


#set lib out put directory
if(CONFIG_RTL87X3E)
set(lib_output_path ${CMAKE_CURRENT_SOURCE_DIR}/bin/rtl87x3e/lib_gcc)
message(STATUS "Lib output path: ${lib_output_path}")
endif()

################compile function and macro#######################

MACRO(HEADER_DIRECTORIES return_list search_dict)
    FILE(GLOB_RECURSE new_list ${search_dict}/*.h)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

MACRO(set_library_flag LIBRARY_NAME LIBRARY_PATH IS_COMPILE)
	if(((EXISTS ${LIBRARY_PATH}) AND ${IS_COMPILE}) AND ( NOT ${IS_CHECK_FLOW}))
		set(${LIBRARY_NAME}_exist ON )
		set(${LIBRARY_NAME}_private_type PRIVATE)
		set(${LIBRARY_NAME}_public_type PUBLIC )
		message(DEBUG "${LIBRARY_NAME} exist!")
	else()
		set(${LIBRARY_NAME}_exist OFF )
		set(${LIBRARY_NAME}_private_type INTERFACE)
		set(${LIBRARY_NAME}_public_type INTERFACE )
		message(DEBUG "${LIBRARY_NAME} do not exist!")
                if(NOT "${LIBRARY_NAME}" STREQUAL "mbedtlsLib" AND
                   NOT "${LIBRARY_NAME}" STREQUAL "lwipLib")
                    add_library(${LIBRARY_NAME} INTERFACE "")
				else()
					set(${LIBRARY_NAME}_private_type PRIVATE)
					set(${LIBRARY_NAME}_public_type PUBLIC )
                endif()
	endif()
ENDMACRO()

MACRO(find_exist_library TARGET_NAME LIBRARY_NAME LIBRARY_PATH IS_LINK)
# to avoid cmake compile error
	if(${IS_LINK})
		find_library(
		${TARGET_NAME}_link
		NAMES ${LIBRARY_NAME}.a
		PATHS ${LIBRARY_PATH}
		PATH_SUFFIXES lib
		NO_DEFAULT_PATH)
		set(${TARGET_NAME}_link ${TARGET_NAME})
		message("${TARGET_NAME} is linked name:${TARGET_NAME}_link ${${TARGET_NAME}_link}")
	else()
		set(${TARGET_NAME}_link ${TARGET_NAME})
		message("${TARGET_NAME} is not linked name:${TARGET_NAME}_link")
	endif()
ENDMACRO()

MACRO(check_and_add_subdirectory  DIRECTORY_PATH)
	if(EXISTS ${DIRECTORY_PATH})
		add_subdirectory(${DIRECTORY_PATH})
	endif()
ENDMACRO()

#FOR APP TARGET
#OR name: app_target_source
MACRO(check_and_target_sources  TARGET_NAME  TARGET_TYPE TARGET_SOURCE)
	if(TARGET  ${TARGET_NAME})
		target_sources( ${TARGET_NAME} ${TARGET_TYPE} ${TARGET_SOURCE})
	endif()
ENDMACRO()

#FOR APP TARGET
#OR name: app_target_include_directory
MACRO(check_and_target_include_directories TARGET_NAME  TARGET_TYPE TARGET_INCLUDE_DIRECTORY)
	if(TARGET ${TARGET_NAME})
		target_include_directories(${TARGET_NAME} ${TARGET_TYPE} ${TARGET_INCLUDE_DIRECTORY})
	endif()
ENDMACRO()

# preprocess app.ld, then set the output as app link option
MACRO(generate_ldscript ld_path TARGET_NAME)
    get_target_property(app_includes ${TARGET_NAME} INCLUDE_DIRECTORIES)

    add_custom_command(
        TARGET ${TARGET_NAME} PRE_LINK
        COMMAND ${CMAKE_C_COMPILER} -E -P -x c -iquote "${kconfig_header_dir_path}"
          -iquote "${CURRENT_FLASH_MAP_FULL_PATH}" ${ld_path} -o ${CMAKE_CURRENT_BINARY_DIR}/app.ld
            COMMENT "Generating link script"
            )
        target_link_options(${TARGET_NAME} PRIVATE 
            -T ${CMAKE_CURRENT_BINARY_DIR}/app.ld
            )
ENDMACRO()

################compile lib flag#######################
#avoid unset variable build error for cmake when  macro set_library_flag and check_and_target_include_directories was called
set(lib_target_list "HAL;USB;BLE_AUDIO;BLE_MGR;ENGAGE;BTM;AM_AUDIO;CONSOLE;REMOTE;SYSM;GFPS;FILESYSTEM;SD;USB_HAL;MALLEUS;FRAMEWORK;MBEDTLS;LWIP")
set(build_target_list "" CACHE INTERNAL "build target list")
FOREACH(target IN LISTS lib_target_list)
	if(NOT CONFIG_REALTEK_${target})
		set(CONFIG_REALTEK_${target} OFF)
	endif()
endforeach()

#para 0: LIBRARY_NAME #para 1:LIBRARY_PATH
set_library_flag(halLib ${REALTEK_SDK_BASE_PATH}/../hal ${CONFIG_REALTEK_HAL})
set_library_flag(bleAudioLib "${REALTEK_SDK_BASE_PATH}/board/evb/ble_audio_lib/gcc" ${CONFIG_REALTEK_BLE_AUDIO})
set_library_flag(bleMgrLib "${REALTEK_SDK_BASE_PATH}/board/evb/ble_mgr_lib/gcc" ${CONFIG_REALTEK_BLE_MGR})
set_library_flag(engageLib "${REALTEK_SDK_BASE_PATH}/board/evb/engage_lib/gcc" ${CONFIG_REALTEK_ENGAGE})
set_library_flag(frameworkLib "${REALTEK_SDK_BASE_PATH}/board/evb/framework/framework_gcc" ${CONFIG_REALTEK_FRAMEWORK})
set_library_flag(consoleLib "${REALTEK_SDK_BASE_PATH}/board/evb/framework/console/gcc" ${CONFIG_REALTEK_CONSOLE})
set_library_flag(remoteLib "${REALTEK_SDK_BASE_PATH}/board/evb/framework/remote/gcc" ${CONFIG_REALTEK_REMOTE})
set_library_flag(sysmLib "${REALTEK_SDK_BASE_PATH}/board/evb/framework/sysm/gcc" ${CONFIG_REALTEK_SYSM})
set_library_flag(gfpsLib "${REALTEK_SDK_BASE_PATH}/board/evb/gfps_lib/gcc" ${CONFIG_REALTEK_GFPS} )
set_library_flag(filesystemLib "${REALTEK_SDK_BASE_PATH}/board/evb/filesystem_lib/gcc" ${CONFIG_REALTEK_FILESYSTEM})
set_library_flag(sdLib "${REALTEK_SDK_BASE_PATH}/board/evb/sd_lib/gcc" ${CONFIG_REALTEK_SD})
set_library_flag(usbLib "${REALTEK_SDK_BASE_PATH}/board/evb/usb_lib/gcc" ${CONFIG_REALTEK_USB})
set_library_flag(usbHalLib "${REALTEK_SDK_BASE_PATH}/board/evb/usb_lib/hal/rtl87x3e/gcc" ${CONFIG_REALTEK_USB_HAL})
set_library_flag(lwipLib "${REALTEK_SDK_BASE_PATH}/board/evb/lwip_lib" ${CONFIG_REALTEK_LWIP})
set_library_flag(mbedtlsLib "${REALTEK_SDK_BASE_PATH}/board/evb/mbedtls_lib" ${CONFIG_REALTEK_MBEDTLS})



