#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
message([STATUS] "[APP]usbDemo cmake start build")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
enable_language(ASM)

#add usb audio target
if(CONFIG_REALTEK_USB_AUDIO_APP)

    add_executable(usbAudio " ")
    set(build_target_list ${build_target_list} "usbAudio" CACHE INTERNAL "build target list")

    #sort out output path
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path})
        file(GLOB OUT_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/*MP*.bin)
        if(NOT "${OUT_FILES}" STREQUAL "")
            file(REMOVE ${OUT_FILES})
        endif()
    endif()

    set_target_properties(usbAudio PROPERTIES
                          ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
                          OUTPUT_NAME usbAudio_${build_bank})

    target_link_libraries(usbAudio
                          PUBLIC frameworkLib
                          PUBLIC halLib
                          PUBLIC bleMgrLib
                          PUBLIC sysmLib
                          PUBLIC consoleLib
                          PUBLIC remoteLib
                          PUBLIC romLib
                          PUBLIC gapLib
                          PUBLIC upperstackLib
                          PUBLIC m
                          PUBLIC ${lib_gap}
                          PUBLIC ${lib_rom}
                          PUBLIC ${lib_upperstack}
                          )

    if(CONFIG_RTL87X3E OR CONFIG_RTL87X3D)
        target_link_libraries(usbAudio
                          PUBLIC usbLib
                          PUBLIC usbHalLib )
    endif()

    if(compile_lib)
        target_link_libraries(usbAudio
                              PUBLIC ${frameworkLib_link}
                              PUBLIC ${halLib_link}
                              PUBLIC ${bleMgrLib_link}
                              PUBLIC ${sysmLib_link}
                              PUBLIC ${consoleLib_link}
                              PUBLIC ${remoteLib_link}
                              PUBLIC libm.a)
        if(CONFIG_RTL87X3E OR CONFIG_RTL87X3D)
            target_link_libraries(usbAudio
                            PUBLIC ${usbLib_link}
                            PUBLIC ${usbHalLib_link})
        endif()
    endif()

    #io_driver srcs and headers
    if(CONFIG_RTL87X3D)
        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_rcc.c)
        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_pinmux.c)
        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_tim.c)
        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_gdma.c)
        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_uart.c)
        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_gpio.c)
        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_i2c.c)
    endif()

    #io_hal srcs and headers
#    target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common/hw_tim.c)
#    target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common/hal_gpio.c)
#    target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common/hal_pinmux.c)
#    if(CONFIG_RTL87X3E)
#        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/rtl87x3e/hal_gpio_int.c)
#    elseif(CONFIG_RTL87X3D)
#        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/rtl87x3d/hal_gpio_int.c)
#        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common/hal_dma_internal.c)
#        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common/hal_i2c.c)
#    endif()
#    target_include_directories(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common)
    
    #buck srcs and headers
    if(CONFIG_RTL87X3D)
        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/module/buck/app_buck_apw7564.c)
        target_sources(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/module/buck/app_buck_tps62860.c)
        target_include_directories(usbAudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/module/buck)
    endif()

    #preinclude Kconfig.h
    target_precompile_headers(usbAudio PRIVATE ${kconfig_header_dir_path}/config.h)
    target_include_directories(usbAudio PRIVATE ${kconfig_header_dir_path})

    #include directory for board.h config.h
    target_include_directories(usbAudio PUBLIC ../)
    target_include_directories(usbAudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    if(CONFIG_RTL87X3E)
        target_include_directories(usbAudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc/rtl87x3e)
    elseif(CONFIG_RTL87X3D)
        target_include_directories(usbAudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc/rtl87x3d)
    endif()

    #sdk/inc/usb
    target_include_directories(usbAudio PRIVATE ${REALTEK_SDK_BASE_PATH}/inc/usb)
    
    if(CONFIG_RTL87X3D)
        target_compile_definitions(usbAudio PUBLIC USB_DMA_RAM_TYPE=0)
    endif()

    #app boot file: start_up.s rtl87x3_system.c
    target_sources(usbAudio
                   PRIVATE ${realtek_start_up_boot_file}
                   PRIVATE ${arm_start_up_boot_file})
    set_property(SOURCE ${arm_start_up_boot_file} PROPERTY LANGUAGE C)

    #generate version.h for app
    set(version_path ${CMAKE_CURRENT_SOURCE_DIR}/../version.h)
    generate_version("usb_audio_app" ${version_path})

    #set link script app.ld
    if(CONFIG_RTL87X3E)
        generate_ldscript("${CMAKE_CURRENT_SOURCE_DIR}/app_audio_rtl87x3e.ld" usbAudio)
    elseif(CONFIG_RTL87X3D)
        generate_ldscript("${CMAKE_CURRENT_SOURCE_DIR}/app_audio_rtl87x3d.ld" usbAudio)
    endif()

    #app link option
    set(LINK_FLAGS "-fdata-sections -ffunction-sections -Wl,--gc-sections")
    set(EXTRA_LINK_FLAGS "-Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.map,--cref,--no-warn-mismatch -specs=nosys.specs")
    #app compile option
    target_compile_options(usbAudio PRIVATE
                        -march=armv7-m
                        -mcpu=cortex-m3
                        -mfloat-abi=soft
                        -mthumb -Wall
                        -Wno-parentheses
                        -Wno-unused-variable
                        -fdata-sections
                        -ffunction-sections
                        -std=gnu99)

    target_link_options(usbAudio
                        PRIVATE
                        -march=armv7-m
                        -mcpu=cortex-m3
                        -mfloat-abi=soft
                        -mthumb -Wall
                        -Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.map
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

    add_custom_command(TARGET  usbAudio POST_BUILD
                WORKING_DIRECTORY  ${CMAKE_CURRENT_LIST_DIR}/bin/${bin_output_path}
                COMMAND ${CMAKE_OBJCOPY} -O ihex   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.elf  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.hex
                COMMAND ${gcc_tool_path}/Hex2Bin.exe  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.hex ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.bin
                COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.bin    /rsa ${gcc_tool_path}/../default_rsa_key.pem
                COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.bin   /mp_ini ${CMAKE_CURRENT_SOURCE_DIR}/../mdk/mp.ini
                COMMAND bash ${gcc_tool_path}/md5_generate.sh ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}_MP.bin ${version_path}
                COMMAND ${debug_table_tool_path}/build_debug_table.exe --debug_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path} --su_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../build --out_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
                COMMAND ${CMAKE_OBJDUMP} -D -S ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.elf > ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbAudio_${build_bank}.disasm
                )

endif()

#add usb hid target
if(CONFIG_REALTEK_USB_HID_APP)

    add_executable(usbHID " ")
    set(build_target_list ${build_target_list} "usbHID" CACHE INTERNAL "build target list")

    #sort out output path
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path})
        file(GLOB OUT_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/*MP*.bin)
        if(NOT "${OUT_FILES}" STREQUAL "")
            file(REMOVE ${OUT_FILES})
        endif()
    endif()

    set_target_properties(usbHID PROPERTIES
                          ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
                          OUTPUT_NAME usbHID_${build_bank})

    target_link_libraries(usbHID
                          PUBLIC frameworkLib
                          PUBLIC halLib
                          PUBLIC bleMgrLib
                          PUBLIC sysmLib
                          PUBLIC consoleLib
                          PUBLIC remoteLib
                          PUBLIC romLib
                          PUBLIC gapLib
                          PUBLIC upperstackLib
                          PUBLIC m
                          PUBLIC ${lib_gap}
                          PUBLIC ${lib_rom}
                          PUBLIC ${lib_upperstack}
                          )
    if(CONFIG_RTL87X3E OR CONFIG_RTL87X3D)
        target_link_libraries(usbHID
                          PUBLIC usbLib
                          PUBLIC usbHalLib )
    endif()

    if(compile_lib)
        target_link_libraries(usbHID
                              PUBLIC ${frameworkLib_link}
                              PUBLIC ${halLib_link}
                              PUBLIC ${bleMgrLib_link}
                              PUBLIC ${sysmLib_link}
                              PUBLIC ${consoleLib_link}
                              PUBLIC ${remoteLib_link}
                              PUBLIC libm.a)
        if(CONFIG_RTL87X3E OR CONFIG_RTL87X3D)
            target_link_libraries(usbHID
                            PUBLIC ${usbLib_link}
                            PUBLIC ${usbHalLib_link})
        endif()
    endif()

    #io_driver srcs and headers
    if(CONFIG_RTL87X3D)
        target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_rcc.c)
        target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_pinmux.c)
        target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_tim.c)
        target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_gdma.c)
        target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_uart.c)
        target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io/rtl87x3d/rtl876x_gpio.c)
    endif()

    #io_hal srcs and headers
#    target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common/hw_tim.c)
#    target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common/hal_gpio.c)
#    target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common/hal_pinmux.c)
#    if(CONFIG_RTL87X3E)
#        target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/rtl87x3e/hal_gpio_int.c)
#    elseif(CONFIG_RTL87X3D)
#        target_sources(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/rtl87x3d/hal_gpio_int.c)
#    endif()
#    target_include_directories(usbHID PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/mcu/io_hal/common)

    #preinclude Kconfig.h
    target_precompile_headers(usbHID PRIVATE ${kconfig_header_dir_path}/config.h)
    target_include_directories(usbHID PRIVATE ${kconfig_header_dir_path})

    #include directory for board.h config.h
    target_include_directories(usbHID PUBLIC ../)
    target_include_directories(usbHID PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    if(CONFIG_RTL87X3E)
        target_include_directories(usbHID PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc/rtl87x3e)
    elseif(CONFIG_RTL87X3D)
        target_include_directories(usbHID PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc/rtl87x3d)
    endif()

    #sdk/inc/usb
    target_include_directories(usbHID PRIVATE ${REALTEK_SDK_BASE_PATH}/inc/usb)
    
    if(CONFIG_RTL87X3D)
        target_compile_definitions(usbHID PUBLIC USB_DMA_RAM_TYPE=0)
    endif()

    #app boot file: start_up.s rtl87x3_system.c
    target_sources(usbHID
                   PRIVATE ${realtek_start_up_boot_file}
                   PRIVATE ${arm_start_up_boot_file})
    set_property(SOURCE ${arm_start_up_boot_file} PROPERTY LANGUAGE C)

    #generate version.h for app
    set(version_path ${CMAKE_CURRENT_SOURCE_DIR}/../version.h)
    generate_version("usb_hid_app" ${version_path})

    #set link script app.ld
    if(CONFIG_RTL87X3E)
        generate_ldscript("${CMAKE_CURRENT_SOURCE_DIR}/app_hid_rtl87x3e.ld" usbHID)
    elseif(CONFIG_RTL87X3D)
        generate_ldscript("${CMAKE_CURRENT_SOURCE_DIR}/app_hid_rtl87x3d.ld" usbHID)
    endif()

    #app link option
    set(LINK_FLAGS "-fdata-sections -ffunction-sections -Wl,--gc-sections")
    set(EXTRA_LINK_FLAGS "-Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.map,--cref,--no-warn-mismatch -specs=nosys.specs")
    #app compile option
    target_compile_options(usbHID PRIVATE
                        -march=armv7-m
                        -mcpu=cortex-m3
                        -mfloat-abi=soft
                        -mthumb -Wall
                        -Wno-parentheses
                        -Wno-unused-variable
                        -fdata-sections
                        -ffunction-sections
                        -std=gnu99)

    target_link_options(usbHID
                        PRIVATE
                        -march=armv7-m
                        -mcpu=cortex-m3
                        -mfloat-abi=soft
                        -mthumb -Wall
                        -Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.map
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

    add_custom_command(TARGET  usbHID POST_BUILD
                WORKING_DIRECTORY  ${CMAKE_CURRENT_LIST_DIR}/bin/${bin_output_path}
                COMMAND ${CMAKE_OBJCOPY} -O ihex   ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.elf  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.hex
                COMMAND ${gcc_tool_path}/Hex2Bin.exe  ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.hex ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.bin
                COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.bin    /rsa ${gcc_tool_path}/../default_rsa_key.pem
                COMMAND ${gcc_tool_path}/prepend_header.exe /app_code ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.bin   /mp_ini ${CMAKE_CURRENT_SOURCE_DIR}/../mdk/mp.ini
                COMMAND bash ${gcc_tool_path}/md5_generate.sh ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}_MP.bin ${version_path}
                COMMAND ${debug_table_tool_path}/build_debug_table.exe --debug_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path} --su_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../build --out_dir ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}
                COMMAND ${CMAKE_OBJDUMP} -D -S ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.elf > ${CMAKE_CURRENT_SOURCE_DIR}/bin/${bin_output_path}/usbHID_${build_bank}.disasm
                )

endif()