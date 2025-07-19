cmake_minimum_required(VERSION 3.15)
project(controllerdriver C)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall -Wextra -O2")

include_directories(
    $(PROJECT_SOURCE_DIR)/include
    $(PROJECT_SOURCE_DIR)/libs/hidapi
    $(PROJECT_SOURCE_DIR)/libs/libusb
)


add_executable(
    controllerdriver
    src/main.c
)


