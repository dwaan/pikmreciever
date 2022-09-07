cmake_minimum_required(VERSION 3.9)

project(pikmreciever LANGUAGES C CXX)

# for asprintf
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
add_compile_definitions(_GNU_SOURCE)

option(NO_OUTPUT "Disable USB HID output" OFF)

if(NOT HOOK_PATH)
    set(HOOK_PATH "/home/pi/pikmreciever/hook.sh" CACHE STRING "Path to .sh toggle hook")
endif()

add_library(libusbgx INTERFACE)
target_sources(libusbgx INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg_common.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg_common_libconfig.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg_schemes_libconfig.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg_error.c

    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/ether.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/ffs.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/hid.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/loopback.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/midi.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/ms.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/phonet.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/serial.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/uac2.c
)

# disable -Waddress-of-packed-member unaligned pointer warning
set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/ether.c
    PROPERTIES COMPILE_FLAGS
    "-Wno-address-of-packed-member"
)

target_include_directories(libusbgx INTERFACE ${CMAKE_CURRENT_LIST_DIR}/libusbgx/include)
target_include_directories(libusbgx INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

target_compile_definitions(libusbgx INTERFACE HAS_GADGET_SCHEMES)

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/libusbgx/include/usbg/usbg_version.h.in)
    set(USBG_VERSION_HEX "0x00000000")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/libusbgx/include/usbg/usbg_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/usbg_version.h @ONLY)
else()
    message(FATAL_ERROR "Missing usbg_version.h.in. Did you forget to `git submodule update --init`?")
endif()

find_library(libconfig libconfig)
# find_library(hidapi hidapi)

target_link_libraries(libusbgx INTERFACE config)

add_executable(
    pikmreciever
    pikmreciever.c
    include/create-gadget-hid.c
    include/remove-gadget-hid.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    HAS_GADGET_SCHEMES

    HOOK_PATH="${HOOK_PATH}"
)

if(NO_OUTPUT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NO_OUTPUT)
endif()

target_link_libraries(${PROJECT_NAME} libusbgx)
target_link_libraries(${PROJECT_NAME} hidapi-libusb)

install(FILES
    ${CMAKE_CURRENT_LIST_DIR}/hook.sh
    ${CMAKE_CURRENT_LIST_DIR}/pikmreciever.service
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)

install(TARGETS ${PROJECT_NAME} DESTINATION .)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)