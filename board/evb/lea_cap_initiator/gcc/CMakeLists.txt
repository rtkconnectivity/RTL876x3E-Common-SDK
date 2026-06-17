#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

message([STATUS] "[APP]leaCAPInitiator cmake start build")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
enable_language(ASM)
add_executable(leaCAPInitiator " ")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path})
file(GLOB OUT_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/*MP*.bin)
if(NOT "${OUT_FILES}" STREQUAL "")
file(REMOVE ${OUT_FILES})
endif()
endif()

set_target_properties(leaCAPInitiator PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
                      OUTPUT_NAME leaCAPInitiator_${build_bank})

#remove to sdk/bin
target_link_libraries(leaCAPInitiator
                      PUBLIC ${lib_gap}
                      PUBLIC ${lib_rom}
                      PUBLIC ${lib_upperstack}
                      PUBLIC sysmLib
                      PUBLIC halLib
                      PUBLIC bleMgrLib
                      PUBLIC bleAudioLib
                      PUBLIC romLib
                      PUBLIC gapLib
                      PUBLIC upperstackLib
                      PUBLIC m
                      )
if(compile_lib)
target_link_libraries(leaCAPInitiator
                      PUBLIC ${sysmLib_link}
                      PUBLIC ${halLib_link}
                      PUBLIC ${bleMgrLib_link}
                      PUBLIC ${consoleLib_link}
                      PUBLIC ${bleAudioLib_link}
                      PUBLIC libm.a
                      )
endif()
#preinclude kconfig.h
target_precompile_headers(leaCAPInitiator PRIVATE  ${kconfig_header_dir_path}/config.h)
target_precompile_headers(leaCAPInitiator PRIVATE  ${kconfig_header_dir_path}/board.h)
target_precompile_headers(leaCAPInitiator PRIVATE  ../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_flags.h)

#include directory for board.h config.h
target_include_directories(leaCAPInitiator PUBLIC ../)
target_include_directories(leaCAPInitiator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(leaCAPInitiator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc/rtl87x3e_gcc)
target_include_directories(leaCAPInitiator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/bluetooth/leaudio)
target_include_directories(leaCAPInitiator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/framework/sys)

target_include_directories(leaCAPInitiator PRIVATE ${kconfig_header_dir_path})

#boot file
target_sources(bleCentral
               PRIVATE ${realtek_start_up_boot_file}
               PRIVATE ${arm_start_up_boot_file})
set_property(SOURCE ${arm_start_up_boot_file} PROPERTY LANGUAGE C)

#boot file

target_sources(leaCAPInitiator PRIVATE 
								../../../../src/mcu/module/data_uart_cmd/data_uart.c
								../../../../src/mcu/module/data_uart_cmd/user_cmd_parse.c
								../../../../src/ble/gatt_client/bt_ext_ftl.c
								../../../../src/ble/gatt_client/gattc_tbl_storage.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_gap.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_main.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_user_cmd.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_profile.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_link.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_csis.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_scan.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_cap.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_bap.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_bap_bsrc.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_audio_data.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_mcp.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_ccp.c
								../../../../src/sample/lea_samples/lea_cap_initiator/app_lea_cap_ini_ecfc.c
								)

#sdk/inc/app
target_include_directories(leaCAPInitiator PRIVATE 
								../../../../src/ble/profile/server
								../../../../src/ble_mgr
								./../../../src/ble/gatt_client
								../../../../src/mcu/module/data_uart_cmd
								../../../../src/sample/lea_samples/lea_cap_initiator
								)

#generate version.h for app
set(version_path ${CMAKE_CURRENT_SOURCE_DIR}/../version.h)
if(NOT compile_lib)
generate_version("lea_cap_initiator_app" ${version_path})
endif()

#set link script app.ld
generate_ldscript("${CMAKE_CURRENT_SOURCE_DIR}/app.ld" leaCAPInitiator)

#app link option
set(LINK_FLAGS "-fdata-sections -ffunction-sections -Wl,--gc-sections")
set(EXTRA_LINK_FLAGS "-Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}.map,--cref,--no-warn-mismatch -specs=nosys.specs")
#app compile option
target_compile_options(leaCAPInitiator PRIVATE
                    -march=armv7-m
                    -mcpu=cortex-m3
                    -mfloat-abi=soft
                    -mthumb -Wall
                    -Wno-parentheses
                    -Wno-unused-variable
                    -fdata-sections
                    -ffunction-sections
                    -std=gnu99)

target_link_options(leaCAPInitiator
                    PRIVATE
                    -march=armv7-m
                    -mcpu=cortex-m3
                    -mfloat-abi=soft
                    -mthumb -Wall
                    -Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}.map
                    -fdata-sections
                    -ffunction-sections
                    -Wl,--gc-sections
                    -Wl,--cref
                    -specs=nosys.specs
                    -Wl,-wrap=malloc
                    -Wl,-wrap=calloc
                    -Wl,-wrap=free
                    -Wl,-wrap=memset_s
                    -Wl,-wrap=memcpy_s)

 
# post build
set(gcc_tool_path ${REALTEK_SDK_BASE_PATH}/tool/Gadgets/gcc_tool)
set(debug_table_tool_path ${REALTEK_SDK_BASE_PATH}/tool/Gadgets/build_debug_table_tool)

add_custom_command(TARGET  leaCAPInitiator POST_BUILD
            WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
            COMMAND ${CMAKE_OBJCOPY} -O ihex   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}.elf  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}.hex
            COMMAND ${gcc_tool_path}/Hex2Bin.exe  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}.hex ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}.bin
            COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}.bin   /rsa ${gcc_tool_path}/../default_rsa_key.pem
            COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/lleaCAPInitiator_${build_bank}.bin   /mp_ini ${CMAKE_CURRENT_SOURCE_DIR}/../mdk/mp.ini
            COMMAND bash ${gcc_tool_path}/md5_generate.sh ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}_MP.bin ${version_path}
            COMMAND ${debug_table_tool_path}/build_debug_table.exe --debug_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path} --su_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../build --out_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
            COMMAND ${CMAKE_OBJDUMP} -D -S ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}.elf > ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/leaCAPInitiator_${build_bank}.disasm
            )

