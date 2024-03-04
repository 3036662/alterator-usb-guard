cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -pedantic)

project(alterator_usbguard LANGUAGES CXX VERSION 0.1)

find_package(PkgConfig REQUIRED)
pkg_check_modules(USBGUARD REQUIRED IMPORTED_TARGET libusbguard)
message("USBGUARD_FOUND: ${USBGUARD_FOUND}")

find_package(Boost REQUIRED)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# dbus + systemd
find_package(sdbus-c++ REQUIRED)
pkg_check_modules(SYSDTEMD REQUIRED systemd )


# backend
add_subdirectory(backend)

# for now - required only for fstab test
#pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)
#add_executable(test_fstab fstab_test.cpp)
#target_link_libraries(test_fstab PRIVATE PkgConfig::UDEV)
#target_include_directories(test_fstab PUBLIC ${UDEV_INCLUDE_DIRS})

# android vid_pid parser
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_subdirectory(android_vidpid_parser)
endif()

# tests
IF (DEFINED UNIT_TEST)
    message("Building tests...")
    add_compile_definitions(UNIT_TEST=1)
    add_subdirectory(tests)
ENDIF (DEFINED UNIT_TEST)

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

#cmake -S . -B build -DUNIT_TEST=1 -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ -G Ninja