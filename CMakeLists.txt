set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.15)
project(controllerdriver C)

set(CMAKE_C_FLAGS "-Wall -Wextra -O2")

add_subdirectory(libs/hidapi)
add_subdirectory(libs/libusb)

include_directories(
    $(PROJECT_SOURCE_DIR)/include
    $(PROJECT_SOURCE_DIR)/libs/hidapi/hidapi
    $(PROJECT_SOURCE_DIR)/libs/libusb/libusb
)

add_executable(
    controllerdriver
    src/main.c
)

target_link_libraries(controllerdriver PRIVATE hidapi::hidapi usb-1.0 m)

