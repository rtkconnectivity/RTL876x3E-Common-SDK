#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
#HEADER_DIRECTORIES(lib_mbedtls_library_dir ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories(mbedtlsLib ${mbedtlsLib_public_type} ${lib_mbedtls_library_dir})
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/drivers/builtin/include")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/include")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/include/library")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/include/library/tf-psa-crypto")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/drivers")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/drivers/builtin")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/drivers/builtin/include")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/drivers/builtin/mbedtls")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/drivers/builtin/src")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/core")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/include/psa")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/include")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/include/mbedtls")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/library")
target_include_directories(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/port")
if(NOT CONFIG_REALTEK_RTL87X3G_ALIPAY)
target_precompile_headers(mbedtlsLib ${mbedtlsLib_public_type}	"${CMAKE_CURRENT_SOURCE_DIR}/port/mbedtls_config.h")
endif()

file(GLOB_RECURSE  lib_mbedtls_library_sources ${CMAKE_CURRENT_SOURCE_DIR}/library/*.c)
file(GLOB_RECURSE  lib_mbedtls_port_sources ${CMAKE_CURRENT_SOURCE_DIR}/port/*.c)

if(NOT CONFIG_RTL87X3G)
  file(GLOB_RECURSE  lib_mbedtls_port_remove_sources  ${CMAKE_CURRENT_SOURCE_DIR}/port/ecdsa_alt.c )
  list(REMOVE_ITEM   lib_mbedtls_port_sources   ${lib_mbedtls_port_remove_sources})
endif()

target_include_directories(mbedtlsLib ${mbedtlsLib_public_type} "${CMAKE_CURRENT_SOURCE_DIR}/port")
target_sources(mbedtlsLib ${mbedtlsLib_private_type} ${lib_mbedtls_library_sources})
target_sources(mbedtlsLib ${mbedtlsLib_private_type} ${lib_mbedtls_port_sources})
file(GLOB_RECURSE  lib_mbedtls_builtin_sources ${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/drivers/builtin/src/*.c)
file(GLOB_RECURSE  lib_mbedtls_builtin_remove_sources  
										   ${CMAKE_CURRENT_SOURCE_DIR}/tf-psa-crypto/driver/builtin/src/aesce.c
										   )
										   
list(REMOVE_ITEM lib_mbedtls_builtin_sources ${lib_mbedtls_builtin_remove_sources})
target_sources(mbedtlsLib ${mbedtlsLib_private_type} ${lib_mbedtls_builtin_sources})

