#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
if(CONFIG_REALTEK_APP_BT_AUDIO_TRX)
	check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common)
	check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_audio_trx)

	if(CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE)
		check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_audio_tri_dongle)
	endif()
endif()

if(CONFIG_REALTEK_IO_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/io_demo)
endif()

if(CONFIG_REALTEK_APP_BLE_BROADCASTER)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_broadcaster)
endif()

if(CONFIG_REALTEK_APP_BLE_BT5_CENTRAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_bt5_central)
endif()

if(CONFIG_REALTEK_APP_BLE_BT5_PERIPHERAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_bt5_peripheral)
endif()

if(CONFIG_REALTEK_APP_BLE_CENTRAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_central)
endif()

if(CONFIG_REALTEK_APP_BLE_OBSERVER)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_observer)
endif()

if(CONFIG_REALTEK_APP_BLE_PERIPHERAL)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_peripheral)
endif()

if(CONFIG_REALTEK_APP_BLE_SCATTERNET)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ble_scatternet)
endif()

if(CONFIG_REALTEK_APP_AUDIO_DEMO)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/audio_demo)
endif()

if(CONFIG_REALTEK_BT_HID_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_hid_demo)
endif()

if(CONFIG_REALTEK_BT_AUDIO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_audio)
endif()

if(CONFIG_REALTEK_APP_LEA_CAP_ACCEPTOR)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lea_cap_acceptor)
endif()

if(CONFIG_REALTEK_APP_LEA_CAP_COMMANDER)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lea_cap_commander)
endif()

if(CONFIG_REALTEK_APP_LEA_CAP_INITIATOR)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lea_cap_initiator)
endif()

if(CONFIG_REALTEK_BT_IAP_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_iap_demo)
endif()

if(CONFIG_REALTEK_BT_MAP_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_map_demo)
endif()

if(CONFIG_REALTEK_BT_PBAP_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_pbap_demo)
endif()

if(CONFIG_REALTEK_BT_SPP_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_spp_demo)
endif()

if(CONFIG_REALTEK_USB_AUDIO_APP OR CONFIG_REALTEK_USB_HID_APP)
    check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/usb_demo)
endif()
if(CONFIG_REALTEK_APP_OTA_DEMO)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ota_demo)
endif()
if(CONFIG_REALTEK_LWIP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lwip)
endif()

if(CONFIG_REALTEK_MBEDTLS)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mbedtls)
endif()

if(CONFIG_REALTEK_BT_PAN_DEMO_APP)
check_and_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bt_pan_demo)
endif()
