#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
#build module
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#environment set up related
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-as)
set(CMAKE_C_COMPILER_AR arm-none-eabi-ar)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_STATIC_LIBRARY_PREFIX "")

set(CONFIG_RTL87X3E ON)
if(CONFIG_RTL87X3D)
	add_compile_options(-g1
						-mcpu=cortex-m4
						-mfpu=fpv4-sp-d16
						-mfloat-abi=hard
						-mthumb -Wall
						-Wno-parentheses
						-Wno-unused-variable
						-Wno-return-type
						-Wno-address-of-packed-member
						-fdata-sections
						-ffunction-sections
						-std=gnu99
						-march=armv8-m
						-fstack-usage
                                                -fno-delete-null-pointer-checks)

elseif(CONFIG_RTL87X3E)
	add_compile_options(-g1
						-O2
						-march=armv7-m
						-mcpu=cortex-m3
						-mfloat-abi=soft
						-mthumb -Wall
						-Wno-parentheses
						-Wno-unused-variable
						-Wno-return-type
						-Wno-address-of-packed-member
						-Wno-unknown-pragmas
						-Wno-format-truncation
						-Wno-format
						-Wno-missing-braces
						-fdata-sections
						-ffunction-sections
						-std=gnu99
						-fstack-usage
                                                -fno-delete-null-pointer-checks)
endif()
set(CMAKE_EXECUTABLE_SUFFIX ".elf")