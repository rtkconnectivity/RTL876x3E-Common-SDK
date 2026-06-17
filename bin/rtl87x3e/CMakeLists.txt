#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
message(STATUS "[library] start to add uncompiled library")

#FOR ROM LIB
find_library(
    lib_rom
    NAMES librom.a
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH
)

set(lib_rom  ${lib_rom} CACHE INTERNAL "for rom lib link")
message("rom lib path: ${lib_rom}")
add_library(romLib INTERFACE "")
target_include_directories(romLib INTERFACE ./rom)

#FOR UPPERSTACK LIB
find_library(
    lib_upperstack
    NAMES libupperstack_4M.a
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH
)
# find_library(lib_upperstack_4M NAME upperstack_4M.a
							   # PATH  ./gcc_lib
			# )
set(lib_upperstack  ${lib_upperstack} CACHE INTERNAL "for upperstack lib link")
message("upperstack lib path: ${lib_upperstack}")

add_library(upperstackLib INTERFACE "")
target_include_directories(upperstackLib INTERFACE ./rom)
target_include_directories(upperstackLib INTERFACE ./upperstack_stamp)
#FOR GAP LIB
find_library(
    lib_gap
    NAMES libgap_4M.a
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH
)
# find_library(lib_gap_4M NAME gap_4M.a
						# PATH  ./gcc_lib
			# )
set(lib_gap  ${lib_gap} CACHE INTERNAL "for gap lib link")
message("gap lib path: ${lib_gap}")
add_library(gapLib INTERFACE "")

#to decide weather lib to compile or only to link
if(NOT compile_lib_only)
	if(CONFIG_REALTEK_HAL)
		find_exist_library(halLib ${CONFIG_REALTEK_HAL_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_HAL})
	endif()

	if(CONFIG_REALTEK_BLE_AUDIO)
		find_exist_library(bleAudioLib ${CONFIG_REALTEK_BLE_AUDIO_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_BLE_AUDIO})
	endif()

	if(CONFIG_REALTEK_BLE_MGR)
		find_exist_library(bleMgrLib ${CONFIG_REALTEK_BLE_MGR_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_BLE_MGR})
	endif()

	if(CONFIG_REALTEK_FRAMEWORK)
		find_exist_library(frameworkLib ${CONFIG_REALTEK_FRAMEWORK_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_FRAMEWORK})
	endif()
	
	if(CONFIG_REALTEK_ENGAGE)
		find_exist_library(engageLib ${CONFIG_REALTEK_ENGAGE_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_ENGAGE})
	endif()
	
	if(CONFIG_REALTEK_CONSOLE)
		find_exist_library(consoleLib ${CONFIG_REALTEK_CONSOLE_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_CONSOLE})
	endif()

	if(CONFIG_REALTEK_REMOTE)
		find_exist_library(remoteLib  ${CONFIG_REALTEK_REMOTE_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_REMOTE})
	endif()

	if(CONFIG_REALTEK_SYSM)
		find_exist_library(sysmLib ${CONFIG_REALTEK_SYSM_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_SYSM})
	endif()

	if(CONFIG_REALTEK_GFPS)
		find_exist_library(gfpsLib ${CONFIG_REALTEK_GFPS_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_GFPS})
	endif()

	if(CONFIG_REALTEK_FILESYSTEM)
		find_exist_library(filesystemLib ${CONFIG_REALTEK_FILESYSTEM_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_FILESYSTEM})
	endif()

	if(CONFIG_REALTEK_SD)
		find_exist_library(sdLib ${CONFIG_REALTEK_SD_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_SD})
	endif()

	if(CONFIG_REALTEK_USB)
		find_exist_library(usbLib ${CONFIG_REALTEK_USB_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_USB})
	endif()

	if(CONFIG_REALTEK_USB_HAL)
		find_exist_library(usbHalLib ${CONFIG_REALTEK_USB_HAL_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_USB_HAL})
	endif()
	
	find_exist_library(lwipLib liblwip ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc true)
	find_exist_library(mbedtlsLib libmbedtls ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc true)

else()
find_exist_library(halLib CONFIG_REALTEK_HAL_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_HAL})
find_exist_library(bleAudioLib CONFIG_REALTEK_BLE_AUDIO_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_BLE_AUDIO})
find_exist_library(bleMgrLib CONFIG_REALTEK_BLE_MGR_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_BLE_MGR})
find_exist_library(engageLib CONFIG_REALTEK_ENGAGE_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_ENGAGE})
find_exist_library(frameworkLib CONFIG_REALTEK_FRAMEWORK_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_FRAMEWORK})
find_exist_library(consoleLib CONFIG_REALTEK_CONSOLE_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_CONSOLE})
find_exist_library(remoteLib  CONFIG_REALTEK_REMOTE_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_REMOTE})
find_exist_library(sysmLib CONFIG_REALTEK_SYSM_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_SYSM})
find_exist_library(gfpsLib CONFIG_REALTEK_GFPS_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_GFPS})
find_exist_library(filesystemLib CONFIG_REALTEK_FILESYSTEM_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_FILESYSTEM})
find_exist_library(sdLib CONFIG_REALTEK_SD_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_SD})
find_exist_library(usbLib CONFIG_REALTEK_USB_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_USB})
find_exist_library(usbHalLib CONFIG_REALTEK_USB_HAL_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_USB_HAL})
find_exist_library(lwipLib liblwip ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_LWIP})
find_exist_library(mbedtlsLib libmbedtls ${CMAKE_CURRENT_SOURCE_DIR}/lib_gcc ${CONFIG_REALTEK_MBEDTLS})
endif()
message(STATUS "[library] add library done")