#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
if(NOT ${IS_CHECK_FLOW})
#ifconfig compile framework lib
if(CONFIG_REALTEK_FRAMEWORK)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/framework/framework_gcc)
endif()

#ifconfig compile sysm lib
if(CONFIG_REALTEK_SYSM)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/framework/sysm/gcc)
endif()

#ifconfig compile console lib
if(CONFIG_REALTEK_CONSOLE)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/framework/console/gcc)
endif()

#ifconfig compile remote lib 
if(CONFIG_REALTEK_REMOTE)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/framework/remote/gcc)
endif()

#ifconfig compile ble mgr lib
if(CONFIG_REALTEK_BLE_MGR)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_mgr_lib/gcc)
endif()

#ifconfig compile filesystem lib
if(CONFIG_REALTEK_FILESYSTEM)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/filesystem_lib/gcc)
endif()

#ifconfig compile sd lib
if(CONFIG_REALTEK_SD)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sd_lib/gcc)
endif()

#ifconfig compile usb lib
if(CONFIG_REALTEK_USB)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/usb_lib/gcc)
endif()

#ifconfig compile usb hal lib
if(CONFIG_REALTEK_USB_HAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/usb_lib/hal/rtl87x3e/gcc)
endif()

#ifconfig compile gfps lib
if(CONFIG_REALTEK_GFPS)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gfps_lib/gcc)
endif()

#ifconfig compile ble audio lib
if(CONFIG_REALTEK_BLE_AUDIO)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_audio_lib/gcc)
endif()

	if(CONFIG_REALTEK_TEMPLATE)
	    check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib_template_demo/gcc)
	endif()
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_LWIP)
	check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lwip_lib/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_MBEDTLS)
	check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mbedtls_lib/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BT_AUDIO_TRX)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_audio_trx/gcc)
endif()

#for io demo app
if(NOT compile_lib_only AND CONFIG_REALTEK_IO_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/io_demo/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_AUDIO_DEMO)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/audio_demo/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_BT_HID_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_hid_demo/gcc)
endif()

if((NOT compile_lib_only) AND(CONFIG_REALTEK_BT_AUDIO_APP))
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_audio/gcc)
endif()

if(CONFIG_REALTEK_APP_LEA_CAP_ACCEPTOR)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lea_cap_acceptor/gcc)
endif()

if(CONFIG_REALTEK_APP_LEA_CAP_COMMANDER)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lea_cap_commander/gcc)
endif()

if(CONFIG_REALTEK_APP_LEA_CAP_INITIATOR)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lea_cap_initiator/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_BT_IAP_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_iap_demo/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_BT_MAP_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_map_demo/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_BT_PBAP_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_pbap_demo/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_BT_SPP_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_spp_demo/gcc)
endif()

#for usb demo app
if((NOT compile_lib_only) AND(CONFIG_REALTEK_USB_AUDIO_APP OR CONFIG_REALTEK_USB_HID_APP))
    check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/usb_demo/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_OTA_DEMO)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ota_demo/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_BT_PAN_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_pan_demo/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_BROADCASTER)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_broadcaster/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_BT5_CENTRAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_bt5_central/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_BT5_PERIPHERAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_bt5_peripheral/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_CENTRAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_central/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_OBSERVER)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_observer/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_PERIPHERAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_peripheral/gcc)
endif()

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BLE_SCATTERNET)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_scatternet/gcc)
endif()

