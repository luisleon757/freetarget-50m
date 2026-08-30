cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH environment variable is not set")
endif()

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ft50_pico_single_sensor C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(ft50_pico_single_sensor
    src/main.c
)

target_include_directories(ft50_pico_single_sensor PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(ft50_pico_single_sensor
    pico_stdlib
    hardware_adc
    hardware_dma
    pico_cyw43_arch_lwip_threadsafe_background
)

# Field operation is wireless. USB/UART stdio is intentionally not required.
pico_enable_stdio_usb(ft50_pico_single_sensor 0)
pico_enable_stdio_uart(ft50_pico_single_sensor 0)

pico_add_extra_outputs(ft50_pico_single_sensor)
