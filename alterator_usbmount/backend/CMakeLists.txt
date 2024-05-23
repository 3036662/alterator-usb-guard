add_executable(usbmount
    main.cpp
    usb_mount.cpp
    dispatcher_impl.cpp
    active_device.cpp
)

target_link_libraries(usbmount PRIVATE lisp_bindings)
target_link_libraries(usbmount PRIVATE SDBusCpp::sdbus-c++)
target_link_libraries(usbmount PRIVATE boost_json)
target_link_libraries(usbmount PRIVATE systemd_dbus)
target_link_libraries(usbmount PRIVATE log_reader)
target_include_directories(usbmount PUBLIC ${CMAKE_SOURCE_DIR}/alterator_bindings)
target_include_directories(usbmount PUBLIC ${CMAKE_SOURCE_DIR}/common)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
include(Format)
Format(usbmount .)
include(ClangTidy)
AddClangTidy(usbmount)
AddCppCheck(usbmount_backend_cppcheck)
add_dependencies(usbmount usbmount_backend_cppcheck)
endif()

install (TARGETS usbmount 
    RUNTIME DESTINATION "/usr/lib/alterator/backend3"
)