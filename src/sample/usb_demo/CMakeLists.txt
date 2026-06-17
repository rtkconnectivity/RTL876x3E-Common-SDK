#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#
file(GLOB_RECURSE  usb_demo_srcs  ./*.c)

file(GLOB_RECURSE  usb_demo_hid_remove_srcs  ./usb/hid/usb_hid_mouse.c)
list(REMOVE_ITEM   usb_demo_srcs ${usb_demo_hid_remove_srcs})

file(GLOB_RECURSE  usb_demo_remove_srcs  ./usb_hid_mouse/*.c)
list(REMOVE_ITEM   usb_demo_srcs ${usb_demo_remove_srcs})

if(CONFIG_REALTEK_USB_AUDIO_APP)
    file(GLOB_RECURSE  usb_demo_key_srcs  ./key/*.c)
    list(REMOVE_ITEM   usb_demo_srcs ${usb_demo_key_srcs})

    file(GLOB_RECURSE  usb_demo_usb_hid_srcs  ./usb_hid/*.c)
    list(REMOVE_ITEM   usb_demo_srcs ${usb_demo_usb_hid_srcs})
else(CONFIG_REALTEK_USB_HID_APP)
    file(GLOB_RECURSE  usb_demo_audio_srcs  ./usb/audio/*.c)
    list(REMOVE_ITEM   usb_demo_srcs ${usb_demo_audio_srcs})

    file(GLOB_RECURSE  usb_demo_usb_audio_srcs  ./usb_audio/*.c)
    list(REMOVE_ITEM   usb_demo_srcs ${usb_demo_usb_audio_srcs})
endif()

HEADER_DIRECTORIES(usb_demo_private_include_dir ${CMAKE_CURRENT_SOURCE_DIR})

if((NOT compile_lib_only) AND CONFIG_REALTEK_USB_AUDIO_APP)
    target_include_directories(usbAudio PUBLIC ${usb_demo_private_include_dir})

    #app preinclude
    target_precompile_headers(usbAudio PUBLIC 
              ${CMAKE_CURRENT_SOURCE_DIR}/app_flags.h)

    target_sources(usbAudio PRIVATE ${usb_demo_srcs})
endif()

if((NOT compile_lib_only) AND CONFIG_REALTEK_USB_HID_APP)
    target_include_directories(usbHID PUBLIC ${usb_demo_private_include_dir})

    #app preinclude
    target_precompile_headers(usbHID PUBLIC 
              ${CMAKE_CURRENT_SOURCE_DIR}/app_flags.h)

    target_sources(usbHID PRIVATE ${usb_demo_srcs})
endif()
