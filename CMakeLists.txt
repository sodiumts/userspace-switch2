set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.15)
project(controllerdriver C)

set(CMAKE_C_FLAGS "-Wall -Wextra -O2")

add_subdirectory(libs/hidapi)
add_subdirectory(libs/libusb)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PipeWire REQUIRED libpipewire-0.3)
pkg_check_modules(SPA REQUIRED libspa-0.2)

include_directories(
    $(PROJECT_SOURCE_DIR)/include
    $(PROJECT_SOURCE_DIR)/libs/hidapi/hidapi
    $(PROJECT_SOURCE_DIR)/libs/libusb/libusb
    ${PipeWire_INCLUDE_DIRS}
    ${SPA_INCLUDE_DIRS}
)

add_executable(
    controllerdriver
    src/main.c
    src/ringbuffer.c
)


target_link_libraries(controllerdriver PRIVATE hidapi::hidapi usb-1.0 m Threads::Threads ${PipeWire_LIBRARIES} ${SPA_LIBRARIES})
target_compile_options(controllerdriver PRIVATE ${PipeWire_CFLAGS_OTHER} ${SPA_CFLAGS_OTHER})

