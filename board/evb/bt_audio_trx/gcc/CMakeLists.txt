#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
message([STATUS] "[APP]btAudioTrx cmake start build")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
enable_language(ASM)
add_executable(btAudioTrx " ")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path})
file(GLOB OUT_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/*MP*.bin)
if(NOT "${OUT_FILES}" STREQUAL "")
file(REMOVE ${OUT_FILES})
endif()
endif()

set_target_properties(btAudioTrx PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
                      OUTPUT_NAME btAudioTrx_${build_bank})

#remove to sdk/bin
target_link_libraries(btAudioTrx
						PUBLIC
						-Wl,--start-group
                       ${lib_gap}
                       ${lib_rom}
                       ${lib_upperstack}
                       frameworkLib
                       sysmLib
                       halLib
                       gfpsLib
                       usbLib
                       usbHalLib
                       filesystemLib
                       sdLib
                       bleMgrLib
                       consoleLib
                       remoteLib
                       bleAudioLib
                       romLib
                       gapLib
                       upperstackLib
					   mbedtlsLib
						-Wl,--end-group
                      PUBLIC m
                      )
if(CONFIG_REALTEK_FLASH_MAP_16M)
find_library(
    lib_gui
    NAMES libgui.a
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/sample/bt_audio_trx/panel/gui_lib/RTL8763E
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH
)
target_link_libraries(btAudioTrx
						PUBLIC ${lib_gui}
						)
target_include_directories(btAudioTrx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/sample/bt_audio_trx/panel/gui_lib/include)
endif()
if(compile_lib)
target_link_libraries(btAudioTrx
                      PUBLIC ${mbedtlsLib_link}
                      PUBLIC ${frameworkLib_link}
                      PUBLIC ${sysmLib_link}
                      PUBLIC ${gfpsLib_link}
                      PUBLIC ${usbLib_link}
                      PUBLIC ${usbHalLib_link}
                      PUBLIC ${filesystemLib_link}
                      PUBLIC ${sdLib_link}
                      PUBLIC ${bleMgrLib_link}
                      PUBLIC ${consoleLib_link}
                      PUBLIC ${remoteLib_link}
                      PUBLIC ${bleAudioLib_link}
					  PUBLIC ${halLib_link}
                      PUBLIC libm.a
                      )
endif()
#preinclude kconfig.h
target_precompile_headers(btAudioTrx PRIVATE  ${kconfig_header_dir_path}/config.h)
target_include_directories(btAudioTrx PRIVATE ${kconfig_header_dir_path})

#include directory for board.h config.h
target_include_directories(btAudioTrx PUBLIC ../)
target_include_directories(btAudioTrx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(btAudioTrx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc/rtl87x3e_gcc)
target_include_directories(btAudioTrx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/bluetooth/leaudio)
target_include_directories(btAudioTrx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/framework/sys)
#sdk/inc/app
target_include_directories(btAudioTrx PRIVATE ${sdk_app_dir_path})
target_include_directories(btAudioTrx PRIVATE ../../../../src/sample/mbedtls/include)
target_include_directories(btAudioTrx PRIVATE ../../../../src/sample/mbedtls/library)
target_include_directories(btAudioTrx PRIVATE ../../../../src/sample/mbedtls/tf-psa-crypto/core)
target_include_directories(btAudioTrx PRIVATE ../../../../src/sample/mbedtls/tf-psa-crypto/drivers/builtin/include)
target_include_directories(btAudioTrx PRIVATE ../../../../src/sample/mbedtls/tf-psa-crypto/include)

#boot file
target_sources(btAudioTrx
               PRIVATE ${realtek_start_up_boot_file}
               PRIVATE ${arm_start_up_boot_file})
set_property(SOURCE ${arm_start_up_boot_file} PROPERTY LANGUAGE C)

#generate version.h for app
set(version_path ${CMAKE_CURRENT_SOURCE_DIR}/../version.h)
if(NOT compile_lib)
generate_version("audio_trx_app" ${version_path})
endif()

#set link script app.ld
generate_ldscript("${CMAKE_CURRENT_SOURCE_DIR}/app.ld" btAudioTrx)

#app link option
set(LINK_FLAGS "-fdata-sections -ffunction-sections -Wl,--gc-sections")
set(EXTRA_LINK_FLAGS "-Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.map,--cref,--no-warn-mismatch -specs=nosys.specs")
#app compile option
target_compile_options(btAudioTrx PRIVATE
                    -march=armv7-m
                    -mcpu=cortex-m3
                    -mfloat-abi=soft
                    -mthumb -Wall
                    -Wno-parentheses
                    -Wno-unused-variable
                    -fdata-sections
                    -ffunction-sections
                    -std=gnu99)

target_link_options(btAudioTrx
                    PRIVATE
                    -march=armv7-m
                    -mcpu=cortex-m3
                    -mfloat-abi=soft
                    -mthumb -Wall
                    -Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.map
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

add_custom_command(TARGET  btAudioTrx POST_BUILD
            WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
            COMMAND ${CMAKE_OBJCOPY} -O ihex   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.elf  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.hex
            COMMAND ${gcc_tool_path}/Hex2Bin.exe  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.hex ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.bin
            COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.bin   /rsa ${gcc_tool_path}/../default_rsa_key.pem
            COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.bin   /mp_ini ${CMAKE_CURRENT_SOURCE_DIR}/../mdk/mp.ini
            COMMAND bash ${gcc_tool_path}/md5_generate.sh ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}_MP.bin ${version_path}
            COMMAND ${debug_table_tool_path}/build_debug_table.exe --debug_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path} --su_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../build --out_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
            COMMAND ${CMAKE_OBJDUMP} -D -S ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.elf > ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/btAudioTrx_${build_bank}.disasm
            )

