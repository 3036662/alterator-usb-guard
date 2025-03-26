cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -pedantic)

project(alterator_usbguard LANGUAGES CXX VERSION 0.1)

find_package(PkgConfig REQUIRED)
find_package(Boost CONFIG REQUIRED)
include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
# dbus + systemd
find_package(sdbus-c++ REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET systemd)



# alterator lisp bingings
add_subdirectory(alterator_bindings)

# common

add_subdirectory(common)



# backend for alterator-usbguard
pkg_check_modules(USBGUARD REQUIRED IMPORTED_TARGET libusbguard)
message("USBGUARD_FOUND: ${USBGUARD_FOUND}")
add_subdirectory(backend)


# only for debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # android vid_pid parser
    message("Building tests...")
    add_subdirectory(android_vidpid_parser)    
    add_compile_definitions(UNIT_TEST=1)
    add_subdirectory(tests)
endif()


# docs
IF (DEFINED BUILD_DOCS)
find_package(Doxygen)
if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")
    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
    message ("Doxygen is required to be installed to generate documentation.")
endif (DOXYGEN_FOUND)
ENDIF (DEFINED BUILD_DOCS)

#cmake -S . -B build  -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ -G Ninja