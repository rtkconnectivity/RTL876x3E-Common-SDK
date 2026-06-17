#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
message([STATUS] "[APP]ioDemo cmake start build")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
enable_language(ASM)
add_executable(ioDemo " ")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path})
file(GLOB OUT_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/*MP*.bin)
if(NOT "${OUT_FILES}" STREQUAL "")
file(REMOVE ${OUT_FILES})
endif()
endif()

set_target_properties(ioDemo PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
                      OUTPUT_NAME ioDemo)

target_compile_definitions(ioDemo PRIVATE NOT_SUPPORT_UPPERSTACK )
#remove to sdk/bin
target_link_libraries(ioDemo
                      PUBLIC ${lib_rom}
                      PUBLIC halLib
                      PUBLIC sdLib
                      PUBLIC romLib
                      PUBLIC m
                      )
if(compile_lib)

target_link_libraries(ioDemo
                      PUBLIC ${halLib_link}
                      PUBLIC ${sdLib_link}
                      PUBLIC libm.a
                      )
endif()
#preinclude kconfig.h
target_precompile_headers(ioDemo PRIVATE  ${kconfig_header_dir_path}/config.h)
target_include_directories(ioDemo PRIVATE ${kconfig_header_dir_path})

#include directory for board.h config.h
target_include_directories(ioDemo PUBLIC ../)
target_include_directories(ioDemo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ioDemo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc/rtl87x3e)

#io srcs and headers
target_sources(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3e/rtl876x_keyscan.c)
target_sources(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/module/rtc_calendar/rtc_calendar.c)
target_sources(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/module/ir_detect_pulse/ir_detect_pulse.c)

target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/rtl87x3e/platform)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/rtl87x3e/platform/cmsis)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/rtl87x3e/io)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/rtl87x3e/hal)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/rtl87x3e/os)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/rtl87x3e/hal/io)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/hal/power)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/hal/platform)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/hal/io)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/module/rtc_calendar)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../bin/rtl87x3e/upperstack_stamp)
target_include_directories(ioDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/module/ir_detect_pulse)

#sdk/inc/app
target_include_directories(ioDemo PRIVATE ${sdk_app_dir_path})

#boot file
target_sources(ioDemo
               PRIVATE ${realtek_start_up_boot_file}
               PRIVATE ${arm_start_up_boot_file})
set_property(SOURCE ${arm_start_up_boot_file} PROPERTY LANGUAGE C)

#generate version.h for app
set(version_path ${CMAKE_CURRENT_SOURCE_DIR}/../version.h)
if(NOT compile_lib)
generate_version("io_demo_app" ${version_path})
endif()

#set link script app.ld
generate_ldscript("${CMAKE_CURRENT_SOURCE_DIR}/app.ld" ioDemo)

#app link option
set(LINK_FLAGS "-fdata-sections -ffunction-sections -Wl,--gc-sections")
set(EXTRA_LINK_FLAGS "-Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.map,--cref,--no-warn-mismatch -specs=nosys.specs")
#app compile option
target_compile_options(ioDemo PRIVATE
                    -march=armv7-m
                    -mcpu=cortex-m3
                    -mfloat-abi=soft
                    -mthumb -Wall
                    -Wno-parentheses
                    -Wno-unused-variable
                    -fdata-sections
                    -ffunction-sections
                    -std=gnu99)

target_link_options(ioDemo
                    PRIVATE
                    -march=armv7-m
                    -mcpu=cortex-m3
                    -mfloat-abi=soft
                    -mthumb -Wall
                    -Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.map
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

add_custom_command(TARGET  ioDemo POST_BUILD
            WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
            COMMAND ${CMAKE_OBJCOPY} -O ihex   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.elf  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.hex
            COMMAND ${gcc_tool_path}/Hex2Bin.exe  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.hex ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.bin
            COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.bin   /rsa ${gcc_tool_path}/../default_rsa_key.pem
            COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.bin   /mp_ini ${CMAKE_CURRENT_SOURCE_DIR}/../mdk/mp.ini
            COMMAND bash ${gcc_tool_path}/md5_generate.sh ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo_MP.bin ${version_path}
            COMMAND ${debug_table_tool_path}/build_debug_table.exe --debug_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path} --su_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../build --out_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
            COMMAND ${CMAKE_OBJDUMP} -D -S ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.elf > ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/ioDemo.disasm
            )

