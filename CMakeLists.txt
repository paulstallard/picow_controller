set(PROJECT       hid_keyboard_demo)
set(PICO_BOARD    pico_w)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})

cmake_minimum_required(VERSION 3.12)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(${PROJECT} C CXX ASM)
pico_sdk_init()
add_executable(${PROJECT}
    hid_keyboard_controller.c
    picow_bt_example_background.c
)
target_include_directories(${PROJECT} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(${PROJECT}
    pico_stdlib
    pico_btstack_ble
    pico_btstack_classic #not needed for BLE only demos
    pico_btstack_cyw43
    pico_cyw43_arch_none
)
pico_add_extra_outputs(${PROJECT})
pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)
