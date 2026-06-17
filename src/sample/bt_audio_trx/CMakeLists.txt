#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
#all subdirectory are included in bt_audio_trx_private_include_dir
if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BT_AUDIO_TRX)
HEADER_DIRECTORIES(bt_audio_trx_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(btAudioTrx PRIVATE ${bt_audio_trx_private_include_dir})

#app preinclude
target_precompile_headers(btAudioTrx PUBLIC app/app_flags.h)
target_precompile_headers(btAudioTrx PUBLIC app/app_flags_customer.h)
target_precompile_headers(btAudioTrx PUBLIC app/app_lea_flags.h)
#to be removed--ble_audio_lib had include this header file
target_precompile_headers(btAudioTrx PUBLIC ./../../../inc/bluetooth/leaudio/ble_audio_flags.h)

#sources add
#common sources
file(GLOB_RECURSE  bt_audio_trx_app_srcs  ./app/*.c)
file(GLOB_RECURSE  bt_audio_trx_app_customer_srcs  ./app_customer/*.c)
file(GLOB_RECURSE  bt_audio_trx_ble_srcs  ./ble/*.c)
file(GLOB_RECURSE  bt_audio_trx_bond_srcs  ./bond/*.c)
file(GLOB_RECURSE  bt_audio_trx_bqb_srcs  ./bqb/*.c)
file(GLOB_RECURSE  bt_audio_trx_bredr_srcs  ./bredr/*.c)
file(GLOB_RECURSE  bt_audio_trx_bt_aduio_srcs  ./bt_audio/*.c)
file(GLOB_RECURSE  bt_audio_trx_bt_common_srcs  ./bt_common/*.c)
file(GLOB_RECURSE  bt_audio_trx_cmd_srcs  ./cmd/*.c)

file(GLOB_RECURSE  bt_audio_trx_findmy_srcs  ./findmy/*.c)
file(GLOB_RECURSE  bt_audio_trx_findmy_remove_srcs  ./findmy/fmna/UARP/*.c
                                                    ./findmy/ble_profile/firmware_update_service.c)
list(REMOVE_ITEM   bt_audio_trx_findmy_srcs ${bt_audio_trx_findmy_remove_srcs})

file(GLOB_RECURSE  bt_audio_trx_gfps_srcs  ./gfps/*.c)
file(GLOB_RECURSE  bt_audio_trx_io_srcs  ./io/*.c)
#file(GLOB_RECURSE  bt_audio_trx_ipc_srcs  ./ipc/*.c)
file(GLOB_RECURSE  bt_audio_trx_lea_snk_srcs  ./lea_snk/*.c)
file(GLOB_RECURSE  bt_audio_trx_lea_snk_remove_sources  ./app_lea_distribution.c)
list(REMOVE_ITEM bt_audio_trx_lea_snk_srcs ${bt_audio_trx_lea_snk_remove_sources})


file(GLOB_RECURSE  bt_audio_trx_lea_src_srcs  ./lea_src/*.c)
file(GLOB_RECURSE  bt_audio_trx_module_srcs  ./module/*.c)
file(GLOB_RECURSE  bt_audio_trx_mp_srcs  ./mp/*.c)
file(GLOB_RECURSE  bt_audio_trx_music_playback_srcs  ./music_playback/*.c)
file(GLOB_RECURSE  bt_audio_trx_ota_srcs  ./ota/*.c)
file(GLOB_RECURSE  bt_audio_trx_power_srcs  ./power/*.c)
file(GLOB_RECURSE  bt_audio_trx_smart_pa_srcs  ./smart_pa/*.c)
file(GLOB_RECURSE  bt_audio_trx_source_play_srcs  ./source_play/*.c)
file(GLOB_RECURSE  bt_audio_trx_test_srcs  ./test/*.c)
file(GLOB_RECURSE  bt_audio_trx_transfer_path_srcs  ./transfer_path/*.c)
file(GLOB_RECURSE  bt_audio_trx_uart_srcs  ./uart/*.c)
file(GLOB_RECURSE  bt_audio_trx_uart_dfu_srcs  ./uart_dfu/*.c)
file(GLOB_RECURSE  bt_audio_trx_usb_srcs  ./usb/*.c)

target_sources(btAudioTrx 
				PRIVATE ${bt_audio_trx_app_srcs}
				PRIVATE ${bt_audio_trx_app_customer_srcs}
				PRIVATE ${bt_audio_trx_ble_srcs}
				PRIVATE ${bt_audio_trx_bond_srcs}
				PRIVATE ${bt_audio_trx_bqb_srcs}
				PRIVATE ${bt_audio_trx_bredr_srcs}
				PRIVATE ${bt_audio_trx_bt_aduio_srcs}
				PRIVATE ${bt_audio_trx_bt_common_srcs}
				PRIVATE ${bt_audio_trx_cmd_srcs}
				PRIVATE ${bt_audio_trx_findmy_srcs}
				PRIVATE ${bt_audio_trx_gfps_srcs}
				PRIVATE ${bt_audio_trx_io_srcs}
				PRIVATE ${bt_audio_trx_ipc_srcs}
				PRIVATE ${bt_audio_trx_lea_snk_srcs}
				PRIVATE ${bt_audio_trx_lea_src_srcs}
				PRIVATE ${bt_audio_trx_module_srcs}
				PRIVATE ${bt_audio_trx_mp_srcs}
				PRIVATE ${bt_audio_trx_music_playback_srcs}
				PRIVATE ${bt_audio_trx_ota_srcs}
				PRIVATE ${bt_audio_trx_power_srcs}
				PRIVATE ${bt_audio_trx_smart_pa_srcs}
				PRIVATE ${bt_audio_trx_source_play_srcs}
				PRIVATE ${bt_audio_trx_test_srcs}
				PRIVATE ${bt_audio_trx_transfer_path_srcs}
				PRIVATE ${bt_audio_trx_uart_srcs}
				PRIVATE ${bt_audio_trx_uart_dfu_srcs}
				PRIVATE ${bt_audio_trx_usb_srcs})
endif()
#gui target source
if(CONFIG_REALTEK_FLASH_MAP_16M)

target_precompile_headers(btAudioTrx PUBLIC ./panel/app_gui_port/gui_port_flags.h)
file(GLOB_RECURSE  bt_audio_trx_panel_srcs  ./panel/*.c)

if(NOT compile_lib_only)
target_sources(btAudioTrx PRIVATE ${bt_audio_trx_panel_srcs})
endif()

endif()
